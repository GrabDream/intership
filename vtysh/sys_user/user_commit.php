<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php

$page_name = 'm_systemadmin';
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed_writable.php");
require_once("../../../model/tools.php");
include_once($_SERVER["DOCUMENT_ROOT"]."/model/common_param.php");

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
</head>
<body>
<?php
include_once("queryValid.php");
include_once("user_functions.php");
if (isset($_SESSION['token']) && isset($_POST['encrypt'])) {
    $decrypt_key = substr($_SESSION['token'], 0, 16);
    if ($_GET['action'] != "edit" || $_GET['action'] != "editpasswd") {
        $_REQUEST['setPassword']     = crypto_decrypt($_REQUEST['setPassword'], $decrypt_key);
        $_REQUEST['confirmPassword'] = crypto_decrypt($_REQUEST['confirmPassword'], $decrypt_key);
    } else {
        if ($_POST['reset_psw']) {
            $_REQUEST['setPassword']     = crypto_decrypt($_REQUEST['setPassword'], $decrypt_key);
            $_REQUEST['confirmPassword'] = crypto_decrypt($_REQUEST['confirmPassword'], $decrypt_key);
        }
    }
    if ($_POST['DKey']) {
        $_REQUEST['initialPWD']  = crypto_decrypt($_REQUEST['initialPWD'], $decrypt_key);
        $_REQUEST['initialPWD2'] = crypto_decrypt($_REQUEST['initialPWD2'], $decrypt_key);
    }
}
$user_array  = get_user_recordsets();
$username    = $_REQUEST['user_name'];
$new_api_pwd = crypto_decrypt($_REQUEST['api_pwd'], $decrypt_key);
$old_api_pwd = "";
foreach ($user_array as $user_data_detail) {
    if ($username == $user_data_detail['name']) {
        $oldpwd            = $user_data_detail['password'];
        $old_dkey_password = $user_data_detail['dkey_password'];
        $old_api_pwd       = $user_data_detail['api_pwd'];
        break;
    }
}
if ($_POST['DKey']) {
    if ($_REQUEST['initialPWD'] == "******" && $_REQUEST['initialPWD2'] == "******") {
        $_REQUEST['initialPWD']  = $old_dkey_password;
        $_REQUEST['initialPWD2'] = $old_dkey_password;
    }
} else {
    $_REQUEST['initialPWD']  = $old_dkey_password;
    $_REQUEST['initialPWD2'] = $old_dkey_password;
}

$_REQUEST['api_pwd'] = $new_api_pwd =='******' ? $old_api_pwd : md5($new_api_pwd);
//弱口令
if(file_exists("/home/config/cfg-scripts/web_pw_filter.filter")){
	$pwd_lib=file_get_contents("/home/config/cfg-scripts/web_pw_filter.filter");
	$pwd_lib_arr=explode("\r\n",$pwd_lib);

}
if ($_GET['action'] == 'add') {
    if (!isDefect(18) && in_array($_REQUEST['confirmPassword'], $pwd_lib_arr)) {
        echo "<script>alert('"._gettext('pwd_ruo')."');location.href='user.php'</script>";
        exit;
    }
}
if (!$_POST['reset_psw'] && ($_GET['action'] == "edit" || $_GET['action'] == "editpasswd")) {
    $_REQUEST['setPassword']     = $oldpwd;
    $_REQUEST['confirmPassword'] = $oldpwd;
} else {
    if ($_POST['reset_psw'] && ($_GET['action'] == "edit" || $_GET['action'] == "editpasswd")) {
        if (!isDefect(18) && in_array($_REQUEST['confirmPassword'], $pwd_lib_arr)) {
            echo "<script>alert('"._gettext('pwd_ruo')."');location.href='user.php'</script>";
            exit;
        }
        if (md5($_REQUEST['confirmPassword']) == $oldpwd) {
            echo "<script>alert('"._gettext('newpsw_equal_oldpsw')."');location.href='user.php'</script>";
            die();
        }
        if (in_array($username, array('audit', 'guest')) && $_REQUEST['confirmPassword'] == ($username.'*PWD')){
            echo "<script>alert('"._gettext('password_error_msg0')."');location.href='user.php'</script>";
            die();
        }
    }
}
$_REQUEST['oldPassword']=$oldpwd;
$action = $_GET["action"];

