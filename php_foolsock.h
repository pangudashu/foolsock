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

#ifndef PHP_FOOLSOCK_H
#define PHP_FOOLSOCK_H

extern zend_module_entry foolsock_module_entry;
#define phpext_foolsock_ptr &foolsock_module_entry

#define PHP_FOOLSOCK_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_FOOLSOCK_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_FOOLSOCK_API __attribute__ ((visibility("default")))
#else
#	define PHP_FOOLSOCK_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION >= 4)
#    define FOOLSOCK_LIST_INSERT(item, list) zend_list_insert(item, list TSRMLS_CC);
#else
#    define FOOLSOCK_LIST_INSERT(item, list) zend_list_insert(item, list);
#endif

PHP_MINIT_FUNCTION(foolsock);
PHP_MSHUTDOWN_FUNCTION(foolsock);
PHP_RINIT_FUNCTION(foolsock);
PHP_RSHUTDOWN_FUNCTION(foolsock);
PHP_MINFO_FUNCTION(foolsock);
PHP_METHOD(foolsock,__construct);
PHP_METHOD(foolsock,pconnect);
PHP_METHOD(foolsock,read);
PHP_METHOD(foolsock,write);
PHP_METHOD(foolsock,pclose);

#ifdef ZTS
#define FOOLSOCK_G(v) TSRMG(foolsock_globals_id, zend_foolsock_globals *, v)
#else
#define FOOLSOCK_G(v) (foolsock_globals.v)
#endif

#endif	/* PHP_FOOLSOCK_H */

#define CLASS_PROPERTY_RESOURCE "_resource"

typedef struct _foolsock_s{
	php_stream* stream;
	char*       host;
	int         port;
	long	    timeoutms;
	int		    status;
}foolsock_t;

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
