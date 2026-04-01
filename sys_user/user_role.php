<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
require_once ($_SERVER ["DOCUMENT_ROOT"] . "model/lan.php");
include_once ($_SERVER ["DOCUMENT_ROOT"] . "/authenticed.php");
isAvailable ( 'm_systemadmin' );
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
<style type="text/css">
.list td a,.actions td a span{
	margin:0px !important;
}
 
</style>
</head>
<?php
include_once ("user_functions.php");
function showRecordRows() {
	$role_array = get_role_recordsets ();
	$i = 0;
	
	foreach ( $role_array as $roledata ) {
		$rowStr = "<tr class='actions'>";
		$rowStr .= "<td>" . ($i + 1) . "</td><td>" . _gettext ( $roledata ['name'] ) . "</td>";
		if ($roledata ['comment'] != "" && $roledata ['pre_define'] == 1) {
			$rowStr .= "<td>" . _gettext ( $roledata ['comment'] ) . "</td>";
		} elseif ($roledata ['comment'] != "")
			$rowStr .= "<td>" . $roledata ['comment'] . "</td>";
		else
			$rowStr .= "<td></td>";
			
			// $rowStr.="".get_privilege($roledata['user_func_num'], $roledata['user_func_idx'])."</td>";
		if ($roledata ['all'] != "") {
			$rowStr .= "<td>";
			if ($i > 1)
				$rowStr .= "<a href='user_role_commit.php?action=up_remove&name=" . urlencode ( $roledata ["name"] ) . "'>";
			$rowStr .= _gettext ( 'moveUp' ) . "<span></span>";
			if ($i > 1)
				$rowStr .= "</a>";
			if ($i > 0 && ($i + 1) != count ( $role_array ))
				$rowStr .= "<a href='user_role_commit.php?action=down_remove&name=" . urlencode ( $roledata ["name"] ) . "'>";
			$rowStr .= _gettext ( 'moveDown' ) . "<span></span>";
			if ($i > 0 && ($i + 1) != count ( $role_array ))
				$rowStr .= "</a>";
			$rowStr .= _gettext ( 'Modify' ) . "<span></span>";
			$rowStr .= _gettext ( 'Delete' ) . "</td>";
		} else if ($roledata ['pre_define'] == 1) {
			$rowStr .= "<td>";
			if ($i > 1)
				$rowStr .= "<a href='user_role_commit.php?action=up_remove&name=" . urlencode ( $roledata ["name"] ) . "'>";
			$rowStr .= _gettext ( 'moveUp' ) . "<span></span>";
			if ($i > 1)
				$rowStr .= "</a>";
			if ($i > 0 && ($i + 1) != count ( $role_array ))
				$rowStr .= "<a href='user_role_commit.php?action=down_remove&name=" . urlencode ( $roledata ["name"] ) . "'>";
			$rowStr .= _gettext ( 'moveDown' ) . "<span></span>";
			if ($i > 0 && ($i + 1) != count ( $role_array ))
				$rowStr .= "</a>";
			$rowStr .= _gettext ( 'Modify' ) . "<span></span>";
			$rowStr .= _gettext ( 'Delete' ) . "</td>";
		} else {
			$rowStr .= "<td>";
			if ($i > 1)
				$rowStr .= "<a href='user_role_commit.php?action=up_remove&name=" . urlencode ( $roledata ["name"] ) . "'>";
			$rowStr .= _gettext ( 'moveUp' ) . "<span></span>";
			if ($i > 1)
				$rowStr .= "</a>";
			if ($i > 0 && ($i + 1) != count ( $role_array ))
				$rowStr .= "<a href='user_role_commit.php?action=down_remove&name=" . urlencode ( $roledata ["name"] ) . "'>";
			$rowStr .= _gettext ( 'moveDown' ) . "<span></span>";
			if ($i > 0 && ($i + 1) != count ( $role_array ))
				$rowStr .= "</a>";
			$rowStr .= "<a href='user_role_edit.php?name=" . urlencode ( $roledata ["name"] ) . "'>" . _gettext ( 'Modify' ) . "" . "</a>";
			$rowStr .= "<a class='delLink' title='" . _gettext ( 'suredel' ) . "' href='user_role_commit.php?action=delete&name=" . urlencode ( $roledata ["name"] ) . "'> " . _gettext ( 'Delete' ) . "</a></td>";
		}
		
		$rowStr .= "</tr>\r\n";
		
		echo $rowStr;
		unset ( $rowStr );
		unset ( $roledata );
		
		$i ++;
	}
}

?>
<script type="text/javascript">
	new MoveOverLightTable("adminTable");
	new ConfirmDelForLink();
	function cancelbt() {
		parent.frames.showUserrole.close();
	}
</script>

<body>
	<div id="fld_main">
		<form id="form1"   name="form1" method="post"
			action="">
			<input type="hidden" name="tokenid" value="<?=$tokenid?>"/>
			<table id="adminTable" class="table_list">
				<caption>
					<div class="table_title"><?=_gettext('m_systemrole');?></div>  
					<div class="center title_btn_left">
						<input class="btn_add" type="button" value="<?=_gettext('Add');?>"
							onclick="javascript:location.href='user_role_add.php'" />
					</div>
				</caption>
				<thead>
					<tr>
						<th class="index"><?=_gettext('Index');?></th>
						<th><?=_gettext('role_name');?></th>
						<th><?=_gettext('role_comment');?></th>
						<th width="200"><?=_gettext('Operation');?></th>
					</tr>
				</thead>
				<tbody>
				<?php  showRecordRows();  ?>
			</tbody>
			</table>
		 
			<input name="text_static_command" type="hidden"
				id="text_static_command" value="">
		
		</form>
		 

	</div>
<p class="crumbs" ><?=_gettext('role_range_tips')?></p>
</body>
</html>
