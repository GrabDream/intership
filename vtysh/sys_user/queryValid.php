<?php include_once("/var/www/html/model/charFilter.php"); ?>
<?php
/*  
**数据验证类  
*/  
class checker{  
	// 函数定义  
	var $array_data="";     //要验证的数组数据  
	var $var_key="";     //当前要验证的数据的key  
	var $var_value="";     //当前要验证的数据的值  
	var $is_empty="";     //要验证的值可以为空  
	var $array_info="";     //提示信息收集  
	var $array_errors=array();   //错误信息收集  
	  
	  
	//--------------------->构造函数<------------ 
	function __construct($date)
	{  
		$this->array_data=$date;  
	}  
	  
	//--------------------->数据检验函数<------------- 
	function check($array_datas)
	{  
		foreach($array_datas as $value_key => $value_v)
		{  
			$temp1=explode('|',$value_v);  
			if($temp1[0]=="i_empty" and empty($this->array_data[$value_key]))
			{  
				;  
			}
			else
			{  
				foreach($temp1 as $temp_key => $value_con)
				{  
					
					//$data_temp=$this->array_data;  
					//var_dump($data_temp['birthday']);  
					//echo "--".$value_key."--<br>";  
					$this->var_key=$value_key;  
					$this->var_value=$this->array_data[$value_key];  
					
					$temp2=explode(':',$value_con);  
					
					switch(count($temp2))
					{  
						 case 0:  
						  $this->array_errors[$this->var_key]= "not avaliable!"; //"此值的验证请求不存在";  
						 	break;  
						 case 1:  
						  //如果用户没有指定验证动作  
						  if(empty($temp2[0]))
						  {  
						  	$this->array_errors[$this->var_key]= "not avaliable!"; //"此值的验证请求不存在";  
						   	break;  
						  }
						  else
						  {  
						   	$this->$temp2[0]();   //如果返回值为非，就不用进行下一步验证  
						   	break;  
						  }  
						 case 2:  
						  $this->$temp2[0]($temp2[1]);  
						  break;  
						 case 3:  
						  $this->$temp2[0]($temp2[1],$temp2[2]);  
						  break;  
					}  
				}  
			}  
		}  
	}

    function i_empty()
    {  
    	$this->is_empty=1;  //这个值没什么用，只是说明要验证的值可以是空值  
    }  
    	  
    	  
    //日期数据、邮件地址、浮点数据、整数、IP地址、字符串、最大值、最小值、字符串长度、域名、URL 
    //-------------------->日期验证--------------------  
    function i_date()
    {  
    	//约定格式：2000-02-01或者：2000-5-4  
    	if (!eregi("^[1-9][0-9][0-9][0-9]-[0-9]+-[0-9]+$", $this->var_value)) 
    	{  
    		$this->array_errors[$this->var_key]= "wrong date format";//"日期格式错误";  
    	  return false;  
    	}  
    	$time = strtotime($this->var_value);  
    	if ($time === -1) 
    	{  
    		$this->array_errors[$this->var_key]= "wrong date format"; //"日期格式错误";  
    	  return false;  
    	}  
    	$time_e = explode('-', $this->var_value);  
    	$time_ex = explode('-', Date('Y-m-d', $time));  
    	for ($i = 0; $i < count($time_e); $i++) 
    	{  
    	  if ((int)$time_e[$i] != (int)$time_ex[$i]) 
    	  {  
    			$this->array_errors[$this->var_key]= "wrong date format"; //"日期格式错误";  
    	     return false;  
    	  }  
    	}  
    	return true;  
    }

