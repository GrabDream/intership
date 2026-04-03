<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticate.php");
$auth = new authentication();
if($_SESSION['configsave']!=1)
{
	jumpTo("","/login.php");
	exit();
}

$ret = ace_config_file_save(4000);

if($ret == 0)
	$msg =_gettext("saveConfigureSuccess")."! ";
else
	$msg = _gettext("saveConfigureFail")."($ret)! ";

$log = array();
$log['content'] = array();
$log['content'][0] = _gettext('saveConfg');
if ($ret==0)
{
	$log['result'] = _gettext('Success');
}
else
{
	$log['result'] = _gettext('fail');
	$log['ret'] = $msg;
}
saveLog($log);	

echo $msg;
exit();
?>
