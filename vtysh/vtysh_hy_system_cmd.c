// SPDX-License-Identifier: GPL-2.0-or-later
/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 */

#include <zebra.h>

#include <pwd.h>
#include <grp.h>

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/file.h> 

/* readline carries some ancient definitions around */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <readline/readline.h>
#include <readline/history.h>
#pragma GCC diagnostic pop

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <linux/if.h>    
#include <linux/sockios.h>  
#include <unistd.h>      

#include "linklist.h"
#include "command.h"
#include "memory.h"
#include "network.h"
#include "filter.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_hy_system_cmd.h"
#include "lib/vtysh_daemons.h"
#include "log.h"
#include "vrf.h"
#include "libfrr.h"
#include "command_graph.h"
#include "frrstr.h"
#include "json.h"
#include "ferr.h"
#include "vtysh/vtysh_hy_system_cmd_clippy.c"
#include "vty.h"
#include <ctype.h>

#ifdef CLIPPY
/* clippy/clidef can't process the DEFPY below without some value for this */
#define DAEMONS_LIST "daemon"
#endif

//#ifdef HYLAB_CLI
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <sys/stat.h>
#include "../../ace_include/ace_common_macro.h"
#include "../../ace_include/ace_common_struct.h"
#include <time.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <termios.h>
#include <shadow.h>

extern int vtysh_exit(struct vty *vty);
extern void execute_command(const char *command, int argc, const char *arg1,
			    const char *arg2);

static char *__vtysh_str_rtrim (char *s)
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

static char *__vtysh_str_ltrim (char *s)
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

static char *vtysh_str_trim (char *s)
{
    char*p = __vtysh_str_ltrim(s);
	p = __vtysh_str_rtrim(p);
	
    return p;
}

#ifndef ace_printf
#define ace_printf( fmt... ) syslog( LOG_USER | LOG_INFO,  fmt )
#endif

static int g_configuration_reauthentication = 1;
int do_termios_oldt = 0;

char g_hw_sn[33] = {0};
char g_console_password_seed[4096] = {0};

static size_t b64_encode(const unsigned char *src, size_t len, char *dst) {
    static const char tab[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i = 0, o = 0;
    while (i + 2 < len) {
        unsigned a = src[i++];
        unsigned b = src[i++];
        unsigned c = src[i++];
        dst[o++] = tab[(a >> 2) & 0x3F];
        dst[o++] = tab[((a & 0x3) << 4) | ((b >> 4) & 0x0F)];
        dst[o++] = tab[((b & 0x0F) << 2) | ((c >> 6) & 0x03)];
        dst[o++] = tab[c & 0x3F];
    }
    if (i + 1 < len) {
        unsigned a = src[i++];
        unsigned b = src[i++];
        dst[o++] = tab[(a >> 2) & 0x3F];
        dst[o++] = tab[((a & 0x3) << 4) | ((b >> 4) & 0x0F)];
        dst[o++] = tab[(b & 0x0F) << 2];
        dst[o++] = '=';
    } else if (i < len) {
        unsigned a = src[i++];
        dst[o++] = tab[(a >> 2) & 0x3F];
        dst[o++] = tab[(a & 0x3) << 4];
        dst[o++] = '=';
        dst[o++] = '=';
    }
    return o;
}
static int console_data_encrypt(char* dynamic_key, int dynamic_len, char* share_key, int share_len, unsigned char* in_byte,unsigned int in_len, unsigned char* out_byte, unsigned int out_len)
{
	MD5_CTX  ctx;
	unsigned char md5_out[32];
	//unsigned char md5_in[64];
	//int md5_len;
	char md5_text[64];
	unsigned int encrypt_len;
	AES_KEY encryptKey;
	//md5_len = snprintf((char*)md5_in, sizeof(md5_in), "%s%s", share_key, dynamic_key);
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

static int console_data_decrypt(char* dynamic_key, int dynamic_len, char* share_key, int share_len, unsigned char* in_byte,unsigned int in_len, unsigned char* out_byte, unsigned int out_len)
{
	MD5_CTX  ctx;
	unsigned char md5_out[32];
	//unsigned char md5_in[64];
	//int md5_len;
	char md5_text[64];
	unsigned int encrypt_len;
	AES_KEY encryptKey;
	//md5_len = snprintf((char*)md5_in, sizeof(md5_in), "%s%s", share_key, dynamic_key);
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
	unsigned int len;
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

static void get_console_password(char *passwd, char *seed, int *seed_len)
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

static void password_prompt(const char *prompt, char *buffer, size_t size) 
{
    struct termios oldt, newt;
    int ch;
    size_t len = 0;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
	do_termios_oldt = 1;

    newt.c_lflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("%s", prompt);
    fflush(stdout);

    while ((ch = getchar()) != '\n' && len < size - 1) {
        buffer[len++] = ch;
    }
    buffer[len] = '\0';

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	do_termios_oldt = 0;

    printf("\n");
}

static int is_console_display(void)
{
    static int g_is_vm_x86 = -1;
    if (g_is_vm_x86 < 0) {
        int fd = open("/usr/private/Firmware_Board_Type", O_RDONLY);
        if (fd >= 0) {
            char buf[128];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (n > 0) {
                buf[n] = '\0';
                g_is_vm_x86 = strstr(buf, ":VM-X86") ? 1 : 0;
            } else {
                g_is_vm_x86 = 0;
            }
        } else {
            g_is_vm_x86 = 0;
        }
    }
	char name[128] = { 0 };
    const char *dev = NULL;
    dev = ttyname(STDIN_FILENO);
    if (!dev) dev = ttyname(STDOUT_FILENO);
    if (!dev) dev = ttyname(STDERR_FILENO);
    if (dev) {
        strncpy(name, dev, sizeof(name) - 1);
    } else {
        char linkpath[64];
        char target[256];
        ssize_t n = 0;
        snprintf(linkpath, sizeof(linkpath), "/proc/self/fd/%d", STDIN_FILENO);
        n = readlink(linkpath, target, sizeof(target) - 1);
        if (n <= 0) {
            snprintf(linkpath, sizeof(linkpath), "/proc/self/fd/%d", STDOUT_FILENO);
            n = readlink(linkpath, target, sizeof(target) - 1);
        }
        if (n <= 0) {
            snprintf(linkpath, sizeof(linkpath), "/proc/self/fd/%d", STDERR_FILENO);
            n = readlink(linkpath, target, sizeof(target) - 1);
        }
        if (n > 0) {
            target[n] = '\0';
            strncpy(name, target, sizeof(name) - 1);
        } else {
            const char *ssh_tty = getenv("SSH_TTY");
            if (ssh_tty && ssh_tty[0]) return 0;
            const char *term = getenv("TERM");
            if (term && !strncmp(term, "linux", sizeof("linux") - 1)) return 1;
            return 0;
        }
    }
	
	if (!name[0])
		return 0;
	
    if (!strncmp("/dev/pts", name, sizeof("/dev/pts") - 1))
        return 0;
    if (!strcmp("/dev/console", name))
        return 0;
    if (!strncmp("/dev/hvc", name, sizeof("/dev/hvc") - 1))
        return 1;
    if (!strncmp("/dev/tty", name, sizeof("/dev/tty") - 1)) {
        const char *p = name + sizeof("/dev/tty") - 1;
        if (*p >= '0' && *p <= '9')
            return 1;
        if (!strncmp(p, "S", 1))
            return g_is_vm_x86 ? 1 : 0;
        if (!strncmp(p, "AMA", 3))
            return g_is_vm_x86 ? 1 : 0;
        if (!strncmp(p, "USB", 3))
            return g_is_vm_x86 ? 1 : 0;
        if (!strncmp(p, "ACM", 3))
            return g_is_vm_x86 ? 1 : 0;
        return 0;
    }
    return 0;
}

static int need_reauthentication(struct vty *vty)
{
	if(g_configuration_reauthentication){
		char password[256];
		memset(password, 0, sizeof(password));
		vty_out(vty, "This command is disruptive or has significant impact on services. Please enter the login password again for reauthentication.\n"
			"Password:");
		password_prompt("", password, sizeof(password)-1);
		
		struct spwd *shadow_entry = getspnam("root");
		if (!shadow_entry) {
			vty_out(vty, "password error!\n");
			return 1;
		}
		
		char *encrypted = crypt(password, shadow_entry->sp_pwdp);
		if (!encrypted) {
			vty_out(vty, "password error!\n");
			return 1;
		}
		
		int match = (strncmp(encrypted, shadow_entry->sp_pwdp, sizeof(password)) == 0);
		if (match) {		
			return 0;
		} else {
			vty_out(vty, "password error!\n");
			return 1;
		}
	}

	return 0;
}

static void confirm_prompt(const char *prompt, char *buffer, size_t size)
{
	struct termios oldt, newt;
	int ch;
	size_t len = 0;
	memset(buffer, 0, size);
	
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	do_termios_oldt = 1;

	//newt.c_lflag &= ~(ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	printf("%s", prompt);
	fflush(stdout);

	while ((ch = getchar()) != '\n' && len < size - 1) {
		buffer[len++] = ch;
	}
	buffer[len] = '\0';

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	do_termios_oldt = 0;

	printf("\n");
}

static int need_confirm(const char *prompt, int default_yes)
{
	char buf[32];
	confirm_prompt(prompt, buf, sizeof(buf));
	if(default_yes){
		if(strlen(buf) && strncmp(buf, "Y", sizeof("Y"))
			&& strncmp(buf, "y", sizeof("y"))){
			return 1;
		}
	}
	else{
		if(!strlen(buf) || (strlen(buf) && strncmp(buf, "Y", sizeof("Y"))
			&& strncmp(buf, "y", sizeof("y")))){
			return 1;	
		}
	}

	return 0;
}

static int save_configuration(void)
{
	int retval = 0;
	const char *cmdBuf = "/usr/local/php/bin/php -r \"ace_config_file_save(4000);\" > /dev/null 2>&1";
	retval = system(cmdBuf);
	return retval;
}

static int backup_configuration(void)
{
	int retval = 0;
	const char *cmdBuf = "/usr/local/php/bin/php -r \"ace_config_checkout();\" > /dev/null 2>&1";
	retval = system(cmdBuf);
	return retval;
}

static int restore_configuration(const char *filename)
{
	int retval = 0;
	char cmdBuf[512];
	const char *format = "/usr/local/php/bin/php -r \"ace_config_checkin(%s);\" > /dev/null 2>&1";
	snprintf(cmdBuf, sizeof(cmdBuf), format, hylab_escape_shell_arg(filename));
	retval = system(cmdBuf);
	system("sleep 2; sync;");
	return retval;
}

static int upgrade_system(const char *filename)
{
	int retval = 0;
	char cmdBuf[512];
	snprintf(cmdBuf, sizeof(cmdBuf), "ace_upsys -f /mnt/uploadsystem/%s", 
		hylab_escape_shell_arg(filename));
	retval = system(cmdBuf);
	system("sync;");
	return retval;
}

static int rollback_system(void)
{
	int retval = 0;
	retval = system("ace_upsys -f /usr/download/NACFirmware.bin.old");
	system("sync;");
	return retval;
}

static void find_first_file(struct vty *vty, const char *dir, char *ofile, int len)
{
    DIR *dirp;
    struct dirent *direntp;
	
	memset(ofile, 0, len);
	
    if((dirp=opendir(dir)) ==NULL){
		vty_out(vty, "opendir failed\n");
        return;
    }
             
    while((direntp=readdir(dirp))!=NULL) {
        char file[1024]; 
        struct stat statbuf;
        
        snprintf(file, sizeof(file), "%s/%s", dir, direntp->d_name);
        if(stat(file, &statbuf)==-1){
            continue;
        }
    
        if(S_ISREG(statbuf.st_mode)){
			snprintf(ofile, len, "%s", direntp->d_name);
			//vty_out(vty, "ofile:[%s]\n", ofile);
			break;
        }
    }  
	
    closedir(dirp); 
}

static int ace_online_update(char* token, int value)
{
	unsigned int feature_autoUpgrade_flag = 0;
	unsigned int urllib_autoUpgrade_flag = 0; 
	unsigned int autoUpgrade_delay_flag = 0;
	unsigned int autoUpgrade_delay_time = 0;
	char autoUpgrade_http_server[24];
	unsigned int http_proxy = 0;
	char http_proxy_address_ip[24];
	unsigned int http_proxy_address_port = 80;
	char http_proxy_user_name[512];
	char http_proxy_user_passwd[512];
	unsigned int ipslib_autoUpgrade_flag = 0;
	unsigned int avlib_autoUpgrade_flag = 0;
	unsigned int waflib_autoUpgrade_flag = 0;
	unsigned int vslib_autoUpgrade_flag = 0;

	char line[512];
	FILE* fptr;
	char * tmp;
	const char *filename = "/etc/auto_upgrade.conf";
	const char *def1 = "#feature_flag urllib_flag delay_flag delay_time http_server http_proxy http_proxy_address_ip http_proxy_address_port http_proxy_user_name http_proxy_user_passwd ips av waf vs\r\n";

	snprintf(autoUpgrade_http_server, sizeof(autoUpgrade_http_server), "0.0.0.0");
	snprintf(http_proxy_address_ip, sizeof(http_proxy_address_ip), "127.0.0.1");
	snprintf(http_proxy_user_name, sizeof(http_proxy_user_name), "test");
	snprintf(http_proxy_user_passwd, sizeof(http_proxy_user_passwd), "123456");

	if (NULL != (fptr = fopen(filename, "r"))) {
		memset(line, 0x00, sizeof(line));
		while(fgets( line, sizeof(line), fptr)){
			if(line[0] == '#'){
				memset(line, 0x00, sizeof(line));
				continue;
			}
			sscanf(line, "%d %d %d %d %s %d %s %d %s %s %d %d %d %d", 
				&feature_autoUpgrade_flag, 
				&urllib_autoUpgrade_flag, 
				&autoUpgrade_delay_flag, 
				&autoUpgrade_delay_time, 
				autoUpgrade_http_server,
				&http_proxy,
				http_proxy_address_ip,
				&http_proxy_address_port,
				http_proxy_user_name,
				http_proxy_user_passwd,
				&ipslib_autoUpgrade_flag,
				&avlib_autoUpgrade_flag,
				&waflib_autoUpgrade_flag,
				&vslib_autoUpgrade_flag);
			break;
		}
		fclose(fptr);
	}

	if(!strncmp(token, "applib", sizeof("applib")-1)){
		if(value){
			feature_autoUpgrade_flag |= 1;
		}
		else{
			feature_autoUpgrade_flag &= (~1);
		}
	}
	else if(!strncmp(token, "urllib", sizeof("urllib")-1)){	
		urllib_autoUpgrade_flag = value;
	}
	else if(!strncmp(token, "avlib", sizeof("avlib")-1)){	
		avlib_autoUpgrade_flag = value;
	}
	else if(!strncmp(token, "ipslib", sizeof("ipslib")-1)){	
		ipslib_autoUpgrade_flag = value;
	}
	else if(!strncmp(token, "waflib", sizeof("waflib")-1)){	
		waflib_autoUpgrade_flag = value;
	}
	else if(!strncmp(token, "geoip-lib", sizeof("geoip-lib")-1)){	
		if(value){
			feature_autoUpgrade_flag |= (1<<2);
		}
		else{
			feature_autoUpgrade_flag &= (~(1<<2));
		}
	}
	else if(!strncmp(token, "security-client", sizeof("security-client")-1)){	
		if(value){
			feature_autoUpgrade_flag |= (1<<3);
		}
		else{
			feature_autoUpgrade_flag &= (~(1<<3));
		}
	}
	else if(!strncmp(token, "safe-patch", sizeof("safe-patch")-1)){	
		if(value){
			feature_autoUpgrade_flag |= (1<<4);
		}
		else{
			feature_autoUpgrade_flag &= (~(1<<4));
		}
	}

	fptr = fopen( filename, "w+" );
	if(NULL == fptr){
		return 0;
	}

	snprintf(line, sizeof(line), "%d %d %d %d %s %d %s %d %s %s %d %d %d %d", 
				feature_autoUpgrade_flag, 
				urllib_autoUpgrade_flag, 
				autoUpgrade_delay_flag, 
				autoUpgrade_delay_time, 
				autoUpgrade_http_server,
				http_proxy,
				http_proxy_address_ip,
				http_proxy_address_port,
				http_proxy_user_name,
				http_proxy_user_passwd,
				ipslib_autoUpgrade_flag,
				avlib_autoUpgrade_flag,
				waflib_autoUpgrade_flag,
				vslib_autoUpgrade_flag);
	fwrite( def1, strlen( def1 ), sizeof( char ), fptr ); 
	fwrite( line, strlen( line ), sizeof( char ), fptr ); 
	fclose(fptr);

    return 0;
}

static int ace_AIS_DB_update(char* filename)
{
    char    cmdBuf[256]     = { 0 };
    int     retval          = 0;

    sprintf( cmdBuf, 
             "/usr/local/php/bin/php -r \"ace_update_l7_filter(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static int ace_URL_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"ace_update_url_lib(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static int ace_IPS_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"ace_update_ips_rules(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
	system("/usr/local/php/bin/php /var/www/html/action/updateIpsConfTable.php > /dev/null &");
    return retval;
}

static int ace_WAF_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"ace_update_waf_rules(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static int ace_AV_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"ace_update_av_rules(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static int ace_GEOIP_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"ace_update_geoip_lists(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static int ace_SCV_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"hytf_update_security_client(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static int ace_SA_DB_update(char* filename)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;

    sprintf( cmdBuf, "/usr/local/php/bin/php -r \"hytf_update_sslvpn_client(%s);\" > /dev/null 2>&1",
             hylab_escape_shell_arg(filename) );
    retval = system( cmdBuf);
    return retval;
}

static void md5_name(char *in, char *out)
{
	MD5_CTX  ctx;
	unsigned char md5_out[32];
	//int md5_len;
	char md5_text[64];
	int encrypt_len;
	memset(md5_out, 0x00, sizeof(md5_out));

	MD5_Init(&ctx);
	MD5_Update(&ctx, (void *)in, strlen(in));
	MD5_Final(md5_out, &ctx);

	memset(md5_text, 0, sizeof(md5_text) );
	for ( encrypt_len = 0; encrypt_len < MD5_DIGEST_LENGTH; encrypt_len++ )
	{
		sprintf(md5_text + (encrypt_len<<1), "%02x", md5_out[encrypt_len]);
	}	

	snprintf(out, 512, "%s", md5_text);
}

static int ace_prepare_update(struct vty *vty, const char* type, char *filename, int len, const char *keyword)
{
    char    cmdBuf[1024]     = { 0 };
    int     retval           = 0;
	char file[512];
	char dir[512];
	char *token = NULL;

    snprintf( cmdBuf, sizeof(cmdBuf), "mkdir -p /mnt/uploadruledb/%s;rm -rf /mnt/uploadruledb/%s/*;"
				"cd /mnt/uploadruledb/%s/;rz -y;",
             type, type, type );
    retval = system( cmdBuf);

    snprintf( dir, sizeof(dir), "/mnt/uploadruledb/%s", type );
	find_first_file(vty, dir, filename, len);
	token = strrchr(filename, '.');

	if(!filename[0] || !token || strncmp(token, ".bin", sizeof(".bin")-1) || keyword&&!strstr(filename, keyword)){
		vty_out(vty, "File error. Please upload %sxxx.bin...\n", keyword?keyword:"");
		return 1;
	}

    snprintf( file, sizeof(file), "/mnt/uploadruledb/%s/%s", type, filename);
	md5_name(filename, filename);
    snprintf( cmdBuf, sizeof(cmdBuf), "mkdir -p /usr/download; mv %s /usr/download/%s", file, filename);

	if(need_confirm("Update rule database. Note:Press [y]/n Then Press Enter To Confirm\n", 1)){
		return 1; 
	}
		
	retval = system(cmdBuf);
    return retval;
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

static int license_sendmsg( U32 txMsgType, U32 rxMsgType, U32 timeout )
{
    //ACE_LICANSE_MSG sendmsg        = { 0 };
    //ACE_LICANSE_MSG recvmsg        = { 0 };
    HYTF_LIC_REQ_MSG sendmsg        = { 0 };
    HYTF_LIC_ACK_MSG recvmsg        = { 0 };

    sendmsg.mtype = ACE_LIC_MSG_ID;
    sendmsg.subType = txMsgType;
    lic_rand_req_fill(&sendmsg);
    if (0 == hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, rxMsgType, &sendmsg, sizeof(sendmsg), &recvmsg, sizeof(recvmsg), timeout*1000))
	{
		if (!lic_rand_check(recvmsg.mtype, recvmsg.subType, recvmsg.checkSum, recvmsg.checkRandom))
        	{
           	//phpext_debug_with_file_line( "invalid checksum");
                return (0 - LOG_LICENSE_IN_BLACKLIST);
        	}
		if (recvmsg.version < LIC_VER_VALUE(LIC_VER_MAIN, LIC_VER_SUB, LIC_VER_EXT))
		{
            	//phpext_debug_with_file_line( "invalid license version:%d.%d.%d (current:%d.%d.%d)", (recvmsg.version>>16)&0xFF, (recvmsg.version>>8)&0xFF, (recvmsg.version)&0xFF,
                //LIC_VER_MAIN, LIC_VER_SUB, LIC_VER_EXT);
                return (0 - LOG_LICENSE_IN_BLACKLIST);
        	}
        	return ( recvmsg.result );
	}
	else
	{
		//phpext_debug_with_file_line( "wait for response timeout");
        return ( 0 - LOG_LICENSE_DEMON_RESPONSE_TIMEOUT );
	}
}

static int lic_get_extmod(HYTF_LIC_MOD_MSG *rsp_msg)
{
    HYTF_LIC_REQ_MSG req_msg = {0};
    //HYTF_LIC_MOD_MSG rsp_msg = {0};

    req_msg.mtype = ACE_LIC_MSG_ID;
    req_msg.subType = HYTF_LIC_GET_MOD_EXT;
    lic_rand_req_fill(&req_msg);
    if (0 == hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, HYTF_LIC_GET_MOD_EXT_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), rsp_msg, sizeof(HYTF_LIC_MOD_MSG), 1000))
    {
        if (lic_rand_check(rsp_msg->mtype, rsp_msg->subType, rsp_msg->checkSum, rsp_msg->checkRandom))
        {
            return 0;
        }
    }

    return -1;
}

static int lic_get_extmod_str(HYTF_LIC_RSP_MSGV2 *rsp_msg)
{
/*expired_days[0] =>firmware_days,
  expired_days[1] =>urllib_days,		 1
  expired_days[2] =>applib_days,		 1
  expired_days[3] =>viruslib_days,		 1
  expired_days[4] =>ipslib_days,		 1
  expired_days[5] =>waflib_days,		 1
  expired_days[6] =>botnetlib_days,
  expired_days[7] =>vslib_days, 		 1
  expired_days[8] =>tilib_days, 		 1
  expired_days[9] =>rsvd ...,
  expired_days[10]=>rsvd ...,
 */
	int i;
	char	 line[1024] 				= { 0 };
	const char	   *module[9]  = {NULL, "urllib", "applib", "viruslib", "ipslib", "waflib", NULL, "vslib", "tilib"};
	HYTF_LIC_MOD_MSG rsp_mod_msg;
	memset(&rsp_mod_msg, 0, sizeof(HYTF_LIC_MOD_MSG));
	lic_get_extmod(&rsp_mod_msg);
	for(i=1; i<9; i++){
		if(module[i] /*&& ((i<<1)&rsp_mod_msg.normal_module)*/){
			int len = strlen(line);
			int days = (rsp_msg->expired_days[i] * 24 - rsp_msg->lic_runout_hours[i])/24;
			days = (days<=0 ? rsp_msg->lic_stop_days[i]: days);
			if(len < sizeof(line)){
				snprintf(line+len, sizeof(line)-len, "%s:%d+", module[i], days);
			}
		}	
	}

	if(line[0]){
		line[strlen(line)-1] = '\0';
	}
	vty_out(vty, "Expiredcontent: %s\n", line);
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

#define BOXED_LINES    (19)
int show_serial_num(void)
{    
#if defined(ARCH_ARM64) || defined(CONFIG_HYTF_FW) || defined(aiTai) || defined(Hillstone)
    HYTF_LIC_REQ_MSG req_msg = {0};
    HYTF_LIC_RSP_MSGV2 rsp_msg = {0};

    req_msg.mtype = ACE_LIC_GET_STATE;
    req_msg.subType = ACE_LIC_GET_LICENSE;
    lic_rand_req_fill(&req_msg);
    char *license_info_detail[3] = {"Unauthorized","Demo","Normal"};

    if (0 != hytf_cmd_exchange(SOCKET_UDP_DPORT_TO_LICENSED, ACE_LIC_GET_LICENSE_RESP, &req_msg, sizeof(HYTF_LIC_REQ_MSG), &rsp_msg, sizeof(HYTF_LIC_RSP_MSGV2), 1000))
        return -1;

	vty_out(vty, "Device Serial Number: %s\n", rsp_msg.host_id);
	vty_out(vty, "\n");
	vty_out(vty, "begin time: %s\n", rsp_msg.start_date);
	vty_out(vty, "License Type: %s\n", license_info_detail[rsp_msg.feature_value]);

    time_t now = time(NULL);
    struct tm tm_expired;
    char buffer[64] = {0};

    now += (rsp_msg.expired_days[0] * 86400 - rsp_msg.lic_runout_hours[0] * 3600);
    localtime_r(&now, &tm_expired);

    snprintf(buffer, sizeof(buffer)-1, "%04d-%02d-%02d",
        tm_expired.tm_year + 1900, tm_expired.tm_mon + 1, tm_expired.tm_mday);

	if ((rsp_msg.feature_value == 0 || rsp_msg.feature_value == 1))
	{
		vty_out(vty, "Demo Expired Date: %s\n", buffer);
	}
	else
	{
		vty_out(vty, "Firmware Update Expired Date: %s\n", buffer);
	}

	vty_out(vty, "Customer: %s\n", rsp_msg.customer);
	vty_out(vty, "Vendor: %s\n", rsp_msg.vendor);

	vty_out(vty, "\n");
	lic_get_extmod_str(&rsp_msg);

    if (rsp_msg.session_value >= 1000000)
        snprintf(buffer, sizeof(buffer)-1, "%uM", rsp_msg.session_value/1000000);
    else if(rsp_msg.session_value >= 1000)
        snprintf(buffer, sizeof(buffer)-1, "%uK", rsp_msg.session_value/1000);
    else
        snprintf(buffer, sizeof(buffer)-1, "%u", rsp_msg.session_value);

	vty_out(vty, "Authorized Session Number: %s\n", buffer);

    snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.l7filter_bandwidth_value);
	vty_out(vty, "Authorized L7 Throughput: %s\n", buffer);
	
    snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.firewall_bandwidth_value);
	vty_out(vty, "Authorized Firewall Throughput: %s\n", buffer);
#else

    char     firewall_string[256] = "";
    char     cmdBuf[256]               = { 0 };
    int      retval                    = 0;
    
    FILE*    fp                        = 0;
    char     filename[256]             = "/usr/.lic/.ace_lic_info";
    char     line[256]                 = { 0 };
    
    
    char     sequence[256]             = { 0 };
    char*    pstr                      = NULL;
	int i = 0;
    
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

		vty_out(vty, "Device Serial Number: %s\n", rsp_msg.host_id);
		vty_out(vty, "\n");
		vty_out(vty, "begin time: %s\n", rsp_msg.start_date);
		vty_out(vty, "License Type: %s\n\n", license_info_detail[rsp_msg.feature_value]);

    	time_t now = time(NULL);
    	struct tm tm_expired;
    	char buffer[64] = {0};

    	now += (rsp_msg.expired_days[0] * 86400 - rsp_msg.lic_runout_hours[0] * 3600);
    	localtime_r(&now, &tm_expired);

    	snprintf(buffer, sizeof(buffer)-1, "%04d-%02d-%02d",
    	    tm_expired.tm_year + 1900, tm_expired.tm_mon + 1, tm_expired.tm_mday);

		if ((rsp_msg.feature_value == 0 || rsp_msg.feature_value == 1))
		{
			vty_out(vty, "Demo Expired Date: %s\n", buffer);
		}
		else
		{
			vty_out(vty, "Firmware Update Expired Date: %s\n", buffer);
		}

		vty_out(vty, "Customer: %s\n", rsp_msg.customer);
		vty_out(vty, "Vendor: %s\n", rsp_msg.vendor);
		
		vty_out(vty, "\n");

		lic_get_extmod_str(&rsp_msg);

    	if (rsp_msg.session_value >= 1000000)
    	    snprintf(buffer, sizeof(buffer)-1, "%uM", rsp_msg.session_value/1000000);
    	else if(rsp_msg.session_value >= 1000)
    	    snprintf(buffer, sizeof(buffer)-1, "%uK", rsp_msg.session_value/1000);
    	else
    	    snprintf(buffer, sizeof(buffer)-1, "%u", rsp_msg.session_value);

		vty_out(vty, "Authorized Session Number: %s\n", buffer);


    	snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.l7filter_bandwidth_value);
		vty_out(vty, "Authorized L7 Throughput: %s\n", buffer);

    	snprintf(buffer, sizeof(buffer)-1, "%uMbps", rsp_msg.firewall_bandwidth_value);
		vty_out(vty, "Authorized Firewall Throughput: %s\n", buffer);
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
						 vty_out(vty, "%s\n", line);
    	                 i++;
    	            }
    	        }
    	        else
    	        {
    	            if ( 0 == strncmp(line, "Device Serial Number", strlen("Device Serial Number")))
    	            {
    	                get_serial_number(sequence);
						vty_out(vty, "Device Serial Number: %s\n",sequence);
    	            }
    	            else
    	            {
    	                if (0 == strncmp(line, "Authorized Firewall Throughput", strlen("Authorized Firewall Throughput")))
    	                {
    	                     strncpy(firewall_string, line, sizeof(firewall_string));
    	                }
    	                else
    	                {
							vty_out(vty, "%s\n", line);
    	                }
    	            }
    	        }
    	    }
    	}
    	
    	if (i < BOXED_LINES - 2 && firewall_string[0])
    	{
    	    if (strlen(firewall_string) < 50)
    	    {
				vty_out(vty, "%s\n", firewall_string);
    	        i++;
    	    }
    	    else
    	    {
				vty_out(vty, "%s\n", firewall_string);
    	        i++;
    	    }
    	}
    }
