<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
	include_once($_SERVER["DOCUMENT_ROOT"]."/authenticed.php");
	isAvailable('m_systemadmin');
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="/css/common.css" rel="stylesheet" type="text/css" />
<link href="/css/skin.css" rel="stylesheet" type="text/css" />
<link rel="StyleSheet" href="/css/dtree.css" type="text/css" />

<script type="text/javascript" src="/js/dtreeGroup.js"></script>
<script type="text/javascript" src="/js/prototype.js"></script>
<script type="text/javascript" src="/js/base.js"></script>
<script type="text/javascript" language="javascript">

Event.observe(window,'load',function(){
	//new AllCheck('look_smal_all1','looks');
	//new AllCheck('look_all','looks');	
});


/*显示影藏树*/  
               //          二级数   三级数
function display(obj, img,  son ,  id_max)
{

   if($(obj).style.display=="none")
     
   {
          $(img).src="/images/minus_down.gif"
    
		  if(obj.indexOf("two")>-1)//zi
		  {
			 $(obj).style.display="";
			 for(i=1;i<=id_max;i++)
			 {
			  $(obj+'_grandson'+i).style.display=""; 
			 }
			 
		  }
		  else
		  {
			  $(obj).style.display="";
			  for(i=1;i<=son;i++)
			  {
			   $(obj+'_son'+i).style.display="";
			  }
		  }
   }
   else
   {
        $(img).src="/images/plus_right.gif"
    
          if(obj.indexOf("two")>-1)
		  {
			 $(obj).style.display="none";
			 for(i=1;i<=id_max;i++)
			 {
			  $(obj+'_grandson'+i).style.display="none"; 
			 }
		  }
		  else
		  {
			  $(obj).style.display="none";
			  
			  for(i=1;i<=son;i++)
			  {
			  $(obj+'_son'+i).style.display="none";
			  
			   
				for(j=1;j<=2;j++)
			   {
	            $("two_parent1_son"+i+'_grandson'+j).style.display="none";
				$("two_parent1_img"+i).src="/images/plus_right.gif"
			   }
			  }
			  
              
		  }
   }


}

/*点击’ 查看权限‘ 第二级菜单的复选框时 ，选中第二级菜单下所有子菜单复选框*/
function checkBox(obj,nameClass)
{
    var checkall = $$("input."+nameClass);
  //var checkall = document.getElementsByName(name);
        var check = document.getElementById(obj);
        for(var i = 0;i < checkall.length; i ++)
        {
            if(checkall[i].type == "checkbox" && check.checked == true )
            {
                checkall[i].checked = true;
            }
            else
            {
                checkall[i].checked = false;
            }
        }

}


/*点击’ 编辑权限‘ 第二级菜单的复选框时 ，选中编辑菜单 第二级菜单下所有子菜单复选框 
和 查看权限 第二级菜单下所有子菜单复选框*/
function checkBox2(obj,nameClass,obj2,nameClass2)
{

   //var checkall = document.getElementsByName(name);
   // var checkall2 = document.getElementsByName(name2);
	
	var checkall = $$("input."+nameClass);
	var checkall2= $$("input."+nameClass2);
	
     var check = document.getElementById(obj);
     	   if($(obj).checked==true)
		 {
		   $(obj2).checked=true;
		 }
		 else
		 {
		   $(obj2).checked=false;
		 }
        for(var i = 0;i < checkall.length; i ++)
        {
            if(checkall[i].type == "checkbox" && checkall2[i].type=="checkbox" && check.checked == true )
            {
                checkall[i].checked = true;
				checkall2[i].checked = true;
				//$(obj2).checked =true;
            }
            else
            {
                checkall[i].checked = false;
				checkall2[i].checked = false;
			//	$(obj2).checked =false;
            }
	
        }
}

