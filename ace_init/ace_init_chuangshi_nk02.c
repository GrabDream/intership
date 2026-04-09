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
#include "ace_init_chuangshi_nk02.h"
#include "../../ace_include/ace_common_macro.h"
#include "ace_port_map.h"
#include "ace_init_common.h"
#include "ext_card_common.h"

static int nk02_card_adaptive(struct x_eth_item *item, struct x_card_struct *card);
static unsigned int uiSlots_card_type[2] = {0};

static struct x_card_config conf[] =
{
	{" ../../devices/pci0000:00/0000:00:15.0/usb1/", TYPE_MGT, NULL, onboard1, 0},
	{" ../../devices/pci0000:00/0000:00:16.0/", TYPE_XGE, NULL, onboard2, 0},
	{" ../../devices/pci0000:00/0000:00:17.0/", TYPE_GE, NULL, onboard3, 2},

	{" ../../devices/pci0000:00/0000:00:0b.0/", TYPE_GE, nk02_card_adaptive, slot1, 0},//slot1
	{" ../../devices/pci0000:00/0000:00:09.0/", TYPE_GE, nk02_card_adaptive, slot2, 0},//slot2

	{NULL, 0, 0, 0},
};

extern int exchange_8_card(struct x_dev_struct *x_dev);
extern int adaptive_HPC03(struct x_dev_struct *x_dev);
extern int adaptive_HPC11(struct x_dev_struct *x_dev);
extern int adaptive_HPC13(struct x_dev_struct *x_dev);
extern int adaptive_HPC22(struct x_dev_struct *x_dev);
extern void adaptive_i40e_light(struct x_dev_struct *x_dev);

static int nk02_get_chuangshi_slot_card_type(void)
{
	int i = 0;
	int iRet = 0;
	char szCmd[256] = {0};
	char szLine[256] = {0};
	FILE* fp;
	int iI2c = -1;

	unsigned int *iArrSlot = uiSlots_card_type;
	const char *i2ccmd[2] = {
		"/usr/sbin/i2cset -f -y %d 0x70 2", /*slot1*/
		"/usr/sbin/i2cset -f -y %d 0x70 1", /*slot2*/
	};

	snprintf( szCmd, sizeof( szCmd ), "/usr/sbin/i2cdetect -l | grep 'I801' | awk '{print$1}' | awk -F '-' '{print$2}'");
	fp = popen( szCmd, "r" );
	if ( fp ){
		while ( 0 != fgets( szLine, sizeof( szLine ), fp ) ){
			snprintf( szCmd, sizeof( szCmd ), "/usr/sbin/i2cdetect -y -r %d | grep \"^70: 70\" >/dev/null", atoi(szLine));

			init_coordinate_log( "[%s]",  szCmd);
			if(0 == system(szCmd)){
				iI2c = atoi(szLine);
				break;
			}
		}

		pclose( fp );
	}

	if ( -1 != iI2c )
	{
		for ( i=0; i<2; i++ )
		{
			snprintf( szCmd, sizeof( szCmd ) - 1, i2ccmd[i], iI2c);
			system( szCmd );

			snprintf( szCmd, sizeof( szCmd ) - 1, "/usr/sbin/i2cdump -f -y %d 0x57 | grep 'a0:' | awk '{print $3$4}'", iI2c);
			fp = popen( szCmd, "r" );
			if ( fp )
			{
				memset( szLine, 0, sizeof( szLine ) );

				szLine[0] = '0'; szLine[1] = 'x';

				if ( 0 != fgets( szLine + 2, sizeof( szLine ) - 3, fp ) )
				{
					if ( 'X' != szLine[2] )
					{
						iArrSlot[i] = strtoul( szLine, NULL, 16 );
					}
				}

				pclose( fp );
			}

			init_coordinate_log( "[%s:%d] ==> Info, slot%d, card info 0x%X", __FUNCTION__, __LINE__, i, iArrSlot[i] );
		}
	}

	return 0;
}

static int nk02_get_chuangshi_onboard_card_type(void)
{
	int i3338 = 0;
	char szLine[256] = {0};

	FILE* fp = popen( "cat /proc/cpuinfo | grep \"model name\" | awk 'NR==1'", "r" );

	if ( fp )
	{
		memset( szLine, 0x00, sizeof( szLine ) );

		fgets( szLine, sizeof( szLine ), fp );

		pclose( fp ); fp = NULL;

		if ( !strncmp( szLine, "model name\t: Intel(R) Atom(TM) CPU C3338", sizeof("model name\t: Intel(R) Atom(TM) CPU C3338") - 1 ) )
		{
			i3338  = 1;
		}

		if ( 1 == i3338 )
		{
			conf[1].type = TYPE_GE;
		}
	}

	return 0;
}

