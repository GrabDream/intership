#ifndef __ACE_INIT_COMMON_H
#define __ACE_INIT_COMMON_H
#include <stddef.h>

#define FIRMWARE_BOARD_TYPE "/usr/private/Firmware_Board_Type"

typedef struct _board_card_info
{
	int slot;                 /* 1 -> slot1 */
	char card_name[32];       /* HPC11 */
	char i2c[32];
	char iface_range[64];     /* "GE1-0 - GE1-7" or "eth5 - eth12" */
	int  online;              /* 1:online   0:offline */
}board_card_info;

enum board_type
{
	board_type_RESV,
	board_type_NT06,
	board_type_NT09,
	board_type_NT03,
	board_type_NM59,
	board_type_NM32,
	board_type_NM15,
	board_type_NH31,
	board_type_NH10,
	board_type_NZ02,
	board_type_NR01,
	board_type_NK02,
	board_type_NK03,
	board_type_NK05,
	board_type_NK06,
	board_type_NR03,
	board_type_NSC2101,
	board_type_RIS5060,
	board_type_RIS5172,
	board_type_Z8620,
	board_type_Z8680,
	board_type_U3100,
	board_type_U3210,
	board_type_NA720,
	board_type_NA860,
	board_type_NA590,
	board_type_GH7131,
	board_type_X300D,
	board_type_T7L,
	board_type_RIS297,
	board_type_ZHGX_N100,
	board_type_ZHGX_M1,
	board_type_ZHGX_AC2000_ULTRA,
	board_type_T5H,
	board_type_T7H,
	board_type_HG3250,
	board_type_XGT_D2000,
	board_type_BXY_HG3X,
	board_type_CS_NL01,
	board_type_CS_NT12,
	board_type_rk3568_NSC2C01,
	board_type_rk3568_NSC1107,
	board_type_trx_E2000Q,
	board_type_YWER_HG5X,
	board_type_YR_F28622,
	board_type_WESTONE_HG3X,
	board_type_COMMON_X86,
	board_type_CS_NH12,
	board_type_CS_NT13,
	board_type_jdsa_E2000Q,
	board_type_jdsa_HG3X,
	board_type_leyan_HYSA7620,
	board_type_VM_X86,
	board_type_yanhao_i54590,
	board_type_leyan_ris5066,
	board_type_antaike_hg332,
	board_type_sanwang_e2000q,
	board_type_MAX
};

typedef int (*board_adaptive)(void);
typedef struct _board_adaptive_fun_set
{
	board_adaptive set_map_port_pre;
	board_adaptive set_map_port;
	board_adaptive set_map_port_post;
	board_adaptive def_hylab_vpp_cmd_conf;
	board_adaptive update_hylab_vpp_cmd_conf;
}board_adaptive_fun_set;

typedef struct _board_interface_config_
{
	int m_iEthCount;
	int m_iEthExist[ACE_MAX_SUPPORT_PORT_BUSYBOX];
	char m_arrEthConfig[ACE_MAX_SUPPORT_PORT_BUSYBOX][32];
} board_interface_config, *lpboard_interface_config;

typedef enum _chuangshi_card_type
{
	type_HPC01_PASS        = 0x101,
	type_HPC01             = 0x102,
	type_HPC02_FIBER       = 0x201,
	type_HPC02_COPPER      = 0x202,
	type_HPC02_COPPER_PASS = 0x203,
	type_HPC03             = 0x301,
	type_HPC04_PASS        = 0x402,
	type_HPC05_PASS        = 0x501,
	type_HPC05             = 0x502,
	type_HPC06             = 0x601,
	type_HPC07_PASS        = 0x701,
	type_HPC07             = 0x702,
	type_HPC08             = 0x801,
	type_HPC09             = 0x901,
	type_HPC10             = 0xa01,
	type_HPC11_FIBER       = 0xb01,
	type_HPC11_COPPER_PASS = 0xb02,
	type_HPC11_COPPER      = 0xb03,
	type_HPC12             = 0xc01,
	type_HPC13_PASS_A      = 0xd01,
	type_HPC13_PASS_B      = 0xd02,
	type_UNKNOW            = 0,
}chuangshi_card_type;


