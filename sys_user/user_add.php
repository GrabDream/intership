<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = 'm_systemadmin';
include_once($_SERVER["DOCUMENT_ROOT"] . "/authenticed_writable.php");
include_once("user_functions.php");
$parent_id_array = array();
$id_array = array();
$name_array = array();
manage_frame_get_tree_group_simple($_SESSION['regUserPath'], $parent_id_array, $id_array, $name_array);
$pwdpolicy = hytf_pwdpolicy_get();
$pwdpolicy_format = 'Lisenable/Lpwdvaliday/Lmembernum';
$pwdpolicy_detail = unpack($pwdpolicy_format, $pwdpolicy);
$token = md5(uniqid(rand(), true));
$_SESSION['token'] = $token;
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
    <title>addUser</title>
    <link href="/css/common.css" rel="stylesheet" type="text/css"/>
    <link href="/css/skin.css" rel="stylesheet" type="text/css"/>
    <link rel="StyleSheet" href="/css/dtree.css" type="text/css"/>
    <link rel="StyleSheet" href="/css/newgroup.css" type="text/css"/>
    <script type="text/javascript" src="/js/prototype.js"></script>
    <script type="text/javascript" src="/js/base.js"></script>
    <script type="text/javascript" src="/js/dtreeGroup.js"></script>
    <script type="text/javascript" src="/js/qin.js"></script>
    <script type="text/javascript" src="/js/popup.js"></script>
    <script type="text/javascript" src="/js/WdatePicker.js"></script>
    <script type="text/javascript" src="/js/treeGroup_checkbox.js"></script>
    <script type="text/javascript" src="/js/crypto-js.js"></script>
    <style type="text/css">
        .divs {
            width: 110px;
        }

        #initialPWD2, #initialPWD {
            width: 164px;
        }


    </style>
    <script type="text/javascript">
        Event.observe(window, 'load', function () {
            new ValidateForm('form1', [checkEmpty,checkPhone, checkIp2, checkEmailto, checkNameCH, checkNum, checkPwd2,checkPwd1, checkPassWord, checkHost], '', '', '', checkEncode);
            clickDefault('default_cache', ['<?=_gettext("All");?>'], true);
            Event.observe($("systemrole"), 'click', showIframe1);
            Event.observe($("systemmail"), 'click', showIframe2);
            Event.observe($("systemsms"), 'click', showIframe3);
            regDateInput();
            dispHide();
            apichange();
            showTotp();
        });
        function apichange() {
            var selval = document.getElementById("api_status").value;
            if(selval == '1') {
                document.getElementById("api_signature").style.display="";
                document.getElementById("api_trust").style.display="";
			    new dispCheck(['api_pwd'], [['isNotEmpty']]);
                new dispCheck(['api_white_list'], [['isIp2', 'isNotEmpty']]);
            } else {
                document.getElementById("api_signature").style.display="none";
                document.getElementById("api_trust").style.display="none";
			    new hideCheck(['api_pwd'], [['isNotEmpty']]);
                new hideCheck(['api_white_list'], [['isIp2', 'isNotEmpty']]);

            }
        } 
        var showUserrole;
        // 弹出显示系统角色
        function showIframe1() {
            showUserrole = new Popup({contentType: 1, scrollType: 'yes', isReloadOnClose: 'role', width: '1000', height: '600'});
            showUserrole.setContent("contentUrl", "/view/systemConfig/sys_user/user_role.php");
            showUserrole.setContent("title", "<?=_gettext('m_systemrole');?>");
            showUserrole.build();
            showUserrole.show();
        }
        
        // 弹出邮箱配置
        function showIframe2() {
            var pop = new Popup({contentType: 1, scrollType: 'yes', isReloadOnClose: 'role', width: '1000', height: '600'});
            pop.setContent("contentUrl", "/view/systemConfig/mailConfig/mailinfo.php");
            pop.setContent("title", "<?=_gettext('mailConfig');?>");
            pop.build();
            pop.show();
        }
        
        // 弹出短信配置
        function showIframe3() {
            var pop = new Popup({contentType: 1, scrollType: 'yes', isReloadOnClose: 'role', width: '1000', height: '600'});
            pop.setContent("contentUrl", "/view/systemConfig/smsConfig/list.php");
            pop.setContent("title", "<?=_gettext('m_smsconfig');?>");
            pop.build();
            pop.show();
        }

        function checkEmailto(idForm) {
            var isEmailto = $$('#' + idForm + ' .isEmailto');
            var arr = [];
            if (isEmailto.length == 0) return;
            var rg = /^([a-zA-Z0-9_.-])+@(([a-zA-Z0-9-])+.)+([a-zA-Z0-9]{2,4})+$/;
            for (var i = 0; i < isEmailto.length; i++) {
                if (isEmailto[i].value != "" && isEmailto[i].disabled == false && !rg.test(isEmailto[i].value)) {
                    arr.push(isEmailto[i]);
                }
            }
            return arr;
        }

        /*密码长度8-15，根据密码策略包含类型，不启用则支持任意，不支持全角字符*/
        function checkPassWord(idForm) {
            var isPassWord = $$('#' + idForm + ' .isPassWord');
            var arr = [];
            var rg = /^[0-9a-z\~\!\@\#\$\%\^\&\*\-\+\,\.\;\|\>\<]{8,15}$/i;
            var rg_word_dx = /[A-Z]+/;
            var rg_word_xx = /[a-z]+/;
            var rg_num = /[0-9]+/;
            var rg_char = /[\~\!\@\#\$\%\^\&\*\-\+\,\.\;\|\>\<]+/;
            var chkNum = <?php echo "\"" . $pwdpolicy_detail['membernum'] . "\""?>;
            if (isPassWord.length == 0) {
                return;
            }
            for (var i = 0; i < isPassWord.length; i++) {

                if (isPassWord[i].value != "" && isPassWord[i].disabled == false) {

                    if (!rg.test(isPassWord[i].value)) {
                        arr.push(isPassWord[i]);
                    } else {
                        var ret1 = rg_word_dx.test(isPassWord[i].value);
                        var ret2 = rg_word_xx.test(isPassWord[i].value);
                        var ret3 = rg_num.test(isPassWord[i].value);
                        var ret4 = rg_char.test(isPassWord[i].value);
                        var num = ret1 + ret2 + ret3 + ret4;
                        <?php if($pwdpolicy_detail["membernum"]) { ?>
                        if (num < chkNum) {
                            arr.push(isPassWord[i]);
                        }
                        <?php }else{?>
                        if (num <= 1) {
                            arr.push(isPassWord[i]);
                        }
                        <?php }?>

                    }
                }
            }
            return arr;

        }

      function regDateInput(){
            if($$("input.Wdate").length == 0) return;
            var lan = "<?= $_SESSION["lan"] ?>".toLowerCase().replace('_', '-');
            $$("input.Wdate").each(function(node){
                if(node.id == 'minDate' && $('maxDate')){
                    node.observe('focus',function(){ WdatePicker({dateFmt:'yyyy-MM-dd HH:mm',isShowWeek:false,maxDate:'#F{$dp.$D(\'maxDate\')}',lang:lan}); });
                }else if(node.id == 'maxDate' && $('minDate')){
                    node.observe('focus',function(){ WdatePicker({dateFmt:'yyyy-MM-dd HH:mm',isShowWeek:false,minDate:'#F{$dp.$D(\'minDate\')}',lang:lan}); });
                }else{
                    node.observe('focus',function(){ WdatePicker({dateFmt:'yyyy-MM-dd HH:mm',isShowWeek:false,lang:lan}); });				
                }
            });
        }

        //信任主机判断
        function checkHost(idForm) {
            var isHost = $$('#' + idForm + ' .isHost');
            var arr = [];
            var ipArr = [];	//检查IP
            var ipArrsub = [];
            var rg2 = /^(((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\.){3}((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\/((([1-9])|([1-2][0-9])|(3[0-2]))|(254|252|248|240|224|192|128|0)\.0\.0\.0|255\.(254|252|248|240|224|192|128|0)\.0\.0|255\.255\.(254|252|248|240|224|192|128|0)\.0|255\.255\.255\.(254|252|248|240|224|192|128|0))$/;
            var rg3 = /^(((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\.){3}((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))-(((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\.){3}((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))$/;
            var rg4 = /^([0-9a-fA-F]{2})(([/\s:][0-9a-fA-F]{2}){5})$/;
            var rg5 = /^(((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\.){3}((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\/([0-9a-fA-F]{2})(([/\s:][0-9a-fA-F]{2}){5})$/;
            if (isHost.length == 0) {
                return arr;
            }
            var checks = 0;
            var sum = null;
            for (var i = 0; i < isHost.length; i++) {
                if (_trim(isHost[i].value) == "") {
                    return arr;
                }
                ipArr = isHost[i].value.split(/\n/g);
                for (var j = 0; j < ipArr.length; j++) {
                    if (isHost[i].value != "" &&
                        isHost[i].value != "0.0.0.0/0" &&
                        isHost[i].disabled == false &&
                        isHost[i].style.display != "none" &&
                        isHost[i].className.indexOf("hide") == -1 &&
                        !(
                            rg2.test(_trim(ipArr[j])) ||
                            rg3.test(_trim(ipArr[j])) ||
                            rg4.test(_trim(ipArr[j])) ||
                            rg5.test(_trim(ipArr[j])) ||
                            isHost[i].value == "全部" ||
                            isHost[i].value.toLowerCase() == 'all'
                        )) {
                        arr.push(isHost[i]);
                    }
                }
            }
            return arr;
        }
        
        // 认证方式下拉切换
        function dispHide() {
	        // $('emailAddress1').className = 'text isNotEmpty isEmailto';
	        // $('setPassword').className = 'text isNotEmpty isPassWord';
	        // $('confirmPassword').className = 'text isNotEmpty isPwd';
	        var objSel = document.getElementById("Auth").value;
			
	        //获取当前账号的认证方式
	        // 1:密码认证 2:Radius认证 3:密码+邮箱认证 4:密码+短信认证
	        if (objSel === 1) {
		        $('tr_radius').style.display = "none";
		        var objPasswordPolicy = document.getElementById("passwordPolicy");
		        if (objPasswordPolicy.value == "1") {
			        $('tr_setPassword').style.display = "";
			        $('tr_confirmPassword').style.display = "";
			        $('tr_emailAddress1').style.display = "none";
			        $('lb_remark').style.display = "none";
			        new hideCheck(['emailAddress1'], ['isEmailto', 'isNotEmpty']);
			        new dispCheck(['setPassword'], [['isNotEmpty']]);
			        new dispCheck(['confirmPassword'], [['isNotEmpty']]);
		        } else {
			        $('tr_setPassword').style.display = "none";
			        $('tr_confirmPassword').style.display = "none";
			        $('tr_emailAddress1').style.display = "none";
			        $('lb_remark').style.display = "";
			        new dispCheck(['emailAddress1'], [['isEmailto', 'isNotEmpty']]);
			        new hideCheck(['setPassword'], ['isNotEmpty']);
			        new hideCheck(['confirmPassword'], ['isNotEmpty']);
		        }
	        } else if (objSel === "2") {
		        $('tr_radius').style.display = "";
		        $('tr_setPassword').style.display = "none";
		        $('tr_confirmPassword').style.display = "none";
		        $('tr_emailAddress1').style.display = "none";
		        $('emailAddress1').className = '';
		        $('setPassword').className = '';
		        $('confirmPassword').className = '';
	        } else if (objSel === "3") {
		        // 启用邮箱
		        $('tr_radius').style.display = "none";
		        $('tr_setPassword').style.display = "";
		        $('tr_confirmPassword').style.display = "";
		        $('tr_emailAddress1').style.display = "";
		        $('tr_telephone').style.display = "none";
		        $('lb_remark').style.display = "none";
		        new hideCheck(['phone'], ['isNotEmpty',"isphone"]);
		        new dispCheck(['emailAddress1'], [['isNotEmpty']]);
		        new dispCheck(['setPassword'], [['isNotEmpty']]);
		        new dispCheck(['confirmPassword'], [['isNotEmpty']]);
	        } else if (objSel === "4") {
		        // 启用短信
		        $('tr_radius').style.display = "none";
		        $('tr_setPassword').style.display = "";
		        $('tr_confirmPassword').style.display = "";
		        $('tr_telephone').style.display = "";
		        $('tr_emailAddress1').style.display = "none";
		        $('emailAddress1').className = '';
		        $('lb_remark').style.display = "none";
		        new dispCheck(['setPassword'], [['isNotEmpty']]);
		        new dispCheck(['phone'], [['isNotEmpty',"isphone"]]);
		        new dispCheck(['confirmPassword'], [['isNotEmpty']]);
	        } else {
		        $('tr_radius').style.display = "none";
		        $('tr_setPassword').style.display = "";
		        $('tr_confirmPassword').style.display = "";
		        $('tr_emailAddress1').style.display = "none";
		        $('lb_remark').style.display = "none";
		        $('tr_telephone').style.display = "none";
                new hideCheck(['phone'], ['isNotEmpty',"isphone"]);
		        new hideCheck(['emailAddress1'], ['isEmailto', 'isNotEmpty']);
		        new dispCheck(['setPassword'], [['isNotEmpty']]);
		        new dispCheck(['confirmPassword'], [['isNotEmpty']]);
	        }
        }

        function checkPwd2(idForm) {
            var isPwd = $$('#' + idForm + ' .isPwd');
            var pwd1 = $('setPassword').value;
            var pwd2 = $('confirmPassword').value;
            var arr = [];
            if (isPwd.length == 0) return;
            for (var i = 0; i < isPwd.length; i++) {
                if (isPwd[i].value != "") {
                    if (pwd1 == pwd2) {
                        $('checkPwd').innerText = "";
                    } else {
                        $('checkPwd').innerHTML = " ";
                        arr.push(isPwd[i]);
                    }
                }
            }
            return arr;
        }
        function checkPwd1(idForm) {
            var isPwd = $$('#' + idForm + ' .isPwd1');
            var pwd1 = $('initialPWD').value;
            var pwd2 = $('initialPWD2').value;
            var arr = [];
            if (isPwd.length == 0) return;
            for (var i = 0; i < isPwd.length; i++) {
                if (isPwd[i].value != "") {
                    if (pwd1 == pwd2) {
                        $('checkPwd').innerText = "";
                    } else {
                        $('checkPwd').innerHTML = " ";
                        arr.push(isPwd[i]);
                    }
                }
            }
            return arr;
        }
        function showDkey() {
            if ($('DKey').checked) {
                $('DkeyPWD').style.display = '';
                $('DkeyType').style.display = '';
                $('DkeyStatus').style.display = '';
                $('DkeyPWD_two').style.display = '';
                new dispCheck(['initialPWD'], [['isNotEmpty']]);
                new dispCheck(['initialPWD2'], [['isNotEmpty',"isPwd1"]]);
            } else {
                $('DkeyPWD').style.display = 'none';
                $('DkeyType').style.display = 'none';
                $('DkeyStatus').style.display = 'none';
                $('DkeyPWD_two').style.display = 'none';
                new hideCheck(['initialPWD'], ['isNotEmpty']);
                new hideCheck(['initialPWD2'], ['isNotEmpty','isPwd1']);
            }
        }

        function handleTotpStatusChange(checkbox) {
            if (!checkbox.checked) {
                var totpSecret = $("totp_secret").value;
                if (totpSecret != '') {
                    if (!confirm("<?php echo _gettext('confirm_totp_unbind_msg'); ?>")) {
                        checkbox.checked = true;
                        return false;
                    } else {
                        $("totp_secret").value = '';
                    }
                }
            }
            showTotp();
        }

        function showTotp() {
            var totpStatus = $("totp_status").checked;
            var totpSecret = $("totp_secret").value;
            if (totpStatus) {
                if (totpSecret != '') {
                    $("totp_unbind").style.display = "";
                    $("totp_refresh").style.display = "";
                    $("totp_bind").style.display = "none";
                } else {
                    $("totp_unbind").style.display = "none";
                    $("totp_refresh").style.display = "none";
                    $("totp_bind").style.display = "";
                }
            } else {
                $("totp_unbind").style.display = "none";
                $("totp_refresh").style.display = "none";
                $("totp_bind").style.display = "none";
            }
        }

        function totp_unbind() {
            if (confirm("<?php echo _gettext('confirm_totp_unbind_msg'); ?>")) {
                $("totp_status").checked = false;
                $("totp_secret").value = '';
                showTotp();
            }
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

            var password = document.getElementById("initialPWD").value;
            if (password) {
                password = crypto_encrypt(password, encrypt_key);
                document.getElementById("initialPWD").value = password;
            }

            var password = document.getElementById("initialPWD2").value;
            if (password) {
                password = crypto_encrypt(password, encrypt_key);
                document.getElementById("initialPWD2").value = password;
            }

            var password = document.getElementById("api_pwd").value;
            if (password) {
                password = crypto_encrypt(password, encrypt_key);
                document.getElementById("api_pwd").value = password;
            }

            return true;
        }
        // 保存提交前加密
        function check_pwd() {

            var result = 1;

            var encrypt_key = "<?php
                if (isset($_SESSION['token'])) {
                    echo substr($_SESSION['token'], 0, 16);
                } else {
                    echo "1234567887654321";
                }
                ?>";
            var setPassword = crypto_encrypt($("setPassword").value, encrypt_key);
            var confirmPassword = crypto_encrypt($("confirmPassword").value, encrypt_key);


            var url = 'checkpwd.php?setPassword=' + setPassword + '&confirmPassword=' + confirmPassword + '&randtime=' + Math.random();
            var result = true;
            var myAjax = new Ajax.Request(url, {
                asynchronous: false,
                onSuccess: function (resp) {
                    if (resp.responseText == 0) {
                        alert('<?=_gettext("pwd_ruo");?>!');
                        $("setPassword").value = '';
                        $("confirmPassword").value = '';
                        result = false;
                    } else {
                        result = true;
                    }
                },
                onFailure: function () {
                    result = false;
                }
            });
            return result;

        }
    </script>
</head>

<body class="body">
<?php
include_once($_SERVER["DOCUMENT_ROOT"] . "/model/tree_checkbox.php");//tree
?>
<div class="navigation">
    <img src="/images/house.gif" class="navImg"/>
    <span class="current"><?= _gettext('currentPosition'); ?>：</span>
    <span class="position"><?= _gettext('m_sysconfg'); ?> >> <?= _gettext('systemadmin'); ?> >> <?= _gettext('addsystemuser'); ?></span>
</div>
<div id="fld_main">
    <form name="form1" id="form1" method="post" action="user_commit.php?action=add">
        <input type="hidden" name="tokenid" value="<?= $tokenid ?>"/>
        <input type="hidden" name="encrypt" value="1"/>
        <input type="hidden" name="token" value="<?php echo $token; ?>"/>
        <div class="operate">
            <div class="title"><?= _gettext('addsystemuser'); ?></div>
            <div class="operate_table">
                <table class="list wtwo">
                    <tbody>
                    <tr>
                        <th><em class="xing">*</em> <?= _gettext("User name"); ?></th>
                        <td>
                            <input type="text" name='user_name' id='userName' class="text isNotEmpty isNameCH" msg="<?= _gettext('username_error_32'); ?>" value="" tabindex="2"/>
                        </td>
                    </tr>
                    <tr>
                        <th><em class="xing">*</em> <?= _gettext("auth_method"); ?></th>
                        <td>
                            <select name="auth_method" id="Auth" onchange="dispHide()" tabindex="4">
                                <option value="1" selected="selected"><?= _gettext("password_auth"); ?></option>
                                <option value="2"><?= _gettext("Radius_authentication"); ?></option>
                                <!-- 增加邮件和短信的验证判断 -->
                                <option value="3"><?= _gettext("password_auth") . '+' . _gettext("email_verification"); ?></option>
                                <?php if(!DEVICE_IS_NSA_AUTH) { ?>
                                <option value="4"><?= _gettext("password_auth") . '+' . _gettext("sms_verification_code"); ?></option>
                                <?php } ?>
                            </select>
                        </td>
                    </tr>
                    <tr id="tr_radius" style="display:none">
                        <th><em class="xing">*</em><?= _gettext("m_RADIUSserver"); ?></th>
                        <td>
                            <select name='radius_sername' id='radius_sername'>
                                <?php
                                $radius_size = ace_userspace_query_size(3014, "");
                                if (0 < $radius_size) {
                                    $radius_format = 'Lno1/Lno2/Lno3/Lno4/a32name/a32secret/Lip/Lport';
                                    $radius_arr = array();
                                    $tmp = $radius_size;
                                    $ret = ace_userspace_query(3016, '', '', $radius_arr, $tmp, 0);
                                    if (0 == $ret) {
                                        foreach ($radius_arr as $radius_list) {
                                            $radius_detail = unpack($radius_format, $radius_list);
                                            echo "<option value='" . $radius_detail["name"] . "'>" . $radius_detail["name"] . "</option>";
                                        }
                                    }
                                }
                                ?>
                            </select>
                        </td>
                    </tr>
                    <tr id="tr_passwordPolicy" style="display:none">
                        <th><em class="xing">*</em> <?= _gettext("password_policy"); ?></th>
                        <td>
                            <select name="pwd_policy" id="passwordPolicy" onchange="dispHide()" tabindex="6">
                                <option value="1" selected="selected"><?= _gettext("manual_configuration"); ?></option>
                                <option value="2"><?= _gettext("autogeneration"); ?>(<?= _gettext("EMN"); ?>)</option>
                            </select>
                            <label id="lb_remark" class="lbremark" style="display:none;">(<?= _gettext("password_policy_explain"); ?>)</label></td>
                    </tr>
                    <tr id="tr_setPassword" style="display:none">
                        <th><em class="xing">*</em> <?= _gettext("set_password"); ?></th>
                        <td>
                            <strong style="float:left; width:200px">
                                <input type="password" class="text isNotEmpty <?php if(!isDefect(18)) { echo 'isPassWord'; } ?>" msg="<? 
                                if($is_defect) {
                                    echo _gettext("pwd_nonull");
                                } else if ($pwdpolicy_detail['membernum'] == 4) {
                                    echo _gettext("userpass_length_error_4");
                                } else {
                                    if ($pwdpolicy_detail['membernum'] == 3) {
                                        echo _gettext("userpass_length_error_3");
                                    } else {
                                        if ($pwdpolicy_detail['membernum'] == 2) {
                                            echo _gettext("userpass_length_error_0");
                                        } else {
                                            echo _gettext("userpass_length_error");
                                        }
                                    }
                                } ?>" name='setPassword'
                                       id='setPassword' value="" onblur="chkpwd(this);checkPwd()" onkeyup="chkpwd(this);" tabindex="7"/>
                            </strong>

                        </td>
                    </tr>
                    <tr id="tr_confirmPassword" style="display:none">
                        <th><em class="xing">*</em> <?= _gettext("suepassword"); ?></th>
                        <td>
                            <input type="password" class="text isNotEmpty isPwd" msg="<?= _gettext("suepassword_null"); ?>" name='confirmPassword' id='confirmPassword' value="" onblur="checkPwd()" onkeyup="checkPwd()" tabindex="8"/>
                            <label class="xing" id="checkPwd" style="display:none;"><?= _gettext("pwd_confirm"); ?></label></td>
                    </tr>
                    <tr>
                        <th></th>
                        <td>
                            <input type="checkbox" id="DKey" name="DKey" onclick="showDkey()" tabindex="9"/>
                            <label for="DKey"><?= _gettext('dkey_auth_tip') ?></label>
                            <img class="tipsfont" src="/images/question.gif" title="<?= _gettext('dkey_auth_note') ?>"/>

                    <tr id="DkeyType" style="display:none">
                        <th></th>
                        <td>
                            <input type="radio" name="dkey_type" tabindex="10" id="dkey" value="0" checked='checked'><label for="dkey" class="space-r15"><?= _gettext("SMKey"); ?></label>
                            <input type="radio" name="dkey_type" tabindex="10" id="ukey" value="1"/><label for="ukey"><?= _gettext("UKey"); ?></label>
                        </td>
                    </tr>
                    <tr id="DkeyStatus" style="display:none">
                        <th></th>
                        <td>
                            <input type="radio" name="dkey_status" tabindex="10" id="dkey_audit" value="0" checked='checked';?> <label for="dkey_audit" class="space-r15"><?= _gettext("dkey_audit_tip"); ?></label>
                            <input type="radio" name="dkey_status" tabindex="10" id="dkey_login" value="1"/><label for="dkey_login"><?= _gettext("dkey_login_tip"); ?></label>
                        </td>
                    </tr>
                    <tr id="DkeyPWD" style="display:none">
                        <th></th>
                        <td>
                            <span class="divs"><font color="red">*</font><?= _gettext('dkey_initial_pwd') ?></span>
                            <input type="password" id="initialPWD" name="initialPWD" class="" msg="<?= _gettext('dkey_initial_pwd_error') ?>" tabindex="10"/><br/>
                        </td>
                    </tr>
                    <tr id="DkeyPWD_two" style="display:none">
                        <th></th>
                        <td>
                            <span class="divs"><font color="red">*</font><?= _gettext('checked_dkey_initial_pwd') ?></span>
                            <input type="password" id="initialPWD2" class="isPwd1" msg="<?= _gettext('checked_dkey_initial_pwd_error') ?>" name="FirstPWD2" tabindex="11"/>
                        </td>
                    </tr>
                    <tr>
                        <th><?= _gettext("totp_2FA"); ?></th>
                        <td>
                            <input type="checkbox" id="totp_status" name="totp_status" <?php if (1 == $user_data_detail['totp_status']) echo("checked=\"checked\"") ?> onchange="handleTotpStatusChange(this)" tabindex="6"/>
                            <label for="totp_status"><?= _gettext('enable') ?></label>
                            <input type="hidden" name='totp_secret' id='totp_secret' value="<?=  $user_data_detail['totp_secret']; ?>"/>
                            <a href="#" onclick="showIframe5('<?php echo setHyDetailData(); ?>');" class="totp_refresh" id="totp_refresh" style="display: <?php echo (1 == $user_data_detail['totp_status'] && !empty($user_data_detail['totp_secret'])) ? '' : 'none'; ?>"><?= _gettext('refresh') ?></a>
                            <a href="#" onclick="totp_unbind()" class="totp_unbind" id="totp_unbind" style="display: <?php echo (1 == $user_data_detail['totp_status'] && !empty($user_data_detail['totp_secret'])) ? '' : 'none'; ?>"><?= _gettext('unbind') ?></a>
                            <a href="#" onclick="showIframe4('<?php echo setHyDetailData(); ?>');" class="totp_bind" id="totp_bind" style="display: <?php echo (1 == $user_data_detail['totp_status'] && empty($user_data_detail['totp_secret'])) ? '' : 'none'; ?>"><?= _gettext('bind') ?></a>
                        </td>
                    </tr>
                    <tr style="<?=DEVICE_IS_NSA_AUTH ? "display:none" : ""?>">
                        <th><?= _gettext("trueName"); ?></th>
                        <td><input type="text" class="text" name='realname' id='realname' value="" tabindex="12"/></td>
                    </tr>
                    <tr style="<?=DEVICE_IS_NSA_AUTH ? "display:none" : ""?>">
                        <th><?= _gettext("company_department"); ?></th>
                        <td>
                            <input type="text" name='company' id='companyBranch' class="text" value="" tabindex="14"/>
                        </td>
                    </tr>
                    <!-- 增加邮箱配置显示 -->
                    <tr id="tr_emailAddress1" style="">
                        <th><em class="xing">*</em><?= _gettext("mail_address"); ?></th>
                        <td>
                            <input type="text" name='email' class="text isNotEmpty isEmailto" msg="<?= _gettext("mail_address_error1"); ?>" id='emailAddress1' value="" tabindex="16"/>
                            <a href="###" id="systemmail">[<?= _gettext('mail_server'); ?>]</a>
                            <?= _gettext("check_email_validate"); ?>
                        </td>
                    </tr>
                    <!-- 邮箱配置end -->
                    <!-- 短信服务配置 -->
                    <tr id="tr_telephone" style="">
                        <th><font color="red">*</font><?= _gettext("telephone"); ?></th>
                        <td>
                            <input type="text" name='telephone' id='phone' class="isNotEmpty isphone" msg="<?= _gettext("telephone_error"); ?>" value="" tabindex="20"/>
                            <a href="###" id="systemsms">[<?= _gettext('sms_server'); ?>]</a>
                            <?= _gettext("check_sms_validate"); ?>
                        </td>
                    </tr>
                    <!-- 短信服务配置end -->
                    <tr>
                        <th><?= _gettext('role'); ?></th>
                        <td>
                            <select name="role" id="role" url='/view/systemRole.php' tabindex="22">
                                <?php
                                $system_user_role_array = get_role_recordsets();
                                foreach ($system_user_role_array as $system_user_role_detail) {
                                    if(!isThreeConf() && $system_user_role_detail["name"] == 'super_admin') {
                                        continue;
                                    }
                                    echo "<option value='" . $system_user_role_detail["name"] . "'>" . _gettext($system_user_role_detail["name"]) . "</option>";
                                }
                                ?>
                            </select>
                            <a href="###" id="systemrole" <?php echo (isOemFeatureByName(array("ccrc","beixinyuan_fw","beixinyuan_ws","Nsas360","jiuding")) || isHylabDtr()) && isThreeConf() && $_SESSION["regRole"] == $three_conf_sysadmin_role ? "style='display: none;'" : ""; ?>>[<?= _gettext('m_systemrole'); ?>]</a>
                        </td>
                    </tr>
                    <tr>
                        <th class="baselines"><?= _gettext('GroupBelonged'); ?></th>
                        <td>
                            <textarea readonly="readonly" id='moveList' name='moveList' msg='<?= _gettext('dest_group_url') ?>' tabindex="24" class="disablebox"></textarea>
                            <a href="###" id="show" class="over" tabindex="26"><?= _gettext("Select group"); ?></a>
                        </td>
                    </tr>
                    <tr>
                        <th class="checkAll"><?= _gettext('status'); ?></th>
                        <td>
                        <?php if(isOemFeatureByName(array("beixinyuan_fw","beixinyuan_ws","Nsas360","jiuding")) || isHylabDtr()) {  
                                echo _gettext('enable');
                                echo "<input type='hidden' name='status' value='1'>";
                            } else { ?>
                            <select name="status" id="status" tabindex="28">
                                <option value="1"><?= _gettext('enable'); ?></option>
                                <option value="0"><?= _gettext('disable'); ?></option>
                            </select>
                            <?php } ?>
                        </td>
                    </tr>
                    <tr>
                        <th><?= _gettext('remark'); ?></th>
                        <td>
                            <input type="text" name='comment' id='comment' class="text" value="" tabindex="30"/>
                        </td>
                    </tr>
                    <tr>
                        <th class="baselines"><font color="red">*</font><?= _gettext('Trust Host'); ?></th>
                        <td class="alignLeft">
                            <textarea name='trusthost' class="isNotEmpty isHost" msg='<?= _gettext('trust_host_err') ?>' tabindex="31"></textarea>
                            <img src="/images/question.gif" title="<?= _gettext('dest_group_url_tips') ?>"/>
                        </td>
                    </tr>
                    <tr>
                        <th class="checkAll"><?= _gettext('API_interface_status'); ?></th>
                        <td>
                            <select name="api_status" id="api_status" tabindex="32" onchange="apichange()">
                                <option value="1"><?= _gettext('enable'); ?></option>
                                <option value="0" selected ><?= _gettext('disable'); ?></option>
                            </select>
                        </td>
                    </tr>
                    <tr id="api_signature">
                        <th><font color="red">*</font><?= _gettext('API_signature'); ?></th>
                        <td>
                            <input type="password" id="api_pwd" autocomplete="new-password" name="api_pwd" class="" msg="<?= _gettext('api_signature_pwd_error') ?>" tabindex="33"/><br/>
                        </td>
                    </tr>
                    <tr id="api_trust">
                        <th class="baselines"><font color="red">*</font><?= _gettext('API_trust_ip'); ?></th>
                        <td class="alignLeft">
                            <textarea id="api_white_list" name='api_white_list' class="isIp2 public" msg='<?= _gettext('api_trust_err') ?>' value="0.0.0.0/0" tabindex="31">0.0.0.0/0</textarea>
                            <img src="/images/question.gif" title="<?= _gettext('ip_format_policyroute') ?>"/>
                        </td>
                    </tr>
                    <tr>
                        <th></th>
                        <td class="operateBtn">
                            <input type="submit" class="btn_ok" onclick="return check_pwd()" value="<?= _gettext("OK"); ?>"/>
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
    function chkpwd(obj) {
        var t = obj.value;
        var id = getResult(t);
        var msg = new Array(4);
        var chkNum = <?php echo "\"" . $pwdpolicy_detail['membernum'] . "\""?>;
        var error_msg = "<?=_gettext('userpass_length_error');?>";
        if (chkNum == 2) {
            error_msg = "<?=_gettext('userpass_length_error_0');?>";
        }
        if (chkNum == 3) {
            error_msg = "<?=_gettext('userpass_length_error_3');?>";
        }
        if (chkNum == 4) {
            error_msg = "<?=_gettext('userpass_length_error_4');?>";
        }
        if (id >= chkNum) {
            error_msg = '';
        }


        msg[0] = "<?=_gettext('pwd_null_short');?>";
        msg[1] = "<?=_gettext('pwd_point2');?>";
        msg[2] = "<?=_gettext('pwd_point3');?>";
        msg[3] = "<?=_gettext('pwd_point4');?>";
        msg[4] = "";

        var sty = new Array(4);
        sty[0] = -45;
        sty[1] = -30;
        sty[2] = -15;
        sty[3] = 0;
        sty[4] = -100;

        var col = new Array(4);
        col[0] = "gray";
        col[1] = "red";
        col[2] = "#ff6600";
        col[3] = "Green";
        col[4] = "#6666FF";

        var bImg = "/images/pwdCheck.gif";
        var sWidth = 750;
        var sHeight = 15;
    }

    function getResult(s) {
        if (s.length == 0) {
            return 4;
        }
        var rg = /^[0-9a-z\~\!\@\#\$\%\^\&\*\-\+\,\.\;\|\>\<]{8,15}$/i;
        var rg_word_dx = /[A-Z]+/;
        var rg_word_xx = /[a-z]+/;
        var rg_num = /[0-9]+/;
        var rg_char = /[\~\!\@\#\$\%\^\&\*\-\+\,\.\;\|\>\<]+/;
        if (!rg.test(s)) {
            return 0;
        }
        var ret1 = rg_word_dx.test(s);
        var ret2 = rg_word_xx.test(s);
        var ret3 = rg_num.test(s);
        var ret4 = rg_char.test(s);
        var ls = 0;
        ls = ret1 + ret2 + ret3 + ret4;
        return ls;

    }
    function checkIp2(idForm) {
		var isIp2 = $$('#' + idForm + ' .isIp2');
		var ipArr = []; //检查IP
		var ipArr2 = [];

		var arr = [];
		var rg = /^(((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))\.){3}((\d{1})|([1-9]\d{1})|(1\d{2})|(2[0-4]\d{1})|(25[0-5]))$/;

		var rg2 = /^(((254|252|248|240|224|192|128|0)\.0\.0\.0)|(255\.(254|252|248|240|224|192|128|0)\.0\.0)|(255\.255\.(254|252|248|240|224|192|128|0)\.0)|(255\.255\.255\.(255|254|252|248|240|224|192|128|0)))$/
		var rg3 = /^([0-9]|1[0-9]|2[0-9]|3[0-2])$/;

		var str1 = "0.0.0.0/0";

		for (var i = 0; i < isIp2.length; i++) 
		{
			if (str1 == isIp2[i].value) {
				continue;
			} else {
				ipArr = isIp2[i].value.split(/\n/g); 
				for (var l = 0; l < ipArr.length; l++) {
					var ipRange = ipArr[l].split("-");

					if (ipRange.length > 1) {
						var startIP = _ip2int(ipRange[0]);
						var endIP = _ip2int(ipRange[1]);

						if (startIP > endIP) {
							arr.push(isIp2[i]);
							return arr;
						}
					}
				}

				for (var j = 0; j < ipArr.length; j++) 
				{
					var subArr = []; 
					for (var k = j + 1; k < ipArr.length; k++) {
						if (ipArr[j] === ipArr[k]) {
							ipArr.splice(k, 1);
							arr.push(isIp2[i]);
						}
					}

					if ((navigator.appName.indexOf("Microsoft") != -1) && (!-[1, ]) && (j < ipArr.length - 1)) {

						ipArr[j] = ipArr[j].substring(0, ipArr[j].length - 1);
					}
					ipArr[j] = ipArr[j].replace(/(^\s*)|(\s*$)/g, ""); 
					if (ipArr[j].search(/\//g) != -1) 
					{
						subArr.push(ipArr[j].substring(ipArr[j].search(/\//g) + 1)); 
						ipArr[j] = ipArr[j].substring(0, ipArr[j].search(/\//g));
					} else if (ipArr[j].search(/\-/g) != -1) {
						ipArr2.push(ipArr[j].substring(ipArr[j].search(/\-/g) + 1));
						ipArr[j] = ipArr[j].substring(0, ipArr[j].search(/\-/g));
					}
					if (isIp2.length == 0) {
						return;
					}

					if (isIp2[i].disabled == false && isIp2[i].value != "" && !rg.test(ipArr[j])) {
						arr.push(isIp2[i]);
						return arr;
					} else if (isIp2[i].disabled == false && isIp2[i].value && ipArr2[j] != null) {
						for (var k = 0; k < ipArr2.length; k++) {
							var ip1 = ipArr[j];
							var ip2 = ipArr2[j];

							var ip1Int = _ip2int(ip1);
							var ip2Int = _ip2int(ip2);

							if (ip1Int > ip2Int) {
								arr.push(isIp2[i]);
								return arr;
							}

							if (!(rg.test(ipArr2[k]))) {
								arr.push(isIp2[i]);
								return arr;
							}
						}
					} else if (isIp2[i].value != '' && isIp2[i].disabled == false && isIp2[i].style.display != 'none' && rg.test(ipArr[j])) {
						for (var k = 0; k < subArr.length; k++) {
							if (!(rg2.test(subArr[k]) || rg3.test(subArr[k]))) {
								arr.push(isIp2[i]);
								return arr;
							}
						}
					} else if (isIp2[i].value != '' && isIp2[i].disabled == false && isIp2[i].style.display != 'none') {
						for (var k = 0; k < subArr.length; k++) {
							if (!(rg2.test(subArr[k]) || rg3.test(subArr[k]) || (subArr[k]) <= 128 && (subArr[k]) >= 0)) {
								arr.push(isIp2[i]);
								return arr;
							}
						}
					}


				}
			}
		}
		return arr;
	}

    var showIframe4pop;

    function showIframe4() {
        var user_name = $("userName").value;
        if (!user_name){
            alert("<?=_gettext('pleaseEnterName')?>");
            return false;
        }
        var rg= /^[\u4e00-\u9fa5a-zA-Z0-9\!\@\#\^\*\-\_\.]{0,31}$/;
        var exceptStr = "NMC_";
        if (user_name != "" && (!rg.test(user_name) || user_name.indexOf(exceptStr) == 0 || user_name.replace(/[\u4e00-\u9fa5]/ig,'xxx').length > 31)) {
            alert("<?=_gettext('username_error_32')?>");
            return false;
        }
        showIframe4pop = new Popup({
            contentType: 1,
            scrollType: 'yes',
            isReloadOnClose: false,
            width: '750',
            height: '430'
        });
        showIframe4pop.setContent("contentUrl", "/view/systemConfig/sys_user/user_totp_bind.php?tokenid=<?= $tokenid ?>&user_name=" + encodeURIComponent(user_name));
        showIframe4pop.setContent("title", "Google Authenticator "+"<?=_gettext("bind")?>");
        showIframe4pop.build();
        showIframe4pop.show();
    }

    var showIframe5pop;

    function showIframe5() {
        var user_name = $("userName").value;
        var totp_secret = $("totp_secret").value;
        if (!user_name){
            alert("<?=_gettext('pleaseEnterName')?>");
            return false;
        }
        var rg= /^[\u4e00-\u9fa5a-zA-Z0-9\!\@\#\^\*\-\_\.]{0,31}$/;
        var exceptStr = "NMC_";
        if (user_name != "" && (!rg.test(user_name) || user_name.indexOf(exceptStr) == 0 || user_name.replace(/[\u4e00-\u9fa5]/ig,'xxx').length > 31)) {
            alert("<?=_gettext('username_error_32')?>");
            return false;
        }
        showIframe5pop = new Popup({
            contentType: 1,
            scrollType: 'yes',
            isReloadOnClose: false,
            width: '750',
            height: '430'
        });
        showIframe5pop.setContent("contentUrl", "/view/systemConfig/sys_user/user_totp_bind.php?tokenid=<?= $tokenid ?>&user_name=" + encodeURIComponent(user_name) + "&totp_secret=" + totp_secret);
        showIframe5pop.setContent("title", "Google Authenticator "+"<?=_gettext("refresh")?>");
        showIframe5pop.build();
        showIframe5pop.show();
    }
</script>

</body>
</html>