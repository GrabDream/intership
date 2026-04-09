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
#include "ace_init_chuangshi_nr03.h"
#include "../../ace_include/ace_common_macro.h"
#include "ace_port_map.h"
#include "ace_init_common.h"
#include "ext_card_common.h"

#define I40E_COMMAND "%s > /sys/kernel/debug/i40e/%s/command"

static unsigned int uiSlots_card_type[6] = {0};
static int nr03_card_adaptive(struct x_eth_item *item, struct x_card_struct *card);

static struct x_card_config conf[] =
{
	{" ../../devices/pci0000:00/0000:00:15.0/", TYPE_HA, NULL, onboard1, 0},
	{" ../../devices/pci0000:00/0000:00:16.0/", TYPE_MGT, NULL, onboard2, 0},

	/* ../../devices/pci0000:88/0000:88:04.0/0000:89:00.0  switch1
	   ../../devices/pci0000:88/0000:88:04.0/0000:89:00.1  eth30 | HGE0-30 | LAN16 */
	{" ../../devices/pci0000:88/0000:88:04.0/", TYPE_HGE, NULL, onboard3, -1},

	{" ../../devices/pci0000:50/0000:50:02.0/", TYPE_GE, nr03_card_adaptive, slot1, 0},//slot1
	{" ../../devices/pci0000:90/0000:90:02.0/", TYPE_GE, nr03_card_adaptive, slot2, 0},//slot2
	{" ../../devices/pci0000:00/0000:00:10.0/", TYPE_GE, nr03_card_adaptive, slot3, 0},//slot3	
	{" ../../devices/pci0000:50/0000:50:04.0/", TYPE_GE, nr03_card_adaptive, slot4, 0},//slot4
	{" ../../devices/pci0000:90/0000:90:04.0/", TYPE_GE, nr03_card_adaptive, slot5, 0},//slot5 
	{" ../../devices/pci0000:90/0000:90:05.0/", TYPE_GE, nr03_card_adaptive, slot6, 0},//slot6
	
	{NULL, 0, 0, 0},
};

static int nr03_get_chuangshi_slot_card_type(void)
{
	int i = 0;
	int iRet = 0;
	char szCmd[256] = {0};
	char szLine[256] = {0};
	unsigned int *iArrSlot = uiSlots_card_type;
	int iSlots = 6;
	const char *i2ccmd[6] = {
		"/usr/sbin/i2cset -f -y %d 0x71 0x08", /*slot1*/
		"/usr/sbin/i2cset -f -y %d 0x70 0x01", /*slot2*/
		"/usr/sbin/i2cset -f -y %d 0x70 0x02", /*slot3*/
		"/usr/sbin/i2cset -f -y %d 0x71 0x01", /*slot4*/
		"/usr/sbin/i2cset -f -y %d 0x71 0x02", /*slot5*/
		"/usr/sbin/i2cset -f -y %d 0x71 0x04", /*slot6*/
	};

	if ( iSlots <= 6 )
	{
		int iI2c = get_i2c_buf_addr( "I801" );

		if ( -1 != iI2c )
		{
			for ( i=0; i<iSlots; i++ )
			{
				snprintf( szCmd, sizeof( szCmd ) - 1, "/usr/sbin/i2cset -f -y %d 0x70 0x0; /usr/sbin/i2cset -f -y %d 0x71 0x0", iI2c, iI2c );
				system( szCmd );

				snprintf( szCmd, sizeof( szCmd ) - 1, i2ccmd[i], iI2c);
				system( szCmd );


				FILE* fp = popen( "/usr/sbin/i2cdump -f -y 0 0x57 b | grep 'a0:' | awk '{print $3$4}'", "r" );
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
	}

	return 0;
}

static int nr03_card_adaptive(struct x_eth_item *item, struct x_card_struct *card)
{
	int iDriver = check_uevent_content(item->uevent, "DRIVER=i40e" );
	int iDevType = check_uevent_content(item->uevent, "PCI_ID=8086:1572" );
	chuangshi_card_type type = type_UNKNOW;
	
	int idx = item->slot_index - 1;
	if (idx>=0 && idx <6){
		type = uiSlots_card_type[idx];
		card->card_type = type;
	}

	/* HPC32 ,HPC13 ,HPC03 ,HPC12*/
	if(iDriver || check_uevent_content(item->uevent, "DRIVER=txgbe" )){
		card->type = TYPE_XGE;
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

	return -1;
}

static void open_i40e_light(char *domain)
{
	char cmd[512];
	/*
	cd /sys/kernel/debug/i40e/0000:04:00.0 
	echo write 0x00088100 0x03f80018 > command
	echo write 0x00088104 0x03f80018 > command
	echo write 0x00088108 0x03f80018 > command
	echo write 0x0008810c 0x03f80018 > command
	echo write 0x00088184 0x40 > command
	echo write 0x00088184 0x41 > command
	echo write 0x00088184 0x42 > command
	echo write 0x00088184 0x43 > command	
	*/
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088100 0x03f80018", domain); 
	system(cmd);
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088104 0x03f80018", domain); 
	system(cmd);
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088108 0x03f80018", domain); 
	system(cmd);
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x0008810c 0x03f80018", domain); 
	system(cmd);
	
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088184 0x40", domain); 
	system(cmd);
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088184 0x41", domain); 
	system(cmd);
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088184 0x42", domain); 
	system(cmd);
	snprintf(cmd, sizeof(cmd), I40E_COMMAND,
		"echo write 0x00088184 0x43", domain); 
	system(cmd);

	init_coordinate_log("%s", cmd);
}

void adaptive_i40e_light(struct x_dev_struct *x_dev)
{
	int i, j, found = 0;
	struct x_card_struct *item = NULL;
	
	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
			
		found = 0;
		if(item->num == 4 || item->num == 2){ 		
			//init_coordinate_log("zzq---------");
			int iDriver = check_uevent_content(item->eth_item[0].uevent, "DRIVER=i40e" );
				
			if(iDriver){
				found = 1;
				//init_coordinate_log("zzq---------ok");
			}

			if(found){	
				for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
					open_i40e_light(item->eth_item[j].domain);
				}
			}
		}
	}
}

static int adaptive_board(struct x_dev_struct *x_dev)
{
	int i, j;		
	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		struct x_card_struct *item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
		
		for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
			if(strstr(item->eth_item[j].readlink, "devices/pci0000:88/0000:88:04.0/0000:89:00.0")){
				snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
						"switch1");	
				item->eth_item[j].type = TYPE_NONE;
			}
			else if(strstr(item->eth_item[j].readlink, "devices/pci0000:88/0000:88:04.0/0000:89:00.1")){
				if(x_dev->mode == MOD_GE){
					snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							"HGE0-30");
				}
				else if(x_dev->mode == MOD_eth){
					snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							"eth30");
				}
				else{
					snprintf( item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							"HGE0-30"); 						
				}
			}			
		}
	}

	return 0;
}

