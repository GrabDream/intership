<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = 'm_systemadmin';
include_once ($_SERVER ["DOCUMENT_ROOT"] . "/authenticed_writable.php");

$mode =  isThreeConf();

if($mode && $_SESSION['regRole'] == $three_conf_secadmin_role) {
	//通用三权模式 secadmin 操作
}
elseif ( 0 != strcmp($_SESSION['regRole'],'super_admin')) {
    exit("error");
}

$pwLisenable_detail = array();
$is_bmj = isHylabBmj();
if ($is_bmj){
    include_once("user_functions.php");
    $pwLisenable_detail = get_password_custom_length();
}
$pwdpolicy = hytf_pwdpolicy_get ();
$pwdpolicy_format = 'Lisenable/Lpwdvaliday/Lmembernum';
$pwdpolicy_detail = unpack ( $pwdpolicy_format, $pwdpolicy );
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>set pwd policy</title>
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<style type="text/css">
    .password_length label{margin-right: 0 !important;}
    .password_length input{margin-right: 0 !important;max-width: 50px;}
</style>
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
<script type="text/javascript" src="/js/WdatePicker.js"></script>
<script type="text/javascript" src="/js/qin.js"></script>
<script type="text/javascript">
Event.observe(window,'load',function(){
    new ValidateForm('form1',[checkEmpty,checkNum365,checkNum7]);
	checkEnable();
});

function checkNum365(idForm)
{
	var isNum365=$$('#'+idForm+' .isNum365');
	var arr=[];
	var rg=/^[0-9]\d{0,2}$/;
	if(isNum365.length==0) return ;
	for(var i=0;i<isNum365.length;i++)
	{
		if(isNum365[i].value!="" &&
			isNum365[i].disabled==false &&
			!(rg.test(isNum365[i].value)?((isNum365[i].value>=1 && isNum365[i].value<=365)?true:false):false))
		{

			arr.push(isNum365[i]);
		}
	}
	return arr;
}

function checkNum7(idForm)
{
    var isNum7=$$('#'+idForm+' .isNum7');
    var arr=[];
    var rg=/^[0-9]\d{0,2}$/;
    if(isNum7.length==0) return ;
    for(var i=0;i<isNum7.length;i++)
    {
        if(isNum7[i].value!="" &&
            isNum7[i].disabled==false &&
            !(rg.test(isNum7[i].value)?((isNum7[i].value>=1 && isNum7[i].value<=7)?true:false):false))
        {
            arr.push(isNum7[i]);
        }
    }
    return arr;
}

function checkEnable(){
	if($("isenable").checked == true){
		$("pwdvaliday").disabled=false;
	}else{
		$("pwdvaliday").disabled=true;
	}
    if($("pwLisenable").checked == true){
        $("pwdlength_up").disabled=false;
        $("pwdlength_down").disabled=false;
    }else{
        $("pwdlength_up").disabled=true;
        $("pwdlength_up").value = 15;
        $("pwdlength_down").disabled=true;
        $("pwdlength_down").value = 8;
    }
}

function pwdlength_down_input() {
    let down_d = parseInt(document.getElementById("pwdlength_down").value) || 8;
    let up_d = parseInt(document.getElementById("pwdlength_up").value) || 15;

    if (down_d < 8) {
        down_d = 8;
    } else if (down_d >= up_d) {
        down_d = up_d - 1;
    } else if (down_d > 32) {
        down_d = Math.min(32, up_d - 1);
    }

    document.getElementById("pwdlength_down").value = down_d;
    // console.log("Updated pwdlength_down:", down_d);
}

function pwdlength_up_input() {
    let down_u = parseInt(document.getElementById("pwdlength_down").value) || 8;
    let up_u = parseInt(document.getElementById("pwdlength_up").value) || 15;

    if (up_u > 32) {
        up_u = 32;
    } else if (up_u <= down_u) {
        up_u = 15;
    } else if (up_u < 15) {
        up_u = 15;
    }

    document.getElementById("pwdlength_up").value = up_u;
    // console.log("Updated pwdlength_up:", up_u);
}

</script>
</head>
<body class="body">
	<div id="fld_main">
		<form name="form1" id="form1" method="post"
			action="pwdSafePolicy_commit.php">
			<input type="hidden" name="tokenid" value="<?=$tokenid?>"/>
			<div class="operate">
