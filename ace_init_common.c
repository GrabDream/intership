#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <errno.h>
#include <syslog.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>

#include "../../ace_include/ace_common_macro.h"
#include "ace_port_map.h"
#include "ace_init_common.h"
#include "ace_init_ruijie_z8680.h"
#include "ace_init_ruijie_u3100.h"
#include "ace_init_ruijie_u3210.h"
#include "ace_init_ruijie_x300d.h"
#include "ace_init_topsec_t7l.h"
#include "ace_init_chuangshi_nr03.h"
#include "ace_init_leyan_e2000q.h"
#include "ace_init_chuangshi_nm59.h"
#include "ace_init_chuangshi_nm32.h"
#include "ace_init_chuangshi_nm15.h"
#include "ace_init_chuangshi_nz02.h"
#include "ace_init_chuangshi_nr01.h"
#include "ace_init_chuangshi_nl01.h"
#include "ace_init_leyan_ft2000.h"
#include "ace_init_zhgx_n100.h"
#include "ace_init_topsec_t5h.h"
#include "ace_init_topsec_t7h.h"
#include "ace_init_topsec_hg3250.h"
#include "ace_init_chuangshi_nk03.h"
#include "ace_init_chuangshi_nt09.h"
#include "ace_init_guanghe_7131.h"
#include "ace_init_xgt_d2000.h"
#include "ace_init_beixinyuan_hg3x.h"
#include "ace_init_chuangshi_nh31.h"
#include "ace_init_chuangshi_nh10.h"
#include "ace_init_chuangshi_nt12.h"
#include "ace_init_rk3568_nsc2c01.h"
#include "ace_init_chuangshi_nk02.h"
#include "ace_init_trx_e2000q.h"
#include "ace_init_zhgx_m1.h"
#include "ace_init_ywer_hg5x.h"
#include "ace_init_yanrui_f28622.h"
#include "ace_init_westone_hzc8311.h"
#include "ace_init_common_x86.h"
#include "ace_init_chuangshi_nk06.h"
#include "ace_init_zhgx_ac2000_ultra.h"
#include "ace_init_chuangshi_nt03.h"
#include "ace_init_rk3568_nsc1107.h"
#include "ace_init_chuangshi_nh12.h"
#include "ace_init_chuangshi_nt13.h"
#include "ace_init_jdsa_e2000q.h"
#include "ace_init_jdsa_hg3x.h"
#include "ace_init_leyan_hysa7620.h"
#include "ace_init_vmx86.h"
#include "ace_init_yanhao_i54590.h"
#include "ace_init_leyan_ris5066.h"
#include "ace_init_antaike_hg332.h"
#include "ace_init_sanwang_e2000q.h"

extern int ace_get_port_num( void );
extern int vpclose(FILE *fp);
extern FILE *vpopen(const char* cmdstring, const char *type);
extern char *get_default_ip(void);

typedef struct _value_type {
    int      value;
    const char *strptr;
} value_type;

unsigned int com_serial_ext_card_number = 0;
struct ruijie_ext_card_struct com_serial_ext_card_entry[MAX_CARD_ONE_DEV];
unsigned int com_serial_ext_card_sorted[MAX_CARD_ONE_DEV];
const char* com_rename_attribute_name[] = 
{
	"TRUST",
	"UNTRUST",
	"MANAGE",
};

#define BYPASS_SWITCH_CONFIG "/home/config/cfg-scripts/bypass_switch_config"

static const value_type g_allBoardType[] = {
	{ board_type_NT06, "NT06"},
	{ board_type_NT09, "NT09"},
	{ board_type_NT03, "NT03"},
	{ board_type_NM59, "NM59"},
	{ board_type_NM32, "NM32"},
	{ board_type_NM15, "NM15"},
	{ board_type_NH31, "NH31"},
	{ board_type_NH10, "NH10"},
	{ board_type_NZ02, "NZ02"},
	{ board_type_NZ02, "NZ10"},
	{ board_type_NR01, "NR01"},
	{ board_type_NK02, "NK02"},
	{ board_type_NK03, "NK03"},
	{ board_type_NK05, "NK05"},
	{ board_type_NK06, "NK06"},
	{ board_type_NR03, "NR03"},
	{ board_type_NSC2101, "NSC2101"},
	{ board_type_RIS5060, "RIS5060"},
	{ board_type_RIS5172, "RIS5172"},
	{ board_type_Z8620, "Z8620"},
	{ board_type_Z8680, "Z8680"},
	{ board_type_U3100, "U3100"},
	{ board_type_U3210, "U3210"},
	{ board_type_NA720, "NA720"},
	{ board_type_NA860, "NA860"},
	{ board_type_NA590, "NA590"},
	{ board_type_GH7131, "GUANGHE-7131"},
	{ board_type_X300D, "X300D"},
	{ board_type_T7L, "T7L"},
	{ board_type_RIS297, "RIS297"},
	{ board_type_ZHGX_N100, "ZHGX-N100"},
	{ board_type_ZHGX_M1, "ZHGX-M1"},
	{ board_type_ZHGX_AC2000_ULTRA, "ZHGX-AC2000-ULTRA"},
	{ board_type_T5H, "T5H"},
	{ board_type_T7H, "T7H"},
	{ board_type_HG3250, "TNS-HG-2U-GF2C4E"},
	{ board_type_XGT_D2000, "XGT-D2000"},
	{ board_type_BXY_HG3X, "BXY-HG3X"},
	{ board_type_CS_NL01, "NL01"},
	{ board_type_CS_NT12, "NT12"},
	{ board_type_rk3568_NSC2C01, "NSC2C01"},
	{ board_type_rk3568_NSC1107, "NSC1107"},
	{ board_type_trx_E2000Q, "TRX-E2000Q"},
	{ board_type_YWER_HG5X, "YWER-HG5X"},
	{ board_type_YR_F28622, "F28622"},
	{ board_type_WESTONE_HG3X, "WESTONE-HG3X"},
	{ board_type_COMMON_X86, "COMMON-X86:COMMON-X86"},
	{ board_type_CS_NH12, "CS-NH12"},
	{ board_type_CS_NT13, "NT13" },
	{ board_type_jdsa_E2000Q, "JDSA-E2000Q"},
	{ board_type_jdsa_HG3X, "JDSA-HG3X:COMMON-X86"},
	{ board_type_leyan_HYSA7620, "HYSA-7620:LEYAN-KUNPENG"},
	{ board_type_VM_X86, "VM-X86:VM-X86"},
	{ board_type_yanhao_i54590, "YH-I54590:COMMON-X86"},
	{ board_type_leyan_ris5066, "LEYAN-RIS5066:LEYAN-E2000Q"},
	{ board_type_antaike_hg332, "ANTAIKE-HG332:ANTAIKE-X86"},
	{ board_type_sanwang_e2000q, "BAIXIN-F619:BAIXIN-E2000Q"},

	{ 0, NULL }
};

static value_type g_cur_board_type = {board_type_RESV, "unknown"};
static board_adaptive_fun_set g_adaptive_fun_set[board_type_MAX] = {NULL};

static const char *g_lanwan_attr[3] = { "TRUST", "UNTRUST", "MANAGE"};

struct x_dev_struct g_x_dev;

static char *__str_rtrim (char *s)
{
    register char *p;

    p = s + strlen (s);
    if (p > s)
    {
        do { --p; } while (isspace (*p));
        p[1] = '\0';
    }
    return s;
}

static char *__str_ltrim (char *s)
{
    register char *p, *q;
    p = q = s;
    while (isspace (*q))
        ++q;
    while (*q)
        *p++ = *q++;
    *p = '\0';
    return s;
}

char *my_str_trim (char *s)
{
    char*p = __str_ltrim(s);
	p = __str_rtrim(p);

    return p;
}

static int read_ace_port_map(void)
{
    FILE* fp               = NULL;
    char  cmdBuf[128]      = { 0 };
    char  line[256]        = { 0 };
    U32   portIndex        = 0;
    U32   autoCfgMode      = 0;
    char  mac[32]          = { 0 };
    U32   lanIndex         = 0;
    U32   wanIndex         = 0;
    char  oldname[32]      = { 0 };
    char  newname[32]      = { 0 };
    char  property[32]     = { 0 };
    char  noIgnore[32]     = { 0 };
    int   ifindex          = 0;
    U32   portNum          = 0;
    U32   trustPortNum     = 0;
    U32   ignoreNum        = 0;
    U32   readPortNum      = 0;
    U32   nmPortNum        = 0;
	unsigned int man_make  = 0;
	char  tmpstr[256]      = { 0 };
	char  band[32]     = { 0 };
	
    memset( g_port_info, 0, sizeof( g_port_info ) );

    fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if(fp){
#ifdef CONFIG_HYTF_FW
		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
			if ( ( '#' == line[0] ) || ( '\n' == line[0] ) )
			{
				continue;
			}
#if CONFIG_HYTF_FW_NEW==1
			if ( sscanf( line, "%s %s %s %s", oldname, newname, noIgnore, band ) != 4 )
#else
			if ( sscanf( line, "%s %s %s", oldname, newname, noIgnore ) != 3 )
#endif
			{
				continue;
			}
	
			if ( memcmp( oldname, "eth", strlen( "eth" ) ) 
				&& memcmp( oldname, "switch", strlen( "switch" ) )
#if defined(MT_RK_PLATFORM)
				 && memcmp( oldname, "ETH", strlen( "ETH" ) )
#endif
				)
			{
				continue;
			}
	
			if ( ( memcmp( noIgnore, "no-ignore", strlen( "no-ignore" ) ) ) )
			{
				ignoreNum++;
				continue;
			}
	
			strcpy( g_port_info[portNum].oldPhyname, oldname );
			strcpy( g_port_info[portNum].newPhyname, newname );
#if CONFIG_HYTF_FW_NEW==1
			if ( 'o' == band[0] )
#else
			if ( 'M' == newname[0] )
#endif
			{
				g_port_info[portNum].isTrust = 2;
				nmPortNum++;
			}
	
			portNum++;
			if ( portNum >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
			{
				break;
			}
		}
#else
    	while ( 0 != fgets( line, sizeof( line ), fp ) )
	    {
	        if ( ( '#' == line[0] ) || ( '\n' == line[0] ) )
	        {
			    init_coordinate_log(" comment line");
	            continue;
	        }

	        memset(mac, 0, sizeof(mac));
	        memset(newname, 0, sizeof(newname));
	        memset(property, 0, sizeof(property));
	        memset(noIgnore, 0, sizeof(noIgnore));

	        if ( sscanf( line, "%s %s %d %s %s", oldname, newname, &ifindex, property, noIgnore ) != 5 )
	        {
			    init_coordinate_log(" invalid line %s",line);
	            continue;
	        }
	        
	        if ( ( memcmp( oldname, "eth", strlen( "eth" ) ) && memcmp( oldname, "switch", strlen( "switch" ) ) ) )
	        {
	            continue;
	        }

	        if ( strcmp( property, "TRUST" ) && strcmp( property, "UNTRUST" ) && strcmp( property, "MANAGE" ) )
	        {
	            continue;
	        }
	        
	        if ( ( memcmp( noIgnore, "no-ignore", strlen( "no-ignore" ) ) ) )
	        {
	            ignoreNum++;
	            continue;
	        }

	        if ( !strncmp( property, "TRUST", strlen( "TRUST" ) ) )
	        {
	            trustPortNum++;
	        }

	        strcpy( g_port_info[portNum].oldPhyname, oldname );
	        strcpy( g_port_info[portNum].newPhyname, newname );
	        g_port_info[portNum].ifIndex = ifindex;
	        g_port_info[portNum].isTrust = 0;
	        if ( !strcmp( property, "TRUST" ) )
	        {
	            g_port_info[portNum].isTrust = 1;
	        }

	        if ( !strcmp( property, "MANAGE" ) )
	        {
	            g_port_info[portNum].isTrust = 2;
	            nmPortNum++;
	        }

	        portNum++;
	        if ( portNum >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
	        {
	            break;
	        }
	    }
#endif
        fclose( fp );
        fp = NULL;
    }

    g_portNum = portNum;
	g_iMgtNum = nmPortNum;

	init_coordinate_log("g_portNum = %d, g_iMgtNum = %d", g_portNum, g_iMgtNum);

}

int get_i2c_buf_addr( char* pFilter )
{
	int iRet = -1;
	char szCmd[256] = {0};
	char szLine[128] = {0};

	if ( pFilter )
	{
		snprintf( szCmd, sizeof( szCmd ), "/usr/sbin/i2cdetect -l | grep '%s' | awk '{print$1}' | cut -d '-' -f 2", pFilter );

		FILE* fp = popen( szCmd, "r" );

		if ( fp )
		{
			szLine[0] = '0'; szLine[1] = 'x';

			if ( 0 != fgets( szLine + 2, sizeof( szLine ) - 3, fp ) )
			{
				if ( ( szLine[3] >= 'a' && szLine[3] <= 'f' ) || ( szLine[3] >= 'A' && szLine[3] <= 'F' ) || ( szLine[3] >= '0' && szLine[3] <= '9' ) )
				{
					szLine[4] = 0;
				}
				else
				{
					szLine[3] = 0;
				}

				iRet = strtoul( szLine, NULL, 16 );
			}

			pclose( fp );
		}
	}

	return iRet;
}

int check_uevent_content( char* pFile, char* pDstContent )
{
	int iRet = 0;
	FILE* fp = NULL;
	char szLine[256] = {0};

	if ( ( fp = fopen( pFile, "r" ) ) )
	{
		while( 0 != fgets( szLine, sizeof( szLine ) - 1, fp ) )
		{
			if ( strstr( szLine, pDstContent ) )
			{
				iRet = 1;

				break;
			}
		}

		fclose( fp );
	}

	return iRet;
}

void remove_linux2vpp_for_legacy(void)
{
	if(0 == system("cat /home/ace_port_map.dat 2>/dev/null  |grep -q  \"ManMakeExtMap 0\"")){
#ifdef CS_D2000_PLATFORM
			;
#else
		if(get_boardtype() != board_type_NT09 && get_boardtype() != board_type_NK06)
		remove("/home/linux2vpp.dat");
#endif
	}
}

static void rename_mgt_ha(void)
{
	FILE* fp = fopen( ACE_PORT_MAP_FILE, "r" );
	char cmd[128] = {0};
    char  line[256]        = { 0 };
    char  oldname[32]      = { 0 };
    char  newname[32]      = { 0 };
    char  property[32]     = { 0 };
    char  noIgnore[32]     = { 0 };
    int   ifindex          = 0;

    if (fp)
    {
		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
			if ( ( '#' == line[0] ) || ( '\n' == line[0] ) )
			{
				init_coordinate_log(" comment line");
				continue;
			}

			memset(oldname, 0, sizeof(oldname));
			memset(newname, 0, sizeof(newname));
			memset(property, 0, sizeof(property));
			memset(noIgnore, 0, sizeof(noIgnore));

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
			if ( sscanf( line, "%s %s %s %s", oldname, newname, noIgnore, property) != 4 )
			{
				//init_coordinate_log(" invalid line %s",line);
				continue;
			}

	        if (0 == memcmp( property, "inband", strlen( "inband" ) ) )
	        {
	            continue;
	        }
#else
			if ( sscanf( line, "%s %s %s", oldname, newname, noIgnore ) != 3 )
			{
				//init_coordinate_log(" invalid line %s",line);
				continue;
			}

	        if (0 == memcmp( noIgnore, "no-ignore", strlen( "no-ignore" ) ) )
	        {
	            continue;
	        }
#endif
#else
			if ( sscanf( line, "%s %s %d %s %s", oldname, newname, &ifindex, property, noIgnore ) != 5 )
			{
				//init_coordinate_log(" invalid line %s",line);
				continue;
			}

	        if (0 != memcmp( property, "MANAGE", strlen( "MANAGE" ) ) )
	        {
	            continue;
	        }
#endif
			snprintf(cmd, sizeof(cmd), "ifconfig %s down; ip link set %s name %s; ifconfig %s up; ",
				oldname, oldname, newname, newname);
			system(cmd);
			init_coordinate_log("%s", cmd);
		}
	}

	return;
}

