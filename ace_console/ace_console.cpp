/***********************************************************************
* (c) Copyright 2001-2009, Photonic Bridges, All Rights Reserved.
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF PHOTONIC BRIDGES, INC.
* The copyright notice above does not evidence any actual or intended
* publication of such source code. 
*
*  Subsystem:   XXX
*  File:        sysappmonitro.cpp
*  Author:      ke.xiong
*  Description: AceNet NSSP Console Administratio 
*               Version: 1.0an
*
*************************************************************************/
#ifndef _ACE_CONSOLE_CPP_
#define _ACE_CONSOLE_CPP_

#include <openssl/aes.h>
#include <openssl/md5.h>

#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef ARCH_ARM64
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>
#include <sys/stat.h>


#include "../../ace_include/ace_common_macro.h"
#include "../../ace_include/ace_common_struct.h"
#include <time.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

char g_hw_sn[33];
char g_console_password_seed[512];

#define CONSOLE_MAX_STRING (256)     /* Longest allowed response       */

#define MESSAGE_LINE (6)             /* Misc. messages go here         */
#define Q_LINE       (20)            /* Line for Questions             */
#define PROMPT_LINE  (18)            /* Line for prompting on          */

#define BOXED_LINES    (19)
#define BOXED_ROWS     (110) //74

//#define ACE_CONSOLE_PASSWORD      "root*AceNetTech"
//#define ACE_CONSOLE_USERNAME      "root"
#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
#define ACE_CONSOLE_PASSWORD      "talent"
#define ACE_CONSOLE_USERNAME      "superman"
#define ACE_CONSOLE_CUSTOM_CONSOLE        1
#define ACE_CONSOLE_CUSTOM_SSH    0
#elif defined(ruiyuntong)
//#define ACE_CONSOLE_PASSWORD      "airsec123"
#define ACE_CONSOLE_PASSWORD      "Login*PWD"
#define ACE_CONSOLE_USERNAME      "admin"
#define ACE_CONSOLE_CUSTOM_CONSOLE        1
#define ACE_CONSOLE_CUSTOM_SSH    0
#elif defined(gsi)
#define ACE_CONSOLE_PASSWORD      "Login*PWD"
#define ACE_CONSOLE_USERNAME      "admin"
#define ACE_CONSOLE_CUSTOM_CONSOLE        0
#define ACE_CONSOLE_CUSTOM_SSH    1
#else
#define ACE_CONSOLE_PASSWORD      "Login*PWD"
#define ACE_CONSOLE_USERNAME      "admin"
#define ACE_CONSOLE_CUSTOM_CONSOLE 0
#define ACE_CONSOLE_CUSTOM_SSH    0

#endif

#if defined(ruijie)
#define SAY_HELLO_PRE "RG-UAC "
#define SAY_HELLO_SUFFIX ""
#else
#define SAY_HELLO_PRE ""
#define SAY_HELLO_SUFFIX " Administration"
#endif


#define FTP_CONFIG_FILE "/home/config/cfg-scripts/ace_ftp.conf"

static char ftp_username[CONSOLE_MAX_STRING]        = { 0 };
static char ftp_password[CONSOLE_MAX_STRING]        = { 0 };
static char ftp_ipaddress[CONSOLE_MAX_STRING]       = { 0 };
static char ftp_password_new[CONSOLE_MAX_STRING]    = { 0 };
static char ftp_ipaddress_new[CONSOLE_MAX_STRING]   = { 0 };
static char ftp_username_new[CONSOLE_MAX_STRING]    = { 0 };
static int  g_port_num                              = 0;

#ifndef ACE_FIRMWARE
#define ACE_FIRMWARE        "/usr/download/NACFirmware.bin.new"
#define ACE_FIRMWARE_SHORT  "NACFirmware.bin"
#endif

#ifndef ACE_FEATURE
#define ACE_FEATURE        "/usr/download/NACV-FEATURE-LIB.bin.new"
#define ACE_FEATURE_SHORT  "NACV-FEATURE-LIB.bin"
#endif

#ifndef ACE_URLLIB
//#define ACE_URLLIB       "/usr/download/NAC-URL-LIB.bin"
#define ACE_URLLIB       "/usr/download/NACII-URL-LIB.bin.new"
#define ACE_URLLIB_SHORT  "NACII-URL-LIB.bin"
#endif

typedef struct ifname_on_board
{
    char name[32];
    char ipaddr[128];
    char oldaddr[128];
}ifname_on_board;

static ifname_on_board g_ifname_on_board[24];
static int g_bridges[16] = { 0 };
static int lic_rand_req_fill(HYTF_LIC_REQ_MSG* p_req);
static int lic_rand_check(unsigned long mtype, unsigned int subType, unsigned int peer_checksum, unsigned int peer_random);
static void my_reboot(void);


typedef struct ace_page 
{
    int index;
    char contex[BOXED_LINES][1024];
}ace_page;

char* main_menu_console[] =
{
    "sSystem Status",
    "1Serial Number",
    "5Version Information",
#if defined(topsec)
    "6System Time",
#endif
	"cClear Data",
    "wWork Mode",
    "mManagement IP Address",
    "uSystem Update",
    "dDownload Debug information",
    "2Add Route Table",
    "rReset Factory Default",
    "3Save Configuration",
    "4Reset WebUI Acount Mode & Password & Management Policy",
    "eReboot",
#if defined(ruijie) || defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
    "lDebug",
#endif
#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
	"hHycli",
#endif
    0,
};

char* main_menu[] =
{
    "sSystem Status",
    "1Serial Number",
    "5Version Information",
#if defined(topsec)
    "6System Time",
#endif
    "wWork Mode",
    "mManagement IP Address",
    "uSystem Update",
    "dDownload Debug information",
    "2Add Route Table",
    "3Save Configuration",
#if !defined(ruijie) && !ACE_CONSOLE_CUSTOM_SSH
    "bShow Console Seed",
#endif
#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
	"hHycli",
#endif
#if defined(ruijie)
    "lDebug",
#endif
    0,
};


char* status_menu[] =
{
    "1Ps",
    "2Ifconfig",
    "3Netstat",
    "4Freememory",
    "5Df",
    "6Syslog(100 line)",
    "7Syslog(all)",
    "8Route",
    "9Arp",
    "aLsmod",
    "bTop",
    "qBack",
    0,
};

/*
char* system_upgrade_menu[] =
{
    "1FTP Service Config",
    "2System Firmware ",
    "3AIS Database ",
    "4URL Database ",
    "5Unit License ",
    "qBack",
    0,
};
*/
char* system_upgrade_menu[] =
{
#if 0
    (char*)"1FTP Service Config",
#endif
    (char*)"2System Firmware ",
    (char*)"3AIS Database ",
    (char*)"4URL Database ",
    (char*)"5Unit License ",
    (char*)"qBack",
    0,
};

char* transfer_file_menu[] =
{
    "1Upload File",
    "2Download File",
    "qBack",
    0,
};

char g_product_type[256] = { 0 };

static void my_reboot(void)
{
    if(0 == system("grep -q CS-NK02 /usr/private/Firmware_Board_Type")
    	||0 == system("grep -q CS-NL01 /usr/private/Firmware_Board_Type")
    	||0 == system("grep -q CS-NK03 /usr/private/Firmware_Board_Type")
		||0 == system("grep -q YWER-HG5X /usr/private/Firmware_Board_Type")
		)
    {
    	system( "/sbin/reboot -f" );
    }
    else
    {
    	system( "/sbin/reboot" );
    }
}

int console_data_encrypt(char* dynamic_key, int dynamic_len, char* share_key, int share_len, unsigned char* in_byte,unsigned int in_len, unsigned char* out_byte, unsigned int out_len)
{
	MD5_CTX  ctx;
	unsigned char md5_out[32];
	unsigned char md5_in[64];
	int md5_len;
	char md5_text[64];
	unsigned int encrypt_len;
	AES_KEY encryptKey;
	md5_len = snprintf((char*)md5_in, sizeof(md5_in), "%s%s", share_key, dynamic_key);
	memset(md5_out, 0x00, sizeof(md5_out));

	if ((in_len & (AES_BLOCK_SIZE-1)) || (out_len < in_len))
	{
		return -1;
	}

    MD5_Init(&ctx);
    MD5_Update(&ctx, (void *)share_key, share_len);
    MD5_Update(&ctx, (void *)dynamic_key, dynamic_len);
    MD5_Final(md5_out, &ctx);
/*
	if (NULL == MD5 ( md5_in, strlen((char*)md5_in), (unsigned char*)md5_out ))
	{
		cmc_debug_printf("%s %s %d --- failed to MD5", __FUNCTION__,__FILE__,__LINE__);
		return -1;
	}
*/
	memset(md5_text, 0, sizeof(md5_text) );
	for ( encrypt_len = 0; encrypt_len < MD5_DIGEST_LENGTH; encrypt_len++ )
	{
		sprintf(md5_text + (encrypt_len<<1), "%02x", md5_out[encrypt_len]);
	}	

//	noc_client_log(LOG_LEVEL_DEBUG,"%s %s %d --- dynamic %s share %s md5 %s", __FUNCTION__,__FILE__,__LINE__,dynamic_key,share_key,md5_text);

	AES_set_encrypt_key(md5_out, 128, &encryptKey);

 	for (encrypt_len = 0; encrypt_len < in_len; encrypt_len += AES_BLOCK_SIZE)
 	{
    	AES_ecb_encrypt(in_byte + encrypt_len, out_byte + encrypt_len, &encryptKey, AES_ENCRYPT);
 	}

	return 0;
}

int console_data_decrypt(char* dynamic_key, int dynamic_len, char* share_key, int share_len, unsigned char* in_byte,unsigned int in_len, unsigned char* out_byte, unsigned int out_len)
{
	MD5_CTX  ctx;
	unsigned char md5_out[32];
	unsigned char md5_in[64];
	int md5_len;
	char md5_text[64];
	unsigned int encrypt_len;
	AES_KEY encryptKey;
	md5_len = snprintf((char*)md5_in, sizeof(md5_in), "%s%s", share_key, dynamic_key);
	memset(md5_out, 0x00, sizeof(md5_out));

	if ((in_len & (AES_BLOCK_SIZE-1)) || (out_len & (AES_BLOCK_SIZE-1)))
	{
		return -1;
	}
/*
	if (NULL == MD5 ( md5_in, strlen((char*)md5_in), (unsigned char*)md5_out ))
	{
		cmc_debug_printf("%s %s %d --- failed to MD5", __FUNCTION__,__FILE__,__LINE__);
		return -1;
	}
*/
	MD5_Init(&ctx);
	MD5_Update(&ctx, (void *)share_key, share_len);
	MD5_Update(&ctx, (void *)dynamic_key, dynamic_len);
	MD5_Final(md5_out, &ctx);

	memset(md5_text, 0, sizeof(md5_text) );
	for ( encrypt_len = 0; encrypt_len < MD5_DIGEST_LENGTH; encrypt_len++ )
	{
		sprintf(md5_text + (encrypt_len<<1), "%02x", md5_out[encrypt_len]);
	}

//	noc_client_log(LOG_LEVEL_DEBUG,"%s %s %d --- dynamic %s share %s md5 %s", __FUNCTION__,__FILE__,__LINE__,dynamic_key,share_key,md5_text);

	AES_set_decrypt_key(md5_out, 128, &encryptKey);

 	for (encrypt_len = 0; encrypt_len < in_len; encrypt_len += AES_BLOCK_SIZE)
 	{
    	AES_ecb_encrypt(in_byte + encrypt_len, out_byte + encrypt_len, &encryptKey, AES_DECRYPT);
 	}

	return 0;
}

#ifdef CONFIG_ZHGX
static const unsigned char hylab_shell_public_key0[] = 
{
	0xb0, 0x11, 0xbe, 0xa4, 0x86, 0x47, 0xf5, 0x51, 0x9a, 0x79, 0x60, 0x01, 0x5d, 0x43, 0x55, 0xf5,
	0xc5, 0x18, 0xe5, 0x47, 0xb8, 0x3f, 0x30, 0x5a, 0xa1, 0x8d, 0xde, 0xd4, 0x80, 0x22, 0x55, 0x2d,
	0xdd, 0xf8, 0xb0, 0x0e, 0x98, 0xd3, 0x1e, 0xf3, 0x96, 0xc5, 0xcc, 0xa9, 0xf3, 0x56, 0xe6, 0x6f,
	0x00, 0x1d, 0x76, 0x7d, 0x55, 0x09, 0xe0, 0xaa, 0xb0, 0x3b, 0xcc, 0xd2, 0x8a, 0x8c, 0x64, 0x1a,
	0xdb, 0x57, 0x89, 0x0a, 0xc2, 0x55, 0x4e, 0xef, 0xac, 0xb0, 0xe7, 0x1e, 0x54, 0x45, 0xc4, 0x0d,
	0x1f, 0xa6, 0xd0, 0x8a, 0x65, 0x06, 0x89, 0x2a, 0x83, 0x1d, 0x8b, 0xa4, 0x65, 0xb1, 0x79, 0xc0,
	0x93, 0xab, 0x6f, 0xce, 0x62, 0x71, 0x66, 0x93, 0x48, 0x99, 0x5b, 0x28, 0x9e, 0xf5, 0xc3, 0x00,
	0xfa, 0x41, 0x3e, 0xde, 0x11, 0x73, 0xd2, 0x76, 0x01, 0x2d, 0x80, 0x0a, 0x22, 0x5d, 0x62, 0x33,
	0xb4, 0x56, 0x48, 0xbb, 0x3c, 0x23, 0xa2, 0x68, 0x77, 0x98, 0x62, 0x47, 0xc4, 0x84, 0xaf, 0x09,
	0x92, 0xb1, 0xb6, 0x1f, 0x8d, 0xeb, 0xda, 0x78, 0x8b, 0xd3, 0xe6, 0x75, 0xae, 0xfb, 0xec, 0xe3,
	0xf7, 0xff, 0xd5, 0x95, 0x83, 0x57, 0xa1, 0x69, 0x26, 0x45, 0x0c, 0x5e, 0x88, 0x82, 0x3e, 0x90,
	0x9c, 0x55, 0x21, 0xd3, 0xc0, 0xf2, 0xb8, 0xdc, 0x25, 0xd9, 0xb4, 0xa9, 0x18, 0x20, 0xcf, 0x7b,
	0x9f, 0x4e, 0xa2, 0xde, 0xae, 0xa0, 0x6c, 0x9a, 0x7a, 0x12, 0xb8, 0xb1, 0x01, 0x00, 0x85, 0x6b,
	0x5a, 0xe7, 0xf5, 0x59, 0xda, 0xdc, 0xf5, 0xab, 0x76, 0xd4, 0xdf, 0xb5, 0x2f, 0x3b, 0xd8, 0x86,
	0x27, 0x06, 0xa1, 0x9e, 0x60, 0xd8, 0x96, 0x60, 0x6f, 0xfa, 0x7f, 0xe6, 0xa3, 0x8e, 0xc1, 0x78,
	0x85, 0x2a, 0x08, 0x04, 0x59, 0x48, 0x14, 0x49, 0x98, 0xe5, 0xc2, 0x9e, 0x42, 0x36, 0xf0, 0x73,
	0xa2, 0xdf, 0x6c, 0x04, 0x39, 0x90, 0x8e, 0xf9, 0x25, 0xa9, 0x6b, 0x02, 0xd9, 0xcf, 0x7f
};
static const unsigned char hylab_shell_public_key1[] = 
{
	0xc4, 0xca, 0xdd, 0x89, 0x03, 0xdc, 0x1b, 0x55, 0x36, 0x6f, 0x59, 0xf5, 0x27, 0xac, 0x6d, 0x96,
	0x4d, 0x47, 0xe7, 0x75, 0x33, 0xec, 0xa8, 0xb7, 0x36, 0x69, 0xa7, 0x05, 0x8a, 0x99, 0x69, 0x4e,
	0x63, 0x46, 0xd7, 0x67, 0x22, 0xf2, 0xbc, 0x58, 0x62, 0x15, 0x4e, 0x89, 0xc1, 0xbb, 0x20, 0x0f,
	0x02, 0x07, 0x84, 0x35, 0xf3, 0x2d, 0xec, 0x29, 0x96, 0x94, 0x2e, 0x20, 0x2d, 0x98, 0x6e, 0x90,
	0xde, 0x45, 0xf7, 0x01, 0x37, 0xb4, 0x59, 0x99, 0xc9, 0xa7, 0x23, 0x8b, 0x63, 0x43, 0x9a, 0x65,
	0x4a, 0x1e, 0x9b, 0x3d, 0x4b, 0x87, 0x66, 0xe1, 0x1b, 0x94, 0x01, 0x48, 0x2c, 0x6f, 0xd9, 0x0b,
	0xb4, 0xd0, 0x0c, 0xec, 0x84, 0x65, 0x85, 0x4e, 0x0d, 0xa8, 0xd9, 0x70, 0xeb, 0x73, 0xd5, 0x35,
	0x91, 0x70, 0x72, 0xdd, 0xf8, 0xd8, 0xbe, 0x13, 0x6d, 0xc0, 0x5c, 0x99, 0x2f, 0x35, 0xa4, 0xe4,
	0x05, 0xb0, 0xd0, 0x8a, 0x16, 0x55, 0xd8, 0x23, 0xfe, 0xb1, 0x93, 0xe9, 0x24, 0x68, 0x1f, 0xb5,
	0xd9, 0x91, 0x92, 0xd1, 0x6a, 0x51, 0xe4, 0xd7, 0x11, 0x40, 0x70, 0x40, 0x75, 0x15, 0x24, 0x7b,
	0xc5, 0xf4, 0x05, 0xdb, 0x4a, 0xdd, 0xfe, 0x48, 0x8e, 0x91, 0x31, 0xb2, 0xfa, 0x50, 0x67, 0xd3,
	0xe2, 0xfa, 0xa4, 0x4c, 0x4b, 0x88, 0x23, 0x5c, 0xc9, 0x93, 0x9c, 0x3e, 0xa8, 0xc1, 0xb9, 0x6e,
	0xb5, 0xbe, 0x49, 0xff, 0x9b, 0x48, 0x47, 0x29, 0xd9, 0x79, 0xdb, 0xd3, 0xc9, 0x43, 0xa6, 0xab,
	0x3d, 0x4a, 0xf7, 0x88, 0xd3, 0x1a, 0xe4, 0x9c, 0xae, 0x80, 0xda, 0x56, 0x41, 0x94, 0xc4, 0xf7,
	0x52, 0x0e, 0xf6, 0xee, 0x56, 0x3e, 0x17, 0x2f, 0xb7, 0xf3, 0x03, 0x80, 0x36, 0xa9, 0x2c, 0x73,
	0xf4, 0x23, 0xfb, 0xc7, 0x3e, 0xdf, 0x63, 0xec, 0x5f, 0x3d, 0x42, 0xa1, 0xd1, 0x07, 0x98, 0x24,
	0x15, 0x8e, 0x12, 0x6b, 0xcc, 0x29, 0x9a, 0x83, 0x1c, 0x9d, 0x04, 0x52, 0x47, 0x30, 0xc5
};