static void nk02_ext_card_fiber_light_on( void )
{
	char szCmd[256] = {0};
	char szLine[256] = {0};

	FILE* fp = popen("i2cdetect -l | grep I801 | awk '{print $1}' | awk -F'-' '{print $2}'", "r");
	if ( fp )
	{
		fgets( szLine, sizeof( szLine ), fp );

		pclose(fp);
	}
	else
	{
		init_coordinate_log( "[%s:%d] ==> Error, call function [i2cdetect -l | grep I801 | awk '{print $1}' | awk -F'-' '{print $2}'] failed.", __FUNCTION__, __LINE__ );
	}

	if ( 0 == szLine[0] )
	{
		init_coordinate_log( "[%s:%d] ==> Error, get i2c id failed.", __FUNCTION__, __LINE__ );
	}
	else
	{
		snprintf( szCmd, sizeof( szCmd ) - 1, "i2cget -f -y %c 0x70 2 && i2cset -f -y %c 0x21 0x2 0xf0 && i2cset -f -y %c 0x21 0x6 0xf0", szLine[0], szLine[0], szLine[0] );
		system( szCmd );
		init_coordinate_log( "[%s:%d] ==> Info, run command [%s] to turn on slot1.", __FUNCTION__, __LINE__, szCmd );

		snprintf( szCmd, sizeof( szCmd ) - 1, "i2cget -f -y %c 0x70 1 && i2cset -f -y %c 0x21 0x2 0xf0 && i2cset -f -y %c 0x21 0x6 0xf0", szLine[0], szLine[0], szLine[0] );
		system( szCmd );
		init_coordinate_log( "[%s:%d] ==> Info, run command [%s] to turn on slot2.", __FUNCTION__, __LINE__, szCmd );
	}

	return;
}

static int nk02_card_adaptive(struct x_eth_item *item, struct x_card_struct *card)
{
	int iDriver  = check_uevent_content( item->uevent, "DRIVER=i40e" );
	int iDevType = check_uevent_content( item->uevent, "PCI_ID=8086:1572" );
	chuangshi_card_type type = type_UNKNOW;

	int idx = item->slot_index - 1;
	if (idx>=0 && idx <2){
		type = uiSlots_card_type[idx];
		card->card_type = type;
	}

	/* HPC32 ,HPC13 ,HPC03 ,HPC12*/
	if(iDriver){
		card->type = TYPE_XGE;
	}
	if(check_uevent_content(item->uevent, "DRIVER=txgbe" )){
		card->type = TYPE_XGE;
		card->card_type = type_HPC08;
	}

	/* HPC03 */
	iDevType = check_uevent_content(item->uevent, "PCI_ID=8086:1581" );
	if(iDriver && iDevType){
		card->card_type = type_HPC03;

		/* HPC13 */
		if ( ( type_HPC13_PASS_A == type ) || ( type_HPC13_PASS_B == type ) )
		{
			card->card_type = type;
		}
	}

	/* HPC12 */
	iDevType = check_uevent_content(item->uevent, "PCI_ID=8086:1583" );
	if(iDriver && iDevType){
		card->card_type = type_HPC12;
		card->type = TYPE_FGE;

		//init_coordinate_log("zzq---------%x", type);
	}
	else if((check_uevent_content( item->uevent, "DRIVER=rnpm" ) && check_uevent_content( item->uevent, "PCI_ID=8848:1020" ))){
		card->type = TYPE_XGE;	
	}

	return -1;
}

static int adaptive_onboard2(struct x_dev_struct *x_dev)
{
	int i, j;
	struct x_card_struct *item = NULL;
	struct x_eth_item tmp_item[2];
	//char tmpname[4][16];
	int idx[2] = {1, 0};

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;

		if(item->conf->slot_index == onboard2 && item->num ==2){
			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				memcpy(&tmp_item[j], &item->eth_item[idx[j]], offsetof(struct x_eth_item, newname));
				//snprintf(tmpname[j], 16, item->eth_item[idx[j]].oldname);
			}

			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				memcpy(&item->eth_item[j], &tmp_item[j], offsetof(struct x_eth_item, newname));
				//snprintf(item->eth_item[j].oldname, sizeof(item->eth_item[j].oldname), 
				//		tmpname[j]); 
			}
		}
	}

	return 0;
}