int generate_bridege_create()
{
	FILE* fp = NULL;
	const char *filename = HYLAB_VPP_CMD_FILE;

	const char *def1 = "comment {Bridge1 configure start}\n";
	const char *def2 = "comment {Bridge1 configure end}\n\n";

	fp = fopen( filename, "a" );

	if ( NULL == fp )
	{
		return 0;
	}

	fwrite( def1, strlen( def1 ), sizeof( char ), fp );

	if ( !access( "/usr/mysys/xconnect_bridge", F_OK ) )
	{
		fprintf( fp, "set interface l2 xconnect LAN1 WAN1\n" );
		fprintf( fp, "set interface l2 xconnect WAN1 LAN1\n" );
	}
	else
	{
		fprintf( fp, "set interface l2 bridge LAN1 1\n" );
		fprintf( fp, "set interface l2 bridge WAN1 1\n" );
#ifndef CONFIG_HYTF_FW
		fprintf( fp, "set interface l2 pair LAN1 WAN1\n" );
#endif
	}

	fprintf( fp, "create loopback interface mac %s instance 1\n", com_get_dev_mac( com_get_orig_port( "LAN1" ) ) );
	fprintf( fp, "set interface l2 bridge loop1 1 bvi\n" );
	fprintf( fp, "set interface state loop1 up\n" );
	fprintf( fp, "lcp create loop1 host-if Bridge1\n" );

	fwrite( def2, strlen( def2 ), sizeof( char ), fp );

	fclose( fp );

	return 1;
}

void generate_lcp_create(FILE* fp, struct x_dev_struct *x_dev)
{
	int i, j;
	char line[256] = {0};
	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		struct x_card_struct *item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;

		for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
			//init_coordinate_log("1");
			if(( item->eth_item[j].type != TYPE_NONE)
				&&( item->eth_item[j].type != TYPE_HA)
				&&( item->eth_item[j].type != TYPE_MGT)
				&&(!strstr(item->eth_item[j].newname, "switch"))){
				//init_coordinate_log("2");
				memset(line, 0, sizeof(line));
				snprintf(line, sizeof(line), "lcp create %s host-if %s\n",
					item->eth_item[j].newname, item->eth_item[j].newname);
				fwrite( line, strlen( line ), sizeof( char ), fp ); 
			}
		}
	}

	return;
}

void generate_inband_mgt_config(void)
{
	FILE *fp_n = fopen( "/home/config/current/inband_mgt_config", "w+");
	if (fp_n)
	{
#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
		fprintf( fp_n, "GE0-0" );
#else
		fprintf( fp_n, "eth0" );
#endif
#endif

		fflush( fp_n );
		fclose( fp_n );
	}
}

void generate_default_ip(const char *mgt_name)
{
	const char *file = ACE_SYSTEM_CONFIG_FILE;
	char buf[128];
	char cmd[256];

	char *default_ip = get_default_ip();

	if ( mgt_name )
	{
		if ( default_ip )
		{
			snprintf( buf, sizeof(buf), "ip addr add %s dev %s", default_ip, mgt_name );
		}
		else
		{
			snprintf( buf, sizeof(buf), "ip addr add %s/%s dev %s", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK, mgt_name );
		}
	}
	else
	{
#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
		if ( default_ip )
		{
			snprintf(buf, sizeof(buf), "ip addr add %s dev GE0-0", default_ip );
		}
		else
		{
			snprintf(buf, sizeof(buf), "ip addr add %s/%s dev GE0-0", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK );
		}
#else
		if ( default_ip )
		{
			snprintf(buf, sizeof(buf), "ip addr add %s dev eth0", default_ip );
		}
		else
		{
			snprintf(buf, sizeof(buf), "ip addr add %s/%s dev eth0", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK );
		}
#endif
#else
		if ( default_ip )
		{
			snprintf(buf, sizeof(buf), "ip addr add %s dev LAN1", default_ip );
		}
		else
		{
			snprintf(buf, sizeof(buf), "ip addr add %s/%s dev LAN1", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK );
		}
#endif
	}

	if ( access( file, F_OK ) ){
		snprintf(cmd, sizeof(cmd), "echo \"%s\" > %s", buf, file);
	}
	else{
		snprintf(cmd, sizeof(cmd), "echo \"%s\" >> %s", buf, file);
	}

	system(cmd);
}

void generate_hylab_vpp_cmd_conf(lcp_create create)
{
	FILE* fp;
	struct x_dev_struct *x_dev = &g_x_dev;
	const char *filename = HYLAB_VPP_CMD_FILE;

	const char *def1 = "comment {hylab pre-exec configure start}\n"
					   "create host-interface name v2k4cp\n"
					   "set interface mac address host-v2k4cp 00:01:02:03:04:06\n"
					   "set interface state host-v2k4cp up\n"
					   "lcp lcp-sync on\n"
					   "lcp lcp-auto-subint on\n"
					   "comment {hylab pre-exec configure end}\n"
					   "comment {hylab interface configure start}\n";
	const char *def2 = "comment {hylab interface configure end}\n";

	fp = fopen( filename, "w+" );
	if(NULL == fp){
		return;
	}

	fwrite( def1, strlen( def1 ), sizeof( char ), fp );

	if(create)
		create(fp, x_dev);

	fwrite( def2, strlen( def2 ), sizeof( char ), fp );
	fclose(fp);
}

void handle_hylab_vpp_cmd_conf(lcp_create create)
{
	FILE* fp;
	FILE* fp_new;
	struct x_dev_struct *x_dev = &g_x_dev;
	char line[1024] = {0};
	const char *filename = HYLAB_VPP_CMD_FILE;
	const char *filename_new = HYLAB_VPP_CMD_FILE".tmp";
	int t1 = 0;
	int t2 = 0;
	const char *token1 = NULL;/*comment {hylab interface configure start}*/
	const char *token2 = NULL;/*comment {hylab interface configure end}*/
	const char *def1 = "comment {hylab interface configure start}\n";
	const char *def2 = "comment {hylab interface configure end}\n";

	if(0 == system("cat /home/ace_port_map.dat 2>/dev/null  |grep -q  \"ifAutoCfgMode 0\"")){
		return;
	}

	if(0 != access(filename, F_OK)){
		return;
	}

	fp = fopen( filename, "r" );
	if(NULL == fp){
		return;
	}

	fp_new = fopen( filename_new, "w+" );
	if(NULL == fp_new){
		fclose(fp);
		return;
	}

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *start;
		if(token1 == NULL){
			token1 = strstr(line, "comment {hylab interface configure start}");
			if(token1){
				t1 = 1;
			}
		}

		if(token2 == NULL){
			token2 = strstr(line, "comment {hylab interface configure end}");
			if(token2){
				t2 = 1;
			}
		}

		if(t1){
			fwrite( def1, strlen( def1 ), sizeof( char ), fp_new );
			if(create)
				create(fp_new, x_dev);
			memset(line, 0, sizeof(line));
			t1 = 0;
			continue;
		}

		if(t2){
			fwrite( def2, strlen( def2 ), sizeof( char ), fp_new );
			memset(line, 0, sizeof(line));
			t2 = 0;
			continue;
		}

		if(!((token1 == NULL) || (token2 != NULL))){
			if(strstr(line, "lcp create")
			&& strstr(line, "host-if")
			&& (!strstr(line, "Br"))
			&& (!strstr(line, "Agg"))
			&& (!strstr(line, "."))
			&& (strstr(line, "AN") || strstr(line, "E") || strstr(line, "eth"))){
				memset(line, 0, sizeof(line));
				continue;
			}
		}

		fwrite( line, strlen( line ), sizeof( char ), fp_new );
		memset(line, 0, sizeof(line));
	}

	fclose(fp);
	fclose(fp_new);

	rename(filename_new, filename);
}

static void parse_x_eth_item(char *line, struct x_card_struct *item)
{
#define ETH_UEVENT "/sys/class/net/%s/device/uevent"

	char* token = strrchr( line, '/' );

	if (item==NULL || item->num >= CARD_ETH_MAX ){
		init_coordinate_log( "%s parse(%s) overflow!\n", __FUNCTION__, line );
		return;
	}

	if(token){
		strncpy(item->eth_item[item->num].oldname, token+1, sizeof(item->eth_item[item->num].oldname));
		my_str_trim(item->eth_item[item->num].oldname);

		snprintf(item->eth_item[item->num].uevent, sizeof(item->eth_item[item->num].uevent), 
			ETH_UEVENT, item->eth_item[item->num].oldname);
	}

	strncpy(item->eth_item[item->num].readlink, line, sizeof(item->eth_item[item->num].readlink));		
	my_str_trim(item->eth_item[item->num].readlink);

	com_get_eth_slot_dpdk_value2(&(item->eth_item[item->num]), item->eth_item[item->num].realname, sizeof( item->eth_item[item->num].realname ));
	com_get_eth_slot_domain( item->eth_item[item->num].oldname, item->eth_item[item->num].domain, sizeof( item->eth_item[item->num].domain));
	my_str_trim(item->eth_item[item->num].domain);

	item->eth_item[item->num].index = item->conf->start_index + item->num;
	if(item->conf->slot_index >= onboard1
		&& item->conf->slot_index <= onboard_end){
		item->eth_item[item->num].slot_index = slot0;
	}
	else if(item->conf->slot_index > slot_start
		&& item->conf->slot_index < slot_type_max){
		item->eth_item[item->num].slot_index = (item->conf->slot_index - slot_start);
	}

	if(item->type == TYPE_NONE){
		item->type = item->conf->type;

		if(item->conf->adaptive){
			item->conf->adaptive(&(item->eth_item[item->num]), item);
		}
	}

	item->eth_item[item->num].type = item->type;

	init_coordinate_log( "uevent [%s],\n oldname [%s],\n readlink [%s],\n realname [%s],\n "	
			"type [%d],\n, slot_index [%d],\n, index [%d]\n", 
		item->eth_item[item->num].uevent,
		item->eth_item[item->num].oldname,
		item->eth_item[item->num].readlink,
		item->eth_item[item->num].realname,
		item->eth_item[item->num].type,
		item->eth_item[item->num].slot_index,
		item->eth_item[item->num].index);

	item->num++;
}

static void parse_x_card(struct x_card_config *conf)
{
	FILE* fp						  = NULL;
	char  line[256] 				  = { 0 };
	int i;
	struct x_dev_struct *x_dev = &g_x_dev;
	struct x_card_config *c = conf;
	struct x_card_struct *item;

	fp = popen( "ls -al /sys/class/net/ | grep eth | awk -F\\-\\> {'print $2'} | sort", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );
		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
			for (c = conf; c->feature!=NULL; c++)
			{
				if ( !strncmp( line, c->feature, strlen(c->feature)))
				{
					item = NULL;

					if ( x_dev->num >= DEV_CARD_MAX ){
						goto error_out;
					}

					for (i=0; i<DEV_CARD_MAX; i++){
						if (c == x_dev->card_item[i].conf){
							item = &(x_dev->card_item[i]);
						}
					}

					if(item == NULL && (c->slot_index>=0 && c->slot_index<DEV_CARD_MAX)){
						x_dev->card_item[c->slot_index].conf = c;
						item = &(x_dev->card_item[c->slot_index]);
						x_dev->num++;
					}

					if(item){
						parse_x_eth_item(line, item);
					}

					break;
				}
			}
		}

		pclose( fp ); fp = NULL;
		return;
	}

error_out:
	if ( fp ){
		pclose( fp );
	}

	init_coordinate_log( "%s overflow!\n", __FUNCTION__);
	return ;
}

static void generate_linux2vpp(void)
{
	struct x_dev_struct *x_dev = &g_x_dev;
	FILE*   fp                          = NULL;
	char    tmpstr[256]                 = { 0 };
	int i, j;
	const char *head = "ifAutoCfgMode 0\n";

	unlink(ACE_VPP_PORT_MAP_FILE);
	fp = fopen( ACE_VPP_PORT_MAP_FILE, "w+" );
	if ( fp )
	{
		fwrite( head, strlen( head ), sizeof( char ), fp );

		for ( i = 0; i<DEV_CARD_MAX; i++ ){
			struct x_card_struct *item = &(x_dev->card_item[i]);
			if(item->conf==NULL)
				continue;

			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				memset(tmpstr, 0, sizeof(tmpstr));

				if ( ( item->eth_item[j].type == TYPE_HA ) 
					|| ( item->eth_item[j].type == TYPE_MGT )){
					continue;
				}

				if(x_dev->mode == MOD_GE){
						snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore inband\n", item->eth_item[j].realname, item->eth_item[j].newname );
				}
				else if(x_dev->mode == MOD_LANWAN){
					snprintf( tmpstr, sizeof(tmpstr), "%s %s %d %s no-ignore\n", item->eth_item[j].realname, item->eth_item[j].newname,
						item->eth_item[j].lanwan_index, item->eth_item[j].lanwan_attr);
				}
				else{
					snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore\n", item->eth_item[j].realname, item->eth_item[j].newname );
				}

				if(tmpstr[0]){
					fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
				}
			}
		}

		fclose( fp ); fp = NULL;
	}
}

static void generate_port_map(eth_adaptive adaptive)
{
	struct x_dev_struct *x_dev = &g_x_dev;
	FILE*   fp                          = NULL;
	char    tmpstr[256]                 = { 0 };
	int i, j;

	if(x_dev->mode == MOD_GE){

		init_coordinate_log( "%s \n", __FUNCTION__);
		for ( i = 0; i<DEV_CARD_MAX; i++ ){
			struct x_card_struct *item = &(x_dev->card_item[i]);
			if(item->conf==NULL)
				continue;

			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				char *prefix = "GE";
				if ( ( item->eth_item[j].type == TYPE_Eth )){
					prefix = "Eth";
				}
				else if ( ( item->eth_item[j].type == TYPE_FE )){
					prefix = "FE";
				}
				else if ( ( item->eth_item[j].type == TYPE_GE )){
					prefix = "GE";
				}
				else if ( ( item->eth_item[j].type == TYPE_XGE )){
					prefix = "XGE";
				}
				else if ( ( item->eth_item[j].type == TYPE_TGE )){
					prefix = "TGE";
				}
				else if ( ( item->eth_item[j].type == TYPE_FGE )){
					prefix = "FGE";
				}
				else if ( ( item->eth_item[j].type == TYPE_HGE )){
					prefix = "HGE";
				}
				else if ( ( item->eth_item[j].type == TYPE_HA)){
					if(item->eth_item[j].newname[0] == 0)
						snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							"HA");
				}
				else if ( ( item->eth_item[j].type == TYPE_MGT)){
					if(item->eth_item[j].newname[0] == 0)
						if(item->conf->start_index<=0){
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"MGT");
						}
						else{
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"MGT%d", item->conf->start_index);
						}
				}

				if(item->eth_item[j].newname[0] == 0)
					snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
						"%s%d-%d", prefix, item->eth_item[j].slot_index, item->eth_item[j].index);
			}
		}
	}else if(x_dev->mode == MOD_LANWAN){
		const char *prefix[2] = { "LAN", "WAN"};
		int start_who = x_dev->start_who;
		int start_index = x_dev->start_index;
		for (i = 0; i<DEV_CARD_MAX; i++ ){
			struct x_card_struct *item = &(x_dev->card_item[i]);
			if(item->conf==NULL)
				continue;

			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				if ( ( item->eth_item[j].type == TYPE_HA)){
					if(item->eth_item[j].newname[0] == 0){
						snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							"HA");
						item->eth_item[j].lanwan_attr = g_lanwan_attr[2];
						item->eth_item[j].lanwan_index = 1;
					}
				}
				else if ( ( item->eth_item[j].type == TYPE_MGT)){
					if(item->eth_item[j].newname[0] == 0){
						if(item->conf->start_index<=0){
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"MGT");
						}
						else{
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"MGT%d", item->conf->start_index);
						}

						item->eth_item[j].lanwan_attr = g_lanwan_attr[2];
						item->eth_item[j].lanwan_index = 1;
					}
				}
				else{
					if(item->conf->start_index != -1){
						if(item->eth_item[j].newname[0] == 0){
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"%s%d", prefix[start_who%2], start_index);
							item->eth_item[j].lanwan_attr = g_lanwan_attr[start_who%2];
							item->eth_item[j].lanwan_index = start_index;
						}

						if((start_who%2) == 1)
							start_index++;

						start_who++;
					}
				}
			}
		}
	}
	else{
		char *prefix = "eth";
		int start_index = x_dev->start_index;
		for ( i = 0; i<DEV_CARD_MAX; i++ ){
			struct x_card_struct *item = &(x_dev->card_item[i]);
			if(item->conf==NULL)
				continue;

			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				if ( ( item->eth_item[j].type == TYPE_HA)){
					if(item->eth_item[j].newname[0] == 0)
						snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							"HA");
				}
				else if ( ( item->eth_item[j].type == TYPE_MGT)){
					if(item->eth_item[j].newname[0] == 0)
						if(item->conf->start_index<=0){
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"MGT");
						}
						else{
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"MGT%d", item->conf->start_index);
						}
				}
				else{
					if(item->conf->start_index != -1){
						if(item->eth_item[j].newname[0] == 0)
							snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
								"%s%d", prefix, start_index++);
					}
				}
			}
		}
	}

	if(adaptive){
		adaptive(x_dev);
	}

	if(0 != system("cat /home/ace_port_map.dat 2>/dev/null  |grep -q  \"ManMakeExtMap\"")
		&&
		0 == system("cat /home/ace_port_map.dat 2>/dev/null  |grep -q  \"ifAutoCfgMode 0\"")){
		return;
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode 1\n");
		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		for ( i = 0; i<DEV_CARD_MAX; i++ ){
			struct x_card_struct *item = &(x_dev->card_item[i]);
			if(item->conf==NULL)
				continue;

			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				if(item->eth_item[j].panel_info[0] != 0){
						snprintf( tmpstr, sizeof(tmpstr), "%s\n", item->eth_item[j].panel_info);			
						fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
				}

				if(x_dev->mode == MOD_GE){
					if ( ( item->eth_item[j].type == TYPE_HA ) || ( item->eth_item[j].type == TYPE_MGT ) ){
						snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore outband\n", item->eth_item[j].oldname, item->eth_item[j].newname );
					}
					else{
						snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore inband\n", item->eth_item[j].oldname, item->eth_item[j].newname );
					}
				}
				else if(x_dev->mode == MOD_LANWAN){
					snprintf( tmpstr, sizeof(tmpstr), "%s %s %d %s no-ignore\n", item->eth_item[j].oldname, item->eth_item[j].newname,
						item->eth_item[j].lanwan_index, item->eth_item[j].lanwan_attr);
				}
				else{
					snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore\n", item->eth_item[j].oldname, item->eth_item[j].newname );
				}
				fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
			}
		}

		fclose( fp ); fp = NULL;

		generate_linux2vpp();
	}
}