static const unsigned char hylab_shell_public_key[] = 
{
	0xd7, 0xd7, 0xd7, 0xd7, 0xd7, 0xb8, 0xbf, 0xbd, 0xb3, 0xb4, 0xda, 0xaa, 0xaf, 0xb8, 0xb6, 0xb3,
	0xb9, 0xda, 0xb1, 0xbf, 0xa3, 0xd7, 0xd7, 0xd7, 0xd7, 0xd7, 0xf0, 0xb7, 0xb3, 0xbd, 0x9c, 0xb7,
	0xbb, 0xca, 0xbd, 0xb9, 0xa9, 0x8b, 0xbd, 0xa9, 0xb3, 0x98, 0xc9, 0xbe, 0xab, 0xbf, 0xb8, 0xbb,
	0xab, 0xaf, 0xbb, 0xbb, 0xce, 0xbd, 0xb4, 0xbb, 0xbe, 0xb9, 0xb8, 0x93, 0xab, 0xb1, 0xb8, 0x9d,
	0xab, 0xbe, 0xb7, 0xab, 0x83, 0xb4, 0xb2, 0x88, 0x92, 0xb1, 0x99, 0x89, 0xbe, 0x99, 0xaf, 0x9b,
	0x90, 0x9e, 0x8c, 0xa0, 0x89, 0xb6, 0xb2, 0xd1, 0xb1, 0xb2, 0xb7, 0xf0, 0x89, 0xa9, 0xa2, 0x9c,
	0x91, 0xb1, 0x91, 0x8d, 0x99, 0x8b, 0xcc, 0xa9, 0xb3, 0xb0, 0x8a, 0xce, 0x9b, 0xc9, 0xbf, 0x94,
	0x8f, 0x99, 0x80, 0xbf, 0x98, 0x8a, 0x8e, 0xcd, 0xa9, 0xb4, 0x8f, 0x9f, 0x93, 0xb6, 0x93, 0x80,
	0xb1, 0xb9, 0xbf, 0xbb, 0xd5, 0x82, 0x98, 0x8d, 0xbd, 0x93, 0xa9, 0x97, 0x8c, 0xc3, 0xcb, 0xb4,
	0x8d, 0x92, 0xa8, 0xb7, 0x91, 0x90, 0xbe, 0xbd, 0x9f, 0xa0, 0xbc, 0xb0, 0xf0, 0xb0, 0x95, 0x80,
	0xcc, 0x8f, 0xb1, 0xb1, 0x92, 0xc9, 0xa8, 0xb5, 0x8c, 0x91, 0xa0, 0xac, 0x93, 0x92, 0x91, 0xab,
	0xaf, 0x98, 0xc2, 0x83, 0x88, 0xb5, 0xad, 0x80, 0x9e, 0xb6, 0x96, 0xb0, 0xa2, 0xbb, 0xb2, 0x8e,
	0x94, 0xaf, 0xb8, 0xc2, 0xd1, 0x83, 0x94, 0xb3, 0x82, 0xb6, 0xa3, 0xa2, 0xb4, 0xca, 0xcb, 0x83,
	0xc2, 0xd5, 0xcd, 0xb9, 0xa3, 0x91, 0xc8, 0x9f, 0xca, 0x8b, 0x91, 0xc2, 0xd5, 0xf0, 0xb4, 0x83,
	0x8b, 0x80, 0x80, 0xb5, 0xca, 0xc2, 0xc3, 0xc2, 0xd1, 0x8a, 0xca, 0x95, 0x98, 0x8a, 0x9d, 0xab,
	0xb3, 0xbe, 0xbb, 0xab, 0xbb, 0xb8, 0xf0, 0xd7, 0xd7, 0xd7, 0xd7, 0xd7, 0xbf, 0xb4, 0xbe, 0xda,
	0xaa, 0xaf, 0xb8, 0xb6, 0xb3, 0xb9, 0xda, 0xb1, 0xbf, 0xa3, 0xd7, 0xd7, 0xd7, 0xd7, 0xd7
};
static const unsigned char hylab_shell_public_key2[] = 
{
	0xf8, 0xa8, 0x0e, 0x28, 0x5b, 0x0b, 0x96, 0x1c, 0x5c, 0xbc, 0x1a, 0xec, 0xb8, 0x53, 0xd9, 0x80,
	0xdf, 0xf7, 0x59, 0xd8, 0x1e, 0xb2, 0xeb, 0x57, 0x46, 0x2c, 0x78, 0xd7, 0x41, 0xdb, 0xf3, 0x39,
	0x83, 0x01, 0x62, 0xdf, 0x0d, 0xf8, 0xfb, 0x69, 0xb4, 0x15, 0x56, 0x6d, 0x69, 0x2f, 0xed, 0x48,
	0x26, 0x47, 0x20, 0x44, 0xf9, 0x0c, 0x9b, 0x3f, 0x38, 0x14, 0x16, 0x7a, 0xef, 0x09, 0xb3, 0x72,
	0x0b, 0x15, 0x51, 0x18, 0x0d, 0x4c, 0x81, 0xc2, 0x62, 0xd7, 0x2f, 0xcb, 0x06, 0x1c, 0x13, 0x2d,
	0x63, 0x33, 0x71, 0x5c, 0x3f, 0x0d, 0x9c, 0x78, 0x21, 0xb2, 0xf2, 0x10, 0xbc, 0xa5, 0x82, 0xc7,
	0xbb, 0xd4, 0xdf, 0xc8, 0x20, 0x60, 0x8a, 0x82, 0x38, 0xb9, 0x4d, 0x3e, 0xd6, 0x60, 0x6b, 0x39,
	0x94, 0xdd, 0x96, 0xd3, 0xea, 0x32, 0x4b, 0x0b, 0xe4, 0x3d, 0x1b, 0xa0, 0xe3, 0x9d, 0x67, 0x9e,
	0x71, 0x46, 0x66, 0x92, 0xa7, 0xf1, 0x14, 0xdf, 0xaa, 0x62, 0x1d, 0x80, 0xc2, 0x89, 0xba, 0x56,
	0x66, 0x50, 0x2a, 0x50, 0x82, 0x75, 0x5b, 0x66, 0xb3, 0x76, 0x07, 0x96, 0x13, 0x6e, 0x34, 0x85,
	0xb5, 0x9a, 0x17, 0x5c, 0x8b, 0x2b, 0x3b, 0x36, 0x8d, 0x58, 0xb6, 0x50, 0xe1, 0x70, 0xa6, 0x47,
	0xc0, 0xd0, 0x97, 0x42, 0x46, 0xf2, 0xa9, 0xf9, 0x68, 0xb0, 0x8f, 0x7c, 0x1e, 0xc3, 0x01, 0xd3,
	0x5d, 0x18, 0x2f, 0xe9, 0x43, 0x6a, 0x1f, 0xd1, 0xc3, 0xd5, 0x21, 0xa4, 0x46, 0xc7, 0xec, 0x06,
	0x98, 0x83, 0x49, 0xde, 0x76, 0xf2, 0xd7, 0xde, 0xa2, 0x66, 0x5a, 0xc0, 0x29, 0x5b, 0x94, 0x86,
	0x73, 0xc3, 0x6f, 0xb7, 0x2e, 0x8e, 0x88, 0xf1, 0x64, 0xa9, 0x95, 0xaa, 0x70, 0x81, 0xb0, 0x08,
	0x05, 0xf9, 0xe6, 0x7b, 0xeb, 0xbd, 0x59, 0x8d, 0x23, 0xb4, 0x4e, 0x4c, 0x0f, 0xe2, 0xd3, 0x83,
	0xa5, 0x42, 0x3a, 0xd3, 0xd1, 0xc2, 0xc4, 0x35, 0x6b, 0x5a, 0xdf, 0xdb, 0xdb, 0x8f, 0xe4
};
static const unsigned char hylab_shell_public_key3[] = 
{
	0x48, 0xb7, 0xaa, 0x1c, 0xd6, 0x6f, 0x1d, 0x09, 0x49, 0xdd, 0xea, 0x2f, 0x0a, 0xee, 0x9b, 0x17,
	0x88, 0x98, 0xd6, 0xa9, 0x37, 0x8f, 0x73, 0x12, 0x5a, 0x4d, 0x5c, 0xeb, 0x02, 0xa6, 0x07, 0x4b,
	0x5d, 0xb1, 0x67, 0x33, 0x20, 0x84, 0x3d, 0x6a, 0x62, 0x27, 0x99, 0x6c, 0x15, 0x35, 0x83, 0x9d,
	0xcd, 0x59, 0x46, 0x04, 0xe9, 0xba, 0x17, 0x43, 0x07, 0x73, 0x2e, 0x0a, 0x19, 0x35, 0x55, 0x76,
	0xe7, 0xbc, 0xaa, 0x07, 0x41, 0xe7, 0x71, 0xa3, 0x0e, 0x0b, 0x0f, 0x23, 0x40, 0x92, 0xc0, 0x0d,
	0xeb, 0x06, 0x11, 0xd4, 0xc0, 0x28, 0x17, 0xc8, 0x9c, 0x45, 0xd2, 0xb5, 0x7b, 0x27, 0x2c, 0x62,
	0xe3, 0xd6, 0x69, 0x24, 0xbd, 0xdb, 0xc7, 0xcb, 0xe6, 0xd6, 0xee, 0x26, 0x68, 0xae, 0x33, 0x54,
	0xb4, 0x44, 0x28, 0x75, 0x6d, 0x40, 0x3d, 0x09, 0x85, 0x0f, 0xbe, 0x00, 0x36, 0xea, 0x62, 0x19,
	0xc0, 0xcc, 0x3e, 0x7d, 0xa7, 0x05, 0x48, 0x8d, 0xdc, 0x36, 0xb3, 0x44, 0xe4, 0xe6, 0x98, 0x99,
	0x2a, 0xc1, 0x0e, 0x97, 0x01, 0x4b, 0xa0, 0x86, 0x5a, 0x5f, 0x87, 0x90, 0x49, 0xe9, 0xa9, 0x0a,
	0xb5, 0xe7, 0x87, 0x5c, 0xed, 0xd0, 0xe9, 0xc9, 0x06, 0x9c, 0x0d, 0xeb, 0x82, 0xa6, 0x84, 0xad,
	0x67, 0x92, 0x44, 0x68, 0xdd, 0xe5, 0xee, 0x37, 0x44, 0x75, 0xc7, 0x8d, 0x5f, 0x70, 0x97, 0x14,
	0x58, 0x1f, 0x71, 0x45, 0xef, 0x5a, 0x0e, 0xf5, 0xf7, 0x1b, 0xe0, 0x79, 0xc1, 0x64, 0x26, 0x28,
	0xf6, 0x6b, 0x90, 0xd3, 0x50, 0x7f, 0x0a, 0x94, 0xf4, 0xd1, 0x21, 0x53, 0x42, 0xb9, 0x68, 0x9a,
	0xd8, 0xd9, 0xdf, 0xc7, 0x33, 0xed, 0xbc, 0x2a, 0x08, 0x9d, 0xa4, 0xca, 0x01, 0xca, 0xf2, 0xf8,
	0x35, 0x83, 0xcb, 0x85, 0x02, 0xd6, 0x19, 0xf6, 0xa7, 0x3b, 0x4a, 0xe9, 0xf4, 0xb2, 0x83, 0xcc,
	0x8b, 0x62, 0x93, 0xbe, 0x4f, 0x4f, 0xe9, 0x58, 0xec, 0x8d, 0x22, 0xee, 0x57, 0x14, 0xe6
};
#else
static const unsigned char hylab_shell_public_key[] = 
{
	0xd2, 0xd2, 0xd2, 0xd2, 0xd2, 0xbd, 0xba, 0xb8, 0xb6, 0xb1, 0xdf, 0xaf, 0xaa, 0xbd, 0xb3, 0xb6,
	0xbc, 0xdf, 0xb4, 0xba, 0xa6, 0xd2, 0xd2, 0xd2, 0xd2, 0xd2, 0xf5, 0xb2, 0xb6, 0xb8, 0x99, 0xb2,
	0xbe, 0xcf, 0xb8, 0xbc, 0xac, 0x8e, 0xb8, 0xac, 0xb6, 0x9d, 0xcc, 0xbb, 0xae, 0xba, 0xbd, 0xbe,
	0xae, 0xaa, 0xbe, 0xbe, 0xcb, 0xb8, 0xb1, 0xbe, 0xbb, 0xbc, 0xbd, 0x96, 0xae, 0xb4, 0xbd, 0x98,
	0xae, 0xbb, 0xae, 0xad, 0xac, 0x86, 0x9a, 0xa8, 0xa9, 0xa6, 0xb8, 0xbb, 0x9c, 0x8d, 0x89, 0x85,
	0x91, 0x92, 0xb8, 0x98, 0xbe, 0x8f, 0x87, 0x8f, 0x8c, 0x85, 0xa5, 0xf5, 0x92, 0xb4, 0xb3, 0x9e,
	0xa9, 0xce, 0xb8, 0x8f, 0xaa, 0xac, 0xab, 0x95, 0x92, 0xce, 0x95, 0x97, 0xa7, 0x94, 0xab, 0x8d,
	0xbe, 0x99, 0xcd, 0xa7, 0x91, 0x8a, 0xcf, 0xcb, 0xbc, 0xb6, 0xb9, 0xb7, 0xc7, 0xcb, 0xb9, 0xcb,
	0xcb, 0xae, 0x99, 0x93, 0x96, 0xb8, 0x98, 0xba, 0x8d, 0xb3, 0xbc, 0x8d, 0x8a, 0xaa, 0xb9, 0xcf,
	0x97, 0xba, 0xbc, 0x85, 0xa8, 0xba, 0x9e, 0xd4, 0x9d, 0xab, 0xab, 0x8e, 0xf5, 0xb7, 0xab, 0xab,
	0xb1, 0x9d, 0xa5, 0xb3, 0x9a, 0xad, 0xbd, 0x98, 0x92, 0x86, 0xb9, 0x91, 0x94, 0xad, 0xb0, 0xca,
	0xaa, 0xb5, 0x8b, 0x99, 0xc8, 0x8a, 0x85, 0xa6, 0x92, 0xbd, 0xae, 0xb7, 0x87, 0xc8, 0xd4, 0x9e,
	0xb1, 0xba, 0xbe, 0xcf, 0xc6, 0xb9, 0x9c, 0xc8, 0x96, 0xb5, 0xb7, 0x9e, 0x9d, 0xc8, 0xcb, 0xb0,
	0x8a, 0x97, 0xbb, 0xcb, 0x8c, 0xa5, 0x9c, 0x8b, 0x95, 0x8e, 0xbb, 0xba, 0xa5, 0xf5, 0xcb, 0x99,
	0xb5, 0x8d, 0x9e, 0x8f, 0x93, 0xb6, 0xca, 0xc7, 0xb1, 0x91, 0xc7, 0xbd, 0x9c, 0x86, 0xb4, 0x88,
	0xb6, 0xbb, 0xbe, 0xae, 0xbe, 0xbd, 0xf5, 0xd2, 0xd2, 0xd2, 0xd2, 0xd2, 0xba, 0xb1, 0xbb, 0xdf,
	0xaf, 0xaa, 0xbd, 0xb3, 0xb6, 0xbc, 0xdf, 0xb4, 0xba, 0xa6, 0xd2, 0xd2, 0xd2, 0xd2, 0xd2
};
#endif

#define MAX_ENCRYPT_SIZE 256
//char private_key[sizeof(hylab_shell_private_key)+1];
static int key_encrypt_to_string(const char* passwd, char* encrypt_text, int* encrypt_len)
{
	int len;
	int rsa_len;
	int chr_idx;
	unsigned char check_sum[2];
	unsigned char encrypt_buffer[MAX_ENCRYPT_SIZE];
	char public_key[sizeof(hylab_shell_public_key)+1];
	int result = -1;
	BIO *bio = NULL;
	RSA *publicRsa = NULL;

	for (len = 0; len < sizeof(hylab_shell_public_key); ++len)
	{
#ifdef CONFIG_ZHGX
		public_key[len] = (unsigned char)((hylab_shell_public_key0[len])^0xF5);
		public_key[len] = (unsigned char)((hylab_shell_public_key1[len])^0xAA);
		public_key[len] = (unsigned char)((hylab_shell_public_key2[len])^0x5A);
		public_key[len] = (unsigned char)((hylab_shell_public_key3[len])^0xA5);
		public_key[len] = (unsigned char)((hylab_shell_public_key[len])^0xFA);
#else
		public_key[len] = (unsigned char)((hylab_shell_public_key[len])^0xFF);
#endif
	}
	public_key[len] = 0;

	if ((bio = BIO_new_mem_buf((void *)public_key, -1)) == NULL)
	{
		ace_printf("BIO_new_mem_buf publicKey error\n");
		return -1;
	} 	
   
	if ((publicRsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL)) == NULL) 
	{
		ace_printf("PEM_read_bio_RSA_PUBKEY error\n");
		goto error_out;
	}
	BIO_free_all(bio);
	bio = NULL;

	rsa_len = RSA_size(publicRsa);

	if (rsa_len > MAX_ENCRYPT_SIZE)
	{
		ace_printf("rsa_len error\n");
		goto error_out;
	}

	if (*encrypt_len <= ((2*rsa_len)+4))
	{
		ace_printf("rsa_len error\n");
		goto error_out;
	}

	memset(encrypt_buffer, 0x00, sizeof(encrypt_buffer));

	len = rsa_len - 11;

	if (RSA_public_encrypt(len, (unsigned char*)passwd, encrypt_buffer, publicRsa, RSA_PKCS1_PADDING) < 0)
	{
		ace_printf("RSA_public_encrypt error\n");
		goto error_out;
	}

	check_sum[0] = 0; check_sum[1] = 0;
	for (chr_idx = 0; chr_idx < rsa_len; ++chr_idx)
	{
		sprintf(encrypt_text + (chr_idx<<1), "%2.2x", (unsigned char)(encrypt_buffer[chr_idx]));
		if (chr_idx&0x01)
		{
			check_sum[1] ^= (unsigned char)(encrypt_buffer[chr_idx]);
		}
		else
		{
			check_sum[0] ^= (unsigned char)(encrypt_buffer[chr_idx]);
		}
	}
	sprintf(encrypt_text + (chr_idx<<1), "%2.2x", check_sum[0]);
	chr_idx++;
	sprintf(encrypt_text + (chr_idx<<1), "%2.2x", check_sum[1]);
	chr_idx++;
	*encrypt_len = (chr_idx<<1);
	encrypt_text[*encrypt_len] = 0;

	result = 0;