#define CARD_ETH_MAX	    32
#define DEV_CARD_MAX	    slot_type_max
#define LEN_8	            8

typedef enum _slot_type
{
	slot0 = 0,
	onboard1 = 0,
	onboard2 = 1,
	onboard3 = 2,
	onboard4 = 3,
	onboard5 = 4,
	onboard6 = 5,
	onboard7 = 6,
	onboard8 = 7,

	onboard_end = onboard8,
	slot_start = onboard_end,

	slot1 = 8,
	slot2 = 9,
	slot3 = 10,
	slot4 = 11,
	slot5 = 12,
	slot6 = 13,
	slot7 = 14,
	slot8 = 15,

	slot_type_max
}slot_type;

typedef enum _rename_type
{
	TYPE_NONE,
	TYPE_Eth,  /* 100M */
	TYPE_FE,  /*  fast 100M */
	TYPE_GE,	/*	1000M */
	TYPE_XGE,	/*	10G */
	TYPE_TGE, /*	25G */
	TYPE_FGE, /*	40G */
	TYPE_HGE, /*	100G */

	TYPE_HA,
	TYPE_MGT,
}rename_type;

typedef enum _rename_mode
{
	MOD_eth,
	MOD_GE,
	MOD_LANWAN,
}rename_mode;

typedef enum _rename_who
{
	WHO_eth,
	WHO_WAN,
	WHO_LAN,
}rename_who;

struct x_dev_struct;
struct x_eth_item;
struct x_card_struct;

typedef int (*eth_adaptive)(struct x_dev_struct *dev);
typedef int (*card_adaptive)(struct x_eth_item *item, struct x_card_struct *card);
typedef void (*lcp_create)(FILE* fp, struct x_dev_struct *x_dev);

struct x_card_config
{
	const char* feature; /* ../../devices/pci0000:00/0000:00:1c.2/ */
	rename_type type;
	card_adaptive adaptive;
	int slot_index;
	int start_index;
};

struct x_eth_item
{
	char uevent [96]; /* /sys/class/net/eth6/uevent */
	char readlink[128];   /* ../../devices/pci0000:00/0000:00:1c.2/0000:06:00.0/net/eth6 */
	char domain[32];/* 0000:06:00.0 */
	char realname[32];/* d0/enp6/s0/f0 */
	char oldname[32];  /* eth6 */

	char newname[32];  /* GE2-3 */
	rename_type type;  /* GE */
	int slot_index; /* 2 */
	int index;  /* 3 */

	char panel_info[96]; /* "#panel_info/LAN7/board/ETH5" */
	int  lanwan_index; /*LAN7|WAN7  ->  7*/
	const char *lanwan_attr; /*"TRUST", "UNTRUST", "MANAGE"*/
};

struct x_card_struct
{
	struct x_card_config *conf;

	int card_type; /* HPC13 */
	rename_type type; /* TYPE_XGE */

	int num;
	struct x_eth_item eth_item[CARD_ETH_MAX];
};

struct x_dev_struct
{
	rename_mode mode;

	int start_who;   /* 0: eth,  1:LAN, 2: WAN */
	int start_index;

	int num;
	struct x_card_struct card_item[DEV_CARD_MAX];
};

#if 1
#define STARTUP_MEMORY_2G               2
#define STARTUP_MEMORY_4G               4
#define STARTUP_MEMORY_8G               8
#define STARTUP_MEMORY_16G              16
#define STARTUP_MEMORY_32G              32
#define STARTUP_MEMORY_64G              64
#define STARTUP_MEMORY_128G             128

#define STARTUP_RX_DESC_512             512
#define STARTUP_RX_DESC_1024            1024
#define STARTUP_RX_DESC_2048            2048
#define STARTUP_RX_DESC_4096            4096

#define STARTUP_RX_QUEUES_1             1
#define STARTUP_RX_QUEUES_2             2
#define STARTUP_RX_QUEUES_4             4
#define STARTUP_RX_QUEUES_8             8
#define STARTUP_RX_QUEUES_16            16

