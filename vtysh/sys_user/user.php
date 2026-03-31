<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
require_once($_SERVER["DOCUMENT_ROOT"] . "/model/lan.php");
include_once($_SERVER["DOCUMENT_ROOT"] . "/authenticed.php");
require_once($_SERVER["DOCUMENT_ROOT"] . "/model/common_param.php");
isAvailable('m_systemadmin');
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
    <link href="/css/common.css" rel="stylesheet" type="text/css"/>
    <link href="/css/skin.css" rel="stylesheet" type="text/css"/>
    <style type="text/css">
        .table_list td span {
            margin-right: 6px;
        }
    </style>
    <script type="text/javascript" src="/js/prototype.js"></script>
    <script type="text/javascript" src="/js/base.js"></script>
    <script type="text/javascript" src="/js/base64.js"></script>
</head>
<?php

include_once("user_functions.php");
$auth_method = array(
    '1' => 'password_auth',
    '2' => 'Radius_authentication',
    '3' => 'pwd_auth_email_verification',
    '4' => 'pwd_auth_sms_verification',
);
$userInfo = getUserInfo($_SESSION['regUser']);

$mode = isThreeConf();

function showRecordRows()
{
    global $mode;
    global $auth_method;
    global $common_papam_admin_name;
    global $common_param_audit_name;

    $user_array = get_user_recordsets();
    $i = 0;
    foreach ($user_array as $userdata) {
        if (!$mode && !strcmp($_SESSION['regUser'], $common_papam_admin_name)) {
        } else {
            if ($mode && $_SESSION['regUser'] == 'sysadmin') {
            } else {
                if (strcmp($_SESSION['regUser'], $userdata['name'])) {
                    continue;
                }
            }
        }
       
        $rowStr = "<tr>";
        $rowStr .= "<td>" . ($i + 1) . "</td><td>" . $userdata['name'] . "</td>";
        if ($userdata['role']) {
            $rowStr .= "<td>" . _gettext($userdata['role']) . "</td>";
        } else {
            $rowStr .= "<td></td>";
        }

        if ($userdata['auth_method']) {
            $authvalue = $userdata['auth_method'];
        } else {
            $authvalue = 1;
        }
        $rowStr .= "<td>" . _gettext($auth_method[$authvalue]) . "</td>";
        $rowStr .= "<td>" . $userdata['path'] . "</td>";
        $rowStr .= "<td>" . ($userdata['api_status'] ? _gettext("enable") : _gettext("disable"))  . "</td>";
        $rowStr .= "<td>" . ($userdata['status'] ? _gettext("enable") : _gettext("disable"))  . "</td>";

        if (0 == strcmp($_SESSION['regUser'], $common_papam_admin_name) || $_SESSION['regUser'] == 'sysadmin') {
            $rowStr .= "<td><a href='user_edit.php?name=" . urlencode($userdata["name"]) . "'>" . _gettext('Modify') . "</a> ";
            if ($userdata["pre_define"]) {
                $rowStr .="<span>". _gettext('Delete') . "</span> <span>" . _gettext('Key_Write_in') . "</span>";
            } else {
                $rowStr .= "<a class=\"delLink\" title=\"" . _gettext('suredel') . "\" href=\"###\" onclick=\"del_ajax_post('" . urlencode($userdata["name"]) . "')\"> " . _gettext('Delete') . "</a>";
                if ($userdata['DKey'] == "on" && $userdata['dkey_type'] != 1 && $userdata['dkey_password'] != "") {
                    $rowStr .= " <a href='write_to_dkey.php?usrmd5=" . md5(urlencode($userdata["name"])) . "&pwdmd5=" . $userdata['dkey_password'] . "'> " . _gettext('Key_Write_in') . "</a>";
                }
                if ($userdata['DKey'] == "on" && $userdata['dkey_type'] == 1 && $userdata['dkey_password'] != "") {
                    $rowStr .= "<a href='#' onclick=\" return write_to_key('" . urlencode($userdata["name"]) . "','" . substr($userdata['dkey_password'], 0, 16) . "');\"> " . _gettext('Key_Write_in') . "</a>";
                }
            }
            $rowStr .= "</td>";
        } else {
            $rowStr .= "<td><a href='user_passwd.php?name=" . urlencode($userdata["name"]) . "'>" . _gettext('Modify') . "</a> ";
        }
        $rowStr .= "</tr>\r\n";
        $i++;
        echo $rowStr;
        unset($rowStr);
        unset($userdata);
    }
}