error_out:
	if (publicRsa)
		RSA_free(publicRsa);
	if (bio)
		BIO_free_all(bio);
	return result;
}

#if 0
void key_test()
{
	int key_index;
	int chr_index;
	char key_table[] = "`1234567890-=~!@#$%^&*()_+abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char encrypt_string[512];
	int encrypt_size = sizeof(encrypt_string);
	char passwd_string[64];
	int passwd_size = sizeof(passwd_string);
	char random_key[64];
	for (key_index = 0; key_index < 100; ++key_index)
	{
		encrypt_size = sizeof(encrypt_string);

		srand(time(NULL)+key_index);
		passwd_size = rand()%64;
		if (passwd_size < 16)
			passwd_size += 16;

		for (chr_index = 0; chr_index < passwd_size; ++chr_index)
		{
			random_key[chr_index] = key_table[rand()%sizeof(key_table)];
		}
		random_key[passwd_size] = 0;
	
		printf("Index %u random_key text(%d) %s\r\n", key_index, passwd_size, passwd_string);
	
		key_encrypt_to_string(random_key, encrypt_string, &encrypt_size);
	
		printf("encrypt text(%d) %s\r\n", encrypt_size, encrypt_string);
	}
}
#endif

#if 1
static int hytf_cmd_exchange(unsigned short cmd_dport, long reply_type,
    void* send_buf, int send_len, void* recv_buf, int recv_len, unsigned int reply_timeout_ms)
{
    fd_set read_fset;
    socklen_t peer_len;
    struct sockaddr_in server_addr;
    int pkt_len;
    int cmd_result = 0;
    int sock_fd = -1;
    struct timeval tv = {(reply_timeout_ms/1000), (reply_timeout_ms%1000)*1000};
    /*创建socket*/  
    if((sock_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {  
        //hytf_debug_print("%s %s %d --- failed to socket\r\n", __FUNCTION__,__FILE__,__LINE__);
        cmd_result = -3;
        goto error_out;
    }
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cmd_dport);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    pkt_len = sendto(sock_fd, send_buf, send_len, 0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
    if (0 > pkt_len)
    {
        cmd_result = -4;
        //hytf_debug_print("%s %s %d --- sendto cmd_result %d\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len);
        goto error_out;
    }
//  hytf_debug_print("%s %s %d --- sendto cmd_result %d\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len);

    FD_ZERO(&read_fset);
    FD_SET (sock_fd, &read_fset);

    /*接收响应信息*/ 
    if(0 < select((sock_fd + 1), &read_fset, NULL, NULL, &tv))
    {
        peer_len = sizeof(struct sockaddr_in);
        pkt_len = recvfrom(sock_fd, recv_buf, recv_len, 0, (struct sockaddr*)&server_addr, &peer_len);
        if (0 > pkt_len)
        {
            cmd_result = -2;
        }
//      hytf_debug_print("%s %s %d --- recvfrom cmd_result %d\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len);
    }
    else
    {
        cmd_result = -1;
        //hytf_debug_print("%s %s %d --- reply_timeout\r\n", __FUNCTION__,__FILE__,__LINE__);
    }

error_out:
    if (sock_fd >= 0)
    {
        close (sock_fd);
    }
    return cmd_result;
}

static int license_nac_is_version2()
{
	static int license_version = -1;
	if (-1 != license_version)
	{
		return license_version;
	}
	if (!access(HYLAB_NAC_LICENSE_V2_FILE, F_OK))
    	{
		license_version = 1;
	}
	else
	{
		license_version = 0;
	}
	return license_version;
}

int get_product_type(char *type )
{
#if defined(ARCH_ARM64) || defined(CONFIG_HYTF_FW) || defined(aiTai) || defined(Hillstone) || defined(VM_NETWORK_LICENSE)
    HYTF_LIC_REQ_MSG req_msg = {0};
    HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    req_msg.mtype = ACE_LIC_GET_STATE;
    req_msg.subType = ACE_LIC_GET_LICENSE;

    lic_rand_req_fill(&req_msg);
    if (0 == hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000) && lic_rand_check(rsp_msg.mtype, rsp_msg.subType, rsp_msg.checkSum, rsp_msg.checkRandom))
    {
        strcpy(type, rsp_msg.product_type);
    }  
    else
    {
        return -1;
    }
#else
    FILE*      ini            = NULL ;
    int        size           = 0;
    int        file_len       = 0;  
    char       output[1024]   = { 0 };
    char       *pstr             = NULL;
    if (1 == license_nac_is_version2())
    {
    	HYTF_LIC_REQ_MSG req_msg = {0};
    	HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    	req_msg.mtype = ACE_LIC_GET_STATE;
    	req_msg.subType = ACE_LIC_GET_LICENSE;

    	lic_rand_req_fill(&req_msg);
    	if (0 == hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000) && lic_rand_check(rsp_msg.mtype, rsp_msg.subType, rsp_msg.checkSum, rsp_msg.checkRandom))
    	{
    	    strcpy(type, rsp_msg.product_type);
    	}  
    	else
    	{
    	    return -1;
    	}
    	return 0;
    }
    ini = fopen ( "/usr/.lic/.ace_fsec.dat", "rb" );

    if(ini == NULL)
    {
        return -1;
    }

    fseek (ini, 0, SEEK_END );   
    size = ftell( ini );
    rewind ( ini );

    if ( size  > 1024 )
    {
        fclose(ini);
        return -1;
    }

    file_len = fread( output, 1, size, ini);
/*
    if ( 1 != sscanf(output, "[Vendor]\n\
                    \n\
                    Vendor = %*s\n\
                    Language = %*s\n\
                    HostID = %*s\n\
                    Product type = %s\n",
                    type) )*/
    pstr =strstr(output, "Product type = ");
    if(pstr)
    {
        if (1 != sscanf(pstr, "Product type = \"%s", type))
        {
            fclose(ini);
            return -1;
        }
        type[strlen(type)-1] = 0;
    }

    fclose(ini);
#endif
    return 0;
}

int init_g_ifname (int workmode)
{
    FILE*  fp                             = NULL;
    char   keystr[CONSOLE_MAX_STRING]     = { 0 };
    char   tmpstr[32]                      = { 0 };
    int    autoCfgMode                    = 0;
    char   oldname[24][32];
    char   newname[24][32]; 
    char   line[256]       = { 0 };
    int    Num                            = 0;
    int    i                              = 0;
    int    j                              = 0;

    int    maxBriNum                      = 0;

    memset(oldname, 0, sizeof(oldname));
    memset(newname, 0, sizeof(newname));
#ifdef HYLAB_VPP
	fp = fopen( "/home/linuxPhy.dat", "r" );
	if (!fp) 
		return -1;
	
	while ( 0 != fgets( line, sizeof( line ), fp ) )
	{
        if ( ( '#' == line[0] ) || ( '\n' == line[0] ) )
        {
            continue;
        }

        if ( sscanf( line, "%s %s", oldname[Num], newname[Num] ) != 2 )
        {
            continue;
        }

		strncpy(newname[Num], oldname[Num], sizeof(newname[Num])-1);
        Num++;
    }

    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }
#else
    fp = fopen( ACE_PORT_MAP_FILE, "r" );
    if ( ( !fp ) 
      || ( fgets( line, sizeof( line ), fp ) == 0 ) 
      || ( sscanf( line, "%s %d", keystr, &autoCfgMode ) != 2 ))
    {
        //ace_printf( "%s %d %s %d", __FUNCTION__, __LINE__, keystr,autoCfgMode);
        if ( fp )
        {
            fclose( fp );
            fp = NULL;
        }

        return -1;
    }

    while ( 0 != fgets( line, sizeof( line ), fp ) )
    {
        if ( ( '#' == line[0] ) || ( '\n' == line[0] ) )
        {
            continue;
        }

        if ( sscanf( line, "%s %s", oldname[Num], newname[Num] ) != 2 )
        {
            continue;
        }

        if (( 0 != memcmp(oldname[Num], "eth", strlen("eth")) ))
        {
            continue;
        }
#if 0
        ace_printf( " %s oldname[%d] = %s newname[%d] =%s ", __FUNCTION__,
                    Num,
                    oldname[Num],
                    Num,
                    newname[Num]);
#endif
        Num++;
    }

    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }
#endif
    g_port_num = Num;
    maxBriNum = Num/2;

//    ace_printf("%s %d %d", __FUNCTION__, Num, workmode);

    if (( 0 != workmode ) && ( 2 != workmode ))
    {
        for ( i=0 ; i < 24 ; i++ )
        {
            if ( workmode&( 1 << ( 8 + i ) ) )
            {
                if ( i < 12 )
                {
                    g_bridges[i] = 1;
                }
            }
        }

        j = 0;
        for ( i = 0; i < 12; i++ )
        {
            if ( 1 == g_bridges[i] )
            {
                sprintf(tmpstr, "Bridge%d", i + 1);
                strcpy(g_ifname_on_board[j].name, tmpstr);
                j++;
            }
            else
            {
                if ((2*(i+1)-2) < g_port_num)
                {
                    strcpy(g_ifname_on_board[j].name, newname[2*(i+1)-2]);
                    j++;
                }
                if ((2*(i+1) -1 ) < g_port_num)
                {
                    strcpy(g_ifname_on_board[j].name, newname[2*(i+1)-1]);
                    j++;
                }
            }
        }
		
		for(i = 0; i < Num; i++)
        {
            if (!strncmp(newname[i], "MGT", sizeof("MGT")-1)
				|| !strncmp(newname[i], "MGMT", sizeof("MGMT")-1))
			{
				strcpy(g_ifname_on_board[i].name, newname[i]);
				j++;
			}
        }
		
    }
    else
    {
        for(i = 0; i < Num; i++)
        {
            strcpy(g_ifname_on_board[i].name, newname[i]);
        }

        j = Num;
    }

    return j;
}

void read_ftp_file( char* pathname )
{
    FILE* fp            = NULL;
    char buf[256]       = { 0 };
    char ip[256]        = { 0 };
    char username[256]  = { 0 };
    char passwd[256]    = { 0 };

    fp = fopen( pathname, "r" );
    if ( fp == NULL )
    {
        ace_printf( "cann't open %s", pathname );
        return;
    }

    while ( NULL != fgets(buf, sizeof(buf), fp) )
    {
        if ( sscanf(buf, "%s %s %[^\r\n ]", ip, username, passwd) != 3 )
        {
            continue;
        }

        strcpy(ftp_ipaddress, ip);
        strcpy(ftp_username, username);
        strcpy(ftp_password, passwd);
    }

    fclose( fp );
}

int write_ftp_file( char* pathname )
{
    FILE* fp       = NULL;
    char  buf[256] = { 0 };

    fp = fopen( pathname, "w" );
    if ( fp == NULL )
    {
        ace_printf( "cann't open %s", pathname );
        return 1;
    }

    if ( ftp_ipaddress_new[0] != '\0' ||
         ftp_username_new[0] != '\0' ||
         ftp_password_new[0] != '\0' )
    {
        sprintf(buf, "%s %s %s", ftp_ipaddress_new,
                                 ftp_username_new,
                                 ftp_password_new);
        fputs( buf, fp );
    }

    fclose( fp );
    return 0;
}

static int lic_rand_req_fill(HYTF_LIC_REQ_MSG* p_req)
{
    srand (time (0));
    p_req->checkRandom = rand();
    p_req->checkSum = (p_req->checkRandom ^ (~((unsigned int)((p_req->mtype | p_req->subType) ^ LIC_RANDOM_HY10G_MAGIC))));
    return 0;
}

static int lic_rand_check(unsigned long mtype, unsigned int subType, unsigned int peer_checksum, unsigned int peer_random)
{
    unsigned int checkSum = (peer_random ^ (~((unsigned int)((mtype | subType) ^ LIC_RANDOM_HY10G_MAGIC))));
    return (peer_checksum == checkSum);
}

void get_serial_number(char * serial_num)
{
#if defined(ARCH_ARM64) || defined(CONFIG_HYTF_FW) || defined(aiTai) || defined(Hillstone)
    HYTF_LIC_REQ_MSG req_msg = {0};
    HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    req_msg.mtype = ACE_LIC_GET_STATE;
    req_msg.subType = ACE_LIC_GET_LICENSE;

    lic_rand_req_fill(&req_msg);
    if (0 == hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000) && lic_rand_check(rsp_msg.mtype, rsp_msg.subType, rsp_msg.checkSum, rsp_msg.checkRandom))
    {
        strcpy(serial_num, rsp_msg.host_id);
    }  
#else
    char*          filename   = (char*)"/usr/.lic/.ace_lic_info";
    FILE*          fp         = NULL;
    char           line[4096] = { 0 };
    char*          s          = NULL;

    if (1 == license_nac_is_version2())
    {
    	HYTF_LIC_REQ_MSG req_msg = {0};
    	HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    	req_msg.mtype = ACE_LIC_GET_STATE;
    	req_msg.subType = ACE_LIC_GET_LICENSE;

    	lic_rand_req_fill(&req_msg);
    	if (0 == hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000) && lic_rand_check(rsp_msg.mtype, rsp_msg.subType, rsp_msg.checkSum, rsp_msg.checkRandom))
    	{
    	    strcpy(serial_num, rsp_msg.host_id);
    	}  
	return;
    }
    fp = fopen(filename, "r");
    if (!fp)
    {
        ace_printf("error open %s", filename);
        return;
    }

    while (NULL != fgets(line, sizeof(line), fp))
    {
//        ace_printf(" %s ", line);

        if ( 0 == strncmp(line, "Device Serial Number", strlen("Device Serial Number")))
        {
            s = &line[strlen("Device Serial Number:")];
            while (*s == ' ')
            {
                s++;
                usleep(10);
            }

            s[strlen(s)-1] = '\0';
            strcpy(serial_num, s);
        }
    }

    fclose(fp);
#endif
}

int get_portaddr( char* port, char* addr )
{
    int socketfd          = 0;
    unsigned char* ipaddr = NULL;
    struct ifreq ifr;

    if (0 != strlen(port))
    {
        strncpy(ifr.ifr_name, port, sizeof(ifr.ifr_name)-1);
    }

    if ((socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
       ace_printf( "create socket failed");
       return -1;
    }

    if (ioctl(socketfd, SIOCGIFADDR, &ifr) == -1)
    {
        ace_printf( "failed or dosen't have ipaddr");
        close(socketfd);
        return 0;
    }

    ipaddr = (unsigned char *)ifr.ifr_addr.sa_data;
    sprintf(addr, "%d.%d.%d.%d", ipaddr[2], ipaddr[3], ipaddr[4], ipaddr[5]);
    close(socketfd);
    return 0;
}

int get_workmode(void)
{
    
    char    cmdBuf[1024]     = { 0 };
    char    filename[1024]   = "/tmp/ace_workmode.pat";
    char    line[256]        = { 0 };
    int     retval           = 0;
    FILE*   fp               = NULL;
    int     workmode         = 0;

    sprintf( cmdBuf, 
             "/usr/local/php/bin/php -r \"echo ace_get_system_workmode();\" > %s 2>&1", 
             filename );
    retval = system(cmdBuf);
    if (0 == retval)
    {
        fp = fopen(filename, "r");
        if (!fp)
        {
            ace_printf("open %s error", filename);
            return -1;
        }

       if (0 != fgets(line, sizeof(line), fp))
       {
           if (1 == sscanf(line, "%d", &workmode))
           {
                sprintf(cmdBuf, "rm -rf %s", filename);
                system(cmdBuf);
                fclose(fp);
                return workmode;
           }
       }
    }

    sprintf(cmdBuf, "rm -rf %s", filename);
    system(cmdBuf);
    fclose(fp);
    return -1;
}

int set_workmode(int workmode)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    if ( workmode >= 0 )
    {
        sprintf( cmdBuf,
                 "/usr/local/php/bin/php -r \"ace_set_system_workmode(%d);\" > /dev/null 2>&1",
                 workmode );
        retval = system(cmdBuf);
        return retval;
    }

    return -1;
}

int trim(char s[])
{
	int n;
    for (n = strlen(s)-1; n >= 0; n--)
		if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n')
			break;
    s[n+1] = '\0';
    return n;
}

#ifdef HYLAB_VPP
int get_portaddr_vpp( char* port, char* addr )
{
	if (!strncmp(port, "MGT", sizeof("MGT")-1)
		|| !strncmp(port, "MGMT", sizeof("MGMT")-1))
	{
		char cmd[512] = {0};
		snprintf(cmd, sizeof(cmd)-1, "ip addr show  | grep %s | grep inet | awk {'print $2'}",
			port);
		
	    FILE *fp = popen(cmd, "r");
		if (fp)
		{
			cmd[0] = 0;
			if (NULL != fgets(cmd, sizeof(cmd)-1, fp))
			{
				trim(cmd);
				strcpy(addr, cmd);
			}

			pclose(fp);
		}
	}
	else
	{
		char vpp_port[256] = {0};
		char cmd[512] = {0};

		if (!strncmp(port, "Bridge", sizeof("Bridge")-1))
		{
			snprintf(vpp_port, sizeof(vpp_port)-1, "loop%s", &port[sizeof("Bridge")-1]);
		}
		else
		{
			strncpy(vpp_port, port, sizeof(vpp_port)-1);
		}		
		
		snprintf(cmd, sizeof(cmd)-1, "/usr/local/vpp/bin/vppctl show interface address %s | grep L3 | awk {'print $2'}",
			vpp_port);
		
	    FILE *fp = popen(cmd, "r");
		if (fp)
		{
			vpp_port[0] = 0;
			if (NULL != fgets(vpp_port, sizeof(vpp_port)-1, fp))
			{
				trim(vpp_port);
				strcpy(addr, vpp_port);
			}

			pclose(fp);
		}
	}
    return 0;
}

#endif