#endif

    return 0;
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
				vty_out(vty, "%s\n", line);
                i++;
            }
        }
        info_index++;
    }

    return 0;
}

#if 1
#define MAX_INTERFACES 64
#define MAX_IP_ADDRS 8
#define MAX_OUTPUT_SIZE 65536

	struct interface_info {
		char name[16];
		char number[16];
		char full_name[32];
		char status[8];
		char description[256];
		char zone[64]; 
		char mac_addr[32];
		int no_phyif;
		int mtu;
		char link_speed[32];
		char duplex_mode[16];   
    	char auto_negotiation[8];
		char ipv4_addrs[MAX_IP_ADDRS][64];
		char ipv6_addrs[MAX_IP_ADDRS][64];
		int ipv4_count;
		int ipv6_count;
		unsigned long tx_frames;  
    	unsigned long tx_bytes;  
    	unsigned long rx_frames;  
    	unsigned long rx_bytes;
		unsigned long tx_errors;    
    	unsigned long tx_dropped;   
    	unsigned long tx_overruns; 
    	unsigned long rx_errors;  
    	unsigned long rx_dropped; 
    	unsigned long rx_overruns;
		
	};
	
	
	static char* get_command_output(const char* cmd) {
		FILE* fp = popen(cmd, "r");
		if (!fp) return NULL;
	
		char* buffer = malloc(MAX_OUTPUT_SIZE);
		if (!buffer) {
			pclose(fp);
			return NULL;
		}
	
		size_t len = fread(buffer, 1, MAX_OUTPUT_SIZE - 1, fp);
		buffer[len] = '\0';
		pclose(fp);
		return buffer;
	}
	
	
	static void parse_hardware_interfaces(const char* output, struct interface_info* interfaces, int count) {
		char* copy = strdup(output);
		char* line = strtok(copy, "\n");
		int n = 0, found = 0;
		struct interface_info* iface = NULL;
		while (line) {
			if(line[0] && !isspace(line[0])){
				iface = NULL;
				found = 0;
			}
			
			if (strstr(line, "GE") == line 
				|| strstr(line, "Eth") == line
				|| strstr(line, "FE") == line
				|| strstr(line, "XGE") == line
				|| strstr(line, "TGE") == line
				|| strstr(line, "FGE") == line
				|| strstr(line, "HGE") == line
				|| strstr(line, "MGT") == line
				|| strstr(line, "HA") == line
				|| strstr(line, "Agg") == line
				|| strstr(line, "loop") == line
				|| strstr(line, "LAN") == line
				|| strstr(line, "WAN") == line) {
				char iface_name[32];
				if (sscanf(line, "%31s", iface_name) == 1) {
					int j;
					if (iface_name[0] == 'l') { /*loop*/
						iface_name[0] = ' ';
						iface_name[1] = ' ';
						iface_name[2] = 'B';
						iface_name[3] = 'r';
						vtysh_str_trim(iface_name);
					}

					for (j = 0; iface_name[j]; j++) {
						if (isdigit(iface_name[j])) break;
					}

					for (int i = 0; i < count; i++) {
						if (strstr(iface_name, interfaces[i].full_name)) {
							n = i;
							found = 1;
							break;
						}
					}
	
					if (found) {
						iface = &interfaces[n];
						//memset(iface, 0, sizeof(struct interface_info));
					
						strncpy(iface->name, iface_name, j);
						iface->name[j] = '\0';
						strcpy(iface->number, iface_name + j);
						//strcpy(iface->full_name, iface_name);

						//strcpy(iface->duplex_mode, "unknown");
                    	//strcpy(iface->auto_negotiation, "unknown");
					}
				}
			}
				
			if(iface){			
				if(strstr(line ,"carrier up")) {
					strcpy(iface->status, "up");
				}
				
				if(strstr(line ,"carrier down")) {
					strcpy(iface->status, "down");
				}

				if(strstr(line ,"half duplex")) {
					strcpy(iface->duplex_mode, "half");
				}

				if(strstr(line ,"full duplex")) {
					strcpy(iface->duplex_mode, "full");
				}

				if(strstr(line ,"Auto-negotiation on")) {
					strcpy(iface->auto_negotiation, "on");
				}

				if(strstr(line ,"Auto-negotiation off")) {
					strcpy(iface->auto_negotiation, "off");
				}

				if(strstr(line ,"Link speed:")) {
					strncpy(iface->link_speed, line, sizeof(iface->link_speed));
					vtysh_str_trim(iface->link_speed);
				}

				char* mac_pos = strstr(line, "Ethernet address");
            	if (mac_pos) {
                	char* mac_start = mac_pos + strlen("Ethernet address ");
                	sscanf(mac_start, "%17s", iface->mac_addr);
            	}

				char* tx_frames_pos = strstr(line, "tx frames ok");
            	if (tx_frames_pos) {
                	sscanf(tx_frames_pos + strlen("tx frames ok"), "%lu", &iface->tx_frames);
            	}

            	char* tx_bytes_pos = strstr(line, "tx bytes ok");
            	if (tx_bytes_pos) {
                	sscanf(tx_bytes_pos + strlen("tx bytes ok"), "%lu", &iface->tx_bytes);
            	}

            	char* rx_frames_pos = strstr(line, "rx frames ok");
            	if (rx_frames_pos) {
               	 	sscanf(rx_frames_pos + strlen("rx frames ok"), "%lu", &iface->rx_frames);
            	}

            	char* rx_bytes_pos = strstr(line, "rx bytes ok");
            	if (rx_bytes_pos) {
               		sscanf(rx_bytes_pos + strlen("rx bytes ok"), "%lu", &iface->rx_bytes);
            	}
			}
			line = strtok(NULL, "\n");
		}
		free(copy);
	}


	static int netmask_to_prefix(const char *netmask) {
    struct in_addr addr;
    if (inet_pton(AF_INET, netmask, &addr) != 1) {
        return -1;
    }
    
    unsigned long mask = ntohl(addr.s_addr);
    if (mask == 0) return 0;
    
   
    int prefix = 0;
    while ((mask & 0x80000000) && prefix < 32) {
        prefix++;
        mask <<= 1;
    }
    
    
    if (mask != 0) {
        return -1; 
    }
    
    return prefix;
}
	
	
	static void parse_ifconfig_ips(const char* output, struct interface_info* interfaces, int *count) {
		char* copy = strdup(output);
		char* line = strtok(copy, "\n");
		char* current_iface = NULL;
		int i = 0;
		*count = 0;

		while (line) {
			if (isalpha(line[0]) && strstr(line, "Link encap:")) { 
				current_iface = NULL;
				if ((strstr(line, "GE") == line 
					|| strstr(line, "Eth") == line
					|| strstr(line, "FE") == line
					|| strstr(line, "XGE") == line
					|| strstr(line, "TGE") == line
					|| strstr(line, "FGE") == line
					|| strstr(line, "HGE") == line
					|| strstr(line, "MGT") == line
					|| strstr(line, "HA") == line
					|| strstr(line, "Agg") == line
					|| strstr(line, "Br") == line
					|| strstr(line, "LAN") == line
					|| strstr(line, "WAN") == line) && (*count)<=MAX_INTERFACES) {
					char *token = strchr(line, ' ');
					if(token){
						*token = '\0';
					}	
					i = (*count);
					memset(&interfaces[i], 0, sizeof(struct interface_info));
					current_iface = &interfaces[i];
					snprintf(interfaces[i].full_name, sizeof(interfaces[i].full_name), "%s", line);	
					if(strstr(line, "Br") || strstr(line, "Agg") || strstr(line, ".")){
						interfaces[i].no_phyif = 1;
					}
					(*count)++;
					
				}
			}
			else if (current_iface) {
				if (strstr(line, "inet addr:") && interfaces[i].ipv4_count < MAX_IP_ADDRS) {
                	char ip[64], bcast[64], mask[64];
                	if (sscanf(strstr(line, "inet addr:") + 10, "%s Bcast:%s Mask:%s", ip, bcast, mask) == 3) {
                    	int prefix = netmask_to_prefix(mask);
                    	if (prefix > 0) {
                        	snprintf(interfaces[i].ipv4_addrs[interfaces[i].ipv4_count++], 
                                	64, "%s/%d", ip, prefix);
                    	} else {
                        // 如果无法转换，只显示IP地址
                        	strncpy(interfaces[i].ipv4_addrs[interfaces[i].ipv4_count++], ip, 64);
                    	}
                	} 
            	}
            
            	else if (strstr(line, "inet6 addr:") && interfaces[i].ipv6_count < MAX_IP_ADDRS) {
                	char addr[64], scope[32];
                	if (sscanf(strstr(line, "inet6 addr:") + 11, "%s %s", addr, scope) >= 1) {
                   
                    	if (strstr(scope, "Scope:")) {
                        	snprintf(interfaces[i].ipv6_addrs[interfaces[i].ipv6_count++], 
                                	64, "%s %s", addr, scope);
                    	} else {
                        	strncpy(interfaces[i].ipv6_addrs[interfaces[i].ipv6_count++], addr, 64);
                    	}
                	}
            	}
				
				else if (strstr(line, "MTU:")) {
                	sscanf(strstr(line, "MTU:") + 4, "%d", &interfaces[i].mtu);
            	}

				else if (strstr(line, "TX packets:")) {

					sscanf(strstr(line, "errors:") + 7, "%lu", &interfaces[i].tx_errors);
					sscanf(strstr(line, "dropped:") + 8, "%lu", &interfaces[i].tx_dropped);
					sscanf(strstr(line, "overruns:") + 9, "%lu", &interfaces[i].tx_overruns);
				}
				
				else if (strstr(line, "RX packets:")) {

					sscanf(strstr(line, "errors:") + 7, "%lu", &interfaces[i].rx_errors);
					sscanf(strstr(line, "dropped:") + 8, "%lu", &interfaces[i].rx_dropped);
					sscanf(strstr(line, "overruns:") + 9, "%lu", &interfaces[i].rx_overruns);
				}
			}
			line = strtok(NULL, "\n");
		}
		free(copy);
	}