$role_data=showRole();
$true=false;
$mod_true=false;

$mode = isThreeConf();

if ( 0 == strcmp($_SESSION['regUser'],$common_papam_admin_name) || ($mode && $_SESSION['regUser'] == 'sysadmin')){
	if($action=='delete' && ( $username==$common_papam_admin_name || $username==$common_param_audit_name || $username=='guest')){
		echo "<script>alert('"._gettext('This_operation_illegal')."');location.href='user.php'</script>";
		die();
	}
    //Under the three authority mode, it is not possible to add a regular mode administrator
    if($action == 'add' && $mode && ($username == $common_papam_admin_name || $username == $common_param_audit_name || $username == 'guest')) {
        echo "<script type='text/javascript'>alert('" . _gettext('error807') . "');location.href='user.php'</script>";
        die();
    }
}else{
    if($action=='add' || $action=='delete' || $action=="edit"){
        echo "<script>alert('"._gettext('nowritable')."');location.href='user.php'</script>";
        die();
    }

	if(is_array($role_data)){
		foreach($role_data as $val){
			if($action!='add'){
				if (strcmp($_POST['user_name'], $val['name']))
				{
					$true=false;
				}else{
					$true=true;
					$mod_true=$val['modify'];
					break;
				}
			}else{
				$true=true;
			}
		}
		if(!$true){
			echo "<script>alert('"._gettext('nowritable')."');location.href='user.php'</script>";
			die();
		}
		if($mod_true==2){
			if($action=='add' || $action=='delete'){
				echo "<script>alert('"._gettext('nowritable')."');location.href='user.php'</script>";
				die();
			}
		}else{
			if($action=='delete' && ( $_POST['user_name']==$common_papam_admin_name || $_POST['user_name']==$common_param_audit_name || $_POST['user_name']=='guest')){
				echo "<script>alert('"._gettext('nowritable')."');location.href='user.php'</script>";
				die();
			}
		}
	}
}
$action          = $_GET["action"];
$system_user_ret = 0;
// 提交前的参数验证
if ($action == "add" || $action == "edit") {
    $pwdpolicy                = hytf_pwdpolicy_get();
    $pwdpolicy_format         = 'Lisenable/Lpwdvaliday/Lmembernum';
    $pwdpolicy_detail         = unpack($pwdpolicy_format, $pwdpolicy);
    $valicheck['user_name']   = array("type" => 2, "val" => array('1|5', "checkValidateNameCH"));
    $valicheck['auth_method'] = array("type" => 2, "val" => array('1|6', array(1, 2, 3, 4)));
    $oldpwd                   = get_user_pwd($_REQUEST['user_name']);
    if ($pwdpolicy_detail['membernum']) {
        $maxnum = 15;
    } else {
        $maxnum = 0;
    }
    if (!isDefect(18) && ($valicheck['auth_method'] == 1 || $valicheck['auth_method'] == 3 || $valicheck['auth_method'] == 4)) {
        if ($oldpwd != $_REQUEST['setPassword']) {
            $valicheck['setPassword'] = array("type" => 2, "val" => array('1|5', "checkValidatePwdChk", 8, $maxnum));
        }
        if ($oldpwd != $_REQUEST['confirmPassword']) {
            $valicheck['confirmPassword'] = array("type" => 2, "val" => array('1|5', "checkValidatePwdChk", 8, $maxnum));
        }
    }
    if ($_POST['moveList']) {
        $valicheck['moveList'] = array("type" => 2, "val" => array('1|5', "checkUserGroup"));
    }
    $valicheck['trusthost'] = array("type" => 2, "val" => array('1|5', "checkTrustHost"));
    ValidateCommit::validate("user.php", $valicheck);
}