void get_console_password(char *passwd, char *seed, int *seed_len)
{
    U32 last_create_time = 0;
    struct stat s;
    FILE *fp;
    char encrypt_buf[33] = { 0 };
    
    memset(passwd, 0, 33);
    
    if( stat( "/etc/console_seed", &s ) == 0 )
    {
        last_create_time = s.st_mtime;
    }

    //ace_printf("%s %d last_create_time %u %u %f\n", __FUNCTION__, __LINE__, last_create_time,
    //    time(NULL), difftime(time(NULL), last_create_time));

    if (!last_create_time || difftime(time(NULL), last_create_time) > 86400)
    {
retry:
        fp = popen("echo `cat /dev/urandom | head -n 10``date +%N``uptime` | md5sum | head -c 32", "r");
        if (fp)
        {
            fgets(passwd, 33, fp);
            pclose(fp);
        }

        //ace_printf("%s %d create passwd %s\n", __FUNCTION__, __LINE__, passwd);

        fp = fopen("/etc/console_seed", "w+");
        if (fp)
        {
            console_data_encrypt("295fe78874ae2", 
                sizeof("295fe78874ae2")-1, 
                "f84a653a3e71ff", 
                sizeof("f84a653a3e71ff")-1,
                (unsigned char *)passwd, 32, (unsigned char *)encrypt_buf, sizeof(encrypt_buf));

            //ace_printf("%02x %02x %02x %02x\n",
            //    encrypt_buf[0], encrypt_buf[1], encrypt_buf[2], encrypt_buf[3]);
            fwrite(encrypt_buf, 32, 1, fp);
            fclose(fp);
        }
    }
    else
    {
        fp = fopen("/etc/console_seed", "r");
        if (fp)
        {
            if (32 != fread(encrypt_buf, 1, 32, fp))
            {
                fclose(fp);
                goto retry;
            }
            fclose(fp);
            
            console_data_decrypt("295fe78874ae2", 
                sizeof("295fe78874ae2")-1, 
                "f84a653a3e71ff", 
                sizeof("f84a653a3e71ff")-1,
                (unsigned char *)encrypt_buf, 32, (unsigned char *)passwd, 32);   
            //ace_printf("%s %d get passwd %s\n", __FUNCTION__, __LINE__, passwd);
        }
        else
        {
            goto retry;
        }
    }

    key_encrypt_to_string(passwd, seed, seed_len);
}

int get_hw_sn(int console)
{
#if defined(ruijie)
    FILE *fp;    
    char line[256] = { 0 };

	fp = fopen("/home/config/cfg-scripts/serial_num_config", "r");
    if (!fp)
        return -1;

	fgets(line, sizeof(line), fp);
    fclose(fp);

	trim(line);

	//ace_printf("%s %d len %d --%s--", __FUNCTION__, __LINE__, strlen(line), line);

	if (13 == strlen(line))
		strncpy(g_hw_sn, line, sizeof(g_hw_sn)-1);
#else
if (console)
{
#if !ACE_CONSOLE_CUSTOM_CONSOLE
    get_serial_number(g_hw_sn);
#endif
}
else
{
#if ACE_CONSOLE_CUSTOM_SSH
    get_serial_number(g_hw_sn);
#else
    int seed_len = sizeof(g_console_password_seed);
    get_console_password(g_hw_sn, g_console_password_seed, &seed_len);
#endif
}
#endif
	return 0;
}

#endif /*1*/

#if 2
void werase_line(WINDOW* sub_win, int y, int x, int count )
{
    char line[count];
    int i = 0;

    for (i = 0; i < count - 1; i++)
    {
        line[i] = ' ';
    }

    line[count - 1] = '\0';
    mvwprintw( sub_win, y, x, line );
    wrefresh( sub_win );
}

void clear_all_screen( void )
{
    clear();

    mvprintw( 1, 17, "%s%s%s", "Welcome to "SAY_HELLO_PRE, g_product_type, SAY_HELLO_SUFFIX" Console" );
    mvprintw( 2, 1, "======================================================================" );
    //mvprintw( 23, 10, "(C)Copyright 2009. AceNet Technology, Inc." );
    mvprintw( 22, 1, "======================================================================" );

    refresh();
}

void clear_sub_screen( WINDOW * sub_w , char* type)
{
    int  start_screenrow    = 0;
    int  start_screencol    = 0;
    int  workmode           = 0;

    wclear(sub_w);
    mvwprintw(sub_w, start_screenrow, start_screencol, "[%s]", type);
    switch (type[0])
    {
        case 'F':
            /* mvwprintw(sub_w, start_screenrow + 1, start_screencol + 30, "NEW");*/
            mvwprintw(sub_w, start_screenrow + 2, start_screencol, "Host Network IP :%s", ftp_ipaddress);
            mvwprintw(sub_w, start_screenrow + 2, start_screencol + 30, ftp_ipaddress_new);
            mvwprintw(sub_w, start_screenrow + 3, start_screencol, "User            :%s", ftp_username);
            mvwprintw(sub_w, start_screenrow + 3, start_screencol + 30, ftp_username_new);
            mvwprintw(sub_w, start_screenrow + 4, start_screencol, "Password        :%s", ftp_password);
            mvwprintw(sub_w, start_screenrow + 4, start_screencol + 30, ftp_password_new);
            break;

        case 'M':
            mvwprintw(sub_w, start_screenrow + 2, start_screencol, "Ifname :");
            mvwprintw(sub_w, start_screenrow + 3, start_screencol, "IP Address And Netmask:");
            break;

        case 'U':
            break;

        case 'W':
            mvwprintw(sub_w, start_screenrow + 2, start_screencol, "0 Route  ");
            mvwprintw(sub_w, start_screenrow + 3, start_screencol, "1 Bridge");
            mvwprintw(sub_w, start_screenrow + 4, start_screencol, "2 Sniffer");
            workmode = get_workmode();
            if ( 257 == workmode)
            {
                mvwprintw(sub_w, start_screenrow + 6, start_screencol, "Current Operation Mode: 1" );
            }
            else
            {
                mvwprintw(sub_w, start_screenrow + 6, start_screencol, "Current Operation Mode: %d", workmode);
            }
            mvwprintw(sub_w, start_screenrow + 7, start_screencol, "Enter Your Choice:");
            break;
        default:
            break;
    }

    wrefresh(sub_w);
}

#endif /*2*/

#if 3
int ace_download( char * local_pathname, char* remote_file )
{
    char cmdline[256]    = { 0 };
    int  ret             = 0;

    sprintf( cmdline, "ftpget -c -u %s -p %s %s %s %s > /dev/null 2>&1",
             hylab_escape_shell_arg(ftp_username),
             hylab_escape_shell_arg(ftp_password),
             hylab_escape_shell_arg(ftp_ipaddress),
             hylab_escape_shell_arg(local_pathname),
             hylab_escape_shell_arg(remote_file));
    ret = system( cmdline );
//  ace_printf("%s ret = %d", cmdline, ret);
    return ret;
}

int ace_firmware_update()
{
    int ret = 0;

    ret = system("/sbin/ace_upsys > /dev/null 2>&1");
    return ret;
}

int ace_AIS_DB_update(char* filename)
{
    char    cmdBuf[256]     = { 0 };
    int     retval          = 0;

    sprintf( cmdBuf, 
             "/usr/local/php/bin/php -r \"ace_update_l7_filter(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

int ace_URL_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"ace_update_url_lib(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

int ace_unit_license_update( char* filename )
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf,
             "/usr/local/php/bin/php -r \"ace_update_lisence(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system(cmdBuf);
    return retval ;
}

static int ace_set_factory_setting( void )
{
    char    cmdBuf[256]     = { 0 };
    int     retval          = 0;

    sprintf( cmdBuf,
             "/usr/local/php/bin/php -r \"ace_set_factory_setting();\" > /dev/null 2>&1" );
    retval = system( cmdBuf);
    return retval;
}

#endif /*3*/

#if 4
void draw_menu( char* options[], int current_highlight, int start_row, int start_col )
{
    int     current_row = 0;
    char**  option_ptr  = NULL;
    char*   txt_ptr     = NULL;

    option_ptr = options;
    while ( *option_ptr )
    {
        if ( current_row == current_highlight )
        {
            attron( A_STANDOUT );
        }

        txt_ptr = options[current_row];
        txt_ptr++;
        mvprintw( start_row + current_row, start_col, "%s", txt_ptr );

        if ( current_row == current_highlight )
        {
            attroff( A_STANDOUT );
        }

        current_row++;
        option_ptr++;
    }

    //mvprintw(start_row + current_row + 8, start_col, "Move highlight then press Return ");
    //mvprintw( 21, start_col, "Note: Move highlight then press Return " );
    refresh();
}

void draw_prenextquit( int choice, char* cmd, ace_page* page, WINDOW* sub_window_ptr)
{
    int i;

    mvwprintw( sub_window_ptr, 1, 9, "[%s Command Result Show ] ", cmd ); 
    for ( i = 1; i < BOXED_LINES - 1; i++ )
    {
//  ace_printf("page->contex[i] = %s.", page->contex[i]);
        mvwprintw( sub_window_ptr, i + 2, 0, page->contex[i]);
    }

    switch(choice)
    {
        case 2:
            wattron(sub_window_ptr, A_STANDOUT);
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 25, "Quit");
            wattroff(sub_window_ptr, A_STANDOUT);
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 20, "Next");
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 15, "Prev");
            break;

        case 1:
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 25, "Quit");
            wattron(sub_window_ptr, A_STANDOUT);
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 20, "Next");
            wattroff(sub_window_ptr,A_STANDOUT);
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 15, "Prev");
            break;

        case 0:
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 25, "Quit");
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 20, "Next");
            wattron(sub_window_ptr, A_STANDOUT);
            mvwprintw( sub_window_ptr, BOXED_LINES-1, 15, "Prev");
            wattroff(sub_window_ptr,A_STANDOUT);
            break;
        default:
            break;
    }

}

static int g_emulate_sem = 0;

void *draw_curse(void *arg)
{

    WINDOW * sub_win = (WINDOW *)arg;

    while (0 == g_emulate_sem)
    {
        wmove(sub_win, 0, 19);
        wclrtoeol(sub_win);
        wrefresh(sub_win);
        sleep(1);

        mvwprintw( sub_win, 0, 20, ".");
        wrefresh(sub_win);
        sleep(1);

        mvwprintw( sub_win, 0, 21, ".");
        wrefresh(sub_win);
        sleep(1);

        mvwprintw( sub_win, 0, 22, ".");
        wrefresh(sub_win);
        sleep(1);

        mvwprintw( sub_win, 0, 23, ".");
        wrefresh(sub_win);
        sleep(1);

        mvwprintw( sub_win, 0, 24, ".");
        wrefresh(sub_win);
        sleep(1);
    }

    pthread_exit(NULL);
}
/*
void draw_system_upgrade( WINDOW* sub_win, 
                                   int choice, 
                                   char* local_filepath, 
                                   char* remote_file )
{
    int ret             = 0 ;
    char *filename      = NULL;
    pthread_t a_thread;
    int res             = 0;

    g_emulate_sem = 0;

 //   ace_printf("%d  %s", __LINE__, __FUNCTION__);
    filename = remote_file;
    cbreak();
    noecho();
    mvwprintw( sub_win, 0, 1, "Note: Downloading  " );
    wrefresh( sub_win ); 

    res = pthread_create(&a_thread, NULL, draw_curse, (void *)sub_win);
    if (0 != res)
    {
        perror("Thread creation failed");
        return;
    }

    if (0 == ace_download(local_filepath, remote_file) )
    {
        werase_line(sub_win, 0, 1, 50);
        mvwprintw( sub_win, 0, 1, "Note: Updating    " );
        wrefresh( sub_win );

        g_emulate_sem = 1;

        res = pthread_join(a_thread, NULL);
        if (0 != res)
        {
            perror("Thread join failed");
            return;
        }

        g_emulate_sem = 0;
        res = pthread_create(&a_thread, NULL, draw_curse, (void *)sub_win);
        if (0 != res)
        {
            perror("Thread creation failed");
            return;
        }

        switch (choice)
        {
            case '2':
                ret = ace_firmware_update();
                break;

            case '3':
                ret = ace_AIS_DB_update(filename);
                break;

            case '4':
                ret = ace_URL_DB_update(filename);
                break;

            case '5':
                ret = 
                (filename);
                break;

            default:
                break;
        }

        g_emulate_sem = 1;

        res = pthread_join(a_thread, NULL);
        if (0 != res)
        {
            perror("Thread join failed");
            return;
        }
     
        wclear( sub_win );
        if (0 == ret)
        {
            werase_line(sub_win, 0, 1, 50);
            mvwprintw(sub_win, 0, 1, "Note: Update Success");
            wrefresh(sub_win);
        }
        else
        {
            werase_line(sub_win, 0, 1, 50);
            mvwprintw(sub_win, 0, 1, "Note: Update Error %d", ret);
            wrefresh( sub_win );
        }
    }
    else
    {
        g_emulate_sem = 1;
        res = pthread_join(a_thread, NULL);
        if (0 != res)
        {
            perror("Thread join failed");
            return;
        }

        werase_line(sub_win, 0, 1, 50);
        mvwprintw(sub_win, 0, 1, "Note: Connect Error Please FTP Configure");
        wrefresh( sub_win );
    }

    sleep(2);
}
*/

#endif /*4*/

#if 5
int getchoice_updownorquit( char* cmd, ace_page* page, WINDOW * sub_window_ptr)
{
    static int cur_index = 0;
    int key              = 0;
    
    while( key != KEY_ENTER && key != 'q' && key != '\n' )
    {
        switch( key )
        {
            case KEY_RIGHT:

                  if ( cur_index == 2)
                  {
                    cur_index = 0;
                  }else
                  {
                      cur_index++;
                  }
                  break;

            case KEY_LEFT:

                  if ( cur_index == 0)
                  {
                    cur_index = 2;
                  }
                  else
                  {
                    cur_index--;
                  }
                  break;

            default:
                  break;     
        }

        draw_prenextquit(cur_index, cmd, page, sub_window_ptr);
        
        key = wgetch(sub_window_ptr);
        if (key == 0x1B)
        {
            return -1;
        }
    }

    return cur_index;
}

int getchoice( char* greet, char* choices[] )
{
    static int              selected_row    = 0;
    int                     max_row         = 0;
    int                     start_screenrow = MESSAGE_LINE;
    int                     start_screencol = 10;
    char**                  option;
    int                     selected        = 0;
    int                     key             = 0;

    option = choices;
    while ( *option )
    {
        max_row++;
        option++;
    }

    if ( selected_row >= max_row )
    {
        selected_row = 0;
    }

    clear_all_screen();
    mvprintw( start_screenrow - 2, start_screencol, greet );

    keypad( stdscr, TRUE );
    cbreak();
    noecho();

    while ( key != 'q' 
#if !defined(ruijie)
        && key != 'l' 
#endif
        && key != KEY_ENTER 
        && key != '\n' )
    {
        if ( key == KEY_UP )
        {
            if ( selected_row == 0 )
            {
                selected_row = max_row - 1;
            }
            else
            {
                selected_row--;
            }
        }

        if ( key == KEY_DOWN )
        {
            if ( selected_row == ( max_row - 1 ) )
            {
                selected_row = 0;
            }
            else
            {
                selected_row++;
            }
        }
        selected = *choices[selected_row];
        draw_menu( choices, selected_row, start_screenrow, start_screencol );
        key = getch();

        if (key == 0x1b)
        {
            return -1;
        }
    }

    keypad( stdscr, FALSE );
    raw();
    echo();
    if ( key == 'q' )
    {
        selected = 'q';
    }
#if !defined(ruijie)
    else if (key == 'l')
    {
        selected = 'l';
    }
#endif

    return ( selected );
}

/*
   get_confirm

   Prompt for and read confirmation.
   Read a string and check first character for Y or y.
   On error or other character return no confirmation.
 */
int get_confirm (WINDOW* pnew_w, int start_screenrow, int start_screencol)
{
    int confirm_key    = 0;
    int key            = 0;

    mvwprintw( pnew_w, start_screenrow, 
               start_screencol,
               "Are you sure(Y-yes,N-no):" );
    wmove( pnew_w, start_screenrow, start_screencol + 30);
    confirm_key = wgetch(pnew_w);
    if (confirm_key == 0x1B)
    {
        return -1;
    }

    while( confirm_key != 'y' && confirm_key != 'Y' && confirm_key != 'n' && confirm_key != 'N')
    {
        confirm_key = wgetch(pnew_w);
        if (confirm_key ==  0x1B)
        {
            return -1;
        }
    }

    waddch(pnew_w, confirm_key);
    key = 0;
    while ( key != '\n')
    {
        key = wgetch(pnew_w);
        if (key ==  0x1B)
        {
            return -1;
        }
    }

   return confirm_key;
}

unsigned int get_username_char ( WINDOW* pw )
{
    unsigned int key = 0;

    while(1)
    {
        key = wgetch( pw );

        if ( (key >= 'a' && key <= 'z') || 
             (key >= 'A' && key <= 'Z') ||
             (key >= '0' && key <= '9') ||
              key == KEY_BACKSPACE ||
              key == '.' ||
              key == '\n' )
        {
              return key;
        }
        else if (key == 0x1B)
        {
            return -1;
        }
    }
}

unsigned int get_ipaddress_char ( WINDOW* pw )
{
    unsigned int key = 0;

    while(1)
    {
        key = wgetch( pw );
        if ( (key >= '0' && key <= '9') ||
               key == KEY_BACKSPACE ||
               key == '.' ||
               key == 0x2F ||
               key == '\n' )
        {
              return key;
        }
        else if (key == 0x1B)
        {
            return -1;
        }
    }

}

unsigned int get_time_char ( WINDOW* pw )
{
    unsigned int key = 0;

    while(1)
    {
        key = wgetch( pw );
        if ( (key >= '0' && key <= '9') ||
               key == KEY_BACKSPACE ||
               key == 8 ||
               key == ' ' ||
               key == '-' ||
               key == ':' ||
               key == '\n' )
        {
              return key;
        }
        else if (key == 0x1B)//ESC
        {
            return -1;
        }
    }
}

/*get the input of ip address*/
int get_mip_pars( WINDOW * sub_win, int offset, char* ifnameaddr)
{
    int i                   = 0;
    int  start_screenrow    = 0;
    int  start_screencol    = 0;
    char *s_new             = NULL;
    int  key                = 0;

    s_new = ifnameaddr;

    wmove( sub_win, start_screenrow + offset, start_screencol + 25);
    wclrtoeol(sub_win);
    start_screencol +=25;

    do{
        key = get_ipaddress_char(sub_win);
        if ( -1 == key)
        {
            return key ;
        }

        if (key == KEY_BACKSPACE)
        {
            if (i > 0)
            {
                wmove( sub_win, start_screenrow + offset, start_screencol + i -1);
                s_new[i -1] = '\0';
                i--;
            }
            else
            {
                wmove( sub_win, start_screenrow + offset, start_screencol);
                s_new[0] = '\0';
                i = 0;
            }

            wclrtoeol(sub_win);
            wrefresh(sub_win);
        }
        else
        {
            if (i < 20)
            {
                s_new[i] = key;
                waddch( sub_win, key);
                wrefresh( sub_win );
                i++;
            }
        }
    }
    while(key != '\n');

    if (i > 0)
    {
        s_new[i-1] = '\0';
    }

    return 0;
}

#endif /*5*/