void handle_x_card(struct x_card_config *conf, rename_mode mode, rename_who start_who, int start_index, eth_adaptive adaptive)
{
	struct x_dev_struct *x_dev = &g_x_dev;
	memset(x_dev, 0x00, sizeof(struct x_dev_struct));

	x_dev->start_who   = start_who;
	x_dev->start_index = start_index;
	x_dev->mode = mode;

	parse_x_card(conf);

	generate_port_map(adaptive);
}

int have_pci_device (const char* device)
{
	int result;
	char  cmdbuf [128]={0};
	sprintf( cmdbuf , "cat /proc/bus/pci/devices | grep %s", device);
	result = !(system ( cmdbuf ));
	return result;
}

int set_map_port_pre(void)
{
	int type = get_boardtype();
	board_adaptive_fun_set *fset = g_adaptive_fun_set;

	if(board_type_RESV != type){
		if(fset[type].set_map_port_pre){
			fset[type].set_map_port_pre();
			return 1;
		}
	}

	return 0;
}

int set_map_port( void )
{
	int iRet = 0;
	int type = get_boardtype();

	board_adaptive_fun_set *fset = g_adaptive_fun_set;

	if ( board_type_RESV != type )
	{
		if ( fset[type].set_map_port )
		{
			fset[type].set_map_port();

			rename_mgt_ha();

			read_ace_port_map();
			return 1;
		}
	}

	return 0;
}

int set_map_port_post(void)
{
	int iRet = 0;
	int type = get_boardtype();

	board_adaptive_fun_set *fset = g_adaptive_fun_set;

	if ( board_type_RESV != type )
	{
		if ( fset[type].set_map_port_post )
		{
			fset[type].set_map_port_post();

			iRet = 1;
		}
	}

	// for firewall, eth, mgt feature control
	if ( 1 == ace_is_firewall() )
	{
		if ( F_OK == access( "/usr/mysys/.nomgt", F_OK ) )
		{
			if ( F_OK == access( "/home/config/current/inband_mgt_config", F_OK ) )
			{
				unlink( "/home/config/current/inband_mgt_config" );
			}
		}
	}

	return iRet;
}

int def_hylab_vpp_cmd_conf(void)
{
	int type = get_boardtype();

	board_adaptive_fun_set *fset = g_adaptive_fun_set;

	if ( board_type_RESV != type )
	{
		if(fset[type].def_hylab_vpp_cmd_conf)
		{
			fset[type].def_hylab_vpp_cmd_conf();
			return 1;
		}
	}

	return 0;
}

int update_hylab_vpp_cmd_conf( void )
{
	int type = get_boardtype();

	board_adaptive_fun_set *fset = g_adaptive_fun_set;

	if ( board_type_RESV != type )
	{
		if ( fset[type].update_hylab_vpp_cmd_conf )
		{
			fset[type].update_hylab_vpp_cmd_conf();

			return 1;
		}
	}

	return 0;
}

static int ruijie_init_boardtype(void)
{
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     eth_num = 0;

	fp = fopen( BYPASS_SWITCH_CONFIG, "r" );

	if ( fp )
	{
		while ( fgets( line, sizeof( line ), fp ) != NULL )
		{
			if ( line[0] == '#' )
			{
				continue;
			}

			if ( 1 == sscanf( line, "TYPE = %s", tmpstr ) )
			{
				break;
			}
		}

		fclose( fp ); fp = NULL;
	}

	if ( !strstr( tmpstr, "RUIJIE" ) )
	{
		return board_type_RESV;
	}

    if ( !access( "/usr/mysys/z8620_config.bcm", F_OK ) )
    {
        return board_type_Z8620;
    }

	if ( !access( "/usr/mysys/z8680_config.bcm", F_OK ) )
	{
		return board_type_Z8680;
	}

	eth_num = ace_get_port_num();

	fp = popen( "cat /proc/cpuinfo | grep \"model name\" | awk 'NR==1'", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		fgets( line, sizeof(line), fp );

		pclose( fp ); fp = NULL;

		// U3210 or U3100
		if ( !strncmp( line, "model name\t: Intel(R) Atom(TM) CPU C3558 @ 2.20GHz", sizeof("model name\t: Intel(R) Atom(TM) CPU C3558 @ 2.20GHz") - 1 ) )
		{
			// U3100
			if ( have_pci_device("80861980") && have_pci_device("808619a1") && have_pci_device("808619db") && ( 10 == eth_num ) )
			{
				return board_type_U3100;
			}
			// U3210
			else if ( have_pci_device("80861980") && have_pci_device("808619a1") && have_pci_device("808619e2") && ( eth_num >= 8 ) )
			{
				return board_type_U3210;
			}
		}
	}

	fp = popen( "mpstat | awk -F '(' '{if(NR==1)print $3}' | awk -F ' ' '{print $1}'", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		fgets( line, sizeof( line ), fp );

		pclose( fp ); fp = NULL;

		// X300D
		if (line[0] == '4' && line[1] == '0')
		{
			return board_type_X300D;
		}
	}

	return board_type_RESV;
}