function create_password($pw_length = 6)
{
    $randpwd = '';
    for ($i = 0; $i < $pw_length / 2; $i++) {
        $randpwd .= chr(mt_rand(97, 122));
        $randpwd .= chr(mt_rand(65, 90));
    }
    return $randpwd;
}

if ($action == "delete") {
    $reMsg           = "";
    $system_user_ret = DelUserRecordsets($reMsg);
    if ($system_user_ret) {
        $system_user_ret = 0;
    } else {
        $system_user_ret = $reMsg;
    }
} else {
    if ($action == "add") {
        $retMsg                = "";
        $_REQUEST['user_name'] = trim($_REQUEST['user_name']);

        if ($_POST['auth_method'] == 1 && $_POST['pwd_policy'] == 2) {
            $password                = create_password(6);
            $_REQUEST['setPassword'] = $password;
        }

        $checkRe = check();
        if (true != $checkRe) {
            $system_user_ret = $checkRe;
            $result          = false;
        } else {
            $result = AddUserRecordsets();
            if (is_bool($result)) {
                if ($result == true) {
                    $system_user_ret = 0;
                }
            } else {
                $system_user_ret = 807;
            }
        }
        //echo $system_user_ret;
    } else {
        if ($action == "edit") {
            $retMsg                = "";
            $_REQUEST['user_name'] = trim($_REQUEST['user_name']);

            $checkRe = check();
            if (true !== $checkRe) {
                $system_user_ret = $checkRe;
                $result          = false;
            } else {
                $result = EditUserRecordsets();
                if ($result !== true) {
                    $system_user_ret = $result;
                }
            }
        } else {
            if ($action == "editpasswd") {
                $retMsg                = "";
                $_REQUEST['user_name'] = trim($_REQUEST['user_name']);
                $_REQUEST['email'] = trim($_REQUEST['email']);
                $_REQUEST['telephone'] = trim($_REQUEST['telephone']);
                if(!empty($_REQUEST['email'])){
                    $resEmail = EditUserEmail($_REQUEST['user_name'], $_REQUEST['email']);
                }
                
                if(!empty($_REQUEST['telephone'])){
                    $resPhone = EditUserPhone($_REQUEST['user_name'], $_REQUEST['telephone']);
                }
               
                $checkRe = check();
                if (true != $checkRe) {
                    $system_user_ret = $checkRe;
                    $result          = false;
                } else {
                    $result = EditUserPWD($_REQUEST['user_name'], $_REQUEST['oldPassword'], $_REQUEST['setPassword']);
                    if(!empty($_REQUEST['email'])){
                        $resEmail = EditUserEmail($_REQUEST['user_name'], $_REQUEST['email']);
                    }
                    if(!empty($_REQUEST['telephone'])){
                        $resPhone = EditUserPhone($_REQUEST['user_name'], $_REQUEST['telephone']);
                    }
                    if ($result !== true) {
                        $system_user_ret = $result;
                    }
                }
            }
        }
    }
}


if ($action == "delete") {
    $log               = array();
    $log['content']    = array();
    $log['content'][0] = _gettext('delAdministrator');
    $log['content'][1] = _gettext('Name').": ".$_POST["name"];

    if ($system_user_ret == 0) {
        $log['result'] = _gettext('Success');
    } else {
        $log['result'] = _gettext('fail');
        $error_text    = 'error'.$system_user_ret;
        $log['ret']    = _gettext($error_text);
    }

    saveLog($log);
}

