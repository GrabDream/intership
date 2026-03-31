<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed.php");
include_once("user_functions.php");
require_once($_SERVER["DOCUMENT_ROOT"] . "/model/common_param.php");
isAvailable('m_systemadmin');
$user_array = get_user_recordsets();
$username = urldecode($_GET['name']);
$role_data=showRole();
$true=false;
$mod_true=false;

$mode = isThreeConf();

$userAuthMethod = [1, 2, 3, 4];

$auth_method = 1;
foreach ($user_array as $user_data_detail) {
    if ($username == $user_data_detail['name']) {
        $auth_method = $user_data_detail['auth_method'];
        break;
    }
}


if($mode && $_SESSION['regUser'] == 'sysadmin') {
	//通用三权模式 sysadmin 操作
} elseif ( 0 == strcmp($_SESSION['regUser'],$common_papam_admin_name)){
	if($action=='delete' && ( $username==$common_papam_admin_name || $_POST['user_name']==$common_param_audit_name || $_POST['user_name']=='guest')){
		echo "<script>alert('"._gettext('This operation is illegal')."');location.href='user.php'</script>";
		die();
	}
}else{
	if(is_array($role_data)){
		foreach($role_data as $val){
			if (strcmp($username, $val['name']))
			{
				$true=false;
			}else{
				$true=true;
				$mod_true=$val['modify'];
				break;
			}
		}
		if(!$true){
			echo "<script>alert('"._gettext('This operation is illegal')."');location.href='user.php'</script>";
			die();
		}
		if($mod_true==2){
			if($action=='add' || $action=='delete'){
				echo "<script>alert('"._gettext('This operation is illegal')."');location.href='user.php'</script>";
				die();
			}
		}else{
			if($action=='delete' && ( $username==$common_papam_admin_name || $username==$common_param_audit_name || $username=='guest')){
				echo "<script>alert('"._gettext('This operation is illegal')."');location.href='user.php'</script>";
				die();
			}
		}
	}
}
	foreach($user_array as $user_data_detail)
	{
		if($username==$user_data_detail['name'])
			break;
	}
if($_SESSION['token']){
	$token=$_SESSION['token'];
}else{
	$token = md5(uniqid(rand(), TRUE));
	$_SESSION['token'] = $token;
}
$pwdpolicy = hytf_pwdpolicy_get ();
$pwdpolicy_format = 'Lisenable/Lpwdvaliday/Lmembernum';
$pwdpolicy_detail = unpack ( $pwdpolicy_format, $pwdpolicy );

$is_defect = isDefect(18) ? 1 : 0;

