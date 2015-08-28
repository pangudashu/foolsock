<?php
//$memcache = new FoolSockMemcache('127.0.0.1',11211);
$memcache = new FoolSockMemcache('10.147.8.139',11211);

$memcache->Add("foolsock_memcache_test_key","This is a socket connect to memcached by foolsock~",3600);

$start = time();
for($i = 0;$i < 100000;$i++){
	$begin_time = microtime(true);
	$value = $memcache->Get("foolsock_memcache_test_key");
	if(false == $value){
		echo "error to get\n";
	}
	echo $i,"->[",$value,"] [time : ",(microtime(true) - $begin_time)*1000," ms]\n";
}

$total_time = time()-$start;
echo "total time : ",$total_time," qps : ",100000/$total_time," time per request : ",($total_time/100000)*1000,"[ms]\n";


class FoolSockMemcache
{
	private $sock = null;

	public function __construct($host = '127.0.0.1',$port = 11211)
	{
		$this->sock = new FoolSock($host,$port);
		$this->sock->pconnect(100);
	}

	public function Get($key)
	{/*{{{*/
		$header = $this->setHeader(0x80,0x00,strlen($key),0x00,0x00,0x0000,strlen($key),0x00000000);

		/*request body*/
		$body = pack("a*",$key);
		$data = $header.$body;

		$write_res = $this->send($data);
		if(false === $write_res){
			return false;
		}

		//read response header
		$_response_header = $this->read(24);
		if(false === $_response_header){
			return false;
		}
		$response_header = $this->parseHeader($_response_header);

		//go on read response body
		$body = $this->read($response_header['body_len']);
		if(false === $body){
			return false;
		}
		if($response_header['status'] != 0){
			return $body;
		}else{
			return substr($body,4);
		}
	}/*}}}*/

	public function Add($key,$value,$expire = 0)
	{/*{{{*/
		/*request header*/
		$header = $this->setHeader(0x80,0x01,strlen($key),0x08,0x00,0x0000,strlen($key)+strlen($value)+8,0x00000000);
		/*request body*/
		$body['extra_flag']    = pack("N",0xdeadbeef);
		$body['extra_expirte'] = pack("N",$expire);
		$body['key']           = pack("a*",$key);
		$body['value']         = pack("a*",$value);
		$body = implode("",$body);

		$data = $header.$body;

		$write_res = $this->send($data);
		if(false === $write_res){
			return false;
		}
		//read response header
		$_response_header = $this->read(24);
		if(false === $_response_header){
			return false;
		}
		$response_header = $this->parseHeader($_response_header);

		if($response_header['status'] == 0){
			return true;
		}
		//go on read response body
		$body = $this->read($response_header['body_len']);
		if(false === $body){
			return false;
		}
		return $body;
	}/*}}}*/

	public function setHeader($magic,$opcode,$key_len,$extra_len = 0,$data_type = 0,$status = 0,$body_len,$opaque)
	{/*{{{*/
		$header['magic']     = pack("C",$magic);
		$header['opcode']    = pack("C",$opcode);
		$header['ken_len']   = pack("n",$key_len);
		$header['extra_len'] = pack("C",$extra_len);
		$header['date_type'] = pack("C",$data_type);
		$header['status']    = pack("n",$status);
		$header['body_len']  = pack("N",$body_len);
		$header['opaque']    = pack("N",$opaque);
		$header['cas']       = pack("C8",0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

		return implode("",$header);
	}/*}}}*/

	public function parseHeader($response_header)
	{/*{{{*/
		$res_header['magic']     = unpack("C",substr($response_header,0,1));
		$res_header['opcode']    = unpack("C",substr($response_header,1,1));
		$res_header['ken_len']   = unpack("n",substr($response_header,2,2));
		$res_header['extra_len'] = unpack("C",substr($response_header,4,1));
		$res_header['date_type'] = unpack("C",substr($response_header,5,1));
		$res_header['status']     = unpack("n",substr($response_header,6,2));
		$res_header['body_len']  = unpack("N",substr($response_header,8,4));
		$res_header['opaque']    = unpack("N",substr($response_header,12,4));

		foreach($res_header as &$v){
			$v = $v[1];
		}
		return $res_header;
	}/*}}}*/

	public function send($msg)
	{/*{{{*/
		$start = microtime(true)*1000;
		do{
			$res = $this->sock->write($msg);
			if($res != false && strlen($msg) == $res){
				break;
			}
			$now = microtime(true)*1000;
			if($now - $start > 2000){
				return false;
			}else{
				//reconnect
				$this->sock->pclose();
				$this->sock->pconnect(100);
			}
		}while(1);
		return true;
	}/*}}}*/

	public function read($bytes)
	{/*{{{*/
		if($bytes <= 0){
			return true;
		}
		$d = '';
		$start = microtime(true)*1000;
		while(1){
			$d .= $this->sock->read($bytes);
			if(strlen($d) == $bytes){
				return $d;
			}
			if(false === $d){
				return false;
			}
			$now = microtime(true)*1000;
			if($now - $start > 2000){
				return false;
			}
		}
	}/*}}}*/


}



