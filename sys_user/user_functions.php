<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
function get_user_recordsets() {
	$usercount = array ();
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' )) {
		return false;
	}
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$userdata = json_decode ( $contents, true );
	// print_r($userdata);
	return $userdata;
}
function get_password_custom_length() {
	// pwLisenable/pwdlength_down/pwdlength_up
	$data = array();
	$filename = '/usr/local/lighttpd/pwdlength.conf';
	if (!file_exists($filename)){
		return $data;
	}
	$contents = file_get_contents($filename);
	$data = json_decode($contents, true);
	return $data;
}
function set_password_custom_length($data) {
	// pwLisenable/pwdlength_down/pwdlength_up
	$filename = '/usr/local/lighttpd/pwdlength.conf';
	$contents = json_encode($data, JSON_UNESCAPED_UNICODE);
	return file_put_contents($filename, $contents);
}
function getUserInfo($name){
	$userinfo=get_user_recordsets();
	$user = '';
	foreach($userinfo as $val){
		if($val['name']==$name){
			$user=$val;
			break;
		}
	}
	return $user;
}
function get_user_pwd($name){
	$pwd='';
	$userinfo=get_user_recordsets();
	
	foreach($userinfo as $val){
		if($val['name']==$name){
			$pwd=$val['password'];
			break;
		}
	}
	return $pwd;
}
function pwdTransferr2($passwd)
{
	$passwd = str_replace('$', '$', $passwd);
//	$passwd = str_replace('&', '&', $passwd);
	
	return $passwd;
}
function get_role_recordsets() {
	$filename = '/usr/local/lighttpd/role.conf';
	if (! $handle = fopen ( $filename, 'r' )) {
		return false;
	}
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$roledata = json_decode ( $contents, true );
	// print_r($roledata);
	return $roledata;
}
function showRole(){
	global $common_papam_admin_name;
	$user_array = get_user_recordsets();
	$i=0;
	foreach($user_array as $userdata){
		if ( strcmp($_SESSION['regUser'],$common_papam_admin_name) )
		{
			if (strcmp($_SESSION['regUser'], $userdata['name']))	
			{
				continue;	
			}
		} 
		$data[$i]['name']=$userdata['name'];
		if ( 0 == strcmp($_SESSION['regUser'],$common_papam_admin_name))
		{
			$data[$i]['modify']=1;
		}
		else
		{
			$data[$i]['modify']=2;
		}
		$i++;
	}
	return $data;
}
function get_func_recordsets() {
	$funcdata1 = array ();
	$filename = '/var/www/html/js/func.conf';
	if (! $handle = fopen ( $filename, 'r' )) {
		return false;
	}
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$funcdata = json_decode ( $contents, true );
	
	$lines = @file ( '/home/ace_menu_config.conf' );
	if ($lines) {
		
		foreach ( $lines as $data ) {
			if ($data [0] == '#') {
				continue;
			}
			$data = trim ( $data );
			if (strcmp ( $data, "ipv6" ) == 0) {
				$ipv6 = 1;
			} else if (strcmp ( $data, "vpn" ) == 0) {
				$vpn = 1;
			} else if (strcmp ( $data, "!hotel" ) == 0) {
				$hotel = 1;
			}
		}
	}
	
	if (file_exists ( '/home/config/cfg-scripts/bypass_switch_config' )) {
		$bypass = 1;
	} else {
		$bypass = 0;
	}

	if (file_exists("/etc/outerReporter.conf")) {
		$reporter_config = array();

		getConfig();
		if (array_key_exists("inner_function", $reporter_config)) {
			$inner_function = intval($reporter_config["inner_function"]);
		} else {
			$inner_function = 1;
		}

		$noharddisk = !$inner_function;
	} else {
		$noharddisk = 0;
	}

	if (file_exists('/etc/modules_options/illegal_outreach'))
	{
		$illegal_outreach = 1;
	}
	else
	{
		$illegal_outreach = 0;
	}
	if (file_exists('/etc/firewall/ips')) {
		$ips_support = 1;
	} else {
		$ips_support = 0;
	}
	if (file_exists('/etc/firewall/waf')) {
		$waf_support = 1;
	} else {
		$waf_support = 0;
	}
	if (file_exists('/etc/firewall/ad')) {
		$ad_support = 1;
	} else {
		$ad_support = 0;
	}
	if (file_exists('/etc/firewall/av')) {
		$av_support = 1;
	} else {
		$av_support = 0;
	}

	if (file_exists('/etc/firewall/vs')) {
		$vs_support = 1;
	} else {
		$vs_support = 0;
	}

	if (file_exists('/etc/firewall/ti')) {
		$ti_support = 1;
	} else {
		$ti_support = 0;
	}

	if (file_exists('/etc/firewall/wpp')) {
		$wpp_support = 1;
	} else {
		$wpp_support = 0;
	}
	if (file_exists('/etc/firewall/iop')) {
		$iop_support = 1;
	} else {
		$iop_support = 0;
	}
	if (file_exists('/etc/firewall/ra')) {
		$ra_support = 1;
	} else {
		$ra_support = 0;
	}

	if (file_exists('/usr/private/pptpd')) {
		$vpn_support = 1;
	} else {
		$vpn_support = 0;
	}

	if (file_exists('/usr/private/openvpn')) {
		$ssl_vpn_support = 1;
	} else {
		$ssl_vpn_support = 0;
	}

	$oem = hylab_platform_feature();

	if (strstr($oem, "#nologo#")) {
		$sslConfig = 1;
	} else {
		$sslConfig = 0;
	}

	if(is_industrial_control()) 
	{
		$industrial_control_support = 1;
	} else {
		$industrial_control_support = 0;
	}

	$nohard = 0;
	if(file_exists("/tmp/.noharddisk.conf") || file_exists("/home/.noharddisk.conf")) {
		$nohard = 1;
	}
	
	$funtions_idx = 0;
	for($idx = 0, $idxMax = count($funcdata); $idx < $idxMax; $idx ++) {
		if ($funcdata [$idx] ['hide'] == '1') {
			continue;
		}
		
		if ($funcdata [$idx] ['bypass'] == '1' && $bypass != 1) {
			continue;
		}
		
		if ($funcdata [$idx] ['vpn'] == '1' && $vpn == 1) {
			continue;
		}
		
		if ($funcdata [$idx] ['ipv6'] == '1' && ($ipv6 == 1 || $_SESSION ["lan"] != 'zh_TW')) {
			continue;
		}
		
		if ($funcdata [$idx] ['hotel'] == '1' && $hotel != 1) {
			continue;
		}
		
		if ($funcdata [$idx] ['noharddisk'] == '1' && $noharddisk == 1) {
			continue;
		}

		if ($funcdata [$idx]['ips_vs']=='1' && $ips_support == 0 && $vs_support == 0 )
		{
			continue;
		}
			
		if ($funcdata [$idx] ['ips'] == '1' && $ips_support == 0) {
			continue;
		}
		
		if ($funcdata [$idx] ['waf'] == '1' && $waf_support == 0) {
			continue;
		}
		
		if ($funcdata [$idx] ['ad'] == '1' && $ad_support == 0) {
			continue;
		}
		
		if ($funcdata [$idx] ['av'] == '1' && $av_support == 0) {
			continue;
		}

		if ($funcdata [$idx]['vs']=='1' && $vs_support == 0 )
		{
			continue;
		}

		if ($funcdata [$idx]['ti']=='1' && $ti_support == 0 )
		{
			continue;
		}

		if ($funcdata [$idx]['wpp']=='1' && $wpp_support == 0 )
		{
			continue;
		}

		if ($funcdata [$idx]['iop']=='1' && $iop_support == 0 )
		{
			continue;
		}

		if ($funcdata [$idx]['ra']=='1' && $ra_support == 0 )
		{
			continue;
		}

		if ($funcdata[$idx]['vpn']=='1' && $vpn_support == 0 )
		{
			continue;
		}
		
		if ($funcdata[$idx]['ssl-vpn']=='1' && $ssl_vpn_support == 0 )
		{
			continue;
		}
		
		if ($funcdata[$idx]['ssl_proxy_rule']=='1' && $sslConfig == 0 )
		{
			continue;
			
		}

		if ($funcdata[$idx]['industrial_control']=='1' && $industrial_control_support == 0 ) 	
		{
			continue;
		}

		if ($funcdata[$idx]['nohard']=='1' && $nohard==1 )
		{
			continue;		
		}

		if ($funcdata[$idx]['industrial_control']=='1' && $industrial_control_support == 0 ) 	
		{
			continue;
		}

		if ($funcdata[$idx]['illegal_outreach']=='1' && $illegal_outreach == 0 )
		{
			continue;
		}

		if (fw_is_new_mode_policy() && $funcdata[$idx]['name']=='m_ICTemplate') 
		{
			$funcdata[$idx]['name']='m_template_management';
		}

		if ($funcdata[$idx]['name'] == 'm_currentStatus' && $funcdata[$idx]['pid'] )
		{
			continue;		
		}
		
		$funcdata1 [$funtions_idx] = $funcdata [$idx];
		$funtions_idx ++;
	}
	return $funcdata1;
}
function get_da_func_recordsets($array){
    $home_filename = "/home/config/da/da_func.conf";
    if(file_exists($home_filename)){
        $filename = $home_filename;
    }else{
        $filename = '/var/www/reporter/da/da_func.conf';
    }
	if(file_exists($filename)){
    if (! $handle = fopen($filename, 'r')) {
        return $array;
    }

    $contents = fread($handle, filesize($filename));
    fclose($handle);
    $funcdata = json_decode($contents, true);
    $count = count($array);
    foreach($funcdata as $key=>$value){
        if($value["level"]!=1){
            $array[$count] = $funcdata[$key];
            $count++;
        }
    }
	}
    return $array;
}
function get_reporter_func_recordsets() {
    define('BASEPATH', '/var/www/public_library/server');

	$oemtype = "";
	$is_nsas = file_exists('/etc/modules_options/nsas') ? 1 : 0;
	$is_ifw = file_exists('/etc/modules_options/ifw') ? 1 : 0;
        
	if (function_exists("hylab_platform_feature") && strstr(hylab_platform_feature(), "#ruijie#")) {
		$oemtype = "ruijie";
		if(function_exists("hylab_ruijie_new") && hylab_ruijie_new() == 2) {
			$oemtype = "ruijie_ics";
		}
	}

	if($oemtype == "ruijie_ics") {
    	include_once(BASEPATH . "/application/config/func_ruijie_ics.php");
		$config["func"] = $config["func_ruijie_ics"];
	} else if(isOemFeatureByName("ugurad")) {
    	include_once(BASEPATH . "/application/config/func_ugurad.php");
		$config["func"] = $config["func_ugurad"];
	} else if($is_nsas) {
		include_once(BASEPATH . "/application/config/func_nsas.php");
		$config["func"] = $config["func_nsas"];
	} else if($is_ifw) {
		include_once(BASEPATH . "/application/config/func_ifw.php");
		$config["func"] = $config["func_ifw"];
	} else if(!hylab_ac_new() && hylab_firewall()) {
		include_once(BASEPATH . "/application/config/func_firewall.php");
		$config["func"] = $config["func_firewall"];
	} else {
		include_once(BASEPATH . "/application/config/func.php");
	}
	$nohard_hide_arr = array();
	if(file_exists("/tmp/.noharddisk.conf") || file_exists("/home/.noharddisk.conf")) {
		include_once(BASEPATH . "/application/config/func_nohard.php");
		$nohard_hide_arr = $config["func_nohard"];
	}
    
    $ips_support = file_exists('/etc/firewall/ips') ? 1 : 0;
    $waf_support = file_exists('/etc/firewall/waf') ? 1 : 0;
    $ad_support = file_exists('/etc/firewall/ad') ? 1 : 0;
    $av_support = file_exists('/etc/firewall/av') ? 1 : 0;
    $ti_support = file_exists('/etc/firewall/ti') ? 1 : 0;
    $industrial_control_support = is_industrial_control() ? 1 : 0;
    
    $funcdata1 = array();
    foreach($config["func"] as $val) {
        if ($val['status'] == '0') {
            continue;
        }
        if ($val['n'] == '1') {
            continue;
        }

		if($val['hide'] == '1') {
			continue;
		}

		if(in_array($val["id"], $nohard_hide_arr) || in_array($val["pid"], $nohard_hide_arr)) {
			continue;
		}

		if($oemtype && is_array($config["ome_hide_func"]) && array_key_exists($oemtype, $config["ome_hide_func"]) && in_array($val["id"], $config["ome_hide_func"][$oemtype])) {
			continue;
		}
		
		//RUIJIE EG 菜单
		if($oemtype != "ruijie" && ($val["id"] == "10" || $val["pid"] == "10")) {
			continue;
		}

		if(!isHylabThreatAll() && $v["threat"] == 1) { //New Threat Log
			continue;
		}
        if ($val['ips'] == '1' && (isHylabThreatAll() || $ips_support == 0)) {
            continue;
        }
        
        if ($val['waf'] == '1' && (isHylabThreatAll() || $waf_support == 0)) {
            continue;
        }
        
        if ($val['ad'] == '1' && (isHylabThreatAll() || $ad_support == 0)) {
            continue;
        }
        
        if ($val['av'] == '1' && (isHylabThreatAll() || $av_support == 0)) {
            continue;
        }

		if ($val['ti'] == '1' && (isHylabThreatAll() || $ti_support == 0)) {
            continue;
        }

		if($val["industrial_control"] == 1 && $industrial_control_support == 0) {
			continue;
		}

		if ($val['firewall_strong']=='1' && hylab_firewall_strong()==0 )
		{
			continue;		
		}

		if($val['name'] == 'rmenu_currentStatus')
		{
			$val['name'] = 'rmenu_reporter_currentStatus';
		}

		if(!$is_nsas && $val["nsas"] == 1) {
			continue;
		}

		if (strcmp($val['name'],'rmenu_big_data')==0 && !is_file('/var/www/reporter/da/index.php'))
		{
			continue;
		}
        if($val["id"] >= 0 && $val["id"] <= 100) {
            $level = 1;
        } else if(($val["id"] > 100 && $val["id"] < 1000) || ($val["pid"] < 100 && $val["id"] >10000)) {
            $level = 2;
        } else {
            $level = 3;
        }
        $funcdata1[] = array(
            "id"=> $val["id"],
            "pid"=> $val["pid"],
            "name"=> $val["name"],
            "level"=>$level,
        );
    }
    $funcdata1 = get_da_func_recordsets($funcdata1);
    return $funcdata1;
}
function get_func_list_by_module($module, $user_func_idx) {
	$func_data = get_func_recordsets ();
	$ret = '';
	$i = 1;
	// print_r($func_data);
	foreach ( $func_data as $func_detail ) {
		$flags = $user_func_idx & pow ( 2, $i );
		if ($func_detail ['module'] != $module) {
			$i ++;
			continue;
		}
		
		$ret .= '<div>';
		if ($flags == 0)
			$ret .= '<input type="checkbox" name="func[]" value="' . ($i) . '" />';
		else
			$ret .= '<input type="checkbox" name="func[]" value="' . ($i) . '" checked="checked"/>';
		
		$ret .= _gettext ( $func_detail ['name'] );
		
		$ret .= '</div>';
		$i ++;
	}
	
	return $ret;
}
function get_privilege($functn_num, $function_idx) {
	$func_data = get_func_recordsets ();
	$first_flags = 1;
	$ret = '';
	$i = 1;
	
	foreach ( $func_data as $func_detail ) {
		$flags = $function_idx & pow ( 2, $i );
		$i ++;
		
		if ($flags == 0)
			continue;
		
		if ($first_flags) {
			$first_flags = 0;
		} else
			$ret .= ', ';
		
		$ret .= _gettext ( $func_detail ['name'] );
	}
	
	return $ret;
}
function check() {
	if (! isset ( $_REQUEST ['user_name'] ) || ! $_REQUEST ['user_name']) {
		return _gettext ( 'wrongparam' );
	}
	if (! ($_REQUEST ['setPassword'] == $_REQUEST ['confirmPassword'])) {
		return _gettext ( 'againwrong' );
	}
	if (! ($_REQUEST ['initialPWD'] == $_REQUEST ['initialPWD2'])) {
		return _gettext ( 'againwrong' );
	}
	$rule_list = array (
			'setPassword' => 'len:8:30' 
	);
	$rule_date = array (
			'setPassword' => $_REQUEST ['setPassword'] 
	);
	$gg = new checker ( $rule_date );
	// $gg->check($rule_list);
	$errStr = implode ( ",", $gg->array_errors );
	if ($errStr)
		return $errStr;
	else
		return true;
}
function AddUserRecordsets() {
	global $common_papam_admin_name;
	$usercount = array ();
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	
	$userdata = json_decode ( $contents, true );
	if (count ( $userdata ) > 0) {
		foreach ( $userdata as $item ) {
			if (($_REQUEST ['user_name']) == $item ['name'])
				return _gettext ( 'useragainwrong' );
		}
	}
	$dkey_password = "";
	$dkey_status = 0;
	$dkey_type = 0;
	if (isset ( $_REQUEST ['DKey'] )) {
		if ($userdata [$i] ['dkey_password'] == $_REQUEST ['initialPWD']) {
			$dkey_password = $userdata [$i] ['dkey_password'];
		} else {
			$dkey_password = md5 ( $_REQUEST ['initialPWD'] );
		}
		$dkey_status = $_REQUEST ['dkey_status'];
		$dkey_type = $_REQUEST ['dkey_type'];
	}
	$totalnum = count ( $userdata );
	$userdata [$totalnum] = array (
			'pre_define' => 0,
			'auth_method' => $_REQUEST ['auth_method'],
			'role' => $_REQUEST ['role'],
			'name' => $_REQUEST ['user_name'],
			'password' => md5 ( $_REQUEST ['setPassword'] ),
			'lastpwdtime' => time (),
			'radius_sername' => $_REQUEST ['radius_sername'],
			'realname' => $_REQUEST ['realname'],
			'status' => $_REQUEST ['status'],
			'email' => $_REQUEST ['pwd_policy'] == 2 ? $_REQUEST ['email2'] : $_REQUEST ['email'],
			'company' => $_REQUEST ['company'],
			'telephone' => $_REQUEST ['telephone'],
			'comment' => $_REQUEST ['comment'],
			'DKey' => $_REQUEST ['DKey'],
			'dkey_password' => $dkey_password,
			'dkey_status' => $dkey_status,
			'dkey_type' => $dkey_type,
			'api_status' => $_REQUEST ['api_status'] ? 1 : 0,
			'api_pwd' => $_REQUEST ['api_status'] ? $_REQUEST ['api_pwd'] : '',
			'api_white_list' => $_REQUEST ['api_status'] ? $_REQUEST ['api_white_list'] : '',
			'totp_status' => $_REQUEST ['totp_status'] == 'on' && $_REQUEST ['totp_secret'] ? 1 : 0,
			'totp_secret' => $_REQUEST ['totp_secret'] && $_REQUEST ['totp_status'] ? $_REQUEST ['totp_secret'] : '',
	);

	if($common_papam_admin_name != $_REQUEST ['user_name'] && isset($_REQUEST['trusthost'])){
		$userdata [$totalnum]['trusthost'] = trim($_REQUEST['trusthost']);
	}

	if (trim ( $_REQUEST ['moveList'] ) != "") {
		$userdata [$totalnum] ['path'] = trim ( $_REQUEST ['moveList'] );
		$userdata [$totalnum] ['hash'] = realtimemonitor_get_path_hash ( substr ( strchr ( trim ( $_REQUEST ['moveList'] ), "/" ), 1 ) );
	}
	
	if (! $handle2 = fopen ( $filename, 'wb' ))
		return _gettext ( 'writefailed' );
	$newContents = json_encode ( $userdata );
	if (fwrite ( $handle2, $newContents ) === FALSE)
		return _gettext ( 'writefailed' );
	
	fclose ( $handle2 );
	
	system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");
	
	return true;
}
function checkDefulPwd($user_name,$pwd){
	$oldpwd=getOldPwd($user_name);
	$filename_default = '/home/config/default/user.conf';
	if ($handle_default = fopen ( $filename_default, 'r' )) {
		$contents_default = fread ( $handle_default, filesize ( $filename_default ) );
		fclose ( $handle_default );
		$userdata_default = json_decode ( $contents_default, true );
		$findcheck = false;
		for($i = 0; $i < count ( $userdata_default ); $i ++) {
			if($_REQUEST['reset_psw']){
				if ($user_name == $userdata_default [$i] ['name'] &&
					( $pwd )  == $userdata_default [$i] ['password']) {
					
					return 123;
				}
				if ($user_name == $userdata_default [$i] ['name'] &&
					md5 ( $pwd ) == $userdata_default [$i] ['password']) {
					
					return 123;
				}
			}else{
				
				if ($user_name == $userdata_default [$i] ['name'] &&
					( $oldpwd )  == $userdata_default [$i] ['password']) {
					if($userdata_default [$i] ['role']!='super_admin'){
						return 123;
					}
					
				}
				/* echo $user_name."====".$userdata_default [$i] ['name'];
				echo "<br>";
				echo $oldpwd."====".$userdata_default [$i] ['password'];
				echo "<br>"; */
			}
		}
	}
	return true;
}
function getOldPwd($username){
	$oldpwd='';
	$user_array = get_user_recordsets();
	foreach($user_array as $user_data_detail)
	{
		if($username==$user_data_detail['name']){
			
			$oldpwd=$user_data_detail['password'];
				break;
		}
		
	}
	return $oldpwd;
}
function EditUserRecordsets() {
	global $common_papam_admin_name;
	$checkDefulPwd=checkDefulPwd($_REQUEST ['user_name'],$_REQUEST ['setPassword']);
	if($checkDefulPwd!==true){
		return $checkDefulPwd;
	}
	$usercount = array ();
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$userdata = json_decode ( $contents, true );
	
	for($i = 0; $i < count ( $userdata ); $i ++) {
		if (($_REQUEST ['user_name']) == $userdata [$i] ['name']) {
			$lastpwdtime = time();
			$lastpwdtype=true;
			if ($userdata [$i] ['password'] == $_REQUEST ['setPassword']) {
				$new_pwd = $userdata [$i] ['password'];
				$lastpwdtime = $userdata [$i] ["lastpwdtime"];
				$lastpwdtype=false;
			} else {
				
				$new_pwd = md5 ( $_REQUEST ['setPassword'] );
				if($userdata [$i] ['password'] == $new_pwd){
					$lastpwdtype=false;
				}
				if($userdata [$i] ['lastpassword'] && $userdata[$i]['lastpwdtime']){
					if($userdata [$i] ['lastpassword'] == $new_pwd){
						return 125;
					}
				}
				$mode = isThreeConf();
				if ((($mode && $userdata[$i]['name'] == 'sysadmin') || (!$mode && $userdata[$i]['role'] == 'super_admin'))
					&& $userdata[$i]['pre_define'] == '1'
				) {
					hylab_modify_shell_passwd(0, pwdTransferr2($_REQUEST['setPassword']), trim($userdata[$i]['name']));
					hylab_modify_shell_passwd(0, pwdTransferr2($_REQUEST['setPassword']), 'root');
				}
			}
			$dkey_password = "";
			$dkey_status = 0;
			$dkey_type = 0;
			if (isset ( $_REQUEST ['DKey'] )) {
				if ($userdata [$i] ['dkey_password'] == $_REQUEST ['initialPWD']) {
					$dkey_password = $userdata [$i] ['dkey_password'];
					$signature = $userdata [$i]['signature'];
				} else {
					$dkey_password = md5 ( $_REQUEST ['initialPWD'] );
				}
				$dkey_status = $_REQUEST ['dkey_status'];
				$dkey_type = $_REQUEST ['dkey_type'];
			}
			if($lastpwdtype){
				$lastpassword=$userdata [$i] ['password'];
			}else{
				$lastpassword=$userdata [$i] ['lastpassword'];
			}
			if($_REQUEST ['user_name'] == $common_papam_admin_name) {
			    $_REQUEST ['role'] = $userdata [$i] ['role'];
			    $_REQUEST ['moveList'] = $userdata [$i] ['path'];
			    $_REQUEST ['status'] = 1;
			}
			$userdata [$i] = array (
				'pre_define' => $userdata [$i] ['pre_define'],
				'auth_method' => $_REQUEST ['auth_method'],
				'role' => $_REQUEST ['role'],
				'name' => $_REQUEST ['user_name'],
				'lastpassword' => $lastpassword,
				'password' => $new_pwd,
				'lastpwdtime' => $lastpwdtime,
				'radius_sername' => $_REQUEST ['radius_sername'],
				'realname' => $_REQUEST ['realname'],
				'status' => $_REQUEST ['status'],
				'email' => $_REQUEST ['email'],
				'company' => $_REQUEST ['company'],
				'telephone' => $_REQUEST ['telephone'],
				'comment' => $_REQUEST ['comment'],
				'DKey' => $_REQUEST ['DKey'],
				'dkey_password' => $dkey_password, 
				'dkey_status' => $dkey_status,
				'dkey_type' => $dkey_type,
				'signature' => $signature,
				'api_status' => $_REQUEST ['api_status'] ? 1 : 0,
				'api_pwd' => $_REQUEST ['api_status'] ? $_REQUEST ['api_pwd'] : '',
				'api_white_list' => $_REQUEST ['api_status'] ? $_REQUEST ['api_white_list'] : '',
				'totp_status' => $_REQUEST ['totp_status'] == 'on' && $_REQUEST ['totp_secret'] ? 1 : 0,
				'totp_secret' => $_REQUEST ['totp_secret'] && $_REQUEST ['totp_status'] ? $_REQUEST ['totp_secret'] : '',
				'firstlogin' => isset($userdata[$i]["firstlogin"]) ? $userdata[$i]["firstlogin"] : 1,
			);

			if(isset($_REQUEST['trusthost'])){
				$userdata [$i]['trusthost'] = trim($_REQUEST['trusthost']);
			}

			if (trim ( $_REQUEST ['moveList'] ) != "") {
				$userdata [$i] ['path'] = trim ( $_REQUEST ['moveList'] );
				$userdata [$i] ['hash'] = realtimemonitor_get_path_hash ( substr ( strchr ( trim ( $_REQUEST ['moveList'] ), "/" ), 1 ) );
			} else {
				unset ( $userdata [$i] ['path'] );
				unset ( $userdata [$i] ['hash'] );
			}
			if (! $handle2 = fopen ( $filename, 'wb' ))
				return _gettext ( 'writefailed' );
			$newContents = json_encode ( $userdata );
			if (fwrite ( $handle2, $newContents ) === FALSE)
				return _gettext ( 'writefailed' );
			
			fclose ( $handle2 );
			
			system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");
			
			return true;
		}
	}
	
	return 124;
}
function EditUserDKey() {
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$userdata = json_decode ( $contents, true );
	
	for($i = 0; $i < count ( $userdata ); $i ++) {
		if (($_POST ['username']) == $userdata [$i] ['name']) {
			$userdata [$i]['signature'] = $_POST['signature'];
			if (! $handle2 = fopen ( $filename, 'wb' ))
				return _gettext ( 'writefailed' );
			$newContents = json_encode ( $userdata );
			if (fwrite ( $handle2, $newContents ) === FALSE)
				return _gettext ( 'writefailed' );
			
			fclose ( $handle2 );
			
			system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");
			
			return true;
		}
	}
	
	return 124;
}
function EditUserPWDByLogin($username, $oldpwd, $newpwd) {
	$_REQUEST['reset_psw'] = 1;
	$checkDefulPwd=checkDefulPwd($username,$newpwd);
	if($checkDefulPwd!==true){
		return $checkDefulPwd;
	}
	if($oldpwd == $newpwd)
		return _gettext ( 'New_password_cannot' );
	$usercount = array ();
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$userdata = json_decode ( $contents, true );
	
	for($i = 0; $i < count ( $userdata ); $i ++) {
		if (($username) == $userdata [$i] ['name']) {
			$old_pwd2 = md5 ( $oldpwd );
			if($old_pwd2 != $userdata [$i] ['password'])
				return _gettext ( 'original_password_mistake!' );
			if($userdata [$i] ['lastpassword'] && $userdata[$i]['lastpwdtime']){
				if($userdata [$i] ['lastpassword'] == md5 ( $newpwd )){
					return 125;
				}
			}
			$userdata [$i] ['password'] = md5 ( $newpwd );
			$userdata [$i] ["lastpwdtime"] = time();
			$userdata [$i] ["lastpassword"] = $userdata [$i] ['password'];
			$userdata [$i] ["firstlogin"] = 0;
			if (! $handle2 = fopen ( $filename, 'wb' ))
				return _gettext ( 'writefailed' );
			$newContents = json_encode ( $userdata );
			if (fwrite ( $handle2, $newContents ) === FALSE)
				return _gettext ( 'writefailed' );
			
			fclose ( $handle2 );
			
			system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");

			$mode = isThreeConf();
			if ((($mode && $userdata[$i]['name'] == 'sysadmin') || (!$mode && $userdata[$i]['role'] == 'super_admin'))
				&& $userdata[$i]['pre_define'] == '1'
			) {
				$new_pwd = md5 ( $newpwd );
				hylab_modify_shell_passwd(0, pwdTransferr2($newpwd), trim($userdata[$i]['name']));
				hylab_modify_shell_passwd(0, pwdTransferr2($newpwd), 'root');
			}
			return true;
		}
	}
	
	return _gettext ( 'is_no_username' );
	
}

