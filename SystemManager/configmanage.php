<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
$nochecklic = true;
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed.php");	
isAvailable('m_sysmaintance');
// $auth = new authentication();
// if(! $auth->isWritable('m_sysmaintance'))
// {
// 	jumpTo(_gettext('nowritable'),"goback");
// }

$licenseExpire = false;
if(function_exists('lic_get_state'))
{
	$licenstatu = lic_get_state();
	if($licenstatu <0)
	$licenseExpire = true;
}

function autosave_file_get()
{
	$recordset = array();
	$dir  = '/home/config/current/autosave_config';

	if(!($handle = opendir($dir)))
	{
		return $recordset;
	}

	while(($file = readdir($handle)) !== false)
	{
		if(0 != strncmp($file,'config-', 7))
			continue;

		$recordset[] = $file;
	}
	
	closedir($handle);
	rsort($recordset);
	return $recordset;
}
$sautosavefile_arr = autosave_file_get();

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>IP_Strict</title>
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<style type="text/css">
.operate_table th{
	width:80px;
}
#submitBtn{
	text-decoration:none;
}
.operate_table th.w20{
	width: 20px !important;
    min-width: 20px !important;
    padding: 0px !important;
}
</style>
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
<script type="text/javascript" src="/js/popup.js"></script>
<script type="text/javascript">
	function showFileBox(rid) //仅显示当前文件选择框
	{
		$('currentcfgsel').style.display = 'none';
		$('currentcfgcomment').style.display = '';
		$('currentautosavecfgsel').style.display = 'none';
		$('currentautosavecfgcomment').style.display = '';
		
		if($(rid+'sel')){
			$(rid+'sel').style.display = '';
			$(rid+'comment').style.display = 'none';
		}
		
		if($('resetautosavemanage').checked)
		{
			$('currentautosavecfgsel').style.display = '';
		}
	}

	function configmanage_commit()
	{
		$("submitBtn").setAttribute('href','#');
		if($('backmanage').checked)
		{
			$("submitBtn").setAttribute('href','config_down.php?in_tabs=<?=$_GET["in_tabs"]?>');			
			//window.open("config_down.php","checkout","height=100,width=400,top=0,left=0,toolbar=no,menubar=no,scrollbars=no,resizable=no,location=no,status=no");
			//return false;
		}
		else if($('resetmanage').checked)
		{

			var currentcfgfile = $('currentcfgfile').value.toLowerCase();
			if(currentcfgfile.indexOf(".conf")<0){		
				alert('<?php echo _gettext("Please select a correct config file"); ?> ');		
				return false;
			}else{
				if(confirm('<?php echo _gettext("resetmanagealert"); ?> ')){		
					$("form1").submit();
				}
			}
		}		
		else if($('resetautosavemanage').checked)
		{
			if(confirm('<?php echo _gettext("resetmanagealert"); ?> ')){				
				$("form1").submit();
			}
		}
		else if($('cfgresetdefault').checked)
		{
			
			// if(confirm('<?php echo _gettext("cfgresetdefaultalert"); ?>')){			
			// 	$("form1").submit();
			// }
			cfgresetdefaults();
			
		}
	}
	function cfgresetdefaults() {
		cfgreset=new Popup({scrollType:'yes',isReloadOnClose:false,width:'450',height:'150'});
		cfgreset.setContent("contentUrl","restore_factory_config.php");
		cfgreset.setContent("title","<?php echo _gettext("cfgresetdefault");?>");
		cfgreset.build();
		cfgreset.show();
	}

	function cfgreset_commit(log_delete) {
		$("log_delete").value = log_delete;
		$("form1").submit();
	}