/*点击’ 编辑权限‘ 第一级菜单的复选框时 ，选中编辑菜单 第一级菜单下所有子菜单复选框 
和 查看权限 第一级菜单下所有子菜单复选框*/
function checkClass(idName,className,className2)
{
	 var a = document.getElementsByTagName("input");
	  if($(idName).checked==true)
	  {
		   for (var i=0; i<a.length; i++) 
			{
			  if (a[i].type == "checkbox" && a[i].className.indexOf(className)>-1)
			  { 
				 a[i].checked = true;
			  }
			  if (a[i].type == "checkbox" && a[i].className.indexOf(className2)>-1)
			  { 
				 a[i].checked = true;
			  }
		   }
	  }
	  else
	  {
		 for (var i=0; i<a.length; i++) 
			{
			  if (a[i].type == "checkbox" && a[i].className.indexOf(className)>-1)
			  { 
				 a[i].checked = false;
			  }
			  if (a[i].type == "checkbox" && a[i].className.indexOf(className2)>-1)
			  { 
				 a[i].checked = false;
			  }
		   }
	  
	  }
	
	
	/*
	var chks = $$("input."+className);
	var looks= $$("input."+className2);
			if($(idName).checked==true)
			{

				for(var i = chks.length - 1; i >= 0; i--)
				{
			
					chks[i].checked = true;
					looks[i].checked = true;
					//$(idName2).checked = true;
					//looks[chks.length].checked = true;
				}
			}
			else
			{
				for(var i = chks.length - 1; i >= 0; i--)
				{
					chks[i].checked = false;
					looks[i].checked = false;
					//looks[chks.length].checked = false;
					//$(idName2).checked = false;
				}
		   }
		   
		   */
		
}

function checkClassUp(idName,className,className2)
{

 var chks = $$("input."+className);
	var looks= $$("input."+className2);
			if($(idName).checked==true)
			{

				for(var i = chks.length - 1; i >= 0; i--)
				{
			
					chks[i].checked = true;
					looks[i].checked = true;
					//$(idName2).checked = true;
					//looks[chks.length].checked = true;
				}
			}
			else
			{
				for(var i = chks.length - 1; i >= 0; i--)
				{
					chks[i].checked = false;
					looks[i].checked = false;
					//looks[chks.length].checked = false;
					//$(idName2).checked = false;
				}
		   }
}


/*点击’ 查看权限‘ 第一级菜单的复选框时 ，选中第一级菜单下所有子菜单复选框*/
function LookClass(idName,className)
{

     var a = document.getElementsByTagName("input");
	  if($(idName).checked==true)
	  {
		   for (var i=0; i<a.length; i++) 
			{
			  if (a[i].type == "checkbox" && a[i].className.indexOf(className)>-1)
			  { 
				 a[i].checked = true;
			  }
		   }
	  }
	  else
	  {
		 for (var i=0; i<a.length; i++) 
			{
			  if (a[i].type == "checkbox" && a[i].className.indexOf(className)>-1)
			  { 
				 a[i].checked = false;
			  }
		   }
	  
	  }
	
/*

	var looks= $$("input."+className);
			if($(idName).checked==true)
			{
				for(var i = looks.length - 1; i >= 0; i--)
				{
					looks[i].checked = true;
				}
			}
			else
			{
				for(var i = looks.length - 1; i >= 0; i--)
				{
					looks[i].checked = false;
				}
		   }
		*/
}


function LookClassDown(idName,className)
{

  var looks= $$("input."+className);
	if($(idName).checked==true)
	{
		for(var i = looks.length - 1; i >= 0; i--)
		{
			looks[i].checked = true;
		}
	}
	else
	{
		for(var i = looks.length - 1; i >= 0; i--)
		{
			looks[i].checked = false;
		}
   }

}

/*点击查看权限时 ，选中所有查看权限复选框*/
function lookAll()
{

  var a = document.getElementsByTagName("input");
  if($('look_all').checked==true)
  {
	   for (var i=0; i<a.length; i++) 
		{
		  if (a[i].type == "checkbox" && a[i].className.indexOf("sys_look")>-1)
		  { 
			 a[i].checked = true;
		  }
	   }
  }
  else
  {
     for (var i=0; i<a.length; i++) 
		{
		  if (a[i].type == "checkbox" && a[i].className.indexOf("sys_look")>-1)
		  { 
			 a[i].checked = false;
		  }
	   }
  
  }
}


/*点击查看权限时 ，选中所有查看权限复选框*/
function reporter_lookAll()
{
   var a = document.getElementsByTagName("input");
  if($('reporter_look_all').checked==true)
  {
	   for (var i=0; i<a.length; i++) 
		{
			if(a[i].disabled) continue;
		  if (a[i].type == "checkbox" && a[i].className.indexOf("rp_look")>-1)
		  { 
			 a[i].checked = true;
		  }
	   }
  }
  else
  {
     for (var i=0; i<a.length; i++) 
		{
			if(a[i].disabled) continue;
		  if (a[i].type == "checkbox" && a[i].className.indexOf("rp_look")>-1)
		  { 
			 a[i].checked = false;
		  }
	   }
  
  }
}


/*点击编辑权限时 ，选中所有 编辑权限复选框 和 所有查看权限复选框*/

