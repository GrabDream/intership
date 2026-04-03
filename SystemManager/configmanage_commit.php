<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = "m_sysmaintance";
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed_writable.php");
include_once($_SERVER["DOCUMENT_ROOT"]."/model/firewall_interface_functions.php");
// $auth = new authentication();
// if(! $auth->isWritable('m_sysmaintance'))
// {
// 	jumpTo("","/login.php");
// }

$destination_folder = "/home/checkin/";
$_FILES['currentcfgfile']["name"] = "Config.conf";

function upload(&$errorInfo)
{
	global $destination_folder;

	$file = $_FILES['currentcfgfile'];
	$filetmpname=$file["tmp_name"];
	if(!is_uploaded_file($filetmpname))
	{
		$errorInfo =  _gettext("uploadFail")."! ";
		return false;
	}

	if(uploadFileTypeAllow($file,2)) {
	    $errorInfo = _gettext ( "uploadFail" ) . "! ";
	    return false;
	}
	$destination = $destination_folder.$file["name"];
	//echo 'de:'.$destination;
	if(!file_exists($destination_folder))
		mkdir($destination_folder);
	if(!move_uploaded_file ($filetmpname, $destination))
	{
		$errorInfo = _gettext("writefailed")."(move fail)! ";
		return false;
	}

	return true;
}

function JumpUrl($url,$msg)
{
	$alertMsg = ($msg)? ("alert('".$msg."');"):"";
	echo <<<EOT
			<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
			<html xmlns="http://www.w3.org/1999/xhtml">
				<head>
				<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
				<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
				<script type="text/javascript" src="/js/prototype.js"></script>
				<script type="text/javascript" src="/js/base.js"></script>
				<script type="text/javascript">
				$alertMsg
				window.location.href = "$url";
				</script>
				</head>
				<body>
				</body>
			</html>
EOT;

	exit;
}

function delete_syslog_file() {
	$logTypeFiles = array('command'=>"/var/log/command_log.txt",'event'=>"/var/log/event_log.txt",
	'auth'=>"/var/log/authentic_log.txt",'blacklist'=>"/var/log/blacklist_log.txt",
	'pptp'=>"/var/log/pptpd.log",'security'=>"/var/log/security_log.txt");
	foreach($logTypeFiles as $log_file)
	{
		if(file_exists($log_file.'.0'))
		{
			@unlink($log_file.'.0');
		}

		if(file_exists($log_file))
		{
			@unlink($log_file);
		}
	}
}

function doreset()
{
	$errorInfo = "";

	if( !upload($errorInfo) )
	{
		$url = "configmanage.php?msg=".urlencode($errorInfo)."&in_tabs=".$_GET['in_tabs'];
		$ret = -1;
	} else {
		$file = $_FILES['currentcfgfile'];
		$ret = ace_config_checkin( $file['name'] );
	}

	$log = array();
	$log['content'] = array();
	$log['content'][0] = _gettext('ConfigureBak&re');
	$log['content'][1] = _gettext("cfgresetcmt");

	if( 0 == $ret )
	{
		$log['result'] = _gettext('Success');
		saveLog($log);
		$msg = _gettext("cfgresetSuccess")."! ";
		$output = array();
		if ( is_support_board_type("NK02") || is_support_board_type("NL01") 
			|| is_support_board_type("NK03") || is_support_board_type("YWER-HG5X")
			)
		{
			exec( "reboot -f", $output, $result );
		}
		else
		{
			exec( "reboot", $output, $result );
		}
	}
	else
	{
		$log['result'] = _gettext('fail');
		saveLog($log);
		if (-8 == $ret)
			$msg = _gettext("cfgresetFailnotVailed")."! ";
		else
			$msg = _gettext("cfgresetFail")."! ";
	}
	$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];

	JumpUrl($url,$msg);
}