int exchange_8_card(struct x_dev_struct *x_dev)
{
	int i, j;
	struct x_card_struct *item = NULL;	
	struct x_eth_item tmp_item[8];
	//char tmpname[8][16];
	int idx[8] = {1, 0, 3, 2, 5, 4, 7, 6};

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
			
		if(item->num == 8 && item->conf->slot_index > slot_start){			
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
}

int adaptive_HPC03(struct x_dev_struct *x_dev)
{
	int i, j;
	struct x_card_struct *item = NULL;
	struct x_eth_item tmp_item[4];
	//char tmpname[4][16];
	int idx[4] = {3, 2, 1, 0};
	//init_coordinate_log("zzq---------%d", offsetof(struct x_eth_item, newname));

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
			
		if(item->num == 4 
			&& item->conf->slot_index > slot_start
			&& item->card_type == type_HPC03){			
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

int adaptive_HPC11(struct x_dev_struct *x_dev)
{
	int i, j;
	struct x_card_struct *item = NULL;	
	struct x_eth_item tmp_item[4];
	//char tmpname[4][16];
	int idx[4] = {0, 1, 3, 2};

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
			
		if(item->num == 4 && item->conf->slot_index > slot_start ){			
			int iDriver = check_uevent_content(item->eth_item[0].uevent, "DRIVER=ngbe" );
			if(iDriver){
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
	}

	return 0;
}

int adaptive_HPC13(struct x_dev_struct *x_dev)
{
	int i, j;
	struct x_card_struct *item = NULL;	
	struct x_eth_item tmp_item[4];
	//char tmpname[4][16];
	int idx[4] = {2, 3, 0, 1};

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
			
		int iDriver = check_uevent_content(item->eth_item[0].uevent, "DRIVER=i40e" );
		if(iDriver && item->num == 4 
			&& item->conf->slot_index > slot_start
			&& (item->card_type == type_HPC13_PASS_A || item->card_type == type_HPC13_PASS_B)){ 		
			if(iDriver){
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
	}

	return 0;
}

int adaptive_HPC08(struct x_dev_struct *x_dev)
{
	int i, j;
	struct x_card_struct *item = NULL;
	char tmpname[2][16];
	int idx[2] = {1, 0};

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;
			
		int iDriver = check_uevent_content(item->eth_item[0].uevent, "DRIVER=txgbe" );
		if(iDriver && item->num == 2 && item->card_type == type_HPC08){ 		
			if(iDriver){
				for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
					snprintf(tmpname[j], 16, item->eth_item[idx[j]].newname);
				}

				for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
					snprintf(item->eth_item[j].newname, sizeof(item->eth_item[j].newname), 
							tmpname[j]); 
				}
			}
		}
	}

	return 0;
}

int adaptive_HPC22(struct x_dev_struct *x_dev)
{
	int i, j, k;
	struct x_card_struct *item = NULL;
	struct x_eth_item tmp_item;

	for ( i = 0; i<DEV_CARD_MAX; i++ ){
		item = &(x_dev->card_item[i]);
		if(item->conf==NULL)
			continue;

		if(item->num == 4 
			&& item->conf->slot_index >= slot1
			&& check_uevent_content(item->eth_item[0].uevent, "DRIVER=rnpm" )
			&& check_uevent_content(item->eth_item[0].uevent, "PCI_ID=8848:1020" )){
			for ( j = 0; j < item->num&&j<CARD_ETH_MAX; j++ ){
				get_mac_from_kernel(item->eth_item[j].oldname, item->eth_item[j].realname, sizeof(item->eth_item[j].realname));
			}
		}
	}
}

static void generate_bypass_map(struct x_dev_struct *x_dev)
{
	int i, j;
	unsigned int *uiSlots = uiSlots_card_type;	
	
	FILE* fpBypass = fopen( "/home/ace_bypass_map_nr01_ext.dat", "w" );
	
	if ( fpBypass )
	{
		int i, j;
		struct x_card_struct *item = NULL;

		for ( i = 0; i<DEV_CARD_MAX; i++ ){
			item = &(x_dev->card_item[i]);
			if(item->conf==NULL)
				continue;
				
			if(i >= slot1 && i<=slot6){
				int iGroup = 0;
				char szSlotBypassLine[64] = {0};
				int iExtCardBypassLen = 0;
				char szExtCardBypass[1024] = {0};
		
				switch ( item->card_type )
				{
				case type_HPC01_PASS: // HPC01
					iGroup = 2;
					snprintf( szSlotBypassLine, sizeof( szSlotBypassLine ) - 1, "#slot%d,HPC01,%d,2\n", i-slot_start, TYPE_8_1G_COPPER_BP );
					break;
				case type_HPC07_PASS: // HPC07
					iGroup = 2;
					snprintf( szSlotBypassLine, sizeof( szSlotBypassLine ) - 1, "#slot%d,HPC07,%d,2\n", i-slot_start, TYPE_8_1G_COPPER_BP );
					break;
				case type_HPC09: // HPC09
					iGroup = 2;
					snprintf( szSlotBypassLine, sizeof( szSlotBypassLine ) - 1, "#slot%d,HPC09,%d,2\n", i-slot_start, TYPE_4_1G_COPPER_4_1G_FIBER_COMBO );
					break;
				case type_HPC11_COPPER_PASS: // HPC11
					iGroup = 2;
					snprintf( szSlotBypassLine, sizeof( szSlotBypassLine ) - 1, "#slot%d,HPC11,%d,2\n", i-slot_start, TYPE_4_1G_COPPER_BP );
					break;
				case type_HPC13_PASS_A: // HPC13
					iGroup = 2;
					snprintf( szSlotBypassLine, sizeof( szSlotBypassLine ) - 1, "#slot%d,HPC13,%d,2\n", i-slot_start, TYPE_4_10G_FIBER_BP );
					break;
				}
		
				if ( szSlotBypassLine[0] ){
					fwrite( szSlotBypassLine, 1, strlen( szSlotBypassLine ), fpBypass );

					for ( j=0; j<iGroup; j++ ){
						iExtCardBypassLen += snprintf( szExtCardBypass + iExtCardBypassLen, sizeof( szExtCardBypass ) - iExtCardBypassLen, "%s<=>%s\n", 
								item->eth_item[j*2].newname, item->eth_item[j*2+1].newname);
					}
					if ( iExtCardBypassLen > 0 ){
						fwrite( szExtCardBypass, 1, strlen( szExtCardBypass ), fpBypass );
					}
				}				
			}
		}		
	
		fclose( fpBypass );
	}
}

static int nr03_eth_adaptive(struct x_dev_struct *x_dev)
{	
	adaptive_i40e_light(x_dev);
	
	adaptive_board(x_dev);
	
	exchange_8_card(x_dev);

	adaptive_HPC03(x_dev);

	adaptive_HPC13(x_dev);

	adaptive_HPC11(x_dev);


	generate_bypass_map(x_dev);

	return 0;
}

int nr03_set_map_port_pre(void)
{		
	return 0;
}

int nr03_set_map_port(void)
{
	nr03_get_chuangshi_slot_card_type();

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
	handle_x_card(conf, MOD_GE, 0, 0, nr03_eth_adaptive);
#else
	handle_x_card(conf, MOD_eth, WHO_eth, 32, nr03_eth_adaptive);
#endif
#else
	handle_x_card(conf, MOD_GE, 0, 0, nr03_eth_adaptive);
#endif

	return 0;
}

int nr03_set_map_port_post(void)
{
	if ( access( ACE_SYSTEM_CONFIG_FILE, F_OK ) ){
		generate_default_ip("MGT");
	}

	return 0;
}

static void nr03_lcp_create(FILE* fp, struct x_dev_struct *x_dev)
{
	const char *s1 = "lcp create eth0 host-if eth0\n"
					 "lcp create eth1 host-if eth1\n"
					 "lcp create eth2 host-if eth2\n"
					 "lcp create eth3 host-if eth3\n"
					 "lcp create eth4 host-if eth4\n"
					 "lcp create eth5 host-if eth5\n"
					 "lcp create eth6 host-if eth6\n"
					 "lcp create eth7 host-if eth7\n"
					 "lcp create eth8 host-if eth8\n"
					 "lcp create eth9 host-if eth9\n"
					 "lcp create eth10 host-if eth10\n"
					 "lcp create eth11 host-if eth11\n"
					 "lcp create eth12 host-if eth12\n"
					 "lcp create eth13 host-if eth13\n"
					 "lcp create eth14 host-if eth14\n"
					 "lcp create eth15 host-if eth15\n"
					 "lcp create eth16 host-if eth16\n"
					 "lcp create eth17 host-if eth17\n"
					 "lcp create eth18 host-if eth18\n"
					 "lcp create eth19 host-if eth19\n"
					 "lcp create eth20 host-if eth20\n"
					 "lcp create eth21 host-if eth21\n"
					 "lcp create eth22 host-if eth22\n"
					 "lcp create eth23 host-if eth23\n"
					 "lcp create eth24 host-if eth24\n"
					 "lcp create eth25 host-if eth25\n"
					 "lcp create eth26 host-if eth26\n"
					 "lcp create eth27 host-if eth27\n"
					 "lcp create eth28 host-if eth28\n"
					 "lcp create eth29 host-if eth29\n"
					 "lcp create eth31 host-if eth31\n";
	const char *s2 = "lcp create GE0-0 host-if GE0-0\n"
					 "lcp create GE0-1 host-if GE0-1\n"
					 "lcp create GE0-2 host-if GE0-2\n"
					 "lcp create GE0-3 host-if GE0-3\n"
					 "lcp create GE0-4 host-if GE0-4\n"
					 "lcp create GE0-5 host-if GE0-5\n"
					 "lcp create GE0-6 host-if GE0-6\n"
					 "lcp create GE0-7 host-if GE0-7\n"
					 "lcp create GE0-8 host-if GE0-8\n"
					 "lcp create GE0-9 host-if GE0-9\n"
					 "lcp create GE0-10 host-if GE0-10\n"
					 "lcp create GE0-11 host-if GE0-11\n"
					 "lcp create GE0-12 host-if GE0-12\n"
					 "lcp create GE0-13 host-if GE0-13\n"
					 "lcp create GE0-14 host-if GE0-14\n"
					 "lcp create GE0-15 host-if GE0-15\n"
					 "lcp create XGE0-16 host-if XGE0-16\n"
					 "lcp create XGE0-17 host-if XGE0-17\n"
					 "lcp create XGE0-18 host-if XGE0-18\n"
					 "lcp create XGE0-19 host-if XGE0-19\n"
					 "lcp create XGE0-20 host-if XGE0-20\n"
					 "lcp create XGE0-21 host-if XGE0-21\n"
					 "lcp create XGE0-22 host-if XGE0-22\n"
					 "lcp create XGE0-23 host-if XGE0-23\n"
					 "lcp create XGE0-24 host-if XGE0-24\n"
					 "lcp create XGE0-25 host-if XGE0-25\n"
					 "lcp create XGE0-26 host-if XGE0-26\n"
					 "lcp create XGE0-27 host-if XGE0-27\n"
					 "lcp create FGE0-28 host-if FGE0-28\n"
					 "lcp create FGE0-29 host-if FGE0-29\n"
					 "lcp create HGE0-31 host-if HGE0-31\n";


#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
	fwrite( s2, strlen( s2 ), sizeof( char ), fp ); 
#else
	fwrite( s1, strlen( s1 ), sizeof( char ), fp ); 
#endif
#else
	fwrite( s2, strlen( s2 ), sizeof( char ), fp ); 
#endif

	generate_lcp_create(fp, x_dev);

}

int nr03_def_hylab_vpp_cmd_conf(void)
{
	generate_hylab_vpp_cmd_conf(nr03_lcp_create);

	generate_default_ip("MGT");

	return 0;
}

int nr03_update_hylab_vpp_cmd_conf(void)
{
	handle_hylab_vpp_cmd_conf(nr03_lcp_create);

	return 0;
}