#define MAX_LOGFILE_SIZE (2000*1024)
#define TIME_LEN 100
#define SYSAPPLOG            "/var/log/hyclid"

static const char* getcurdate(long * ioSec) 
{ 
    static time_t m_time; 
    struct tm pNow; 
    static char tmpDate[TIME_LEN] = { 0 }; 

    memset(&m_time, 0, sizeof(time_t));
    m_time = time( NULL ); 
    if (NULL != ioSec) 
    { 
        *ioSec = m_time; 
    } 

    localtime_r(&m_time,&pNow); 
    memset(tmpDate, 0, sizeof(tmpDate)); 
    sprintf( tmpDate, "%4d-%02d-%02d %02d:%02d:%02d", 
            pNow.tm_year + 1900, pNow.tm_mon + 1, pNow.tm_mday, 
            pNow.tm_hour, pNow.tm_min, pNow.tm_sec); 

    return tmpDate; 
}

static long get_file_size(const char * filename )
{
    struct stat f_stat = { 0 };

    if( stat( filename, &f_stat ) == -1 )
    {
        return -1;
    }

   return (long)f_stat.st_size;
}

static int log_printf(const char * fmt, ...) 
{ 
    char curtime[TIME_LEN]    = { 0 }; 
    FILE *fp                  = NULL;
    long ace_time             = 0;
    long file_size            = 0;
    int  n                    = 0;
    va_list ap; 

    strcpy( curtime, getcurdate( &ace_time) ); 
    file_size = get_file_size(SYSAPPLOG);

    if ( file_size >= MAX_LOGFILE_SIZE )
    {
        fp = fopen( SYSAPPLOG, "w+" );        
    }
    else
    {   
        fp = fopen( SYSAPPLOG, "a+" ); 
    }

    if ( NULL == fp )
    {
        fp = fopen( SYSAPPLOG, "w+");
    }

    if ( NULL == fp ) 
    {
        return -1;
    }

    fprintf( fp, " %s : ",  curtime ); 

    va_start( ap, fmt );
    n = vfprintf( fp, fmt, ap );
    va_end(ap); 

    fprintf( fp, "\n" );
    fflush( fp );
    fclose( fp );
    return n;
}

static void nl_header_init( struct nlmsghdr* p_h,int size,int type )
{
    static int  seq_num = 1;

    memset( p_h, 0, sizeof( struct nlmsghdr ) );
    p_h->nlmsg_len = NLMSG_LENGTH( size );
    p_h->nlmsg_type = type;
    p_h->nlmsg_pid = getpid();
    p_h->nlmsg_seq = seq_num++;
}

static int hylab_cmd_unix_exchange(const char* cmd_path, long reply_type,
    void* send_buf, int send_len, void* recv_buf, int recv_len, unsigned int reply_timeout_ms)
{
    fd_set read_fset;
    socklen_t peer_len;
    struct sockaddr_un server_addr;
	int size;
	int len;
    int pkt_recv_len = 0, pkt_send_len;
	char my_path[sizeof(server_addr.sun_path)];
    int cmd_result = 0;
    int sock_fd = -1;
    struct timeval tv = {(reply_timeout_ms/1000), (reply_timeout_ms%1000)*1000};

	my_path[0] = 0;
    if((sock_fd = socket(AF_UNIX,SOCK_DGRAM,0)) < 0)
    {  
        log_printf("failed to socket reply_type %ld errno %d cmd_path %s\r\n", reply_type,errno,cmd_path);
        cmd_result = -1;
        goto error_out;
    }

#if 1
    int ret;

    // cat /proc/sys/net/core/rmem_default 
    // cat /proc/sys/net/core/wmem_default 
    if (send_len >= 262144) {
    	size = send_len + 1024;
    	ret = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    	if (ret < 0) {
    		log_printf("setsockopt rcvbuf %d(%s) failed", errno,strerror(errno));
            cmd_result = -5;
            goto error_out;
    	}
    	
    	ret = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    	if (ret < 0) {
    		log_printf("setsockopt rcvbuf %d(%s) failed", errno,strerror(errno));
            cmd_result = -6;
            goto error_out;
    	}
    }
#endif

	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;

	if (recv_buf && recv_len)
	{
		size = snprintf(my_path, sizeof(my_path), "/tmp/.unix_p%u", getpid());
		unlink(my_path);
		strncpy(server_addr.sun_path, my_path, sizeof(server_addr.sun_path));
		server_addr.sun_path[sizeof(server_addr.sun_path)-1] = 0;
		len = offsetof (struct sockaddr_un, sun_path) + size;

		if (bind (sock_fd, (struct sockaddr *)&server_addr, len) < 0) {
			log_printf("bind reply_type %ld rcvbuf %d(%s) failed", reply_type,errno,strerror(errno));
	        cmd_result = -2;
	        goto error_out;
		}
	}

	size = snprintf(server_addr.sun_path, sizeof(server_addr.sun_path), "%s", cmd_path);
	len = offsetof (struct sockaddr_un, sun_path) + size;

    pkt_send_len = sendto(sock_fd, send_buf, send_len, 0, (struct sockaddr*)&server_addr, len);
    if (0 > pkt_send_len)
    {
        cmd_result = -3;
        log_printf("sendto reply_type %ld cmd_result %d errno %d cmd_path %s\r\n", reply_type,pkt_send_len,errno,cmd_path);
        goto error_out;
    }
  //phpext_debug_with_file_line("sendto cmd_result %d\r\n", pkt_len);

    if (recv_len && recv_buf)
    {
        FD_ZERO(&read_fset);
        FD_SET (sock_fd, &read_fset);

        if(0 < select((sock_fd + 1), &read_fset, NULL, NULL, &tv))
        {
            peer_len = sizeof(struct sockaddr_un);
			server_addr.sun_path[0] = 0;
            pkt_recv_len = recvfrom(sock_fd, recv_buf, recv_len, 0, (struct sockaddr*)&server_addr, &peer_len);
            if (0 > pkt_recv_len)
            {
                cmd_result = -4;
            }
//          	phpext_debug_with_file_line("recvfrom reply_type %ld cmd_result %d from %s\r\n", reply_type,pkt_len,server_addr.sun_path);
        }
        else
        {
            cmd_result = -1;
            log_printf("reply_timeout %ums reply_type %ld errno %d(%s) cmd_path %s send %d %d recv %d %d\r\n", 
                reply_timeout_ms,reply_type,errno,strerror(errno),cmd_path,
                send_len, pkt_send_len, recv_len, pkt_recv_len);
        }
    }

error_out:
    if (sock_fd >= 0)
    {
        close (sock_fd);
    }
	if (my_path[0])
	{
		unlink(my_path);
	}
    return cmd_result;
}


static int hylab_if_set(HYLAB_IF_CFG_SWAP *cfg_swap)
{
    HYLAB_IF_CFG_SWAP cfg_swap2;
    NL_ACK_MSG ack_msg;

    memset(&cfg_swap2, 0, sizeof(cfg_swap2));
    memset(&ack_msg, 0, sizeof(ack_msg));
    memcpy(&cfg_swap2.cfg_info, &cfg_swap->cfg_info, sizeof(cfg_swap->cfg_info));

    if(0 == cfg_swap->cfg_info.name[0])
    {
        return -1;
    }

    nl_header_init(&cfg_swap2.nlh, sizeof(cfg_swap2)-NL_HEAD_LEN, HYLAB_IF_SET);
    if ( 0 == hylab_cmd_unix_exchange(UNIX_SOCK_PATH_FAKE_KERNEL, HYLAB_IF_SET,
                                       ( char * ) &cfg_swap2,
									   cfg_swap2.nlh.nlmsg_len,
									   ( char * ) &ack_msg,
									   sizeof( ack_msg ), 1500 ) )
    {
	//vty_out(vty, "%s,%d\n", __FILE__, __LINE__);
    }

	return 0;
}


static int hylab_if_get(const char *ifname, HYLAB_IF_CFG_SWAP *cfg_swap)
{
    memset(cfg_swap, 0, sizeof(HYLAB_IF_CFG_SWAP));
    nl_header_init(&cfg_swap->nlh, sizeof(*cfg_swap)-NL_HEAD_LEN, HYLAB_IF_GET_BY_NAME);
    strncpy(cfg_swap->filter.name, ifname, sizeof(cfg_swap->filter.name));

    if ( 0 == hylab_cmd_unix_exchange(UNIX_SOCK_PATH_FAKE_KERNEL, HYLAB_IF_GET_BY_NAME,
                                       ( char * ) cfg_swap,
									   cfg_swap->nlh.nlmsg_len,
									   ( char * ) cfg_swap,
									   sizeof( *cfg_swap ), 1500 ) )
    {
    	//vty_out(vty, "v=%d, token=%s, %s,%d\n", v, token, __FILE__, __LINE__);
    }


	return 0;
}

static struct cmd_node interface_node = {
	.node = INTERFACE_NODE,
	.prompt = "%s(config-if-%s)# ",
};

DEFPY(show_interface,
    show_interface_cmd,
    "show interface [NAME$interface_name]",
    SHOW_STR
    "Interface information\n"
    "Interface name (e.g. GE0-0)\n")
{
    struct interface_info interfaces[MAX_INTERFACES];
    int count = 0;
    
    /* 获取数据 */
    char* hw_output = get_command_output("vppctl show hardware-interfaces");
    char* ifconfig_output = get_command_output("ifconfig");
    
    if (!hw_output || !ifconfig_output) {
        vty_out(vty, "Error: Failed to get interface information\n");
        free(hw_output);
        free(ifconfig_output);
        return CMD_WARNING;
    }
    
    
    parse_ifconfig_ips(ifconfig_output, interfaces, &count);
    parse_hardware_interfaces(hw_output, interfaces, count);
    
    free(hw_output);
    free(ifconfig_output);

	for (int i = 0; i < count; i++) {
        HYLAB_IF_CFG_SWAP cfg_swap;
		char full_name[64];
		snprintf(full_name, sizeof(full_name), "%s", interfaces[i].full_name);
		if (full_name[0] == 'B') { /* Br/Bridge */
			if(strstr(full_name, "Bridge")){
				snprintf(full_name, sizeof(full_name), "loop%d", atoi(interfaces[i].full_name+6));
			}
			else{
				snprintf(full_name, sizeof(full_name), "loop%d", atoi(interfaces[i].full_name+2));	
			}
		}

        if (hylab_if_get(full_name, &cfg_swap) == 0) {
       
            if (cfg_swap.cfg_info.desc[0] != '\0') {
                strncpy(interfaces[i].description, cfg_swap.cfg_info.desc, 
                        sizeof(interfaces[i].description) - 1);
                interfaces[i].description[sizeof(interfaces[i].description) - 1] = '\0';
            }
           
            if (cfg_swap.cfg_info.zone[0] != '\0') {
                strncpy(interfaces[i].zone, cfg_swap.cfg_info.zone,
                        sizeof(interfaces[i].zone) - 1);
                interfaces[i].zone[sizeof(interfaces[i].zone) - 1] = '\0';
            }
        }
    }
    
    for (int i = 0; i < count; i++) {
    if ((!interface_name || strcmp(interfaces[i].full_name, interface_name) == 0)) {
        
        vty_out(vty,"\nInterface: %s\n", interfaces[i].full_name);
        vty_out(vty,"  Status: %s\n", interfaces[i].no_phyif?"up":(interfaces[i].status[0]?interfaces[i].status:"down"));

		if (interfaces[i].zone[0]) 
                vty_out(vty, "  Security Zone: %s\n", interfaces[i].zone);

		
        if (interfaces[i].description[0]) 
                vty_out(vty, "  Description: %s\n", interfaces[i].description);
           
    
        if (interfaces[i].mac_addr[0])
            vty_out(vty,"  MAC Address: %s\n", interfaces[i].mac_addr);

		if (interfaces[i].link_speed[0] && !strstr(interfaces[i].link_speed, "unknown"))
            vty_out(vty,"  %s\n", interfaces[i].link_speed);

		if (interfaces[i].duplex_mode[0] && strcmp(interfaces[i].duplex_mode, "unknown") != 0)
            vty_out(vty,"  Duplex Mode: %s\n", interfaces[i].duplex_mode);
                
        if (interfaces[i].auto_negotiation[0] && strcmp(interfaces[i].auto_negotiation, "unknown") != 0)
            vty_out(vty,"  Auto-negotiation: %s\n", interfaces[i].auto_negotiation);

			
        	vty_out(vty,"  MTU: %d\n", interfaces[i].mtu);
        
        for (int j = 0; j < interfaces[i].ipv4_count; j++)
            vty_out(vty,"  IPv4 Address: %s\n", interfaces[i].ipv4_addrs[j]);
    
        for (int j = 0; j < interfaces[i].ipv6_count; j++)
            vty_out(vty,"  IPv6 Address: %s\n", interfaces[i].ipv6_addrs[j]);
    
        	vty_out(vty,"  Statistics:\n");

			vty_out(vty, "    TX frames: %lu, bytes: %lu, errors: %lu, dropped: %lu, overruns: %lu\n", 
       			interfaces[i].tx_frames, interfaces[i].tx_bytes, 
       			interfaces[i].tx_errors, interfaces[i].tx_dropped, interfaces[i].tx_overruns);

			vty_out(vty, "    RX frames: %lu, bytes: %lu, errors: %lu, dropped: %lu, overruns: %lu\n", 
      			interfaces[i].rx_frames, interfaces[i].rx_bytes,
       			interfaces[i].rx_errors, interfaces[i].rx_dropped, interfaces[i].rx_overruns);
    }
	}
    
    return CMD_SUCCESS;
}

static int render_qr_external(struct vty *vty, const char *data, size_t data_len) {
    char tmp[32] = "/tmp/vtyshqrXXXXXX";
    int fd = mkstemp(tmp);
    if (fd < 0) return 0;
    if (data_len > 0) {
        ssize_t w = write(fd, data, data_len);
        (void)w;
    }
    close(fd);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "qrencode -t ANSIUTF8 -m 2 -l L -s 1 -r %s", tmp);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    char buf[512];
    while (fgets(buf, sizeof(buf), fp)) {
        vty_out(vty, "        %s", buf);
    }
    int rc = pclose(fp);
    if (rc != 0) {
        snprintf(cmd, sizeof(cmd), "qrencode -t ASCII -m 2 -l L -s 1 -r %s", tmp);
        fp = popen(cmd, "r");
        if (fp) {
            while (fgets(buf, sizeof(buf), fp)) {
                vty_out(vty, "        %s", buf);
            }
            rc = pclose(fp);
        }
    }
    unlink(tmp);
    return rc == 0;
}

#endif
DEFUN(show_configuration_autobackup,
      show_configuration_autobackup_cmd,
      "show configuration auto-backup",
	SHOW_STR
	  "Daily backup configuration\n"
      "auto backup\n")
{
	vty_out(vty, "Configuration list\n");
	vty_out(vty, "---------------------\n");
	
#if defined(ARCH_ARM64)
	system("ls -l  /home/config/current/autosave_config/ | awk '{print $9}'");
#else
	system("ls -l  /home/config/current/autosave_config/ | awk '{print $8}'");
#endif

    return CMD_SUCCESS;
}

DEFUN(startup_configuration_backup,
      startup_configuration_backup_cmd,
      "startup configuration backup LINE",
	  "startup\n"
	  "Daily backup configuration\n"
      "backup\n"
      "config-number, such as: config-2025-09-10\n")
{
	char file[512];
	snprintf(file, sizeof(file), "/home/config/current/autosave_config/%s", argv[3]->arg);
    if (access(file, 0) != 0){
		vty_out(vty, "%s not found!\n", argv[3]->arg);
		return CMD_SUCCESS; 
	}

    if (license_sendmsg( ACE_LIC_GET_STATE, ACE_LIC_GET_STATE_RESP, 3 ) < 0){
		vty_out(vty, "License expiration, cannot be restored!\n");
		return CMD_SUCCESS; 
	}
		
	if(!need_reauthentication(vty)){
		
		do{
			char cmd[1024];
			if(need_confirm("Restore from an automated backup configuration? [y]/n\n", 1)){
				break; 	
			}

			vty_out(vty, "Restoring the configuration..\n");
			snprintf(cmd, sizeof(cmd), "cp -af %s /home/checkin/Config.conf", file);
			system(cmd);
			restore_configuration("Config.conf");
			vty_out(vty, "The configuration restoration is complete\n");
			
			if(need_confirm("System reboot, are you sure? y/[n]\n", 0)){
				return CMD_SUCCESS; 	
			}
		
			vty_out(vty, "The system is going down NOW!\n");
			system("reboot -f");
		}while(0);		
	}

    return CMD_SUCCESS;
}

DEFUN (show_license_info,
       show_license_info_cmd,
       "show license",
	   SHOW_STR
       "License information\n")
{
	show_serial_num();
	return CMD_SUCCESS;
}

DEFUN (show_version_info,
       show_version_info_cmd,
       "show version",
	   SHOW_STR
       "Version information\n")
{
	show_all_versions();
	return CMD_SUCCESS;
}

DEFUNSH(VTYSH_INTERFACE,
        vtysh_interface,
        vtysh_interface_cmd,
        "interface IFNAME",
        "Select an interface\n"
        "Interface name\n")
{
    vty->node = INTERFACE_NODE;

    if (argc > 0 && argv[0]) {
        const char *name = argv[0]->arg ? argv[0]->arg : (const char *)argv[0];

        memset(vty->ifname, 0, sizeof(vty->ifname));
        strncpy(vty->ifname, name, sizeof(vty->ifname) - 1);

        memset(vty->hy_node_context_name, 0, sizeof(vty->hy_node_context_name));
        strncpy(vty->hy_node_context_name, name, sizeof(vty->hy_node_context_name) - 1);

        vty->qobj_index = 1;
    }

    return CMD_SUCCESS;
}


