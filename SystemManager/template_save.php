<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
include_once($_SERVER["DOCUMENT_ROOT"]."/authenticate.php");
require_once($_SERVER["DOCUMENT_ROOT"]."/model/getServiceFunc.php");
$auth = new authentication();
if(! $auth->isWritable('m_sysmaintance'))
{
	jumpTo("","/login.php");
}

$language = 0;

/**zhang.huanan 2013-10-09 sync service_list for custome service:**/                                                                                                  
$app_name_table = array(); 

$lan = 0;
$oneService = getAllService();
if(count($oneService) > 0) {
	foreach($oneService as $key=>$value) {
		$ser = getServiceById($value["id"]);
		if(count($ser) > 0) {
			foreach($ser as $ser_key=>$ser_value) {
				$tmp = explode ( "9099", $ser_value ["id"] );
				if ("" != $tmp[1])
				{
					$app_name_table['service'][$tmp[1]]['app_name'] = urlencode(trim($ser_value["name"]));
				}
			}
		}
	}
}

$urllib_size = urllib_object_size();                                                                                                                           
$urllib_format = 'Lcategory_id/a32name/a256comment';                                                                                                           
$urlcustom_size = urlcustom_object_size();                                                                                                                     
$urlcustom_format = 'Lcategory_id/a32name/a256comment/a32template_name'; 
/*......URL*/                                                                                  
$urllib_match = 0;                                                                             
for( $idx=0; $idx < $urllib_size; $idx++ )                                                     
{                                                                                              
		$urllib_rule_one = urllib_object_get_by_url_fc($idx,$language);                        
		$urllib_rule_detail = unpack($urllib_format, $urllib_rule_one);                        
		$app_name_table['url'][$urllib_rule_detail['category_id']]['app_name'] = urlencode($urllib_rule_one['name']);
/*
		$aaa_index = $urllib_rule_detail['category_id']>>5;                                    
		$aaa_str = 'aaa'.$aaa_index;                                                           
																							   
		if ( ($record[$aaa_str]&(1<< ($urllib_rule_detail['category_id']&31) ))!= 0)           
		{                                                                                      
				$urllib_match++;                                                               
		} 
*/                                                                                     
}                                                                                              
																						   
/*.........URL*/                                                                               
$urlcustom_match = 0;                                                                          
for( $idx=0; $idx < $urlcustom_size; $idx++ )                                                  
{                                                                                              
		$urlcustom_rule_one = urlcustom_object_get_by_url_fc($idx+1);                          
		$urlcustom_rule_detail = unpack($urlcustom_format, $urlcustom_rule_one);               
		$app_name_table['url'][$urlcustom_rule_detail['category_id']]['app_name'] = urlencode($urlcustom_rule_detail['name']);																				
/*
		$aaa_index = $urlcustom_rule_detail['category_id']>>5;                                 
		$aaa_str = 'aaa'.$aaa_index;                                                           
																							   
		if ( ($record[$aaa_str]&(1<< ($urlcustom_rule_detail['category_id']&31) ))!= 0)        
		{                                                                                      
				$urlcustom_match++;                                                            
		}
*/                                                                                   
}

$filefilter_size = filefilter_object_size();                                                                    
$filefilter_format = 'Lnumber/Lto_number/Lfile_type_id/a32name/a256comment/a4096keywordstring/a32template_name';
$filetype_match = 0;                                                                                            
for( $idx=0; $idx < $filefilter_size; $idx++ )                                                                  
{                                                                                                               
	$filefilter_rule_one = filefilter_object_get_byindex($idx+1);                                           
	$filefilter_rule_detail = unpack($filefilter_format, $filefilter_rule_one);
	$app_name_table['filetype'][$filefilter_rule_detail['file_type_id']]['app_name'] = urlencode($filefilter_rule_detail['name']);	                          
/*
	   $file_type_index = $filefilter_rule_detail['file_type_id']>>5;                                       
	   $aaa_str = 'aaa'.$file_type_index;                                                                      
	   if ( ($record[$aaa_str]&(1<< ($filefilter_rule_detail['file_type_id']&31) ))!= 0)                       
	   {                                                                                                       
			   $filetype_match++;                                                                              
	   }
*/                                                                                                       
}

if (count($app_name_table))
	file_put_contents("/tmp/.app_id2name_template", json_encode($app_name_table));

$ret = nmc_template_save($_SESSION['template']);

if($ret == 0)
	$msg =_gettext("saveConfigureSuccess")."! ";
else
	$msg = _gettext("saveConfigureFail")."($ret)! ";

$log = array();
$log['content'] = array();
$log['content'][0] = _gettext('SaveTemplateConfig');
if ($ret==0)
{
	$log['result'] = _gettext('Success');
}
else
{
	$log['result'] = _gettext('fail');
	$log['ret'] = $msg;
}
saveLog($log);	


echo $msg;
exit();
?>