function checkAll()
{	
  var a = document.getElementsByTagName("input");
  if($('edit_all').checked==true)
  {
        $("look_all").checked=true;
	   for (var i=0; i<a.length; i++) 
		{
		  if (a[i].type == "checkbox" && a[i].className.indexOf("sys_check")>-1)
		  { 
			 a[i].checked = true;
		  }
		  if (a[i].type == "checkbox" && a[i].className.indexOf("sys_look")>-1)
		  { 
			 a[i].checked = true;
		  }
	   }
  }
  else
  {
     $("look_all").checked=false;
     for (var i=0; i<a.length; i++) 
		{
		  if (a[i].type == "checkbox" && a[i].className.indexOf("sys_check")>-1)
		  { 
			 a[i].checked = false;
		  }
		  if (a[i].type == "checkbox" && a[i].className.indexOf("sys_look")>-1)
		  { 
			 a[i].checked = false;
		  }
	   }
  
  }
	
}


function reporter_checkAll()
{
 
  var a = document.getElementsByTagName("input");
  if($('reporter_edit_all').checked==true)
  {
  
       $('reporter_look_all').checked=true;
	   for (var i=0; i<a.length; i++) 
		{
			if(a[i].disabled) continue;
		  if (a[i].type == "checkbox" && a[i].className.indexOf("rp_check")>-1)
		  { 
			 a[i].checked = true;
		  }
		  if (a[i].type == "checkbox" && a[i].className.indexOf("rp_look")>-1)
		  { 
			 a[i].checked = true;
		  }
		  
	   }
  }
  else
  {
     $('reporter_look_all').checked=false;
     for (var i=0; i<a.length; i++) 
		{
			if(a[i].disabled) continue;
		  if (a[i].type == "checkbox" && a[i].className.indexOf("rp_check")>-1)
		  { 
			 a[i].checked = false;
		  }
		  if (a[i].type == "checkbox" && a[i].className.indexOf("rp_look")>-1)
		  { 
			 a[i].checked = false;
		  }
		  
	   }
  
  }

}

function clickchceck3(nameId1,nameId2)
{
	if($(nameId1).checked==true)
	{
		$(nameId2).checked=true;
		
	}
	else
	{
		$(nameId2).checked=false;
		
	}
	}
</script>
<?php
include_once("user_functions.php");

if(!isset($_GET['name']))
	echo '<script language="javascript">alert("'._gettext('fail:')._gettext('name_is_null').');location.href="user_role.php";</script>';

if(empty($_GET['name']))
	echo '<script language="javascript">alert("'._gettext('fail:')._gettext('name_is_null').');location.href="user_role.php";</script>';
	
$role_array = get_role_recordsets();
$functions_array1 = get_func_recordsets();
$fuctions_array = array();
$funtions_idx=0;
$lines = @file('/home/ace_menu_config.conf');
if($lines)
{

	foreach ($lines as $data)
	{
		if ( $data[0]=='#' )
		{
			continue;
		}
		$data = trim($data);
		if ( strcmp($data, "ipv6")==0 )
		{
			$ipv6 = 1;
		}
		else if ( strcmp($data, "vpn")==0 )
		{
			$vpn = 1;
		}
		else if ( strcmp($data, "!hotel")==0 )
		{
			$hotel = 1;				
		}
	}
}

if ( file_exists( '/home/config/cfg-scripts/bypass_switch_config') )
{
	$bypass=1;
}
else
{
	$bypass=0;	
}

if (file_exists('/usr/sbin/adsl-start'))
{
	$route_mode = 1;
}
else
{
	$route_mode = 0;
}
if (file_exists('/usr/private/pptpd'))
{
	$vpn_support = 1;
}
else
{
	$vpn_support = 0;
}

if (file_exists('/usr/private/openvpn'))
{
	$ssl_vpn_support = 1;
}
else
{
	$ssl_vpn_support = 0;
}

if (file_exists('/etc/firewall/ad'))
{
	$ad_support = 1;
}
else
{
	$ad_support = 0;
}
if (file_exists('/etc/firewall/av'))
{
	$av_support = 1;
}
else
{
	$av_support = 0;
}
if (file_exists('/etc/firewall/ips'))
{
	$ips_support = 1;
}
else
{
	$ips_support = 0;
}
if (file_exists('/etc/firewall/waf'))
{
	$waf_support = 1;
}
else
{
	$waf_support = 0;
}
if (file_exists('/etc/firewall/vs'))
{
	$vs_support = 1;
}
else
{
	$vs_support = 0;
}
$fj_flag = 0;

if ( file_exists( '/etc/rzxflag.conf') )
{
	$rzx_fj=1;
	$fj_flag=1;
}
else
{
	$rzx_fj=0;
}