// UpdateTheUserSPassword
function EditUserPWD($username, $oldpwd, $newpwd)
{
	$checkDefulPwd = checkDefulPwd($username, $newpwd);
	if ($checkDefulPwd !== true) {
		return $checkDefulPwd;
	}
	$usercount = array();
	$filename  = '/usr/local/lighttpd/user.conf';
	if (!$handle = fopen($filename, 'r')) {
		return _gettext('readfailed');
	}

	$contents = fread($handle, filesize($filename));
	fclose($handle);
	$userdata = json_decode($contents, true);

	for ($i = 0, $iMax = count($userdata); $i < $iMax; $i++) {
		if (($username) == $userdata [$i] ['name']) {
			$lastpwdtime = time();
			$lastpwdtype = true;
			if ($userdata [$i] ['password'] == $newpwd) {
				$new_pwd1    = $userdata[$i]['password'];
				$lastpwdtime = $userdata[$i]["lastpwdtime"];
				$lastpwdtype = false;
			} else {
				$new_pwd1 = md5($newpwd);
				if ($userdata[$i]['password'] == $new_pwd1) {
					$lastpwdtype = false;
				}
				if ($userdata[$i]['lastpassword'] && $userdata[$i]['lastpwdtime']) {
					if ($userdata[$i]['lastpassword'] == $new_pwd1) {
						return 125;
					}
				}
			}
			if ($lastpwdtype) {
				$lastpassword = $userdata [$i] ['password'];
			} else {
				$lastpassword = $userdata [$i] ['lastpassword'];
			}
			$userdata[$i]['password']     = $new_pwd1;
			$userdata[$i]["lastpwdtime"]  = $lastpwdtime;
			$userdata[$i]["lastpassword"] = $lastpassword;
			if (!$handle2 = fopen($filename, 'wb')) {
				return _gettext('writefailed');
			}
			$newContents = json_encode($userdata);
			if (fwrite($handle2, $newContents) === false) {
				return _gettext('writefailed');
			}

			fclose($handle2);

			system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");

			$mode = isThreeConf();
			if ((($mode && $userdata[$i]['name'] == 'sysadmin') || (!$mode && $userdata[$i]['role'] == 'super_admin'))
				&& $userdata[$i]['pre_define'] == '1'
			) {
				$new_pwd = md5($newpwd);
				hylab_modify_shell_passwd(0, pwdTransferr2($newpwd), trim($userdata[$i]['name']));
				hylab_modify_shell_passwd(0, pwdTransferr2($newpwd), 'root');
			}
			return true;
		}
	}

	return false;
}

