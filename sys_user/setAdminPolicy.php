<?php
include_once("/var/www/html/model/charFilter.php"); ?>
<?php
require_once($_SERVER["DOCUMENT_ROOT"]."/model/lan.php");
include_once(PUBLIC_LIBRARY_PHP_DIR."core/func/sms.func.php");
$sms_config_list = get_sms_auth_config();
$oldmode = 1;
if(!isOemFeatureByName(array("beixinyuan_fw","beixinyuan_ws","Nsas360","jiuding")) && !isHylabDtr() && !isThreeConf()) {
    $oldmode = 0;
}
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
    <link href="/css/common.css" rel="stylesheet" type="text/css"/>
    <link href="/css/skin.css" rel="stylesheet" type="text/css"/>
    <script type="text/javascript" src="/js/prototype.js"></script>
    <script type="text/javascript" src="/js/base.js"></script>
    <script type="text/javascript" src="/js/base64.js"></script>
    <script type="text/javascript">
        function alertChangeMode() {
            var mode = $("mode").value;
            var oldmode = $("oldmode").value;
            if (mode == oldmode || confirm('<?=_gettext("ModeConfig")?>')) {
                $("form1").submit();
                return true;
            } else {
                return false;
            }
        }
    </script>
</head>

<body class="body">
<div id="fld_main" <?= in_tabs_class() ?> >
    <form id="form1" name="form1" method="post" action="admin_setting.php?in_tabs=<?= $_GET['in_tabs'] ?>">
        <input type="hidden" name="tokenid" value="<?= $tokenid ?>"/>
        <input type="hidden" name="oldmode" id="oldmode" value="<?= $oldmode ?>"/>
        <div class="operate">
            <div class="title"> <?= _gettext('Administrator settings'); ?></div>
            <div class="operate_table">
                <table class="list wtwo" id="table1">
                    <tr <?php if(!((isHylabNsas() && !isOemFeatureByName('ccrc')) || hylab_firewall())) { echo 'style="display: none;"'; }?>>
                        <th class="name"><?= _gettext("Mode setting"); ?></th>
                        <td class="alignLeft">
                            <?php if(!isOemFeatureByName(array("beixinyuan_fw","beixinyuan_ws","Nsas360","jiuding")) && !isHylabDtr()) { ?>
                            <select name="mode" id="mode">
                                <option value="0" <?= (!isThreeConf() ? 'selected' : '') ?>><?= _gettext("Normal mode"); ?></option>
                                <option value="1" <?= (isThreeConf() ? 'selected' : '') ?>><?= _gettext("Triple Power Model"); ?></option>
                            </select>
                            <?php } else {
                                echo _gettext("Triple Power Model");
                                echo '<input type="hidden" name="mode"  id="mode" value="1">';
                            } ?>
                        </td>
                    </tr
                    <?php if(!isHylabBmj()){?>
                        <tr>
                            <th class="name"><?= _gettext("sms_template"); ?></th>
                            <td class="alignLeft">
                                <select name="sms_pwd_auth_tempname" id="sms_pwd_auth_tempname" class="isNotEmpty" msg="<?= _gettext('please_sms_template'); ?>" tabindex="4">
                                    <option value=""><?= _gettext('plsselect'); ?></option>
                                    <?php
                                    foreach ($sms_config_list["items"] as $vv) {
                                        echo "<option value='".$vv["name"]."' ";
                                        if ($vv["name"] == $sms_config_list["sms_pwd_auth_tempname"]) {
                                            echo " selected ";
                                        }
                                        echo ">".$vv["name"]."</option>";
                                    } ?>
                                </select>
                            </td>
                        </tr>
                    <?php }?>
                    <tr>
                        <th></th>
                        <td class="operateBtn">
                            <input class="btn_ok" type="button" name="commit" value="<?= _gettext("OK") ?>" onclick="alertChangeMode()"/>
                            <a href="user.php" class="btn_return"><?= _gettext("Return") ?></a>
                        </td>
                    </tr>
                </table>
            </div>
        </div>
    </form>
</div>
</body>
</html>
