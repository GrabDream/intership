<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = 'm_systemadmin';
include_once ($_SERVER ["DOCUMENT_ROOT"] . "/authenticed_writable.php");
require_once ("../../../model/tools.php");
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

/**
 * *************** LOG **********************
 */
$mode =  isThreeConf();

if($mode && $_SESSION['regRole'] == $three_conf_secadmin_role) {
	//通用三权模式 secadmin 操作
} elseif ( 0 != strcmp($_SESSION['regRole'],'super_admin')) {
    echo "<script>alert('"._gettext('nowritable')."');location.href='user.php'</script>";
    die();
}
$isenable=$_POST['isenable'];
$pwdrule=$_POST['pwdrule'];

if($_POST ['pwdvaliday']<1 || $_POST ['pwdvaliday']>365){
	echo '<script language="javascript">alert('._gettext ( "Illegal parameter" ).');location.href="setPwdSafePolicy.php";</script>';
}
$pat_cfg = pack ( 'LLL', $isenable, ($_POST ['pwdvaliday'] * 1), $pwdrule );
$ret = hytf_pwdpolicy_set ($pat_cfg );

$is_bmj = isHylabBmj();
if ($is_bmj){
    $pwdlength_config = array();
    if ($_POST["pwLisenable"] == 1) {
        $pwdlength_config = [
            'pwLisenable' => $_POST['pwLisenable'],
            'pwdlength_down' => $_POST['pwdlength_down'],
            'pwdlength_up' => $_POST['pwdlength_up']
        ];
    }else{
        $pwdlength_config = [
            'pwLisenable' => 0,
            'pwdlength_down' => "",
            'pwdlength_up' => ""
        ];
    }
    include_once("user_functions.php");
    set_password_custom_length($pwdlength_config);
}

$log = array ();
$log ['content'] = array ();
$log ['content'] [] = _gettext ( 'set pwd safepolicy' );

if($pwdrule==2){
	$log ['content'] [] = _gettext ( 'Password rules' ) . ": " ._gettext("least_two");
}
if($pwdrule==3){
	$log ['content'] [] = _gettext ( 'Password rules' ) . ": " ._gettext("least_three");
}
if($pwdrule==4){
	$log ['content'] [] = _gettext ( 'Password rules' ) . ": " ._gettext("least_four");
}

if ($isenable == 1)
{
	$log ['content'] [] = _gettext ( 'Password_longest_duration' ) . ": " . $_POST ["pwdvaliday"];
}

if ($is_bmj){
    $log ['content'] [] = _gettext ( 'password_custom_length' ) . ": " . ($_POST['pwLisenable'] == 1 ? _gettext("enable") : _gettext("disable"));
    if ($_POST['pwLisenable'] == 1){
        $log ['content'] [] = _gettext ( 'pwdlength_min' ) . ": " .$_POST['pwdlength_down'];
        $log ['content'] [] = _gettext ( 'pwdlength_max' ) . ": " .$_POST['pwdlength_up'];
    }
}

if ($ret == 0) {
	$log ['result'] = _gettext ( 'Success' );
} else {
	$log ['result'] = _gettext ( 'fail' );
}

saveLog ( $log );

if ($ret == 0) {

	?>
<script language="javascript">
	alert("<?=_gettext('Success');?>");
	location.href='setPwdSafePolicy.php';
</script>
<?php
} else {
	?>
<script language="javascript">
	alert("<?php echo _gettext('fail'); ?>");
	location.href="setPwdSafePolicy.php";

</script>
<?php
}
?>
</body>