DEFUNSH (VTYSH_ALL,
         hy_interface_exit,
         hy_interface_exit_cmd,
         "exit",
         "Exit current mode\n")
{
  vty->node = CONFIG_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (VTYSH_ALL,
         hy_interface_end,
         hy_interface_end_cmd,
         "end",
         "End configuration and go to enable mode\n")
{
  vty->node = ENABLE_NODE;
  return CMD_SUCCESS;
}

DEFUN (ip_address,
       ip_address_cmd,
       "ip address A.B.C.D WORD", 
       "Interface Internet Protocol config commands\n"
       "Set the IP address of an interface\n"
       "IP address (e.g. 10.0.0.1)\n"
       "Subnet mask length (1-32)\n")
{
    char cmd[512] = {0};
    const char *ip_addr = NULL;
    const char *mask_str = NULL;
    const char *if_name = vty->ifname;

    if (argc >= 4) {
        ip_addr = argv[2]->arg ? argv[2]->arg : (const char *)argv[2];
        mask_str = argv[3]->arg ? argv[3]->arg : (const char *)argv[3];
    }

    if (if_name[0] == '\0') {
        if_name = vty->hy_node_context_name;
    }

    if (!ip_addr || !mask_str || if_name[0] == '\0') {
        vty_out(vty, "%% Error: Missing parameters.\n");
        return CMD_WARNING;
    }

    int m = atoi(mask_str);
    if (m < 1 || m > 32) {
        vty_out(vty, "%% Error: Mask length must be between 1 and 32.\n");
        return CMD_WARNING;
    }

    snprintf(cmd, sizeof(cmd), "ip addr flush dev %s && ip addr add %s/%d dev %s",
             if_name, ip_addr, m, if_name);

    vty_out(vty, "Executing: %s\n", cmd);
    system(cmd);

    return CMD_SUCCESS;
}

DEFUN (no_ip_address,
       no_ip_address_cmd,
       "no ip address A.B.C.D WORD",
       "Negate a command or set its defaults\n"
       "Interface Internet Protocol config commands\n"
       "Set the IP address of an interface\n"
       "IP address (e.g. 10.0.0.1)\n"
       "Subnet mask length (1-32)\n")
{
    char cmd[512] = {0};
    const char *ip_addr = NULL;
    const char *mask_str = NULL;
    const char *if_name = vty->ifname;

    if (argc >= 2) {
        mask_str = argv[argc - 1]->arg ? argv[argc - 1]->arg : (const char *)argv[argc - 1];
        ip_addr = argv[argc - 2]->arg ? argv[argc - 2]->arg : (const char *)argv[argc - 2];
    }

    if (if_name[0] == '\0' && vty->hy_node_context_name[0] != '\0') {
        if_name = vty->hy_node_context_name;
    }

    if (!ip_addr || !mask_str || if_name[0] == '\0') {
        vty_out(vty, "%% Error: Missing data. IP: %s, Mask: %s, IF: %s\n",
                ip_addr ? ip_addr : "NULL", mask_str ? mask_str : "NULL", if_name);
        return CMD_WARNING;
    }

    int m = atoi(mask_str);
    snprintf(cmd, sizeof(cmd), "ip addr del %s/%d dev %s", ip_addr, m, if_name);
    
    vty_out(vty, "Executing: %s\n", cmd);
    
    if (system(cmd) != 0) {
        vty_out(vty, "%% Error: Failed to delete IP from system.\n");
        return CMD_WARNING;
    }

    return CMD_SUCCESS;
}

DEFUN (show_ip_address,
       show_ip_address_cmd,
       "show ip address",
       "Show running system information\n"
       "Internet Protocol (IP)\n"
       "IP address information\n")
{
    vty_out(vty, "%-10s %-14s %-20s %-20s\n",
            "Interface", "PhysicalState", "IPv4 Address", "IPv6 Address");
    vty_out(vty, "----------------------------------------------------------------------\n");
    char *shell_cmd = "ip -o addr show | awk '"
                      "{split($4, a, \"/\"); "
                      "if($3==\"inet\") {printf \"%-10s %-14s %-20s %-20s\\n\", $2, \"up\", $4, \"--\"} "
                      "else if($3==\"inet6\") {printf \"%-10s %-14s %-20s %-20s\\n\", $2, \"up\", \"--\", $4}}'";
    FILE *fp = popen(shell_cmd, "r");
    if (fp) {
        char buf[256];
        while (fgets(buf, sizeof(buf), fp)) {
            vty_out(vty, "%s", buf);
        }
        pclose(fp);
    }

    return CMD_SUCCESS;
}

DEFUN(show_arp,
	  show_arp_cmd,
	  "show arp",
	  SHOW_STR
	  "ARP table\n")
{
	const char *shell_cmd =
		"ip neigh show | awk 'BEGIN {"
		"printf \"%-24s %-7s %-17s %-5s %-9s %s\\n\", "
		"\"Address\", \"HWtype\", \"HWaddress\", \"Flags\", \"Mask\", \"Iface\"}"
		"{"
		"if ($0 ~ /FAILED|INCOMPLETE/ || NF < 3) next;"
		"dev = \"\"; mac = \"\";"
		"for (i = 1; i <= NF; i++) {"
		"if ($i == \"dev\" && (i + 1) <= NF) dev = $(i + 1);"
		"if ($i == \"lladdr\" && (i + 1) <= NF) mac = $(i + 1);"
		"}"
		"if (mac == \"\") next;"
		"printf \"%-24s %-7s %-17s %-5s %-9s %s\\n\", "
		"$1, \"ether\", mac, \"C\", \"\", dev"
		"}'";
	FILE *fp = popen(shell_cmd, "r");
	if (!fp) {
		vty_out(vty, "%% Error: Failed to read ARP table.\n");
		return CMD_WARNING;
	}

	char buf[256];
	while (fgets(buf, sizeof(buf), fp)) {
		vty_out(vty, "%s", buf);
	}
	pclose(fp);

	return CMD_SUCCESS;
}

DEFUN(clear_arp_cache,
	  clear_arp_cache_cmd,
	  "clear arp-cache",
	  "Clear information\n"
	  "ARP cache\n")
{
	if (system("ip neigh flush all > /dev/null 2>&1") != 0) {
		vty_out(vty, "%% Error: Failed to clear ARP cache.\n");
		return CMD_WARNING;
	}

	vty_out(vty, "ARP cache cleared.\n");
	return CMD_SUCCESS;
}

DEFUN (show_route_static, show_route_static_cmd,
       "show route static",
       SHOW_STR
       "Routing table information\n"
       "Static routes\n")
{
	execute_command("route", 1, "-n", NULL);
	return CMD_SUCCESS;
}

DEFPY (vtysh_ip_route_static,
       vtysh_ip_route_static_cmd,
       "ip route-static A.B.C.D$dest {A.B.C.D$mask|(0-32)$mask_len} A.B.C.D$next [(1-255)$pref]",
       "IP information\n"
       "Establish static routes\n"
       "Destination IP address\n"
       "IP netmask\n"
       "IP mask length\n"
       "Next hop IP address\n"
       "Preference/Metric value (default 1)\n")
{
	char cmd_buffer[256];
    long metric = (pref_str) ? pref : 1;

    if (mask_str) {
        snprintf(cmd_buffer, sizeof(cmd_buffer),
                 "ip route add %s/%s via %s metric %ld",
                 dest_str, mask_str, next_str, metric);
    } else {
        snprintf(cmd_buffer, sizeof(cmd_buffer),
                 "ip route add %s/%ld via %s metric %ld",
                 dest_str, mask_len, next_str, metric);
    }

    execute_command("sh", 2, "-c", cmd_buffer);

    return CMD_SUCCESS;
}

DEFPY (no_vtysh_ip_route_static,
       no_vtysh_ip_route_static_cmd,
       "no ip route-static A.B.C.D$dest {A.B.C.D$mask|(0-32)$mask_len} A.B.C.D$next",
       NO_STR
       "IP information\n"
       "Establish static routes\n"
       "Destination IP address\n"
       "IP netmask\n"
       "IP mask length\n"
       "Next hop IP address\n")
{
	char cmd_buffer[256];
    if (mask_str) {
        snprintf(cmd_buffer, sizeof(cmd_buffer),
                 "ip route del %s/%s via %s",
                 dest_str, mask_str, next_str);
    } else {
        snprintf(cmd_buffer, sizeof(cmd_buffer),
                 "ip route del %s/%ld via %s",
                 dest_str, mask_len, next_str);
    }
    execute_command("sh", 2, "-c", cmd_buffer);
    return CMD_SUCCESS;
}


DEFPY (configuration_reauthentication,
       configuration_reauthentication_cmd,
       "configuration reauthentication {enable|disable}$token",
       "Configuration\n"
       "Reauthentication\n"
       "Enable\n"
       "Disable\n")
{
	if(!strncmp(token, "enable", sizeof("enable")-1)){	
		vty_out(vty, "Reauthentication turn on!\n");
		g_configuration_reauthentication = 1;
	}
	else{	
		vty_out(vty, "Reauthentication turn off!\n");
		g_configuration_reauthentication = 0;
	}

	return CMD_SUCCESS;
}

DEFUN (vtysh_reboot,
       vtysh_reboot_cmd,
       "reboot",
       "Reboot machine\n")
{
	if(!need_reauthentication(vty)){
		int retval;

		do{
			if(need_confirm("System configuration has been modified. Save? [y]/n\n", 1)){
				break; 	
			}

			vty_out(vty, "Building configuration..\n");
			retval = save_configuration();
			vty_out(vty, "Saving configuration is finished\n");
			
			if (retval == 0){
				vty_out(vty, "Note: Save Success\n");
			}
			else{
				vty_out(vty, "Note: Save Error %d\n", retval);
			}
		}while(0);
		
		if(need_confirm("System reboot, are you sure? y/[n]\n", 0)){
			return CMD_SUCCESS; 	
		}
		
		vty_out(vty, "The system is going down NOW!\n");
		system("reboot -f");
	}
	return CMD_SUCCESS;
}

DEFUN (vtysh_poweroff,
       vtysh_poweroff_cmd,
       "poweroff",
       "Poweroff machine\n")
{
	if(!need_reauthentication(vty)){
		int retval;

		do{
			if(need_confirm("System configuration has been modified. Save? [y]/n\n", 1)){
				break; 	
			}

			vty_out(vty, "Building configuration..\n");
			retval = save_configuration();
			vty_out(vty, "Saving configuration is finished\n");
			
			if (retval == 0){
				vty_out(vty, "Note: Save Success\n");
			}
			else{
				vty_out(vty, "Note: Save Error %d\n", retval);
			}
		}while(0);
		
		if(need_confirm("System reboot, are you sure? y/[n]\n", 0)){
			return CMD_SUCCESS; 	
		}
	
		vty_out(vty, "The system is going down NOW!\n");
		system("poweroff -f");
	}
	return CMD_SUCCESS;
}

DEFUN (maintenance_mode,
       maintenance_mode_cmd,
       "maintenance-mode",
       "Maintenance mode\n")
{
    char password[1024];
    int seed_len = sizeof(g_console_password_seed);
    memset(password, 0, sizeof(password));
    
    get_console_password(g_hw_sn, g_console_password_seed, &seed_len);
    //vty_out(vty, "\ng_hw_sn=%s\ng_console_password_seed=%s\nPlease enter your login password to re-authenticate：\n", g_hw_sn,g_console_password_seed);
    {
        size_t seed_len_local = (size_t)seed_len;
        size_t base = seed_len_local / 4;
        size_t rem = seed_len_local % 4;
        size_t offset = 0;
        size_t i = 0;

        vty_out(vty, "Seed:\n");
        for (i = 0; i < 4; i++) {
            size_t part = base + (i < rem ? 1 : 0);
            if (part == 0) {
                vty_out(vty, "\n");
            } else {
                vty_out(vty, "        %.*s\n", (int)part, g_console_password_seed + offset);
                offset += part;
            }
        }
        {
            size_t seed_len_local = (size_t)seed_len;
            size_t b64_len = ((seed_len_local + 2) / 3) * 4;
            char *b64 = (char *)alloca(b64_len);
            if (b64) {
                size_t out = b64_encode((const unsigned char *)g_console_password_seed, seed_len_local, b64);
                vty_out(vty, "Seed(QR):\n");
                if (!render_qr_external(vty, b64, out)) {
                    vty_out(vty, "        qrencode not found or failed\n");
                }
            }
        }
    }

	if (is_console_display())
		password_prompt("Please enter your login password(S) to re-authenticate:", password, sizeof(password)-1);
	else
    	password_prompt("Please enter your login password to re-authenticate:", password, sizeof(password)-1);

    {
        int check_len = is_console_display() ? 6 : 32;
	    if (0==strncmp(g_hw_sn, password, check_len)){		
		vty_out(vty, "Login successful, enter the root terminal!\n");
		exit(0);
	    }
    }
	vty_out(vty, "Login failed, password incorrect!\n");

	return CMD_SUCCESS;
}

static int hy_utf8_decode_one(const unsigned char *s, uint32_t *cp, int *len)
{
    if (!s || !cp || !len)
        return 0;

    if (s[0] < 0x80) {
        *cp = s[0];
        *len = 1;
        return 1;
    }

    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(s[0] & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
        *len = 2;
        return 1;
    }

    if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(s[0] & 0x0F) << 12) |
              ((uint32_t)(s[1] & 0x3F) << 6) |
              (uint32_t)(s[2] & 0x3F);
        *len = 3;
        return 1;
    }

    if ((s[0] & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 &&
        (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(s[0] & 0x07) << 18) |
              ((uint32_t)(s[1] & 0x3F) << 12) |
              ((uint32_t)(s[2] & 0x3F) << 6) |
              (uint32_t)(s[3] & 0x3F);
        *len = 4;
        return 1;
    }

    return 0;
}

static int hy_is_cjk_unified(uint32_t cp)
{
    return ((cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF));
}

static int hy_validate_web_admin_username(const char *username)
{
    const unsigned char *p = (const unsigned char *)username;
    int ch_count = 0;

    if (!username || !username[0])
        return 0;

    if (strncasecmp(username, "NMC_", 4) == 0)
        return 0;

    while (*p) {
        uint32_t cp = 0;
        int len = 0;
        if (!hy_utf8_decode_one(p, &cp, &len))
            return 0;

        if (len == 1) {
        	if (!(isalnum((unsigned char)cp) || cp == '_' || cp == '-' || cp == '.'
				  || cp == '!' || cp == '@' || cp == '#' || cp == '^' || cp == '*'))
                return 0;
        } else {
            if (!hy_is_cjk_unified(cp))
                return 0;
        }

        p += len;
        ch_count++;
    }

    return (ch_count >= 1 && ch_count <= 31);
}

static int hy_validate_web_admin_password(const char *password)
{
    const char *special = "~!@#$%^&*-+,.;|><";
    int len = 0;
    int has_digit = 0, has_lower = 0, has_upper = 0, has_special = 0;
    int category_count = 0;
    const unsigned char *p = (const unsigned char *)password;

    if (!password || !password[0])
        return 0;

    while (*p) {
        unsigned char c = *p++;
        len++;

        if (c < 0x21 || c > 0x7E)
            return 0;

        if (isdigit(c))
            has_digit = 1;
        else if (islower(c))
            has_lower = 1;
        else if (isupper(c))
            has_upper = 1;
        else if (strchr(special, c))
            has_special = 1;
        else
            return 0;
    }

    if (len < 8 || len > 15)
        return 0;

    category_count = has_digit + has_lower + has_upper + has_special;
    return (category_count >= 3);
}

static int hy_save_web_admin_account(const char *username, const char *password)
{
	char cmdBuf[4096] = {0};
	const char *php_code =
		"$f=\"/usr/local/lighttpd/user.conf\";"
		"$u=$argv[1];$p=$argv[2];$pwd=md5($p);$data=array();"
		"if(file_exists($f)){ $c=file_get_contents($f); $j=json_decode($c,true); if(is_array($j)) $data=$j; }"
		"$found=false;"
		"foreach($data as &$it){ if(isset($it[\"name\"]) && $it[\"name\"]===$u){ $it[\"password\"]=$pwd; $found=true; break; }}"
		"unset($it);"
		"if(!$found){ $data[]=array(\"pre_define\"=>0,\"auth_method\"=>1,\"role\"=>\"admin\",\"name\"=>$u,\"password\"=>$pwd,\"lastpwdtime\"=>time(),\"status\"=>1); }"
		"file_put_contents($f,json_encode($data, JSON_UNESCAPED_UNICODE));"
		"@system(\"cp -af /usr/local/lighttpd/user.conf /home/config/current/ >/dev/null 2>&1\");";
	int retval = 0;
	snprintf(cmdBuf, sizeof(cmdBuf),
		"/usr/local/php/bin/php -r '%s' -- %s %s > /dev/null 2>&1",
		php_code, hylab_escape_shell_arg(username), hylab_escape_shell_arg(password));
	retval = system(cmdBuf);
	if (retval == 0)
		return 0;

	snprintf(cmdBuf, sizeof(cmdBuf),
		 "php -r '%s' -- %s %s > /dev/null 2>&1",
		 php_code, hylab_escape_shell_arg(username), hylab_escape_shell_arg(password));
	retval = system(cmdBuf);
	return retval == 0 ? 0 : -1;
}

static int hy_validate_web_admin_role(const char *role)
{
	if (!role)
		return 0;

	return (strcasecmp(role, "admin") == 0 ||
			strcasecmp(role, "reporter") == 0 ||
			strcasecmp(role, "guest") == 0);
}

static int hy_save_web_admin_role(const char *username, const char *role)
{
	char cmdBuf[4096] = {0};
	const char *php_code =
		"$f=\"/usr/local/lighttpd/user.conf\";"
		"$u=$argv[1];$r=$argv[2];$data=array();"
		"if(file_exists($f)){ $c=file_get_contents($f); $j=json_decode($c,true); if(is_array($j)) $data=$j; }"
		"$found=false;"
		"foreach($data as &$it){ if(isset($it[\"name\"]) && $it[\"name\"]===$u){ $it[\"role\"]=$r; $found=true; break; }}"
		"unset($it);"
		"if(!$found){ exit(2); }"
		"file_put_contents($f,json_encode($data, JSON_UNESCAPED_UNICODE));"
		"@system(\"cp -af /usr/local/lighttpd/user.conf /home/config/current/ >/dev/null 2>&1\");";
	int retval = 0;
	snprintf(cmdBuf, sizeof(cmdBuf),
		"/usr/local/php/bin/php -r '%s' -- %s %s > /dev/null 2>&1",
		php_code, hylab_escape_shell_arg(username), hylab_escape_shell_arg(role));

	retval = system(cmdBuf);
	if (retval == 0)
		return 0;

	snprintf(cmdBuf, sizeof(cmdBuf),
			 "php -r '%s' -- %s %s > /dev/null 2>&1",
			 php_code, hylab_escape_shell_arg(username), hylab_escape_shell_arg(role));
	retval = system(cmdBuf);
	return retval == 0 ? 0 : -1;
}

DEFUN (username_password,
       username_password_cmd,
       "username WORD password LINE",
	  "Web local administrator configuration\n"
	  "Administrator username\n"
	  "Set administrator password\n"
	  "Administrator password\n")
{
	const char *username = NULL;
	const char *password = NULL;

	if (argc >= 4) {
		username = argv[1]->arg ? argv[1]->arg : (const char *)argv[1];
		password = argv[3]->arg ? argv[3]->arg : (const char *)argv[3];
	}

	if (!username || !password) {
		vty_out(vty, "%% Error: Missing username/password.\n");
		return CMD_WARNING;
	}

	if (!hy_validate_web_admin_username(username)) {
		vty_out(vty, "%% Error: Invalid username. Allowed: 1-31 chars, letters/digits/_/./-/!/@/#/^/*, Chinese, and must not start with NMC_.\n");
		return CMD_WARNING;
	}

	if (!hy_validate_web_admin_password(password)) {
		vty_out(vty, "%% Error: Invalid password. Length 8-15, include at least 3 classes (digit/lower/upper/special), no Chinese/space/full-width.\n");
		return CMD_WARNING;
	}

	if (hy_save_web_admin_account(username, password) != 0) {
		vty_out(vty, "%% Error: Failed to save web local administrator account.\n");
		return CMD_WARNING;
	}

	vty_out(vty, "Web local administrator account configured successfully.\n");
	return CMD_SUCCESS;
}

DEFUN (username_role,
	   username_role_cmd,
	   "username WORD role <admin|reporter|guest>",
	   "Web local administrator configuration\n"
	   "Administrator username\n"
	   "Set administrator role\n"
	   "Administrator role\n"
	   "Admin role\n"
	   "Reporter role\n"
	   "Guest role\n")
{
	const char *username = NULL;
	const char *role = NULL;

	if (argc >= 4) {
		username = argv[1]->arg ? argv[1]->arg : (const char *)argv[1];
		role = argv[3]->arg ? argv[3]->arg : (const char *)argv[3];
	}

	if (!username || !role) {
		vty_out(vty, "%% Error: Missing username/role.\n");
		return CMD_WARNING;
	}

	if (!hy_validate_web_admin_username(username)) {
		vty_out(vty, "%% Error: Invalid username. Allowed: 1-31 chars, letters/digits/_/./-/!/@/#/^/*, Chinese, and must not start with NMC_.\n");
		return CMD_WARNING;
	}

	if (!hy_validate_web_admin_role(role)) {
		vty_out(vty, "%% Error: Invalid role. Supported roles: admin, reporter, guest.\n");
		return CMD_WARNING;
	}

	if (hy_save_web_admin_role(username, role) != 0) {
		vty_out(vty, "%% Error: Failed to save web local administrator role.\n");
		return CMD_WARNING;
	}

	vty_out(vty, "Web local administrator role configured successfully.\n");
	return CMD_SUCCESS;
}