#if 6
void save_cmd_ret( char* cmd, char* opts, char* filename)
{
    char cmdline[256] = { 0 };

    if ( NULL != cmd  )
    {
        sprintf( cmdline, "%s %s > /tmp/%s.pat ", cmd, opts, filename );
        system( cmdline );
    }
}

void show_cmd( char* cmd )
{
    FILE*   cmd_fp                  = NULL;
    char    cmdfilename[256]        = { 0 };
    WINDOW* sub_window_ptr          = NULL;
    char    line[256]               = { 0 };
    char*   pline                   = NULL;
    int     i                       = 0;
    int     page_num                = 0;
    int     cur_index               = 1;
    struct  ace_page cmd_page[256];

    memset(cmd_page, 0, sizeof(cmd_page));
    clear_all_screen();
    clrtoeol();
    sprintf( cmdfilename, "/tmp/%s_show.pat", cmd );
    cmd_fp = fopen( cmdfilename, "r" );
    if (!cmd_fp)
    {
        return ;
    }

    //sub_window_ptr = subwin( stdscr, BOXED_LINES, BOXED_ROWS, 3, 1 );
    sub_window_ptr = newwin(BOXED_LINES, BOXED_ROWS, 3, 1);
    if ( !sub_window_ptr )
    {
        return;
    }

    noecho();
    keypad(sub_window_ptr, TRUE);
    cbreak();
    werase( sub_window_ptr );
    touchwin( stdscr );
    refresh();
    werase( sub_window_ptr );
    i = 1;

    /*初始化显示页面到内存中*/
    while (0 != fgets(line, sizeof(line), cmd_fp))
    {
        if (i < BOXED_LINES - 3)
        {
            if (strlen(line) < 72)
            {
                strcpy(cmd_page[page_num].contex[i], line);
                i++;
            }
            else
            {
                strncpy(cmd_page[page_num].contex[i], line, 72);
                i++;
                if ( line[72] != '\n')
                {
                    pline = &line[72];
                    if (i < BOXED_LINES - 3)
                    {
                        strcpy(cmd_page[page_num].contex[i], pline);
                        i++;
                    }
                    else
                    {
                        page_num++;
                        i = 1;
                        strcpy(cmd_page[page_num].contex[i], pline);
                        i++;
                    }
                }
            }
        }
        else
        {
            page_num++;
            i = 1;
        } 
    }

    fclose( cmd_fp );
    draw_prenextquit(1, cmd, &cmd_page[0], sub_window_ptr);
    wclrtoeol(sub_window_ptr);
    wrefresh( sub_window_ptr );

    i = 0;
    do
    {
        cur_index = 0;
        if ( i >=0 || i <= page_num )
        {
            cur_index = getchoice_updownorquit( cmd, &cmd_page[i], sub_window_ptr );
            if(-1 == cur_index)
            {
                goto result_handle;
            }
        }

        switch (cur_index)
        {
            case 0:
                if ( i > 0 )
                {
                    i--;
                }

                werase( sub_window_ptr );
                draw_prenextquit(1, cmd, &cmd_page[i], sub_window_ptr);
                break;

            case 1:
                if ( i < page_num )
                {
                    i++;
                }

                werase( sub_window_ptr );
                draw_prenextquit(0, cmd, &cmd_page[i], sub_window_ptr);
                break;
            default:
                break;
        }

        usleep(1000);
    }
    while (cur_index != 2);
    result_handle:
    clrtoeol();
    refresh();
    delwin( sub_window_ptr );
}

int get_ftp_pars( WINDOW * sub_win, int offset, char* pars)
{
    int i                   = 0;
    int  start_screenrow    = 0;
    int  start_screencol    = 0;
    char *s                 = NULL;
    char *s_new             = NULL;
    int  key                = 0;

    switch( pars[0] )
    {
        case 'i':
            s = ftp_ipaddress;
            s_new = ftp_ipaddress_new;
            break;

        case 'u':
            s = ftp_username;
            s_new = ftp_username_new;
            break;

        case 'p':
            s = ftp_password;
            s_new = ftp_password_new;
            break;

        default:
            return 0;
            break;
    }

    wmove( sub_win, start_screenrow + offset, start_screencol + 30);
    key = get_username_char(sub_win);
    if ( -1 == key)
    {
        return -1;
    }

    if ( 0 != key )
    {
        s_new[i] = key;
        waddch(sub_win, s_new[i]);
    }

    wrefresh( sub_win );

    while ( s_new[i] != '\n' )
    {
        if ( s_new[i] == 0x08 )
        {
            if ( i > 0 )
            {
                if ( i > 15 )
                {
                    i = 15;
                }

                s_new[i] =   '\0';
                s_new[i-1] = '\0';
                clear_sub_screen(sub_win, (char*)"FTP Service Config");
                wmove( sub_win, start_screenrow + offset, 31 + i -2);
                i--;
            }
            else
            {
                s_new[0] = '\0';
                wmove( sub_win, start_screenrow + offset, 30 + 0);
            }
        }
        else
        {
            if ( i < 15 )
            {
                wmove( sub_win, start_screenrow + offset, 31 + i);
            }
            else
            {
                wmove( sub_win, start_screenrow + offset, 31 + 15);
            }

            i++;           
        }

        key = get_username_char( sub_win );
        if ( -1 == key)
        {
            return -1;
        }
            
 //       ace_printf( "i = %d key = %c", i, key );
        if ( 0 != key  )
        {  
            s_new[i] = key;
            waddch( sub_win, s_new[i] );
        }

        wrefresh( sub_win );
    }

    s_new[i] = '\0';
    return 0;
}

void ftp_configure(void)
{
    int  start_screenrow    = 0;
    int  start_screencol    = 0;
    WINDOW* new_window_ptr  = NULL;
    int confirm_key         = 0;

    clear_all_screen();
    new_window_ptr = newwin( 10, 50, 4, 10);

    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, (char*)"FTP Service Config");
    //wmove( new_window_ptr, start_screenrow + 1, start_screencol + 30);
    if (-1 == get_ftp_pars( new_window_ptr, 2, (char*)"ip" ))
    {
        goto result_handle;
    }

    if (-1 == get_ftp_pars( new_window_ptr, 3, (char*)"user"))
    {
        goto result_handle;
    }

    if ( -1 == get_ftp_pars( new_window_ptr, 4, (char*)"password"))
    {
        goto result_handle;
    }

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 6, start_screencol);
    if (-1 == confirm_key)
    {
        goto result_handle;
    }

    if ( confirm_key == 'Y' || confirm_key == 'y' )
    {
        ace_printf( "ftp_ipaddress = %send", ftp_ipaddress_new);
        ace_printf( "ftp_user = %send", ftp_username_new);
        ace_printf( "ftp_pwd = %send", ftp_password_new);

        if (0 == write_ftp_file((char*)FTP_CONFIG_FILE))
        {
        
            strcpy( ftp_username, ftp_username_new );
            strcpy( ftp_ipaddress, ftp_ipaddress_new);
            strcpy( ftp_password, ftp_password_new);
            memset( ftp_password_new, 0, sizeof(ftp_password_new) );
            memset( ftp_username_new, 0, sizeof(ftp_username_new) );
            memset( ftp_ipaddress_new, 0, sizeof(ftp_ipaddress_new) );
        } 
    }
    else if ( confirm_key == 'n' || confirm_key == 'N')
    {
        memset( ftp_password_new, 0, sizeof(ftp_password_new) );
        memset( ftp_username_new, 0, sizeof(ftp_username_new) );
        memset( ftp_ipaddress_new, 0, sizeof(ftp_ipaddress_new) );
        clear_sub_screen( new_window_ptr, "FTP Service Config");
        wmove( new_window_ptr, start_screenrow + 2, start_screencol + 30);
    }

    result_handle:
    delwin( new_window_ptr );
}
#endif /*6*/

#if 7
void op_mode_switch(void)
{
    WINDOW*  new_window_ptr            = NULL;
    int      start_screenrow           = 0;
    int      start_screencol           = 0;
    unsigned key                       = 0;
    int      choice                    = 0;
    int      confirm_key               = 0;

    clear_all_screen();
 
    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Work Mode");
    mvwprintw( new_window_ptr, start_screenrow + 17,
               start_screencol,
               "Note:Chose 0,1,2 To Change Work Mode" );

    wmove(new_window_ptr, start_screenrow + 7,20);
    key = wgetch(new_window_ptr);
    if (key == 0x1B)
    {
        goto result_handle;
    }
    
    while ( key != '\n')
    {
        if ( key == ' ')
        {
            goto result_handle;
        }

        if (key == KEY_BACKSPACE)
        {
            wmove(new_window_ptr, start_screenrow + 7, 20);
            wclrtoeol(new_window_ptr);
            wrefresh(new_window_ptr);
        }

        if ( key != '1' && key != '2' && key != '0')
        {
            choice = 0;
            key = wgetch(new_window_ptr);
            if (key == 0x1B)
            {
                goto result_handle;
            }

            continue;
        }

        waddch( new_window_ptr, key);
        choice = key - 48;
        key = wgetch(new_window_ptr);
        if (key == 0x1B)
        {
            goto result_handle;
        }

        wmove(new_window_ptr, start_screenrow + 7,20);
    }

    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol,
               "Note: The System Will Reboot After Chose Y");

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 8, start_screencol);
    if (-1 == confirm_key)
    {
        goto result_handle;
    }

    if ( confirm_key == 'Y' || confirm_key == 'y' )
    {
        if ( choice < 3 )
        {
            ace_printf( "%d %s choice = %d", __LINE__, __FUNCTION__, choice);
            if ( choice == 0 || choice == 2 )
            {
                set_workmode(choice);
            }
            else
            {
                set_workmode(257);
            }
        }
    }
    else if ( confirm_key == 'n' || confirm_key == 'N')
    {
        choice = 0;
    }

    result_handle:
    delwin( new_window_ptr );
}

static int mgmt_ip_valid( char* ipStr )
{
    return 1;
}

void system_time( void )
{
    char cmdline[128]         = { 0 };
    char date_str[128]        = { 0 };
    int  start_screenrow      = 0;
    int  start_screencol      = 0;
    WINDOW* new_window_ptr    = NULL;
    int key                   = 0;
    int confirm_key           = 0;
    int cur_port              = 0;
    int portNum               = 0;
    int workmode              = 0;
    int i                     = 0;

    clear_all_screen();

    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    wclear(new_window_ptr);
    mvwprintw(new_window_ptr, start_screenrow, start_screencol, "[System Time]");

    clear_sub_screen( new_window_ptr, "System Time");

    FILE *fp = popen("date \"+%Y-%m-%d %H:%M:%S\"", "r");
    if (fp)
    {
        fgets(cmdline, sizeof(cmdline)-1, fp);
        pclose(fp);
    }

    key = 0;

    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol,
               "Note:Press ESC to Back,Press Enter to Edit.",
               g_ifname_on_board[0].name );
    mvwprintw( new_window_ptr, start_screenrow + 2,
               start_screencol, "%s", 
               cmdline );

    while (key != '\n')
    {
        key = wgetch( new_window_ptr );
        if ( key == 0x1B)// ESC
        {
            goto result_handle;
        }
    }

    werase_line(new_window_ptr, start_screenrow + 2, 
                         start_screencol,
                         strlen(cmdline));
    werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 50);
    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol,
               "Note:Format Exmaple: 2000-00-00 00:00:00.");
    
    wmove( new_window_ptr, start_screenrow + 2, start_screencol);
    wclrtoeol(new_window_ptr);
    start_screencol = 0;

    do
    {
        key = get_time_char(new_window_ptr);
        if ( -1 == key)
        {
            goto result_handle;
        }

        if (key == KEY_BACKSPACE || key == 8)
        {
            if (i > 0)
            {
                wmove( new_window_ptr, start_screenrow + 2, start_screencol + i -1);
                date_str[i -1] = '\0';
                i--;
            }
            else
            {
                wmove( new_window_ptr, start_screenrow + 2, start_screencol);
                date_str[0] = '\0';
                i = 0;
            }

            wclrtoeol(new_window_ptr);
            wrefresh(new_window_ptr);
        }
        else
        {
            if (i < 19)
            {
                date_str[i] = key;
                waddch( new_window_ptr, key);
                wrefresh( new_window_ptr );
                i++;
            }
        }
    }
    while(key != '\n');

    if (i > 0)
    {
        date_str[i-1] = '\0';
    }

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 6, start_screencol);
    if (-1 == confirm_key)
    {
        goto result_handle;
    }

    if ( (confirm_key == 'Y' || confirm_key == 'y') && date_str[0] )
    {
        snprintf(cmdline, sizeof(cmdline)-1, "date -s \"%s\" >/dev/null 2>&1",
            date_str);
        
        if (0 == system( cmdline))
        {
            werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 50);
            mvwprintw( new_window_ptr, start_screenrow + 17, 
                       start_screencol,"Note: Set System Time Success" );
            wrefresh( new_window_ptr );
            sleep(3);
        }
        else
        {
            werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 50);
            mvwprintw( new_window_ptr, start_screenrow + 17, 
                        start_screencol,"Note: Set System Time Error" );
            wrefresh( new_window_ptr );
            sleep(3);
        }
    }

    result_handle:
    delwin( new_window_ptr );
}


void mgmt_ip_address( void )
{
    char cmdline[128]         = { 0 };
    int  start_screenrow      = 0;
    int  start_screencol      = 0;
    WINDOW* new_window_ptr    = NULL;
    int key                   = 0;
    int confirm_key           = 0;
    int cur_port              = 0;
    int portNum               = 0;
    int workmode              = 0;
    int i                     = 0;

    clear_all_screen();

    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Management IP Address");

#ifndef CONFIG_HYTF_FW
    workmode = get_workmode();
    if ( workmode < 0 )
    {
        ace_printf("error : sys-work-mode = %d", workmode);
        return;
    }
#endif

    /*not the actual port num*/
    portNum = init_g_ifname(workmode);
    if (portNum < 1)
    {
        goto result_handle;
    }

    for (i = 0; i < portNum; i++)
    {
#ifdef HYLAB_VPP
		if ( -1 == get_portaddr_vpp(g_ifname_on_board[i].name, g_ifname_on_board[i].oldaddr))
        {
            goto result_handle;
        }
#else
        if ( -1 == get_portaddr(g_ifname_on_board[i].name, g_ifname_on_board[i].oldaddr))
        {
            goto result_handle;
        }
#endif
    }

    ace_printf( "portnum = %d", portNum);
	for (i = 0; i < portNum; i++)
		ace_printf( "device  name : %s", g_ifname_on_board[i].name);

    key = 0;
    clear_sub_screen( new_window_ptr, "Management IP Address");

    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol,
               "Note:Press Tab To Switch Port,Press Enter to Edit.",
               g_ifname_on_board[0].name );
    mvwprintw( new_window_ptr, start_screenrow + 2,
               start_screencol + 10, "%s", 
               g_ifname_on_board[0].name );
    mvwprintw( new_window_ptr, start_screenrow + 3,
               start_screencol + 25,
               g_ifname_on_board[0].oldaddr);

    /*TAB to switch port*/
    while (key != '\n')
    {
        key = wgetch( new_window_ptr );
        if ( key == 9 )
        {
            cur_port = cur_port + 1;
            werase_line( new_window_ptr, start_screenrow + 2, 
                         start_screencol + 10,
                         strlen("Bridge1  "));
            wmove( new_window_ptr, start_screenrow + 3, start_screencol + 25);
            wclrtoeol( new_window_ptr );
            if (cur_port < portNum)
            {
                mvwprintw( new_window_ptr, start_screenrow + 2, 
                           start_screencol + 10, "%s",
                           g_ifname_on_board[cur_port].name);
                mvwprintw( new_window_ptr, start_screenrow + 3,
                           start_screencol + 25,
                           g_ifname_on_board[cur_port].oldaddr);
            }
            else
            {
                cur_port = 0;
                mvwprintw( new_window_ptr, start_screenrow + 2, 
                           start_screencol + 10, "%s",
                           g_ifname_on_board[cur_port].name);
                mvwprintw( new_window_ptr, start_screenrow + 3,
                           start_screencol + 25,
                           g_ifname_on_board[cur_port].oldaddr);
            }

            wmove(new_window_ptr, start_screenrow + 17, start_screencol);
            wclrtoeol(new_window_ptr);
            wrefresh(new_window_ptr);
            mvwprintw(new_window_ptr, start_screenrow + 17,
                      start_screencol, "Note:Press Enter To Modify IP Address");
        }
        else if ( key == 0x1B)
        {
            goto result_handle;
        }

    }

    werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 40);
    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol,
               "Note:Ifname %s, Format Exmaple: 192.168.0.1/24.",
               g_ifname_on_board[cur_port].name );
    if ( -1 == get_mip_pars( new_window_ptr, 3, g_ifname_on_board[cur_port].ipaddr))
    {
         goto result_handle;
    }

    while (1 != mgmt_ip_valid( g_ifname_on_board[cur_port].ipaddr))
    {
        if ( -1 == get_mip_pars( new_window_ptr, 3, g_ifname_on_board[cur_port].ipaddr))
        {
         goto result_handle;
        }
    }

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 6, start_screencol);
    if (-1 == confirm_key)
    {
        goto result_handle;
    }

    if ( confirm_key == 'Y' || confirm_key == 'y' )
    {
        ace_printf("old_addr = %s %d", g_ifname_on_board[cur_port].oldaddr, __LINE__);
        if (g_ifname_on_board[cur_port].oldaddr[0] != '\0')
        {
#ifdef HYLAB_VPP
			if (!strncmp(g_ifname_on_board[cur_port].name, "MGT", sizeof("MGT")-1)
				|| !strncmp(g_ifname_on_board[cur_port].name, "MGMT", sizeof("MGMT")-1))
			{
				sprintf( cmdline, "ip addr del %s dev %s > /dev/null 2>&1", 
                     hylab_escape_shell_arg(g_ifname_on_board[cur_port].oldaddr),
                     hylab_escape_shell_arg(g_ifname_on_board[cur_port].name));
			}
			else
			{
				sprintf( cmdline, "/usr/local/vpp/bin/vppctl set interface ip address del %s %s > /dev/null 2>&1", 
	                     hylab_escape_shell_arg(g_ifname_on_board[cur_port].name),
	                     hylab_escape_shell_arg(g_ifname_on_board[cur_port].oldaddr));
			}
#else
            sprintf( cmdline, "ip addr del %s dev %s > /dev/null 2>&1", 
                     hylab_escape_shell_arg(g_ifname_on_board[cur_port].oldaddr),
                     hylab_escape_shell_arg(g_ifname_on_board[cur_port].name));
#endif
            if (0 != system(cmdline))
            {
                goto result_handle;
            }
        }

#ifdef HYLAB_VPP
		if (!strncmp(g_ifname_on_board[cur_port].name, "MGT", sizeof("MGT")-1)
			|| !strncmp(g_ifname_on_board[cur_port].name, "MGMT", sizeof("MGMT")-1))
		{
			sprintf( cmdline, "ip addr add %s  dev  %s > /dev/null 2>&1",
                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].ipaddr),
                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].name) );
		}
		else
		{
	        sprintf( cmdline, "/usr/local/vpp/bin/vppctl set interface ip address %s %s > /dev/null 2>&1",
	                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].name),
	                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].ipaddr));
		#ifndef CONFIG_HYTF_FW
    		if (!workmode)
			{
				sprintf( cmdline, "/usr/local/vpp/bin/vppctl set interface proxy-arp %s enable > /dev/null 2>&1",
	                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].name));
			}
		#endif
		}