int init_boardtype( void )
{
	board_adaptive_fun_set *fset = g_adaptive_fun_set;

	memset(g_adaptive_fun_set, 0, sizeof(g_adaptive_fun_set));

	fset[board_type_COMMON_X86].update_hylab_vpp_cmd_conf = common_x86_update_hylab_vpp_cmd_conf;
	fset[board_type_NK06].update_hylab_vpp_cmd_conf = nk06_update_hylab_vpp_cmd_conf;

	fset[board_type_Z8680].set_map_port_pre = Z8680_set_map_port_pre;
	fset[board_type_Z8680].set_map_port_post = Z8680_set_map_port_post;
	fset[board_type_Z8680].update_hylab_vpp_cmd_conf = Z8680_update_hylab_vpp_cmd_conf;

	fset[board_type_U3100].set_map_port_pre = U3100_set_map_port_pre;
	fset[board_type_U3100].set_map_port_post = U3100_set_map_port_post;
	fset[board_type_U3100].update_hylab_vpp_cmd_conf = U3100_update_hylab_vpp_cmd_conf;

	fset[board_type_U3210].set_map_port_pre = U3210_set_map_port_pre;
	fset[board_type_U3210].set_map_port_post = U3210_set_map_port_post;
	fset[board_type_U3210].update_hylab_vpp_cmd_conf = U3210_update_hylab_vpp_cmd_conf;

#ifndef ARCH_ARM64
	fset[board_type_X300D].set_map_port_pre = X300D_set_map_port_pre;
	fset[board_type_X300D].set_map_port = X300D_set_map_port;
	fset[board_type_X300D].set_map_port_post = X300D_set_map_port_post;
	fset[board_type_X300D].def_hylab_vpp_cmd_conf = X300D_def_hylab_vpp_cmd_conf;
	fset[board_type_X300D].update_hylab_vpp_cmd_conf = X300D_update_hylab_vpp_cmd_conf;
#endif

	fset[board_type_T7L].set_map_port_pre = T7L_set_map_port_pre;
	fset[board_type_T7L].set_map_port = T7L_set_map_port;
	fset[board_type_T7L].set_map_port_post = T7L_set_map_port_post;
	fset[board_type_T7L].def_hylab_vpp_cmd_conf = T7L_def_hylab_vpp_cmd_conf;
	fset[board_type_T7L].update_hylab_vpp_cmd_conf = T7L_update_hylab_vpp_cmd_conf;

	fset[board_type_NR03].set_map_port_pre = nr03_set_map_port_pre;
	fset[board_type_NR03].set_map_port = nr03_set_map_port;
	fset[board_type_NR03].set_map_port_post = nr03_set_map_port_post;
	fset[board_type_NR03].def_hylab_vpp_cmd_conf = nr03_def_hylab_vpp_cmd_conf;
	fset[board_type_NR03].update_hylab_vpp_cmd_conf = nr03_update_hylab_vpp_cmd_conf;

	fset[board_type_RIS5060].set_map_port_pre = leyan_e2000q_set_map_port_pre;
	fset[board_type_RIS5060].set_map_port = leyan_e2000q_set_map_port;
	fset[board_type_RIS5060].set_map_port_post = leyan_e2000q_set_map_port_post;
	fset[board_type_RIS5060].def_hylab_vpp_cmd_conf = leyan_e2000q_def_hylab_vpp_cmd_conf;
	fset[board_type_RIS5060].update_hylab_vpp_cmd_conf = leyan_e2000q_update_hylab_vpp_cmd_conf;

	fset[board_type_NM59].set_map_port_pre = nm59_set_map_port_pre;
	fset[board_type_NM59].set_map_port_post = nm59_set_map_port_post;
	fset[board_type_NM59].def_hylab_vpp_cmd_conf = nm59_def_hylab_vpp_cmd_conf;
	fset[board_type_NM59].update_hylab_vpp_cmd_conf = nm59_update_hylab_vpp_cmd_conf;

	fset[board_type_NM32].set_map_port_pre = nm32_set_map_port_pre;
	fset[board_type_NM32].set_map_port_post = nm32_set_map_port_post;
	fset[board_type_NM32].def_hylab_vpp_cmd_conf = nm32_def_hylab_vpp_cmd_conf;
	fset[board_type_NM32].update_hylab_vpp_cmd_conf = nm32_update_hylab_vpp_cmd_conf;

	fset[board_type_NM15].set_map_port_pre = nm15_set_map_port_pre;
	fset[board_type_NM15].set_map_port_post = nm15_set_map_port_post;
	fset[board_type_NM15].def_hylab_vpp_cmd_conf = nm15_def_hylab_vpp_cmd_conf;
	fset[board_type_NM15].update_hylab_vpp_cmd_conf = nm15_update_hylab_vpp_cmd_conf;

	fset[board_type_NZ02].set_map_port_pre = nz02_set_map_port_pre;
	fset[board_type_NZ02].set_map_port = nz02_set_map_port;
	fset[board_type_NZ02].set_map_port_post = nz02_set_map_port_post;
	fset[board_type_NZ02].def_hylab_vpp_cmd_conf = nz02_def_hylab_vpp_cmd_conf;
	fset[board_type_NZ02].update_hylab_vpp_cmd_conf = nz02_update_hylab_vpp_cmd_conf;

	fset[board_type_NR01].update_hylab_vpp_cmd_conf = nr01_update_hylab_vpp_cmd_conf;

	fset[board_type_RIS297].set_map_port = leyan_ft2000_set_map_port;
	fset[board_type_RIS297].set_map_port_post = leyan_ft2000_set_map_port_post;
	fset[board_type_RIS297].def_hylab_vpp_cmd_conf = leyan_ft2000_def_hylab_vpp_cmd_conf;
	fset[board_type_RIS297].update_hylab_vpp_cmd_conf = leyan_ft2000_update_hylab_vpp_cmd_conf;

	fset[board_type_ZHGX_N100].set_map_port_pre = zhgx_n100_set_map_port_pre;
	fset[board_type_ZHGX_N100].set_map_port = zhgx_n100_set_map_port;
	fset[board_type_ZHGX_N100].set_map_port_post = zhgx_n100_set_map_port_post;
	fset[board_type_ZHGX_N100].def_hylab_vpp_cmd_conf = zhgx_n100_def_hylab_vpp_cmd_conf;
	fset[board_type_ZHGX_N100].update_hylab_vpp_cmd_conf = zhgx_n100_update_hylab_vpp_cmd_conf;

	fset[board_type_T5H].update_hylab_vpp_cmd_conf = t5h_update_hylab_vpp_cmd_conf;

	fset[board_type_T7H].update_hylab_vpp_cmd_conf = t7h_update_hylab_vpp_cmd_conf;

	fset[board_type_HG3250].set_map_port_post = hg3250_set_map_port_post;
	fset[board_type_HG3250].update_hylab_vpp_cmd_conf = hg3250_update_hylab_vpp_cmd_conf;

	fset[board_type_NK03].set_map_port_pre = nk03_set_map_port_pre;
	fset[board_type_NK03].set_map_port = nk03_set_map_port;
	fset[board_type_NK03].set_map_port_post = nk03_set_map_port_post;
	fset[board_type_NK03].def_hylab_vpp_cmd_conf = nk03_def_hylab_vpp_cmd_conf;
	fset[board_type_NK03].update_hylab_vpp_cmd_conf = nk03_update_hylab_vpp_cmd_conf;

	fset[board_type_NT09].update_hylab_vpp_cmd_conf = nt09_update_hylab_vpp_cmd_conf;

	fset[board_type_NH31].update_hylab_vpp_cmd_conf = nh31_update_hylab_vpp_cmd_conf;

	fset[board_type_NH10].update_hylab_vpp_cmd_conf = nh10_update_hylab_vpp_cmd_conf;

	fset[board_type_NT03].update_hylab_vpp_cmd_conf = nt03_update_hylab_vpp_cmd_conf;
	fset[board_type_NT03].set_map_port_post = nt03_set_map_port_post;

	fset[board_type_GH7131].set_map_port_pre = guanghe_7131_set_map_port_pre;
	fset[board_type_GH7131].set_map_port = guanghe_7131_set_map_port;
	fset[board_type_GH7131].set_map_port_post = guanghe_7131_set_map_port_post;
	fset[board_type_GH7131].def_hylab_vpp_cmd_conf = guanghe_7131_def_hylab_vpp_cmd_conf;
	fset[board_type_GH7131].update_hylab_vpp_cmd_conf = guanghe_7131_update_hylab_vpp_cmd_conf;

	fset[board_type_XGT_D2000].set_map_port_pre = xgt_d2000_set_map_port_pre;
	fset[board_type_XGT_D2000].set_map_port = xgt_d2000_set_map_port;
	fset[board_type_XGT_D2000].set_map_port_post = xgt_d2000_set_map_port_post;
	fset[board_type_XGT_D2000].def_hylab_vpp_cmd_conf = xgt_d2000_def_hylab_vpp_cmd_conf;
	fset[board_type_XGT_D2000].update_hylab_vpp_cmd_conf = xgt_d2000_update_hylab_vpp_cmd_conf;

	fset[board_type_BXY_HG3X].set_map_port_pre = bxy_hg3x_set_map_port_pre;
	fset[board_type_BXY_HG3X].set_map_port = bxy_hg3x_set_map_port;
	fset[board_type_BXY_HG3X].set_map_port_post = bxy_hg3x_set_map_port_post;
	fset[board_type_BXY_HG3X].def_hylab_vpp_cmd_conf = bxy_hg3x_def_hylab_vpp_cmd_conf;
	fset[board_type_BXY_HG3X].update_hylab_vpp_cmd_conf = bxy_hg3x_update_hylab_vpp_cmd_conf;

	fset[board_type_CS_NL01].set_map_port_pre = nl01_set_map_port_pre;
	fset[board_type_CS_NL01].set_map_port = nl01_set_map_port;
	fset[board_type_CS_NL01].set_map_port_post = nl01_set_map_port_post;
	fset[board_type_CS_NL01].def_hylab_vpp_cmd_conf = nl01_def_hylab_vpp_cmd_conf;
	fset[board_type_CS_NL01].update_hylab_vpp_cmd_conf = nl01_update_hylab_vpp_cmd_conf;

	fset[board_type_CS_NT12].set_map_port_pre = nt12_set_map_port_pre;
	fset[board_type_CS_NT12].set_map_port = nt12_set_map_port;
	fset[board_type_CS_NT12].set_map_port_post = nt12_set_map_port_post;
	fset[board_type_CS_NT12].def_hylab_vpp_cmd_conf = nt12_def_hylab_vpp_cmd_conf;
	fset[board_type_CS_NT12].update_hylab_vpp_cmd_conf = nt12_update_hylab_vpp_cmd_conf;

	fset[board_type_rk3568_NSC2C01].set_map_port_pre = rk3568_nsc2c01_set_map_port_pre;
	fset[board_type_rk3568_NSC2C01].set_map_port = rk3568_nsc2c01_set_map_port;
	fset[board_type_rk3568_NSC2C01].set_map_port_post = rk3568_nsc2c01_set_map_port_post;
	fset[board_type_rk3568_NSC2C01].def_hylab_vpp_cmd_conf = rk3568_nsc2c01_def_hylab_vpp_cmd_conf;
	fset[board_type_rk3568_NSC2C01].update_hylab_vpp_cmd_conf = rk3568_nsc2c01_update_hylab_vpp_cmd_conf;

	fset[board_type_NK02].set_map_port_pre = nk02_set_map_port_pre;
	fset[board_type_NK02].set_map_port = nk02_set_map_port;
	fset[board_type_NK02].set_map_port_post = nk02_set_map_port_post;
	fset[board_type_NK02].def_hylab_vpp_cmd_conf = nk02_def_hylab_vpp_cmd_conf;
	fset[board_type_NK02].update_hylab_vpp_cmd_conf = nk02_update_hylab_vpp_cmd_conf;

	fset[board_type_trx_E2000Q].set_map_port_pre = trx_e2000q_set_map_port_pre;
	fset[board_type_trx_E2000Q].set_map_port = trx_e2000q_set_map_port;
	fset[board_type_trx_E2000Q].set_map_port_post = trx_e2000q_set_map_port_post;
	fset[board_type_trx_E2000Q].def_hylab_vpp_cmd_conf = trx_e2000q_def_hylab_vpp_cmd_conf;
	fset[board_type_trx_E2000Q].update_hylab_vpp_cmd_conf = trx_e2000q_update_hylab_vpp_cmd_conf;

	fset[board_type_ZHGX_M1].set_map_port_pre = zhgx_m1_set_map_port_pre;
	fset[board_type_ZHGX_M1].set_map_port = zhgx_m1_set_map_port;
	fset[board_type_ZHGX_M1].set_map_port_post = zhgx_m1_set_map_port_post;
	fset[board_type_ZHGX_M1].def_hylab_vpp_cmd_conf = zhgx_m1_def_hylab_vpp_cmd_conf;
	fset[board_type_ZHGX_M1].update_hylab_vpp_cmd_conf = zhgx_m1_update_hylab_vpp_cmd_conf;

	fset[board_type_YWER_HG5X].set_map_port_pre = ywer_hg5x_set_map_port_pre;
	fset[board_type_YWER_HG5X].set_map_port = ywer_hg5x_set_map_port;
	fset[board_type_YWER_HG5X].set_map_port_post = ywer_hg5x_set_map_port_post;
	fset[board_type_YWER_HG5X].def_hylab_vpp_cmd_conf = ywer_hg5x_def_hylab_vpp_cmd_conf;
	fset[board_type_YWER_HG5X].update_hylab_vpp_cmd_conf = ywer_hg5x_update_hylab_vpp_cmd_conf;

	fset[board_type_YR_F28622].set_map_port_pre = yanrui_f28622_set_map_port_pre;
	fset[board_type_YR_F28622].set_map_port = yanrui_f28622_set_map_port;
	fset[board_type_YR_F28622].set_map_port_post = yanrui_f28622_set_map_port_post;
	fset[board_type_YR_F28622].def_hylab_vpp_cmd_conf = yanrui_f28622_def_hylab_vpp_cmd_conf;
	fset[board_type_YR_F28622].update_hylab_vpp_cmd_conf = yanrui_f28622_update_hylab_vpp_cmd_conf;

	fset[board_type_WESTONE_HG3X].set_map_port_pre = hzc8311_set_map_port_pre;
	fset[board_type_WESTONE_HG3X].set_map_port = hzc8311_set_map_port;
	fset[board_type_WESTONE_HG3X].set_map_port_post = hzc8311_set_map_port_post;
	fset[board_type_WESTONE_HG3X].def_hylab_vpp_cmd_conf = hzc8311_def_hylab_vpp_cmd_conf;
	fset[board_type_WESTONE_HG3X].update_hylab_vpp_cmd_conf = hzc8311_update_hylab_vpp_cmd_conf;

	fset[board_type_CS_NH12].set_map_port_pre = nh12_set_map_port_pre;
	fset[board_type_CS_NH12].set_map_port = nh12_set_map_port;
	fset[board_type_CS_NH12].set_map_port_post = nh12_set_map_port_post;
	fset[board_type_CS_NH12].def_hylab_vpp_cmd_conf = nh12_def_hylab_vpp_cmd_conf;
	fset[board_type_CS_NH12].update_hylab_vpp_cmd_conf = nh12_update_hylab_vpp_cmd_conf;

	fset[board_type_ZHGX_AC2000_ULTRA].set_map_port_pre = zhgx_ac2000_ultra_set_map_port_pre;
	fset[board_type_ZHGX_AC2000_ULTRA].set_map_port = zhgx_ac2000_ultra_set_map_port;
	fset[board_type_ZHGX_AC2000_ULTRA].set_map_port_post = zhgx_ac2000_ultra_set_map_port_post;
	fset[board_type_ZHGX_AC2000_ULTRA].def_hylab_vpp_cmd_conf = zhgx_ac2000_ultra_def_hylab_vpp_cmd_conf;
	fset[board_type_ZHGX_AC2000_ULTRA].update_hylab_vpp_cmd_conf = zhgx_ac2000_ultra_update_hylab_vpp_cmd_conf;

	fset[board_type_rk3568_NSC1107].set_map_port_pre = rk3568_nsc1107_set_map_port_pre;
	fset[board_type_rk3568_NSC1107].set_map_port = rk3568_nsc1107_set_map_port;
	fset[board_type_rk3568_NSC1107].set_map_port_post = rk3568_nsc1107_set_map_port_post;
	fset[board_type_rk3568_NSC1107].def_hylab_vpp_cmd_conf = rk3568_nsc1107_def_hylab_vpp_cmd_conf;
	fset[board_type_rk3568_NSC1107].update_hylab_vpp_cmd_conf = rk3568_nsc1107_update_hylab_vpp_cmd_conf;

	fset[board_type_CS_NT13].set_map_port_pre = nt13_set_map_port_pre;
	fset[board_type_CS_NT13].set_map_port = nt13_set_map_port;
	fset[board_type_CS_NT13].set_map_port_post = nt13_set_map_port_post;
	fset[board_type_CS_NT13].def_hylab_vpp_cmd_conf = nt13_def_hylab_vpp_cmd_conf;
	fset[board_type_CS_NT13].update_hylab_vpp_cmd_conf = nt13_update_hylab_vpp_cmd_conf;

	fset[board_type_jdsa_E2000Q].set_map_port_pre = jdsa_e2000q_set_map_port_pre;
	fset[board_type_jdsa_E2000Q].set_map_port = jdsa_e2000q_set_map_port;
	fset[board_type_jdsa_E2000Q].set_map_port_post = jdsa_e2000q_set_map_port_post;
	fset[board_type_jdsa_E2000Q].def_hylab_vpp_cmd_conf = jdsa_e2000q_def_hylab_vpp_cmd_conf;
	fset[board_type_jdsa_E2000Q].update_hylab_vpp_cmd_conf = jdsa_e2000q_update_hylab_vpp_cmd_conf;

	fset[board_type_jdsa_HG3X].set_map_port_pre = jdsa_hg3x_set_map_port_pre;
	fset[board_type_jdsa_HG3X].set_map_port = jdsa_hg3x_set_map_port;
	fset[board_type_jdsa_HG3X].set_map_port_post = jdsa_hg3x_set_map_port_post;
	fset[board_type_jdsa_HG3X].def_hylab_vpp_cmd_conf = jdsa_hg3x_def_hylab_vpp_cmd_conf;
	fset[board_type_jdsa_HG3X].update_hylab_vpp_cmd_conf = jdsa_hg3x_update_hylab_vpp_cmd_conf;

	fset[board_type_leyan_HYSA7620].set_map_port_pre = leyan_hysa7620_set_map_port_pre;
	fset[board_type_leyan_HYSA7620].set_map_port = leyan_hysa7620_set_map_port;
	fset[board_type_leyan_HYSA7620].set_map_port_post = leyan_hysa7620_set_map_port_post;
	fset[board_type_leyan_HYSA7620].def_hylab_vpp_cmd_conf = leyan_hysa7620_def_hylab_vpp_cmd_conf;
	fset[board_type_leyan_HYSA7620].update_hylab_vpp_cmd_conf = leyan_hysa7620_update_hylab_vpp_cmd_conf;

	fset[board_type_VM_X86].set_map_port_pre = vmx86_set_map_port_pre;
	fset[board_type_VM_X86].set_map_port = vmx86_set_map_port;
	fset[board_type_VM_X86].set_map_port_post = vmx86_set_map_port_post;
	fset[board_type_VM_X86].def_hylab_vpp_cmd_conf = vmx86_def_hylab_vpp_cmd_conf;
	fset[board_type_VM_X86].update_hylab_vpp_cmd_conf = vmx86_update_hylab_vpp_cmd_conf;

	fset[board_type_yanhao_i54590].set_map_port_pre = yh_i54590_set_map_port_pre;
	fset[board_type_yanhao_i54590].set_map_port = yh_i54590_set_map_port;
	fset[board_type_yanhao_i54590].set_map_port_post = yh_i54590_set_map_port_post;
	fset[board_type_yanhao_i54590].def_hylab_vpp_cmd_conf = yh_i54590_def_hylab_vpp_cmd_conf;
	fset[board_type_yanhao_i54590].update_hylab_vpp_cmd_conf = yh_i54590_update_hylab_vpp_cmd_conf;

	fset[board_type_leyan_ris5066].set_map_port_pre = leyan_ris5066_set_map_port_pre;
	fset[board_type_leyan_ris5066].set_map_port = leyan_ris5066_set_map_port;
	fset[board_type_leyan_ris5066].set_map_port_post = leyan_ris5066_set_map_port_post;
	fset[board_type_leyan_ris5066].def_hylab_vpp_cmd_conf = leyan_ris5066_def_hylab_vpp_cmd_conf;
	fset[board_type_leyan_ris5066].update_hylab_vpp_cmd_conf = leyan_ris5066_update_hylab_vpp_cmd_conf;

	fset[board_type_antaike_hg332].set_map_port_pre = antaike_hg332_set_map_port_pre;
	fset[board_type_antaike_hg332].set_map_port = antaike_hg332_set_map_port;
	fset[board_type_antaike_hg332].set_map_port_post = antaike_hg332_set_map_port_post;
	fset[board_type_antaike_hg332].def_hylab_vpp_cmd_conf = antaike_hg332_def_hylab_vpp_cmd_conf;
	fset[board_type_antaike_hg332].update_hylab_vpp_cmd_conf = antaike_hg332_update_hylab_vpp_cmd_conf;

	fset[board_type_sanwang_e2000q].set_map_port_pre = sanwang_e2000q_set_map_port_pre;
	fset[board_type_sanwang_e2000q].set_map_port = sanwang_e2000q_set_map_port;
	fset[board_type_sanwang_e2000q].set_map_port_post = sanwang_e2000q_set_map_port_post;
	fset[board_type_sanwang_e2000q].def_hylab_vpp_cmd_conf = sanwang_e2000q_def_hylab_vpp_cmd_conf;
	fset[board_type_sanwang_e2000q].update_hylab_vpp_cmd_conf = sanwang_e2000q_update_hylab_vpp_cmd_conf;

	/*----------------new disk--------------------*/
	const value_type *abt = g_allBoardType;
	int i, j, found = 0;
	FILE*   fp[] = { NULL, NULL, NULL, NULL };
	char    buffer[512]  = { 0 };
	char *file[] = {
		"/proc/device-tree/model",
		FIRMWARE_BOARD_TYPE,
		"/sys/devices/virtual/dmi/id/product_version",
		"/sys/devices/virtual/dmi/id/product_name"
	};

	for(i=0; i<(sizeof(file)/sizeof(file[0])); i++)
	{
		fp[i] = fopen(file[i], "r");
		if(fp[i] == NULL){
			//printf("fopen %s failed!\n", file[i]);
			continue;
		}

		if ( 0 != fgets(buffer, sizeof(buffer), fp[i] ) )
		{
			for (j=0; abt[j].strptr!=NULL; j++){
				if (strstr(buffer, abt[j].strptr)){
					g_cur_board_type.value  = abt[j].value;
					g_cur_board_type.strptr = abt[j].strptr;
					found = 1;
					break;
				}
			}

			if(found)
				break;
		}
	}

	for(i=0; i<(sizeof(fp)/sizeof(fp[0])); i++)
		if(fp[i]) fclose( fp[i] );

	if ( found ) {
		return g_cur_board_type.value;
	}

	/*----------------old disk--------------------*/
	if ( board_type_RESV != ( g_cur_board_type.value = ruijie_init_boardtype() ) ) {
		goto oldfound;
	}

oldfound:
	for (j=0; abt[j].strptr!=NULL; j++){
		if (g_cur_board_type.value == abt[j].value){
			g_cur_board_type.strptr = abt[j].strptr;
			break;
		}
	}

	return g_cur_board_type.value;
}

int get_boardtype( void )
{
	return g_cur_board_type.value;
}

const char* get_boardstr( void )
{
	return g_cur_board_type.strptr;
}