static int adaptive_onboard3(struct x_dev_struct *x_dev)
{
	int i;
	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		struct x_card_struct *item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;

		if(item->conf->slot_index == onboard3 && item->num ==2){
				snprintf( item->eth_item[0].newname, sizeof(item->eth_item[0].newname), 
						"switch0");
				item->eth_item[0].type = TYPE_NONE;

				snprintf( item->eth_item[1].newname, sizeof(item->eth_item[1].newname), 
						"switch1");
				item->eth_item[1].type = TYPE_NONE;

				do{
					const char *file = "/tmp/nk02_switch_domain";
					FILE *fp_n = fopen( file, "w+");
					if (fp_n){
						fprintf( fp_n, item->eth_item[0].domain );

						fflush( fp_n );
						fclose( fp_n );
					}
				}while(0);
		}
	}

	return 0;
}

static int nk02_eth_adaptive(struct x_dev_struct *x_dev)
{
	nk02_ext_card_fiber_light_on();

	adaptive_i40e_light(x_dev);

	adaptive_onboard2(x_dev);

	adaptive_onboard3(x_dev);

	exchange_8_card(x_dev);

	adaptive_HPC03(x_dev);

	adaptive_HPC13(x_dev);

	adaptive_HPC11(x_dev);

	adaptive_HPC22(x_dev);

	return 0;
}

int nk02_set_map_port_pre(void)
{
	return 0;
}

int nk02_set_map_port(void)
{
	nk02_get_chuangshi_slot_card_type();

	nk02_get_chuangshi_onboard_card_type();

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
	handle_x_card(conf, MOD_GE, 0, 0, nk02_eth_adaptive);
#else
	handle_x_card(conf, MOD_eth, WHO_eth, 0, nk02_eth_adaptive);
#endif
#else
	handle_x_card( conf, MOD_LANWAN, WHO_LAN, 1, nk02_eth_adaptive);
#endif

	return 0;
}

int nk02_set_map_port_post(void)
{
	if ( access( ACE_SYSTEM_CONFIG_FILE, F_OK ) ){
		generate_default_ip("MGT");
	}

	return 0;
}

static void nk02_lcp_create(FILE* fp, struct x_dev_struct *x_dev)
{
	const char *s1 = "lcp create eth2 host-if eth2\n"
					 "lcp create eth3 host-if eth3\n"
					 "lcp create eth4 host-if eth4\n"
					 "lcp create eth5 host-if eth5\n"
					 "lcp create eth6 host-if eth6\n"
					 "lcp create eth7 host-if eth7\n"
					 "lcp create eth8 host-if eth8\n"
					 "lcp create eth9 host-if eth9\n";
	const char *s2 = "lcp create GE0-2 host-if GE0-2\n"
					 "lcp create GE0-3 host-if GE0-3\n"
					 "lcp create GE0-4 host-if GE0-4\n"
					 "lcp create GE0-5 host-if GE0-5\n"
					 "lcp create GE0-6 host-if GE0-6\n"
					 "lcp create GE0-7 host-if GE0-7\n"
					 "lcp create GE0-8 host-if GE0-8\n"
					 "lcp create GE0-9 host-if GE0-9\n";
	const char *s3 = "lcp create LAN2 host-if LAN2\n"
					 "lcp create WAN2 host-if WAN2\n"
					 "lcp create LAN3 host-if LAN3\n"
					 "lcp create WAN3 host-if WAN3\n"
					 "lcp create LAN4 host-if LAN4\n"
					 "lcp create WAN4 host-if WAN4\n"
					 "lcp create LAN5 host-if LAN5\n"
					 "lcp create WAN5 host-if WAN5\n";

	generate_lcp_create(fp, x_dev);

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
	fwrite( s2, strlen( s2 ), sizeof( char ), fp );
#else
	fwrite( s1, strlen( s1 ), sizeof( char ), fp );
#endif
#else
	fwrite( s3, strlen( s3 ), sizeof( char ), fp );
#endif

}

int nk02_def_hylab_vpp_cmd_conf(void)
{
	generate_hylab_vpp_cmd_conf(nk02_lcp_create);

	generate_default_ip("MGT");

	return 0;
}

int nk02_update_hylab_vpp_cmd_conf(void)
{
	handle_hylab_vpp_cmd_conf(nk02_lcp_create);

	return 0;
}