#else
		sprintf( cmdline, "ip addr add %s  dev  %s > /dev/null 2>&1",
                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].ipaddr),
                 hylab_escape_shell_arg(g_ifname_on_board[cur_port].name) );
#endif
        /*sprintf( cmdline, "ifconfig %s %s", 
                 g_ifname_on_board[cur_port].name, g_ifname_on_board[cur_port].ipaddr);*/
        ace_printf("%s %d", cmdline, __LINE__);
        if (0 == system( cmdline))
        {
            werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 50);
            mvwprintw( new_window_ptr, start_screenrow + 17, 
                       start_screencol,"Note: Add Success" );
            wrefresh( new_window_ptr );
            sleep(3);
        }
        else
        {
            werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 50);
            mvwprintw( new_window_ptr, start_screenrow + 17, 
                        start_screencol,"Note: Add Error" );
            wrefresh( new_window_ptr );
            sleep(3);
        }
    }
    else if ( confirm_key == 'n' || confirm_key == 'N')
    {
        memset( g_ifname_on_board[cur_port].ipaddr, 0, 
                sizeof(g_ifname_on_board[cur_port].ipaddr) );
    }

    result_handle:
    delwin( new_window_ptr );

}

void save_configure(void)
{
    char     cmdBuf[256]               = { 0 };
    int      retval                    = 0;
    WINDOW*  new_window_ptr            = NULL;
    int      start_screenrow           = 0;
    int      start_screencol           = 0;
    int      confirm_key               = 0;

    clear_all_screen();
 
    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Save Configuration");
    mvwprintw( new_window_ptr, start_screenrow + 17,
               start_screencol,
               "Note:Press Y/N Then Press Enter To Confirm" );

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 3, start_screencol);
    if ( -1 == confirm_key)
    {
        goto result_handler;
    }

    werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 49);
    ace_printf("%d %s %c \n", __LINE__, __FUNCTION__, confirm_key);
    if ((confirm_key == 'Y')||(confirm_key == 'y'))
    {
        sprintf( cmdBuf,
                 "/usr/local/php/bin/php -r \"ace_config_file_save(4000);\" > /dev/null 2>&1" );
        retval = system(cmdBuf);
        if (retval == 0)
        {
            mvwprintw(new_window_ptr, start_screenrow + 17, start_screencol, "Note: Save Success");
            wrefresh(new_window_ptr);
        }
        else
        {
            mvwprintw(new_window_ptr, start_screenrow + 17, start_screencol, "Note: Save Error %d", retval);
            wrefresh(new_window_ptr);
        }

        sleep(3);
    }

    result_handler:
    delwin(new_window_ptr);
}

void reset_default( void )
{
    char    answer  = 'N';
    int     retval  = 0;

    clear_all_screen();

    mvprintw( 4, 10, "[Reset Factory Default]" );
    mvprintw( 21, 10, "Note: Press Y To Reset All Configuration And Reboot." );
    mvprintw( 6, 10, "Are you sure reset system configuration?(N/Y)" );

    clrtoeol();
    refresh();
    cbreak();

    answer = getch();
    if ( answer == 'Y' || answer == 'y' )
    {
        nocbreak();
        mvprintw( 8, 10, "Reset System Configuration To Factory Default.\n" );
        clrtoeol();
        refresh();
        retval = ace_set_factory_setting();
        if (0 == retval)
        {
            sleep(2);
            mvprintw(10, 10, "Rebooting System");
            refresh();
            my_reboot();
        }
        else
        {
            sleep(2);
            mvprintw(10, 10, "recovery error %d", retval);
            refresh();
        }
    }
    else
    {
        nocbreak();
        mvprintw( 8, 10, "Cancel" );
        clrtoeol();
        refresh();
        sleep( 1 );
    }

    refresh();

    move( PROMPT_LINE, 0 );
}

void reset_data( void )
{
    char    answer  = 'N';
    int     retval  = 0;

    clear_all_screen();

    mvprintw( 4, 10, "[Clear Data]" );
    mvprintw( 21, 10, "Note: Press Y To Clear All Data And Reboot." );
    mvprintw( 6, 10, "Are you sure clear data?(N/Y)" );

    clrtoeol();
    refresh();
    cbreak();

    answer = getch();
    if ( answer == 'Y' || answer == 'y' )
    {
        nocbreak();
        mvprintw( 8, 10, "Clear Data.\n" );
        clrtoeol();
        refresh();
        retval = system("mv -f /mnt/mysql /mnt/mysql.bak >/dev/null 2>&1; mv -f /mnt/pgsql /mnt/pgsql.bak >/dev/null 2>&1; mv -f /mnt/clickhouse /mnt/clickhouse.bak >/dev/null 2>&1; mv -f /mnt/reporter /mnt/reporter.bak >/dev/null 2>&1");
        if (0 == retval)
        {
			system("rm -rf /var/log/*");
            sleep(2);
            mvprintw(10, 10, "Rebooting System");
            refresh();
            my_reboot();
        }
        else
        {
            sleep(2);
            mvprintw(10, 10, "recovery error %d", retval);
            refresh();
        }
    }
    else
    {
        nocbreak();
        mvprintw( 8, 10, "Cancel" );
        clrtoeol();
        refresh();
        sleep( 1 );
    }

    refresh();

    move( PROMPT_LINE, 0 );
}


void reset_nm_password(void)
{
    char     cmdBuf[512]               = { 0 };
    int      retval                    = 0;
    WINDOW*  new_window_ptr            = NULL;
    int      start_screenrow           = 0;
    int      start_screencol           = 0;
    int      confirm_key               = 0;
/*  int      key                       = 0;
    int      i                         = 0;
    char     serialnum[1024]           = { 0 };
    char     sequence[1024]            = { 0 };*/
    char*    user_conf = "/usr/local/lighttpd/user.conf";

    clear_all_screen();
 
    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Reset WebUI Acount Mode & Password & Management Policy");
    /*mvwprintw(new_window_ptr, start_screenrow + 2, start_screencol, "Serial Number: ");

    get_serial_number(sequence);

    mvwprintw(new_window_ptr, start_screenrow + 17,
              start_screencol,
              "Note:Please Input Serial Number");
    wrefresh(new_window_ptr);
  
    wmove( new_window_ptr, start_screenrow + 2, 15);

    do 
    {
        key = get_username_char( new_window_ptr);
        if (key == -1)
        {
            goto result_handler;
        }

        if (i < 30) 
        {
            wrefresh(new_window_ptr);
            if (key == KEY_BACKSPACE)
            {
                if (i > 0)
                {
                    wmove( new_window_ptr, start_screenrow + 2, 15 + i -1);
                    serialnum[i -1] = '\0';
                    i--;
                }
                else
                {   
                    i = 0;
                    wmove( new_window_ptr, start_screenrow + 2, 15 + i -1);
                    serialnum[0] = '\0';
                }

                wclrtoeol(new_window_ptr);
                wmove(new_window_ptr, start_screenrow + 2, 15 + i);
                wrefresh(new_window_ptr);
            }
            else
            {
                if (i < 30)
                {
                    serialnum[i] = key;
                    waddch( new_window_ptr, serialnum[i]);
                    wrefresh( new_window_ptr );
                    i++;
                }
            }
        }
        else if ( i >= 30 && i <= 75) 
        {
            if (key == KEY_BACKSPACE)
            {
                if (i > 30)
                {
                    wmove( new_window_ptr, start_screenrow + 3, i - 1 - 30);
                    serialnum[i -1] = '\0';
                    i--;
                    wclrtoeol(new_window_ptr);
                    wmove( new_window_ptr, start_screenrow + 3, i - 30);
                    wrefresh(new_window_ptr);
                }
                else
                {
                    wmove(new_window_ptr,
                          start_screenrow + 2, 
                          start_screencol + 30 -1);
                    serialnum[30] = '\0';
                    i = 29;
                    wmove( new_window_ptr, start_screenrow + 2, 15 + 29);
                    wclrtoeol(new_window_ptr);
                    wrefresh(new_window_ptr);
                }
            }
            else
            {
                if (i >= 30 && i < 75)
                {
                    serialnum[i] = key;
                    wmove( new_window_ptr, start_screenrow +3 , i - 30);
                    waddch( new_window_ptr, serialnum[i]);
                    wrefresh( new_window_ptr );
                    i++;
                }
            }
        }
    }
    while(key != '\n');

    if (strlen(serialnum) > 0)
    {
        serialnum[strlen(serialnum)-1] = '\0';
    }*/

    mvwprintw( new_window_ptr, start_screenrow + 17,
               start_screencol,
               "Note:Press Y/N Then Press Enter To Confirm" );

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 5, start_screencol);
    if ( -1 == confirm_key)
    {
        goto result_handler;
    }

    werase_line(new_window_ptr, start_screenrow + 17, start_screencol, 49);
    if ((confirm_key == 'Y')||(confirm_key == 'y'))
    {
       // if (0 == strcmp(sequence, serialnum))
       // {
            if (access("/home/config/default/passwd", 0) == 0)
            {
                system("cp -f /home/config/default/passwd /etc/");
                system("cp -f /home/config/default/passwd /home/config/current/");
            }

            if (access("/home/config/default/shadow", 0) == 0)
            {
                system("cp -f /home/config/default/shadow /etc/");
                system("cp -f /home/config/default/shadow /home/config/current/");
            }
            else
            {
                system("rm -rf /home/config/current/shadow");
            }

            snprintf(cmdBuf, sizeof(cmdBuf),
                    "rm -rf /usr/mysys/strong_password; rm -rf /usr/local/lighttpd/admin_mode; rm -rf /usr/local/lighttpd/webuser.conf; cp -f %s %s;cp -f /home/config/default/role.conf /usr/local/lighttpd/role.conf;"
                    "/usr/local/php/bin/php -r \"ace_manage_whitelist_man_set_status(2206,0);\" > /dev/null 2>&1",
                    "/home/config/default/user.conf",
                    user_conf);

            retval = system(cmdBuf);
            if ( 0 == retval )
            {
#ifdef DEFAULT_THREE_ROLE
				system("cp -af /home/config/cfg-scripts/three_role.conf /usr/local/lighttpd/role.conf;cp -af /home/config/cfg-scripts/three_role.conf /home/config/current/role.conf;");
				system("cp -af /home/config/cfg-scripts/three_user.conf /usr/local/lighttpd/user.conf;cp -af /home/config/cfg-scripts/three_user.conf /home/config/current/user.conf;");
				system("echo 1 > /usr/local/lighttpd/admin_mode;echo 1 > /home/config/current/admin_mode;");
#endif
                mvwprintw( new_window_ptr, start_screenrow + 17,
                           start_screencol,
                           "Note:Reset Success!" );
                wrefresh(new_window_ptr);
            }
            else
            {
                mvwprintw( new_window_ptr, start_screenrow + 17,
                           start_screencol,
                           "Note:Reset Failed!" );
                wrefresh(new_window_ptr);
            }

            sleep(3);
        //}
        /*else
        {
            mvwprintw(new_window_ptr, start_screenrow + 17,
                      start_screencol,
                      "Note:Please input the right serial number!" );
            wrefresh(new_window_ptr);
            sleep(3);
        }*/
        
       // usleep(3);
    }

    result_handler:
    delwin(new_window_ptr);
    
}

void reboot_system( void )
{
    char    answer  = 'N';

    clear_all_screen();

    mvprintw( 4, 10, "[Reboot System]" );
    mvprintw( 21, 10, "Note: Press Y To Reboot System." );
    mvprintw( 6, 10, "Are you sure to reboot system?(N/Y)" );

    clrtoeol();
    refresh();
    cbreak();

    answer = getch();
    if ( answer == 'Y' || answer == 'y' )
    {
        nocbreak();
        mvprintw( 8, 10, "System rebooting.\n" );
        clrtoeol();
        refresh();
        my_reboot();			
        sleep( 2 );
    }
    else
    {
        nocbreak();
        mvprintw( 8, 10, "Cancel" );
        clrtoeol();
        refresh();
        sleep( 1 );
    }

    refresh();
    move( PROMPT_LINE, 0 );
}

int add_route(void)
{
    char cmdline[128]         = { 0 };   
    int  start_screenrow      = 0;
    int  start_screencol      = 0;
    WINDOW* new_window_ptr    = NULL;
    int confirm_key           = 0;
    char route[256]           = { 0 };
    char netmask[256]         = { 0 };
    char gateway[256]         = { 0 };
    int  retval               = 0;

    clear_all_screen();

    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return -1;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Add Route Table");
    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol,
               "Add Route Format Exmaple: 192.168.0.1"
                     );
    mvwprintw( new_window_ptr, start_screenrow + 2,
               start_screencol, 
               "New Route Address:");
    mvwprintw( new_window_ptr, start_screenrow + 3,
               start_screencol, 
               "Netmask:");
    mvwprintw( new_window_ptr, start_screenrow + 4,
               start_screencol, 
               "Gateway:");
    if (-1 == get_mip_pars(new_window_ptr, 2, route))
    {
        goto result_handle;
    }

    if (-1 == get_mip_pars(new_window_ptr, 3, netmask))
    {
        goto result_handle;
    }

    if (-1 == get_mip_pars(new_window_ptr, 4, gateway))
    {
        goto result_handle;
    }

    confirm_key = get_confirm(new_window_ptr, start_screenrow + 8, start_screencol);
    if (-1 == confirm_key)
    {
        goto result_handle;
    }

    if ( confirm_key == 'Y' || confirm_key == 'y' )
    {
        ace_printf( "route = %s  netmask = %s  gateway = %s a%lu a%lu a%lu a%da", 
                    route, netmask, gateway,
                    strlen(route), strlen(netmask), strlen(gateway), __LINE__);
        if ( 0 != strlen(route) && 0 != strlen(netmask) && 0 != strlen(gateway))
        {
			if (strstr(netmask, "."))
			{
	            sprintf( cmdline, "busybox-1.35 route add -net %s netmask %s gw %s > /dev/null 2>&1", 
	                     hylab_escape_shell_arg(route), 
	                     hylab_escape_shell_arg(netmask), 
	                     hylab_escape_shell_arg(gateway) );
			}
			else
			{
				sprintf( cmdline, "busybox-1.35 route add -net %s/%s gw %s > /dev/null 2>&1", 
	                     hylab_escape_shell_arg(route), 
	                     hylab_escape_shell_arg(netmask), 
	                     hylab_escape_shell_arg(gateway) );
			}
            retval = system(cmdline);
        }
        else if ( 0 != strlen(gateway) )
        {
        #ifdef HYLAB_VPP
			sprintf(cmdline, "/usr/local/vpp/bin/vppctl ip route add 0.0.0.0/0 via %s > /dev/null 2>&1",
				hylab_escape_shell_arg(gateway));
		#else
            sprintf(cmdline, "route add default gw %s > /dev/null 2>&1", hylab_escape_shell_arg(gateway));
		#endif
			retval = system(cmdline);
        }
        else
        {
            retval = -1;
        }
    }
    else if ( confirm_key == 'n' || confirm_key == 'N')
    {
         goto result_handle;
    }

    if ( 0 == retval )
    {
        werase_line(new_window_ptr, start_screenrow + 17, start_screencol , 50);
        mvwprintw( new_window_ptr, start_screenrow + 17, 
                   start_screencol,
                   "Add Success");
        wrefresh(new_window_ptr);
    }
    else
    {
        werase_line(new_window_ptr, start_screenrow + 17, start_screencol , 50);
        mvwprintw( new_window_ptr, start_screenrow + 17, 
                   start_screencol,
                   "Add Error %d",
                   retval);
        wrefresh(new_window_ptr);
    }

    sleep(3);
    result_handle:
    delwin(new_window_ptr);
    return retval;
}

int read_first_line( char* pathname, char* buffer, int max_len )
{
    FILE* fp            = NULL;
    int read_ok = 0;
    fp = fopen( pathname, "r" );
    if ( fp == NULL )
    {
        return read_ok;
    }
    if (NULL != fgets(buffer, max_len, fp))
    {
        read_ok = 1;
    }
    fclose( fp );
    return read_ok;
}