function doautosavereset($file)
{
	$output = array();
	$file = "/home/config/current/autosave_config/".filenameValidate($file);
	if("/home/config/current/autosave_config"!= dirname($file)) {
	    exit("error");
	}
	$cmd = "cp -af ".hyenshellarg($file)." /home/checkin/Config.conf";
	exec($cmd,$output,$ret);

	$ret = ace_config_checkin("Config.conf");

	$log = array();
	$log['content'] = array();
	$log['content'][0] = _gettext('ConfigureBak&re');
	$log['content'][1] = _gettext("cfgresetcmt");

	if( 0 == $ret )
	{
		$log['result'] = _gettext('Success');
		saveLog($log);
		$msg = _gettext("cfgresetSuccess")."! ";
		$output = array();
		if ( is_support_board_type("NK02") || is_support_board_type("NL01")
			|| is_support_board_type("NK03") || is_support_board_type("YWER-HG5X")
			)
		{
			exec( "reboot -f", $output, $result );
		}
		else
		{
			exec( "reboot", $output, $result );
		}
	}
	else
	{
		$log['result'] = _gettext('fail');
		saveLog($log);
		$msg = _gettext("cfgresetFail")."($ret)! ";
	}
	$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];

	JumpUrl($url,$msg);
}

if ( $_REQUEST['cfgmanage'] == 'resetcfg' )
{
	if ( function_exists('lic_get_state') )//license过期不能恢复
	{
		$licenstatu = lic_get_state();
		if ( $licenstatu < 0 )
		{
			$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];
			$msg = _gettext("fail").'!';
			JumpUrl($url,$msg);
		}
	}
	if(isDefect(15)) {
		$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];
		$msg = _gettext("cfgresetSuccess").'!';
		JumpUrl($url,$msg);
	} else {
		doreset();
	}
}
else if ( $_REQUEST['cfgmanage'] == 'resetautosavecfg' )
{
	if ( function_exists('lic_get_state') )//license过期不能恢复
	{
		$licenstatu = lic_get_state();
		if ( $licenstatu < 0 )
		{
			$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];
			$msg = _gettext("fail").'!';
			JumpUrl($url,$msg);
		}
	}
	if(isDefect(15)) {
		$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];
		$msg = _gettext("cfgresetSuccess").'!';
		JumpUrl($url,$msg);
	} else {
		doautosavereset($_REQUEST['autosavefile']);
	}
}
else if ($_REQUEST['cfgmanage']=='cfgresetdefault')
{
	$auth->isGuestAdmin();
	$ret = ace_set_factory_setting();
	if($ret == 0)
	{
		if($_POST["log_delete"] == 1) { //删除日志
			system("rm -rf /var/www/reporter/store/*");
			system("rm -rf /mnt/pgsql/data/*");
			system("rm -rf /mnt/clickhouse/data/* && rm -rf /mnt/clickhouse/store/*");
			system("rm -rf /var/log/sqlite/*");
			delete_syslog_file();
		}
		$output = array();
		if ( is_support_board_type("NK02") || is_support_board_type("NL01")
			|| is_support_board_type("NK03") || is_support_board_type("YWER-HG5X")
			)
		{
			exec( "reboot -f", $output, $result );
		}
		else
		{
			exec( "reboot", $output, $result );
		}

		$msg = _gettext("cfgresetdefaultSuccess")."! ";
	}
	else
	{
		$msg = _gettext("cfgresetdefaultFail")."($ret)! ";
	}

	$log = array();
	$log['content'] = array();
	$log['content'][0] = _gettext('ConfigureBak&re');
	$log['content'][1] = _gettext("cfgresetdefaultcmt");
	if(isset($_POST["log_delete"])) {
		$log['content'][2] = ($_POST["log_delete"] ? _gettext('Restore factory settings')."("._gettext('Delete historical data').")" : _gettext('Restore factory settings')."("._gettext('Keep historical data').")");
	} else {
		$log['content'][1] = _gettext("cfgresetdefaultcmt");
	}
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

	$url = 'configmanage.php?in_tabs='.$_GET['in_tabs'];
	JumpUrl($url,$msg);
}
?>