if ( file_exists( '/usr/local/squid_install/sbin/squid') )
{
	$squid=1;
}
else
{
	$squid=0;	
}

if ( file_exists( '/etc/rzxflag.conf') )
{
	$rzx_fj=1;
	$fj_flag=1;
}
else
{
	$rzx_fj=0;
}

if ( file_exists( '/etc/hengbangflag.conf') )
{
	$hengbang_fj=1;
	$fj_flag=1;	
}
else
{
	$hengbang_fj=0;	
}

if (file_exists("/etc/outerReporter.conf"))
{
	$reporter_config = array();

	getConfig();
	if (array_key_exists("inner_function", $reporter_config))
		$inner_function = intval($reporter_config["inner_function"]);
	else
		$inner_function = 1;

	$noharddisk=!$inner_function;
}
else
{
	$noharddisk=0;
}

$nohard = 0;
if(file_exists("/tmp/.noharddisk.conf") || file_exists("/home/.noharddisk.conf")) {
	$nohard = 1;
}

for($idx = 0 ; $idx < count($functions_array1) ; $idx++)
{
	if ($functions_array1[$idx]['hide']=='1')
	{
		continue;
	}
	
	if ($functions_array1[$idx]['ipv6']=='1')
	{
		continue;
	}
	
	if ($functions_array1[$idx]['bypass']=='1' && $bypass!=1 )
	{
		continue;		
	}
	
	if ($functions_array1[$idx]['squid']=='1' && $squid!=1 )
	{
		continue;		
	}
	
	if ($functions_array1[$idx]['fj_flag']=='1' && $fj_flag==0 )
	{
		continue;		
	}
	
	if ($functions_array1[$idx]['route_mode']=='1' && $route_mode==0 )
	{
		continue;		
	}
	
	if ($functions_array1[$idx]['route_mode']=='0' && $route_mode==1 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['pbbg_fj']=='1' && $pbbg_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['hb_fj']=='1' && $hb_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['rzx_fj']=='1' && $rzx_fj==0 )
	{
		continue;		
	}
	
	if ($functions_array1[$idx]['ga_fj']=='1' && $ga_fj==0 )
	{
		continue;		
	}
	
	/*
	if ($functions_array1[$idx]['ha']=='1' && $currentWorkMode != 0 )
	{
		continue;
	}
	*/
	
	if ($functions_array1[$idx]['vpn']=='1' && $vpn_support == 0 )
	{
		continue;
	}
	
	if ($functions_array1[$idx]['ssl-vpn']=='1' && $ssl_vpn_support == 0 )
	{
		continue;
	}
	
	if ($functions_array1[$idx]['ad']=='1' && $ad_support == 0 )
	{
		continue;
	}
	
	if ($functions_array1[$idx]['av']=='1' && $av_support == 0 )
	{
		continue;
	}

	if ($functions_array1[$idx]['ips_vs']=='1' && $ips_support == 0 && $vs_support == 0 )
	{
		continue;
	}
	
	if ($functions_array1[$idx]['ips']=='1' && $ips_support == 0 )
	{
		continue;
	}
	
	if ($functions_array1[$idx]['waf']=='1' && $waf_support == 0 )
	{
		continue;
	}
		
	if ($functions_array1[$idx]['vs']=='1' && $vs_support == 0 )
	{
		continue;
	}
	
	if ($functions_array1[$idx]['hengbang_fj']=='1' && $hengbang_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['fh_fj']=='1' && $fh_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['zkxy_fj']=='1' && $zkxy_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['kgzt_fj']=='1' && $kgzt_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['xh_fj']=='1' && $xh_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['xw_fj']=='1' && $xw_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['zwsl_fj']=='1' && $zwsl_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['ym_fj']=='1' && $ym_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['fhwiff_fj']=='1' && $fhwiff_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['bhbcp_fj']=='1' && $bhbcp_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['aswa_fj']=='1' && $aswa_fj==0 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['hotel']=='1' && $hotel!=1 )
	{
		continue;		
	}
	if ($functions_array1[$idx]['noharddisk']=='1' && $noharddisk==1 )
	{
		continue;		
	}

	if ($functions_array1[$idx]['nohard']=='1' && $nohard==1 )
	{
		continue;		
	}
	
	if ($functions_array1[$idx]['af']=='1' && $licenstatu!=1 )
	{
		continue;		
	}	
	/* if (isset($functions_array1[$idx]['module_ctrl']))
	{
			if (!((1<<$functions_array1[$idx]['module_ctrl'])&($lic_ext_detail['normal_module']|$lic_ext_detail['demo_module'])))
			{
					continue;
			}
	} */
	$fuctions_array[$funtions_idx]=$functions_array1[$idx];
	$funtions_idx++;
}