int show_all_versions(void)
{
    WINDOW*  new_window_ptr            = NULL;
    int      start_screenrow           = 0;
    int      start_screencol           = 0;
    char     line[256]                 = { 0 };
    int      i                         = 0;
    int info_index = 0;
    int char_len = 0;
    int key = 0;
    long pos_len = 0;
    char info_lines[10][256] = {"","","","","","","","","",""};
    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "Firmware:");
    if (!read_first_line("/usr/private/Firmware_version", info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1))
    {
        
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    if ((pos_len = (strstr((const char*)info_lines[info_index], "-") - (char*)info_lines[info_index])) > (long)sizeof("Firmware:"))
    {
        char_len = strlen(info_lines[info_index]);
        memmove(((char*)info_lines[info_index]) + sizeof("Firmware:") - 1, ((char*)info_lines[info_index]) + pos_len + 1, char_len - pos_len);
         *(((char*)info_lines[info_index]) + sizeof("Firmware:") - 1 + char_len - pos_len) = 0;
    }
    
    info_index++;

    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "Feature:");
    if (!read_first_line("/usr/private/featureV-lib-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;

    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "URL:");
    if (!read_first_line("/usr/private/url-lib-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;

#ifndef CONFIG_HYTF_FW
    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "Client:");
    if (!read_first_line("/usr/private/security-client-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;
#endif

    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "ISP:");
    if (!read_first_line("/usr/private/ispiplists/isp-ip-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;

#ifdef CONFIG_HYTF_FW
    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "WAF:");
    if (!read_first_line("/usr/private/waf-crs-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;

    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "IPS:");
    if (!read_first_line("/usr/private/ips-rule-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;

    char_len = snprintf(info_lines[info_index], sizeof(info_lines[info_index]), "AVCRS:");
    if (!read_first_line("/usr/private/av-crs-ver", info_lines[info_index] + char_len, sizeof(info_lines[1]) - char_len - 1))
    {
        char_len += snprintf(info_lines[info_index] + char_len, sizeof(info_lines[info_index]) - char_len - 1, "None");
    }
    info_index++;
#endif

    clear_all_screen();
 
    new_window_ptr = newwin( 18, 60, 4, 10);
    if ( !new_window_ptr )
    {
        return -1;
    }
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Version Info");

    i = 0;
    info_index = 0;
    while (0 != info_lines[info_index][0])
    {
        line[0] = 0;
        strncpy(line, info_lines[info_index], sizeof(line));
        if (i < BOXED_LINES - 2)
        {
            if (strlen(line) < 60)
            {
                mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, line);
                i++;
            }
        }
        info_index++;
    }

    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol, 
               "Note:Press Enter To Return" );
    wrefresh(new_window_ptr);
    key = wgetch(new_window_ptr);
    while (key != '\n')
    {
        key = wgetch(new_window_ptr);
    }

    delwin(new_window_ptr);
    return 0;
}

int show_console_seed(void)
{    
    WINDOW*  new_window_ptr            = NULL;
    int      start_screenrow           = 0;
    int      start_screencol           = 0;
    int      i                         = 0;
    int      key                       = 0;

    clear_all_screen();
 
    new_window_ptr = newwin( 18, 50, 4, 10);
    if ( !new_window_ptr )
    {
        return -1;
    }
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Unit Console Seed");

    mvwprintw( new_window_ptr, start_screenrow + i,
               start_screencol, g_console_password_seed);
    i++;

    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol, 
               "Note:Press Enter To Return" );
    wrefresh(new_window_ptr);
    key = wgetch(new_window_ptr);
    while (key != '\n')
    {
        key = wgetch(new_window_ptr);
    }

    delwin(new_window_ptr);
    return 0;
}


int show_serial_num(void)
{    
    WINDOW*  new_window_ptr            = NULL;
    int      start_screenrow           = 0;
    int      start_screencol           = 0;
    int      i                         = 0;
    int      key                       = 0;

    clear_all_screen();
 
    new_window_ptr = newwin( 18, 60, 4, 10);
    if ( !new_window_ptr )
    {
        return -1;
    }
    cbreak();
    noecho();

    clear_sub_screen( new_window_ptr, "Unit License");

#if defined(ARCH_ARM64) || defined(CONFIG_HYTF_FW) || defined(aiTai) || defined(Hillstone)
    HYTF_LIC_REQ_MSG req_msg = {0};
    HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    req_msg.mtype = ACE_LIC_GET_STATE;
    req_msg.subType = ACE_LIC_GET_LICENSE;
    lic_rand_req_fill(&req_msg);
    char *license_info_detail[3] = {"Unauthorized","Demo","Normal"};

    if (0 != hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000))
        return -1;

    mvwprintw( new_window_ptr, start_screenrow + i++, start_screencol, "Device Serial Number:");
    mvwprintw( new_window_ptr, start_screenrow + i++, start_screencol, rsp_msg.host_id);

    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "License Type:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, license_info_detail[rsp_msg.feature_value]);
    i++;

    time_t now = time(NULL);
    struct tm tm_expired;
    char buffer[64] = {0};

    now += (rsp_msg.expired_days[0] * 86400 - rsp_msg.lic_runout_hours[0] * 3600);

	if (now >= time(NULL))
	{
		mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "License Status:");
	    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, "Available");
	    i++;
	}
	else
	{
		mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "License Status:");
	    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, "Expired");
	    i++;
	}	
	
    localtime_r(&now, &tm_expired);

    snprintf(buffer, sizeof(buffer)-1, "%04d-%02d-%02d",
        tm_expired.tm_year + 1900, tm_expired.tm_mon + 1, tm_expired.tm_mday);

    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, (rsp_msg.feature_value == 0 || rsp_msg.feature_value == 1) ? "Demo Expired Date:" : "Firmware Update Expired Date:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    i++;

    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Customer:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, rsp_msg.customer);
    i++;

    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Vendor:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, rsp_msg.vendor);
    i++;

    mvwprintw( new_window_ptr, start_screenrow + i++, start_screencol, "");


    if (rsp_msg.session_value >= 1000000)
        snprintf(buffer, sizeof(buffer)-1, "%uM", rsp_msg.session_value/1000000);
    else if(rsp_msg.session_value >= 1000)
        snprintf(buffer, sizeof(buffer)-1, "%uK", rsp_msg.session_value/1000);
    else
        snprintf(buffer, sizeof(buffer)-1, "%u", rsp_msg.session_value);
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Authorized Session Number:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    i++;

    snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.l7filter_bandwidth_value);
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Authorized L7 Throughput:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    i++;

    snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.firewall_bandwidth_value);
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Authorized Firewall Throughput:");
    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    i++;
    
#else

    char     firewall_string[256] = "";
    char     cmdBuf[256]               = { 0 };
    int      retval                    = 0;
    
    FILE*    fp                        = 0;
    char     filename[256]             = "/usr/.lic/.ace_lic_info";
    char     line[256]                 = { 0 };
    
    
    char     sequence[256]             = { 0 };
    char*    pstr                      = NULL;
    
    if (1 == license_nac_is_version2())
    {
    	HYTF_LIC_REQ_MSG req_msg = {0};
    	HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    	req_msg.mtype = ACE_LIC_GET_STATE;
    	req_msg.subType = ACE_LIC_GET_LICENSE;
    	lic_rand_req_fill(&req_msg);
    	char *license_info_detail[3] = {"Unauthorized","Demo","Normal"};

    	if (0 != hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000))
    	    return -1;

    	mvwprintw( new_window_ptr, start_screenrow + i++, start_screencol, "Device Serial Number:");
    	mvwprintw( new_window_ptr, start_screenrow + i++, start_screencol, rsp_msg.host_id);

    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "License Type:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, license_info_detail[rsp_msg.feature_value]);
    	i++;

    	time_t now = time(NULL);
    	struct tm tm_expired;
    	char buffer[64] = {0};

    	now += (rsp_msg.expired_days[0] * 86400 - rsp_msg.lic_runout_hours[0] * 3600);
    	localtime_r(&now, &tm_expired);

    	snprintf(buffer, sizeof(buffer)-1, "%04d-%02d-%02d",
    	    tm_expired.tm_year + 1900, tm_expired.tm_mon + 1, tm_expired.tm_mday);

    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, (rsp_msg.feature_value == 0 || rsp_msg.feature_value == 1) ? "Demo Expired Date:" : "Firmware Update Expired Date:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    	i++;

    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Customer:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, rsp_msg.customer);
    	i++;

    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Vendor:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, rsp_msg.vendor);
    	i++;

    	mvwprintw( new_window_ptr, start_screenrow + i++, start_screencol, "");


    	if (rsp_msg.session_value >= 1000000)
    	    snprintf(buffer, sizeof(buffer)-1, "%uM", rsp_msg.session_value/1000000);
    	else if(rsp_msg.session_value >= 1000)
    	    snprintf(buffer, sizeof(buffer)-1, "%uK", rsp_msg.session_value/1000);
    	else
    	    snprintf(buffer, sizeof(buffer)-1, "%u", rsp_msg.session_value);
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Authorized Session Number:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    	i++;

    	snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.l7filter_bandwidth_value);
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Authorized L7 Throughput:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    	i++;

    	snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.firewall_bandwidth_value);
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, "Authorized Firewall Throughput:");
    	mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 32, buffer);
    	i++;
    }
    else
    {
    	sprintf(cmdBuf, "/usr/local/php/bin/php -r \"ace_get_lisence();\" > /dev/null 2>&1");
    	retval = system(cmdBuf);
    	if (0 != retval)
    	{
    	    return -1;
    	} 

    	fp = fopen(filename, "r");
    	if (!fp)
    	{
    	    ace_printf("error open %s", filename);
    	    return -1;
    	}

    	while (NULL != fgets(line, sizeof(line), fp))
    	{
    	    if (i < BOXED_LINES - 2)
    	    {
    	        if (strlen(line) < 50)
    	        {
    	            if (0 == strncmp(line, "Authorized Firewall Throughput", strlen("Authorized Firewall Throughput")))
    	            {
    	                 strncpy(firewall_string, line, sizeof(firewall_string));
    	            }
    	            else
    	            {
    	                 mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, line);
    	                 i++;
    	            }
    	        }
    	        else
    	        {
    	            if ( 0 == strncmp(line, "Device Serial Number", strlen("Device Serial Number")))
    	            {
    	                mvwprintw( new_window_ptr, start_screenrow + i,
    	                           start_screencol, "Device Serial Number:");
    	                i++;
    	                get_serial_number(sequence);
    	                mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, sequence);
    	                i++;
    	            }
    	            else
    	            {
    	                if (0 == strncmp(line, "Authorized Firewall Throughput", strlen("Authorized Firewall Throughput")))
    	                {
    	                     strncpy(firewall_string, line, sizeof(firewall_string));
    	                }
    	                else
    	                {
    	                    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, line);
    	                    pstr = &line[50];
    	                    i++;
    	                    werase_line(new_window_ptr, start_screenrow + i, start_screencol, 30);
    	                    mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 31, pstr);
    	                    i++;
    	                }
    	            }
    	        }
    	    }
    	}
    	
    	if (i < BOXED_LINES - 2 && firewall_string[0])
    	{
    	    if (strlen(firewall_string) < 50)
    	    {
    	        mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, firewall_string);
    	        i++;
    	    }
    	    else
    	    {
    	        mvwprintw( new_window_ptr, start_screenrow + i, start_screencol, firewall_string);
    	        pstr = &firewall_string[50];
    	        i++;
    	        werase_line(new_window_ptr, start_screenrow + i, start_screencol, 30);
    	        mvwprintw( new_window_ptr, start_screenrow + i, start_screencol + 31, pstr);
    	        i++;
    	    }
    	}
    }
#endif

    mvwprintw( new_window_ptr, start_screenrow + 17, 
               start_screencol, 
               "Note:Press Enter To Return" );
    wrefresh(new_window_ptr);
    key = wgetch(new_window_ptr);
    while (key != '\n')
    {
        key = wgetch(new_window_ptr);
    }

    delwin(new_window_ptr);
    return 0;
}

/*
void system_upgrade(void)
{
    int choice = 0;
    WINDOW*  new_window_ptr   = NULL;            

    new_window_ptr = subwin( stdscr, 1, 60, 21, 9);
    cbreak();
    noecho();

    clear_all_screen();
    read_ftp_file( FTP_CONFIG_FILE );
    touchwin(stdscr);
    do
    {
        choice = getchoice("[System Update]", system_upgrade_menu);
        ace_printf("%d choice = %d",  __LINE__, choice);
        switch (choice)
        {
            case '1':
                ftp_configure();
                break;

            case '2':
                draw_system_upgrade(new_window_ptr, choice, ACE_FIRMWARE, ACE_FIRMWARE_SHORT);
                break;

            case '3':
                draw_system_upgrade(new_window_ptr, choice, ACE_FEATURE, ACE_FEATURE_SHORT);
                break;

            case '4':
                draw_system_upgrade(new_window_ptr, choice, ACE_URLLIB, ACE_URLLIB_SHORT);
                break;

            case '5':
                draw_system_upgrade(new_window_ptr, choice, ACE_LIC_UPDATE_FILE, 
                                    ACE_LIC_UPDATE_FILE_SHORT);
                break;
            default:
                break;
        }
    }
    while (choice != 'q' && choice != -1);

    delwin(new_window_ptr);
}
*/

void draw_system_upgrade( int choice )
{
    int iRet = 0;
    WINDOW*  new_window_ptr   = NULL;
    char szRet[512] = {"Note: Failed."};

    ace_printf("%s %d choice %d\n", __FUNCTION__, __LINE__, choice);

    new_window_ptr = subwin( stdscr, 18, 70, 3, 0 );
    if ( new_window_ptr != NULL )
    {
        cbreak();
        noecho();

        clear_all_screen();
        touchwin(stdscr);
        mvwprintw( new_window_ptr, 0, 25, "System Upgrade" );
        mvwprintw( new_window_ptr, 17, 25, "Note: Uploading ..." );
        wrefresh( new_window_ptr );

        do
        {
            system( "mkdir /root/sys_up > /dev/null 2>&1" );

            DIR *mydir = NULL;
            if ( NULL == ( mydir = opendir( "/root/sys_up" ) ) )
            {
                ace_printf( "[%s :: %d] ==> Path [/root/sys_up] not exist.", __FUNCTION__, __LINE__ );
                break;
            }

            system( "rm -rf /root/sys_up/* > /dev/null 2>&1" );

            wmove( new_window_ptr, 8, 0 );
            wrefresh( new_window_ptr );         

            iRet = system( "cd /root/sys_up/;rz -b -O -y" );
            if ( 0 != iRet )
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed.", __FUNCTION__, __LINE__ );
                closedir( mydir );
                break;
            }

            int iUploaded = 0;
            struct dirent *ent = NULL;
            char szNac[512] = {0};
            
            strcpy( szNac, "/root/sys_up/" );
            
            while ( ( ent = readdir( mydir ) ) != NULL )  
            {
                if ( !( ent->d_type & DT_DIR ) )
                {
                    ace_printf( "%s", ent->d_name );
                
                    if ( ( '2' == choice ) && strstr( ent->d_name, ".bin" ) )
                    {
                        iUploaded = 1;
                        strcat( szNac, ent->d_name );
                        break;
                    }
                    else if ( ( '3' == choice ) && strstr( ent->d_name, "NACV-FEATURE-LIB" ) && strstr( ent->d_name, ".bin" ) )
                    {
                        iUploaded = 1;
                        strcat( szNac, ent->d_name );
                        break;
                    }
                    else if ( ( '4' == choice ) && strstr( ent->d_name, "NACII-URL-LIB" ) && strstr( ent->d_name, ".bin" ) )
                    {
                        iUploaded = 1;
                        strcat( szNac, ent->d_name );
                        break;
                    }
                    else if ( ( '5' == choice ) && strstr( ent->d_name, ".lic" ) )
                    {
                        iUploaded = 1;
                        strcat( szNac, ent->d_name );
                        break;
                    }
                }
            }

            closedir( mydir );

            if ( 0 == iUploaded )
            {
                iRet = -1;
            
                break;
            }

            char szCmd[512] = {0};
            char szCmdUp[512] = {0};
            
            if ( '2' == choice )
            {
                system( "rm -f /usr/download/NACFirmware.bin.old" );
                system( "mv -f /usr/download/NACFirmware.bin.new /usr/download/NACFirmware.bin.old" );              
                sprintf( szCmd, "mv -f %s /usr/download/NACFirmware.bin.new", hylab_escape_shell_arg(szNac) );
                strcpy( szCmdUp, "/bin/ace_upsys > /dev/null 2>&1" );
            }
            else if ( '3' == choice )
            {
                //system( "rm -f /usr/download/NACII-FEATURE-LIB.bin.old" );
                //system( "mv -f /usr/download/NACII-FEATURE-LIB.bin.new /usr/download/NACII-FEATURE-LIB.bin.old" );                    
                sprintf( szCmd, "mv -f %s /usr/download/%s", hylab_escape_shell_arg(szNac), ACE_FEATURE_SHORT );
                sprintf( szCmdUp, "/usr/local/php/bin/php -r \"ace_update_l7_filter('%s');\" > /dev/null 2>&1", ACE_FEATURE_SHORT );
            }
            else if ( '4' == choice )
            {
                //system( "rm -f /usr/download/NAC-URL-LIB.bin.old" );
                //system( "mv -f /usr/download/NAC-URL-LIB.bin.new /usr/download/NAC-URL-LIB.bin.old" );                    
                sprintf( szCmd, "mv -f %s /usr/download/%s", hylab_escape_shell_arg(szNac), ACE_URLLIB_SHORT );
                sprintf( szCmdUp, "/usr/local/php/bin/php -r \"ace_update_url_lib('%s');\" > /dev/null 2>&1", ACE_URLLIB_SHORT );
            }
            else if ( '5' == choice )
            {
                //system( "rm -f /usr/download/.ace_fsec.dat" );
                sprintf( szCmd, "mv -f %s /usr/download/%s", hylab_escape_shell_arg(szNac), ACE_LIC_UPDATE_FILE_SHORT );
                sprintf( szCmdUp, "/usr/local/php/bin/php -r \"ace_update_lisence('%s');\" > /dev/null 2>&1", ACE_LIC_UPDATE_FILE_SHORT );
            }

            iRet = system( szCmd );
            if ( 0 != iRet )
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed.", __FUNCTION__, __LINE__ );
                break;
            }           

            werase_line( new_window_ptr, 17, 25, 70 );
            mvwprintw( new_window_ptr, 17, 25, "Note: Updating ..." );

            iRet = system( szCmdUp );
            if ( 0 != iRet )
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed.", __FUNCTION__, __LINE__ );
                break;
            }

            strcpy( szRet, "Note: succeeded." );
        }while ( FALSE );

        if ( iRet != 0 )
        {
            sprintf( szRet, "Note: Error [%d].", iRet );
        }

        strcat( szRet, "(Press [Esc] to exit)" );
        
        werase_line( new_window_ptr, 17, 25, 70 );
        mvwprintw( new_window_ptr, 17, 25, szRet );
        wrefresh( new_window_ptr );     

        unsigned int key = 0;

        while ( key != 0x1B )
        {
            key = wgetch( new_window_ptr );
        }   

        delwin( new_window_ptr );
    }
    else
    {
        ace_printf( "[%s :: %d] ==> Call function [subwin] failed.", __FUNCTION__, __LINE__ );
    }   
}

void system_upgrade(void)
{
    int choice = 0;
    WINDOW*  new_window_ptr   = NULL;            

    new_window_ptr = subwin( stdscr, 1, 60, 21, 9);
    cbreak();
    noecho();

    clear_all_screen();
    touchwin(stdscr);
    do
    {
        choice = getchoice("[System Update]", system_upgrade_menu);
        ace_printf("%d choice = %d",  __LINE__, choice);
        switch (choice)
        {
            case '2':
            case '3':
            case '4':
            case '5':
                draw_system_upgrade( choice );
                break;
            default:
                break;
        }
    }
    while (choice != 'q' && choice != -1);

    delwin(new_window_ptr); 
}

