<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
	require_once($_SERVER["DOCUMENT_ROOT"]."/model/lan.php");
	include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed.php");
	isAvailable('m_systemadmin');
	include_once("user_functions.php");
	include_once($_SERVER["DOCUMENT_ROOT"]."/model/common_param.php");
	$true=false;

	$mode = isThreeConf();

	if ( strcmp($_SESSION['regUser'],$common_papam_admin_name)==0 || ($mode && $_SESSION['regUser'] == 'sysadmin')){
		$user_array = get_user_recordsets();
		foreach($user_array as $userdata){
			if($_GET['usrmd5']==md5(urlencode($userdata['name']))){
				if($userdata["pre_define"]&&($userdata['name']==$common_papam_admin_name ||$userdata['name']=="guest" || $userdata['name']==$common_param_audit_name)){
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
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
</head>
<script type="text/javascript">
<?php
if(strpos($_SERVER["HTTP_USER_AGENT"], "Trident"))
{
?>
function Write_RC_Key()
{
	var vUser = "<?=$_GET['usrmd5']?>";
	var vPassword = "<?=$_GET['pwdmd5']?>";

	try
	{
		var RCkeyObj = new ActiveXObject("Bkey_Obj.1");
		var ret = RCkeyObj.WriteRCKey(vUser, vPassword);
//		alert( ret );
	}

	catch(e)
	{
/*
		alert(e.message);
		alert(e.description);
		alert(e.number);
		alert(e.name);
*/
	}

	return ret;
}

function isActivexInstall()
{
	var vRet = true;

	try
	{
		var RCkeyObj = new ActiveXObject("Bkey_Obj.1");
	}
	catch(e)
	{
		vRet = false;
	}

	return vRet;
}

var dirver_install = isActivexInstall();
var result = 0;
if (true == dirver_install)
{
	result = Write_RC_Key();
}
if (dirver_install == false)
{
	alert("<?php echo _gettext('Have not install the boss key driver!'); ?>");
}
else if (result == 0)
{
	alert("<?php echo _gettext('Key_Write_in')._gettext('Success').'!'; ?>");
}
else
{
	alert("<?php echo _gettext('Key_Write_in')._gettext('fail').'!'; ?>");
}
<?php
}
else
{
?>
	alert("<?php echo _gettext('dkey_write_only_in_ie'); ?>");
<?php
}
?>
location.href="user.php";
</script>
</html>