$reporter_fuctions_array1 = get_reporter_func_recordsets();
$reporter_fuctions_array = array();
$funtions_idx1=0;
for($idx = 0 ; $idx < count($reporter_fuctions_array1) ; $idx++)
{
	
        
    
	
	if ($reporter_fuctions_array1[$idx]['hide']=='1')
	{
		continue;
	}
	if (strcmp($reporter_fuctions_array1[$idx]['name'],'rmenu_userBehavior')==0)
	{
		continue;
	}
	if (strcmp($reporter_fuctions_array1[$idx]['name'],'rmenu_log_online_nat')==0 && !is_file('/etc/report_nat_log'))
	{
		continue;
	}
	if (strcmp($reporter_fuctions_array1[$idx]['name'],'rmenu_big_data')==0 && !is_file('/var/www/reporter/da/index.php'))
	{
		continue;
	} 
	
			
	$reporter_fuctions_array[$funtions_idx1]=$reporter_fuctions_array1[$idx];
	$funtions_idx1++;
}
foreach($role_array as $roledata)		
{
	if(trim($roledata['name']) == trim($_GET['name']))
		break;	
}

?>
<style type="text/css">
.xing
{
   color:#FF0000;
}
.tables{
	overflow-Y: scroll;
	margin:0px 20px 10px 20px;
	height:200px;
	border-bottom:1px solid #cccccc;
}
.add_list{
	width:100%  !important;
}
 .list_title{
	width: 94.3%  !important;
    margin: 0px 20px;
 }
 