DEFUN (vtysh_save,
       vtysh_save_cmd,
       "save",
       "Save Configuration\n")
{
	int retval;

	if(need_confirm("Save Configuration. Note:Press [y]/n Then Press Enter To Confirm\n", 1)){
		return CMD_SUCCESS;	
	}

	if ((retval = save_configuration()) == 0){
		vty_out(vty, "Note: Save Success\n");
	}
	else{
		vty_out(vty, "Note: Save Error %d\n", retval);
	}

	return CMD_SUCCESS;
}

DEFUN (vtysh_backup,
       vtysh_backup_cmd,
       "backup configuration",
       "Backup configuration\n"
       "Configuration\n")
{
	if(!need_reauthentication(vty)){

		if(need_confirm("Backup Configuration. Note:Press [y]/n Then Press Enter To Confirm\n", 1)){
			return CMD_SUCCESS; 
		}

		vty_out(vty, "Backuping configuration..\n");

		save_configuration();

		backup_configuration();

		system("sz /home/checkout/Config.conf");
	}
	return CMD_SUCCESS;
}

DEFUN (vtysh_restore,
       vtysh_restore_cmd,
       "restore configuration",
       "Restore configuration\n"
       "Configuration\n")
{
	if(!need_reauthentication(vty)){
		int retval;	
		char filename[512] = {0};
		
		system("rm -rf /home/checkin/*;cd /home/checkin/;rz -y;");
		find_first_file(vty, "/home/checkin", filename, sizeof(filename));

		if(!filename[0]){
			vty_out(vty, "File error. Please upload Config.conf...\n");			
			return CMD_SUCCESS;
		}
			
		do{
			if(need_confirm("Restore Configuration. Note:Press [y]/n Then Press Enter To Confirm\n", 1)){
				break; 
			}

			vty_out(vty, "Restore configuration now ...\n\n");
			retval = restore_configuration(filename);
			vty_out(vty, "Restore configuration is finished\n");
			
			if (retval >= 0){
				vty_out(vty, "Note: Restore Success\n");
			}
			else{
				vty_out(vty, "Note: Restore Error %d\n", retval);
				break;
			}

			if(need_confirm("System reboot, are you sure? y/[n]\n", 0)){
				return CMD_SUCCESS; 	
			}
		
			vty_out(vty, "The system is going down NOW!\n");
			system("reboot -f");			
		}while(0);	
	}
	
	return CMD_SUCCESS;
}

DEFUN (vtysh_upgrade_system,
       vtysh_upgrade_system_cmd,
       "upgrade system",
       "Upgrade Firmware\n"
       "Upgrade Firmware\n")
{
	if(!need_reauthentication(vty)){
		int retval;	
		char filename[512] = {0};;
		
		system("mkdir -p /mnt/uploadsystem; rm -rf /mnt/uploadsystem/*;cd /mnt/uploadsystem/;rz -y;");
		find_first_file(vty, "/mnt/uploadsystem", filename, sizeof(filename));

		if(!filename[0] || !strstr(filename, ".bin")){
			vty_out(vty, "File error. Please upload xxx.bin...\n");
			return CMD_SUCCESS;
		}
		
		do{
			if(need_confirm("Upgrade system. Note:Press [y]/n Then Press Enter To Confirm\n", 1)){
				break; 
			}

			vty_out(vty, "System software update..\n\n");
			retval = upgrade_system(filename);
			vty_out(vty, "The system software update is complete\n");
			
			if (retval == 0){
				vty_out(vty, "Note: Upgrade Success\n");
			}
			else{
				vty_out(vty, "Note: Upgrade Error %d\n", retval);
				break;
			}

			if(!need_confirm("System configuration has been modified. Save? [y]/n\n", 1)){
				if ((retval = save_configuration()) == 0){
					vty_out(vty, "Note: Save Success\n");
				}
				else{
					vty_out(vty, "Note: Save Error %d\n", retval);
				}	
			}
			
			if(need_confirm("System reboot, are you sure? y/[n]\n", 0)){
				return CMD_SUCCESS; 	
			}
		
			vty_out(vty, "The system is going down NOW!\n");
			system("reboot -f");			
		}while(0);	
	}
	
	return CMD_SUCCESS;
}

DEFUN (vtysh_rollback_system,
       vtysh_rollback_system_cmd,
       "rollback system",
       "Rollback system\n"
       "Rollback system\n")
{
	if(!need_reauthentication(vty)){
		int retval;	

		do{
			if(need_confirm("Rollback system. Note:Press [y]/n Then Press Enter To Confirm\n", 1)){
				break; 
			}

			vty_out(vty, "System software rollback...\n\n");
			retval = rollback_system();
			vty_out(vty, "The rollback system version is complete\n");
			
			if (retval == 0){
				vty_out(vty, "Note: Rollback Success\n");
			}
			else{
				vty_out(vty, "Note: Rollback Error %d\n", retval);
				break;
			}

			if(!need_confirm("System configuration has been modified. Save? [y]/n\n", 1)){
				if ((retval = save_configuration()) == 0){
					vty_out(vty, "Note: Save Success\n");
				}
				else{
					vty_out(vty, "Note: Save Error %d\n", retval);
				}	
			}
			
			if(need_confirm("System reboot, are you sure? y/[n]\n", 0)){
				return CMD_SUCCESS; 	
			}
			
			vty_out(vty, "The system is going down NOW!\n");
			system("reboot -f");			
		}while(0);	
	}
	
	return CMD_SUCCESS;
}

DEFPY (update_force_rule,
       update_force_rule_cmd,
       "update force {applib|urllib|avlib|ipslib|waflib|geoip-lib|ssl-vpn-client|security-client}$token",
       "Update libs & clients\n"
       "Force update\n"
       "AIS Database\n"
       "URL Database\n"
       "Virus Database\n"
       "IPS Database\n"
       "SSL-VPN client\n"
       "WAF Database\n"
       "UBoundary Database\n"
       "Security client\n")
{
	char filename[1024] = {0};
	int retval = 1;

	if(need_reauthentication(vty)){	
		return CMD_SUCCESS;
	}

	if(!strncmp(token, "applib", sizeof("applib")-1)){	
		if(ace_prepare_update(vty, "app", filename, sizeof(filename), "FEATURE")){
			return CMD_SUCCESS;
		}
		
		vty_out(vty, "Upgrading.....\n");
		retval = ace_AIS_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "urllib", sizeof("urllib")-1)){	
		if(ace_prepare_update(vty, "url", filename, sizeof(filename), "URL")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_URL_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "avlib", sizeof("avlib")-1)){	
		if(ace_prepare_update(vty, "av", filename, sizeof(filename), "AV")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_AV_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "ipslib", sizeof("ipslib")-1)){	
		if(ace_prepare_update(vty, "ips", filename, sizeof(filename), "IPS")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_IPS_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "ssl-vpn-client", sizeof("ssl-vpn-client")-1)){	
		if(ace_prepare_update(vty, "sa", filename, sizeof(filename), "sslvpn")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_SA_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "waflib", sizeof("waflib")-1)){	
		if(ace_prepare_update(vty, "waf", filename, sizeof(filename), "WAF")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_WAF_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "geoip-lib", sizeof("geoip-lib")-1)){	
		if(ace_prepare_update(vty, "geoip", filename, sizeof(filename), "GEOIP")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_GEOIP_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}
	else if(!strncmp(token, "security-client", sizeof("security-client")-1)){	
		if(ace_prepare_update(vty, "scv", filename, sizeof(filename), "IMS")){
			return CMD_SUCCESS;
		}

		vty_out(vty, "Upgrading.....\n");
		retval = ace_SCV_DB_update(filename);
		vty_out(vty, "The upgrade is complete\n");
	}

	if (retval == 0){
		vty_out(vty, "Note: Upgrade Success\n");
	}
	else{
		vty_out(vty, "Note: Upgrade Error %d\n", retval);
	}

	return CMD_SUCCESS;
}

DEFPY (update_online_rule,
       update_online_rule_cmd,
       "update online {applib|urllib|avlib|ipslib|waflib|geoip-lib|security-client|safe-patch}$token",
       "Update libs & clients\n"
       "Online update\n"
       "AIS Database\n"
       "URL Database\n"
       "Virus Database\n"
       "IPS Database\n"
       "WAF Database\n"
       "UBoundary Database\n"
       "Security client\n"
       "System safe patch\n")
{
	char filename[1024] = {0};
	int retval = 1;

	if(need_reauthentication(vty)){	
		return CMD_SUCCESS;
	}

	vty_out(vty, "Upgrading.....\n");
	retval = ace_online_update(token, 1);
	vty_out(vty, "The upgrade is complete\n");

	return CMD_SUCCESS;
}

DEFUN(debug_trace_download,
      debug_trace_download_cmd,
      "trace-download",
	  "Download trace sum modoule file\n")
{
	//execute_command("sz", 1, "/tmp/trace.txt", NULL);
	system("sz /tmp/trace.txt");
    return CMD_SUCCESS;
}

static pid_t tcpdump_pid = -1;
static int check_vpp_interface_exist(const char *interface_name)
{
    FILE *fp;
    char cmd[256];
    char buf[1024];
    int found = 0;

    // 构建vppctl命令
    snprintf(cmd, sizeof(cmd), "vppctl show interface %s 2>/dev/null", interface_name);
    
    fp = popen(cmd, "r");
    if (!fp) {
        return -1;  // 命令执行失败
    }

    // 检查输出中是否包含接口信息
    while (fgets(buf, sizeof(buf), fp)) {
        if (0==strncmp(buf, interface_name, strlen(interface_name))) {
            found = 1;
            break;
        }
    }

    pclose(fp);
    return found;
}

int check_physical_interface(const char *interface_name)
{
    struct ifreq ifr;
    int sock;
    
    if (!interface_name) {
        return -1;  // 参数错误
    }
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -2;  // 创建socket失败
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ-1);

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        close(sock);
        return 0;   // 接口不存在
    }

    close(sock);
    return 1;       // 接口存在
}


#define VTY_NEWLINE "\n"
#define LOCK_FILE "/var/lock/tcpdump.lock"



static int acquire_lock(void) {
    int fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (fd == -1) return -1;
    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {  
        close(fd);
        return -1;  
    }
    return fd; 
}

static void release_lock(int fd) {
    if (fd != -1) {
        flock(fd, LOCK_UN);
        close(fd);
    }
}


DEFUN(debug_tcpdump_start,
      debug_tcpdump_start_cmd,
      "tcpdump <file|print|download> LINE...",
      "Start tcpdump capture\n"
	  "Save packet to file\n"
	"Print packet to console\n"
	"Download packet\n"
	"other options eg: -s 1500 ...\n")
{
    int pipefd[2];
    char buf[512];
    ssize_t n;
    int running = 1;
	int param_num = argc-2;
	int i = 0;
#define MAX_ARGV 20
	const char *p[MAX_ARGV];
	char inf[32]={0};
	char span_inf[]="tcpdump1";
	int inf_index=0;
	char cmd[256]={0};
	int dpdk_port=0;
	int lock_fd =-1;

	lock_fd = acquire_lock();
    if (lock_fd == -1) {
        vty_out(vty, "Error: Another tcpdump session is running. Try again later.%s", VTY_NEWLINE);
        return CMD_WARNING;
    }
	
	if (0 == strcmp("download", argv[1]->arg))
	{
		execute_command("sz", 1, "/mnt/tcpdump.pcap", NULL);
		release_lock(lock_fd);
		return CMD_SUCCESS;
	}

	for(int i = 0; i < MAX_ARGV; i++) {
		p[i] = NULL;
	}

	if ( argc > 14 )
	{
		vty_out(vty, "Too many parameters!%s", VTY_NEWLINE);
		release_lock(lock_fd);
		return CMD_SUCCESS;
	}

	p[0] = "tcpdump";
	for ( i = 0; i<param_num;i++)
	{
		if ( 0 == strcmp("-w",argv[i+2]->arg))
		{
			vty_out(vty, "Parameter -w can't be used!%s", VTY_NEWLINE);
			release_lock(lock_fd);
			return CMD_SUCCESS;
		}

		if ( 0 == strcmp("-i",argv[i+2]->arg))
		{
			if (i == param_num-1)
			{
				vty_out(vty, "Parameter -i error!%s", VTY_NEWLINE);
				release_lock(lock_fd);
				return CMD_SUCCESS;
			}

			strncpy(inf,argv[i+3]->arg,31);
			inf_index = i+2;	
		}
		
		p[i+1] = argv[i+2]->arg;
	}

	if ( !inf[0])
	{
		vty_out(vty, "Interface name not specified!%s", VTY_NEWLINE);
		release_lock(lock_fd);
		return CMD_SUCCESS;
	}

	if (!check_vpp_interface_exist(inf))
	{
		if (!check_physical_interface(inf))
		{
			vty_out(vty, "Interface %s doesn't exist!%s", inf,VTY_NEWLINE);
			release_lock(lock_fd);
			return CMD_SUCCESS;
		}
	}
	else
	{
		dpdk_port = 1;
	}

	if (0 == strcmp("file", argv[1]->arg))
	{
		p[param_num+1] = "-w";
		p[param_num+2] = "/mnt/tcpdump.pcap";
	}
	else if (0 == strcmp("print", argv[1]->arg))
	{
		p[param_num+1] = "-l";
	}
	else
	{
		return CMD_SUCCESS;
	}

	if ( dpdk_port )
	{
		p[inf_index]= span_inf;
		snprintf(cmd,255,"ip link add name tcpdump1 type veth peer name tcpdump2");
		system(cmd);
		snprintf(cmd,255,"ip link set tcpdump1 up");
		system(cmd);
		snprintf(cmd,255,"ip link set tcpdump2 up");
		system(cmd);
		snprintf(cmd,255,"vppctl create host-interface name tcpdump2");
		system(cmd);
		snprintf(cmd,255,"vppctl set int state host-tcpdump2 up");
		system(cmd);
		snprintf(cmd,255,"vppctl set interface span %s destination host-tcpdump2 both",inf);
		system(cmd);
		sleep(1);
	}
	
	
    if (pipe(pipefd) == -1) {
        vty_out(vty, "Failed to create pipe%s", VTY_NEWLINE);
		release_lock(lock_fd);
		return CMD_SUCCESS;
    }

    tcpdump_pid = fork();
    if (tcpdump_pid == -1) {
        vty_out(vty, "Failed to fork process%s", VTY_NEWLINE);
        close(pipefd[0]);
        close(pipefd[1]);
		release_lock(lock_fd);
		return CMD_SUCCESS;
    }


    if (tcpdump_pid == 0) {
        // 子进程
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        //execl("/usr/sbin/tcpdump", "tcpdump", "-i", "MGT", "-l", NULL);
		//for (i = 0; p[i] != NULL; i++) {
		//	vty_out(vty, "p[%d] = %s%s", i, p[i], VTY_NEWLINE);
		//}
		execv("/usr/sbin/tcpdump", p);
		exit(1);
    }

    // 父进程
    close(pipefd[1]);
    
    // 设置非阻塞读取
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    //fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    vty_out(vty, "Starting tcpdump on %s (Press Enter to exit)%s", 
            inf, VTY_NEWLINE);

    while (running) {
        // 检查输入
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n') {
                running = 0;
                continue;
            }
        }

        // 读取tcpdump输出
        n = read(pipefd[0], buf, sizeof(buf));
        if (n > 0) {
            vty_out(vty, "%s",buf);
        } else if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(100000);  // 100ms
                continue;
            }
            break;
        }
    }

    // 清理子进程
    if (tcpdump_pid > 0) {
        kill(tcpdump_pid, SIGTERM);
        waitpid(tcpdump_pid, NULL, 0);
        tcpdump_pid = -1;
    }

    close(pipefd[0]);

	if (tcpdump_pid > 0) {
        kill(tcpdump_pid, SIGTERM);
        waitpid(tcpdump_pid, NULL, 0);
    }
	
	if ( dpdk_port )
	{
		snprintf(cmd,255,"vppctl set interface span %s disable",inf);
		system(cmd);
		snprintf(cmd,255,"vppctl set int state host-tcpdump2 down");
		system(cmd);
		snprintf(cmd,255,"vppctl delete host-interface name tcpdump2");
		system(cmd);
		snprintf(cmd,255,"ip link set tcpdump2 down");
		system(cmd);
		snprintf(cmd,255,"ip link set tcpdump1 down");
		system(cmd);
		snprintf(cmd,255,"ip link del tcpdump1");
		system(cmd);
		sleep(1);
	}
	
	release_lock(lock_fd);
    return CMD_SUCCESS;
}

DEFUN(debug_tcpdump_download,
      debug_tcpdump_download_cmd,
      "tcpdump download",
      "Start tcpdump capture\n"
	  "Download tcpdump file\n")
{
	system("sz /mnt/tcpdump.pcap");
	//execute_command("sz", 1, "/mnt/tcpdump.pcap", NULL);
	return CMD_SUCCESS;
}