#define STARTUP_CPU_CORES_2             2
#define STARTUP_CPU_CORES_4             4
#define STARTUP_CPU_CORES_8             8
#define STARTUP_CPU_CORES_16            16
#define STARTUP_CPU_CORES_24            24
#define STARTUP_CPU_CORES_32            32
#define STARTUP_CPU_CORES_64            64

#define STARTUP_DRIVER_IGB_UIO          "igb_uio"
#define STARTUP_DRIVER_VFIO_PCI         "vfio-pci"
#define STARTUP_DRIVER_UIO_PCI_GEENRIC  "uio_pci_generic"
#define STARTUP_DRIVER_AUTO             "auto"

#define STARTUP_STANDARD_NUM 16
#define STARTUP_NUMA_MAX     4
#define MAX_CPUS_NUM 128
#define MAX_VPP_NODE 24

struct x_startup_conf
{
	const char *driver;

	struct {
		int mem;
		int rx_desc;
	}mem_params[STARTUP_STANDARD_NUM];

	struct {
		int cpu;
		int rx_queues;	
	}cpu_params[STARTUP_STANDARD_NUM];

	int interfaces_numa[STARTUP_NUMA_MAX];	
};

struct x_startup_param
{
	char main_heap_size[32];
		
	int  main_core;
	char corelist_workers[64];
	int cpus;

	int buffers_numa[STARTUP_NUMA_MAX];	
	int num_rx_queues;
	int num_rx_desc;
	const char *driver;	
};

extern int handle_startup_conf(struct x_startup_conf *xsc);
#endif

typedef int (*board_switch)(lpboard_interface_config, int);

extern char *my_str_trim (char *s);
extern int get_i2c_buf_addr( char* pFilter );
extern void remove_linux2vpp_for_legacy(void);
extern void generate_default_ip(const char *mgt_name);
extern void generate_inband_mgt_config(void);
extern int generate_bridege_create();
extern void generate_lcp_create(FILE* fp, struct x_dev_struct *x_dev);
extern void generate_hylab_vpp_cmd_conf(lcp_create create);
extern void handle_hylab_vpp_cmd_conf(lcp_create create);
extern int get_chuangshi_slot_card_type( unsigned int iArrSlot[], int iArrSizes, int iSlots );
extern void handle_x_card(struct x_card_config *conf, rename_mode mode, rename_who start_who, int start_index, eth_adaptive adaptive);
extern int set_map_port_pre(void);
extern int set_map_port(void);
extern int set_map_port_post(void);
extern int def_hylab_vpp_cmd_conf(void);
extern int update_hylab_vpp_cmd_conf(void);
extern int com_update_hylab_vpp_cmd_conf( board_switch switchFunc, int iDevType );
extern int init_boardtype( void );
extern int get_boardtype( void );
extern const char* get_boardstr( void );
extern int check_uevent_content( char* pFile, char* pDstContent );
extern int com_get_eth_slot_domain( char* pEth , char* pOut, int iOut);
extern int com_get_eth_slot_dpdk_value2(struct x_eth_item *item, char* pOut, int iOut);
extern int com_get_inter_info_from_map( lpboard_interface_config lpEths, int iDevType );
extern int com_get_inter_info_from_switch( lpboard_interface_config lpEths, int iDevType );
extern int com_get_physical_port_num( int iDevType );
extern int com_def_hylab_vpp_cmd_conf_ac( int iDevType );
extern void com_deal_port_map_config( unsigned int eth_total, int iDevType );
extern void com_check_vpp_interface( int iDevType );
extern int com_is_user_define_interface( int iDevType );
extern void write_slot_card_conf(board_card_info info[], int num);
extern int com_get_work_mode();
extern char* com_get_orig_port(char *devname);
extern char* com_get_dev_mac(char *devname);
extern void get_mac_from_kernel(char *ifname, char *mac, int len);
extern void com_generate_linux2vpp(void);
#endif