<div class="title"><?=_gettext('password_security_policy');?></div>
    <div class="operate_table">
			<table class="list wtwo">

				<tbody>
					<tr id="pwd_role">
						<th class="name"></th>
						<td class="password_length" style="line-height:22px;">
							<?=_gettext("must_password_format"); ?>：</br>
							1、<?=_gettext("Password_length"); ?>
                            <?php if($is_bmj){?>
                            <input type="checkbox" name="pwLisenable" id="pwLisenable" onClick="checkEnable()" value="1" <?php if($pwLisenable_detail["pwLisenable"] != 0) echo "checked"; ?> />&nbsp;<label for="pwLisenable" ><?=_gettext('password_custom_length');?></label>：
                            <input type="number" min="8" max="32" name='pwdlength_down' id='pwdlength_down' onchange="pwdlength_down_input()"
                                   class="text isNotEmpty" msg="<?=_gettext('pwdlength_down');?>" value="<? if($pwLisenable_detail["pwdlength_down"]==''){echo 8;}else{echo $pwLisenable_detail["pwdlength_down"];} ?>" tabindex="2" />
                            ~
                            <input type="number" min="8" max="32" name='pwdlength_up' id='pwdlength_up' onchange="pwdlength_up_input()"
                                   class="text isNotEmpty" msg="<?=_gettext('pwdlength_up');?>" value="<? if($pwLisenable_detail["pwdlength_up"]==''){echo 15;}else{echo $pwLisenable_detail["pwdlength_up"];} ?>" tabindex="2" />
                            <?=_gettext('password_custom_info_1');?>
                            <?php }?>
                            </br>
							2、<?=_gettext("password_not_default"); ?></br>
							3、<?=_gettext("password_not_ruo"); ?>
						</td>
					 </tr>
						<th class="name"></th>
						<td >
							<div class="flexAlign">
							<span class="divs"><?=_gettext("password_rules"); ?></span>
							<input type="radio" name="pwdrule" id="pwdrule1" <?php if($pwdpolicy_detail["membernum"] == 4) echo "checked"; ?>  value="4">
							<label  for="pwdrule1"><?=_gettext("least_four"); ?></label>
							<input type="radio" name="pwdrule"  id="pwdrule2" <?php if($pwdpolicy_detail["membernum"] == 3) echo "checked"; ?> value="3">
							<label for="pwdrule2"><?=_gettext("least_three"); ?></label>
							<input type="radio" name="pwdrule" id="pwdrule3" <?php if($pwdpolicy_detail["membernum"] == 2) echo "checked"; ?>  value="2">
							<label for="pwdrule3"><?=_gettext("least_two").($is_bmj?'（'._gettext('number_alphabet').'）':''); ?></label>
							</div>
						</td>
					</tr>
					<tr>
						<th><font color="red">*</font></th>
						<td>
							<input type="checkbox" name="isenable" id="isenable" onClick="checkEnable()" value="1" <?php if($pwdpolicy_detail["isenable"] != 0) echo "checked"; ?> />
							<label for="isenable"><?=_gettext('Password_longest_duration');?>：</label>
                            <?php if(!$is_bmj){ ?>
                                <input type="text" name='pwdvaliday' id='pwdvaliday' class="text isNotEmpty isNum365" msg="<?=_gettext('password_days');?>" value="<? if($pwdpolicy_detail["pwdvaliday"]==''){echo 30;}else{echo $pwdpolicy_detail["pwdvaliday"];} ?>" tabindex="2" />(1-365)
                            <?php }else{ ?>
                                <input type="text" name='pwdvaliday' id='pwdvaliday' class="text isNotEmpty isNum7" msg="<?=_gettext('password_days_7');?>" value="<? if($pwdpolicy_detail["pwdvaliday"]==''){echo 7;}else{echo $pwdpolicy_detail["pwdvaliday"];} ?>" tabindex="2" />(1-7)
                            <?php } ?>
						</td>
					</tr>
					<tr>
						<th></th>
						<td class="operateBtn">
							<input type="submit" class="btn_ok" value="<?=_gettext("OK");?>"  />
							<a href="user.php" class="btn_return"><?=_gettext("Return");?></a>
						</td>
					</tr>
				</tbody>
			</table>
				</div>
					</div>
		</form>
	</div>
</body>
</html>