int com_get_inter_info_from_map( lpboard_interface_config lpEths, int iDevType )
{
	int i = 0;
	int iRet = 0;
	char szLine[256] = { 0 };
	char  oldname[32]  = { 0 };
	char  newname[32]  = { 0 };
	char  noIgnore[32] = { 0 };
	char  band[32]	   = { 0 };
	char  property[32] = { 0 };
	int   ifindex      = 0;

	init_coordinate_log( "%s running\n", __FUNCTION__ );

	if ( NULL == lpEths )
	{
		return -1;
	}

	FILE* fp = fopen( ACE_PORT_MAP_FILE, "r" );

	if ( fp )
	{
		while ( 0 != fgets( szLine, sizeof( szLine ), fp ) )
		{
			if ( ( '#' == szLine[0] ) || ( '\n' == szLine[0] ) )
			{
				continue;
			}
			else if ( strstr( szLine, "ifAutoCfgMode" ) )
			{
				continue;
			}

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
			if ( sscanf( szLine, "%s %s %s %s", oldname, newname, noIgnore, band ) != 4 )
#else
			if ( sscanf( szLine, "%s %s %s", oldname, newname, noIgnore ) != 3 )
#endif
#else
			if ( sscanf( szLine, "%s %s %d %s %s", oldname, newname, &ifindex, property, noIgnore ) != 5 )
#endif
			{
				init_coordinate_log( "warning, invalid line %s", szLine );

				continue;
			}

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
			if ( 'o' == band[0] )
#else
			if ( ( 'M' == newname[0] ) || ( 'H' == newname[0] ) )
#endif
#else
			if ( !strcmp( property, "MANAGE" ) )
#endif
			{
				init_coordinate_log( "warning, manage eth [%s], invalid line %s", newname, szLine );

				continue;
			}

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
			if ( strncmp( newname, "Eth", strlen( "Eth" ) ) && strncmp( newname, "FE", strlen( "FE" ) ) && strncmp( newname, "GE", strlen( "GE" ) )
				&& strncmp( newname, "XGE", strlen( "XGE" ) ) && strncmp( newname, "TGE", strlen( "TGE" ) ) && strncmp( newname, "FGE", strlen( "FGE" ) )
				&& strncmp( newname, "HGE", strlen( "HGE" ) )
			)
#else
			if ( strncmp( newname, "eth", strlen( "eth" ) )
#endif
#else
			if ( strncmp( newname, "LAN", strlen( "LAN" ) ) && strncmp( newname, "WAN", strlen( "WAN" ) ) )
#endif
			{
				init_coordinate_log( "warning, unknown new eth name [%s], invalid line %s", newname, szLine );

				continue;
			}

			if ( ( memcmp( noIgnore, "no-ignore", strlen( "no-ignore" ) ) ) )
			{
				init_coordinate_log( "warning, no-ignore, invalid line %s", szLine );

				continue;
			}

			int iEthSaved = 0;

			for ( i=0; i<lpEths->m_iEthCount; i++ )
			{
				if ( !strcmp( lpEths->m_arrEthConfig[i], newname  ) )
				{
					iEthSaved = 1;

					break;
				}
			}

			if ( 1 == iEthSaved )
			{
				continue;
			}

			if ( lpEths->m_iEthCount >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
			{
				init_coordinate_log( "warning, more than max eth num [%d], line %s", ACE_MAX_SUPPORT_PORT_BUSYBOX, szLine );

				break;
			}

			snprintf( lpEths->m_arrEthConfig[lpEths->m_iEthCount], sizeof( lpEths->m_arrEthConfig[lpEths->m_iEthCount]	), "%s", newname );

			init_coordinate_log( "info, new eth [%d, %s]", lpEths->m_iEthCount, newname );

			lpEths->m_iEthCount++;
		}

		fclose( fp ); fp = NULL;
	}
	else
	{
		iRet = -2;

		init_coordinate_log( "Error, open file [%s] failed.", ACE_PORT_MAP_FILE );
	}

	return iRet;
}

int com_get_inter_info_from_switch( lpboard_interface_config lpEths, int iDevType )
{
	int i = 0;
	int iRet = 0;
	char szLine[1024] = {0};

	init_coordinate_log( "%s running\n", __FUNCTION__ );

	if ( lpEths )
	{
		FILE* fp = fopen( "/home/config/current/hylab_pre_vpp_cmd.conf", "r" );

		if ( fp )
		{
			while ( 0 != fgets( szLine, sizeof( szLine ), fp ) )
			{
				// create switch-interface name LAN1 parent-interface switch0 map-vlan-id 101 switch-unit 0 switch-port-no 1 switch-port-name ge0
				if ( 0 == strncmp( szLine, "create switch-interface name ", strlen( "create switch-interface name " ) ) )
				{
					char szEthInfo[64] = {0};

					char* lpStart = szLine + strlen( "create switch-interface name " );

					char* lpEnd = strstr( szLine,  " parent-interface" );

					if ( lpEnd && ( lpEnd - lpStart > 0 ) && ( lpEnd - lpStart < sizeof( szEthInfo ) ) )
					{
						strncpy( szEthInfo, lpStart, lpEnd - lpStart );

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
						if ( strncmp( szEthInfo, "Eth", strlen( "Eth" ) ) && strncmp( szEthInfo, "FE", strlen( "FE" ) ) && strncmp( szEthInfo, "GE", strlen( "GE" ) )
							&& strncmp( szEthInfo, "XGE", strlen( "XGE" ) ) && strncmp( szEthInfo, "TGE", strlen( "TGE" ) ) && strncmp( szEthInfo, "FGE", strlen( "FGE" ) )
							&& strncmp( szEthInfo, "HGE", strlen( "HGE" ) )
						)
#else
						if ( strncmp( szEthInfo, "eth", strlen( "eth" ) )
#endif
#else
						if ( strncmp( szEthInfo, "LAN", strlen( "LAN" ) ) && strncmp( szEthInfo, "WAN", strlen( "WAN" ) ) )
#endif
						{
							init_coordinate_log( "warning, unknown new eth name [%s], invalid line %s", szEthInfo, szLine );

							continue;
						}

						int iEthSaved = 0;

						for ( i=0; i<lpEths->m_iEthCount; i++ )
						{
							if ( !strcmp( lpEths->m_arrEthConfig[i], szEthInfo  ) )
							{
								iEthSaved = 1;

								break;
							}
						}

						if ( 1 == iEthSaved )
						{
							continue;
						}

						if ( lpEths->m_iEthCount >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
						{
							init_coordinate_log( "warning, more than max eth num [%d], line %s", ACE_MAX_SUPPORT_PORT_BUSYBOX, szLine );

							break;
						}

						snprintf( lpEths->m_arrEthConfig[lpEths->m_iEthCount], sizeof( lpEths->m_arrEthConfig[lpEths->m_iEthCount]	), "%s", szEthInfo );

						init_coordinate_log( "info, new eth [%d, %s]", lpEths->m_iEthCount, szEthInfo );

						lpEths->m_iEthCount++;
					}
				}
			}

			fclose( fp ); fp = NULL;
		}
	}
	else
	{
		iRet = -1;
	}

	return iRet;
}
void str_replace(char *pInput, char *pOutput, char *pSrc, char *pDst);
char *trim(char *s);
int com_update_hylab_vpp_cmd_conf( board_switch switchFunc, int iDevType )
{
	int i = 0;
	int iModify = 0;
	int iLcpCheck = 0;
	FILE* fpOld = NULL;
	FILE* fpNew = NULL;
	FILE *fpConfig = NULL;
	char szLine[1024] = {0};
	char fconfig[1024]={0};
	board_interface_config bicConfig;

	init_coordinate_log( "%s running\n", __FUNCTION__ );

	memset( &bicConfig, 0, sizeof( bicConfig ) );

	com_get_inter_info_from_map( &bicConfig, iDevType );

	if ( NULL != switchFunc )
	{
		switchFunc( &bicConfig, iDevType );
	}

	do
	{
		if ( bicConfig.m_iEthCount <= 0 )
		{
			init_coordinate_log( "Warning, config eth count [%d] is less than 0", bicConfig.m_iEthCount );

			break;
		}

		fpOld = fopen( HYLAB_VPP_CMD_FILE, "r" );

		if ( !fpOld )
		{
			init_coordinate_log( "open "HYLAB_VPP_CMD_FILE" error" );

			break;
		}

		fpNew = fopen( HYLAB_VPP_CMD_FILE".tmp", "w+" );

		if ( !fpNew )
		{
			init_coordinate_log( "create "HYLAB_VPP_CMD_FILE".tmp error" );

			break;
		}

		int search_lcp_flag = 1;
		int lcp_enable_found = 0;
		int flag = 0;
		int fp_config_first = 1;

		while ( NULL != fgets( szLine, sizeof( szLine ) - 1, fpOld ) )
		{
			if (board_type_U3210 == iDevType ||
				board_type_U3100 == iDevType ||
				board_type_Z8680 == iDevType ||
				board_type_Z8620 == iDevType)
			{
				if (search_lcp_flag)
				{
					if ((0 != strncmp(szLine, "create host-interface name v2k4cp", sizeof("create host-interface name v2k4cp")-1) && 0 == strncmp(szLine, "create host-interface name v2k", sizeof("create host-interface name v2k")-1))
						|| 0 == strncmp(szLine, "set interface mac address host-v2k 00:00:02:03:04:06", sizeof("set interface mac address host-v2k 00:00:02:03:04:06")-1)
						|| 0 == strncmp(szLine, "set interface state host-v2k up", sizeof("set interface state host-v2k up")-1)
						|| 0 == strncmp(szLine, "set interface ip address host-v2k 2.3.4.6/30", sizeof("set interface ip address host-v2k 2.3.4.6/30")-1)
						|| 0 == strncmp(szLine, "set ip neighbor host-v2k 2.3.4.5 00:00:02:03:04:05 static", sizeof("set ip neighbor host-v2k 2.3.4.5 00:00:02:03:04:05 static")-1)
						|| 0 == strncmp(szLine, "ip punt redirect add rx all via 2.3.4.5 host-v2k", sizeof("ip punt redirect add rx all via 2.3.4.5 host-v2k")-1)
						|| 0 == strncmp(szLine, "ip6 punt redirect add rx all via host-v2k", sizeof("ip6 punt redirect add rx all via host-v2k")-1))
					{
						iModify = 1;
						continue;
					}

					if (0 == strncmp(szLine, "lcp lcp-sync on",
								sizeof("lcp lcp-sync on")-1))
					{
						lcp_enable_found = 1;
					}
					else if (0 == strncmp(szLine, "comment {hylab pre-exec configure end}",
								sizeof("comment {hylab pre-exec configure end}")-1))
					{
						search_lcp_flag = 0;
						if (!lcp_enable_found)
						{
							fputs("lcp lcp-sync on\n", fpNew);
							fputs("lcp lcp-auto-subint on\n", fpNew);
							iModify = 1;
						}
					}
				}
				else if (0 == strncmp(szLine, "set interface state LCG", sizeof("set interface state LCG")-1)
					|| 0 == strncmp(szLine, "set interface state WCG", sizeof("set interface state WCG")-1)
					|| 0 == strncmp(szLine, "set interface state Agg", sizeof("set interface state Agg")-1))
				{
					if (!lcp_enable_found)
					{
						char bond_interface[32] = {0};
						if (1 == sscanf(szLine, "set interface state %[^ ]", bond_interface))
						{
							if (bond_interface[0])
							{
								fprintf( fpNew, "lcp create %s host-if %s\n" ,
									bond_interface, bond_interface);
								fputs(szLine, fpNew);
								continue;
							}
						}
					}
				}
				else if (0 == strncmp(szLine, "set interface l2 bridge loop",
					sizeof("set interface l2 bridge loop")-1))
				{
					if (!lcp_enable_found)
					{
						int bridge_num = atoi(szLine+sizeof("set interface l2 bridge loop")-1);
						fputs(szLine, fpNew);
						fprintf( fpNew, "lcp create loop%d host-if %s%d\n",
							bridge_num, 
	#ifdef CONFIG_HYTF_FW
							"Br",
	#else
							"Bridge",
	#endif
							bridge_num);
						continue;
					}
				}
				else if (0 == strncmp(szLine, "comment {Ip route config start}",
					sizeof("comment {Ip route config start}")-1))
				{
					flag = 1;
					continue;
				}
				else if (0 == strncmp(szLine, "comment {Ip route config end}",
					sizeof("comment {Ip route config end}")-1))
				{
					flag = 2;
					continue;
				}
				else if (1 == flag)
				{
					if (fp_config_first)
					{
						fpConfig = fopen(ACE_SYSTEM_CONFIG_FILE".tmp", "w+");
						if (!fpConfig)
						{
							init_coordinate_log( "open "ACE_SYSTEM_CONFIG_FILE".tmp error" );
							flag = 2;
							continue;
						}
						fputs("sleep 3\n", fpConfig);
						fp_config_first = 0;
					}
					iModify = 1;
					str_replace(szLine, fconfig, " preference ", " metric ");
					init_coordinate_log( "move cmd:%s", fconfig );
					fputs(fconfig, fpConfig);
					continue;
				}
			}

			// comment {hylab interface configure start}
			if ( 0 == strncmp( szLine, "comment {hylab interface configure start}", sizeof( "comment {hylab interface configure start}" ) - 1 ) )
			{
				iLcpCheck = 1;
			}
			// comment {hylab interface configure end}
			else if ( 0 == strncmp( szLine, "comment {hylab interface configure end}", sizeof( "comment {hylab interface configure end}" ) - 1 ) )
			{
				iLcpCheck = 2;

				// add new interface
				for ( i=0; i < bicConfig.m_iEthCount; i++ )
				{
					char szNewLine[1024] = {0};

					if ( bicConfig.m_arrEthConfig[i][0] && ( 0 == bicConfig.m_iEthExist[i] )  )
					{
						snprintf( szNewLine, sizeof( szNewLine ) - 1, "lcp create %s host-if %s\n", bicConfig.m_arrEthConfig[i], bicConfig.m_arrEthConfig[i] );

						fputs( szNewLine, fpNew );

						init_coordinate_log( "Info, new interface [%s] add, line [%s]", bicConfig.m_arrEthConfig[i], szNewLine );

						iModify = 1;
					}
				}
			}
			else if ( 1 == iLcpCheck )
			{
				if ( !strncmp( szLine, "lcp create ", strlen( "lcp create " ) ) )
				{
					char szEthInfo[64] = {0};
					char* pStart = szLine + strlen( "lcp create " );
					char* pEnd = strstr( szLine, " host-if" );

					if ( pEnd && ( pEnd - pStart > 0 ) && ( pEnd - pStart < sizeof( szEthInfo ) ) )
					{
						strncpy( szEthInfo, pStart, pEnd - pStart );

						for ( i=0; i < bicConfig.m_iEthCount; i++ )
						{
							if ( bicConfig.m_arrEthConfig[i][0] && ( 0 == bicConfig.m_iEthExist[i] )  )
							{
								if ( !strcmp( bicConfig.m_arrEthConfig[i], szEthInfo  ) )
								{
									bicConfig.m_iEthExist[i] = 1;

									break;
								}
							}
						}
					}
				}
			}

			init_coordinate_log( "reserve cmd:%s", szLine );

			fputs( szLine, fpNew );
		}
	} while( 0 );

	if ( fpOld )
	{
		fclose( fpOld ); fpOld = NULL;
	}

	if ( fpNew )
	{
		fclose( fpNew ); fpNew = NULL;
	}

	if ( iModify )
	{
		rename( HYLAB_VPP_CMD_FILE".tmp", HYLAB_VPP_CMD_FILE );

		if (fpConfig)
		{
			init_coordinate_log("megre ace_config_system.conf begin");

			FILE *fp_old = fopen(ACE_SYSTEM_CONFIG_FILE, "r");
			if (fp_old)
			{
				while (NULL != fgets(szLine, sizeof(szLine), fp_old))
				{
					if (strstr(szLine, " via "))
					{
						fprintf(fpConfig, "%s metric 255\n", trim(szLine));
						init_coordinate_log("change %s to %s metric 255",
							trim(szLine), trim(szLine));
					}
					else
					{
						fputs(szLine, fpConfig);
						init_coordinate_log("reserve %s",
							trim(szLine), trim(szLine));
					}
				}

				fclose(fp_old);
			}

			fclose(fpConfig);
			fpConfig = NULL;
			init_coordinate_log("megre ace_config_system.conf success");
			rename(ACE_SYSTEM_CONFIG_FILE".tmp", ACE_SYSTEM_CONFIG_FILE);
			init_coordinate_log("config migrate success");
		}

		init_coordinate_log( "config update success" );
	}
	else
	{
		remove( HYLAB_VPP_CMD_FILE".tmp" );
	}

	return 0;
}

int com_get_physical_port_num( int iDevType )
{
    FILE*   fp                            = NULL;
    char    buffer[512]  = { 0 };
    int     portnum                       = 0;

    fp = vpopen( "/sbin/ifconfig -a | grep Ethernet | grep ^eth | awk '{print$1}'", "r" );

    if ( !fp )
    {
        return 0;
    }

    while ( 0 != fgets(buffer, sizeof(buffer), fp ) )
    {
        portnum++;
    }

    vpclose( fp );

    return portnum;
}

char* com_find_first_mgt_port( U32* retIndex, int iDevType )
{
    int mgt_index = -1;
    U32 portIndex = 0;

    for ( portIndex = 0; portIndex < ACE_MAX_SUPPORT_PORT_BUSYBOX; portIndex++ )
    {
        if ( g_port_info[portIndex].isTrust != 2 )
            continue;

#ifndef CONFIG_HYTF_FW
		if ( strncmp( g_port_info[portIndex].newPhyname, "MGT", 3 ) && strncmp( g_port_info[portIndex].newPhyname, "MGMT", 4 ) )
			continue;
#endif

        if ( mgt_index == -1 )
            mgt_index = portIndex;
        else if ( strcmp( g_port_info[portIndex].newPhyname, g_port_info[mgt_index].newPhyname ) < 0 )
            mgt_index = portIndex;
    }

    if ( mgt_index != -1 )
    {
        *retIndex = mgt_index;

        return g_port_info[mgt_index].newPhyname;
    }

    return NULL;
}

char* com_find_orig_port(char *devname, int iDevType)
{
    U32 portIndex = 0;

    for ( portIndex = 0; portIndex < ACE_MAX_SUPPORT_PORT_BUSYBOX; portIndex++ )
    {
        if (!strcmp(g_port_info[portIndex].newPhyname, devname))
            return g_port_info[portIndex].oldPhyname;
    }

    return "";
}

char* com_get_mac_by_devname(char *devname, int iDevType)
{
	static char macaddress[64] = "00:11:22:33:44:55";
    FILE *fp;
    char buffer[512];
    int i;

    if (!strcmp(devname, ""))
    {
        return macaddress;
    }

    char command[512] = { 0 };
    snprintf(command, sizeof(command)-1, "/sbin/ifconfig -a|grep Ethernet|grep %s|awk \'{print$5}\'", devname); 
    fp = vpopen(command, "r");
    if ( !fp )
    {
        return macaddress;
    }

    if (0 != fgets(buffer, sizeof(buffer), fp))
    {
        for (i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
                break;
            macaddress[i] = buffer[i];
        }
    }
    vpclose( fp );

	return macaddress;
}

int com_def_hylab_vpp_cmd_conf_ac( int iDevType )
{
	char cmd[128] = { 0 };
	FILE *fp  = NULL;
	int cmd_idx;
	int portIndex, ret;
	int workmode = ACE_NETWORK_WORKMODE_DEFAULT;
	int ifd;
	int cfgExist;
	int trustIndex;
	char *mgt_dev = NULL;

	init_coordinate_log( "%s running\n", __FUNCTION__ );

	static const char* pre_exec_cmd[] = 
	{
		"create host-interface name v2k4cp",
		"set interface mac address host-v2k4cp 00:01:02:03:04:06",
		"set interface state host-v2k4cp up",
		"lcp lcp-sync on",
		"lcp lcp-auto-subint on"
	};

	init_coordinate_log( " before read workmode" );

	fp = fopen( ACE_WORK_MODE_FILE, "r" );

	if ( !fp || NULL == fgets( cmd, sizeof(cmd), fp ) || 2 != sscanf( cmd, " %s %d", cmd, &workmode ) )
	{
		sprintf( cmd, "echo \"system-work-mode	%d\" > %s", workmode, ACE_WORK_MODE_FILE );

		ret = system( cmd );

		if ( ret )
		{
			init_coordinate_log( "cmd %s result %d errno %d", cmd, ret, (ret>>8) );
		}
	}

	init_coordinate_log( " after read workmode %d" , workmode);

	if ( fp )
	{
		fclose( fp ); fp = NULL;
	}

	ifd = open( ACE_SYSTEM_CONFIG_FILE, O_RDONLY, 0644 );

	if ( -1 == ifd )
	{
		cfgExist = 0;

		mgt_dev = com_find_first_mgt_port( &trustIndex, iDevType );
	}
	else
	{
		cfgExist = 1;

		close( ifd );
	}

	init_coordinate_log( " cfgExist %d mgt_dev %s" , cfgExist, mgt_dev ? mgt_dev : "");

	fp = fopen( HYLAB_VPP_CMD_FILE, "w+" );
	if ( fp )
	{
		fprintf( fp, "%s\n", "comment {hylab vpp cmd config done}" );

		fprintf( fp, "%s\n", "comment {hylab pre-exec configure start}" );

		for ( cmd_idx = 0; cmd_idx < ( sizeof( pre_exec_cmd )/sizeof( pre_exec_cmd[0] ) ); cmd_idx++ )
		{
			fprintf( fp, "%s\n", pre_exec_cmd[cmd_idx] );
		}

		fprintf( fp, "%s\n\n", "comment {hylab pre-exec configure end}" );

		switch( 0xff & workmode )
		{
			case ACE_NETWORK_WORKMODE_ROUTE:
			case ACE_NETWORK_WORKMODE_BYPASS:
				// create lcp 
				fprintf( fp, "comment {hylab interface configure start}\n" );

				for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
				{
					if ( g_port_info[portIndex].isTrust == 2 )
						continue;

					fprintf( fp, "lcp create %s host-if %s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].newPhyname );
				}

				fprintf( fp, "comment {hylab interface configure end}\n\n" );

				if (!cfgExist && !mgt_dev)
				{
					char *default_ip = get_default_ip();

					fprintf( fp, "comment {Ip address config start}\n" );

					if (default_ip)
					{
						fprintf( fp, "set interface ip address LAN1 %s\n", default_ip );
						init_coordinate_log( " set interface ip address LAN1 %s", default_ip);
					}
					else
					{
						fprintf( fp, "set interface ip address LAN1 %s/%s\n", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK );
						init_coordinate_log( " set interface ip address LAN1 %s/%s",HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK);
					}

					fprintf( fp, "comment {Ip address config end}\n\n" );
				}
				break;
			default:
				fprintf( fp, "comment {Bridge1 configure start}\n" );
				if (!access("/usr/mysys/xconnect_bridge", F_OK))
				{
					fprintf( fp, "set interface l2 xconnect LAN1 WAN1\n" );
					fprintf( fp, "set interface l2 xconnect WAN1 LAN1\n" );
				}
				else
				{
					fprintf( fp, "set interface l2 bridge LAN1 1\n" );
					fprintf( fp, "set interface l2 bridge WAN1 1\n" );

					fprintf( fp, "set interface l2 pair LAN1 WAN1\n");
				}

				fprintf( fp, "create loopback interface mac %s instance 1\n", com_get_mac_by_devname( com_find_orig_port( "LAN1", iDevType), iDevType ) );
				fprintf( fp, "set interface l2 bridge loop1 1 bvi\n" );
				fprintf( fp, "set interface state loop1 up\n" );
				fprintf( fp, "comment {Bridge1 configure end}\n\n" );

				// create lcp
				fprintf( fp, "comment {hylab interface configure start}\n" );

				for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
				{
					if (g_port_info[portIndex].isTrust == 2)
						continue;

					fprintf( fp, "lcp create %s host-if %s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].newPhyname);
				}

				fprintf( fp, "lcp create loop1 host-if Bridge1\n" );

				fprintf( fp, "comment {hylab interface configure end}\n\n" );

				init_coordinate_log( " create Bridge1" );

				if ( !cfgExist && !mgt_dev )
				{
					char *default_ip = get_default_ip();

					fprintf( fp, "comment {Ip address config start}\n" );

					if (default_ip)
					{
						fprintf( fp, "set interface ip address loop1 %s\n", default_ip );
						init_coordinate_log( " set interface ip address Bridge1 %s", default_ip);
					}
					else
					{
						fprintf( fp, "set interface ip address loop1 %s/%s\n", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK );
						init_coordinate_log( " set interface ip address Bridge1 %s/%s",HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK);
					}

					fprintf( fp, "set interface state loop1 up\n" );

					fprintf( fp, "comment {Ip address config end}\n\n" );
				}
				break;
		}

		fflush( fp );

		fclose( fp ); fp = NULL;
	}

	if ( !cfgExist && mgt_dev )
	{
		fp = fopen( ACE_SYSTEM_CONFIG_FILE, "w+");

		if ( fp )
		{
			char *default_ip = get_default_ip();

			if (default_ip)
			{
				fprintf( fp, "ip addr add %s brd + dev %s\n", default_ip, mgt_dev );
				init_coordinate_log( " set interface ip address %s %s",mgt_dev, default_ip);
			}
			else
			{
				fprintf( fp, "ip addr add %s/%s brd + dev %s\n", HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK, mgt_dev );
				init_coordinate_log( " set interface ip address %s %s/%s",mgt_dev, HYLAB_DEFAULT_IP, HYLAB_DEFAULT_NETMASK);
			}

			fflush( fp );

			fclose( fp ); fp = NULL;
		}
	}

	return 0;
}

int com_get_eth_slot_domain( char* pEth , char* pOut, int iOut)
{
	int iRet = 0;
	FILE* fp = NULL;
	char szLine[256] = {0};
	char szEthInfo[128] = {0};

	if ( pOut && pEth && iOut > 0 )
	{
		snprintf( szEthInfo, sizeof( szEthInfo ) - 1, "/sys/class/net/%s/device/uevent", pEth );

		if ( ( fp = fopen( szEthInfo, "r" ) ) )
		{
			while( 0 != fgets( szLine, sizeof( szLine ) - 1, fp ) )
			{
				if ( 0 == strncmp( szLine, "PCI_SLOT_NAME=", strlen( "PCI_SLOT_NAME=" ) ) )
				{
					char* pSlot = szLine + strlen( "PCI_SLOT_NAME=" );
					snprintf( pOut, iOut - 1, "%s", pSlot);

					break;
				}

				memset( szLine, 0, sizeof( szLine ) );
			}

			fclose( fp );
		}
	}
	else
	{
		iRet = -1;
	}

	return iRet;
}

int com_get_eth_slot_dpdk_value( char* pEth , char* pOut, int iOut, int iFormat, int iDevType )
{
	int iRet = 0;
	FILE* fp = NULL;
	char szLine[256] = {0};
	char szEthInfo[128] = {0};

	if ( pOut && pEth && iOut > 0 )
	{
		snprintf( szEthInfo, sizeof( szEthInfo ) - 1, "/sys/class/net/%s/device/uevent", pEth );

		if ( ( fp = fopen( szEthInfo, "r" ) ) )
		{
			while( 0 != fgets( szLine, sizeof( szLine ) - 1, fp ) )
			{
				if ( 0 == strncmp( szLine, "PCI_SLOT_NAME=", strlen( "PCI_SLOT_NAME=" ) ) )
				{
					char* pSlot = szLine + strlen( "PCI_SLOT_NAME=" );

					char szDomain[16] = {0}, szVenvor[16] = {0}, szDev[16] = {0}, szFunc[16] = {0};

					sscanf( pSlot, "%[^:]%*[:]%[^:]%*[:]%[^.]%*[.]%s", szDomain + 2, szVenvor + 2, szDev + 2, szFunc + 2 );

					szDomain[0] = '0'; szDomain[1] = 'x';
					szVenvor[0] = '0'; szVenvor[1] = 'x';
					szDev[0] = '0'; szDev[1] = 'x';
					szFunc[0] = '0'; szFunc[1] = 'x';

					int iDomain = strtoul( szDomain, NULL, 16 );
					int iVendor = strtoul( szVenvor, NULL, 16 );
					int iDev = strtoul( szDev, NULL, 16 );
					int iFunc = strtoul( szFunc, NULL, 16 );

					snprintf( pOut, iOut - 1, "d%u/enp%d/s%d/f%d", iDomain, iVendor, iDev, iFunc );

					break;
				}

				memset( szLine, 0, sizeof( szLine ) );
			}

			fclose( fp );
		}
	}
	else
	{
		iRet = -1;
	}

	return iRet;
}

int com_get_eth_slot_dpdk_value2(struct x_eth_item *item, char* pOut, int iOut)
{
	int iRet = 0;

	char* pEth = item->oldname;
	com_get_eth_slot_dpdk_value(pEth, pOut, iOut, 0, 0);

	if(pOut[0] == 0){
		char *start = NULL;
		char *token = strstr(item->readlink, "/net/");
		if(token){
			*token = 0;
			start = strrchr(item->readlink, '/' );
			if(start){
				snprintf( pOut, iOut - 1, "%s", start+1);
			}
			*token = '/';
		}
	}
	return iRet;
}

void com_deal_port_map_config( unsigned int eth_total, int iDevType )
{
	FILE* fp = NULL;
	int portNum = 0;
	int ifindex = 0;
	int nmPortNum = 0;
	int iAutoMode = 0;
	int iUserDefine = 0;
	int portIndex = 0;
	int lanIndex = 0;
	int wanIndex = 0;
	char cmdBuf[128] = { 0 };
	char szLine[256] = { 0 };
	char tmpstr[256] = { 0 };
	char oldname[32]  = { 0 };
	char newname[32]  = { 0 };
	char property[32] = { 0 };
	char noIgnore[32] = { 0 };
	char band[32]     = { 0 };

	init_coordinate_log( "%s running\n", __FUNCTION__ );

	g_portNum = 0; g_iMgtNum = 0;

	memset( g_port_info, 0, sizeof( g_port_info ) );

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( ( !fp ) 
	  || ( fgets( szLine, sizeof( szLine ), fp ) == 0 ) 
	  || ( sscanf( szLine, "%s %d", cmdBuf, &iAutoMode ) != 2 ) 
	  || ( iAutoMode ) )
	{
		init_coordinate_log( "Warning, auto mode config" );

		if ( NULL != fp )
		{
			fgets( szLine, sizeof( szLine ), fp );

			if ( strstr( szLine, "####ManMakeExtMap" ) )
			{
				sscanf( szLine, "%s %d", tmpstr, &iUserDefine );
			}
		}

		if ( !iUserDefine )
		{
			goto PORT_CFG;
		}
		else
		{
			init_coordinate_log( "Warning, use use defined config" );
		}
	}

	portNum = 0;

	while ( 0 != fgets( szLine, sizeof( szLine ), fp ) )
	{
		if ( ( '#' == szLine[0] ) || ( '\n' == szLine[0] ) )
		{
			init_coordinate_log( " skip comment line [%s]", szLine );

			continue;
		}

		memset(oldname, 0, sizeof(oldname));
		memset(newname, 0, sizeof(newname));
		memset(property, 0, sizeof(property));
		memset(noIgnore, 0, sizeof(noIgnore));

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
		if ( sscanf( szLine, "%s %s %s %s", oldname, newname, noIgnore, band ) != 4 )
#else
		if ( sscanf( szLine, "%s %s %s", oldname, newname, noIgnore ) != 3 )
#endif
#else
		if ( sscanf( szLine, "%s %s %d %s %s", oldname, newname, &ifindex, property, noIgnore ) != 5 )
#endif
		{
			init_coordinate_log( " invalid line %s", szLine );

			continue;
		}

		if ( memcmp( oldname, "eth", strlen( "eth" ) ) )
		{
			init_coordinate_log( "warning, unknow old eth [%s], line [%s]", oldname, szLine );

			continue;
		}

#ifndef CONFIG_HYTF_FW
		if ( strcmp( property, "TRUST" ) && strcmp( property, "UNTRUST" ) && strcmp( property, "MANAGE" ) )
		{
			init_coordinate_log("warning, unknow property [%s], line [%s]", property, szLine);

			continue;
		}
#endif

		if ( ( memcmp( noIgnore, "no-ignore", strlen( "no-ignore" ) ) ) )
		{
			init_coordinate_log("warning, unknow noIgnore [%s], line [%s]", noIgnore, szLine);

			continue;
		}

		g_port_info[portNum].ethPciId[0] = 0;
		com_get_eth_slot_dpdk_value( oldname, g_port_info[portNum].ethPciId, sizeof( g_port_info[portNum].ethPciId ), 0, iDevType );

		strcpy( g_port_info[portNum].oldPhyname, oldname );

		strcpy( g_port_info[portNum].newPhyname, newname );

		g_port_info[portNum].ifIndex = ifindex;

		g_port_info[portNum].isTrust = 0;

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
		if ( 'o' == band[0] )
#else
		if ( 'M' == newname[0] )
#endif
		{
			g_port_info[portNum].isTrust = 2;

			nmPortNum++;
		}
#else
		if ( !strcmp( property, "TRUST" ) )
		{
			g_port_info[portNum].isTrust = 1;
		}

		if ( !strcmp( property, "MANAGE" ) )
		{
			g_port_info[portNum].isTrust = 2;

			nmPortNum++;
		}
#endif

		portNum++;

		if ( portNum >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
		{
			break;
		}
	}

	if ( fp )
	{
		fclose( fp ); fp = NULL;
	}

	if ( !portNum )
	{
		goto PORT_CFG;
	}

	g_portNum = portNum;

	g_iMgtNum = nmPortNum;

	init_coordinate_log( "g_portNum %d nmPortNum %d", g_portNum, g_iMgtNum );

	return;

PORT_CFG:
	if ( fp )
	{
		fclose( fp ); fp = NULL;
	}

	g_portNum = eth_total;

	for ( portIndex = 0; ( portIndex < g_portNum ) && ( portIndex < ACE_MAX_SUPPORT_PORT_BUSYBOX ); portIndex++ )
	{
		sprintf( g_port_info[portIndex].oldPhyname, "eth%d", portIndex );

		g_port_info[portIndex].ethPciId[0] = 0;
		com_get_eth_slot_dpdk_value( g_port_info[portIndex].oldPhyname, g_port_info[portNum].ethPciId, sizeof( g_port_info[portNum].ethPciId ), 0, iDevType );

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
		sprintf( g_port_info[portIndex].newPhyname, "GE0-%d", portIndex );
#else
		sprintf( g_port_info[portIndex].newPhyname, "eth%d", portIndex );
#endif
#else
		if ( !( portIndex % 2 ) )
		{
			lanIndex++;
			sprintf( g_port_info[portIndex].newPhyname, "%s%d", PHY_TRUST_NAME, lanIndex );
			g_port_info[portIndex].ifIndex = lanIndex;
			g_port_info[portIndex].isTrust = 1;
		}
		else
		{
			wanIndex++;
			sprintf( g_port_info[portIndex].newPhyname, "%s%d", PHY_UNTRUST_NAME, wanIndex );
			g_port_info[portIndex].ifIndex = wanIndex;
			g_port_info[portIndex].isTrust = 0;
		}
#endif
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( g_portNum && fp )
	{
		sprintf( cmdBuf, "ifAutoCfgMode %d\n", 1 );
		fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
		{
#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
			sprintf( cmdBuf, "%s %s no-ignore inband\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname );
#else
			sprintf( cmdBuf, "%s %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname );
#endif
#else
			sprintf( cmdBuf, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname,
				g_port_info[portIndex].ifIndex, g_port_info[portIndex].isTrust ? "TRUST" : "UNTRUST" );
#endif
			fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
		}
	}
	else
	{
		init_coordinate_log( "warning, eth num is 0 or open file [%s] failed.", ACE_PORT_MAP_FILE );
	}

	if ( fp )
	{
		fclose( fp ); fp = NULL;
	}

	return;
}

int com_get_vpp_eth_config_info( char pOut[128][64], int* pRLine, int iDevType )
{
	char szVppLine[64] = {0};

	if ( pOut && pRLine )
	{
		*pRLine = 0;

		FILE* fVpp = fopen( ACE_VPP_PORT_MAP_FILE, "r" );

		if ( fVpp )
		{
			while ( fgets( szVppLine, sizeof( szVppLine ) - 1, fVpp ) != NULL  )
			{
				if ( strstr( szVppLine, "ifAutoCfgMode" ) )
				{
					continue;
				}

				if ( szVppLine[strlen( szVppLine ) - 1] == '\n' )
				{
					szVppLine[strlen( szVppLine ) - 1] = 0;
				}

				if ( *pRLine < 128 )
				{
					snprintf( pOut[*pRLine], 64 - 1, "%s", szVppLine ); (*pRLine)++;
				}
			}

			fclose( fVpp );
		}
		else
		{
			return -2;
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

void com_check_vpp_interface( int iDevType )
{
	init_coordinate_log( "%s running\n", __FUNCTION__ );

	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int i = 0;
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = com_get_vpp_eth_config_info( szVppEthCache, &iVppEth, iDevType );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( ( g_portNum - g_iMgtNum ) != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "warning, the ext card positon maybe changed 1, %d:%d, delete [%s].", g_portNum - g_iMgtNum, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( ( g_portNum - g_iMgtNum ) == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( i = 0; i < g_portNum && !iEnd; ++i )
				{
					if ( 's' != g_port_info[i].newPhyname[0] && 'M' != g_port_info[i].newPhyname[0] && 'H' != g_port_info[i].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
						snprintf( szLine, sizeof( szLine ) - 1, "%s %s no-ignore inband", g_port_info[i].ethPciId, g_port_info[i].newPhyname );
#else
						snprintf( szLine, sizeof( szLine ) - 1, "%s %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname );
#endif
#else
						if ( ( 'L' == g_port_info[i].newPhyname[0] ) || ( 'W' == g_port_info[i].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[i].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 1;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname, iPair,
							( com_rename_attribute_name[iEthAttribute] ) );
#endif

						init_coordinate_log( "Info, new vpp eth info [%s].", szLine );

						for ( x = 0; x < iVppEth; x++ )
						{
							if ( !strcmp( szLine, szVppEthCache[x] ) )
							{
								iFind = 1;

								break;
							}
						}

						if ( 0 == iFind )
						{
							iEnd = 1;

							unlink( ACE_VPP_PORT_MAP_FILE );

							init_coordinate_log( "warning, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	return;
}

int com_is_user_define_interface( int iDevType )
{
	int iManInter = 0;
	char szTemp[128] = {0};
	char szLine[256] = {0};

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
	{
		init_coordinate_log( "%s, file [/usr/mysys/skip_auto_interface] exist\n", __FUNCTION__ );

		return 1;
	}

	FILE* fp = fopen( ACE_PORT_MAP_FILE, "r" );

	if ( fp )
	{
		fgets( szLine, sizeof( szLine ), fp );

		fgets( szLine, sizeof( szLine ), fp );

		fclose( fp ); fp = NULL;

		if ( strstr( szLine, "####ManMakeExtMap") )
		{
			if ( 2 != sscanf( szLine, "%s %d", szTemp, &iManInter ) )
			{
				iManInter = 0;
			}
		}
	}

	return iManInter;
}

void write_slot_card_conf(board_card_info info[], int num)
{
	FILE* fpSlot = fopen( "/home/slot_card.conf", "w+" );
	if ( fpSlot )
	{
		int i;
		for (i=0; i< num; i++){
			if ( info[i].online ){
				fprintf( fpSlot, "slot%d:%s,1\n", info[i].slot?(info[i].slot):(i+1), info[i].card_name);
			}
			else{
				fprintf( fpSlot, "slot%d:,\n", info[i].slot?(info[i].slot):(i+1));
			}
		}

		fclose( fpSlot ); fpSlot = NULL;
	}
}

int com_get_work_mode()
{
	char szLine[256] = { 0 };
	int workmode = ACE_NETWORK_WORKMODE_DEFAULT;

	FILE* fp = fopen( ACE_WORK_MODE_FILE, "r" );

	if ( !fp || NULL == fgets( szLine, sizeof( szLine ), fp ) || 2 != sscanf( szLine, " %s %d", szLine, &workmode ) )
	{
		sprintf( szLine, "echo \"system-work-mode  %d\" > %s", workmode, ACE_WORK_MODE_FILE );

		int ret = system( szLine );

		if ( ret )
		{
			init_coordinate_log( "cmd %s result %d errno %d", szLine, ret, ( ret >> 8 ) );
		}
	}

	return workmode;
}

char* com_get_orig_port(char *devname)
{
	int i = 0, j = 0;
	U32 portIndex = 0;
	struct x_dev_struct *x_dev = &g_x_dev;

	for ( i = 0; i < DEV_CARD_MAX; i++ )
	{
		struct x_card_struct *item = &( x_dev->card_item[i] );

		if ( item->conf == NULL )
			continue;

		for ( j = 0; j < item->num && j < CARD_ETH_MAX; j++ )
		{
			if ( !strcmp( item->eth_item[j].newname, devname ) )
			{
				return item->eth_item[j].oldname;
			}
		}
	}

	return "";
}

char* com_get_dev_mac(char *devname)
{
	static char macaddress[64] = "00:11:22:33:44:55";
    FILE *fp;
    char buffer[512];
    int i;

    if (!strcmp(devname, ""))
    {
        return macaddress;
    }

    char command[512] = { 0 };
    snprintf(command, sizeof(command)-1, "/sbin/ifconfig -a|grep Ethernet|grep %s|awk \'{print$5}\'", devname); 
    fp = vpopen(command, "r");
    if ( !fp )
    {
        return macaddress;
    }

    if (0 != fgets(buffer, sizeof(buffer), fp))
    {
        for (i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
                break;

            macaddress[i] = buffer[i];
        }
    }

    vpclose( fp );

	return macaddress;
}

void get_mac_from_kernel(char *ifname, char *mac, int len)
{
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };

	snprintf(line, sizeof(line), "/sys/class/net/%s/address", ifname);
	fp = fopen( line, "r" );
	if ( fp ) {
		
		fgets( mac, len, fp );
		
		my_str_trim(mac);
		fclose( fp );
	}
}

void com_generate_linux2vpp(void)
{
	int num = g_portNum;
	ACE_PORT_INFO_ST *apis = g_port_info;
	FILE*   fp                          = NULL;
	char    tmpstr[256]                 = { 0 };
	int i, j;
	const char *head = "ifAutoCfgMode 0\n";

	unlink(ACE_VPP_PORT_MAP_FILE);
	fp = fopen( ACE_VPP_PORT_MAP_FILE, "w+" );
	if ( fp )
	{
		fwrite( head, strlen( head ), sizeof( char ), fp );

		for(i=0; i<num&&i<ACE_MAX_SUPPORT_PORT_BUSYBOX; i++){
			if(!strstr(apis[i].newPhyname,"HA") && !strstr(apis[i].newPhyname,"MG") 
				&& apis[i].ethPciId[0] && apis[i].isTrust <2){
				memset(tmpstr, 0, sizeof(tmpstr));
				
#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
				snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore inband\n", apis[i].ethPciId, apis[i].newPhyname );
#else
				snprintf( tmpstr, sizeof(tmpstr), "%s %s no-ignore\n", apis[i].ethPciId, apis[i].newPhyname);
#endif
#else
				snprintf( tmpstr, sizeof(tmpstr), "%s %s %d %s no-ignore\n", apis[i].ethPciId, apis[i].newPhyname,
						atoi(apis[i].newPhyname+3), apis[i].newPhyname[0]=='L'?"TRUST":"UNTRUST");
#endif	
				if(tmpstr[0]){
					init_coordinate_log("%s", tmpstr);
					fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
				}
			}
		}

		fclose( fp ); fp = NULL;
	}
}

#if 1
#define FILE_NUMA_NODE "/sys/class/net/%s/device/numa_node"
#define FILE_STARTUP   "/usr/local/vpp/etc/vpp/startup.conf"
#define HANDLE_STARTUP "/home/handle_startup_conf"

#define INTERFACE_NUM_MAX  64
static struct x_startup_conf g_xstartup_conf =
{
	STARTUP_DRIVER_AUTO,
		
	{			
		{STARTUP_MEMORY_128G, STARTUP_RX_DESC_4096},
		{STARTUP_MEMORY_64G,  STARTUP_RX_DESC_4096},
		{STARTUP_MEMORY_32G,  STARTUP_RX_DESC_4096},	
		{STARTUP_MEMORY_16G,  STARTUP_RX_DESC_4096},
		{STARTUP_MEMORY_8G,   STARTUP_RX_DESC_2048},
		{STARTUP_MEMORY_4G,   STARTUP_RX_DESC_1024},
		{STARTUP_MEMORY_2G,   STARTUP_RX_DESC_512 },	
		{0, 0}	
	},

	{			
		{STARTUP_CPU_CORES_64,  STARTUP_RX_QUEUES_16},
		{STARTUP_CPU_CORES_32,  STARTUP_RX_QUEUES_16},
		{STARTUP_CPU_CORES_24,  STARTUP_RX_QUEUES_16},	
		{STARTUP_CPU_CORES_16,  STARTUP_RX_QUEUES_8},
		{STARTUP_CPU_CORES_8,   STARTUP_RX_QUEUES_4},
		{STARTUP_CPU_CORES_4,   STARTUP_RX_QUEUES_2},
		{STARTUP_CPU_CORES_2,   STARTUP_RX_QUEUES_1},	
		{0, 0}		
	},

	{16, 0, 0, 0}
};

static void get_vpp_cpus(char *isol_str, unsigned char cpus[], int MAX_CPUS)
{
    char value[256] = {0};
    char *p = value;

	my_str_trim(isol_str);
	//memset(cpus, 0, MAX_CPUS);
	
    while (*isol_str && !isspace(*isol_str)) {
        *p++ = *isol_str++;
    }
    *p = '\0';
    
    //printf("Found isolcpus value: %s\n", value);
    
    char *token = strtok(value, ",");
    while (token) {
        // (e.g., "1-3")
        char *dash = strchr(token, '-');
        if (dash) {
            *dash = '\0';
            int start = atoi(token);
            int end = atoi(dash + 1);
            
            if (start >= 0 && end >= start && end < MAX_CPUS) {
                for (int i = start; i <= end; i++) {
                    cpus[i] = 1;
                }
            } else {
                init_coordinate_log("Invalid range: %s\n", token);
            }
        } 
        // single CPU (e.g., "5")
        else {
            int cpu = atoi(token);
            if (cpu >= 0 && cpu < MAX_CPUS) {
                cpus[cpu] = 1;
            } else {
                init_coordinate_log("Invalid CPU: %s\n", token);
            }
        }
        
        token = strtok(NULL, ",");
    }

}

static int get_cpu_num(void)
{
	unsigned int cpu_num = sysconf(_SC_NPROCESSORS_CONF);

	if (cpu_num > 128){
		cpu_num = 128;
	}

	return cpu_num;
}

static int get_mem_num(void)
{
	char buf[128];
	FILE *fp = fopen("/proc/meminfo", "r");
	int total = 4*1024*1024;///kB
	int mem = 2;//2G

    if (fp) {
        fscanf(fp, "MemTotal: %lu %s\n", &total, buf);
        fclose(fp);
    }	

	if(total > 120*1024*1024){
		mem = STARTUP_MEMORY_128G;
	}
	else if(total > 60*1024*1024){
		mem = STARTUP_MEMORY_64G;
	}
	else if(total > 30*1024*1024){
		mem = STARTUP_MEMORY_32G;
	}
	else if(total > 14*1024*1024){
		mem = STARTUP_MEMORY_16G;
	}
	else if(total > 7*1024*1024){
		mem = STARTUP_MEMORY_8G;
	}
	else if(total > 3*1024*1024){
		mem = STARTUP_MEMORY_4G;
	}
	else{
		mem = STARTUP_MEMORY_2G;
	}

	return mem;
}

static int ace_parse_sysfs_value(const char *filename, unsigned long *val)
{
	FILE *f;
	char buf[BUFSIZ];
	char *end = NULL;

	if ((f = fopen(filename, "r")) == NULL) {
		init_coordinate_log("%s(): cannot open sysfs value %s\n",
			__func__, filename);
		return -1;
	}

	if (fgets(buf, sizeof(buf), f) == NULL) {
		init_coordinate_log("%s(): cannot read sysfs value %s\n",
			__func__, filename);
		fclose(f);
		return -1;
	}
	*val = strtoul(buf, &end, 0);
	if ((buf[0] == '\0') || (end == NULL) || (*end != '\n')) {
		init_coordinate_log("%s(): cannot parse sysfs value %s\n",
				__func__, filename);
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

static int get_cpu_params(struct x_startup_conf *conf, struct x_startup_param *param)
{
	FILE* fp;
	int found = 0;
	char *token = NULL;
	char line[1024] = {0};
	int cpu_num = 0;
	unsigned char grub_cpus[MAX_CPUS_NUM];
	int i;
	const char *cmd1      = "cat /proc/cmdline";

	memset(grub_cpus, 0, sizeof(grub_cpus));

	/* cat /proc/cmdline */
	fp = popen( cmd1, "r" );
	if(fp){
		if (fgets(line, sizeof(line), fp) != NULL) {
			token = strstr(line, " isolcpus=");
			if (token){
				char tmp[128] = {0};
				char *p1 = token+strlen(" isolcpus=");
				char *p2 = strchr(p1, ' ');
				if(p2){
					strncpy(tmp, p1, (p2-p1)>128?128:(p2-p1));
				}
				else{
					strncpy(tmp, p1, sizeof(tmp));
				}
				
				p1=strchr(tmp, ',');
				if(p1){
					*p1 = '\0';
					p2=strchr(tmp, '-');
					if(p2){
						*p1 = ',';
						int start = atoi(tmp);
						int end = atoi(p2+1);
						param->main_core = start;
						if(end - start >1){
							snprintf(param->corelist_workers, sizeof(param->corelist_workers),
							"%d%s", start+1, p2);
						}
						else{
							snprintf(param->corelist_workers, sizeof(param->corelist_workers),
							"%d,%s", end, p1+1);							
						}
					}
					else{
						int start = atoi(tmp);
						param->main_core = start;
						snprintf(param->corelist_workers, sizeof(param->corelist_workers),
							"%s", p1+1);
					}
				}
				else{		
					p2=strchr(tmp, '-');
					if(p2){	
						int start = atoi(tmp);
						int end = atoi(p2+1);
						param->main_core = start;
						if(end - start >1){
							snprintf(param->corelist_workers, sizeof(param->corelist_workers),
							"%d-%d", start+1, end);
						}
						else{
							snprintf(param->corelist_workers, sizeof(param->corelist_workers),
							"%d", end);							
						}
					}
					else{
						param->main_core = 0;
						snprintf(param->corelist_workers, sizeof(param->corelist_workers),
						"%s", tmp);						
					}
				}
				
				found = 1;
				get_vpp_cpus(token+strlen(" isolcpus="), grub_cpus, MAX_CPUS_NUM);
				for(i=0, cpu_num=0; i<MAX_CPUS_NUM; i++){
					if(grub_cpus[i]){
						cpu_num++;
					}
				}
				if(cpu_num==1){/* isolcpus=1 */
					cpu_num = 2;
				}
					
				if(cpu_num >=17){
					param->num_rx_queues = STARTUP_RX_QUEUES_16;
				}
				else if(cpu_num >=9){
					param->num_rx_queues = STARTUP_RX_QUEUES_8;
				}
				else if(cpu_num >=5){
					param->num_rx_queues = STARTUP_RX_QUEUES_4;
				}
				else if(cpu_num >=3){
					param->num_rx_queues = STARTUP_RX_QUEUES_2;
				}
				else{
					param->num_rx_queues = STARTUP_RX_QUEUES_1;
				}
			}						
		}

		pclose(fp); 	
	}

	int num = get_cpu_num();
	if(found == 0){
		if(num >= STARTUP_CPU_CORES_24){
			if(!cpu_num)
				cpu_num = 17;
			param->main_core = 3;
			snprintf(param->corelist_workers, sizeof(param->corelist_workers),"4-19");	
		}
		else if(num >= STARTUP_CPU_CORES_16){
			if(!cpu_num)
				cpu_num = 9;
			param->main_core = 3;
			snprintf(param->corelist_workers, sizeof(param->corelist_workers),"4-11");	
		}
		else if(num >= STARTUP_CPU_CORES_8){
			if(!cpu_num)
				cpu_num = 5;			
			param->main_core = 1;
			snprintf(param->corelist_workers, sizeof(param->corelist_workers),"2-5");	
		}
		else if(num >= STARTUP_CPU_CORES_4){
			if(!cpu_num)
				cpu_num = 3;			
			param->main_core = 1;
			snprintf(param->corelist_workers, sizeof(param->corelist_workers),"2-3");	
		}
		else{
			if(!cpu_num)
				cpu_num = 2;			
			param->main_core = 0;
			snprintf(param->corelist_workers, sizeof(param->corelist_workers),"1");	
		}

		for(i=0; conf->cpu_params[i].cpu &&i<STARTUP_STANDARD_NUM; i++){
			if(num >= conf->cpu_params[i].cpu){
				param->num_rx_queues = conf->cpu_params[i].rx_queues;
				break;
			}		
		}		
	}

	if(0 == strncmp(param->corelist_workers, "2-3", sizeof("2-3"))){
		snprintf(param->corelist_workers, sizeof(param->corelist_workers),"2,3");	
	}
	param->cpus = cpu_num;
	return 0;
}

static int get_mem_params(struct x_startup_conf *conf, struct x_startup_param *param)
{
	int i;
	int num = get_mem_num();

	if(num >= STARTUP_MEMORY_128G){
		snprintf(param->main_heap_size, sizeof(param->main_heap_size),"20G");
	}
	else if(num >= STARTUP_MEMORY_64G){
		snprintf(param->main_heap_size, sizeof(param->main_heap_size),"12G");
	}
	else if(num >= STARTUP_MEMORY_4G){
		snprintf(param->main_heap_size, sizeof(param->main_heap_size),"%dG", num/4);
	}
	else{
		snprintf(param->main_heap_size, sizeof(param->main_heap_size),"500M");
	}

	param->num_rx_desc = STARTUP_RX_DESC_512;
	for(i=0; conf->mem_params[i].mem &&i<STARTUP_STANDARD_NUM; i++){
		if(num >= conf->mem_params[i].mem){
			param->num_rx_desc =  conf->mem_params[i].rx_desc;
			
			break;
		}			
	}	
	return 0;
}

static int check_conflict_cpus(char *line, const char *program, unsigned char src_cpus[], int MAX_CPUS)
{
	int i;
	unsigned char tmp_cpus[MAX_CPUS];
	char tmp[256];
	char *end;
	char *token;
	snprintf(tmp, sizeof(tmp), "%s\":\"", program);
	token = strstr(line, tmp);
	if (token){
		token += strlen(tmp);
		if((end=strchr(token, '\"'))){
			*end = '\0';
			
			memset(tmp_cpus, 0, sizeof(tmp_cpus));
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), "%s", token);
			*end = '\"';

			get_vpp_cpus(tmp, tmp_cpus, MAX_CPUS);
			for(i=0; i<MAX_CPUS; i++){
				if(src_cpus[i] && tmp_cpus[i]){
					return 1;
				}
			}
		}
	}
		
	return 0;
}

static int check_vpp_runtime_params(void)
{
#define MAX_CPUS_NUM 128
	FILE* fp;
	int i;
	int found = 0;
	char line[1024] = {0};
	int cpu_num = 0;
	char *token = NULL;
	int num_rx_queues = -1;
	int max_rx_threads = -1;
	unsigned char startup_cpus[MAX_CPUS_NUM];
	const char *filename1 = "/usr/local/vpp/etc/vpp/startup.conf";
	const char *filename2 = "/home/sys_param.conf";
	const char *filename3 = "/home/sys_cpus.conf";

	memset(startup_cpus, 0, sizeof(startup_cpus));

	/* /usr/local/vpp/etc/vpp/startup.conf */
	found = 0;
	fp = fopen( filename1, "r" );
	if(fp){
		while (fgets(line, sizeof(line), fp) != NULL) {
			
			my_str_trim(line);
			
			if(line[0] == '#' || line[0] == '\r' || line[0] == '\n')
				continue;
			
			if ((token = strstr(line, "main-core"))){
				found++;
				token += strlen("main-core");
				get_vpp_cpus(token, startup_cpus, MAX_CPUS_NUM);
			}
			else if ((token = strstr(line, "corelist-workers"))){
				found++;
				token += strlen("corelist-workers");
				get_vpp_cpus(token, startup_cpus, MAX_CPUS_NUM);
			}	
			else if ((token = strstr(line, "num-rx-queues"))){
				found++;
				token += strlen("num-rx-queues");
				my_str_trim(token);
				num_rx_queues = atoi(token);
			}	
		}

		fclose(fp); 	
	}

	/* /home/sys_param.conf */
	found = 0;
	fp = fopen( filename2, "r" );
	if(fp){
		if (fgets(line, sizeof(line), fp) != NULL) {
			token = strstr(line, "max_rx_threads\":\"");
			if (token){
				found = 1;
				max_rx_threads = atoi(token+strlen("max_rx_threads\":\""));
			}	
		}

		fclose(fp); 	
	}
	if(0 == found){
		remove(filename2);
		init_coordinate_log("sys_param.conf max_rx_threads not found!\n");
		//return -1;
	}

	for(i=0, cpu_num=0; i<MAX_CPUS_NUM; i++){
		if(startup_cpus[i]){
			cpu_num++;
		}
	}
	if(cpu_num != (max_rx_threads+1)){
		remove(filename2);
		init_coordinate_log("max_rx_threads error!%d:%d\n", cpu_num, (max_rx_threads+1));
		//return -1;
	}

	/* /home/sys_cpus.conf */
	fp = fopen( filename3, "r" );
	if(fp){
		if (fgets(line, sizeof(line), fp) != NULL) {

			if(check_conflict_cpus(line, "init", startup_cpus, MAX_CPUS_NUM)){
				init_coordinate_log(" conflict cpus error!\n");
				remove(filename3);
				fclose(fp);
				return -1;
			}

			if(check_conflict_cpus(line, "cgi", startup_cpus, MAX_CPUS_NUM)){
				init_coordinate_log(" conflict cpus error!\n");
				remove(filename3);
				fclose(fp);
				return -1;
			}

			if(check_conflict_cpus(line, "ace_userspace", startup_cpus, MAX_CPUS_NUM)){
				init_coordinate_log(" conflict cpus error!\n");
				remove(filename3);
				fclose(fp);
				return -1;
			}

			if(check_conflict_cpus(line, "Collector", startup_cpus, MAX_CPUS_NUM)){
				init_coordinate_log(" conflict cpus error!\n");
				remove(filename3);
				fclose(fp);
				return -1;
			}
			
			if(check_conflict_cpus(line, "bypass", startup_cpus, MAX_CPUS_NUM)){
				init_coordinate_log(" conflict cpus error!\n");		
				remove(filename3);
				fclose(fp);
				return -1;
			}				
		}

		fclose(fp); 	
	}	
	
	return 0;
}

static int get_interface_params(struct x_startup_conf *conf, struct x_startup_param *param)
{
	int i;
	int n = 0;
	int num = g_portNum;
	ACE_PORT_INFO_ST *apis = g_port_info;
	char filename[128];
	unsigned long tmp;
	int numa_num = 0;
	int mem = get_mem_num();
	init_coordinate_log("g_portNum=%d\n", num);
	if(num){
		memset(conf->interfaces_numa, 0, sizeof(conf->interfaces_numa));
	}

	for(i=0; i<num&&i<INTERFACE_NUM_MAX; i++){
		if(apis[i].isTrust < 2){
			if(apis[i].oldPhyname[0]){
				int numa_node = -1;
				snprintf(filename, sizeof(filename), FILE_NUMA_NODE, apis[i].oldPhyname);
				init_coordinate_log("filename=%s\n", filename);
				if (access(filename, F_OK) == 0 &&
				    ace_parse_sysfs_value(filename, &tmp) == 0){
					numa_node = tmp;
				}

				if(numa_node>=0 && numa_node<STARTUP_NUMA_MAX){
					conf->interfaces_numa[numa_node]++;
				}
				else{
					conf->interfaces_numa[0]++;
				}
			}
		}
	}

	for(i=0; i<STARTUP_NUMA_MAX; i++){
		if(conf->interfaces_numa[i])
			numa_num++;		
	}

	if(!numa_num)
		numa_num = 1;
	
	for(i=0; i<STARTUP_NUMA_MAX; i++){
		int extra;
		if(conf->interfaces_numa[i] == 0)
			continue;

		if(mem >= STARTUP_MEMORY_16G){
			extra = (param->cpus)?(param->cpus*MAX_VPP_NODE*256):100000;
			param->buffers_numa[i] = conf->interfaces_numa[i] * param->num_rx_desc * param->num_rx_queues * 2 + extra/numa_num;
		}
		else if(mem >= STARTUP_MEMORY_8G){
			extra = (param->cpus)?(param->cpus*MAX_VPP_NODE*256):65536;
			param->buffers_numa[i] = conf->interfaces_numa[i] * param->num_rx_desc * param->num_rx_queues * 2 + extra/numa_num;
		}
		else if(mem >= STARTUP_MEMORY_4G){
			extra = (param->cpus)?(param->cpus*MAX_VPP_NODE*256):16384;
			param->buffers_numa[i] = conf->interfaces_numa[i] * param->num_rx_desc * param->num_rx_queues * 2 + extra/numa_num;
		}
		else{
			param->buffers_numa[i] = conf->interfaces_numa[i] * param->num_rx_desc * param->num_rx_queues * 2 + 4096/numa_num;
		}

	}

	if(param->buffers_numa[0] == 0){
		param->buffers_numa[0] = 8192;
	}	
	return 0;
}

int handle_startup_conf(struct x_startup_conf *xsc)
{
	int i, j;
	struct x_startup_param param;
	struct x_startup_conf conf;

	memset(&param, 0, sizeof(param));
	memcpy(&conf, &g_xstartup_conf, sizeof(conf));

	if(0 != access(HANDLE_STARTUP, F_OK)){
		return 0;
	}

	if(xsc){
		if(xsc->driver){
			conf.driver = xsc->driver;
		}

		if(xsc->interfaces_numa[0]){
			memcpy(conf.interfaces_numa, xsc->interfaces_numa, sizeof(xsc->interfaces_numa));
		}

		for(i=0; xsc->mem_params[i].mem &&i<STARTUP_STANDARD_NUM; i++){
			for(j=0; conf.mem_params[j].mem && j<STARTUP_STANDARD_NUM; j++){
				if(conf.mem_params[j].mem == xsc->mem_params[i].mem){
					conf.mem_params[j].rx_desc = xsc->mem_params[i].rx_desc;
					break;
				}
			}			
		}

		for(i=0; xsc->cpu_params[i].cpu &&i<STARTUP_STANDARD_NUM; i++){
			for(j=0; conf.cpu_params[j].cpu && j<STARTUP_STANDARD_NUM; j++){
				if(conf.cpu_params[j].cpu == xsc->cpu_params[i].cpu){
					conf.cpu_params[j].rx_queues = xsc->cpu_params[i].rx_queues;
					break;
				}
			}			
		}		
	}

	param.driver = conf.driver;
	
	get_cpu_params(&conf, &param);
	
	get_mem_params(&conf, &param);

	get_interface_params(&conf, &param);

	init_coordinate_log("%s(): (main_heap_size=%s,main_core=%d,corelist_workers=%s,cpus=%d,"
		"num_rx_queues=%d,num_rx_desc=%d,buffers_numa[0]=%d,buffers_numa[1]=%d,"
		"interfaces_numa[0]=%d,interfaces_numa[1]=%d\n",
			__func__, param.main_heap_size,param.main_core,param.corelist_workers,param.cpus,
			 param.num_rx_queues,  param.num_rx_desc, param.buffers_numa[0], param.buffers_numa[1],
			 conf.interfaces_numa[0], conf.interfaces_numa[1]);

	do{
		int i;
		FILE* fp;
		FILE* fp_new;
		char line[1024] = {0};
		char buf[1024];
		const char *filename = FILE_STARTUP;
		const char *filename_new = FILE_STARTUP"_tmp";
		const char *filename_template = FILE_STARTUP"_template";
		const char *token = NULL;
		int main_heap_size_found = 0;
		int main_core_found = 0;
		int corelist_workers_found = 0;
		int num_rx_queues_found = 0;
		int num_rx_desc_found = 0;
		int num_tx_desc_found = 0;
		int driver_found = 0;
		int buffers_numa_found_count = 0;
		int buffers_numa_found[STARTUP_NUMA_MAX];
		memset(buffers_numa_found, 0, sizeof(buffers_numa_found));

		fp = fopen( filename_template, "r" );
		if(NULL == fp){
			return 0;
		}

		fp_new = fopen( filename_new, "w+" );
		if(NULL == fp_new){
			fclose(fp);
			return 0;
		}

	    while (fgets(line, sizeof(line), fp) != NULL) {
			token = strstr(line, "main-heap-size");
			if(token){
				if(main_heap_size_found == 0 && param.main_heap_size[0]){
					snprintf(buf, sizeof(buf), "        main-heap-size %s\n", param.main_heap_size);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					main_heap_size_found = 1;
				}
				goto next;;
			}

			token = strstr(line, "main-core");
			if(token){
				if(main_core_found == 0 && param.main_core>=0){
					snprintf(buf, sizeof(buf), "        main-core %d\n", param.main_core);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					main_core_found = 1;
				}
				goto next;;
			}

			token = strstr(line, "corelist-workers");
			if(token){
				if(corelist_workers_found == 0 && param.corelist_workers[0]){
					snprintf(buf, sizeof(buf), "        corelist-workers %s\n", param.corelist_workers);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					corelist_workers_found = 1;
				}
				goto next;;
			}

			for(i=0; i<STARTUP_NUMA_MAX; i++){
				char numa_buf[64];
				snprintf(numa_buf, sizeof(numa_buf), "buffers-numa%d", i);
				token = strstr(line, numa_buf);
				if(token){
					if(buffers_numa_found[i] == 0 && param.buffers_numa[i]){
						snprintf(buf, sizeof(buf), "         %s %d\n", numa_buf,
							param.buffers_numa[i]);
						fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
						buffers_numa_found[i] = 1;
						buffers_numa_found_count++;
					}
					goto next;
				}	
			}

			token = strstr(line, "num-rx-queues");
			if(token){
				if(num_rx_queues_found == 0 && param.num_rx_queues){
					snprintf(buf, sizeof(buf), "                num-rx-queues %d\n", param.num_rx_queues);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					num_rx_queues_found = 1;
				}
				goto next;
			}	

			token = strstr(line, "num-rx-desc");
			if(token){
				if(num_rx_desc_found == 0 && param.num_rx_desc){
					snprintf(buf, sizeof(buf), "                num-rx-desc %d\n", param.num_rx_desc);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					num_rx_desc_found = 1;
				}
				goto next;
			}

			token = strstr(line, "num-tx-desc");
			if(token){
				if(num_tx_desc_found == 0 && param.num_rx_desc){
					snprintf(buf, sizeof(buf), "                num-tx-desc %d\n", param.num_rx_desc);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					num_tx_desc_found = 1;
				}
				goto next;
			}

			token = strstr(line, "uio-driver");
			if(token){
				if(driver_found == 0 && param.driver && !strstr(param.driver,STARTUP_DRIVER_AUTO)){
					snprintf(buf, sizeof(buf), "        uio-driver %s\n", param.driver);
					fwrite( buf, strlen( buf ), sizeof( char ), fp_new );
					driver_found = 1;
				}
				goto next;
			}

			fwrite( line, strlen( line ), sizeof( char ), fp_new );
			
	next:		
			memset(line, 0, sizeof(line));
		}

		fclose(fp);
		fclose(fp_new);

		if( !main_heap_size_found
			 || !main_core_found
			 || !corelist_workers_found
			 || !num_rx_queues_found
			 || !num_rx_desc_found
			 || !num_tx_desc_found
			 || !buffers_numa_found_count){
			init_coordinate_log("%s(): cannot found startup.conf param.\n",__func__);
			break;
		}
			 
		rename(filename_new, filename);	

		check_vpp_runtime_params();			 
	}while(0);
	
	return 0;
}
#endif

