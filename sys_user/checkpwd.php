<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = 'm_systemadmin';
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed_writable.php");
require_once("../../../model/tools.php");
include_once($_SERVER["DOCUMENT_ROOT"]."/model/common_param.php");
if(file_exists("/home/config/cfg-scripts/web_pw_filter")){
	$pwd_lib=file_get_contents("/home/config/cfg-scripts/web_pw_filter");
	$pwd_lib_arr=explode("\r\n",strtolower($pwd_lib));
}

$decrypt_key = substr($_SESSION['token'], 0, 16);
$_REQUEST['setPassword'] = crypto_decrypt($_REQUEST['setPassword'], $decrypt_key);
$_REQUEST['confirmPassword'] = crypto_decrypt($_REQUEST['confirmPassword'], $decrypt_key);

if($_REQUEST['setPassword']==$_REQUEST['confirmPassword']){
	if(!isDefect(18) && in_array(strtolower($_REQUEST['confirmPassword']),$pwd_lib_arr)){
		echo 0;
		exit;
	}
	if(strstr(hylab_platform_feature(), "#ruijie#")){
		$ruijie_pwd_lib_arr = array('password','123456','88888','admin','root','abcde','qwer','ruijie@123','Ruijie@123');
		if(in_array(strtolower($_REQUEST['confirmPassword']),$ruijie_pwd_lib_arr)
			|| stripos($_REQUEST['confirmPassword'], 'Ruijie') !== false 
			|| stripos($_REQUEST['confirmPassword'], 'rj') !== false
			|| stripos($_REQUEST['confirmPassword'], 'N18010') !== false
			|| stripos($_REQUEST['confirmPassword'], 'UAC') !== false
			|| stripos($_REQUEST['confirmPassword'], 'X300D') !== false
			|| stripos($_REQUEST['confirmPassword'], 'U3100') !== false
			|| stripos($_REQUEST['confirmPassword'], 'U3120') !== false){
			echo 0;
			exit;
		}
	}
}
echo 1;
exit;
?>
