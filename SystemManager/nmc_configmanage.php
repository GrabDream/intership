<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticate.php");
$auth = new authentication();
if(! $auth->isWritable('m_sysmaintance'))
{
	jumpTo(_gettext('nowritable'),"goback");
}

$licenseExpire = false;
if(function_exists('lic_get_state'))
{
	$licenstatu = lic_get_state();
	if($licenstatu <0)
	$licenseExpire = true;
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>IP_Strict</title>
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<style type="text/css">
.alignLabel{
	width:120px;
	display:inline-block;
}
</style>
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript">
	function showFileBox(rid) //仅显示当前文件选择框
	{
		//$('resetsel').checked = true;
		var radios = new Array('currentcfgsel');
		for(i=0; i< radios.length;i++)
		{
				$(radios[i]).style.display = 'none';
				$('comment').style.display = '';
		}
		if($(rid+'sel')){
			$(rid+'sel').style.display = '';
			$('comment').style.display = 'none';
		}
	}

	function configmanage_commit()
	{
		$("submitBtn").setAttribute('href','#');
		if($('backmanage').checked)
		{
			$("submitBtn").setAttribute('href','nmc_config_down.php');
			//window.open("config_down.php","checkout","height=100,width=400,top=0,left=0,toolbar=no,menubar=no,scrollbars=no,resizable=no,location=no,status=no");
			//return false;
		}
		else if($('resetmanage').checked)
		{
			if(confirm('<?php echo _gettext("resetmanagealert"); ?> ')){				
				$("form1").submit();
			}
		}
		else if($('cfgresetdefault').checked)
		{
			if(confirm('<?php echo _gettext("cfgresetdefaultalert"); ?>')){			
				$("form1").submit();
			}
		}
	}
</script>
</head>
<body class="body">
<div class="navigation"> 
<img src="/images/house.gif" class="navImg" /> 
</div>
<div id="fld_main">
	<form id="form1" name="form1" enctype="multipart/form-data" method="post" action="configmanage_commit.php">
	<input type="hidden" name="tokenid" value="<?=$tokenid?>"/>
		<table class="list wtwo">
			<caption>
				<div class="leftTop"></div>
				<div class="center"><?php echo _gettext('ConfigureBak&re'); ?></div>
				<div class="rightTop"></div>
				<div class="floatRight">
					<a id="submitBtn" class="btn_ok" href="###" onclick="configmanage_commit()"><?php echo _gettext("OK"); ?></a>
				</div>
			</caption>
			<tr>
				<td >
					<input id="backmanage" checked="checked" name="cfgmanage" value="bakcurrentcfg" type="radio" onclick="showFileBox('bakcurrentcfg')" /><label for="backmanage" class="alignLabel"><?php echo _gettext("confbak"); ?></label>
					( <?php echo _gettext("confbakcmt"); ?> )
				</td>
			</tr>
			<tr>
				<td >
					<input id="resetmanage" name="cfgmanage" value="resetcfg" type="radio" onclick="showFileBox('currentcfg')" <?php if($licenseExpire == true) echo "disabled"; ?>/><label for="resetmanage" class="alignLabel"><?php echo _gettext("cfgreset"); ?></label>
					<span id="comment">( <?php echo _gettext("cfgresetcmt"); ?> )</span>
					<div class="fileBox" id="currentcfgsel" style="display:none;">
						<input type="text" name="filepath" readonly>
						<button class="btn upfile" onclick="document.form1.currentcfgfile.click()" id="logo_img_fake"><?= _gettext('Select_file'); ?></button>
						<input type="file" title=" " name="currentcfgfile" id="currentcfgfile" size='40' class="files" onChange="document.form1.filepath.value=this.value"value=""/>
					</div>
				</td>
			</tr>
			<tr>
				<td >
					<input id="cfgresetdefault" name="cfgmanage" value="cfgresetdefault" type="radio" onclick="showFileBox('cfgresetdefault')" /><label for="cfgresetdefault" class="alignLabel"><?php echo _gettext("cfgresetdefault"); ?></label>
					( <?php echo _gettext("cfgresetdefaultcmt"); ?> )
				</td>
			</tr>
		</table>
	</form>
	<div><?php if(isset($_REQUEST['msg']) && $_REQUEST['msg']) echo _gettext("note").': '.$_REQUEST['msg']; ?></div>
</div>
</body>
</html>