?>
<script type="text/javascript">
    new MoveOverLightTable("adminTable");
    // new ConfirmDelForLink();
    // var btnenale = true;

    function del_ajax_post(name) {
        if (name) {
            if (confirm('<?=_gettext('suredel')?>' + "?")) {
                var formdata = 'name=' + name + '&tokenid=<?=$tokenid?>';
                var url = "user_commit.php?action=delete";
                var myAjax = new Ajax.Request(
                    url,
                    {
                        method: 'post', parameters: formdata, onComplete: function () {
                            location.href = 'user.php';
                        }
                    }
                );
                // btnenale = false;
            } else {
                return false;
            }

        }
    }

    var socket;
    var type = 0;
    var username = '';
    var dkey_password = '';

    function write_to_key(uname, pwd) {
        dkey_password = pwd;
        username = uname;
        $('username').value = '';
        $('dkey_password').value = '';
        $('signature').value = '';
        for (var port = 60000; port < 60010; port++) {
            getKey(port);
        }
        key_result(0);
    }

    function key_result(first) {
        if (type != 12) {
            if (first == 1) {
                alert("<?=_gettext("dkey_connect_fail")?>");
                location.reload();
            } else {
                setTimeout('key_result(1)', 3000);
            }
        }
    }

    function getKey(port) {
        var url = "ws://127.0.0.1:" + port;
        var client = new WebSocket(url);
        client.onopen = sOpen;
        client.onmessage = sMessage;
    }

    function sOpen(e) {
        var msg = {"type": 11};
        if (type == 11 || type == 0) type = 11;
        var str = base64encode(JSON.stringify(msg));
        e.target.send(str);
    }

    function sMessage(msg) {
        console.log('msg', msg);
        buffer = base64decode(msg.data);
        if (buffer) {
            buffer = JSON.parse(buffer);
            console.log('buffer', buffer);
            if (buffer.result == 'OK') {
                socket = msg.target;
                if (type == 11) {
                    type = 12;
                    msg = {"type": 12, "pin": dkey_password, "data": username};
                    console.log('send_msg', msg);
                    var str = base64encode(JSON.stringify(msg));
                    socket.send(str);
                } else if (type == 12) {
                    if (!buffer.data) {
                        //alert('写入Key失败');
                        return;
                    }
                    $('username').value = username;
                    $('dkey_password').value = dkey_password;
                    $('signature').value = buffer.data;
                    socket.close();
                    $('form2').submit();
                }
            }
        }
    }

    function showDownload(val) {
        if (!val) return;
        $("download_btn").href = "driver_download.php?type=" + val;
        $("download_btn").click();
    }
</script>

<body class="body">
<div class="navigation">
    <img src="/images/house.gif" class="navImg"/>
    <span class="current"><?= _gettext('currentPosition'); ?>：</span>
    <span class="position"><?= _gettext('m_sysconfg'); ?> >> <?= _gettext('systemadmin'); ?></span>
