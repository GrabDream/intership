
#ifndef __ACE_PORT_MAP_H
#define __ACE_PORT_MAP_H

#define MAX_ETH_ONE_CARD	(32)
#define MAX_CARD_ONE_DEV	(16)
#define CARD_SLOD_NAME_LEN  (64)

struct ruijie_ext_eth_struct
{
	char ethPciName[32];
	char ethPciId[16];
	char ethOldName[8];
	char ethNewName[8];
	unsigned int ethPair:8, ethBoardIndex:8, ethAttribute:2, ethMgtIdx:2,x710:1,used:1,ethReserved:10;
};

struct ruijie_ext_card_struct
{
	/*A, B, C, D, E*/
	char cardId;
	char mode;
	unsigned int ethNum;
	unsigned int have_x710_num:4,have_fiber_num:4,eth_speed_type:10,slot_index:6,Reserved:8;
	char card_slot[CARD_SLOD_NAME_LEN];
	struct ruijie_ext_eth_struct ethCard[MAX_ETH_ONE_CARD];
};

typedef struct tag_ACE_PORT_INFO_ST
{
	char oldPhyname[32]; /*fe0, fe1, fe2...*/
	char newPhyname[32]; /*LAN1, LAN2, WAN1, WAN2*/
	U32  ifIndex;        /*start from 1, trust and untrust*/
	U32  isTrust;        /*1: TRUST, 0: UNTRUST, 2: MANAGE*/
	char mac[18];
	char ethPciId[32];
	int  isExistInConf;
	U32  eth_speed_type:10,slot_index:6,Reserved:16;
	char m_szPanel[32];
}ACE_PORT_INFO_ST;
#define ACE_MAX_SUPPORT_PORT_BUSYBOX (ACE_MAX_SUPPORT_PORT + 32)

extern ACE_PORT_INFO_ST  g_port_info[ACE_MAX_SUPPORT_PORT_BUSYBOX];

extern U32 g_portNum;
extern U32 g_iMgtNum;

extern int ace_port_map();

extern int ace_is_firewall();

extern int hylab_init_debug_print(const char * fmt, ...);

#define init_coordinate_log(fmt, args...)		do{hylab_init_debug_print("%s %s %d->" fmt,__FUNCTION__,__FILE__,__LINE__,##args);}while(0)

#endif//#ifndef __ACE_PORT_MAP_H

