/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_network.h"
#include "php_foolsock.h"

#define PHP_FOOLSOCK_NAME "foolsock persistent connection"

static int le_foolsock;

zend_class_entry *foolsock_ce;

/* {{{ foolsock_functions[]
 */
const zend_function_entry foolsock_functions[] = {
	PHP_FE_END	/* Must be the last line in foolsock_functions[] */
};
/* }}} */

/*{{{ foolsock_methods[]
 */
const zend_function_entry foolsock_methods[] = {
	ZEND_ME(foolsock,__construct,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(foolsock,pconnect,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(foolsock,read,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(foolsock,write,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(foolsock,pclose,NULL,ZEND_ACC_PUBLIC)
	PHP_FE_END	/* Must be the last line in foolconf_functions[] */
};
/*}}}*/

/* {{{ foolsock_module_entry
 */
zend_module_entry foolsock_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"foolsock",
	foolsock_functions,
	PHP_MINIT(foolsock),
	PHP_MSHUTDOWN(foolsock),
	PHP_RINIT(foolsock),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(foolsock),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(foolsock),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_FOOLSOCK_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FOOLSOCK
ZEND_GET_MODULE(foolsock)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("foolsock.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_foolsock_globals, foolsock_globals)
    STD_PHP_INI_ENTRY("foolsock.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_foolsock_globals, foolsock_globals)
PHP_INI_END()
*/
/* }}} */

/*{{{ static foolsock_t* create_new_resource(char* host, int port TSRMLS_DC)
 */
static foolsock_t* create_new_resource(char* host, int port TSRMLS_DC)
{
	int host_len = strlen(host);
	foolsock_t* resource = (foolsock_t*)pemalloc(sizeof(foolsock_t),1);
	if(resource == NULL){
		return NULL;
	}
	memset(resource, 0, sizeof(*resource));

	resource->host = pemalloc(host_len + 1, 1);
	memcpy(resource->host, host, host_len);
	resource->host[host_len] = '\0';
	resource->port = port;

	return resource;
}
/*}}}*/

/*{{{ static void foolsock_free(foolsock_t* fs TSRMLS_DC)
 */
static void foolsock_free(foolsock_t* fs TSRMLS_DC)
{
	if(fs->stream != NULL){
		php_stream_pclose(fs->stream);
	}
	pefree(fs->host,1);
	pefree(fs,1);
}
/*}}}*/

/*{{{ static struct timeval convert_timeoutms_to_ts(long msecs)
 */
static struct timeval convert_timeoutms_to_ts(long msecs)
{
	struct timeval tv;
	int secs = 0;

	secs = msecs / 1000;
	tv.tv_sec = secs;
	tv.tv_usec = ((msecs - (secs * 1000)) * 1000) % 1000000;
	return tv;
}
/*}}}*/

/*{{{ static int get_stream(foolsock_t* f_obj TSRMLS_DC)
 */
static int get_stream(foolsock_t* f_obj TSRMLS_DC)
{
	char* hash_key;

	spprintf(&hash_key, 0, "foolsock:%s:%d", f_obj->host,f_obj->port);

	switch(php_stream_from_persistent_id(hash_key, &(f_obj->stream) TSRMLS_CC)) {
	case PHP_STREAM_PERSISTENT_SUCCESS:
		if (php_stream_eof(f_obj->stream)) {
			php_stream_pclose(f_obj->stream);
			f_obj->stream = NULL;
			break;
		}
	case PHP_STREAM_PERSISTENT_FAILURE:
		break;
	}

	struct timeval tv = convert_timeoutms_to_ts(f_obj->timeoutms);

	if(!f_obj->stream){
		int socktype = SOCK_STREAM;
		f_obj->stream = php_stream_sock_open_host(f_obj->host, f_obj->port, socktype, &tv, hash_key);
	}
	efree(hash_key);

	if(!f_obj->stream){
		return 0;
	}

	php_stream_auto_cleanup(f_obj->stream);
	php_stream_set_option(f_obj->stream, PHP_STREAM_OPTION_READ_TIMEOUT, 0, &tv);
	php_stream_set_option(f_obj->stream, PHP_STREAM_OPTION_WRITE_BUFFER, PHP_STREAM_BUFFER_NONE, NULL);
	php_stream_set_chunk_size(f_obj->stream, 8192);

	return 1;
}
/*}}}*/

/*{{{ public function FoolSock::__construct(string $host, string $port)
 */
PHP_METHOD(foolsock,__construct)
{
	long port;
	char* host;
	int host_len;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"sl|l",&host,&host_len,&port) == FAILURE){
		RETURN_FALSE;
	}

	char* hash_key;
	int hash_key_len;
	int resource_id;
	int re_conn = 0;
	zend_rsrc_list_entry *le;
	foolsock_t* f_obj = NULL;

	hash_key_len = spprintf(&hash_key, 0,"foolsock_connect:%s:%d", host,port);
	if(zend_hash_find(&EG(persistent_list), hash_key,hash_key_len + 1, (void **)&le) == FAILURE)
	{
		f_obj = create_new_resource(host,port TSRMLS_CC);
		if(NULL == f_obj){
			RETURN_FALSE;
		}

		zend_rsrc_list_entry new_le;
		new_le.type = le_foolsock;
		new_le.ptr  = f_obj;

		if(zend_hash_update(&EG(persistent_list),hash_key, hash_key_len + 1,(void*)&new_le, sizeof(zend_rsrc_list_entry), NULL) == FAILURE){
			foolsock_free(f_obj TSRMLS_CC);
		}else{
			//add to EG(regular_list)
			resource_id = FOOLSOCK_LIST_INSERT(f_obj,le_foolsock);
		}
		re_conn = 1;
	}else if(le->type != le_foolsock || le->ptr == NULL){
		zend_hash_del(&EG(persistent_list), hash_key, hash_key_len+1);

		f_obj = create_new_resource(host,port TSRMLS_CC);
		if(NULL == f_obj){
			RETURN_FALSE;
		}

		zend_rsrc_list_entry new_le;
		new_le.type = le_foolsock;
		new_le.ptr  = f_obj;

		if(zend_hash_update(&EG(persistent_list),hash_key, hash_key_len + 1,(void*)&new_le, sizeof(zend_rsrc_list_entry), NULL) == FAILURE){
			foolsock_free(f_obj TSRMLS_CC);
		}else{
			resource_id = FOOLSOCK_LIST_INSERT(f_obj,le_foolsock);
		}
		re_conn = 1;
	}else{
		f_obj = (foolsock_t*)le->ptr;
		resource_id = FOOLSOCK_LIST_INSERT(f_obj,le_foolsock);
	}

	efree(hash_key);
	add_property_resource(getThis(),CLASS_PROPERTY_RESOURCE,resource_id);
}
/*}}}*/

/*{{{ public function FoolSock::pconnect([int $timeoutms])
 */
PHP_METHOD(foolsock,pconnect)
{
	zval* resource;
	int resource_type;
	foolsock_t* f_obj;
	long timeoutms = 0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"|l",&timeoutms) == FAILURE){
		RETURN_FALSE;
	}

	resource = zend_read_property(foolsock_ce,getThis(),ZEND_STRL(CLASS_PROPERTY_RESOURCE),1 TSRMLS_CC);
	if(resource == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource");
		RETURN_FALSE;
	}

	f_obj = (foolsock_t*)zend_list_find(Z_LVAL_P(resource),&resource_type TSRMLS_CC);

	if(f_obj == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource");
		RETURN_FALSE;
	}

	if(resource_type != le_foolsock){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource Type");
		RETURN_FALSE;
	}
	f_obj->timeoutms = timeoutms;
	
	int stream_r = get_stream(f_obj TSRMLS_CC);
	if(!stream_r){
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/*}}}*/

/*{{{ public function FoolSock::write(string $msg)
 */
PHP_METHOD(foolsock,write)
{
	zval* resource;
	foolsock_t* f_obj;
	char* msg;
	int msg_len,res,resource_type;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s",&msg,&msg_len) == FAILURE){
		RETURN_FALSE;
	}

	resource = zend_read_property(foolsock_ce,getThis(),ZEND_STRL(CLASS_PROPERTY_RESOURCE),1 TSRMLS_CC);
	if(resource == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource");
		RETURN_FALSE;
	}

	f_obj = (foolsock_t*)zend_list_find(Z_LVAL_P(resource),&resource_type TSRMLS_CC);

	if(f_obj == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource");
		RETURN_FALSE;
	}

	if(resource_type != le_foolsock){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource Type");
		RETURN_FALSE;
	}

	if(f_obj->stream == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Socket Not Connected");
		RETURN_FALSE;
	}

	res = php_stream_write(f_obj->stream,msg,msg_len);
	if(res != msg_len){
		RETURN_FALSE;
	}
	RETURN_LONG(res);
}
/*}}}*/

/*{{{ public function FoolSock::read(int $size)
 */
PHP_METHOD(foolsock,read)
{
	long size;
	foolsock_t* f_obj;
	char* response_buf;
	zval* resource;
	int resource_type;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"l",&size) == FAILURE){
		RETURN_FALSE;
	}

	resource = zend_read_property(foolsock_ce,getThis(),ZEND_STRL(CLASS_PROPERTY_RESOURCE),1 TSRMLS_CC);
	if(resource == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource");
		RETURN_FALSE;
	}

	f_obj = (foolsock_t*)zend_list_find(Z_LVAL_P(resource),&resource_type TSRMLS_CC);

	if(f_obj == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource");
		RETURN_FALSE;
	}

	if(resource_type != le_foolsock){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Resource Type");
		RETURN_FALSE;
	}
	
	if(f_obj->stream == NULL){
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Socket Not Connected");
		RETURN_FALSE;
	}

	response_buf = emalloc(size + 1);

	int r = php_stream_read(f_obj->stream,response_buf,size);
	if(r <= 0){
		if(errno == EAGAIN || errno == EINPROGRESS){
			RETURN_TRUE;
		}
		RETURN_FALSE;
	}
	response_buf[r] = '\0';
	RETURN_STRINGL(response_buf,r,0);
}
/*}}}*/

/*{{{ public function FoolSock::pclose()
 */
PHP_METHOD(foolsock,pclose)
{
	zval* resource;
	int resource_type,hash_key_len;
	char* hash_key;
	foolsock_t* f_obj;

	resource = zend_read_property(foolsock_ce,getThis(),ZEND_STRL(CLASS_PROPERTY_RESOURCE),1 TSRMLS_CC);
	if(resource == NULL){
		RETURN_TRUE;
	}
	
	f_obj = (foolsock_t*)zend_list_find(Z_LVAL_P(resource),&resource_type TSRMLS_CC);
	if(f_obj == NULL){
		RETURN_TRUE;
	}

	if(resource_type != le_foolsock){
		RETURN_TRUE;
	}

	if(f_obj->stream != NULL){
		php_stream_pclose(f_obj->stream);
		f_obj->stream = NULL;
	}
	
	RETURN_TRUE;
}
/*}}}*/

/*{{{ static void foolsock_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
 */
static void foolsock_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	foolsock_t* f_obj = (foolsock_t*)rsrc->ptr;

	foolsock_free(f_obj TSRMLS_CC);
}
/*}}}*/

/*{{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(foolsock)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, "FoolSock",foolsock_methods);

	foolsock_ce = zend_register_internal_class(&ce TSRMLS_CC);

	le_foolsock = zend_register_list_destructors_ex(NULL,foolsock_dtor,PHP_FOOLSOCK_NAME,module_number);

	return SUCCESS;
}
/*}}}*/

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(foolsock)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(foolsock)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(foolsock)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(foolsock)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "foolsock support", "enabled");
	php_info_print_table_end();
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
