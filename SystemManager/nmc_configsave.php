<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticate.php");
$auth = new authentication('m_sysmaintance');
if(! $auth->isWritable())
{
	jumpTo("","/login.php");
}

$ret = ace_config_file_save(4000);
$ret2 = nmc_mf_save();

if($ret == 0 && $ret2 == 0)
	$msg =_gettext("saveConfigureSuccess")."! ";
else if($ret)
	$msg = _gettext("saveConfigureFail")."($ret)! ";
else
	$msg = _gettext("saveConfigureFail")."($ret2)! ";

$log = array();
$log['content'] = array();
$log['content'][0] = _gettext('saveConfg');
if ($ret == 0 && $ret2 == 0)
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