$pwdlength_down = 8;
$pwdlength_up = 15;
$pwd_spchr = '@ # % ^ * - _ . ';
$is_bmj = isHylabBmj();
if ($is_bmj){
    include_once("user_functions.php");
    $detail = get_password_custom_length();
    if ($detail['pwLisenable'] == 1){
        $pwdlength_down = $detail['pwdlength_down'] ?: 8;
        $pwdlength_up = $detail['pwdlength_up'] ?: 15;
    }
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>addUser</title>
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<link rel="StyleSheet" href="/css/dtree.css" type="text/css" />
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
<script type="text/javascript" src="/js/crypto-js.js"></script>
<style type="text/css">

</style>
<script type="text/javascript">
	var formsubmit;
Event.observe(window,'load',function(){
   formsubmit=new ValidateForm('form1',[checkEmpty,checkPwd2,checkPassWord],'','','',checkEncode);
	Reset_PSW();
});


/*ÃÜÂë³¤¶È8-15¡£¸ù¾ÝÃÜÂë²ßÂÔ°üº¬ÀàÐÍ£¬²»ÆôÓÃÔòÖ§³ÖÈÎÒâ£¬²»Ö§³ÖÈ«½Ç×Ö·û*/
function checkPassWord(idForm){
	var isPassWord = $$('#'+idForm+' .isPassWord');
	var arr = [];
    var rg = /^[0-9a-z\~\!\@\#\$\%\^\&\*\-\+\,\.\;\|\>\<]{<?=$pwdlength_down; ?>,<?=$pwdlength_up; ?>}$/i;
    var rg_word_dx = /[A-Z]+/;
	var rg_word_xx= /[a-z]+/;
    var rg_word_dx_xx = /[a-zA-Z]+/;
	var rg_num = /[0-9]+/;
	var rg_char = /[\~\!\@\#\$\%\^\&\*\-\+\,\.\;\|\>\<]+/;
	// The special requirements of the Confidentiality Bureau require passwords to be at least numbers and letters
    var is_bmj = "<?=$is_bmj?>";
    var chkNum = "<?=$pwdpolicy_detail['membernum']?>";
    if(isPassWord.length == 0){return;}
	for(var i = 0;i < isPassWord.length;i++){
		if (isPassWord[i].value != "" && isPassWord[i].disabled == false ){
			if(isPassWord[i].value == '<?=$user_data_detail['password'];?>' && isPassWord[i].value.length >= 8){
				return true;
			}
			if(!rg.test(isPassWord[i].value) ) {
				arr.push(isPassWord[i]);
			} else {
			    if (is_bmj && chkNum == 2){
                    var ret1 = rg_num.test(isPassWord[i].value);
                    var ret2 = rg_word_dx_xx.test(isPassWord[i].value);
                    var num=ret1+ret2;
                }else {
                    var ret1 = rg_word_dx.test(isPassWord[i].value);
                    var ret2 = rg_word_xx.test(isPassWord[i].value);
                    var ret3 = rg_num.test(isPassWord[i].value);
                    var ret4 = rg_char.test(isPassWord[i].value);
                    var num=ret1+ret2+ret3+ret4;
                }
				<?php if($pwdpolicy_detail["membernum"]) { ?>
					if(num<chkNum){
						arr.push(isPassWord[i]);
					}
				<?php }else{?>
					if(num<=1){
						arr.push(isPassWord[i]);
					}
				<?php }?>
			}
		}
	}
	return arr;
}

function checkPwd2(idForm)
{
   var isPwd = $$('#'+idForm+' .isPwd');
   var pwd1=$('setPassword').value;
   var pwd2=$('confirmPassword').value;

   var arr = [];
	if(isPwd.length == 0) return;
	for(var i = 0;i < isPwd.length;i++){
		if(isPwd[i].value != "")
		{
		 if(pwd1!=pwd2)
		   {
				arr.push(isPwd[i]);
		   }
		}
	}
		return arr;
}

function checkEncode() {
	<?php
		if (!isset($_SESSION['token']))
		{
	?>
			return true;
	<?php
		}
	?>
	//return;

	var encrypt_key = "<?php
		if (isset($_SESSION['token'])) {
			echo substr($_SESSION['token'], 0, 16);
		} else {
			echo "1234567887654321";
		}
		?>";

	var password = document.getElementById("setPassword").value;
	if (password) {
		password = crypto_encrypt(password, encrypt_key);
		document.getElementById("setPassword").value = password;
	}

	var password = document.getElementById("confirmPassword").value;
	if (password) {
		password = crypto_encrypt(password, encrypt_key);
		document.getElementById("confirmPassword").value = password;
	}


	return true;
}
function Reset_PSW(){
    var resetPswEl = $('reset_psw');
    var oldPasswordEl = $('oldPassword');
    var setPasswordEl = $('setPassword');
    var confirmPasswordEl = $('confirmPassword');
    var trSetPasswordEl = $('tr_setPassword');
    var trConfirmPasswordEl = $('tr_confirmPassword');
	var is_defect = '<?=$is_defect?>' == '1' ? 1 : 0;
    
    // 检查必要的元素是否存在
    if (!resetPswEl || !setPasswordEl || !confirmPasswordEl || 
        !trSetPasswordEl || !trConfirmPasswordEl) {
        return;
    }
    
    if(resetPswEl.checked){
        trSetPasswordEl.style.display = "";
        trConfirmPasswordEl.style.display = "";
		var class_name = is_defect == 1 ? 'isNotEmpty' : 'isNotEmpty isPassWord';
        setPasswordEl.className = class_name;
        confirmPasswordEl.className = "isNotEmpty isPwd";
    }else{
        trSetPasswordEl.style.display = "none";
        trConfirmPasswordEl.style.display = "none";
        setPasswordEl.className = "";
        confirmPasswordEl.className = "";
    }
}
function check_pwd(){
	var result = 1;
	if($('reset_psw').checked){
		// const vv=formsubmit.validate();
		// console.log("name",vv);
		var encrypt_key = "<?php
		if (isset($_SESSION['token'])) {
			echo substr($_SESSION['token'], 0, 16);
		} else {
			echo "1234567887654321";
		}
		?>";
		var setPassword=crypto_encrypt($("setPassword").value, encrypt_key);
		var confirmPassword=crypto_encrypt($("confirmPassword").value, encrypt_key);
		var url = 'checkpwd.php?setPassword='+setPassword+'&confirmPassword='+confirmPassword+'&randtime='+Math.random();
		var result = true;
		 var myAjax =new Ajax.Request(url,{
			asynchronous:false,
			onSuccess:function(resp){
				if(resp.responseText== 0){
					alert('<?=_gettext("pwd_ruo");?>!');
					$("setPassword").value='';
					$("confirmPassword").value='';
					result=false;
				}else{
					result=true;
				}
			},
			onFailure:function(){
				result=false;
			}
		});
		return result;
	}
}
</script>
</head>
<?php
$parent_id_array = array();
$id_array = array();
$name_array = array();

manage_frame_get_tree_group_simple($_SESSION['regUserPath'], $parent_id_array, $id_array, $name_array);


include_once($_SERVER["DOCUMENT_ROOT"]."/model/tree.php");//tree

?>
<body class="body">
<div id="fld_main">
	<form name="form1"	id="form1" method="post" action="user_commit.php?action=editpasswd">
	<input type="hidden" name="tokenid" value="<?=$tokenid?>"/>
		<input type="hidden" name="encrypt" value="1" />
		<input type="hidden" name="token" value="<?php echo $token; ?>" />
		<div class="operate">
<div class="title"><?=_gettext('editsystemuser');?></div>
    <div class="operate_table">
		<table class="list wtwo">

			<tbody>
				<tr>
					<th  ><em class="xing">*</em> <?=_gettext("User name");?></th>
					<td ><?=$username;?>
					<input type="hidden" name='user_name' id='userName'  value="<?=$username;?>" />
					<input type="hidden" name='pre_define' value="<?=$user_data_detail['pre_define'];?>" />
					</td>
				</tr>
				<?php  if (in_array($user_data_detail["auth_method"], $userAuthMethod)){ ?>
				<tr>
					<th>
						<?=_gettext("resetpwd");?>
					</th>
					<td >
						<input type="checkbox" id="reset_psw" name="reset_psw" onclick="Reset_PSW()" />
					</td>
                </tr>
                    <tr id="tr_setPassword">
                        <th><em class="xing">*</em> <?= _gettext("set_password"); ?></th>
                        <td>
						<span style="float:left; width:200px">
                            <?php if(!$is_bmj){?>
                                <input type="password" class="text isNotEmpty <?php if (!$is_defect) { echo 'isPassWord';} ?>" msg="
                                <?php if ($is_defect) {
                                    echo _gettext("pwd_nonull");
                                } else if ($pwdpolicy_detail['membernum'] == 4) {
                                    echo _gettext("userpass_length_error_4");
                                } else {
                                    if ($pwdpolicy_detail['membernum'] == 3) {
                                        echo _gettext("userpass_length_error_3");
                                    } else {
                                        if ($pwdpolicy_detail['membernum'] == 2) {
                                            echo _gettext("userpass_length_error_0");
                                        }
                                    }
                                } ?>" name='setPassword' id='setPassword'
                                       value="" tabindex="2"/>
                            <?php }else{ if ($pwdpolicy_detail['membernum'] != 2){ ?>
                                <input type="password" class="text isNotEmpty isPassWord" msg="<?=_gettext("custom_userpass_length_error_1") . $pwdlength_down . "~" . $pwdlength_up . _gettext("custom_userpass_length_error_2") . $pwdpolicy_detail["membernum"] . _gettext("custom_userpass_length_error_3") . $pwd_spchr . "]。"; ?>" name='setPassword' id='setPassword' value="" tabindex="2"/>
                                <?php }else{ ?>
                                <input type="password" class="text isNotEmpty isPassWord" msg="<?=_gettext("custom_userpass_length_error_1") . $pwdlength_down . "~" . $pwdlength_up . _gettext("custom_userpass_length_error_2_two"); ?>" name='setPassword' id='setPassword' value="" tabindex="2"/>
                            <?php }} ?>
						</span>
                            <input type="password" class="hide" name='oldPassword' id='oldPassword' value=""/>
                        </td>
                    </tr>

				<tr id="tr_confirmPassword">
					<th><em class="xing">*</em> <?=_gettext("suepassword");?></th>
					<td >
						<input type="password" class="text isNotEmpty isPwd" msg="<?=_gettext("suepassword_null");?>" name='confirmPassword' id='confirmPassword'  value=""    tabindex="4" />
					</td>
				</tr>
				<?php }?>

                <tr id="tr_emailAddress1" style="<?= ($auth_method == 3) ? 'display:table-row;' : 'display:none;'; ?>">
                    <th><em class="xing">*</em><?= _gettext("mail_address"); ?></th>
                    <td>

                        <input
                                type="text"
                                name='email'
                                id='emailAddress1'
                                class="text <?= ($auth_method == 3) ? 'isNotEmpty' : ''; ?> email"
                                msg="<?= _gettext("mail_address_error1"); ?>"
                                value="<?= $user_data_detail['email']; ?>"
                                tabindex="14"
                        />
                    </td>
                </tr>
                
                <tr id="tr_telephone" style="<?= ($auth_method == 4) ? 'display:table-row;' : 'display:none;'; ?>">
                    <th><font color="red">*</font><?= _gettext("telephone"); ?></th>
                    <td>

                        <input
                                type="text"
                                name='telephone'
                                id='phone'
                                class="text <?= ($auth_method == 4) ? 'isNotEmpty' : ''; ?> isphone"
                                msg="<?= _gettext("telephone_error"); ?>"
                                value="<?= $user_data_detail['telephone']; ?>"
                                tabindex="14"
                        />
                    </td>
                </tr>
                
				<tr>
					<th><?=_gettext('role');?></th>
					<td ><?=_gettext($user_data_detail['role'])?></td>
				</tr>
				<tr>
					<th><?=_gettext('GroupBelonged');?></th>
				  <td  >
				  	<?=$user_data_detail['path']?>
					</td>
				</tr>
                <tr>
                    <th></th>
                    <td class="baselines">
                        <?php if (in_array($user_data_detail["auth_method"], $userAuthMethod)) { ?>
                            <input type="submit" class="btn_ok" onclick="return check_pwd()" value="<?= _gettext("OK"); ?>"/>
                        <?php } ?>
                        <a href="user.php" class="btn_return"><?= _gettext("Return"); ?></a>
                    </td>
                </tr>
			</tbody>
		</table>
		</div>
		</div>
	</form>
</div>

<script type="text/javascript">
	function chkpwd(obj){

		var t=obj.value;
		var id=getResult(t);

		var msg=new Array(4);
		msg[0]="<?=_gettext('pwd_point1');?>";
		msg[1]="<?=_gettext('pwd_point2');?>";
		msg[2]="<?=_gettext('pwd_point3');?>";
		msg[3]="<?=_gettext('pwd_point4');?>";
		msg[4]=""

		var sty=new Array(4);
		sty[0]=-45;
		sty[1]=-30;
		sty[2]=-15;
		sty[3]=0;
		sty[4]=-100;

		var col=new Array(4);
		col[0]="gray";
		col[1]="red";
		col[2]="#ff6600";
		col[3]="Green";
		col[4]="#6666FF";

		var bImg="/images/pwdCheck.gif";
		var sWidth=300;
		var sHeight=15;
		// var Bobj=document.getElementById("chkResult");

		/* if(id==4)
		{
		  Bobj.style.width="150px";
		  Bobj.style.height="15px";
		  Bobj.style.background-color="#EBEBEB";
		}*/

		// Bobj.style.fontSize="12px";
		// Bobj.style.color=col[id];
		// Bobj.style.width=sWidth + "px";
		// Bobj.style.height=sHeight + "px";
		// Bobj.style.lineHeight=sHeight + "px";
		// Bobj.style.background="url(" + bImg + ") no-repeat left " + sty[id] + "px";
		// Bobj.style.textIndent="20px";
		// Bobj.innerHTML= msg[id];


	}

	function getResult(s){
	    if(s.length==0)
		{
		   return 4;
		}
		if(s.length < 4&& s.length >0){
			return 0;
		}
		var ls = 0;
		if (s.match(/[a-z]/ig)){
			ls++;
		}
		if (s.match(/[0-9]/ig)){
			ls++;
		}
	 	if (s.match(/(.[^a-z0-9])/ig)){
			ls++;
		}
		if (s.length < 6 && ls > 0){
			ls--;
		}
		return ls;

	}
</script>

</body>
</html>
