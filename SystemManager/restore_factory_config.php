<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$page_name = 'm_sysmaintance';
include_once ($_SERVER ["DOCUMENT_ROOT"] . "/authenticed_writable.php");
 
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/plain; charset=utf-8" />
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<style type="text/css">
	label{
		font-size:12px;
		margin-right:20px;
		white-space: nowrap;
	}
	 
	.list p {
		margin-bottom:10px;
		text-align:center;
	}
	.msg{
		text-align:center;
	}

</style>
<script type="text/javascript" src="/js/prototype.js"></script>
<script language="javascript">
 	function okbt() {
		if($("deleteMsg").style.display!="none" || $("reserveMsg").style.display!="none") {
			var log_delete = 0;
			if($("delete").checked) log_delete = 1;
			parent.frames.cfgreset_commit(log_delete);
    		parent.frames.cfgreset.close();
		} else {
			if($("reserve").checked==true){
				$("selects").style.display="none";
				$("reserveMsg").style.display="block";
				$("deleteMsg").style.display="none";
			}
			if($("delete").checked==true){
				$("selects").style.display="none";
				$("reserveMsg").style.display="none";
				$("deleteMsg").style.display="block";
			}
		}
	}
	
function cancelbt() {
	parent.frames.cfgreset.close();
}
</script>

</head>
<body class="pop_body">
	<div id="fld_main">
		<div id="selects" class="list">
			<p>
			<input type="radio" name="restore" id="reserve" value="0" checked="checked">
			<label for="reserve"><?=_gettext('Restore factory settings');?>(<?=_gettext('Keep historical data');?>)</label>
			</p>
			<p>
				<input type="radio" name="restore" id="delete" value="1">
				<label for="delete"><?=_gettext('Restore factory settings');?>(<?=_gettext('Delete historical data');?>)</label>
			</p>
		</div>
		<div id="reserveMsg" class="msg" style="display:none;"><?=_gettext('reserveMsg');?></div>
		<div id="deleteMsg" class="msg" style="display:none;"><?=_gettext('deleteMsg');?></div>
		<div id="operate">
			<div>
				<input type="button" onclick="okbt();" class="btn_ok" value="<?=_gettext('OK');?>" />
				<input type="button" onclick="cancelbt();" class="btn_cancel" value="<?=_gettext('cancel');?>" />
			</div>
		</div>
	</div>
</body>
</html>
 