if ($action=="add")
{

	$i=0;
	$email = '';
	$status_array= array('0'=>_gettext('disable'),'1'=>_gettext('enable'));
	$log = array();
	$log['content'] = array();
	$log['content'][$i++] = _gettext('Add Administrator');
	$log['content'][$i++] = _gettext('User name').": ".$_POST["user_name"];
	if($_POST["auth_method"] == 1)
	{
		$log['content'][$i++] = _gettext('auth_method').": "._gettext("password_auth");
		if($_POST["pwd_policy"] ==1)
		{
			$log['content'][$i++] = _gettext('password_policy').": "._gettext("manual_configuration");
			$email = $_POST["email"];
		}
		elseif($_POST["pwd_policy"] ==2)
		{
			$log['content'][$i++] = _gettext('password_policy').": "._gettext("autogeneration");
			$email = $_POST["email2"];
		}
	}
	if($_POST["auth_method"] == 2){
		$log['content'][$i++] = _gettext('auth_method').": "._gettext("Radius_authentication");
		$log['content'][$i++] = _gettext('password_policy').": ".$_POST['radius_sername'];
	}
    // 增加邮件和短信的验证方式
    if($_POST['auth_method'] == 3){
        $log['content'][$i++] = _gettext('auth_method') . ": " . _gettext("pwd_auth_email_verification");
        $email = $_POST["email"];
    }
    // 增加短信验证方式
    if($_POST['auth_method'] == 4){
        $log['content'][$i++] = _gettext('auth_method') . ": " . _gettext("pwd_auth_sms_verification");
        $telephone = $_POST['telephone'];
    }
    
	if (isset($_REQUEST['DKey']))
	{
		$log['content'][$i++] = _gettext('dkey_login').": "._gettext('enable');
		$log['content'][$i] = " ".($_POST["dkey_type"] == 1 ? _gettext("UKey") : _gettext("SMKey"));
	}
	else
	{
		$log['content'][$i++] = _gettext('dkey_login').": "._gettext('disable');
	}
    $log['content'][$i++] = _gettext('totp_2FA') . ": " . ($_REQUEST['totp_status'] == 'on' ? _gettext('enable') : _gettext('disable'));
    $log['content'][$i++] = _gettext('trueName').": ".$_POST["realname"];
	$log['content'][$i++] = _gettext('company_department').": ".$_POST["company"];
	$log['content'][$i++] = _gettext('mail_address').": ".$email;
	$log['content'][$i++] = _gettext('telephone').": ".$_POST["telephone"];
	$log['content'][$i++] = _gettext('role').": " ._gettext($_POST["role"]);
	$log['content'][$i++] = _gettext('status').": ".$status_array[$_POST["status"]];
	$log['content'][$i++] = _gettext('remark').": ".$_POST["comment"];
	$log['content'][$i++] = _gettext('Trust Host').": ".$_POST["trusthost"];
	$log['content'][$i++] = _gettext('API_interface_status').": ". ($_POST["api_status"] ? _gettext("enable") : _gettext("disable"));
	if($_POST["api_status"]) {
		$log['content'][$i++] = _gettext('API_trust_ip').": ". $_POST["api_white_list"];
	}

	if ($system_user_ret == 0)
	{
		$log['result'] = _gettext('Success');
	}
	else
	{
		$log['result'] = _gettext('fail');
		$error_text = 'error'.$system_user_ret;
		$log['ret'] = _gettext($error_text);
	}

	saveLog($log);
	if($system_user_ret == 0)
	{
		if($_POST['auth_method'] == 1 && $_POST['pwd_policy'] == 2)
		{
		    $cmd = '/usr/local/php/bin/php /var/www/html/action/user_mail_notification.php pswd '.hyenshellarg($_REQUEST['email2']).' '.hyenshellarg($_REQUEST['user_name']).' '.hyenshellarg($password);
			exec($cmd);
		}
	}
}
else if($action=="edit")
{
	$i=0;
	$email = '';
	$status_array= array('0'=>_gettext('disable'),'1'=>_gettext('enable'));
	$log = array();
	$log['content'] = array();
	$log['content'][$i++] = _gettext('Modify Administrator');
	$log['content'][$i++] = _gettext('User name').": ".$_POST["user_name"];
	if($_POST["auth_method"] == 1)
	{
		$log['content'][$i++] = _gettext('auth_method').": "._gettext("password_auth");
		if($_POST["pwd_policy"] ==1)
		{
			$log['content'][$i++] = _gettext('password_policy').": "._gettext("manual_configuration");
			$email = $_POST["email"];
		}
		elseif($_POST["pwd_policy"] ==2)
		{
			$log['content'][$i++] = _gettext('password_policy').": "._gettext("autogeneration");
			$email = $_POST["email2"];
		}
	}
	if($_POST["auth_method"] == 2){
		$log['content'][$i++] = _gettext('auth_method').": "._gettext("Radius_authentication");
		$log['content'][$i++] = _gettext('password_policy').": ".$_POST['radius_sername'];
	}

    if($_POST['auth_method'] == 3){
        $log['content'][$i++] = _gettext('auth_method') . ": " . _gettext("pwd_auth_email_verification");
        $email = $_POST["email"];
    }

    if($_POST['auth_method'] == 4){
        $log['content'][$i++] = _gettext('auth_method') . ": " . _gettext("pwd_auth_sms_verification");
        $telephone = $_POST['telephone'];
    }
    
	if (isset($_REQUEST['DKey']))
	{
		$log['content'][$i++] = _gettext('dkey_login').": "._gettext('enable');
	}
	else
	{
		$log['content'][$i++] = _gettext('dkey_login').": "._gettext('disable');
	}

    $log['content'][$i++] = _gettext('totp_2FA') . ": " . ($_REQUEST['totp_status'] == 'on' ? _gettext('enable') : _gettext('disable'));
	$log['content'][$i++] = _gettext('trueName').": ".$_POST["realname"];
	$log['content'][$i++] = _gettext('company_department').": ".$_POST["company"];
	$log['content'][$i++] = _gettext('mail_address').": ".$email;
	$log['content'][$i++] = _gettext('telephone').": ".$_POST["telephone"];
	$log['content'][$i++] = _gettext('role').": "._gettext($_POST["role"]);
	$log['content'][$i++] = _gettext('status').": ".$status_array[$_POST["status"]];
	$log['content'][$i++] = _gettext('remark').": ".$_POST["comment"];
	$log['content'][$i++] = _gettext('Trust Host').": ".$_POST["trusthost"];
	$log['content'][$i++] = _gettext('API_interface_status').": ". ($_POST["api_status"] ? _gettext("enable") : _gettext("disable"));
	if($_POST["api_status"]) {
		$log['content'][$i++] = _gettext('API_trust_ip').": ". $_POST["api_white_list"];
	}


	if ($system_user_ret == 0)
	{
		$log['result'] = _gettext('Success');
	}
	else
	{
		$log['result'] = _gettext('fail');
		$error_text = 'error'.$system_user_ret;
		$log['ret'] = _gettext($error_text);
	}

	saveLog($log);
}

else if($action=="editpasswd")
{
	$log = array();
	$log['content'] = array();
	$log['content'][$i++] = _gettext('Modify Administrator');
	$log['content'][$i++] = _gettext('User name').": ".$_POST["user_name"];

	if ($system_user_ret === 0)
	{
		$log['result'] = _gettext('Success');
	}
	else
	{
		$log['result'] = _gettext('fail');
		$error_text = 'error'.$system_user_ret;
		$log['ret'] = _gettext($error_text);
	}

	saveLog($log);
}

if ($system_user_ret===0) {

?>
<script language="javascript">
	location.href='user.php';
</script>
<?php
}else{
?>
<script language="javascript">
	alert("<?php $error_text = 'error'.$system_user_ret; echo _gettext('fail:')._gettext($error_text); ?>");
	location.href="user.php";

</script>
<?php
}
?>
</body>