</style>
</head>
<body >
<div id="fld_main">
<div class="menu_2"></div>
	<form id="form1"  name="form1" method="post" action="user_role_commit.php?action=edit">
	<input type="hidden" name="tokenid" value="<?=$tokenid?>"/>
		<table id="adminTable" class="list wtwo">
			<caption>
				<div class="leftTop"></div>
				<div class="center"><?=_gettext('editsystemrole');?></div>
				<div class="rightTop"></div>
				 
			</caption>
			<tbody>
				<tr>
					<th class="name"><label class="xing">*</label> <?=_gettext('role_name');?></th>
					<td ><?=$roledata['name'];?>
					<input type="hidden" name="name" value="<?=$roledata['name'];?>"/></td>
				</tr>
				<tr>
					<th><?=_gettext('role_comment');?></th>
					<td ><input type="text" name="comment"  size="20" value="<?=$roledata['comment'];?>"/> </td>
				</tr>
				<tr>
				<td colspan="2">
					<table class="add_list list_title">
						<thead>
							<tr>
								<th  ><?=_gettext('SNMP');?></th>
								<th align="center" style="width:120px !important;">
									<span class="divs"><?=_gettext('editPermissions');?> </span>
									<input name="checkbox" type="checkbox" id="edit_all" onclick="checkAll()" /></th>
								<th align="center" style="width:119px !important;"><span class="divs"><?=_gettext('seachPermissions');?></span> <input type="checkbox" id="look_all" name="look_all" onclick="lookAll()" /></th>
							</tr>
						</thead>
					</table>
			<div class="tables">
				<table id="adminTable" class="add_list" >
				<?php
		
					$idx_level1=0;
					$idx_level2=1;
					$idx_image=1;
					$commit_idx=1;
					$total_number = count($fuctions_array);
					for($idx = 1 ; $idx <= $total_number ; $idx++)
					{
						if ($fuctions_array[$idx]['level']=='1')
						{
							if ($idx!=1)
								echo '</tbody>';
							$idx_level1++;
							echo "<tr>";
							echo "<td class=\"alignLeft\" ><img  id=\"one_parent".$idx_level1."_img".$idx_image."\" src=\"/images/plus_right.gif\" />"._gettext($fuctions_array[$idx]['name'])."</td>";
							echo "<td style='width:120px;'><input type=\"checkbox\" id=\"check_smal_all_".$idx_level1."\" class=\"sys_checks".$idx_level1."\" name=\"check_smal_all_".$idx_level1."\" onclick=\"checkClassUp('check_smal_all_".$idx_level1."','sys_checks".$idx_level1."','sys_looks".$idx_level1."')\" /></td>";
							echo "<td style='width:120px;'> <input type=\"checkbox\" id=\"look_smal_all_".$idx_level1."\" class=\"sys_looks".$idx_level1."\" name=\"select_all\" onclick=\"LookClassDown('look_smal_all_".$idx_level1."','sys_looks".$idx_level1."')\" /></td>";

							echo "</tr>";
							echo "<tbody id=\"one_parent".$idx_level1."\" >";
							$idx_level2=1;
							$idx_image++;
							continue;
						}
						else if ($fuctions_array[$idx]['level']=='2')
						{
				
							echo "<tr id=\"one_parent".$idx_level1."_son".$idx_level2."\" >";
							echo "<input type=\"hidden\" value=\"0\" name=\"one_parent".$idx_level1."_son".$idx_level2."_count\" id=\"one_parent".$idx_level1."_son".$idx_level2."_count\"/>";
							echo "<td  class=\"alignLeft\" style=\"padding-left:30px;\" >"._gettext($fuctions_array[$idx]['name'])."</td>";


							$match=0;
							for ($role_idx=1 ; $role_idx<=$total_number; $role_idx++)
							{
								if (strcmp($roledata[strval($role_idx)] , $fuctions_array[$idx]['name'])==0 )
								{
									$match=1;
									break;
								}
								if (strcmp($roledata[strval($role_idx+1000)] , $fuctions_array[$idx]['name'])==0 )
								{
									$match=2;
									break;
								}
							}
							
							if ( $match==1)
							{
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"sys_checks".$idx_level1."\" value=\"".$fuctions_array[$idx]['name']."\" checked=\"checked\"  name=\"".$commit_idx."\" id=\"check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('check".$idx_level1."_".$idx_level2."','check".$idx_level1."_".$idx_level2."[]','look".$idx_level1."_".$idx_level2."','look".$idx_level1."_".$idx_level2."[]')\" /></td>";
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"sys_looks".$idx_level1."\" value=\"".$fuctions_array[$idx]['name']."\" checked=\"checked\"  name=\"read_".$commit_idx."\" id=\"look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('look".$idx_level1."_".$idx_level2."','look".$idx_level1."_".$idx_level2."[]')\" /></td>";
							}
							else if ( $match==2)
							{
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"sys_checks".$idx_level1."\" value=\"".$fuctions_array[$idx]['name']."\" name=\"".$commit_idx."\" id=\"check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('check".$idx_level1."_".$idx_level2."','check".$idx_level1."_".$idx_level2."[]','look".$idx_level1."_".$idx_level2."','look".$idx_level1."_".$idx_level2."[]')\" /></td>";
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"sys_looks".$idx_level1."\" value=\"".$fuctions_array[$idx]['name']."\" checked=\"checked\"  name=\"read_".$commit_idx."\" id=\"look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('look".$idx_level1."_".$idx_level2."','look".$idx_level1."_".$idx_level2."[]')\" /></td>";
							}
							else
							{
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"sys_checks".$idx_level1."\" value=\"".$fuctions_array[$idx]['name']."\" name=\"".$commit_idx."\" id=\"check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('check".$idx_level1."_".$idx_level2."','check".$idx_level1."_".$idx_level2."[]','look".$idx_level1."_".$idx_level2."','look".$idx_level1."_".$idx_level2."[]')\" /></td>";
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"sys_looks".$idx_level1."\" value=\"".$fuctions_array[$idx]['name']."\" name=\"read_".$commit_idx."\" id=\"look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('look".$idx_level1."_".$idx_level2."','look".$idx_level1."_".$idx_level2."[]')\" /></td>";
							}

							echo "</tr>";
							$idx_level2++;	
							$commit_idx++;						
							$idx_image++;
							
							continue;							

						}
						else
						{
							continue;	
						}
					}
				?>

		</table>					
				</div>			
		<table class="add_list list_title">
			<thead>
				<tr>
					<th ><?=_gettext('reporterMode');?></th>
					<th align="center" style="width:120px !important;"><span class="divs"><?=_gettext('editPermissions');?></span> <input type="checkbox" id="reporter_edit_all" onclick="reporter_checkAll()" /></th>
					<th align="center" style="width:119px !important;"><span class="divs"><?=_gettext('seachPermissions');?></span> <input type="checkbox" id="reporter_look_all" name="reporter_look_all" onclick="reporter_lookAll()" /></th>
				</tr>
			</thead>
		</table>
	<div class="tables">
		<table id="adminTable" class="add_list">
				<?php
	
					$idx_level1=200;
					$idx_level2=1;
					$idx_level3=1;
					$commit_idx=201;
					$total_number = count($reporter_fuctions_array);
					for($idx = 0 ; $idx <= $total_number ; $idx++)
					{
						if ($reporter_fuctions_array[$idx]['level']=='1')
						{
							$ck1 = "";
							$ck2 = "";
							$disabled = "";
							if(is_array($roledata["reporter"]) && $idx == 0) {
								$ck1 = "checked=\"checked\"";
								$ck2 = "checked=\"checked\"";
								$disabled = "disabled";
							}
							
							if ($idx!=1)
								echo '</tbody>';
							$idx_level1++;
							echo "<tr>";
							echo "<td class=\"alignLeft\" ><img  id=\"one_parent".$idx_level1."_img".$idx_image."\" src=\"/images/plus_right.gif\" />"._gettext($reporter_fuctions_array[$idx]['name'])."</td>";
							echo "<td style='width:120px;'><input type=\"checkbox\" {$ck1} {$disabled} id=\"rp_check_smal_all_".$idx_level1."\" class=\"rp_checks".$idx_level1."\"  name=\"rp_check_smal_all_".$idx_level1."\" onclick=\"checkClass('rp_check_smal_all_".$idx_level1."','rp_checks".$idx_level1."','rp_looks".$idx_level1."')\" /></td>";
							echo "<td style='width:120px;'> <input type=\"checkbox\" {$ck2} {$disabled} id=\"rp_look_smal_all_".$idx_level1."\" class=\"rp_looks".$idx_level1."\" name=\"rp_look_smal_all_".$idx_level1."\" onclick=\"LookClass('rp_look_smal_all_".$idx_level1."','rp_looks".$idx_level1."')\" /></td>";
							echo "</tr>";
							echo "<tbody id=\"one_parent".$idx_level1."\" >";
							$idx_level2=1;
							$idx_image++;
							continue;
						}
						else if ($reporter_fuctions_array[$idx]['level']=='2')
						{
							echo "<tr id=\"one_parent".$idx_level1."_son".$idx_level2."\" >";
							echo "<input type=\"hidden\" value=\"0\" name=\"one_parent".$idx_level1."_son".$idx_level2."_count\" id=\"one_parent".$idx_level1."_son".$idx_level2."_count\"/>";
							echo "<td  class=\"alignLeft\" style=\"padding-left:30px;\" >"._gettext($reporter_fuctions_array[$idx]['name'])."</td>";
							
							if ((($idx+1) < count($reporter_fuctions_array) ) && ($reporter_fuctions_array[$idx+1]['level']=='3'))
							{
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_checks".$idx_level1."\" id=\"rp_check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('rp_check".$idx_level1."_".$idx_level2."','three_rp_checks".$idx_level1."_".$idx_level2."','rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_looks".$idx_level1."\" id=\"rp_look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								echo "</tr>";

								echo "<tbody id=\"two_parent".$idx_level1."_son".$idx_level2." >";
								$idx_level3=1;
							}
							else
							{
							    $match=0;
							    if(is_array($roledata["reporter"]) && array_key_exists($reporter_fuctions_array[$idx]['id'], $roledata["reporter"])) {
							        $match = ($roledata["reporter"][$reporter_fuctions_array[$idx]['id']] == 1) ? 1 : 2;
							    }
								
								if ( $match==1)
								{
								    echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_checks".$idx_level1."\"  value=\"".$reporter_fuctions_array[$idx]['id']."\" checked=\"checked\" name=\"r_".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('rp_check".$idx_level1."_".$idx_level2."','three_rp_checks".$idx_level1."_".$idx_level2."','rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								    echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_looks".$idx_level1."\"  value=\"".$reporter_fuctions_array[$idx]['id']."\" checked=\"checked\" name=\"r_read_".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								}
								else if ( $match==2)
								{
								    echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_checks".$idx_level1."\"  value=\"".$reporter_fuctions_array[$idx]['id']."\" name=\"r_".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('rp_check".$idx_level1."_".$idx_level2."','three_rp_checks".$idx_level1."_".$idx_level2."','rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
									echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_looks".$idx_level1."\"  value=\"".$reporter_fuctions_array[$idx]['id']."\" checked=\"checked\" name=\"r_read_".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								}
								else
								{
								    echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_checks".$idx_level1."\"  value=\"".$reporter_fuctions_array[$idx]['id']."\" name=\"r_".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_check".$idx_level1."_".$idx_level2."\" onclick=\"checkBox2('rp_check".$idx_level1."_".$idx_level2."','three_rp_checks".$idx_level1."_".$idx_level2."','rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								    echo "<td style='width:120px;'> <input type=\"checkbox\" class=\"rp_looks".$idx_level1."\"  value=\"".$reporter_fuctions_array[$idx]['id']."\" name=\"r_read_".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_look".$idx_level1."_".$idx_level2."\" onclick=\"checkBox('rp_look".$idx_level1."_".$idx_level2."','three_rp_looks".$idx_level1."_".$idx_level2."')\" /></td>";
								}

								echo "</tr>";
								$commit_idx++;
								$idx_level2++;									
							}
						
							$idx_image++;
							
							continue;							

						}
						else if ($reporter_fuctions_array[$idx]['level']=='3')
						{
							echo "<tr id=\"two_parent".$idx_level1."_son".$idx_level2."_grandson".$idx_level3."\">";
							echo "<td class=\"alignLeft\" style=\"padding-left:55px;\">"._gettext($reporter_fuctions_array[$idx]['name'])."</td>";
							
							$match=0;
							if(is_array($roledata["reporter"]) && array_key_exists($reporter_fuctions_array[$idx]['id'], $roledata["reporter"])) {
							    $match = ($roledata["reporter"][$reporter_fuctions_array[$idx]['id']] == 1) ? 1 : 2;
							}
							
							if ( $match==1)
							{
							    echo "<td style='width:120px;'><input type=\"checkbox\" checked=\"checked\" value=\"".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_check".$idx_level1."_".$idx_level2."_".$idx_level3."[]\" class=\"three_rp_checks".$idx_level1."_".$idx_level2."\" name=\"r_".$reporter_fuctions_array[$idx]['id']."\" 
								onclick=\"clickchceck3('rp_check".$idx_level1."_".$idx_level2."_".$idx_level3."[]', 'rp_look".$idx_level1."_".$idx_level2."_".$idx_level3."[]')\" /></td>";
								echo "<td style='width:120px;'><input type=\"checkbox\" checked=\"checked\" value=\"".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_look".$idx_level1."_".$idx_level2."_".$idx_level3."[]\" class=\"three_rp_looks".$idx_level1."_".$idx_level2."\" name=\"r_read_".$reporter_fuctions_array[$idx]['id']."\" /></td>";
							}
							else if ( $match==2)
							{
							    echo "<td  style='width:120px;'><input type=\"checkbox\" value=\"".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_check".$idx_level1."_".$idx_level2."_".$idx_level3."[]\" class=\"three_rp_checks".$idx_level1."_".$idx_level2."\" name=\"r_".$reporter_fuctions_array[$idx]['id']."\" 
								onclick=\"clickchceck3('rp_check".$idx_level1."_".$idx_level2."_".$idx_level3."[]', 'rp_look".$idx_level1."_".$idx_level2."_".$idx_level3."[]')\" /></td>";
								echo "<td style='width:120px;'><input type=\"checkbox\" value=\"".$reporter_fuctions_array[$idx]['id']."\" checked=\"checked\" id=\"rp_look".$idx_level1."_".$idx_level2."_".$idx_level3."[]\" class=\"three_rp_looks".$idx_level1."_".$idx_level2."\" name=\"r_read_".$reporter_fuctions_array[$idx]['id']."\" /></td>";
							}
							else
							{
							    echo "<td style='width:120px;'><input type=\"checkbox\" value=\"".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_check".$idx_level1."_".$idx_level2."_".$idx_level3."[]\" class=\"three_rp_checks".$idx_level1."_".$idx_level2."\" name=\"r_".$reporter_fuctions_array[$idx]['id']."\" 
								onclick=\"clickchceck3('rp_check".$idx_level1."_".$idx_level2."_".$idx_level3."[]', 'rp_look".$idx_level1."_".$idx_level2."_".$idx_level3."[]')\" /></td>";
								echo "<td style='width:120px;'><input type=\"checkbox\" value=\"".$reporter_fuctions_array[$idx]['id']."\" id=\"rp_look".$idx_level1."_".$idx_level2."_".$idx_level3."[]\" class=\"three_rp_looks".$idx_level1."_".$idx_level2."\" name=\"r_read_".$reporter_fuctions_array[$idx]['id']."\" /></td>";
							}

							echo "</tr>";
							$idx_level3++;							
							$idx_image++;
							$commit_idx++;

							if ((($idx+1) >= count($reporter_fuctions_array) ) || ($reporter_fuctions_array[$idx+1]['level']!='3'))
							{
								$idx_level2++;		
								echo "  </tbody>";
							}
							
							continue;							

						}
						else
						{
							continue;	
						}
					}
				?>

	 		
					
				</td>
				</tr>			
 			</tbody>
		</table>
	</div>
		<div id="errorMsg"></div>
		<div class="middleBottom">
			
		</div>
		<div id="operate">
			<div>
				<input class="btn_ok" type="submit" value="<?=_gettext('OK');?>" />
		      		<a href="user_role.php" class="btn_return"><?=_gettext('Return');?></a>
			</div>
		</div>

	</form>
</div>

</body>
</html>
