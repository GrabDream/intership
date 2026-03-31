<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
	require_once($_SERVER["DOCUMENT_ROOT"]."/model/lan.php");
	include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed.php");
	isAvailable('m_systemadmin');
	include_once("user_functions.php");
	include_once($_SERVER["DOCUMENT_ROOT"]."/model/common_param.php");
	$true=false;

	$mode = isThreeConf();

	if ( strcmp($_SESSION['regUser'],$common_papam_admin_name)==0 || ($_SESSION['regRole'] == 'super_system' && $_SESSION['preDefined'] == 1)
	|| (hylab_firewall() && strstr(hylab_platform_feature(), "#kedong#") && $_SESSION['regUser'] == 'sys')
	|| ($mode && $_SESSION['regUser'] == 'sysadmin')){
		$user_array = get_user_recordsets();
		foreach($user_array as $userdata){
			if($_POST['username']==$userdata['name']){
				if($userdata["pre_define"]&&(($userdata['name']==$common_papam_admin_name && !strstr(hylab_platform_feature(), "#topsec#")) ||$userdata['name']=="guest" || $userdata['name']==$common_param_audit_name || $userdata['role'] == "super_system")){
					$true=false;
				}else{
					if ($userdata['DKey'] == "on" || $userdata['dkey_password'] != "")
					{
						$true=true;
					}
				}
				break;
			}else{
				$true=false;
			}
		}
	}else{
		$true=false;
	}

	if(!$true){
		echo "<script>alert('"._gettext('nowritable')."');location.href='user.php'</script>";
		die();
	}
	$result = EditUserDKey();

	$log = array();
	$log['content'] = array();
	$log['content'][$i++] = _gettext('Key_Write_in');
	$log['content'][$i++] = _gettext('User name').": ".$_POST["username"];

	if($result != true)
	{
		$log['result'] = _gettext('fail');
		$error_text = 'error'.$result;
		$log['ret'] = _gettext($error_text);
	}
	else
	{
		$log['result'] = _gettext('Success');
	}

	saveLog($log);

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
</head>
<body>
<script type="text/javascript">
	alert('<?=_gettext('Operation succeeded')?>');
	location.href="user.php";
</script>
</body>
</html>
