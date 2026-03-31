<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php include_once(PUBLIC_LIBRARY_PHP_DIR . "core/func/sms.func.php"); ?>
<?php

$mode = 0;
$file = '/usr/local/lighttpd/admin_mode';

if (file_exists($file)) {
    $mode = file_get_contents($file);
}
if(!function_exists('pwdTransferr')){
	function pwdTransferr($passwd)
	{
		$passwd = str_replace('$', '$', $passwd);
		//$passwd = str_replace('&', '\&', $passwd);

		return $passwd;
	}
}


$smsTempLateName = $_POST['sms_pwd_auth_tempname'];

$msg = setSmsTempContentKV(array(
    'sms_pwd_auth_tempname' => $smsTempLateName,
    'sms_forgotpwd_tempname' => 5,
));

if(!isOemFeatureByName(array("beixinyuan_fw","beixinyuan_ws","Nsas360","jiuding")) && !isHylabDtr() && $mode != $_POST['mode']){
	$location = 'parent.location.href="/loginout.php";';
	$rolefile = '/home/config/cfg-scripts/three_role.conf';

	if($_POST['mode'] == 1){
		if(file_exists($rolefile)){
			exec('cp -af /home/config/cfg-scripts/three_user.conf /usr/local/lighttpd/user.conf');
			exec('cp -af '.$rolefile.' /usr/local/lighttpd/role.conf');
			exec('cp -af /home/config/cfg-scripts/three_user.conf /home/config/current/user.conf');
			exec('cp -af '.$rolefile.' /home/config/current/role.conf');
			file_put_contents('/usr/local/lighttpd/admin_mode', 1);
			file_put_contents('/home/config/current/admin_mode', 1);
			exec("cp -af /home/config/default/passwd /etc/passwd");
			exec("cp -af /home/config/default/passwd /home/config/current/passwd");
			exec("cp -af /home/config/default/shadow /etc/shadow");
			exec("cp -af /home/config/default/shadow /home/config/current/shadow");
			exec("rm -rf /usr/mysys/strong_password");
			if(isOemFeatureByName("liufangyun")){
				hylab_modify_shell_passwd(0, pwdTransferr( 'zxas6cld'), 'root');
			} else {
				hylab_modify_shell_passwd(0, pwdTransferr( 'root'), 'root');
			}
			exec("rm -rf /etc/homemode.txt");
		}
	}else{
		$commoncfg_ufile = '/home/config/default/user.conf';
		$commoncfg_rfile = '/home/config/default/role.conf';

		exec('cp -af '.$commoncfg_ufile.' /usr/local/lighttpd/user.conf');
        exec('cp -af '.$commoncfg_rfile.' /usr/local/lighttpd/role.conf');
        exec('cp -af '.$commoncfg_ufile.' /home/config/current/user.conf');
        exec('cp -af '.$commoncfg_rfile.' /home/config/current/role.conf');

        file_put_contents('/home/config/current/admin_mode', 0);
		file_put_contents('/usr/local/lighttpd/admin_mode', 0);
		exec("cp -af /home/config/default/passwd /etc/passwd");
		exec("cp -af /home/config/default/passwd /home/config/current/passwd");
		exec("cp -af /home/config/default/shadow /etc/shadow");
		exec("cp -af /home/config/default/shadow /home/config/current/shadow");
		exec("rm -rf /usr/mysys/strong_password");
        if(isOemFeatureByName("liufangyun")){
            hylab_modify_shell_passwd(0, pwdTransferr( 'administrator123'), 'root');
        } else {
			hylab_modify_shell_passwd(0, pwdTransferr( 'root'), 'root');
		}
		exec("rm -rf /etc/homemode.txt");
	}

	//cancel the original mode session
	$old_mode_str = "admin_mode|i:$mode;";
	$session_dir = '/tmp/php/session/';
	if(is_dir($session_dir)){
		$firearr = scandir($session_dir);
		foreach($firearr as $key => $value)
		{
			if(stripos($value, "sess_") !== false) {
				$file = $session_dir.$value;
				$fileContent = file_get_contents($file);
				if(stripos($fileContent, $old_mode_str)!== false){
					@unlink($file);
				}
			}
		}
	}
} else {
	$location = 'location.href="user.php";';
}


?>

<script language="javascript">
	<?php echo $location; ?>
</script>

