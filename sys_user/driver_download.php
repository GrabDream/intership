<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
	$mode =  isThreeConf();

	if($mode && $_SESSION['regUser'] == 'sysadmin'){

	}elseif ( 0 != strcmp($_SESSION['regRole'],'super_admin')) {
        exit("error");
    }
	if($_REQUEST["type"] == "ukey") {
		$savename = "UKeySetup.exe";
		$filename = "/var/www/auth/UKeySetup.exe";
	} else {
		$savename = "Driver_setup.exe";
		$filename = "/var/www/auth/BossKey_Driver_Setup.exe";
	}

	$file = fopen($filename, "r");
	Header("Accept-Ranges: bytes");
	header("Content-Type: application/octet-stream");
	header("Content-Length: " . filesize($filename));

	header( "Content-Disposition: attachment; filename=".urlencode($savename) );
	fpassthru($file);
	fclose($file);

	exit();
?>