</div>
<div id="fld_main">
    <form id="form1" name="form1" method="post" action="">
        <input type="hidden" name="tokenid" value="<?= $tokenid ?>"/>
        <table id="adminTable" class="table_list">
            <caption>
                <div class="table_title"><?= _gettext('systemadmin'); ?></div>
                <div class="center title_btn_left">
                    <a id="download_btn" href="driver_download.php?type=dkey" style="display:none"></a>
                    <?php
                    if ($_SESSION['regRole'] === 'super_admin'): ?>
                        <select id="download_driver" onchange="showDownload(this.value)">
                            <option value="" selected>-- <?= _gettext("Download driver") ?> --</option>
                            <option value="dkey"><?= _gettext("Download smkey driver") ?></option>
                            <option value="ukey"><?= _gettext("Download ukey driver") ?></option>
                        </select>
                        <?php
                        if ($userInfo['pre_define'] == 1): ?>
                            <!-- <a class="btn_organization" href='driver_download.php'><?= _gettext('Download Boss key driver'); ?></a> -->
                            <a class="btn_config" href="setPwdSafePolicy.php"><?= _gettext('Password_policies') ?></a>
                            <?php if(!DEVICE_IS_NSA_AUTH) { ?>
                            <a class="btn_config" href="setAdminPolicy.php"><?= _gettext('Management Settings') ?></a>
                            <?php } ?>
                        <?php
                        endif; ?>
                    <?php
                    endif; ?>
                    <?php
                    if ($mode) {
                        if($_SESSION['regUser'] == 'sysadmin') {
                        ?>
                        <select id="download_driver" onchange="showDownload(this.value)">
                            <option value="" selected>-- <?= _gettext("Download driver") ?> --</option>
                            <option value="dkey"><?= _gettext("Download smkey driver") ?></option>
                            <option value="ukey"><?= _gettext("Download ukey driver") ?></option>
                        </select>
                        <!-- <a class="btn_config" href='setPwdSafePolicy.php'><?= _gettext('Password_policies'); ?></a> -->
                        <?php if(!DEVICE_IS_BMJ) { ?>
                        <a class="btn_config" href='setAdminPolicy.php'><?= _gettext('Management Settings'); ?></a>
                        <?php } ?>
                        <?php
                        } else if($_SESSION['regUser'] == 'secadmin') {
                    ?>
                        <a class="btn_config" href='setPwdSafePolicy.php'><?= _gettext('Password_policies'); ?></a>
                    <?php
                        }
                    }
                    if (0 == strcmp($_SESSION['regUser'], $common_papam_admin_name)) {
                        ?>
                        <input class="btn_add" type="button" value="<?= _gettext('Add'); ?>" onclick="javascript:location.href='user_add.php'"/>
                        <?php
                    }
                    if ($_SESSION['regUser'] == 'sysadmin' && !DEVICE_IS_BMJ) {
                        ?>
                        <input class="btn_add" type="button" value="<?= _gettext('Add'); ?>" onclick="javascript:location.href='user_add.php'"/>
                        <?php
                    }
                    ?>
                </div>
            </caption>

            <thead>
            <tr>
                <th class="index"><?= _gettext('Index'); ?></th>
                <th><?= _gettext('username'); ?></th>
                <th><?= _gettext('role'); ?></th>
                <th><?= _gettext('auth_method'); ?></th>
                <th><?= _gettext('GroupBelonged'); ?></th>
                <th><?= _gettext('API_interface_status'); ?></th>
                <th width="80"><?= _gettext('status'); ?></th>
                <th width="200"><?= _gettext('Operation'); ?></th>
            </tr>
            </thead>
            <tbody>
            <?php showRecordRows(); ?>
            </tbody>
        </table>

        <input name="text_static_command" type="hidden" id="text_static_command" value="">
    </form>
    <form id="form2" name="form2" method="post" action="write_to_ukey.php">
        <input type="hidden" name="tokenid" value="<?= $tokenid ?>"/>
        <input type="hidden" id="username" name="username" value="">
        <input type="hidden" id="dkey_password" name="dkey_password" value="">
        <input type="hidden" id="signature" name="signature" value="">
    </form>
</div>
<?php if(!isOemFeatureByName("nsas360")){ ?>
    <p class="crumbs tableCrumbs"><?= _gettext('dkey_write_only_in_ie'); ?></p>
<?php }?>
</body>
</html>
