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
include_once ("user_functions.php");
// print_r($_POST);
function get_user_func() {
	if (! isset ( $_POST ['func'] )) {
		
		return array (
				"user_func_num" => 0,
				"user_func_idx" => 0 
		);
	}
	
	$user_func_num = 0;
	$user_func_idx = 0;
	
	for($i = 0; $i < count ( $_POST ['func'] ); $i ++) {
		$user_func_num ++;
		
		$user_func_idx |= pow ( 2, $_POST ['func'] [$i] );
	}
	
	return array (
			"user_func_num" => $user_func_num,
			"user_func_idx" => $user_func_idx 
	);
}
function is_role_used() {
	$usercount = array ();
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$userdata = json_decode ( $contents, true );
	
	for($i = 0; $i < count ( $userdata ); $i ++) {
		if ($userdata [$i] ['role'] == $_GET ['name']) {
			return true;
		}
	}
	return false;
}

$action = $_GET ["action"];

if ($action == "delete") {
	
	if (is_role_used () == true) {
		$system_user_role_ret = - 1;
	} else {
		DelRoleRecordsets ( $system_user_role_ret );
	}
} else if ($action == "add") {
	$system_user_role_cfg = array (
			'pre_define' => 0,
			'name' => $_POST ['name'],
			'comment' => $_POST ['comment'] 
	);
	for($idx = 1; $idx < 200; $idx ++) {
		if ($_POST [strval ( $idx )] != "") {
			$system_user_role_cfg [strval ( $idx )] = $_POST [strval ( $idx )];
		}
	}
	
	for($idx = 1001; $idx < 1200; $idx ++) {
		$tmp = "read_" . strval ( $idx - 1000 );
		if ($_POST [strval ( $idx - 1000 )] == "" && $_POST [$tmp] != "") {
			$system_user_role_cfg [strval ( $idx )] = $_POST [$tmp];
		}
	}
	
//报表权限
	$reporter_fuctions_array1 = get_reporter_func_recordsets();
	$system_user_role_cfg["reporter"] = array();
	foreach($reporter_fuctions_array1 as $v) {
	    if($v["id"] == 1) {
			$system_user_role_cfg["reporter"][$v["id"]] = 1;
			continue;
		} else if($_POST["r_" . $v["id"]] != '' && $_POST["r_read_" . $v["id"]] != '') {
	        $system_user_role_cfg["reporter"][$v["id"]] = 1;
	    } else if($_POST["r_read_" . $v["id"]] != '') {
	        $system_user_role_cfg["reporter"][$v["id"]] = 2;
	    } else {
			continue;
		}
		if($v["pid"] <= 0) {
			continue;
		}
		//判断第一
		foreach($reporter_fuctions_array1 as $vv) {
			if($v["pid"] == $vv["id"]) {
				if(!array_key_exists($vv["id"],$system_user_role_cfg["reporter"])) {
					$system_user_role_cfg["reporter"][$vv["id"]] = 1;
					if($vv["pid"] <= 0) {
						break;
					}
					//第二层
					foreach($reporter_fuctions_array1 as $vvv) {
						if($vv["pid"] == $vvv["id"]) {
							if(!array_key_exists($vvv["id"],$system_user_role_cfg["reporter"])) {
								$system_user_role_cfg["reporter"][$vvv["id"]] = 1;
								break;
							}
							break;
						}
					}	
				}
				break;
			}
		}
	}
	
	// print_r($_POST);
	// print_r($system_user_role_cfg);
	$system_user_role_ret = AddRoleRecordsets ( $system_user_role_cfg );
	if (is_bool ( $system_user_role_ret )) {
		if ($system_user_role_ret == true)
			$system_user_role_ret = 0;
	} else {
		$system_user_role_ret = 807;
	}
} else if ($action == "edit") {
	// print_r($user_func);
	$system_user_role_cfg = array (
			'pre_define' => 0,
			'name' => $_POST ['name'],
			'comment' => $_POST ['comment'] 
	);
	// echo $system_user_role_cfg;
	for($idx = 1; $idx < 200; $idx ++) {
		if ($_POST [strval ( $idx )] != "") {
			$system_user_role_cfg [strval ( $idx )] = $_POST [strval ( $idx )];
		}
	}
	for($idx = 1001; $idx < 1200; $idx ++) {
		$tmp = "read_" . strval ( $idx - 1000 );
		if ($_POST [strval ( $idx - 1000 )] == "" && $_POST [$tmp] != "") {
			$system_user_role_cfg [strval ( $idx )] = $_POST [$tmp];
		}
	}
	
//报表权限
	$reporter_fuctions_array1 = get_reporter_func_recordsets();
	$system_user_role_cfg["reporter"] = array();
	foreach($reporter_fuctions_array1 as $v) {
		if($v["id"] == 1) {
			$system_user_role_cfg["reporter"][$v["id"]] = 1;
			continue;
		} else if($_POST["r_" . $v["id"]] != '' && $_POST["r_read_" . $v["id"]] != '') {
	        $system_user_role_cfg["reporter"][$v["id"]] = 1;
	    } else if($_POST["r_read_" . $v["id"]] != '') {
	        $system_user_role_cfg["reporter"][$v["id"]] = 2;
	    } else {
			continue;
		}
		if($v["pid"] <= 0) {
			continue;
		}
		//判断第一
		foreach($reporter_fuctions_array1 as $vv) {
			if($v["pid"] == $vv["id"]) {
				if(!array_key_exists($vv["id"],$system_user_role_cfg["reporter"])) {
					$system_user_role_cfg["reporter"][$vv["id"]] = 1;
					if($vv["pid"] <= 0) {
						break;
					}
					//第二层
					foreach($reporter_fuctions_array1 as $vvv) {
						if($vv["pid"] == $vvv["id"]) {
							if(!array_key_exists($vvv["id"],$system_user_role_cfg["reporter"])) {
								$system_user_role_cfg["reporter"][$vvv["id"]] = 1;
								break;
							}
							break;
						}
					}	
				}
				break;
			}
		}
	}
	
	$system_user_role_ret = EditRoleRecordsets ( $system_user_role_cfg );
	$system_user_role_ret = ! $system_user_role_ret;
} else if ($action == "up_remove" || $action == "down_remove") {
	$system_user_role_ret = RemoveRoleRecordsets ( $_GET ["name"], $action );
	$system_user_role_ret = ! $system_user_role_ret;
}