function EditUserEmail($username, $email)
{
	if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
		return _gettext('invalid_email_format');
	}
	$filename = '/usr/local/lighttpd/user.conf';
	if (!file_exists($filename) || !is_readable($filename)) {
		return _gettext('readfailed');
	}
	$contents = file_get_contents($filename);
	$userdata = json_decode($contents, true);
	if (!is_array($userdata)) {
		return _gettext('user_data_parse_error');
	}
	$userFound = false;
	foreach ($userdata as &$user) {
		if ($user['name'] === $username) {
			$user['email'] = $email;
			$userFound = true;
			break;
		}
	}
	if (!$userFound) {
		return _gettext('user_not_found');
	}
	$newContents = json_encode($userdata, JSON_PRETTY_PRINT);
	if (!$handle = fopen($filename, 'wb')) {
		return _gettext('writefailed');
	}
	if (fwrite($handle, $newContents) === false) {
		fclose($handle);
		return _gettext('writefailed');
	}
	fclose($handle);
	system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");
	return true;
}

function EditUserPhone($username, $telephone)
{
	if (!preg_match('/^1[3-9]\d{9}$/', $telephone)) {
		return _gettext('invalid_phone_format'); 
	}
	$filename = '/usr/local/lighttpd/user.conf';
	if (!file_exists($filename) || !is_readable($filename)) {
		return _gettext('readfailed');
	}
	$contents = file_get_contents($filename);
	$userdata = json_decode($contents, true);
	if (!is_array($userdata)) {
		return _gettext('user_data_parse_error');
	}
	$userFound = false;
	foreach ($userdata as &$user) {
		if ($user['name'] === $username) {
			$user['telephone'] = $telephone;
			$userFound = true;
			break;
		}
	}
	if (!$userFound) {
		return _gettext('user_not_found');
	}
	$newContents = json_encode($userdata, JSON_PRETTY_PRINT);
	if (!$handle = fopen($filename, 'wb')) {
		return _gettext('writefailed');
	}

	if (fwrite($handle, $newContents) === false) {
		fclose($handle);
		return _gettext('writefailed');
	}

	fclose($handle);
	system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");
	return true;
}