DEFUN(debug_vtysh_ping,
      debug_vtysh_ping_cmd,
      "ping LINE...",
      "Ping\n"
	"Options {dest-ip-Address}  [ -c count |  -I { interface-type number } | source-ip-address } | -s size | -t ttl | -W timeout |-w deadline | -i interval | -4 | -6| -D | -R] ...\n")
{
	int status;
	int i = 0;
#define MAX_ARGV 20
	const char *p[MAX_ARGV];
	pid_t ping_pid = -1;

	for(int i = 0; i < MAX_ARGV; i++) {
		p[i] = NULL;
	}

	if ( argc > 14 )
	{
		vty_out(vty, "Too many parameters!%s", VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	p[0] = "ping";
	for ( i = 1; i<argc;i++)
	{	
		p[i] = argv[i]->arg;		
	}

    ping_pid = fork();
    if (ping_pid == -1) {
        vty_out(vty, "Failed to fork process%s", VTY_NEWLINE);
		return CMD_SUCCESS;
    }

    if (ping_pid == 0) {
        // 子进程
		execv("/bin/ping", p);
		exit(1);
    }

    // 清理子进程
    if (ping_pid > 0) {
		execute_flag = 1;
		wait4(ping_pid, &status, 0, NULL);
		execute_flag = 0;
        ping_pid = -1;		
    }

    return CMD_SUCCESS;
}

DEFUN(debug_vtysh_traceroute,
      debug_vtysh_traceroute_cmd,
      "traceroute LINE...",
      "Traceroute\n"
	"Options  {host} [packetlen| -s source-ip-address| -I | -T | -U |-p port |-4 |-6 |-q nqueries | -w waittime | -f first_ttl | -m max_ttl |-t tos-value ] ...\n")
{
	int status;
	int i = 0;
#define MAX_ARGV 20
	const char *p[MAX_ARGV];
	pid_t pid = -1;

	for(int i = 0; i < MAX_ARGV; i++) {
		p[i] = NULL;
	}

	if ( argc > 14 )
	{
		vty_out(vty, "Too many parameters!%s", VTY_NEWLINE);
		return CMD_SUCCESS;
	}

	p[0] = "traceroute";
	for ( i = 1; i<argc;i++)
	{	
		p[i] = argv[i]->arg;		
	}

    pid = fork();
    if (pid == -1) {
        vty_out(vty, "Failed to fork process%s", VTY_NEWLINE);
		return CMD_SUCCESS;
    }

    if (pid == 0) {
        // 子进程
		execv("/usr/bin/traceroute", p);
		exit(1);
    }

    // 清理子进程
    if (pid > 0) {
		execute_flag = 1;
		wait4(pid, &status, 0, NULL);
		execute_flag = 0;
        pid = -1;		
    }

    return CMD_SUCCESS;
}

static int parse_ip_mask(const char *input, char *ip_str, int *mask)
{
    char temp[64] = {0};
    char *p = NULL;
    
    if (!input || !ip_str || !mask) {
        return -1;
    }
    
    strncpy(temp, input, sizeof(temp) - 1);
    
    // 查找'/'分隔符
    p = strchr(temp, '/');
    if (!p) {
        return -1;
    }
    
    // 分割IP和掩码
    *p = '\0';
    p++;
    
    // 获取IP地址字符串
    strncpy(ip_str, temp, 63);
    
    // 获取掩码值
    *mask = atoi(p);
    if (*mask < 0 || *mask > 32) {
        return -1;
    }
    
    return 0;
}


DEFPY(debug_pcappacket_start,
      debug_pcappacket_all_cmd,
      "pcappacket INTERFACE [ether-type ETYPE$etype|arp] [vlan (1-4095)$vlanid]  [ip1 A.B.C.D$ip1|ip1 A.B.C.D/M$ip1_subnet| ip1 X:X::X:X$ip1v6 | ip1 X:X::X:X/M$ip1v6_subnet ] [ip2 A.B.C.D$ip2|ip2 A.B.C.D/M$ip2_subnet| ip2 X:X::X:X$ip2v6 | ip2 X:X::X:X/M$ip2v6_subnet ] [ipv4-proto (1-255)$ipv4proto | ipv6-proto (1-255)$ipv6proto | icmp | icmpv6 | tcp [port1 (1-65535)$port1] [port2 (1-65535)$port2] | udp [port1 (1-65535)$u_port1] [port2 (1-65535)$u_port2]] [count (1-10000)$count]",
      "Capture special packets to file\n"
	  "Interface to capture\n"
	  "Ethernet Frame Type\n"
	  "Ethernet Frame Type, eg. 0x0806 0x0800\n"
	  "Arp protocol\n"
	  "vlan id\n"
	  "vlan id(1-4095)\n"
	"IP1 address\n"
	"IPv4 address ( A.B.C.D ) \n"
	"IP1 address\n"
	  "IPv4 address/mask( A.B.C.D/M )\n"
	"IP1 address\n"
	"IPv6 ( X:X::X:X ) \n"
	"IP1 address\n"
	"IPv6 address/mask( X:X::X:X/M )\n"
	"IP2 address\n"
	"IPv4 address ( A.B.C.D ) \n"
	"IP2 address\n"
	  "IPv4 address/mask( A.B.C.D/M )\n"
	"IP2 address\n"
	"IPv6 ( X:X::X:X ) \n"
	"IP2 address\n"
	"IPv6 address/mask( X:X::X:X/M )\n"
	"IPV4 Network Layer Protocol Type\n"
	"IPV4 Network Layer Protocol Type (1-255)\n"
	"IPV6 Network Layer Protocol Type\n"
	"IPV6 Network Layer Protocol Type (1-255)\n"
	"Icmp protocol\n"
	"IcmpV6 protocol\n"
	"Tcp protocol\n"
	"Tcp port1\n"
	"Tcp port(1-65535)\n"
	"Tcp port2\n"
	"Tcp port(1-65535)\n"
	"Udp protocol\n"
	"Udp port1\n"
	"Udp port(1-65535)\n"
	"Udp port2\n"
	"Udp port(1-65535)\n"
	"Packet number to save\n"
	"Max packet num to save(default 100)\n")
{
    char cmd[512];
	int number = 100;
    int running = 1;
    char c;
    char l2mask[256]="l2 ";
    char l2match[256]="l2 ";
    char l3mask[256]="l3 ";
    char l3match[256]="l3 ";
    char l3mask_r[256]="l3 ";
    char l3match_r[256]="l3 ";
    char l4mask[256]="l4 ";
    char l4match[256]="l4 ";
    char l4mask_r[256]="l4 ";
    char l4match_r[256]="l4 ";
	int have_l2 =0,have_l3 =0,have_l4 =0,have_ip1=0, have_ip2=0;
	char tmp_str[256]={0};
	int i = 0,ip1_mask=0,ip2_mask=0;
    char tmp_ip1_str[64]="";
    char tmp_ip2_str[64]="";
	int is_icmp=0,is_icmpv6=0,is_tcp=0,is_udp=0,is_arp=0;

	for ( i =2 ;i <argc; i++)
	{
		if ( 0 == strcasecmp("icmp",argv[i]->arg))
		{
			is_icmp = 1;
		}
		else if ( 0 == strcasecmp("icmpv6",argv[i]->arg))
		{
			is_icmpv6 = 1;
		}
		else if ( 0 == strcasecmp("tcp",argv[i]->arg))
		{
			is_tcp = 1;
		}
		else if ( 0 == strcasecmp("udp",argv[i]->arg))
		{
			is_udp = 1;
		}
		else if ( 0 == strcasecmp("arp",argv[i]->arg))
		{
			is_arp = 1;
		}
	}

	if (count)
	{
		number = count;
	}

	//l2
	if ( etype )
	{
		have_l2 = 1;
		strncat(l2mask,"proto ",255); 
		snprintf(tmp_str,255,"proto %s ",etype);
		strncat(l2match,tmp_str,255); 
	}

	if (is_arp)
	{
		have_l2 = 1;
		strncat(l2mask,"proto ",255); 
		snprintf(tmp_str,255,"proto 0x0806 ");
		strncat(l2match,tmp_str,255); 
	}

	if ( vlanid )
	{
		have_l2 = 1;
		strncat(l2mask,"dot1q ",255); 
		snprintf(tmp_str,255,"dot1q %ld ",vlanid);
		strncat(l2match,tmp_str,255); 
	}
	//l3

	if (ip1_str)
	{
		have_ip1 =4;
		strncpy(tmp_ip1_str,ip1_str,63);
	}
	else if (ip1_subnet_str)
	{
		have_ip1 =4;
		parse_ip_mask( ip1_subnet_str,tmp_ip1_str, &ip1_mask);
	}
	else if (ip1v6_str)
	{
		have_ip1 =6;
		strncpy(tmp_ip1_str,ip1v6_str,63);
	}
	else if (ip1v6_subnet_str)
	{
		have_ip1 =6;
		parse_ip_mask( ip1v6_subnet_str,tmp_ip1_str, &ip1_mask);
	}

	if (ip2_str)
	{
		have_ip2 =4;
		strncpy(tmp_ip2_str,ip2_str,63);
	}
	else if (ip2_subnet_str)
	{
		have_ip2 =4;
		parse_ip_mask( ip2_subnet_str,tmp_ip2_str, &ip2_mask);
	}
	else if (ip2v6_str)
	{
		have_ip2 =6;
		strncpy(tmp_ip2_str,ip2v6_str,63);
	}
	else if (ip2v6_subnet_str)
	{
		have_ip2 =6;
		parse_ip_mask( ip2v6_subnet_str,tmp_ip2_str, &ip2_mask);
	}

	if ( have_ip1 == 4 || ipv4proto || is_icmp || is_tcp || is_udp )
	{
		strncat(l3mask,"ip4 ",255);
		strncat(l3mask_r,"ip4 ",255);
		strncat(l3match,"ip4 ",255);
		strncat(l3match_r,"ip4 ",255);
		have_l3 = 1;
	}
	else if ( have_ip1 == 6  || ipv6proto || is_icmpv6)
	{
		strncat(l3mask,"ip6 ",255);
		strncat(l3mask_r,"ip6 ",255);
		strncat(l3match,"ip6 ",255);
		strncat(l3match_r,"ip6 ",255);
		have_l3 = 1;
	}
	
	if ( have_ip1 && have_ip2 )
	{
		if (ip1_mask && ip2_mask)
		{
			snprintf(tmp_str,255,"src/%d dst/%d ",ip1_mask,ip2_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"src/%d dst/%d ",ip2_mask,ip1_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else if (ip1_mask)
		{
			snprintf(tmp_str,255,"src/%d dst ",ip1_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"src dst/%d ",ip1_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else if (ip2_mask)
		{
			snprintf(tmp_str,255,"src dst/%d ",ip2_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"src/%d dst ",ip2_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else
		{
			strncat(l3mask,"src dst",255); 
			strncat(l3mask_r,"src dst",255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
	}
	else if (have_ip1)
	{
		if (ip1_mask)
		{
			snprintf(tmp_str,255,"src/%d ",ip1_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"dst/%d ",ip1_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s ",tmp_ip1_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"dst %s ",tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else
		{
			strncat(l3mask,"src ",255); 
			strncat(l3mask_r,"dst ",255); 
			snprintf(tmp_str,255,"src %s ",tmp_ip1_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"dst %s ",tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
	}

	if (ipv4proto)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		snprintf(tmp_str,255,"proto %ld ",ipv4proto);
		strncat(l3match,tmp_str,255); 
		strncat(l3match_r,tmp_str,255); 
	}
	else if (is_icmp)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 1 ",255); 
		strncat(l3match_r,"proto 1 ",255); 
	}
	else if (is_tcp)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 6 ",255); 
		strncat(l3match_r,"proto 6 ",255); 
	}
	else if (is_udp)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 17 ",255); 
		strncat(l3match_r,"proto 17 ",255); 
	}
	if (ipv6proto)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		snprintf(tmp_str,255,"proto %ld ",ipv6proto);
		strncat(l3match,tmp_str,255); 
		strncat(l3match_r,tmp_str,255); 
	}
	else if (is_icmpv6)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 58 ",255); 
		strncat(l3match_r,"proto 58 ",255); 
	}

	//l4
	if (port1 || port2 || u_port1 || u_port2)
	{
		have_l4 = 1;
	}

	if (port1 && port2)
	{
		strncat(l4mask,"src_port dst_port ",255); 
		strncat(l4mask_r,"src_port dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",port1,port2);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",port2,port1);
		strncat(l4match_r,tmp_str,255); 
	}
	else if (port1)
	{
		strncat(l4mask,"src_port ",255); 
		strncat(l4mask_r,"dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld ",port1);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"dst_port %ld ",port1);
		strncat(l4match_r,tmp_str,255); 
	}
	else if (u_port1 && u_port2)
	{
		strncat(l4mask,"src_port dst_port ",255); 
		strncat(l4mask_r,"src_port dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",u_port1,u_port2);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",u_port2,u_port1);
		strncat(l4match_r,tmp_str,255); 
	}
	else if (u_port1)
	{
		strncat(l4mask,"src_port ",255); 
		strncat(l4mask_r,"dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld ",u_port1);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"dst_port %ld ",u_port1);
		strncat(l4match_r,tmp_str,255); 
	}

    // 清理旧的trace
    system("vppctl pcap trace off");
    system("vppctl classify filter pcap delete");

	if ( have_l2 || have_l3 || have_l4 )
	{
		strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
		if (have_l2)
		{
			strncat(cmd,l2mask,sizeof(cmd)-1);
		}
		if (have_l3)
		{
			strncat(cmd,l3mask,sizeof(cmd)-1);
		}
		if (have_l4)
		{
			strncat(cmd,l4mask,sizeof(cmd)-1);
		}
		strncat(cmd,"match ",sizeof(cmd)-1);
		if (have_l2)
		{
			strncat(cmd,l2match,sizeof(cmd)-1);
		}
		if (have_l3)
		{
			strncat(cmd,l3match,sizeof(cmd)-1);
		}
		if (have_l4)
		{
			strncat(cmd,l4match,sizeof(cmd)-1);
		}
		vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
		system(cmd);

		if ( have_l3 && have_l4 )
		{
			//reverse1
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask,sizeof(cmd)-1);
			strncat(cmd,l4mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match,sizeof(cmd)-1);
			strncat(cmd,l4match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);

			//reverse2
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask_r,sizeof(cmd)-1);
			strncat(cmd,l4mask,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match_r,sizeof(cmd)-1);
			strncat(cmd,l4match,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);

			//reverse3
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask_r,sizeof(cmd)-1);
			strncat(cmd,l4mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match_r,sizeof(cmd)-1);
			strncat(cmd,l4match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);

		}
		else if ( have_l3 )
		{
			//reverse1
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);
		}
		else if ( have_l4 )
		{
			//reverse1
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l4mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l4match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);
		}
	}	

    // 开始抓包
    if ( have_l2 || have_l3 || have_l4 )
    {
		snprintf(cmd, sizeof(cmd), "vppctl pcap trace rx tx drop intfc %s max %d file dpdk.pcap filter", argv[1]->arg,number);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "vppctl pcap trace rx tx drop intfc %s max %d file dpdk.pcap", argv[1]->arg,number);
	}
	vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
    system(cmd);
    
    vty_out(vty, "Started capturing on %s, saving to file%s", 
            argv[1]->arg, VTY_NEWLINE);
    vty_out(vty, "Press Enter to stop...%s", VTY_NEWLINE);
    
	//fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	
	char buf[256];
    int current_count = 0;
    while (running && current_count < number) {
        // 检查是否按下Enter键
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n') {
                running = 0;
                break;
            }
        }
        
        // 检查抓包数量
        FILE *fp = popen("vppctl pcap trace tx rx drop status | awk '/pkts/ {print $10}'", "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                current_count = atoi(buf);
            }
            pclose(fp);
        }
		vty_out(vty, "%d packets captured%s", 
				current_count, VTY_NEWLINE);
        usleep(50000);  // 100ms
    }
	
	fcntl(STDIN_FILENO, F_SETFL, 0);
	
    // 获取最终的抓包数量
    FILE *fp = popen("vppctl pcap trace tx rx drop status | awk '/pkts/ {print $10}'", "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp)) {
            current_count = atoi(buf);
        }
        pclose(fp);
    }


    // 停止抓包
    system("vppctl pcap trace off");

    // 清空trace
    system("vppctl classify filter pcap delete");
	system("cp -rf /tmp/dpdk.pcap /mnt/dpdk.pcap");
	system("rm -rf /tmp/dpdk.pcap");
	
    vty_out(vty, "Capture completed, %d packets captured%s", 
            current_count, VTY_NEWLINE);
	
    return CMD_SUCCESS;
}

DEFUN(debug_pcappacket_download,
      debug_pcappacket_download_cmd,
      "pcappacket download",
      "Capture special packets to file\n"
	  "Download pcap file\n")
{
	system("sz /mnt/dpdk.pcap");
	//execute_command("sz", 1, "/mnt/tcpdump.pcap", NULL);
	return CMD_SUCCESS;
}

DEFUN(debug_pcappacket_print,
      debug_pcappacket_print_cmd,
      "pcappacket print",
      "Capture special packets to file\n"
	  "Print pcap file\n")
{
	system("tcpdump -r /mnt/dpdk.pcap");
	//execute_command("sz", 1, "/mnt/tcpdump.pcap", NULL);
	return CMD_SUCCESS;
}

