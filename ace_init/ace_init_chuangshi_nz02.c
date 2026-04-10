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
#include "ace_init_chuangshi_nz02.h"
#include "../../ace_include/ace_common_macro.h"
#include "ace_port_map.h"
#include "ace_init_common.h"
#include "ext_card_common.h"

extern int exchange_8_card(struct x_dev_struct *x_dev);
extern int adaptive_HPC03(struct x_dev_struct *x_dev);
extern int adaptive_HPC11(struct x_dev_struct *x_dev);
extern int adaptive_HPC13(struct x_dev_struct *x_dev);
extern int adaptive_HPC08(struct x_dev_struct *x_dev);
extern int adaptive_HPC22(struct x_dev_struct *x_dev);


static unsigned int nz02_slot_card_type[2] = {0};
static int nz02_card_adaptive(struct x_eth_item *item, struct x_card_struct *card);

static struct x_card_config conf[] =
{
	{ " ../../devices/platform/soc/3400000.pcie/pci0000:00/0000:00:00.0/", TYPE_GE, nz02_card_adaptive, slot1, 0 }, // slot1
	{ " ../../devices/platform/soc/3500000.pcie/pci0001:00/0001:00:00.0/", TYPE_GE, nz02_card_adaptive, slot2, 0 }, // slot2
	
	{ NULL, 0, 0, 0 },
};

static int nz02_card_adaptive( struct x_eth_item *item, struct x_card_struct *card )
{
	char pi2c1[16] = {0};
	char pi2c2[16] = {0};

	int iDriver = check_uevent_content(item->uevent, "DRIVER=i40e" );
	int iDevType = check_uevent_content(item->uevent, "PCI_ID=8086:1572" );
	chuangshi_card_type type = type_UNKNOW;
	
	int idx = item->slot_index - slot_start - 1;
	if (idx>=0 && idx <2)
		type = nz02_slot_card_type[idx];

	/* HPC32 ,HPC13 ,HPC03 ,HPC12*/
	if ( iDriver ) {
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

	/* HPC08 */
	iDevType = check_uevent_content(item->uevent, "PCI_ID=8088:1001" );
	if ( iDevType ) {
		card->card_type = type_HPC08;
		card->type = TYPE_XGE;
	}

	/* HPC11 */
	iDevType = check_uevent_content(item->uevent, "PCI_ID=8088:0103" );
	if ( iDevType ) {
		card->card_type = type_HPC11_FIBER;
	}

	/* HPC12 */
	iDevType = check_uevent_content(item->uevent, "PCI_ID=8086:1583" );
	if(iDriver && iDevType){
		card->card_type = type_HPC12;
		card->type = TYPE_FGE;
	}

	return -1;
}

static int adaptive_slot_card_conf(void)
{
	int iSlotRet[2] = {0};
	int i;
	board_card_info card_info[2];
	memset(card_info, 0, sizeof(card_info));
	
	changshi_get_card_type_by_i2c( 7, "2190000.i2c", iSlotRet );

	for(i=0; i<2; i++){
		if (iSlotRet[i]){
			card_info[i].online = 1;
			snprintf(card_info[i].card_name, sizeof(card_info[i].card_name), "HPC%02d", iSlotRet[i]);
		}
	}
	write_slot_card_conf(card_info, 2);
	return 0;
}

static int nz02_eth_adaptive(struct x_dev_struct *x_dev)
{
	//memset(uiSlots_card_type, sizeof( uiSlots_card_type ), 0);
	//get_chuangshi_slot_card_type(uiSlots_card_type, sizeof( uiSlots_card_type ) / sizeof( unsigned int ), 6);
	
	exchange_8_card(x_dev);

	adaptive_HPC03(x_dev);

	adaptive_HPC08(x_dev);

	adaptive_HPC11(x_dev);

	adaptive_HPC13(x_dev);

	adaptive_HPC22(x_dev);

	adaptive_slot_card_conf();

	return 0;
}

void nz02_get_slot_info()
{
	char szPic1[16] = {0};
	char szPic2[16] = {0};
	char szArgs[128] = {0};

	// get slot card i2c bus
	get_ext_i2c_address( "2190000.i2c", szPic1, szPic2, 4, 5 );

	if ( szPic1[0] )
	{
		snprintf( szArgs, sizeof( szArgs ) - 1, "-f -y %s 0x57 b | grep '^a0' | awk '{print $3$4}'", szPic1 );
		
		nz02_slot_card_type[0] = get_chuangshi_slot_type( szArgs );
	}

	if ( szPic2[0] )
	{
		snprintf( szArgs, sizeof( szArgs ) - 1, "-f -y %s 0x57 b | grep '^a0' | awk '{print $3$4}'", szPic2 );
		
		nz02_slot_card_type[1] = get_chuangshi_slot_type( szArgs );
	}

	init_coordinate_log( "slot i2c addresss [%s, %s], slot type [0x%X, 0x%X]\n", szPic1, szPic2, nz02_slot_card_type[0], nz02_slot_card_type[1] );

	return;
}

int nz02_set_map_port_pre(void)
{		
	return 0;
}

int nz02_set_map_port(void)
{
	nz02_get_slot_info();

#ifdef CONFIG_HYTF_FW
#if CONFIG_HYTF_FW_NEW==1
	handle_x_card(conf, MOD_GE, 0, 0, nz02_eth_adaptive);
#else
	handle_x_card(conf, MOD_eth, WHO_eth, 10, nz02_eth_adaptive);
#endif
#else
	handle_x_card(conf, MOD_GE, 0, 0, nz02_eth_adaptive);
#endif

	return 0;
}

int nz02_set_map_port_post(void)
{
	/* inband MGT */
	generate_inband_mgt_config();

	if ( access( ACE_SYSTEM_CONFIG_FILE, F_OK ) ){
		generate_default_ip(NULL);
	}

	return 0;
}

static void nz02_lcp_create(FILE* fp, struct x_dev_struct *x_dev)
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
					 "lcp create eth13 host-if eth13\n";

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
					 "lcp create GE0-13 host-if GE0-13\n";


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

int nz02_def_hylab_vpp_cmd_conf(void)
{
	generate_hylab_vpp_cmd_conf(nz02_lcp_create);

	generate_default_ip(NULL);

	return 0;
}

int nz02_update_hylab_vpp_cmd_conf(void)
{
	handle_hylab_vpp_cmd_conf(nz02_lcp_create);

	return 0;
}