	//-------------------->时间验证--------------------  
	function i_time() 
	{  
		if (!eregi('^[0-2][0-3](:[0-5][0-9]){2}$', $this->var_value)) 
		{  
		 	$this->array_errors[$this->var_key]= "wrong date format"; //"时间格式错误";  
		  return false;  
		}  
		return true;  
	}  
	//-------------------->email验证--------------------  
	function i_email()
	{  
		if(!eregi("^[0-9a-z~'!#$%&_-]([.]?[0-9a-z~!#$%&_-])*" . "@[0-9a-z~!#$%&_-]([.]?[0-9a-z~!#$%&_-])*$", $this->var_value))  
			$this->array_errors[$this->var_key]= " wrong email format <br>"; //"邮件格式错误<br>";  
		 //echo $this->var_value;  
		return true;  
	}  
	//-------------------->浮点数验证--------------------  
	function i_float()
	{  
		//if(!is_float($this->var_value))  
		if(!ereg("^[1-9][0-9]?\.([0-9])+$",$this->var_value))  
		 	$this->array_errors[$this->var_key]= "  This is not a decimal! "; //"这不是一个小数";  
	}  
	//-------------------->字符串验证--------------------  
	function i_string()
	{  
		if(empty($this->var_value))    //允许为空  
		 return true;  
		if(!is_string($this->var_value))  
		 $this->array_errors[$this->var_key]="This is not a string";  
		return true;  
	}  
	//-------------------->字符串长度验证--------------------  
	function len($minv,$maxv=-1)
	{  
	   $len = strlen($this->var_value);  
	  if($len==0)
	  {  
	   $this->array_errors[$this->var_key]= "can not be null";//"不能为空值";  
	   return false;  
	  }  
	  if ($len < $minv) 
	  {  
	   	$this->array_errors[$this->var_key]="Strings are too short";  
	    return false;  
	  }  
	  if ($maxv != -1)
	  {  
	  	if ($len > $maxv) 
	  	{  
	   	  $this->array_errors[$this->var_key]="Strings are too long";  
	      return false;  
	    }  
		}  
	  return true;  
	}  
	//-------------------->整数验证--------------------  
	function i_int()
	{  
		if(!ereg("^[0-9]*$",$this->var_value))  
		 	$this->array_errors[$this->var_key]= "this is not a integer";//"这不是一个整数";  
	}  
	//-------------------->IP地址验证--------------------  
	function i_ip()
	{  
		if(!ereg("^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$", $this->var_value))
		{  
		 $this->array_errors[$this->var_key]="wrong IP address";  
		}
		else
		{  
			 //每个不大于255  
			 $array_temp=preg_split("/\./",$this->var_value);  
			 foreach($array_temp as $ip_value)
			 {  
				 if((int)$ip_value >255)  
				   $this->array_errors[$this->var_key]= "wrong IP address";//"错误的IP地址";  
			 }  
		}  
		return true;  
	}  
	//-------------------->最大值验证--------------------  
	function i_max($maxv)
	{  
		if($this->var_value >= $maxv)
		{  
		 $this->array_errors[$this->var_key]= " data is too short";//"数据值太大";  
		 return false;  
		}  
		return true;  
	}  
	//-------------------->最小值验证--------------------  
	function i_min($minv)
	{  
		if($this->var_value <= $minv){  
		 $this->array_errors[$this->var_key]= " data is too large";//"数据值太小";  
		 return false;  
		}  
		return true;  
	}  
	//-------------------->域名验证--------------------  
	function i_domain() 
	{  
		if(!eregi("^@([0-9a-z\-_]+\.)+[0-9a-z\-_]+$", $this->var_value))  
		 $this->array_errors[$this->var_key]="invalid domain name";  
		return eregi("^@([0-9a-z\-_]+\.)+[0-9a-z\-_]+$", $this->var_value);  
	}  
	//-------------------->URL验证--------------------  
	function i_url()
	{  
		if(!eregi('^(http://|https://){1}[a-z0-9]+(\.[a-z0-9]+)+$' , $this->var_value))  
		 $this->array_errors[$this->var_key]="invalid URL address";  
		return true;  
	}  
	//-------------------->自定义正则校验--------------------  
	function check_own($user_pattern)
	{  
		//自定义校验。出错返回false，匹配返回1，不匹配返回0  
		$tempvar=preg_match($user_pattern,$this->var_value);  
		if($tempvar!=1)  
		 $this->array_errors[$this->var_key]="invalid data";//数据不合法";  
	}
} 
/*
$rule_list = array(  
 'temp' =>'check_own:"^[0-9a-zA-Z]+$"|len:6:16',  //自定义正则校验
 'time' => 'i_time',  
 'fload' => 'i_float|i_min:1|i_max:10.10|len:0:20',  
 'ipadr' => 'i_ip',  
 'url' =>'i_url',  
    'birthday' => 'i_date',  
    'email' => 'i_email|len:1:128',  
 'gender' => 'i_int|i_min:1|i_max:20',  
    'city' => 'i_string|len:1:32');  
  
$rule_date = array(  
 'temp' => '@nnnnnnh',  
 'time' => '23:59:00',  
 'fload' => '10.0',  
 'ipadr' => '251.255.1.1',  
 'url' => 'Https://www.gg',  
    'birthday' => '2004-5-4',  
 'gender' => '15',  
    'email' => 'tonerzhang@sohu.com',  
    'city' => 'Guangzhou');  
  
$gg=new checker($rule_date);  
$gg->check($rule_list);  
  
print_r($gg->array_errors);  
*/ 

?>