DEFPY(debug_pcapdrop_start,
      debug_pcapdrop_all_cmd,
      "pcapdrop INTERFACE [ether-type ETYPE$etype|arp] [vlan (1-4095)$vlanid]  [ip1 A.B.C.D$ip1|ip1 A.B.C.D/M$ip1_subnet| ip1 X:X::X:X$ip1v6 | ip1 X:X::X:X/M$ip1v6_subnet ] [ip2 A.B.C.D$ip2|ip2 A.B.C.D/M$ip2_subnet| ip2 X:X::X:X$ip2v6 | ip2 X:X::X:X/M$ip2v6_subnet ] [ipv4-proto (1-255)$ipv4proto | ipv6-proto (1-255)$ipv6proto | icmp | icmpv6 | tcp [port1 (1-65535)$port1] [port2 (1-65535)$port2] | udp [port1 (1-65535)$u_port1] [port2 (1-65535)$u_port2]] [count (1-10000)$count]",
      "Capture dropped packets to file\n"
	  "Interface to capture\n"
	  "Ethernet Frame Type\n"
	  "Ethernet Frame Type, eg. 0x0806 0x0800\n"
	  "Arp protocol\n"
	  "vlan id\n"
	  "vlan id(1-4095)\n"
	"IP1 address\n"
	"IPv4 address ( A.B.C.D ) \n"
	"IP1 address\n"
	  "IPv4 address/mask( A.B.C.D/M )\n"
	"IP1 address\n"
	"IPv6 ( X:X::X:X ) \n"
	"IP1 address\n"
	"IPv6 address/mask( X:X::X:X/M )\n"
	"IP2 address\n"
	"IPv4 address ( A.B.C.D ) \n"
	"IP2 address\n"
	  "IPv4 address/mask( A.B.C.D/M )\n"
	"IP2 address\n"
	"IPv6 ( X:X::X:X ) \n"
	"IP2 address\n"
	"IPv6 address/mask( X:X::X:X/M )\n"
	"IPV4 Network Layer Protocol Type\n"
	"IPV4 Network Layer Protocol Type (1-255)\n"
	"IPV6 Network Layer Protocol Type\n"
	"IPV6 Network Layer Protocol Type (1-255)\n"
	"Icmp protocol\n"
	"IcmpV6 protocol\n"
	"Tcp protocol\n"
	"Tcp port1\n"
	"Tcp port(1-65535)\n"
	"Tcp port2\n"
	"Tcp port(1-65535)\n"
	"Udp protocol\n"
	"Udp port1\n"
	"Udp port(1-65535)\n"
	"Udp port2\n"
	"Udp port(1-65535)\n"
	"Packet number to save\n"
	"Max packet num to save(default 100)\n")
{
    char cmd[512];
	int number = 100;
    int running = 1;
    char c;
    char l2mask[256]="l2 ";
    char l2match[256]="l2 ";
    char l3mask[256]="l3 ";
    char l3match[256]="l3 ";
    char l3mask_r[256]="l3 ";
    char l3match_r[256]="l3 ";
    char l4mask[256]="l4 ";
    char l4match[256]="l4 ";
    char l4mask_r[256]="l4 ";
    char l4match_r[256]="l4 ";
	int have_l2 =0,have_l3 =0,have_l4 =0,have_ip1=0, have_ip2=0;
	char tmp_str[256]={0};
	int i = 0,ip1_mask=0,ip2_mask=0;
    char tmp_ip1_str[64]="";
    char tmp_ip2_str[64]="";
	int is_icmp=0,is_icmpv6=0,is_tcp=0,is_udp=0,is_arp=0;

	for ( i =2 ;i <argc; i++)
	{
		if ( 0 == strcasecmp("icmp",argv[i]->arg))
		{
			is_icmp = 1;
		}
		else if ( 0 == strcasecmp("icmpv6",argv[i]->arg))
		{
			is_icmpv6 = 1;
		}
		else if ( 0 == strcasecmp("tcp",argv[i]->arg))
		{
			is_tcp = 1;
		}
		else if ( 0 == strcasecmp("udp",argv[i]->arg))
		{
			is_udp = 1;
		}
		else if ( 0 == strcasecmp("arp",argv[i]->arg))
		{
			is_arp = 1;
		}
	}

	if (count)
	{
		number = count;
	}

	//l2
	if ( etype )
	{
		have_l2 = 1;
		strncat(l2mask,"proto ",255); 
		snprintf(tmp_str,255,"proto %s ",etype);
		strncat(l2match,tmp_str,255); 
	}

	if (is_arp)
	{
		have_l2 = 1;
		strncat(l2mask,"proto ",255); 
		snprintf(tmp_str,255,"proto 0x0806 ");
		strncat(l2match,tmp_str,255); 
	}

	if ( vlanid )
	{
		have_l2 = 1;
		strncat(l2mask,"dot1q ",255); 
		snprintf(tmp_str,255,"dot1q %ld ",vlanid);
		strncat(l2match,tmp_str,255); 
	}
	//l3

	if (ip1_str)
	{
		have_ip1 =4;
		strncpy(tmp_ip1_str,ip1_str,63);
	}
	else if (ip1_subnet_str)
	{
		have_ip1 =4;
		parse_ip_mask( ip1_subnet_str,tmp_ip1_str, &ip1_mask);
	}
	else if (ip1v6_str)
	{
		have_ip1 =6;
		strncpy(tmp_ip1_str,ip1v6_str,63);
	}
	else if (ip1v6_subnet_str)
	{
		have_ip1 =6;
		parse_ip_mask( ip1v6_subnet_str,tmp_ip1_str, &ip1_mask);
	}

	if (ip2_str)
	{
		have_ip2 =4;
		strncpy(tmp_ip2_str,ip2_str,63);
	}
	else if (ip2_subnet_str)
	{
		have_ip2 =4;
		parse_ip_mask( ip2_subnet_str,tmp_ip2_str, &ip2_mask);
	}
	else if (ip2v6_str)
	{
		have_ip2 =6;
		strncpy(tmp_ip2_str,ip2v6_str,63);
	}
	else if (ip2v6_subnet_str)
	{
		have_ip2 =6;
		parse_ip_mask( ip2v6_subnet_str,tmp_ip2_str, &ip2_mask);
	}

	if ( have_ip1 == 4 || ipv4proto || is_icmp || is_tcp || is_udp )
	{
		strncat(l3mask,"ip4 ",255);
		strncat(l3mask_r,"ip4 ",255);
		strncat(l3match,"ip4 ",255);
		strncat(l3match_r,"ip4 ",255);
		have_l3 = 1;
	}
	else if ( have_ip1 == 6  || ipv6proto || is_icmpv6)
	{
		strncat(l3mask,"ip6 ",255);
		strncat(l3mask_r,"ip6 ",255);
		strncat(l3match,"ip6 ",255);
		strncat(l3match_r,"ip6 ",255);
		have_l3 = 1;
	}
	
	if ( have_ip1 && have_ip2 )
	{
		if (ip1_mask && ip2_mask)
		{
			snprintf(tmp_str,255,"src/%d dst/%d ",ip1_mask,ip2_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"src/%d dst/%d ",ip2_mask,ip1_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else if (ip1_mask)
		{
			snprintf(tmp_str,255,"src/%d dst ",ip1_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"src dst/%d ",ip1_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else if (ip2_mask)
		{
			snprintf(tmp_str,255,"src dst/%d ",ip2_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"src/%d dst ",ip2_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else
		{
			strncat(l3mask,"src dst",255); 
			strncat(l3mask_r,"src dst",255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip1_str,tmp_ip2_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"src %s dst %s ",tmp_ip2_str,tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
	}
	else if (have_ip1)
	{
		if (ip1_mask)
		{
			snprintf(tmp_str,255,"src/%d ",ip1_mask);
			strncat(l3mask,tmp_str,255); 
			snprintf(tmp_str,255,"dst/%d ",ip1_mask);
			strncat(l3mask_r,tmp_str,255); 
			snprintf(tmp_str,255,"src %s ",tmp_ip1_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"dst %s ",tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
		else
		{
			strncat(l3mask,"src ",255); 
			strncat(l3mask_r,"dst ",255); 
			snprintf(tmp_str,255,"src %s ",tmp_ip1_str);
			strncat(l3match,tmp_str,255); 
			snprintf(tmp_str,255,"dst %s ",tmp_ip1_str);
			strncat(l3match_r,tmp_str,255); 
		}
	}

	if (ipv4proto)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		snprintf(tmp_str,255,"proto %ld ",ipv4proto);
		strncat(l3match,tmp_str,255); 
		strncat(l3match_r,tmp_str,255); 
	}
	else if (is_icmp)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 1 ",255); 
		strncat(l3match_r,"proto 1 ",255); 
	}
	else if (is_tcp)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 6 ",255); 
		strncat(l3match_r,"proto 6 ",255); 
	}
	else if (is_udp)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 17 ",255); 
		strncat(l3match_r,"proto 17 ",255); 
	}
	if (ipv6proto)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		snprintf(tmp_str,255,"proto %ld ",ipv6proto);
		strncat(l3match,tmp_str,255); 
		strncat(l3match_r,tmp_str,255); 
	}
	else if (is_icmpv6)
	{
		strncat(l3mask,"proto ",255); 
		strncat(l3mask_r,"proto ",255); 
		strncat(l3match,"proto 58 ",255); 
		strncat(l3match_r,"proto 58 ",255); 
	}

	//l4
	if (port1 || port2 || u_port1 || u_port2)
	{
		have_l4 = 1;
	}

	if (port1 && port2)
	{
		strncat(l4mask,"src_port dst_port ",255); 
		strncat(l4mask_r,"src_port dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",port1,port2);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",port2,port1);
		strncat(l4match_r,tmp_str,255); 
	}
	else if (port1)
	{
		strncat(l4mask,"src_port ",255); 
		strncat(l4mask_r,"dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld ",port1);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"dst_port %ld ",port1);
		strncat(l4match_r,tmp_str,255); 
	}
	else if (u_port1 && u_port2)
	{
		strncat(l4mask,"src_port dst_port ",255); 
		strncat(l4mask_r,"src_port dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",u_port1,u_port2);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"src_port %ld dst_port %ld ",u_port2,u_port1);
		strncat(l4match_r,tmp_str,255); 
	}
	else if (u_port1)
	{
		strncat(l4mask,"src_port ",255); 
		strncat(l4mask_r,"dst_port ",255); 
		snprintf(tmp_str,255,"src_port %ld ",u_port1);
		strncat(l4match,tmp_str,255); 
		snprintf(tmp_str,255,"dst_port %ld ",u_port1);
		strncat(l4match_r,tmp_str,255); 
	}

    // 清理旧的trace
    system("vppctl pcap trace off");
    system("vppctl classify filter pcap delete");

	if ( have_l2 || have_l3 || have_l4 )
	{
		strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
		if (have_l2)
		{
			strncat(cmd,l2mask,sizeof(cmd)-1);
		}
		if (have_l3)
		{
			strncat(cmd,l3mask,sizeof(cmd)-1);
		}
		if (have_l4)
		{
			strncat(cmd,l4mask,sizeof(cmd)-1);
		}
		strncat(cmd,"match ",sizeof(cmd)-1);
		if (have_l2)
		{
			strncat(cmd,l2match,sizeof(cmd)-1);
		}
		if (have_l3)
		{
			strncat(cmd,l3match,sizeof(cmd)-1);
		}
		if (have_l4)
		{
			strncat(cmd,l4match,sizeof(cmd)-1);
		}
		vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
		system(cmd);

		if ( have_l3 && have_l4 )
		{
			//reverse1
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask,sizeof(cmd)-1);
			strncat(cmd,l4mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match,sizeof(cmd)-1);
			strncat(cmd,l4match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);

			//reverse2
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask_r,sizeof(cmd)-1);
			strncat(cmd,l4mask,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match_r,sizeof(cmd)-1);
			strncat(cmd,l4match,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);

			//reverse3
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask_r,sizeof(cmd)-1);
			strncat(cmd,l4mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match_r,sizeof(cmd)-1);
			strncat(cmd,l4match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);

		}
		else if ( have_l3 )
		{
			//reverse1
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l3mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l3match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);
		}
		else if ( have_l4 )
		{
			//reverse1
			strncpy(cmd,"vppctl classify filter pcap mask ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2mask,sizeof(cmd)-1);
			}
			strncat(cmd,l4mask_r,sizeof(cmd)-1);
			strncat(cmd,"match ",sizeof(cmd)-1);
			if (have_l2)
			{
				strncat(cmd,l2match,sizeof(cmd)-1);
			}
			strncat(cmd,l4match_r,sizeof(cmd)-1);
			vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
			system(cmd);
		}
	}	

    // 开始抓包
    if ( have_l2 || have_l3 || have_l4 )
    {
		snprintf(cmd, sizeof(cmd), "vppctl pcap trace drop intfc %s max %d file dpdk.pcap filter", argv[1]->arg,number);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "vppctl pcap trace drop intfc %s max %d file dpdk.pcap", argv[1]->arg,number);
	}
	vty_out(vty, "%s%s", cmd,VTY_NEWLINE);
    system(cmd);
    
    vty_out(vty, "Started capturing on %s, saving to file%s", 
            argv[1]->arg, VTY_NEWLINE);
    vty_out(vty, "Press Enter to stop...%s", VTY_NEWLINE);
    
	//fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	
	char buf[256];
    int current_count = 0;
    while (running && current_count < number) {
        // 检查是否按下Enter键
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n') {
                running = 0;
                break;
            }
        }
        
        // 检查抓包数量
        FILE *fp = popen("vppctl pcap trace tx rx drop status | awk '/pkts/ {print $6}'", "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                current_count = atoi(buf);
            }
            pclose(fp);
        }
		vty_out(vty, "%d packets captured%s", 
				current_count, VTY_NEWLINE);
        usleep(50000);  // 100ms
    }
	
	fcntl(STDIN_FILENO, F_SETFL, 0);
	
    // 获取最终的抓包数量
    FILE *fp = popen("vppctl pcap trace tx rx drop status | awk '/pkts/ {print $6}'", "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp)) {
            current_count = atoi(buf);
        }
        pclose(fp);
    }


    // 停止抓包
    system("vppctl pcap trace off");

    // 清空trace
    system("vppctl classify filter pcap delete");
	system("cp -rf /tmp/dpdk.pcap /mnt/dpdk.pcap");
	system("rm -rf /tmp/dpdk.pcap");
	
    vty_out(vty, "Capture completed, %d packets captured%s", 
            current_count, VTY_NEWLINE);
	
    return CMD_SUCCESS;
}

DEFUN(debug_pcapdrop_download,
      debug_pcapdrop_download_cmd,
      "pcapdrop download",
      "Capture dropped packets to file\n"
	  "Download pcap file\n")
{
	system("sz /mnt/dpdk.pcap");
	//execute_command("sz", 1, "/mnt/tcpdump.pcap", NULL);
	return CMD_SUCCESS;
}

DEFUN(debug_pcapdrop_print,
      debug_pcapdrop_print_cmd,
      "pcapdrop print",
      "Capture dropped packets to file\n"
	  "Print pcap file\n")
{
	system("tcpdump -r /mnt/dpdk.pcap");
	//execute_command("sz", 1, "/mnt/tcpdump.pcap", NULL);
	return CMD_SUCCESS;
}



DEFUN(debug_top,
	  debug_top_cmd,
	  "monitor process information",
	  "Monitor system processes\n"
	  "Display process information\n"
	  "Run top command to view system status\n")
{
	  system("top");
	  return CMD_SUCCESS;
}


DEFUN(debug_free,
	  debug_free_cmd,
	  "show meminfo",
	  SHOW_STR
	  "Execute 'free -h -t' command for human-readable output\n")
{
#if defined(ARCH_ARM64)
	system("free");
#else
	system("free -h -t");
#endif
	  return CMD_SUCCESS;
}

DEFUN(debug_df,
	  debug_df_cmd,
	  "show disk information",
	  SHOW_STR
	  "Show partition information\n"
	  "Execute fdisk command to list disks\n")
{
	  system("fdisk -l");
	  return CMD_SUCCESS;
}



DEFUN(debug_mpstat,
      debug_mpstat_cmd,
      "mpstat",
      "Show CPU usage\n")
{
	system("mpstat -P ALL 1 1");
    return CMD_SUCCESS;
}

	  
DEFUN(debug_ifconfig,
      debug_ifconfig_cmd,
      "ifconfig",
      "Show ifconfig\n")
{
	system("ifconfig");
    return CMD_SUCCESS;
}

DEFUN(debug_show_cpuinfo,
      debug_show_cpuinfo_cmd,
      "show cpuinfo",
	SHOW_STR
      "Show CPU info\n")
{
	system("cat /proc/cpuinfo");
    return CMD_SUCCESS;
}