/**
 * *************** LOG **********************
 */

if ($action == "delete") {
	$log = array ();
	$log ['content'] = array ();
	$log ['content'] [0] = _gettext ( 'delSystemrole' );
	$log ['content'] [1] = _gettext ( 'Name' ) . ": " . $_GET ["name"];
	
	if ($system_user_role_ret == 0) {
		$log ['result'] = _gettext ( 'Success' );
	} else {
		$log ['result'] = _gettext ( 'fail' );
		$error_text = 'role_is_inused';
		$log ['ret'] = _gettext ( $error_text );
	}
	
	saveLog ( $log );
}

if ($action == "add") {
	$user_func = get_user_func ();
	$log = array ();
	$log ['content'] = array ();
	$log ['content'] [0] = _gettext ( 'addsystemrole' );
	$log ['content'] [1] = _gettext ( 'role_name' ) . ": " . _gettext($_REQUEST ["name"]) ;
	$log ['content'] [2] = _gettext ( 'role_comment' ) . ": " . $_POST ["comment"];
	//$log ['content'] [3] = _gettext ( 'privilege' ) . ": " . get_privilege ( $user_func ['user_func_num'], $user_func ['user_func_idx'] );
	$first=false;
	$privilege='';
	foreach($system_user_role_cfg as $k=>$v){
		if($k=='pre_define' || $k=='name' || $k=='comment'  ){
			continue;
		}
		if($first){
			$privilege.=',';
		}
		$privilege.=_gettext($v);
		$first=true;
	}
	$log ['content'] [3] = _gettext ( 'privilege' ) . ": ".$privilege;
	if ($system_user_role_ret == 0) {
		$log ['result'] = _gettext ( 'Success' );
	} else {
		$log ['result'] = _gettext ( 'fail' );
		$error_text = 'error' . $system_user_role_ret;
		$log ['ret'] = _gettext ( $error_text );
	}
	
	saveLog ( $log );
} else if ($action == "edit") {
	$user_func = get_user_func ();
	$log = array ();
	$log ['content'] = array ();
	$log ['content'] [0] = _gettext ( 'editsystemrole' );
	$log ['content'] [1] = _gettext ( 'role_name' ) . ": " . _gettext($_REQUEST ["name"]) ;
	$log ['content'] [2] = _gettext ( 'role_comment' ) . ": " . $_POST ["comment"];
	//$log ['content'] [3] = _gettext ( 'privilege' ) . ": " . get_privilege ( $user_func ['user_func_num'], $user_func ['user_func_idx'] );
	$first=false;
	$privilege='';
	foreach($system_user_role_cfg as $k=>$v){
		if($k=='pre_define' || $k=='name' || $k=='comment'  ){
			continue;
		}
		if($first){
			$privilege.=',';
		}
		$privilege.=_gettext($v);
		$first=true;
	}
	$log ['content'] [3] = _gettext ( 'privilege' ) . ": ".$privilege;
	if ($system_user_role_ret == 0) {
		$log ['result'] = _gettext ( 'Success' );
	} else {
		$log ['result'] = _gettext ( 'fail' );
		$error_text = 'error' . $system_user_role_ret;
		$log ['ret'] = _gettext ( $error_text );
	}
	
	saveLog ( $log );
} else if ($action == "up_remove" || $action == "down_remove") {
	$log = array ();
	$log ['content'] = array ();
	//$log ['content'] [0] = _gettext ( 'editsystemrole' ) . _gettext( $action );
	
	if($action == "up_remove"){
		$log ['content'] [0] = _gettext ( 'editsystemrole' ) . _gettext("moveUp");
	}else if($action == "down_remove"){
		$log ['content'] [0] = _gettext ( 'editsystemrole' ) . _gettext("moveDown");
	}
	
	$log ['content'] [1] = _gettext ( 'role_name' ) . ": " . _gettext($_GET ["name"]) ;
	
	if ($system_user_role_ret == 0) {
		$log ['result'] = _gettext ( 'Success' );
	} else {
		$log ['result'] = _gettext ( 'fail' );
		$error_text = 'error' . $system_user_role_ret;
		$log ['ret'] = _gettext ( $error_text );
	}
	
	saveLog ( $log );
}

if ($system_user_role_ret == 0) {
	
	?>
<script language="javascript">
	//alert("<?=_gettext('Success');?>");
	location.href='user_role.php';
</script>
<?php
} else {
	?>
<script language="javascript">
	alert("<?php echo _gettext('fail:')._gettext($error_text); ?>");
	location.href="user_role.php";

</script>
<?php
}
?>
</body>