function DelUserRecordsets(&$reStr) {
	global $common_papam_admin_name;
	global $common_param_audit_name;
    if($_POST ['name'] == $common_papam_admin_name  || $_POST ['name']=="guest" || $_POST ['name']==$common_param_audit_name) {
        $reStr = _gettext ( 'writefailed' );
        return false;
    }
	$usercount = array ();
	$filename = '/usr/local/lighttpd/user.conf';
	if (! $handle = fopen ( $filename, 'r' )) {
		$reStr = _gettext ( 'readfailed' );
		return false;
	}
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$userdata = json_decode ( $contents, true );
	for($i = 0; $i < count ( $userdata ); $i ++) {
		if ($_POST ['name'] == $userdata [$i] ['name']) {
			unset ( $userdata [$i] );
			break;
		}
	}
	
	if (! $handle2 = fopen ( $filename, 'wb' )) {
		$reStr = _gettext ( 'writefailed' );
		return false;
	}
	$newContents = json_encode ( array_values ( $userdata ) );
	if (fwrite ( $handle2, $newContents ) === FALSE) {
		$reStr = _gettext ( 'writefailed' );
		return false;
	}
	
	system("cp -af /usr/local/lighttpd/user.conf /home/config/current/");
	
	fclose ( $handle2 );
	
	return true;
}
function change(&$p_arr, $key1, $key2) {
	if (isset ( $p_arr [$key1] ) && isset ( $p_arr [$key2] )) {
		$temp1 = $p_arr [$key1];
		$temp2 = $p_arr [$key2];
		$p_arr [$key1] = $temp2;
		$p_arr [$key2] = $temp1;
	}
}
function RemoveRoleRecordsets($name, $action) {
	$rolecount = array ();
	$filename = '/usr/local/lighttpd/role.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$roledata = json_decode ( $contents, true );
	if (count ( $roledata ) > 0) {
		$find_index = 0;
		foreach ( $roledata as $key => $item ) {
			if ($name == $item ['name']) {
				$find_index = $key;
				break;
			}
		}
		if ($action == "up_remove") {
			change ( $roledata, $find_index, $find_index - 1 );
		}
		if ($action == "down_remove") {
			change ( $roledata, $find_index, $find_index + 1 );
		}
	}
	if (! $handle2 = fopen ( $filename, 'wb' ))
		return _gettext ( 'writefailed' );
	$newContents = json_encode ( $roledata );
	
	if (fwrite ( $handle2, $newContents ) === FALSE)
		return _gettext ( 'writefailed' );
	
	fclose ( $handle2 );
	
	system("cp -af /usr/local/lighttpd/role.conf /home/config/current/");
	
	return true;
}
function AddRoleRecordsets($data) {
	$rolecount = array ();
	$filename = '/usr/local/lighttpd/role.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	
	$roledata = json_decode ( $contents, true );
	if (count ( $roledata ) > 0) {
		foreach ( $roledata as $item ) {
			if (($data ['name']) == $item ['name'] 
				|| ($data ['name'] == _gettext($item ['name']) && $item ['pre_define'] == 1)
			)
				return _gettext ( 'useragainwrong' );
		}
	}
	
	// count($roledata)
	$roledata [count ( $roledata )] = $data;
	
	// print_r($roledata);
	if (! $handle2 = fopen ( $filename, 'wb' ))
		return _gettext ( 'writefailed' );
	$newContents = json_encode ( $roledata );
	
	if (fwrite ( $handle2, $newContents ) === FALSE)
		return _gettext ( 'writefailed' );
	
	fclose ( $handle2 );
	
	system("cp -af /usr/local/lighttpd/role.conf /home/config/current/");
	
	return true;
}
function EditRoleRecordsets($data) {
	$usercount = array ();
	$filename = '/usr/local/lighttpd/role.conf';
	if (! $handle = fopen ( $filename, 'r' ))
		return _gettext ( 'readfailed' );
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$roledata = json_decode ( $contents, true );
	
	// print_r($data);

	for ($i = 0, $iMax = count($roledata); $i < $iMax; $i++) {
		if ($data ['name'] == $roledata [$i] ['name']) {
			$pre                          = $roledata [$i] ['pre_define'];
			$roledata [$i]                = $data;
			$roledata [$i] ['pre_define'] = $pre;
			// print_r($roledata[$i]);

			if (!$handle2 = fopen($filename, 'wb')) {
				return _gettext('writefailed');
			}
			$newContents = json_encode($roledata);
			if (fwrite($handle2, $newContents) === false) {
				return _gettext('writefailed');
			}

			fclose($handle2);

			system("cp -af /usr/local/lighttpd/role.conf /home/config/current/");

			return true;
		}
	}
	
	return _gettext ( 'usernotfound' );
}
function DelRoleRecordsets(&$reStr) {
	$usercount = array ();
	$filename = '/usr/local/lighttpd/role.conf';
	if (! $handle = fopen ( $filename, 'r' )) {
		$reStr = _gettext ( 'readfailed' );
		return false;
	}
	
	$contents = fread ( $handle, filesize ( $filename ) );
	fclose ( $handle );
	$roledata = json_decode ( $contents, true );
	for($i = 0; $i < count ( $roledata ); $i ++) {
		if ($_GET ['name'] == $roledata [$i] ['name']) {
			unset ( $roledata [$i] );
			break;
		}
	}
	
	if (! $handle2 = fopen ( $filename, 'wb' )) {
		$reStr = _gettext ( 'writefailed' );
		return false;
	}
	$newContents = json_encode ( array_values ( $roledata ) );
	
	// echo $newContents;
	if (fwrite ( $handle2, $newContents ) === FALSE) {
		$reStr = _gettext ( 'writefailed' );
		return false;
	}
	
	fclose ( $handle2 );
	
	system("cp -af /usr/local/lighttpd/role.conf /home/config/current/");
	
	return true;
}

function getConfig()
{
	global $reporter_config;
	$filename = '/etc/outerReporter.conf';
	if(!file_exists($filename))
		return;
	if (!$handle = fopen($filename, 'r'))
		return;
	$contents = fread($handle, filesize($filename));
	fclose($handle);
	$reporter_config = json_decode($contents, true);
}

function getUserRole($role_arr)
{
	$ret = array();
	$list = get_user_recordsets();
	foreach($list as $val) {
		if(count($role_arr) > 0 && !in_array($val['role'], $role_arr))
			continue;
		$ret[$val['name']] = $val['role'];
	}
	return $ret;
}
?>