DEFPY(debug_show_corefile,
      debug_show_corefile_cmd,
      "show corefile [FILENAME$filename]",
		SHOW_STR
	"Corefiles information\n"
      "Panic bt information\n")
{
    FILE *fp;
    char buf[4096];
    char cmd[256];
    char fullpath[256];

	if (argc == 2)
	{
	    fp = popen("ls -al /mnt/corefile/", "r");
	    if (!fp) {
	        vty_out(vty, "Failed to execute command%s", VTY_NEWLINE);
	        return CMD_WARNING;
	    }

	    while (fgets(buf, sizeof(buf), fp)) {
	        vty_out(vty, "%s", buf);
	    }

	    pclose(fp);
	}
	else
	{
	    // 构建完整路径
	    snprintf(fullpath, sizeof(fullpath), "/mnt/corefile/%s", filename);

	    // 检查文件是否存在
	    if (access(fullpath, F_OK) == -1) {
	        vty_out(vty, "Error: Corefile %s not found %s", 
	               filename, VTY_NEWLINE);
	        return CMD_WARNING;
	    }

		if (0== strncmp(filename,"core-vpp_",strlen("core-vpp_")))
		{
			// 构建gdb命令
			snprintf(cmd, sizeof(cmd), "gdb /usr/local/vpp/bin/vpp -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-hyclid",strlen("core-hyclid")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/local/hycli/bin/hyclid -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-hycli",strlen("core-hycli")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/local/hycli/bin/hycli -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-sslvpn",strlen("core-sslvpn")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/private/sslvpn -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-Collector",strlen("core-Collector")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/private/Collector -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-ace_userspace",strlen("core-ace_userspace")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/private/ace_userspace -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-l7-filter",strlen("core-l7-filter")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/private/l7-filter -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else if (0== strncmp(filename,"core-mail-filter",strlen("core-mail-filter")))
		{
			snprintf(cmd, sizeof(cmd), "gdb /usr/private/mail-filter -c /mnt/corefile/%s -ex bt -ex quit 2>&1", filename);
		}
		else
		{
	        vty_out(vty, "Failed to support this file.%s", VTY_NEWLINE);
	        return CMD_WARNING;
		}

		system(cmd);
#if 0		
		fp = popen(cmd, "r");
		if (!fp) {
			vty_out(vty, "Failed to execute gdb command%s", VTY_NEWLINE);
			return CMD_WARNING;
		}
		
		// 读取并显示gdb输出
		while (fgets(buf, sizeof(buf), fp)) {
			vty_out(vty, "%s", buf);
		}
		vty_out(vty, "\n");
		
		pclose(fp);
#endif
	}
    return CMD_SUCCESS;
}

DEFUN(debug_show_process,
      debug_show_process_cmd,
      "show processes",
      SHOW_STR
      "Process information\n")
{
	system("ps -uax");
    return CMD_SUCCESS;
}

DEFUN(debug_netstat,
      debug_netstat_cmd,
      "show network status",
      "Execute netstat -an\n"
	  "netstat\n"
	  "netstat\n")
{
	system("netstat -an");
    return CMD_SUCCESS;
}

DEFPY(debug_show_variable,
      debug_show_variable_cmd,
      "show variable <dpdk|userspace|dpi|sslvpn|collector> NAME$name",
		SHOW_STR
		"Show variable value\n"
		"Dpdk module\n"
		"Userspace module\n"
		"DPI module\n"
		"Sslvpn module\n"
		"Collector module\n"
      "Name of variable\n")
{
    char cmd[256];

	if ( 0 == strcmp("dpdk",argv[2]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -v %s", name);
	}
	else if ( 0 == strcmp("userspace",argv[2]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -u %s", name);
	}
	else if ( 0 == strcmp("dpi",argv[2]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -l %s", name);
	}
	else if ( 0 == strcmp("collector",argv[2]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -c %s", name);
	}
	else if ( 0 == strcmp("sslvpn",argv[2]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -o %s", name);
	}

	system(cmd);
    return CMD_SUCCESS;
}

DEFPY(debug_variable,
      debug_variable_cmd,
      "variable <dpdk|userspace|dpi|sslvpn|collector> NAME$name VALUE$vvalue",
		"Set variable value\n"
		"Dpdk module\n"
		"Userspace module\n"
		"DPI module\n"
		"Sslvpn module\n"
		"Collector module\n"
      "Name of variable\n"
      "Value to set\n")
{
    char cmd[256];

	if ( 0 == strcmp("dpdk",argv[1]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -v %s %s", name ,vvalue);
	}
	else if ( 0 == strcmp("userspace",argv[1]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -u %s %s", name ,vvalue);
	}
	else if ( 0 == strcmp("dpi",argv[1]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -l %s %s", name ,vvalue);
	}
	else if ( 0 == strcmp("collector",argv[1]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -c %s %s", name ,vvalue);
	}
	else if ( 0 == strcmp("sslvpn",argv[1]->arg))
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -v -o %s %s", name ,vvalue);
	}

	system(cmd);
    return CMD_SUCCESS;
}

#define MAX_LINE_LEN 512
#define TIME_STR_LEN 19

static inline int is_valid_time_format(const char *str)
{
    // 快速检查第一个字符和关键分隔符位置
    if (str[4] != '-' || str[7] != '-' || str[10] != ' ' || 
        str[13] != ':' || str[16] != ':') {
        return 0;
    }
    
    // 只检查年份的第一位，减少判断次数
    if (str[0] != '2') return 0;
    
    return 1;
}

static int read_log_after_time(time_t target_time,struct vty *vty, int maxline)
{
    FILE *fp;
    struct tm tm_time;
    time_t line_time;
    char line[MAX_LINE_LEN];
	int num = 0,max_num = 256;

	if ( maxline )
	{
		max_num = maxline;
	}

    fp = fopen("/var/log/sum", "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (!is_valid_time_format(line)) {
            continue;
        }

        memset(&tm_time, 0, sizeof(struct tm));
        if (strptime(line, "%Y-%m-%d %H:%M:%S", &tm_time) == NULL) {
            continue;
        }
        line_time = mktime(&tm_time);

        if (line_time >= target_time) {
            num++;
			if ( num > max_num )
			{
				break;
			}
            vty_out(vty,"%s",line);
        }
    }

    fclose(fp);
    return 0;
}


DEFPY(debug_show_session,
      debug_show_session_cmd,
      "show session [protocol <tcp|udp|icmp|gre>$pro] [sip <A.B.C.D$sipv4|X:X::X:X$sipv6>] [sport (0-65535)$sport] [dip <A.B.C.D$dipv4|X:X::X:X$dipv6>] [dport (0-65535)$dport] [lines (1-10000)$line]",
		SHOW_STR
		"Show session table\n"
		"Transport layer protocol\n"
		"Tcp protocol\n"
		"Udp protocol\n"
		"Icmp protocol\n"
		"Gre protocool\n"
		"Source ip address\n"
		"Source IPv4 address ( A.B.C.D )\n"
		"Source IPv6 address ( X:X::X:X )\n"
		"Source port\n"
		"Source port (0-65535)\n"
		"Destination ip address\n"
		"Destination IPv4 address ( A.B.C.D )\n"
		"Destination IPv6 address ( X:X::X:X )\n"
		"Destination port\n"
		"Destination port (0-65535)\n"
		"Max output line number (default 256)\n"
		"Max output line number (default 256)\n")
{
    char cmd[256];
	time_t now;
	int i = 0;
	int sipindex = -1,dipindex = -1;
	char protocol_str[16]="all";
	char sip_str[64]="0.0.0.0";
	char dip_str[64]="0.0.0.0";
	int sport_value = 0,dport_value = 0;

	for ( i = 2; i < argc-1;i++ )
	{
		if (0==strcasecmp("sip", argv[i]->arg))
		{
			sipindex = i+1;
		}
		
		if (0==strcasecmp("dip", argv[i]->arg))
		{
			dipindex = i+1;
		}
	}

	if (pro)
	{
		strncpy(protocol_str,pro,15);
	}
	
	if (sipindex>0)
	{
		strncpy(sip_str,argv[sipindex]->arg,63);
	}

	if (dipindex>0)
	{
		strncpy(dip_str,argv[dipindex]->arg,63);
	}

	if (sport)
	{
		sport_value = sport;
	}

	if (dport)
	{
		dport_value = dport;
	}
	
	if ( pro || (sipindex>0) || sport || (dipindex>0) || dport )
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -f -v hytf_print_one_session_by_5tuple -s%s -s%s -l%d -s%s -l%d", protocol_str,sip_str,
			sport_value,dip_str,dport_value);
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "hytfdbg -f -v hytf_print_all_sessions");
	}
	//vty_out(vty, "cmd=%s\n", cmd);
    time(&now);
	system(cmd);

    read_log_after_time(now,vty,line);
    return CMD_SUCCESS;
}


DEFPY(debug_show_host,
      debug_show_host_cmd,
      "show host <A.B.C.D$sipv4|X:X::X:X$sipv6>",
		SHOW_STR
		"Show host table\n"
		"Source IPv4 address ( A.B.C.D )\n"
		"Source IPv6 address ( X:X::X:X )\n")
{
    char cmd[256];
	time_t now;

	snprintf(cmd, sizeof(cmd), "hytfdbg -f -v hytf_print_host -s%s",argv[2]->arg);

	vty_out(vty, "cmd=%s\n", cmd);
    time(&now);
	system(cmd);

    read_log_after_time(now,vty,0);
    return CMD_SUCCESS;
}

DEFPY(debug_show_trace,
      debug_show_trace_cmd,
      "show trace <dpdk|sum>",
		SHOW_STR
		"Show trace\n"
		"Show dpdk trace\n"
		"Show sum module trace\n")
{
	if ( 0 ==strcasecmp(argv[2]->arg,"dpdk") )
    {
		system("vppctl show trace");
	}
	else
	{
		system("cat/tmp/trace");
	}

    return CMD_SUCCESS;
}

DEFPY(debug_trace_clear,
      debug_trace_clear_cmd,
      "trace-clear <all|dpdk|sum>",
	  "Vpp clear trace \n"
	  "Clear both dpdk and sum module trace\n"
	  "Clear dpdk(vpp) trace\n"
	  "Clear sum(mytrace) trace\n")
{
	if ( 0 ==strcasecmp(argv[1]->arg,"dpdk") )
    {
		system("vppctl clear trace");
	}	
	else if ( 0 ==strcasecmp(argv[1]->arg,"sum") )
    {
		system("vppctl clear my-trace");
		system("rm /tmp/trace.txt");
	}	
	else
    {
		system("vppctl clear trace");
		system("vppctl clear my-trace");
		system("rm /tmp/trace.txt");
	}	
    return CMD_SUCCESS;
}

DEFPY(debug_trace_save,
      debug_trace_save_cmd,
      "trace-save <all|(1-99)$num>",
	  "Save sum trace info from memory to file\n"
	  "Save all thread information to file\n"
	  "Save specified thread information to file\n")
{
    char cmd[256];


	if ( 0 ==strcasecmp(argv[1]->arg,"all") )
    {
		system("/usr/local/vpp/bin/vppctl save my-trace thread 100s");
	}	
	else
    {
		snprintf(cmd, sizeof(cmd), "/usr/local/vpp/bin/vppctl save my-trace thread %ld",num);
		system(cmd);
	}	
    return CMD_SUCCESS;
}

DEFPY(debug_trace_add,
      debug_trace_add_cmd,
      "trace-add <dpdk-input|af-packet-input|virtio-input> (1-100)$num [protocol <(0-127)$pro|tcp|udp|icmp|icmp6|esp|arp|ppp|all>] [sip <A.B.C.D$sipv4|X:X::X:X$sipv6>] [sport (0-65535)$sport] [dip <A.B.C.D$dipv4|X:X::X:X$dipv6>] [dport (0-65535)$dport] type <path|drop|packet|path-drop|path-packet|drop-packet|path-drop-packet> [oneway]",
		"Vpp trace add function\n"
		"Vpp node:dpdk-input\n"
		"Vpp node:af-packet-input\n"
		"Vpp node:virtio-input\n"
		"Trace packet number\n"
		"Transport layer protocol\n"
		"Protocol value\n"
		"Tcp protocol\n"
		"Udp protocol\n"
		"Icmp protocol\n"
		"Icmp6 protocol\n"
		"Esp protocol\n"
		"Arp protocol\n"
		"Ppp protocol\n"
		"All protocol\n"
		"Source ip address\n"
		"Source IPv4 address ( A.B.C.D )\n"
		"Source IPv6 address ( X:X::X:X )\n"
		"Source port\n"
		"Source port (0-65535)\n"
		"Destination ip address\n"
		"Destination IPv4 address ( A.B.C.D )\n"
		"Destination IPv6 address ( X:X::X:X )\n"
		"Destination port\n"
		"Destination port (0-65535)\n"
		"Type of packet traced\n"
		"Path type\n"
		"Drop type\n"
		"packet type\n"
		"Path and drop type\n"
		"Path and packet type\n"
		"Drop and packet type\n"
		"Path ,drop and packet type\n"
		"Trace one way\n")
{
	int proindex=-1,sipindex = -1,dipindex = -1,typeindex=-1;
	int i = 0;
	char protocol_str[16]="all";
	char sip_str[64]="0.0.0.0";
	char dip_str[64]="0.0.0.0";
	int sport_value = 0,dport_value = 0;
	char type_str[64]="path-drop-packet";
	int oneway_set = 0;
    char cmd[256];
	
	for ( i = 3; i < argc;i++ )
	{
		if (0==strcasecmp("protocol", argv[i]->arg))
		{
			proindex = i+1;
		}
		
		if (0==strcasecmp("sip", argv[i]->arg))
		{
			sipindex = i+1;
		}
		
		if (0==strcasecmp("dip", argv[i]->arg))
		{
			dipindex = i+1;
		}
		
		if (0==strcasecmp("type", argv[i]->arg))
		{
			typeindex = i+1;
		}
		
		if (0==strcasecmp("oneway", argv[i]->arg))
		{
			oneway_set = 1;
		}
	}

	if ( proindex > 0 )
	{
		strncpy(protocol_str,argv[proindex]->arg,15);
	}
	
	if (sipindex>0)
	{
		strncpy(sip_str,argv[sipindex]->arg,63);
	}

	if (dipindex>0)
	{
		strncpy(dip_str,argv[dipindex]->arg,63);
	}

	if (sport)
	{
		sport_value = sport;
	}

	if (dport)
	{
		dport_value = dport;
	}
	
	if (typeindex>0)
	{
		strncpy(type_str,argv[typeindex]->arg,63);
	}

	if (oneway_set)
	{
		strcat(type_str,"-oneway");
	}

	snprintf(cmd, sizeof(cmd), "vppctl trace add %s %ld match %s+%s+%s+%d+%d+%s",argv[1]->arg, num,protocol_str,sip_str,dip_str,sport_value,dport_value,type_str );

	vty_out(vty, "cmd=%s\n", cmd);
    return CMD_SUCCESS;
}

DEFPY(debug_ike_1,
      debug_ike_1_cmd,
      "show ike <stats|list-sas|list-conns>",
      SHOW_STR
		"Excute swanctl command\n"
		"Show daemon stats information\n"
		"List currently active IKE_SAs\n"
        "List loaded configurations\n")
{
	if (0==strcasecmp("stats", argv[2]->arg))
	{
		system("/usr/local/strongswan/sbin/swanctl --stats");
	}
	else if (0==strcasecmp("list-sas", argv[2]->arg))
	{
		system("/usr/local/strongswan/sbin/swanctl --list-sas");
	}
	else if (0==strcasecmp("list-conns", argv[2]->arg))
	{
		system("/usr/local/strongswan/sbin/swanctl --list-conns");
	}

    return CMD_SUCCESS;
}

DEFPY(debug_ike_2,
      debug_ike_2_cmd,
      "ike <load-all|rekey|terminate|initiate> [ike IKENAME$ikename|child CHILDNAME$childname]",
	  "Excute swanctl command\n"
	"Load credentials, authorities, pools and connections\n"
	"Rekey an SA\n"
	"Rerminate a connection\n"
	"Initiate a connection\n"
	"filter ike sa by name\n"
	"IKE sa name\n"
	"filter child sa by name\n"
    "Child sa name\n")
{
	char cmd[256]="/usr/local/strongswan/sbin/swanctl ";

	if (0==strcasecmp("load-all", argv[1]->arg))
	{
		strcat(cmd,"--load-all");
	}
	else if (0==strcasecmp("rekey", argv[1]->arg))
	{
		strcat(cmd,"--rekey");
	}
	else if (0==strcasecmp("terminate", argv[1]->arg))
	{
		strcat(cmd,"--terminate");
	}
	else if (0==strcasecmp("initiate", argv[1]->arg))
	{
		strcat(cmd,"--initiate");
	}
	else
	{
		return CMD_SUCCESS;
	}

	if (ikename)
	{
		strcat(cmd," --ike ");
		strncat(cmd,ikename,255);
	}
	else if (childname)
	{
		strcat(cmd," --child ");
		strncat(cmd,childname,255);
	}

	system(cmd);

    return CMD_SUCCESS;
}

DEFPY(debug_ipsec,
      debug_ipsec_cmd,
      "ipsec <start | restart | stop | up CONNECTIONNAME$connectionname3 | down CONNECTIONNAME$connectionname4>",
	  "Excute ipsec command\n"
	"Ipsec start\n"
	"Ipsec restart\n"
	"Ipsec stop\n"
	"Ipsec up\n"
    "Ipsec connection name\n"
	"Ipsec down\n"
    "Ipsec connection name\n")
{
	char cmd[256]="/usr/local/strongswan/sbin/ipsec ";

	if (0==strcasecmp("up", argv[1]->arg))
	{
		strcat(cmd,"up ");
	}
	else if (0==strcasecmp("down", argv[1]->arg))
	{
		strcat(cmd,"down ");
		strncat(cmd,connectionname3,255);
	}
	else if (0==strcasecmp("start", argv[1]->arg))
	{
		strcat(cmd,"start");
	}
	else if (0==strcasecmp("restart", argv[1]->arg))
	{
		strcat(cmd,"restart");
	}
	else if (0==strcasecmp("stop", argv[1]->arg))
	{
		strcat(cmd,"stop");
	}
	else
	{
		return CMD_SUCCESS;
	}
	system(cmd);

    return CMD_SUCCESS;
}
	  
DEFPY(debug_ipsec1,
      debug_ipsec1_cmd,
      "show ipsec <version | status [CONNECTIONNAME$connectionname1] |statusall [CONNECTIONNAME$connectionname2]>",
	SHOW_STR
	"Excute ipsec command\n"
	"Ipsec version\n"
	"Ipsec status\n"
    "Ipsec connection name\n"
	"Ipsec statusall\n"
    "Ipsec connection name\n")
{
	char cmd[256]="/usr/local/strongswan/sbin/ipsec ";

	if (0==strcasecmp("status", argv[2]->arg))
	{
		strcat(cmd,"status ");
		if (connectionname1)
		{
			strncat(cmd,connectionname1,255);
		}
	}
	else if (0==strcasecmp("statusall", argv[2]->arg))
	{
		strcat(cmd,"statusall ");
		if (connectionname2)
		{
			strncat(cmd,connectionname2,255);
		}
	}
	else if (0==strcasecmp("version", argv[2]->arg))
	{
		strcat(cmd,"version");
	}
	else
	{
		return CMD_SUCCESS;
	}
	system(cmd);

    return CMD_SUCCESS;
}


DEFUN(trouble_shooting_exit, trouble_shooting_exit_cmd, "exit",
	"Exit current mode and down to previous mode\n")
{
	return vtysh_exit(vty);
}

	
DEFUN(config_debug_exit, config_debug_exit_cmd, "exit",
	"Exit current mode and down to previous mode\n")
{
	return vtysh_exit(vty);
}


static struct cmd_node trouble_shooting_node = {
	.name = "trouble_shooting_node",
	.node = TROUBLESHOOTING_NODE,
	.parent_node = VIEW_NODE,
	.prompt = "%s(trouble-shooting)# ",
	.config_write = NULL,
};

static struct cmd_node config_debug_node = {
	.name = "config_debug_node",
	.node = CONFIG_DEBUG_NODE,
	.parent_node = CONFIG_NODE,
	.prompt = "%s(debug)# ",
	.config_write = NULL,
};


DEFUNSH(0,trouble_shooting, 
			trouble_shooting_cmd,
	"trouble-shooting",
	"Trouble shooting mode\n")
{
	vty->node = TROUBLESHOOTING_NODE;
	return CMD_SUCCESS;
}

DEFUNSH(0,config_debug, 
			config_debug_cmd,
	"debug",
	"Config debug mode\n")
{
	vty->node = CONFIG_DEBUG_NODE;
	return CMD_SUCCESS;
}

void vtysh_hy_system_cmd_init(void)
{
	install_element(VIEW_NODE, &show_license_info_cmd);
	install_element(VIEW_NODE, &show_version_info_cmd);

	install_element(CONFIG_NODE, &username_password_cmd);
	install_element(CONFIG_NODE, &username_role_cmd);
	//install_element(CONFIG_NODE, &configuration_reauthentication_cmd);
	//install_element(CONFIG_NODE, &vtysh_reboot_cmd);
	install_element(ENABLE_NODE, &vtysh_reboot_cmd);
	install_element(ENABLE_NODE, &vtysh_poweroff_cmd);
	//install_element(CONFIG_NODE, &vtysh_poweroff_cmd);
	install_element(CONFIG_NODE, &vtysh_save_cmd);
	install_element(VIEW_NODE, &vtysh_save_cmd);
	//install_element(CONFIG_NODE, &vtysh_backup_cmd);
	install_element(ENABLE_NODE, &vtysh_backup_cmd);	
	//install_element(CONFIG_NODE, &vtysh_restore_cmd);
	install_element(ENABLE_NODE, &vtysh_restore_cmd);
	//install_element(CONFIG_NODE, &vtysh_upgrade_system_cmd);
	install_element(ENABLE_NODE, &vtysh_upgrade_system_cmd);
	//install_element(CONFIG_NODE, &vtysh_rollback_system_cmd);
	install_element(ENABLE_NODE, &vtysh_rollback_system_cmd); 
	//install_element(CONFIG_NODE, &update_force_rule_cmd);
	install_element(ENABLE_NODE, &update_force_rule_cmd);
	install_element(ENABLE_NODE, &update_online_rule_cmd);
	install_element(ENABLE_NODE, &startup_configuration_backup_cmd);
	install_element(CONFIG_NODE, &startup_configuration_backup_cmd);


	install_node(&trouble_shooting_node);
	install_node(&config_debug_node);
	install_element(ENABLE_NODE, &trouble_shooting_cmd);
	install_element(CONFIG_NODE, &config_debug_cmd);
	install_element(TROUBLESHOOTING_NODE, &trouble_shooting_exit_cmd);	
    install_element(TROUBLESHOOTING_NODE, &debug_tcpdump_start_cmd);
    install_element(TROUBLESHOOTING_NODE, &debug_tcpdump_download_cmd);
	//install_element(TROUBLESHOOTING_NODE, &debug_ifconfig_cmd);
	

	install_element(CONFIG_DEBUG_NODE, &config_debug_exit_cmd);	
    install_element(VIEW_NODE,            &debug_ike_1_cmd);
    install_element(CONFIG_NODE,          &debug_ike_1_cmd);	
    install_element(TROUBLESHOOTING_NODE, &debug_ike_1_cmd);
    install_element(CONFIG_DEBUG_NODE,    &debug_ike_1_cmd);	
    install_element(CONFIG_DEBUG_NODE, &debug_ike_2_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_ipsec_cmd);
    install_element(VIEW_NODE,            &debug_ipsec1_cmd);
    install_element(CONFIG_NODE,          &debug_ipsec1_cmd);	
    install_element(TROUBLESHOOTING_NODE, &debug_ipsec1_cmd);
    install_element(CONFIG_DEBUG_NODE,    &debug_ipsec1_cmd);	
    install_element(TROUBLESHOOTING_NODE, &debug_show_session_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_show_host_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_show_trace_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_trace_clear_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_trace_add_cmd);
	install_element(CONFIG_DEBUG_NODE, &debug_trace_download_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_show_variable_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_variable_cmd);
	install_element(CONFIG_DEBUG_NODE, &debug_show_cpuinfo_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_show_corefile_cmd);
    install_element(TROUBLESHOOTING_NODE, &debug_show_process_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_pcappacket_all_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_pcappacket_download_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_pcappacket_print_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_pcapdrop_all_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_pcapdrop_download_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_pcapdrop_print_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_tcpdump_start_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_tcpdump_download_cmd);
    install_element(TROUBLESHOOTING_NODE, &debug_top_cmd);
    install_element(TROUBLESHOOTING_NODE, &debug_free_cmd);
    install_element(TROUBLESHOOTING_NODE, &debug_df_cmd);
    install_element(CONFIG_DEBUG_NODE, &debug_mpstat_cmd);
	//install_element(CONFIG_DEBUG_NODE, &debug_ifconfig_cmd);
    install_element(TROUBLESHOOTING_NODE, &debug_netstat_cmd);
	install_element(CONFIG_DEBUG_NODE, &maintenance_mode_cmd);

	install_element(VIEW_NODE,            &show_interface_cmd);
    install_element(CONFIG_NODE,          &show_interface_cmd);	
    install_element(TROUBLESHOOTING_NODE, &show_interface_cmd);
    install_element(CONFIG_DEBUG_NODE,    &show_interface_cmd);
	
    install_element(VIEW_NODE,            &show_configuration_autobackup_cmd);
    install_element(CONFIG_NODE,          &show_configuration_autobackup_cmd);	
    install_element(TROUBLESHOOTING_NODE, &show_configuration_autobackup_cmd);
    install_element(CONFIG_DEBUG_NODE,    &show_configuration_autobackup_cmd);

    install_element(VIEW_NODE,            &debug_vtysh_ping_cmd);
    install_element(CONFIG_NODE,          &debug_vtysh_ping_cmd);	
    install_element(TROUBLESHOOTING_NODE, &debug_vtysh_ping_cmd);
    install_element(CONFIG_DEBUG_NODE,    &debug_vtysh_ping_cmd);

    install_element(VIEW_NODE,            &debug_vtysh_traceroute_cmd);
    install_element(CONFIG_NODE,          &debug_vtysh_traceroute_cmd);	
    install_element(TROUBLESHOOTING_NODE, &debug_vtysh_traceroute_cmd);
    install_element(CONFIG_DEBUG_NODE,    &debug_vtysh_traceroute_cmd);
	
	install_element(ENABLE_NODE, &vtysh_interface_cmd);
    install_element(CONFIG_NODE, &vtysh_interface_cmd);

	install_element(INTERFACE_NODE, &hy_interface_exit_cmd);
    install_element(INTERFACE_NODE, &hy_interface_end_cmd);
	
	install_element(VIEW_NODE, &show_ip_address_cmd);
    install_element(CONFIG_NODE, &show_ip_address_cmd);
    install_element(INTERFACE_NODE, &show_ip_address_cmd);
	
	install_element (CONFIG_NODE, &vtysh_ip_route_static_cmd);
	install_element (CONFIG_NODE, &no_vtysh_ip_route_static_cmd);
	install_element (ENABLE_NODE, &vtysh_ip_route_static_cmd);
	install_element (ENABLE_NODE, &no_vtysh_ip_route_static_cmd);

	install_element (VIEW_NODE, &show_route_static_cmd);
	//install_element (ENABLE_NODE, &show_route_static_cmd);
	install_element (CONFIG_NODE, &show_route_static_cmd);
	
	install_element (INTERFACE_NODE, &ip_address_cmd);
	//install_element (CONFIG_NODE, &ip_address_cmd);
	//install_element (ENABLE_NODE, &ip_address_cmd);
	install_element(INTERFACE_NODE, &no_ip_address_cmd);
	//install_element(CONFIG_NODE, &no_ip_address_cmd);
	//install_element(ENABLE_NODE, &no_ip_address_cmd);

	install_element(VIEW_NODE, &show_arp_cmd);
	install_element(CONFIG_NODE, &show_arp_cmd);
	install_element(INTERFACE_NODE, &show_arp_cmd);
	install_element(ENABLE_NODE, &show_arp_cmd);

	install_element(VIEW_NODE, &clear_arp_cache_cmd);
	install_element(CONFIG_NODE, &clear_arp_cache_cmd);
	install_element(INTERFACE_NODE, &clear_arp_cache_cmd);
	install_element(ENABLE_NODE, &clear_arp_cache_cmd);

	
}
//#endif
