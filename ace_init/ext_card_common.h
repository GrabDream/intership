#ifndef _EXT_CARD_COMMON_H
#define _EXT_CARD_COMMON_H

/* chuangshi C3000 NK06 */
#define SMBUS_HSTS_REG_OFFSET 0x00
#define SMBUS_HCTL_REG_OFFSET 0x02
#define SMBUS_HCMD_REG_OFFSET 0x03
#define SMBUS_TSA_REG_OFFSET  0x04
#define SMBUS_HD0_REG_OFFSET  0x05
#define SMBUS_HD1_REG_OFFSET  0x06
#define SMBUS_HBD_REG_OFFSET  0x07

#define SMBUS_HSTS_BDS_BIT 0x80
#define SMBUS_HSTS_IUS_BIT 0x40
#define SMBUS_HSTS_SMSTS_BIT 0x20
#define SMBUS_HSTS_FAIL_BIT 0x10
#define SMBUS_HSTS_BERR_BIT 0x08
#define SMBUS_HSTS_DERR_BIT 0x04
#define SMBUS_HSTS_INTR_BIT 0x02
#define SMBUS_HSTS_HBSY_BIT 0x01

#define NO_SOLT_ONLINE	0
#define SOLT_1_ONLINE	1
#define SOLT_2_ONLINE	2
#define ALL_SOLT_ONLINE	3

typedef struct exp_card_info{
	char name[8];	//扩展卡型号
	uint8_t type;	//型号编码
	int child_type; //子型号
}ExpCardInfo_t;

typedef struct solt_info{
	int solt_state;			//在位状态
	ExpCardInfo_t solt1;	//solt1 信息
	ExpCardInfo_t solt2;	//solt2 信息
}SoltInfo_t;

/*扩展卡型号编码*/
typedef enum exp_card_type{	
	Unknow = 0x00,
	HPC01  = 0x01,
	HPC02  = 0x02,
	HPC03  = 0x03,
	HPC04  = 0x04,
	HPC05  = 0x05,
	HPC06  = 0x06,
	HPC07  = 0x07,
	HPC08  = 0x08,
	HPC09  = 0x09,
	HPC10  = 0x0a,
	HPC11  = 0x0b,
	HPC12  = 0x0c,
	HPC13  = 0x0d,
	HPC22  = 0x16
}ExpCardType_t;

#define UNKNOW_TYPE				0	//未知类型
#define TYPE_4_1G_COPPER		1	//4电
#define TYPE_4_1G_COPPER_BP		2	//4电带BYPASS
#define TYPE_8_1G_COPPER		3	//8电
#define TYPE_8_1G_COPPER_BP		4	//8电带BYPASS
#define TYPE_4_1G_FIBER			5	//4光
#define TYPE_4_1G_FIBER_BP		6	//4光带BYPASS
#define TYPE_8_1G_FIBER			7	//8光
#define TYPE_8_1G_FIBER_BP		8	//8光带BYPASS
#define TYPE_2_10G_FIBER		9	//2万兆光
#define TYPE_2_10G_FIBER_BP		10	//2万兆光
#define TYPE_4_10G_FIBER		11	//4万兆光
#define TYPE_4_10G_FIBER_BP		12	//4万兆光带BYPASS
#define TYPE_2_40G_FIBER		13	//2*40G光
#define TYPE_2_1G_COPPER_2_1G_FIBER			14	//2电2光
#define TYPE_2_1G_COPPER_2_1G_FIBER_BP		15	//2电2光带BYPASS
#define TYPE_4_1G_COPPER_4_1G_FIBER			16	//4电4光
#define TYPE_4_1G_COPPER_4_1G_FIBER_BP		17	//4电4光带BYPASS
#define TYPE_4_1G_COPPER_4_1G_FIBER_COMBO	18	//4电4光combo

void chuangshi_auto_rename_c3000_nk06_solt_info( int* iSlotFlag, int* iCardModel1, int* iCardModel2, int* iCardType1, int* iCardType2 );

#ifndef ARCH_ARM64
void led_control_t5h( char chLed, char chOn );
int x200_get_eth_num_by_cardid(int slot);
#endif

void get_dev_type_interface( char* pDevType, int iMaxLen );

int is_user_define_interface();

int get_product_info( char* pRet, int iMax );

int is_old_bus_format();

int com_is_user_define_interface();

void get_ext_i2c_address( char* pFilter, char* pI2c1, char* pI2c2, int iIndex1, int iIndex2 );
int changshi_ext_get_card_type( char* pI2cAddr );

int get_chuangshi_slot_type( char* pArgument );
void chuangshi_get_slot_info( int iDevType, char* pFilter, int* pSlot1, int* pSlot2, int* piCount );

#if defined(CS_NXP_PLATFORM) || defined(CS_M3720_PLATFORM)
#define NXP_PLATFORM_CS 0
#define NXP_PLATFORM_ZDAK_HB6110 1
#define NXP_PLATFORM_CS_NH15P 2
#define NXP_PLATFORM_CS_NH21 3
#define NXP_PLATFORM_CS_NH12 4
#define NXP_PLATFORM_CS_NH10 5
#define NXP_PLATFORM_CS_NH31 6
#define NXP_PLATFORM_CS_NZ02 7

#define NXP_PLATFORM_CS_NM15 15
#define NXP_PLATFORM_CS_NM32 32
#define NXP_PLATFORM_CS_NM59 59

int nxp_platform_no_get(void);
#endif

#if defined(MT_RK_PLATFORM)
#define RK_PLATFORM_MT 0
#define RK_PLATFORM_MT_3568 3568

int rk_platform_no_get(void);
#endif

int get_mac_address( const char* interface_name, unsigned char* mac );
int set_mac_address( const char* interface_name, const unsigned char* new_mac );

int changshi_get_card_type_by_i2c( int iDevType, char* pI2cFeature, int* iRetSlot );

#endif