</script>
</head>
<body class="body">
<div id="fld_main" <?=in_tabs_class()?> >
	<form id="form1" name="form1" enctype="multipart/form-data" method="post" action="configmanage_commit.php?in_tabs=<?=$_GET['in_tabs']?>">
	<input type="hidden" name="tokenid" value="<?=$tokenid?>"/>
	<input type="hidden" name="log_delete" id="log_delete" value="0">
	<div class="operate">
		<div class="title"><?php echo _gettext('ConfigureBak&re'); ?></div>
    	<div class="operate_table">
			<table class="list wtwo">
				<tr>
					<th <?php if (isOemFeatureByName("changting")) echo 'class="w20"'; ?>></th>
					<td>
						<input id="backmanage" checked="checked" name="cfgmanage" value="bakcurrentcfg" type="radio" onclick="showFileBox('bakcurrentcfg')" /><label for="backmanage" class="alignLabel"><?php echo _gettext("confbak"); ?>( <?php echo _gettext("confbakcmt"); ?> )</label>
						
					</td>
				</tr>
				<tr>
					<th <?php if (isOemFeatureByName("changting")) echo 'class="w20"'; ?>></th>
					<td class="flexAlign">
						<input id="resetmanage" name="cfgmanage" value="resetcfg" type="radio" onclick="showFileBox('currentcfg')" <?php if($licenseExpire == true) echo "disabled"; ?>/><label for="resetmanage" class="alignLabel"><?php echo _gettext("cfgreset"); ?><span id="currentcfgcomment">( <?php echo _gettext("cfgresetcmt"); ?> )</span></label>
						<div class="fileBox" id="currentcfgsel" style="display:none;">
							<input type="text" name="filepath" readonly>
							<button class="btn upfile" onclick="document.form1.currentcfgfile.click()" id="logo_img_fake"><?= _gettext('Select_file'); ?></button>
							<input type="file" title=" " name="currentcfgfile" id="currentcfgfile" size='40' class="files" onChange="document.form1.filepath.value=this.value"value=""/>
						</div>
					</td>
				</tr>			
				<tr>
					<th <?php if (isOemFeatureByName("changting")) echo 'class="w20"'; ?>></th>
					<td >
						<input id="resetautosavemanage" name="cfgmanage" value="resetautosavecfg" type="radio" onclick="showFileBox('currentautosavecfg')" <?php if($licenseExpire == true) echo "disabled"; ?>/><label for="resetautosavemanage" class="alignLabel"><?php echo _gettext("cfgresetautosave"); ?></label>
						<span id="currentautosavecfgcomment">( <?php echo _gettext("cfgresetautosavecmt"); ?> )</span>

						<span id="currentautosavecfgsel" style="display:none;">
							<select name="autosavefile" style="margin-left:10px;" id="autosavefile" tabindex="20" class="show" ;>
							<!--<option value="">请选择</option>-->
								<?php
									foreach($sautosavefile_arr as $autosavefile)
									{
										echo "<option value='".$autosavefile."'>".$autosavefile."</option>";
									}
								?>
						</select>
						</span>
					</td>
				</tr>
				<tr>
					<th <?php if (isOemFeatureByName("changting")) echo 'class="w20"'; ?>></th>
					<td >
						<input id="cfgresetdefault" name="cfgmanage" value="cfgresetdefault" type="radio" onclick="showFileBox('cfgresetdefault')" /><label for="cfgresetdefault" class="alignLabel"><?php echo _gettext("cfgresetdefault"); ?>( <?php echo _gettext("cfgresetdefaultcmt"); ?> )</label>
						
					</td>
				</tr>
				<tr>
					<th <?php if (isOemFeatureByName("changting")) echo 'class="w20"'; ?>></th>
					<td class="operateBtn">
						<a id="submitBtn" class="btn_ok" href="###" onclick="configmanage_commit()"><?php echo _gettext("OK"); ?></a>
					</td>
				</tr>
			</table>
		</div>
	</div>
	</form>
	<div><?php if(isset($_REQUEST['msg']) && $_REQUEST['msg']) echo _gettext("note").': '.$_REQUEST['msg']; ?></div>
</div>
</body>
</html>