void download_debug_info(void)
{
    int iRet = 0;
    WINDOW*  new_window_ptr   = NULL;
    char szRet[512] = {"Note: Failed."};

    new_window_ptr = subwin( stdscr, 18, 70, 3, 0 );
    if ( new_window_ptr != NULL )
    {
        cbreak();
        noecho();

        clear_all_screen();
        touchwin(stdscr);
        mvwprintw( new_window_ptr, 0, 25, "Download Debug information" );
        mvwprintw( new_window_ptr, 17, 25, "Note: Downloading ..." );
        wrefresh( new_window_ptr );

        do
        {
            system( "rm -f /root/Debug_kmsg.txt > /dev/null 2>&1" );
            
            system( "rm -f /root/DebugMessage.tar.gz > /dev/null 2>&1" );

            system( "rm -f /root/DebugMessage.zip > /dev/null 2>&1" );

            iRet = system( "dmesg > /root/Debug_kmsg.txt" );
            if ( 0 != iRet )
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed.", __FUNCTION__, __LINE__ );
                break;
            }

            iRet = system( "tar -zcvf /root/DebugMessage.tar.gz /mnt/corefile/ /usr/private/Firmware_version /etc/* /home/config/current/ /home/corefile /usr/.lic /var/log/* /root/Debug_kmsg.txt --exclude=/etc/ac_config --exclude=/etc/passwd --exclude=/etc/shadow --exclude=/home/config/current/user.conf --exclude=/home/config/current/passwd --exclude=/home/config/current/shadow > /dev/null 2>&1" );
            if (access("/root/DebugMessage.tar.gz", 0) != 0)
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed.", __FUNCTION__, __LINE__ );
                break;
            }
    
            if ( ( access( "/root/DebugMessage.tar.gz", R_OK ) ) == -1 )
            {
                ace_printf( "[%s :: %d] ==> Call function [access] failed.", __FUNCTION__, __LINE__ );
                break;
            }

            wmove( new_window_ptr, 8, 0 );
            wrefresh( new_window_ptr );

            iRet = system( "zip -rP '"HYLAB_WEB_DEBUG_PASSWORD"' /root/DebugMessage.zip /root/DebugMessage.tar.gz > /dev/null 2>&1" );
            if ( 0 != iRet )
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed, error [%d].", __FUNCTION__, __LINE__, iRet );
                break;
            }
            
            iRet = system( "sz -y -q /root/DebugMessage.zip" );
            if ( 0 != iRet )
            {
                ace_printf( "[%s :: %d] ==> Call function [system] failed, error [%d].", __FUNCTION__, __LINE__, iRet );
                break;
            }

            strcpy( szRet, "Note: succeeded." );
        }while ( FALSE );

        system( "rm -f /root/Debug_kmsg.txt > /dev/null 2>&1" );
        system( "rm -f /root/DebugMessage.tar.gz > /dev/null 2>&1" );
        system( "rm -f /root/DebugMessage.zip > /dev/null 2>&1" );

        if ( iRet != 0 )
        {
            sprintf( szRet, "Note: Error [%d].", iRet );
        }

        strcat( szRet, "(Press [Esc] to exit)" );
        
        werase_line( new_window_ptr, 17, 25, 70 );
        mvwprintw( new_window_ptr, 17, 25, szRet );
        wrefresh( new_window_ptr );     

        unsigned int key = 0;

        while ( key != 0x1B )
        {
            key = wgetch( new_window_ptr );
        }   

        delwin( new_window_ptr );
    }
    else
    {
        ace_printf( "[%s :: %d] ==> Call function [subwin] failed.", __FUNCTION__, __LINE__ );
    }
}

void system_status( void )
{
    int choice = 0;

    clear_all_screen();
    do
    {
        choice = getchoice("[System Status]", status_menu);
        switch (choice)
        {
            case '1':
                save_cmd_ret("ps", "", "Ps_show");
                show_cmd("Ps");
                break;

            case '2':
                save_cmd_ret("ifconfig", "-a", "Ifconfig_show");
                show_cmd("Ifconfig");
                break;

            case '3':
                save_cmd_ret("netstat", "-an", "Netstat_show");
                show_cmd("Netstat");
                break;

            case '4':
                save_cmd_ret("free", "", "Free_show");
                show_cmd("Free");
                break;

            case '5':
                save_cmd_ret("df", "-a -h", "Df_show");
                show_cmd("Df");
                break;

            case '6':
                save_cmd_ret("tail", "-100 /var/log/messages | sed 's/\\%/\\%\\%/g'", "Tail_show");
                show_cmd("Tail");
                break;

            case '7':
                save_cmd_ret("dmesg", " |sed 's/\\%/\\%\\%/g'", "Dmesg_show");
                show_cmd("Dmesg");
                break;

            case '8':
                save_cmd_ret("ip route show","", "Route_show");
                show_cmd("Route");
                break;

            case '9':
                save_cmd_ret("cat /proc/net/arp", "", "Arp_show");
                show_cmd("Arp");
                break;

            case 'a':
                save_cmd_ret("lsmod", "", "Lsmod_show");
                show_cmd("Lsmod");
                break;

         case 'b':
         save_cmd_ret("top", "-b -n 1 | sed 's/\\%/\\%\\%/g' | sed 's/ </</g' | grep -v 'sed s/' | grep -v grep | awk '{if(NR<=3){print $0}else{printf(\"%5s  %4s  %4s  %4s\\n\",$1,$4,$6,$8)}}'", "Top_show");
         show_cmd("Top");
         break;
            default:
                break;
        }
    }
    while (choice != 'q' && choice != -1);
}

void logout_authentication(void)
{
    WINDOW *new_window_ptr = NULL;
    char passwd[128]        = { 0 };
    char username[128]      = { 0 };
    int i                  = 0;
    int trytimes           = 0;
    int key                = 0;

    FILE *fp = popen("tty", "r");
    if (fp)
    {
        char cmdbuf[128] = { 0 };
        if (fgets(cmdbuf, sizeof(cmdbuf)-1, fp))
        {
            if (!strncmp("/dev/tty", cmdbuf, sizeof("/dev/tty")-1)
#ifdef ARCH_ARM64
                && strncmp("/dev/ttyAMA", cmdbuf, sizeof("/dev/ttyAMA")-1)
#else
                && strncmp("/dev/ttyS", cmdbuf, sizeof("/dev/ttyS")-1)
#endif
                )
            {
                pclose(fp);
                endwin();
                system("clear");
                exit( EXIT_SUCCESS );
            }
        }

        pclose(fp);
    }

    new_window_ptr = newwin( 1, 60, 21, 1);
    if ( !new_window_ptr )
    {
        return;
    }

    keypad(new_window_ptr, TRUE);
    cbreak();
    noecho();

    /*input username*/
    do
    {
        werase( new_window_ptr );
        mvwprintw( new_window_ptr,0,1, "Username:");
        mvwprintw( new_window_ptr, 0, 20, "Password:" );
        wrefresh( new_window_ptr );
        wmove( new_window_ptr, 0, 10);
        i = 0;

        /*input username*/
        do 
        {
            key = get_username_char( new_window_ptr);
            if (key == -1)
            {
                goto result_handle;
            }

            if (key == KEY_BACKSPACE)
            {
                if (i > 0)
                {
                    wmove( new_window_ptr, 0, 10 + i -1);
                    username[i -1] = '\0';
                    i--;
                }
                else
                {
                    wmove( new_window_ptr, 0, 10);
                    username[0] = '\0';
                    i = 0;
                }

                wclrtoeol(new_window_ptr);
                mvwprintw( new_window_ptr, 0, 20, "Password:" );
                wmove( new_window_ptr, 0, 10 + i);
                wrefresh(new_window_ptr);
            }
            else
            {
                if (i < (int)(sizeof(ACE_CONSOLE_USERNAME)))
                {
                    username[i] = key;
                    waddch( new_window_ptr, username[i]);
                    wrefresh( new_window_ptr );
                    i++;
                }
            }
        }
        while(key != '\n');

        if ( (username[0] < 'a' || username[0] > 'z') &&
             (username[0] < 'A' || username[0] > 'Z') )
        {
            continue;
        }

        /*input password*/
        mvwprintw( new_window_ptr, 0, 20, "Password:" );
        wrefresh( new_window_ptr );
        wmove( new_window_ptr, 0, 30);

        i = 0;
        memset( passwd, '\0', sizeof(passwd));

        do 
        {
            key = wgetch( new_window_ptr);
            if (key == KEY_BACKSPACE)
            {
                if (i > 0)
                {
                    wmove( new_window_ptr, 0, 30 + i -1);
                    passwd[i -1] = '\0';
                    i--;
                }
                else
                {
                    wmove( new_window_ptr, 0, 30);
                    passwd[0] = '\0';
                    i = 0;
                }

                wclrtoeol(new_window_ptr);
                wrefresh(new_window_ptr);
            }
            else
            {
                if (i < g_hw_sn[0]?(int)(strlen(g_hw_sn)+1):(int)(sizeof(ACE_CONSOLE_PASSWORD)))
                {
                    passwd[i] = key;
                    waddch( new_window_ptr, '*');
                    wrefresh( new_window_ptr );
                    i++;
                }
            }
        }
        while(key != '\n');

        if (strlen(username) > 0)
        {
            username[strlen(username) - 1] = '\0';
        }

        if (strlen(passwd) > 0)
        {
            passwd[strlen(passwd)-1] = '\0';
        }
		
		if ((g_hw_sn[0]?(strcmp(passwd,g_hw_sn) == 0):(strcmp(passwd,ACE_CONSOLE_PASSWORD) == 0)) && 
		    (strcmp(username,ACE_CONSOLE_USERNAME) == 0))
		{
            werase( new_window_ptr );
            mvwprintw( new_window_ptr, 0, 2, "Logout Success" );
            wrefresh( new_window_ptr );
            sleep(1);
            delwin(new_window_ptr);
            endwin();
            system("clear");
            exit( EXIT_SUCCESS );
        }
        else if ( strcmp(username, ACE_CONSOLE_USERNAME) != 0 )
        {  
            werase( new_window_ptr );
            mvwprintw( new_window_ptr, 0, 2, "Incorrect Username" );
            wrefresh( new_window_ptr );
            trytimes++;
            sleep(1);
        }
        else if ( g_hw_sn[0]?strcmp(g_hw_sn,ACE_CONSOLE_PASSWORD) != 0:strcmp(passwd,ACE_CONSOLE_PASSWORD) != 0 )
        {
            werase( new_window_ptr );
            mvwprintw( new_window_ptr, 0, 2, "Incorrect Passwd" );
            wrefresh( new_window_ptr );
            trytimes++;
            sleep(1);
        }
        else
        {
            werase( new_window_ptr );
            mvwprintw( new_window_ptr, 0, 2, "Incorrect Username" );
            wrefresh( new_window_ptr );
            trytimes++;
            sleep(1);
        }
  
    }
    while ( trytimes < 3 );

    result_handle:
    delwin( new_window_ptr );
}

#endif /*67*/

#if 8
void sigquit(int sig)
{
   endwin();
   system("clear");
   exit( EXIT_SUCCESS );
    
}
void sigint(int sig)
{
    return;
}

void signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);
}

void signal_init ()
{
    signal_set (SIGINT, sigint);
    signal_set (SIGTSTP, sigint);
    signal_set (SIGTERM, sigint);
    signal_set (SIGQUIT, sigint);
    signal_set (SIGKILL, sigint);
}

#endif /*8*/

static int init_curses_with_fallback(void)
{
	setenv("TERM", "xterm", 1);

    SCREEN* s = newterm(NULL, stdout, stdin);
    if (s)
    {
        set_term(s);
        return 0;
    }

    const char* candidates[] = {"xterm", "vt100", "linux", "ansi"};
    for (int i = 0; i < (int)(sizeof(candidates)/sizeof(candidates[0])); ++i)
    {
        setenv("TERM", candidates[i], 1);
        s = newterm(NULL, stdout, stdin);
        if (s)
        {
            set_term(s);
            return 0;
        }
    }
    return -1;
}

static int is_admin_user(const char* user)
{
    if (!user) return 0;
    if (strcmp(user, "root") == 0) return 1;
    if (strcmp(user, "LOGIN") == 0) return 1;
    if (strcmp(user, "superman") == 0) return 1;
    if (strcmp(user, "admin") == 0) return 1;
    if (strcmp(user, "kladmin") == 0) return 1;
    if (strcmp(user, "recovery") == 0) return 1;
    return 0;
}

static void run_minimal_cli(int login_console, int is_admin)
{
    char input[256];
    get_hw_sn(login_console);
    printf("%s %s\n", "Console (minimal) -", g_product_type);
    if (g_hw_sn[0]) printf("Serial: %s\n", g_hw_sn);
    printf("Type 'help' for commands.\n");
    for (;;) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        char* p = strchr(input, '\n');
        if (p) *p = 0;
        if (!input[0]) continue;
        if (strcmp(input, "help") == 0) {
            printf("Commands: ps, ifconfig, netstat, free, df, route, arp, version, serial, reboot, exit\n");
            continue;
        }
        if (strcmp(input, "ps") == 0) { system("ps"); continue; }
        if (strcmp(input, "ifconfig") == 0) { system("ifconfig -a"); continue; }
        if (strcmp(input, "netstat") == 0) { system("netstat -an"); continue; }
        if (strcmp(input, "free") == 0) { system("free"); continue; }
        if (strcmp(input, "df") == 0) { system("df -h"); continue; }
        if (strcmp(input, "route") == 0) { system("route -n"); continue; }
        if (strcmp(input, "arp") == 0) { system("arp -an"); continue; }
        if (strcmp(input, "version") == 0) {
            system("uname -a");
            system("cat /usr/private/Firmware_Board_Type 2>/dev/null");
            continue;
        }
        if (strcmp(input, "serial") == 0) {
            if (g_hw_sn[0]) printf("%s\n", g_hw_sn); else printf("N/A\n");
            continue;
        }
        if (strcmp(input, "reboot") == 0) {
            if (is_admin) { my_reboot(); } else { printf("Permission denied\n"); }
            continue;
        }
        printf("Unknown command\n");
    }
}

int main( void )
{
#if !(defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw))
	execl("/usr/local/hycli/bin/hycli", "hycli", NULL);
#endif	
    int choice      = 0;
    char *loguser   = NULL;
    const char *debug_super_key = "login2debug";

    int debug_super_key_len = strlen(debug_super_key);
    int debug_super_key_pos = 0;
    int login_console = 0;

	if (access("/etc/.pgsql", 0) == 0)
	{
		loguser = getenv("USER");
		if (NULL != loguser && 0 == strcmp(loguser, "postgres"))
			exit( EXIT_SUCCESS );
	}

    if (1)
    {
        char buf[256] = { 0 };
        sprintf( buf , "cat /proc/cmdline | grep single");
        if (!system(buf) )
        {
             exit( EXIT_SUCCESS );
        }
    }
#ifdef jeth
    login_console = 1;
#else
    FILE *fp = popen("tty", "r");
    if (fp)
    {
        char cmdbuf[128] = { 0 };
        if (fgets(cmdbuf, sizeof(cmdbuf)-1, fp))
        {
#if defined(CS_E2000Q_PLATFORM) || defined(CS_D2000_PLATFORM) || defined(CS_M3720_PLATFORM) || defined(CS_NXP_PLATFORM) || defined(LEYAN_E2000Q) || defined(LEYAN_D2000) || defined(LEYAN_FT2000)|| defined(TRX_E2000Q) 
            if (strncmp("/dev/pts", cmdbuf, sizeof("/dev/pts")-1))
            {
                login_console = 1;
            }
#else
			if (!strncmp("/dev/tty", cmdbuf, sizeof("/dev/tty")-1))
            {
                login_console = 1;
            }
#endif
        }

        pclose(fp);
    }
#endif

    if ( -1 == get_product_type( g_product_type ) )
    {
        memset(g_product_type, 0, sizeof(g_product_type));
        memcpy(g_product_type, "NSSP", 4);
    }

    if (init_curses_with_fallback() != 0)
    {
        signal_init();
        loguser = getenv("USER");
        run_minimal_cli(login_console, is_admin_user(loguser));
        exit(EXIT_SUCCESS);
    }
    signal_init();
    curs_set(0);

    get_hw_sn(login_console);

    loguser = getenv("USER");

    if (NULL != loguser && (0 == strcmp(loguser, "root")
        ||0 == strcmp(loguser, "LOGIN")
        ||0==strcmp(loguser, "superman")
        ||0==strcmp(loguser, "admin")
        ||0==strcmp(loguser, "kladmin")
        ||0==strcmp(loguser, "recovery")))
    {
        do
        {
            if (login_console)
                choice = getchoice( "[Main Menu]", main_menu_console );
            else
                choice = getchoice( "[Main Menu]", main_menu );

            switch ( choice )
            {
                case 's':
                    system_status();
                    break;

                case 'w':
                    op_mode_switch();
                    break;

                case 'm':
                    mgmt_ip_address();
                    break;

                case '6':
                    system_time();
                    break;

                case '2':
                    add_route();
                    break;

                case 'r':
                    reset_default();
                    break;

                case 'c':
                    reset_data();
                    break;
                    
                case '1':
                    show_serial_num();
                    break;

                case '3':
                    save_configure();
                    break;

                case 'u':
                    system_upgrade();
                    break;

                case 'd':
                    download_debug_info();
                    break;
            
                case 'e':
                    reboot_system();
                    break;

                case 'b':
                    show_console_seed();
                    break;
#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
				case 'h':
					endwin();
					system("clear");
					execl("/usr/local/hycli/bin/hycli", "hycli", NULL);
					initscr();
					curs_set(0);
					break;
#endif


                case 'l':
#if defined(ruijie)
                    logout_authentication();
#else
#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
                    if (login_console)
                    {
                        logout_authentication();
                    }
                    else
#endif
                    {
                        debug_super_key_pos = 0;
                        keypad( stdscr, TRUE );
                        cbreak();
                        noecho();
                        while(1)
                        {
                            int key = getch();
                            if (debug_super_key[++debug_super_key_pos] != key)
                                break;

                            if (debug_super_key_pos == debug_super_key_len)
                            {
                                key = getch();
                                if (key == KEY_ENTER || key == '\n')
                                    break;
                                else
                                    debug_super_key_pos = 0;
                                break;
                            }
                        }

                        keypad( stdscr, FALSE );
                        raw();
                        echo();

                        if (debug_super_key_pos == debug_super_key_len)
                            logout_authentication();
                    }
#endif
                    break;

                case '4':
                    reset_nm_password();
                    break;

                case '5':
                    show_all_versions();
                    break;

                default:
                    break;
            }
        }
        while (1);
    }
    else
    {
        do
        {
            if (login_console)
                choice = getchoice( "[Main Menu]", main_menu_console );
            else
                choice = getchoice( "[Main Menu]", main_menu );

            switch ( choice )
            {
                case '4':
                    reset_nm_password();
                    break;

                case 'l':
                    endwin();
                    system("clear");
                    system("exit");
                    break;

                default:
                    werase_line(stdscr, 21, 10, 20);
                    mvprintw( 21, 10, "Note: Permission denied ! Please login as super administrator!" );
                    refresh();
                    break;
            }
        }
        while (1);
        
    }

   endwin();
   system("clear");
   exit( EXIT_SUCCESS );
}

#endif
