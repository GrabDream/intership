<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = "m_sysmaintance";
include_once ($_SERVER["DOCUMENT_ROOT"] . "/authenticed_writable.php");
	$ret = ace_config_file_save(4000);
	if($ret != 0)
	{		
		include_once($_SERVER["DOCUMENT_ROOT"]."/model/lan.php");
		echo '<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />';
		echo "<script type='text/javascript'>alert('"._gettext('fail')."!');location.href='configmanage.php';</script>";
		exit();
	}
	$filename = ace_config_checkout();
	$savename = substr($filename,(strrpos($filename,'/')+1));
	$file = fopen($filename,"r");
	header( "Pragma: public" );
	header( "Expires: 0" ); // set expiration time
	header( "Cache-Component: must-revalidate, post-check=0, pre-check=0" );
	Header("Accept-Ranges: bytes");
	header("Content-Type: text/plain");
	header("Content-Length: " . filesize($filename));

	header( "Content-Disposition: attachment; filename=".urlencode($savename) );
	header( 'Content-Transfer-Encoding: binary' );
	fpassthru($file);
	fclose($file);
	
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticate.php");
$auth = new authentication();
if(! $auth->isWritable('m_sysmaintance'))
{
	jumpTo("","/login.php");
}
	$log = array();
	$log['content'] = array();
	$log['content'][0] = _gettext('ConfigureBak&re');
	$log['content'][1] = _gettext("confbakcmt");
	$log['result'] = _gettext('Success');
	saveLog($log);		

	exit();
?>
