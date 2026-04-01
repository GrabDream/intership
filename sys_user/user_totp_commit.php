<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = "m_systemadmin";
include_once($_SERVER ["DOCUMENT_ROOT"] . "/authenticed.php");
isAvailable('m_systemadmin');
$apiclass = new ApiInter();
if ($_POST['action'] == 1 || $_POST['action'] == 4){
    $ret = $apiclass->setApiInterData("/netmanage/userauth/UserTotp/verfiyTotp");
    echo json_encode($ret, JSON_UNESCAPED_UNICODE);
    return;
}
echo json_encode(['code' => 1, 'massage' => _gettext('fail')], JSON_UNESCAPED_UNICODE);
return;
