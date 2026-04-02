
#ifndef __ACE_COMMON_MACRO_H__
#define __ACE_COMMON_MACRO_H__ 1

#include <linux/if_ether.h>


#define URL_CYBERMONEY_CATAGORY_ID 53
#define APP_CYBERMONEY_GROUP_ID 32

#ifndef COMPILING_IN_VPP
#ifndef static_always_inline
#define static_always_inline static inline
#endif
#endif
#ifdef __KERNEL__
#include <net/netfilter/nf_nat.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#define nf_nat_multi_range_compat           nf_nat_ipv4_multi_range_compat
#define IP_NAT_MANIP_DST                    NF_NAT_MANIP_DST
#define IP_NAT_MANIP_SRC                    NF_NAT_MANIP_SRC
#define IP_NAT_RANGE_MAP_IPS                NF_NAT_RANGE_MAP_IPS
#define IP_NAT_RANGE_PROTO_SPECIFIED        NF_NAT_RANGE_PROTO_SPECIFIED
#define NF_CT_DEFAULT_ZONE                  &nf_ct_zone_dflt
#define compare_ether_addr                  compare_ether_header
#define strnicmp                            strncasecmp  
#define INIT_DELAYED_WORK_DEFERRABLE        INIT_DEFERRABLE_WORK
#define ipv6_addr_jhash                     ipv6_addr_hash

static inline void hytf_nat_convert_range(struct nf_nat_range *dst,
				 const struct nf_nat_ipv4_range *src)
{
	memset(&dst->min_addr, 0, sizeof(dst->min_addr));
	memset(&dst->max_addr, 0, sizeof(dst->max_addr));

	dst->flags	 = src->flags;
	dst->min_addr.ip = src->min_ip;
	dst->max_addr.ip = src->max_ip;
	dst->min_proto	 = src->min;
	dst->max_proto	 = src->max;
}
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
static inline void hytf_nat_convert_range2(struct nf_nat_range2 *dst,
				 const struct nf_nat_ipv4_range *src)
{
	memset(&dst->min_addr, 0, sizeof(dst->min_addr));
	memset(&dst->max_addr, 0, sizeof(dst->max_addr));

	dst->flags	 = src->flags;
	dst->min_addr.ip = src->min_ip;
	dst->max_addr.ip = src->max_ip;
	dst->min_proto	 = src->min;
	dst->max_proto	 = src->max;
}
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
#define hylab_netlink_ack(_in_skb, _nlh, _err) netlink_ack(_in_skb, _nlh, _err, NULL)
#define hylab_skb_iif(_skb) _skb->hylab_skb_iif
#else
#define hylab_netlink_ack(_in_skb, _nlh, _err) netlink_ack(_in_skb, _nlh, _err)
#define hylab_skb_iif(_skb) _skb->skb_iif
#endif

#else
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#ifdef IFNAMSIZ
#undef IFNAMSIZ
#endif
#define	IFNAMSIZ	16
#ifndef HZ
#if defined(ARCH_ARM64) && ( defined(CS_M3720_PLATFORM) || defined(CS_NXP_PLATFORM))
#define HZ 250
#else
#define HZ 1000
#endif
#endif

#ifdef CONFIG_HYTF_FW
#define HYLAB_DEVNAME_LEN 32
#else
#define HYLAB_DEVNAME_LEN IFNAMSIZ
#endif

#ifndef COMPILING_IN_VPP
#ifndef static_always_inline
#define static_always_inline static inline
#endif
#endif

#ifdef COMPILING_IN_VPP
#include <rte_config.h>
#include <rte_spinlock.h>
#include <rte_rwlock.h>
#include <linux/if_pppox.h>

#define HYLAB_STRNCPY(_dst, _src) hylab_strncpy(_dst, _src, sizeof(_dst))
extern char *hylab_strncpy(char *dest, const char *src, size_t count); 
#define CLIB_STRNCPY(_dst, _src, _len) do{clib_memcpy_fast(_dst, _src, _len); _dst[_len] = 0;}while(0)
#define HYLAB_FAST_STRNCPY(_dst, _src, _max) do{size_t __max = (size_t)(_max); rsize_t _slen = clib_strnlen((const char*)(_src), __max); if (!_slen)break; if (_slen >= __max)--_slen; clib_memcpy_fast((_dst), (_src), _slen); (_dst)[_slen] = 0;}while(0)
#define HYLAB_FAST_STRCPY_SIZEOF(_dst, _src) do{rsize_t _slen = clib_strnlen((const char*)(_src), sizeof((_dst))); if (!_slen)break; if (_slen >= sizeof((_dst)))--_slen; clib_memcpy_fast((_dst), (_src), _slen); (_dst)[_slen] = 0;}while(0)
#ifndef jiffies
extern unsigned long *g_hylab_jiffies;
extern clib_time_t g_hytf_clib_time;
static_always_inline unsigned long get_jiffies(int realtime)
{
#if 0
    if (g_hylab_jiffies)
    {
    	if (realtime)
    	{
        	*g_hylab_jiffies = (unsigned long)(((g_hytf_clib_time.init_cpu_time * g_hytf_clib_time.seconds_per_clock) + clib_time_now (&g_hytf_clib_time)) * HZ);
    	}
        return *g_hylab_jiffies;
    }
    else
    {
        return (unsigned long)(((g_hytf_clib_time.init_cpu_time * g_hytf_clib_time.seconds_per_clock) + clib_time_now (&g_hytf_clib_time)) * HZ);
    }
#else
    if (g_hylab_jiffies)
    {
        if (realtime)
        {
			u64 time_now = clib_cpu_time_now ();
            *g_hylab_jiffies = (unsigned long)(((g_hytf_clib_time.init_cpu_time + time_now)) / (unsigned long)g_hytf_clib_time.clocks_per_second * HZ) + ((time_now / (unsigned long)g_hytf_clib_time.clocks_per_second) % HZ);
        }
        return *g_hylab_jiffies;
    }
    else
    {
        u64 time_now = clib_cpu_time_now ();
        return (unsigned long)(((g_hytf_clib_time.init_cpu_time + time_now)) / (unsigned long)g_hytf_clib_time.clocks_per_second * HZ) + ((time_now / (unsigned long)g_hytf_clib_time.clocks_per_second) % HZ);
    }
#endif
}
#define jiffies get_jiffies(0)/*(*g_hylab_jiffies)*/
#endif

#if 1
typedef rte_rwlock_t* 	hylab_rwlock_t;
#define hylab_rwlock_read_lock(x)	//rte_rwlock_read_lock(*(x))
#define hylab_rwlock_read_unlock(x)	//rte_rwlock_read_unlock(*(x))
#define hylab_rwlock_read_lock_new(x)	rte_rwlock_read_lock(*(x))
#define hylab_rwlock_read_unlock_new(x)	rte_rwlock_read_unlock(*(x))
#define hylab_rwlock_write_lock(x)	rte_rwlock_write_lock(*(x))
#define hylab_rwlock_write_unlock(x)	rte_rwlock_write_unlock(*(x))
#define hylab_rwlock_init(x)	{*(x) = clib_mem_alloc_aligned (CLIB_CACHE_LINE_BYTES, CLIB_CACHE_LINE_BYTES);rte_rwlock_init(*(x));}
#define HYLAB_RWLOCK_SIZE (sizeof(rte_rwlock_t))
#define hylab_rwlock_init2(x,y)	{*(x) = y;rte_rwlock_init(*(x));}
#define DEFINE_RWLOCK(x) hylab_rwlock_t x = NULL/*RTE_RWLOCK_INITIALIZER*/
#else
typedef clib_rwlock_t 	hylab_rwlock_t;
#define hylab_rwlock_read_lock(x)	clib_rwlock_reader_lock(x)
#define hylab_rwlock_read_unlock(x)	clib_rwlock_reader_unlock(x)
#define hylab_rwlock_write_lock(x)	clib_rwlock_writer_lock(x)
#define hylab_rwlock_write_unlock(x)	clib_rwlock_writer_unlock(x)
#define hylab_rwlock_init(x)	clib_rwlock_init(x)

#define DEFINE_RWLOCK(x) hylab_rwlock_t x = NULL;
#endif

#if 1
typedef rte_spinlock_t*  hylab_spinlock_t;
#define hylab_spinlock_lock(x)	rte_spinlock_lock(*(x))
#define hylab_spinlock_unlock(x)	rte_spinlock_unlock(*(x))
#define hylab_spinlock_init(x)	{*(x) = clib_mem_alloc_aligned (CLIB_CACHE_LINE_BYTES, CLIB_CACHE_LINE_BYTES);rte_spinlock_init(*(x));}
#define hylab_spinlock_init2(x,y)	{*(x) = y;rte_spinlock_init(*(x));}
#define HYLAB_SPINLOCK_INITIALIZER	RTE_SPINLOCK_INITIALIZER
#define DEFINE_SPINLOCK(x)	hylab_spinlock_t x = NULL/*HYLAB_SPINLOCK_INITIALIZER*/
#define HYLAB_SPINLOCK_SIZE	(sizeof(rte_spinlock_t))
#else
typedef clib_spinlock_t  hylab_spinlock_t;
#define hylab_spinlock_lock(x)	clib_spinlock_lock(x)
#define hylab_spinlock_unlock(x)	clib_spinlock_unlock(x)
#define hylab_spinlock_init(x)	clib_spinlock_init(x)
#define HYLAB_SPINLOCK_INITIALIZER	NULL
#define DEFINE_SPINLOCK(x)	hylab_spinlock_t x = HYLAB_SPINLOCK_INITIALIZER
#endif

#define DEFINE_MUTEX(x) DEFINE_SPINLOCK(x)
#define mutex_unlock(x) hylab_spinlock_unlock(x) 
#define mutex_lock(x) hylab_spinlock_lock(x) 
#endif
#endif

#pragma pack(push)
#pragma pack(4)

typedef char            S8;
typedef unsigned char   U8;
typedef short           S16;
typedef unsigned short  U16;
typedef int             S32;
typedef unsigned int    U32;
typedef int             STATUS;
typedef void            VOID;

#ifndef true
#define true    1
#endif

#ifndef false
#define false   0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef BOOL
#define BOOL    int
#endif

#ifndef DEFAULT_CONFIG_FILE

#ifndef OK
#define OK         0
#endif

#ifndef ERROR
#define ERROR      ( -1 )
#endif

#endif

#ifndef ACE_OK
#define ACE_OK        0
#endif

#ifndef ACE_ERROR
#define ACE_ERROR  ( -1 )
#endif

#define PHY_TRUST_NAME   "LAN"
#define PHY_UNTRUST_NAME "WAN"

#define LINUX_MY_IP6_ADDR   "\x02\x03\x04\x05\x00\x00\x00\x00\x00\x00\x00\x00\x02\x03\x04\x05"
#define VPP_MY_IP6_ADDR   "\x02\x03\x04\x05\x00\x00\x00\x00\x00\x00\x00\x00\x02\x03\x04\x06"

#define LINUX_MY_IP_HOST	(0x02030405)/**2.3.4.5**/
#define VPP_MY_IP_HOST	(0x02030406)/**2.3.4.6**/
#define WANGXING_OFFLINE_REQUEST "offline.ikuailian.com"

#ifndef NIPQUAD_FMT
#define NIPQUAD_FMT "%u.%u.%u.%u"
#define NIPQUAD(addr) \
 ((unsigned char *)addr)[0], \
 ((unsigned char *)addr)[1], \
 ((unsigned char *)addr)[2], \
 ((unsigned char *)addr)[3]
#endif

enum 
{
    SSO_H3CIMC_TYPE_DEFAULT                  = 0,
    SSO_H3CIMC_TYPE_WITH_RECORD_LOGIN        = 1,
    SSO_H3CIMC_TYPE_WITH_RECORD_RECORD       = 2,
    SSO_H3CIMC_TYPE_WITH_RECORD_HEARTBEAT    = 3,
    SSO_H3CIMC_TYPE_WITH_RECORD_LOGOUT       = 4,
};

#define ALG_SERVICE_NAME_FTP "FTP"
#define ALG_SERVICE_NAME_PPTP "PPTP"
#define ALG_SERVICE_NAME_RTSP "RTSP"
#define ALG_SERVICE_NAME_SIP "SIP"
#define ALG_SERVICE_NAME_H323 "H323"
#define ALG_CFG_PORT_LEN_MAX 48

#define ADDRBOOK_ANY_ADDRESS      "any_address"
#define ADDRBOOK_PRIVATE_ADDRESS  "private_address"


#define NMC_CTR_STATUS 0x01
#define NMC_CTR_ACEFW             (1 << 1)
#define NMC_CTR_PFC               (1 << 2)
#define NMC_CTR_UFC               (1 << 3)
#define NMC_CTR_POR               (1 << 4)
#define NMC_CTR_ACEFW_NMC_FIRST   (1 << 11)
#define NMC_CTR_PFC_NMC_FIRST     (1 << 12)
#define NMC_CTR_UFC_NMC_FIRST     (1 << 13)
#define NMC_CTR_POR_NMC_FIRST     (1 << 14)

#define NMC_CTR_AUDIT     			(1 << 15)
#define NMC_CTR_AUDIT_NMC_FIRST     (1 << 16)
#define NMC_CTR_AUDIT_LOCAL_FIRST     (1 << 17)

#define NMC_CTR_NETAREA			  		(1ULL << 18)
#define NMC_CTR_NETAREA_NMC_FIRST		(1ULL << 19)
#define NMC_CTR_NETAREA_LOCAL_FIRST	    (1ULL << 20)

#define NMC_CTR_ACEFW_LOCAL_FIRST (1 << 21)
#define NMC_CTR_PFC_LOCAL_FIRST   (1 << 22)
#define NMC_CTR_UFC_LOCAL_FIRST   (1 << 23)
#define NMC_CTR_POR_LOCAL_FIRST   (1 << 24)
#define NMC_CTR_IPS               (1 << 25)
#define NMC_CTR_IPS_NMC_FIRST     (1 << 26)
#define NMC_CTR_IPS_LOCAL_FIRST   (1 << 27)
#define NMC_CTR_AV                (1 << 28)
#define NMC_CTR_AV_NMC_FIRST      (1 << 29)
#define NMC_CTR_AV_LOCAL_FIRST    (1 << 30)

// in the higher 32 bit of g_nmc_flag
#define NMC_CTR_SESSION_CONTROL_LOCAL_FIRST     (1ULL << 32) // 1 << 0
#define NMC_CTR_SESSION_CONTROL_NMC_ONLY        (1ULL << 33) // 2 << 0
#define NMC_CTR_SESSION_CONTROL_NMC_FIRST       (1ULL << 34) // 3 << 0

#define NMC_CTR_NSAS_AUDIT (1ULL << 35)
#define NMC_CTR_NSAS_AUDIT_NMC_FIRST     (1ULL << 36)
#define NMC_CTR_NSAS_AUDIT_LOCAL_FIRST     (1ULL << 37)


#define FTP_FILE_DIR "/var/www/reporter/store/ftp_file"
#define NFS_FILE_DIR "/var/www/reporter/store/nfs_file"
#define SMB_FILE_DIR "/var/www/reporter/store/smb_file"
#define HTTP_FILE_DIR "/var/www/reporter/store/httpupload_file"


#define L7DPI_LIB_MAX_QUEUE 32

#define SHAREIP_CTR_OPSYS_WINDOWS 0x01
#define SHAREIP_CTR_OPSYS_ANDROID 0x02
#define SHAREIP_CTR_OPSYS_IOS 0x04
#define SHAREIP_CTR_MAC_CLASSIFY 0x08
#define SHAREIP_CTR_COOKIE 0x10
#define SHAREIP_CTR_PROXY 0x20
#define SHAREIP_CTR_OPSYS_NCSI 0x40
#define SHAREIP_CTR_NEED_DNS_ANALYISI 0x80

#if defined(ARCH_ARM64)
	#if defined(CONFIG_NSAS)
		#define FIRMWARE_DECODE_KEY "UOSNFirmwareOJJ*P1W2D3"
	#elif defined(CONFIG_DAS)
		#define FIRMWARE_DECODE_KEY "UOSNFirmwareJIO*P1W2D3"
    #elif defined(CONFIG_WEBFW)
        #define FIRMWARE_DECODE_KEY "UOSVPPWEBFWFirmware*P1W2D3"
	#elif defined(CONFIG_IFW)
		#define FIRMWARE_DECODE_KEY "UOSNFirmwareJOJ*P1W2D3"
	#elif defined(CONFIG_SIG)
		#define FIRMWARE_DECODE_KEY "UOSNFirmwareDON*P1W2D3"
	#elif defined(HYLAB_NAC_NEW)
		#define FIRMWARE_DECODE_KEY "UOSNFirmware*P1W2D3"
    #elif defined(CONFIG_HYTF_FW)
        #if defined(CONFIG_ZHGX)
            #define FIRMWARE_DECODE_KEY "zhgxxos2.0@PWD"
        #else
            #define FIRMWARE_DECODE_KEY "UOS-FW-Firmware*P1W2D3"
        #endif
    #else
        #define FIRMWARE_DECODE_KEY "UOSNFirmware*P1W2D3"
    #endif
#else
#ifndef OPENEULER
#define OPENEULER
#endif
	#if defined(CONFIG_NSAS)
		#define FIRMWARE_DECODE_KEY "VPPNSASFirmwareKHK*P1W2D3"
	#elif defined(CONFIG_DAS)
		#define FIRMWARE_DECODE_KEY "VPPDASFirmwareGYU*P1W2D3"
    #elif defined(CONFIG_WEBFW)
        #define FIRMWARE_DECODE_KEY "VPPWEBFWFirmware*P1W2D3"
	#elif defined(CONFIG_IFW)
		#define FIRMWARE_DECODE_KEY "VPPIFWFirmwareFTM*P1W2D3"
	#elif defined(CONFIG_SIG)
		#define FIRMWARE_DECODE_KEY "VPPSIGFirmwareVUR*P1W2D3"
	#elif defined(HYLAB_NAC_NEW)
		#define FIRMWARE_DECODE_KEY "VPPNACFirmware*P1W2D3"
	#elif defined(CONFIG_HYTF_FW)
        #if defined(CONFIG_ZHGX)
    	    #define FIRMWARE_DECODE_KEY "zhgxxos2.0@PWD"
        #else
    	#define FIRMWARE_DECODE_KEY "FWFirmware-openEuler*P1W2D3"
        #endif
    #elif defined(__RUIYUNTONG__)
        #define FIRMWARE_DECODE_KEY "RuiYunTong-openEuler*Huayu@123"
    #else
        #define FIRMWARE_DECODE_KEY "VPPNACFirmware*P1W2D3"
    #endif
#endif

#ifdef __ACCESS_SQL
#define HYLAB_MYSQL_PASSWORD "123456"
#else
#define HYLAB_MYSQL_PASSWORD "*^xP'&9V!3d"
#endif
#define HYLAB_PSQL_FLAGFILE "/etc/.pgsql"

#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
#define HYLAB_WEBPATCH_KEY "-----BEGIN PUBLIC KEY-----\r\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCH7DRaSazzSegUH/OUoODIQO3S\r\n" \
"kYN7UHxFBnKjUIG9fXEJ2BuaFmVpvnH+ov8ZZNBQI8MetelR3kKfmmN14+3JQjgt\r\n" \
"XVRA5XDk9//ObkvEgiTY8aHMMiHl/rMnVfbzSKfsr7TD+VXk2Sl93su/Ssj36pwG\r\n" \
"WR1lopu8TFX2fRi/5QIDAQAB\r\n" \
"-----END PUBLIC KEY-----"
#define HYLAB_WEBPATCH_URL "/api.php/inter/PatchCheck"
#elif defined(rzx)
#define HYLAB_WEBPATCH_KEY "-----BEGIN PUBLIC KEY-----\r\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCKZZj4FEIIP0v62bk+qGiCQQTk\r\n" \
"inawV10E00eUpJKNeIB9w82qUlMEYwXjFxb715qB7Zfen/4XFGRcJlYQPFPPaZQT\r\n" \
"qrvAw00nRHZ6CY7m7WlBatTDnTbnkRPnJbgtSy05FduTOzmHkDFfwbsmyGA+7IV0\r\n" \
"wBWyh30WG2Io9ES/cQIDAQAB\r\n" \
"-----END PUBLIC KEY-----"
#define HYLAB_WEBPATCH_URL "/api.php/inter/PatchCheck"
#elif defined(zhongxinwa)
#define HYLAB_WEBPATCH_KEY "-----BEGIN PUBLIC KEY-----\r\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCyZtHqIpRnOzJcCsbLaIUcw5JG\r\n" \
"4WOCbuBVYvhQH59BnEA7wU7FiPLTTv5HfdHpB8T2q4JBRiyGTe1QMHVU21KkDEq/\r\n" \
"zCzpChfjNSRLA4KUhWN52OSCEJhlYuDaGkXKA4yLNzRF0PW4ViNooE+TyrQtKzps\r\n" \
"4uhX1oWUjbSxUViUlwIDAQAB\r\n" \
"-----END PUBLIC KEY-----"
#define HYLAB_WEBPATCH_URL "/api.php/inter/PatchCheck"
#else
#define HYLAB_WEBPATCH_KEY "-----BEGIN PUBLIC KEY-----\r\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDqsxLUO8kfXKauf70ODrDXush5\r\n" \
"dRpotX5jP/JZa6mWlVrHtlluB2iIvGdTR24+6em3t0mEQa8YX8XznexytCO3UHIp\r\n" \
"F4zGFcKEwOrF9wUClJC/dCMr0opHPFbFhit/xAkXYKI2DPDBwMydO1QPfftopgkh\r\n" \
"Veyva7nwt4sqeJI86QIDAQAB\r\n" \
"-----END PUBLIC KEY-----"
#define HYLAB_WEBPATCH_URL "/api.php/inter/PatchCheck"
#endif

#if defined(topsec) || defined(topsec_tf) || defined(topsec_dw) || defined(topsec82) || defined(topsec_auth_dw)
#define HYLAB_LIB_ADDRESS_DEFAULT "acm.topsec.com.cn"
#elif defined(rzx)
#define HYLAB_LIB_ADDRESS_DEFAULT "rag.surfilter.com"
#elif defined(zhongxinwa)
#define HYLAB_LIB_ADDRESS_DEFAULT "upgrade.cicisp.com:9092"
#elif defined(uguard)
#define HYLAB_LIB_ADDRESS_DEFAULT "signature.uguardnetworks.com"
#else
#define HYLAB_LIB_ADDRESS_DEFAULT "lib.rapidupdate.net"
#endif


#define PAGE_OPENSSL_KEY "PageAuthReporter*P1W2D3"

#define HYLAB_REPORTER_LOGIN_PASSWORD "Login*PWD"
#define HYLAB_SSHPASS_PASSWORD "admin*PWD"
#define HYLAB_WEB_ENCRYPT_PASSWORD "asdfAbcdefg23L@#Ssdfadmin*PWDa2#@#$%d123456$%)(!"
#define HYLAB_WEB_DEBUG_PASSWORD "H~iv^n?T?*QjpA*a"
#define HYLAB_WEB_SINGLE_LOGIN_PASSWORD "admin*PWD"

#if defined(changting) || defined(changtingV2) || defined(uguard)
#define HYLAB_TI_ENCRYPT_PASSWD "i_B73ia4!XHTnrZ3"
#else
#define HYLAB_TI_ENCRYPT_PASSWD "Ti_L1b@Hytf"
#endif

#define IF_PPTP_VPN_NAME "PPTP-VPN"
#define IF_L2TP_VPN_NAME "L2TP-VPN"
#define IF_SSL_VPN_NAME "SSL-VPN"
#define IF_IPSEC_VPN_NAME "IPSEC-VPN"

#define ZONE_VPN_NAME "VPN"
#define ZONE_TAP_NAME "TAP"
#define ZONE_MGT_NAME "*@MGT"
#define ZONE_L2_TRUST_NAME "L2trust"
#define ZONE_L2_UNTRUST_NAME "L2untrust"
#define ZONE_L3_TRUST_NAME "trust"
#define ZONE_L3_UNTRUST_NAME "untrust"
#define ZONE_L3_DMZ_NAME "dmz"


#ifdef CONFIG_SIG
#define SIG_SDK_GEO_FILE_PATH	"/mnt/sdk/geo/geo.mmdb"
#define SIG_SDK_MDBX_FILE_PATH	"/mnt/sdk/mdbx/mdbx.db"
#define SIG_SDK_BLKLIST_FILE_PATH	"/mnt/sdk/geo/geo.mmdb"
#define SIG_SDK_BLOOM_FILE_PATH	"/mnt/sdk/bloom/bloom.bin"
#define SIG_SDK_IP_BLACK_PATH	"/mnt/sdk/ip_black/ip.txt"
#endif

#define HYLAB_FIREWALL_NEW_IFS_LEN 1024
#define HYLAB_FIREWALL_IF_NUM_MAX 32

#define HYTF_CONNECTION_AGEOUT_SYNC_MAX        256

enum {
	HYLAB_AUTH_METHOD_IP = 0,            /*新用户以IP地址作为用户名*/
	HYLAB_AUTH_METHOD_MAC = 1,           /*新用户以MAC地址作为用户名*/
	HYLAB_AUTH_METHOD_HOSTNAME = 2,      /*新用户以主机名作为用户名*/
	HYLAB_AUTH_METHOD_PWD = 3,           /*密码认证*/
	HYLAB_AUTH_METHOD_VLAN = 4,          /*新用户以 VLAN ID 作为用户名*/
	
	HYLAB_AUTH_METHOD_SSO = 5,           /*新用户以 SSO获取值作为用户名*/
	HYLAB_AUTH_METHOD_SMS = 6,           /*短信认证*/
	HYLAB_AUTH_METHOD_PWD_SMS = 7,       /*密码短信认证*/
    HYLAB_AUTH_METHOD_WECHAT = 8,        /*微信认证*/
    HYLAB_AUTH_METHOD_WECHAT_PWD = 9,    /*废弃*/
    
    HYLAB_AUTH_METHOD_WECHAT_SMS = 10,   /*废弃*/
    HYLAB_AUTH_METHOD_AD = 11,           /* AD集成windows身份验证*/
    HYLAB_AUTH_METHOD_NOPWD = 12,        /* 无密码认证*/
    HYLAB_AUTH_METHOD_GROUP = 13,        /* 用户组权限认证*/
    HYLAB_AUTH_METHOD_SMP = 14,          /*SMP认证*/

    HYLAB_AUTH_METHOD_WECHAT1 = 15,      /*废弃*/
    HYLAB_AUTH_METHOD_REFUSE = 17,       /*禁止上网*/
    HYLAB_AUTH_METHOD_AWIFI = 18,        /*AWIFI认证*/
    HYLAB_AUTH_METHOD_CARD = 19,         /*智能卡认证*/
    
    HYLAB_AUTH_METHOD_VISITOR = 20,      /*访客认证*/
    HYLAB_AUTH_METHOD_SSO_POATAL = 21,   /*单点登录失败重定向URL*/
    HYLAB_AUTH_METHOD_DINGDING = 22,     /*钉钉认证*/
    HYLAB_AUTH_METHOD_ENTERPRISE_WECHAT = 23,     /*企业微信认证*/
    HYLAB_AUTH_METHOD_MULTIPLE = 24,     /*综合认证*/
    HYLAB_AUTH_METHOD_PORTAL = 25,       /*PORTAL认证*/
};


//MULTIPLE TYPE
enum {
    HYLAB_AUTH_MMT_PWD = 1,
    HYLAB_AUTH_MMT_SMS,
    HYLAB_AUTH_MMT_WECHAT,
    HYLAB_AUTH_MMT_VISITOR,
    
    HYLAB_AUTH_MMT_DINGDING,
    HYLAB_AUTH_MMT_ENTERPRISE_WECHAT,
    HYLAB_AUTH_MMT_GMAIL,
    HYLAB_AUTH_MMT_MINIPROGRAM,
    HYLAB_AUTH_MMT_OAUTH1,
    
    HYLAB_AUTH_MMT_OAUTH2,
    HYLAB_AUTH_MMT_OAUTH3,
    HYLAB_AUTH_MMT_OAUTH4,
    HYLAB_AUTH_MMT_CAS1,
    
    HYLAB_AUTH_MMT_CAS2,
    HYLAB_AUTH_MMT_CAS3,
    HYLAB_AUTH_MMT_CAS4,
    HYLAB_AUTH_MMT_MAX,
};

#define HYLAB_AUTH_MMT_MAP_CHECK_TRUE(_method, _map, _type) ((_method == HYLAB_AUTH_METHOD_MULTIPLE) && (_map & (_type)))
#define HYLAB_AUTH_MMT_MAP_CHECK_FALSE(_method, _map, _type) ((_method != HYLAB_AUTH_METHOD_MULTIPLE) || !(_map & (_type)))
#define HYLAB_AUTM_MTT_MAP_SET(_map, _type) (_map |= (1 << _type))

enum _ace_auth_server_type
{
    ACE_AUTH_NONE = 0,
    ACE_AUTH_LOCAL,
    ACE_AUTH_RADIUS,
    ACE_AUTH_LDAP,
    ACE_AUTH_POP3,
    
    ACE_AUTH_AD = 5,
    ACE_AUTH_HENGBANG,
    ACE_AUTH_PAIBO,
    ACE_AUTH_ZHIAN,
    ACE_AUTH_PORTAL,
    
    ACE_AUTH_DATABASE = 10,
    ACE_AUTH_SMP,
};

enum _ace_auth_error_type
{
    ACE_AUTH_ERR_ING = 0,                /*认证中*/
    ACE_AUTH_ERR_NEED_AUTH = 3,          /*需要去服务器认证*/
    ACE_AUTH_ERR_FREEZE = 4,             /*用户被冻结，不能上网*/
    ACE_AUTH_ERR_AUTH_FAIL = 5,          /*该用户为认证用户，但还未通过认证*/
    ACE_AUTH_ERR_IP_BIND = 6,            /*IP 绑定检查失败*/
    ACE_AUTH_ERR_MAC_BIND = 7,           /*MAC 绑定检查失败*/
    ACE_AUTH_ERR_IP_MAC_BIND = 8,        /*同时绑定 IP 和 MAC 检查失败*/
    ACE_AUTH_ERR_IP_MAC_CONFLICT = 9,    /*找不到用户名，IP 绑定在一个用户，MAC 绑定在另一个用户*/
    ACE_AUTH_ERR_MAC = 10,               /*跨三层识别错误*/
    ACE_AUTH_ERR_WLAN_STATION = 14,      /*移动终端管理禁止上网*/
    ACE_AUTH_ERR_REFUSE = 15,            /*禁止上网*/
    ACE_AUTH_ERR_REFUSE_ALL = 16,        /*禁止上网包括ping和dns*/
    ACE_AUTH_ERR_PORTAL_REFUSE = 17,     /*Portal逃生禁止上网*/
    ACE_AUTH_ERR_PORTAL_PASS = 18,       /*Portal逃生禁止运行上网*/
    ACE_AUTH_ERR_KICK_OFF = 19,          /*kick off*/
};



enum snmptrap_type 
{
    SNMPTRAP_COLD_START = 0,
    SNMPTRAP_LINK_DOWN = 2,
    SNMPTRAP_LINK_UP = 3,

    SNMPTRAP_EVENT = 7,
    SNMPTRAP_SECURE = 8,
    SNMPTRAP_MINING = 9,
    SNMPTRAP_VPN_ABROAD = 10
};


/*from ace_error_message.h*/
enum 
{
    /*0-4*/
    SYSTEM_CONFIG   = 100,
    NETWORK_CONFIG  = 200,
    FIREWALL_CONFIG = 300,
    PAT_CONFIG      = 400,
    TRAFFIC_CONFIG  = 500,

    /*5-9*/
    USER_CONFIG     = 600,
    USER_CONFIG2     = 800,
    REPORTER_CONFIG = 900,
    SYSLOG_CONFIG   = 1000,

    /*10-14*/
    DEBUG_INFO      = 1100,
    AUTH_INFO   = 1000,
    IPS_CONFIG      = 2810,  // who used error2800-->2806??? skip that range
    AV_CONFIG       = 2900,
    AD_CONFIG       = 3000,
    WAF_CONFIG      = 3100,
    ATBL_CONFIG     = 3190,
	PROUTE_CONFIG   = 3200,
	LINK_OBJ_CONFIG = 3300,
	LOCATION_ERR_INFO = 3400,
	HYTF_MIRROR_POLICY = 3500,
	HYTF_SMP_POLICY = 3600,
	HYTF_EDR = 3700,
    TI_CONFIG = 3800,
    IOP_CONFIG = 3900,
    RA_CONFIG = 4000,
    DNS_TPROXY_CONFIG = 4100,
    NETWORK_ZONE_CONFIG = 4200,
    COMM_ERR_OTHER = 4300,
    BLKLIST_FILTER_ERR = 4400,
    
    FW_SEPERATE_POLICY_ERR = 4500,
    SSLVPN_CONFIG_ERR = 4700,//keep 4700~4900 for sslvpn
    
    INDUSTRIAL_CONTROL_ERR = 5000,
    BFD_CONFIG_ERR = 5100,
    SSL_PROXY_LIB_ERR = 5200,
    
    HYNAD_ERR = 5300,
    ACE_MROUTE = 5400,
    SERVICE_GROUP = 5500,

	//#if defined(CONFIG_NSAS)
    NSAS_FILTER_CONFIG_ERR = 5600,
	//#endif
	DAS_OBJECT_CONFIG_ERR = 5700,
	SIG_CONFIG_ERR = 5800,
	SIG_GEN_ERR = 5850,
    ILL_OUTREACH_JS_ERR = 6000,
};

enum _module_error_numer
{
    /*SYSTEM config module*/
    SYSTEM_BASE_ERROR_ID = SYSTEM_CONFIG,

    /*NETWORK config module*/
    NETWORK_BASE_ERROR_ID = NETWORK_CONFIG,
    SAFETY_ZONE_PARAM,
    SAFETY_ZONE_NAME_NULL,
    SAFETY_ZONE_NAME_USED,
    SAFETY_ZONE_MEM_FAIL,
    
    SAFETY_ZONE_NOT_FOUND,    
    SAFETY_ZONE_IF_NOT_FOUND,
    SAFETY_ZONE_USED_BY_FW_POLICY,
    SAFETY_ZONE_USED_BY_IF,
    SAFETY_ZONE_USED_BY_POLICY_ROUTE,
    
    SAFETY_ZONE_USED_BY_AD,   /*210*/ 
    NET_IF_PARAM,
    NET_IF_NAME_NULL,
    NET_IF_NOT_FOUND,
    NET_IF_EXIST,
    
    NET_IF_MEM_FAIL,
    NET_IF_DEV_NOT_FOUND,
    NET_IF_FAMILY_WRONG,
    NET_BR_EXIST,
    NET_BR_MEM_FAIL,
    
    NET_BR_DEV_FAIL,    /*220*/ 
    NET_BR_NOT_FOUND,
    NET_IF_CAR_OVERFLOW,
    NET_IF_CAR_NOT_FOUND,
    SAFETY_ZONE_OVERFLOW,
    
    NET_IF_CAR_USED_BY_PFC,/*225*/ 
    NET_BR_IF_WORKMODE_WRONG,
    NET_PHY_IF_USED_BY_VLAN,
    NET_PHY_IF_USED_BY_PROUTE,
    NET_PHY_IF_ERROR_LINK_OBJ,

	NET_BR_VIRTUAL = 240,
	NET_BR_IF_OVERFLOW = 241,
    

    /*FIREWALL config module*/
    FIREWALL_BASE_ERROR_ID = FIREWALL_CONFIG,
    FIREWALL_SYSTEM_FAIL,
    FIREWALL_MEMORY_FAIL,
    FIREWALL_ADDR_NO_NAME,
    FIREWALL_ADDR_NO_HOST,

    FIREWALL_ADDR_HOST_FORMAT,
    FIREWALL_ADDR_NAME_USED,
    FIREWALL_ADDR_NAME_NOT_FOUND,
    FIREWALL_ADDR_ITEM_NOT_FOUND,
    FIREWALL_ADDR_USED_RULE,

    FIREWALL_ADDR_USED_TRAFFIC_RULE, /* 310 */
    FIREWALL_ADDR_USED_IP_TRAFFIC_RULE,
    FIREWALL_ADDR_USED_AUTH_RULE,
    FIREWALL_ADDR_USED_BEHAVIOR_RULE,
    FIREWALL_ADDR_USED_PAT,

    FIREWALL_SVC_NO_NAME,
    FIREWALL_SVC_NAME_CONFLICT,
    FIREWALL_SVC_PROTOCOL,
    FIREWALL_SVC_PORT,
    FIREWALL_SVC_NAME_USED,

    FIREWALL_SVC_NAME_NOT_FOUND,    /* 320 */
    FIREWALL_SVC_ITEM_NOT_FOUND,
    FIREWALL_SVC_USED_RULE,
    FIREWALL_SVC_USED_PAT,   
    FIREWALL_SVC_USED_TRAFFIC_RULE,

    FIREWALL_SVC_USED_IP_TRAFFIC_RULE,
    FIREWALL_RULE_NO_SERVICE,
    FIREWALL_RULE_SERVICE,
    FIREWALL_RULE_NOT_FOUND,
    FIREWALL_RULE_MODE_ERROR,

    FIREWALL_RULE_SCHEDULE,        /* 330 */
    FIREWALL_RULE_HOST_FORMAT,
    FIREWALL_RULE_HOST_ADDRBOOK,
    FIREWALL_RULE_ADDRESS_MODE,
    FIREWALL_RULE_DELETE_FAIL,

    FIREWALL_RULE_ADD_FAIL,
    WHITELIST_SYSTEM_FAIL,
    WHITELIST_HOST_FORMAT,
    WHITELIST_NOT_FOUND,
    FIREWALL_IPSEC_NO_SERVICE,

    FIREWALL_IPSEC_HOST_FORMAT,      /* 340 */
    FIREWALL_IPSEC_HOST_ADDRBOOK,
    FIREWALL_IPSEC_ADDRESS_MODE,
    FIREWALL_IPSEC_SERVICE,
    FIREWALL_SVC_USED_IPSEC,
    
    FIREWALL_ADDR_USED_IPSEC,
    FIREWALL_SVC_TOO_MANY,
    FIREWALL_SVC_USED_FC,
    FIREWALL_SVC_USED_UC,
    FIREWALL_SVC_USED_POLICY_OBJECT_KERNEL,

    FIREWALL_ADDR_USED_POLICY_OBJECT_KERNEL,  /* 350 */
    FIREWALL_RULE_EXIST,
    FIREWALL_SVC_USED_WHITELIST,  
	FIREWALL_ADDR_USED_NETWORK_ZONE,
    FIREWALL_ADDR_USED_BY_AD,

    FIREWALL_ADDR_USED_BY_PROUTE, //355
    FIREWALL_ADDR_USED_BY_IPSEC,
    FIREWALL_DOAMIN_GORUP_USED_RULE,
    FIREWALL_RULE_HOST_DOAMIN_GROUP,
    FIREWALL_ADDR_USED_BY_SERVICE_CONTROL,
    FIREWALL_ADDR_USED_BY_AV,//360
    FIREWALL_ADDR_USED_BY_IPS,//361
    FIREWALL_ADDR_USED_BY_WAF,//362
    FIREWALL_ADDR_USED_BY_TI,//363

	FIREWALL_INTERFACE_USED_BY_FW,
	FIREWALL_ADDR_USED_BY_DAS = 366,//366
    
    /*PAT config module*/
    PAT_BASE_ERROR_ID = PAT_CONFIG,
    PAT_SYSTEM_FAIL,
    PAT_MEMORY_FAIL,
    PAT_RULE_NOT_FOUND,
    PAT_RULE_NO_SERVICE,

    PAT_RULE_SERVICE,
    PAT_RULE_MODE_ERROR,
    PAT_RULE_HOST_FORMAT,
    PAT_RULE_HOST_ADDRBOOK,
    PAT_RULE_ADDRESS_MODE,

    PAT_RULE_CONVERT_IP,  /* 410 */
    PAT_RULE_DELETE_FAIL,
    PAT_RULE_ADD_FAIL,    
    PAT_STATIC_NOT_FOUND,
    PAT_STATIC_NAT_ADDRESS,

    PAT_STATIC_HOST_ADDRESS,
    PAT_STATIC_NAT_HOST_MISMATCH,
    PAT_SVCMAP_NOT_FOUND,
    PAT_SVCMAP_NEW_PORT,
    PAT_SVCMAP_HOST_PORT,

    PAT_SVCMAP_PROTOCOL, /* 420 */
    PAT_SVCMAP_NEW_HOST_PORT,
    PAT_SVCMAP_EXIST,
    PAT_STATIC_NAT_EXIST,
    PAT_PAT_EXIST,
    
    PAT_INSERT_NOT_FOUND,
    PAT_MOVE_NOT_FOUND,
    NAT_RULE_PARAM,
    NAT_RULE_EXIST,
    NAT_RULE_NOT_FOUND,
    
    NAT_RULE_MEM_FAILED, /* 430 */
    NAT_RULE_IP_TYPE,
    NAT_RULE_IP_FORMAT,
    NAT_RULE_IP_GROUP,
    NAT_RULE_PORT_FORMAT,
    
    NAT_RULE_CONVERT_IP_OVERFLOW,

	PAT_SRC_IF_ZONE_NOT_SUPPORT = 450,
	PAT_DST_IF_ZONE_NOT_SUPPORT,

    /*TRAFFIC config module*/
    TRAFFIC_BASE_ERROR_ID = TRAFFIC_CONFIG,
    TRAFFIC_SYSTEM_FAIL,
    TRAFFIC_MEMORY_FAIL,
    TRAFFIC_LINE_ERROR,
    TRAFFIC_NAME_USED,

    TRAFFIC_SCHEDULE,
    TRAFFIC_HOST_FORMAT,
    TRAFFIC_ADDRESSBOOK,
    TRAFFIC_ADDRESS_MODE,
    TRAFFIC_NOT_FOUND,

    TRAFFIC_INSERT_NOT_FOUND,
    TRAFFIC_MOVE_NOT_FOUND,
    TRAFFIC_USER_NAME_USED,
    TRAFFIC_USER_SCHEDULE,
    TRAFFIC_USER_HOST_FORMAT,
    
    TRAFFIC_USER_ADDRESSBOOK,
    TRAFFIC_USER_ADDRESS_MODE,
    TRAFFIC_USER_NOT_FOUND,
    TRAFFIC_USER_INSERT_NOT_FOUND,
    TRAFFIC_USER_MOVE_NOT_FOUND,    

    POLICY_OBJECT_KERNEL_NO_NAME, /* 520 */
    POLICY_OBJECT_KERNEL_SYSTEM_FAIL,
    POLICY_OBJECT_KERNEL_NAME_USED,
    POLICY_OBJECT_KERNEL_SCHEDULE,
    POLICY_OBJECT_HOST_FORMAT,

    POLICY_OBJECT_ADDRESSBOOK,
    POLICY_OBJECT_KERNEL_SERVICE,
    POLICY_OBJECT_KERNEL_NOT_FOUND,
    TRAFFIC_MASTER,
    TRAFFIC_SERVICE_,

    /*USER config module*/
    USER_BASE_ERROR_ID = USER_CONFIG,
    USER_SYSTEM_FAIL,
    USER_MEMORY_FAIL,
    USER_POLICY_NO_INPUT,
    USER_POLICY_MODE_ERROR,

    USER_POLICY_SCHEDULE,  /*605 */
    USER_POLICY_HOST_FORMAT,
    USER_POLICY_ADDRESSBOOK,
    USER_POLICY_ADDRESS_MODE,
    USER_POLICY_RADIUS,

    USER_POLICY_LDAP,  /*610 */
    USER_POLICY_AD,
    USER_POLICY_POP3,
    USER_POLICY_AUTH_SERVER,
    USER_POLICY_GET_ERROR,

    USER_POLICY_DELETE_FAIL, /*615*/
    USER_POLICY_ADD_FAIL,
    USER_RADIUS_NO_NAME,
    USER_RADIUS_NO_IP,
    USER_RADIUS_NO_PORT,

    USER_RADIUS_NO_SECRET, /*620*/
    USER_RADIUS_GET_ERROR,
    USER_RADIUS_USED,
    USER_LDAP_NO_NAME,
    USER_LDAP_NO_IP,

    USER_LDAP_NO_PORT, /*625*/
    USER_LDAP_NO_DN,
    USER_LDAP_NO_CN,
    USER_LDAP_GET_ERROR,
    USER_LDAP_USED,

    USER_POP3_NO_NAME, /*630*/
    USER_POP3_NO_DNS,
    USER_POP3_GET_ERROR,
    USER_POP3_USED,    
    USER_AD_NO_NAME,

    USER_AD_NO_IP,  /*635*/
    USER_AD_NO_BASE_DN,
    USER_AD_NO_SEARCH_NAME,
    USER_AD_NO_SEARCH_PWD,
    USER_AD_GET_ERROR,

    USER_AD_USED, /*640*/
    USER_LOCAL_NO_NAME,
    USER_LOCAL_NO_PWD,
    USER_LOCAL_NO_GROUP,
    USER_LOCAL_NOT_FOUND,

    USER_SCHEDULE_NO_NAME, /*645*/
    USER_SCHEDULE_NOT_FOUND,
    USER_SCHEDULE_USED_RULE,
    USER_SCHEDULE_USED_TRAFFIC_RULE,
    USER_SCHEDULE_USED_IP_TRAFFIC_RULE,
    
    USER_SCHEDULE_USED_AUTH_RULE, /*650*/
    USER_SCHEDULE_USED_BEHAVIOR_RULE,
    USER_GROUP_NO_NAME,
    USER_GROUP_NOT_FOUND,
    USER_GROUP_USED_RULE,
    
    USER_GROUP_USED_TRAFFIC_RULE,  /*655*/
    USER_GROUP_USED_IP_TRAFFIC_RULE,
    USER_GROUP_USED_BEHAVIOR_RULE,
    USER_GROUP_USED_USER,
    USER_SCHEDULE_USED, 
    
    USER_GROUP_USED, /*660*/
    USER_SCHEDULE_NAME_USED,
    USER_RADIUS_NAME_USED,
    USER_LDAP_NAME_USED,
    USER_POP3_NAME_USED,
    
    USER_AD_NAME_USED, /*665*/
    USER_GROUP_NAME_USED,
    USER_LOCAL_NAME_USED,
    USER_IPUSERMAPPING_NO_NAME,     
    USER_IPUSERMAPPING_NO_GROUP,    

    USER_IPUSERMAPPING_NO_IP,   /*670 */
    USER_IPUSERMAPPING_IP_USED,
    USER_IPUSERMAPPING_NOT_FOUND,
    USER_IPUSERMAPPING_GROUP_USED,
    USER_IPUSERMAPPING_GROUP_NOTEXIST,

    USER_BLACKLIST_NO_NAME, /*675 */
    USER_BLACKLIST_NOT_FOUND,
    USER_POLICY_NO_NAME,
    USER_POLICY_NO_GROUP,
    USER_POLICY_EXIST,

    USER_MANUAL_BLACKLIST_NOT_FOUND, /*680 */
    USER_MANUAL_BLACKLIST_NO_NAME,
    USER_MANAGE_FRAME_BIND_TEXT_ERROR,
    USER_MANAGE_FRAME_MODIFY_USER_GROUP_ERROR,
    USER_BLACKLIST_USED,

    USER_SCHEDULE_USED_POLICY_OBJECT_KERNEL, /* 685 */
    USER_GROUP_USED_AUTH_POLICY,    
    USER_MANAGE_FRAME_BIND_CHECK_ERROR,
    USER_BLACKLIST_SCHEDULE,
    USER_CURRENT_BLACKLIST_NO_NAME,

    USER_CURRENT_BLACKLIST_NOT_FOUND, /* 690 */
    USER_MANAGEFRAME_SYSTEM_FAIL,
    USER_MANAGEFRAME_PATHNAME_TOO_LONG,
    USER_MANAGEFRAME_PARENT_NOT_FOUND,
    USER_MANAGEFRAME_SAME_NAME,

    USER_SMP_NO_NAME = 695, /* 695 */
    USER_SMP_NO_IP,
    USER_SMP_NO_URL,
    USER_SMP_NO_PWD,
    USER_SMP_USED,
    
    USER_RADIUS_USED_BY_OPENVPN = 700, /* 700 */

	USER_PORTAL_IPS_TRACK_ERR = 701,
	ISP_TRACK_IS_USED_BY_PORTAL = 702,

    /*700-715:mail error:*/
    USER_HENGBANG_NO_NAME = 720,
    USER_HENGBANG_NO_IP,
    USER_HENGBANG_NO_PORT,
    USER_HENGBANG_GET_ERROR,
    USER_HENGBANG_USED,
    
    USER_HENGBANG_NAME_USED, /*725*/
    USER_POLICY_HENGBANG,
    USER_PAIBO_NO_NAME,
    USER_PAIBO_NO_IP,
    USER_PAIBO_NO_PORT,
    
    USER_PAIBO_GET_ERROR, /*730*/
    USER_PAIBO_USED,
    USER_PAIBO_NAME_USED,
    USER_POLICY_PAIBO,
    USER_ZHIAN_NO_NAME,

    USER_ZHIAN_GET_ERROR, /*735*/
    USER_ZHIAN_NAME_USED,
    USER_ZHIAN_USED,
    USER_BLACKLIST_NAME_EXIST,
    USER_BLACKLIST_RULE_MAX_NUMBER,
    
    USER_BLACKLIST_RULE_HOST_FORMAT, /*740*/
    USER_SCHEDULE_USED_BLACKLIST_RULE,
    USER_SCHEDULE_USED_ACCESS_RULE,
    USER_PORTAL_SERVER_NO_NAME,
    USER_PORTAL_SERVER_NO_IP,

    USER_PORTAL_SERVER_NO_PORT, /*745*/
    USER_PORTAL_SERVER_GET_ERROR,
    USER_PORTAL_SERVER_USED,
    USER_PORTAL_SERVER_NAME_USED,
    USER_POLICY_PORTAL_SERVER,

    IMCTL_OBJECT_SYSTEM_FAIL = 750,
    IMCTL_OBJECT_NAME_USED,
    IMCTL_OBJECT_SCHEDULE_NOT_FOUND,
    IMCTL_OBJECT_NOT_FOUND,
    P3_OBJECT_SYSTEM_FAIL,
    
    P3_OBJECT_NAME_USED,
    P3_OBJECT_SCHEDULE_NOT_FOUND,
    P3_OBJECT_NOT_FOUND,
    POLICY_OBJECT_IS_USED,
    KEYWORD_OBJECT_IS_USED,
    
    KEYWORD_OBJECT_SYSTEM_FAIL, /*760*/
    KEYWORD_OBJECT_NAME_USED,
    KEYWORD_OBJECT_STRING_FORMAT_ERROR,
    KEYWORD_OBJECT_MAX_NUMBER,
    KEYWORD_OBJECT_NOT_FOUND,

    KEYWORD_OBJECT_CLEAR_USED,
    FILEFILTER_OBJECT_IS_USED,  
    FILEFILTER_OBJECT_SYSTEM_FAIL,
    FILEFILTER_OBJECT_NAME_USED,
    FILEFILTER_OBJECT_STRING_FORMAT_ERROR,

    FILEFILTER_OBJECT_MAX_NUMBER, /*770*/
    FILEFILTER_OBJECT_NOT_FOUND,
    FILEFILTER_OBJECT_CLEAR_USED,
    URLCUSTOM_OBJECT_IS_USED,   
    URLCUSTOM_OBJECT_SYSTEM_FAIL,

    URLCUSTOM_OBJECT_NAME_USED,
    URLCUSTOM_OBJECT_STRING_FORMAT_ERROR,
    URLCUSTOM_OBJECT_MAX_NUMBER,
    URLCUSTOM_OBJECT_NOT_FOUND,
    URLCUSTOM_OBJECT_CLEAR_USED,

    WHITENAME_OBJECT_IS_USED,   /*780*/
    WHITENAME_OBJECT_SYSTEM_FAIL,
    WHITENAME_OBJECT_NAME_USED,
    WHITENAME_OBJECT_STRING_FORMAT_ERROR,
    WHITENAME_OBJECT_MAX_NUMBER,

    WHITENAME_OBJECT_NOT_FOUND,
    WHITENAME_OBJECT_CLEAR_USED,
    POLICY_OBJECT_SYSTEM_FAIL, 
    POLICY_OBJECT_NAME_USED,
    POLICY_OBJECT_IM_SCHEDULE_NOT_FOUND,
    
    POLICY_OBJECT_MAX_NUMBER,       /*790*/
    POLICY_OBJECT_NOT_FOUND,
    ISP_TRACK_IS_USED,  
    ISP_TRACK_SYSTEM_FAIL, 
    ISP_TRACK_NAME_USED,
    
    ISP_TRACK_STRING_FORMAT_ERROR,
    ISP_TRACK_OVER_MAX_NUMBER,
    ISP_TRACK_NOT_FOUND,    
    ISP_TRACK_CLEAR_USED,
    ISP_TRACK_SUBNET_FORMAT_ERROR,

    /*user config module2*/
    USER_CONFIG2_ERROR_ID = USER_CONFIG2,
    USER_MANAGEFRAME_NOT_FOUND,
    USER_MANAGEFRAME_ROOT,
    USER_MANAGEFRAME_TYPE,    
    USER_MANAGEFRAME_SYSTEM_FAIL1,  
    
    USER_MANAGEFRAME_SYSTEM_FAIL2,    
    USER_MANAGEFRAME_SYSTEM_FAIL3,


    WECHAT_SERVER_NO_NAME = 820,
    WECHAT_SERVER_GET_ERROR,
    WECHAT_SERVER_USED,
    WECHAT_SERVER_NAME_USED,

    USER_SMP_GET_ERROR = 830,
    USER_POLICY_SMP,
    USER_POLICY_WECHAT,
    USER_POLICY_LOCATION_ERROR,

	USER_AD_USED_BY_ADSSO = 840,

    URLCUSTOM_OBJECT_USED_BY_SECURITY_PROTECT_POLICY = 841,
    FILEFILTER_OBJECT_USED_BY_SECURITY_PROTECT_POLICY,  
    FILEFILTER_OBJECT_USED_BY_AV_TEMPLATE,  

    /*im whitelist*/
    IM_WHITELIST_RULE_SYSTEM_FAIL = 850,
    IM_WHITELIST_RULE_NAME_USED,
    IM_WHITELIST_RULE_STRING_FORMAT_ERROR,
    IM_WHITELIST_RULE_MAX_NUMBER,
    IM_WHITELIST_RULE_NOT_FOUND,
    IM_WHITELIST_RULE_HOST_FORMAT,
    USER_SCHEDULE_USED_IM_WHITELIST_RULE,

    /*url whitelist*/
    URL_WHITELIST_RULE_SYSTEM_FAIL = 860,
    URL_WHITELIST_RULE_NAME_USED,
    URL_WHITELIST_RULE_STRING_FORMAT_ERROR,
    URL_WHITELIST_RULE_MAX_NUMBER,
    URL_WHITELIST_RULE_NOT_FOUND,
    URL_WHITELIST_RULE_HOST_FORMAT,
    USER_SCHEDULE_USED_URL_WHITELIST_RULE,

    WHITELIST_RULE_HOST_FORMAT,
    
    /*user databse ser config */
    USER_POLICY_DATABASE = 870,
    USER_DATABASE_NO_NAME = 880,
    USER_DATABASE_NO_DBADDRESS,
    USER_DATABASE_NO_DBUNAME,
    USER_DATABASE_NO_DBASENAME,
    USER_DATABASE_GET_ERROR,
    USER_DATABASE_NAME_USED,

    /*global whitelist*/
    GLOBAL_WHITELIST_RULE_SYSTEM_FAIL = 890,
    GLOBAL_WHITELIST_RULE_NAME_USED,
    GLOBAL_WHITELIST_RULE_STRING_FORMAT_ERROR,
    GLOBAL_WHITELIST_RULE_MAX_NUMBER,
    GLOBAL_WHITELIST_RULE_NOT_FOUND,
    GLOBAL_WHITELIST_RULE_HOST_FORMAT,
    
    /*REPORTER config module*/
    REPORTER_BASE_ERROR_ID = REPORTER_CONFIG,
    AUDIT_RULE_NO_INPUT,
    AUDIT_RULE_MODE_ERROR,
    AUDIT_RULE_HOST_FORMAT,
    AUDIT_RULE_ADDRESSBOOK,
    AUDIT_RULE_ADDRESS_MODE,
    AUDIT_RULE_GET_ERROR,
    AUDIT_RULE_DELETE_FAIL,
    AUDIT_RULE_ADD_FAIL,
    AUDIT_RULE_NAME_EXIST,
    AUDIT_RULE_OVERFLOW,
    AUDIT_RULE_NAME_NOTEXIST,
    AUDIT_RULE_USED_SCHEDULE,
    
#if defined(CONFIG_NSAS)
	NSAS_AUDIT_RULE_USED = 920,
#endif

	USER_SCHEDULE_USED_ANTI_SHARING_RULE = 930,
	ANTI_SHARING_RULE_NAME_EXIST,
    ANTI_SHARING_RULE_MAX_NUMBER,
    ANTI_SHARING_RULE_HOST_FORMAT, 
	ANTI_SHARING_RULE_NOT_FOUND,
    
    /*SYSLOG config module*/
    SYSLOG_BASE_ERROR_ID = SYSLOG_CONFIG,
    SYSLOG_SET_FORMAT,

    /*DEBUG config module*/
    DEBUG_BASE_ERROR_ID = DEBUG_INFO,

    AUTH_OLD_PASSWORD_WRONG = AUTH_INFO,
    AUTH_USER_TYPE_WRONG ,
    AUTH_USER_NOT_EXIST ,

    LIMIT_USER_LOGIN_ERROR = 1100,

    USER_LDAP_IMPORT_NO_NAME = 1110,
    USER_LDAP_IMPORT_NAME_USED,
    USER_LDAP_IMPORT_GET_ERROR,
    USER_LDAP_IMPORT_RUNNING,
	USER_LDAP_IMPORT_TEST_FAILED,

    /*auto upgrade*/
    AUTO_UPGRADE_DIR_NOT_EXIST=1200,
    AUTO_UPGRADE_SIGFILE_NOT_EXIST,
    AUTO_UPGRADE_SIGFILE_DECRYPT_ERR,
    AUTO_UPGRADE_NO_LATTER_VERSION,
    AUTO_UPGRADE_GET_HTTP_SERVER_FAIL,
    
    AUTO_UPGRADE_COMMAND_AREUMENT_ERR,//1205
    AUTO_UPGRADE_REMOTE_VERSION_FORMAT_ERR,
    AUTO_UPGRADE_CUR_VERSION_FORMAT_ERR,
    AUTO_UPGRADE_DNAME_RESOLVING_ERR,
    AUTO_UPGRADE_LOCAL_VERSION_FORMAT_ERR,
    
    AUTO_UPGRADE_NO_SPACE,//1210
    AUTO_UPGRADE_DOWNLOAD_PATCH_ERR,
    AUTO_UPGRADE_PATCH_CHECK_ERR,
    AUTO_UPGRADE_SERVER_ERR,

    USER_MANAGEFRAME_PHONE_UESED = 1300,
	USER_MANAGEFRAME_EMAIL_UESED = 1301,

    /*addr book*/
    HYTF_ADDRBOOK_NO_NAME      = 2000,
    HYTF_ADDRBOOK_NOT_FOUND    = 2001,
    HYTF_ADDRBOOK_NAME_USED    = 2002,
    HYTF_ADDRBOOK_TOO_MANY    = 2003,
    HYTF_ADDRBOOK_TEXT_WRONG    = 2004,
    HYTF_ADDRBOOK_ITEM_LARGE    = 2005,
    HYTF_ADDRBOOK_NO_MEMORY    = 2006,
    HYTF_ADDRBOOK_NAME_CONFLICT = 2007,
    HYTF_ADDRBOOK_PREDEFINE     = 2008,
    HYTF_ADDRBOOK_ASSET_SCAN_RANGE     = 2009,
    

    /*domain group*/
    HYTF_DOMAIN_GROUP_NO_NAME      = 2010,
    HYTF_DOMAIN_GROUP_NOT_FOUND    = 2011,
    HYTF_DOMAIN_GROUP_NAME_USED    = 2012,
    HYTF_DOMAIN_GROUP_TOO_MANY     = 2013,
    HYTF_DOMAIN_GROUP_TEXT_WRONG   = 2014,
    HYTF_DOMAIN_GROUP_ITEM_LARGE   = 2015,
    HYTF_DOMAIN_GROUP_ITEM_MANY    = 2016,
    HYTF_DOMAIN_GROUP_NO_MEMORY    = 2017,


    /*ztgw*/
    HYTF_ZTGW_NO_NAME      = 2020,
    HYTF_ZTGW_NOT_FOUND    = 2021,
    HYTF_ZTGW_NAME_USED    = 2022,
    HYTF_ZTGW_TOO_MANY    = 2023,
    HYTF_ZTGW_TEXT_WRONG    = 2024,
    HYTF_ZTGW_ITEM_LARGE    = 2025,
    HYTF_ZTGW_NO_MEMORY    = 2026,
    
    /*mac filter */
    HYTF_COMMON_NO_NAME      = 2030,
    HYTF_COMMON_NOT_FOUND    = 2031,
    HYTF_COMMON_NAME_USED    = 2032,
    HYTF_COMMON_TOO_MANY     = 2033,
    HYTF_COMMON_TEXT_WRONG    = 2034,
    HYTF_COMMON_ITEM_LARGE    = 2035,
    HYTF_COMMON_NO_MEMORY     = 2036,
    HYTF_COMMON_NAME_CONFLICT = 2037,

    HYTF_WHITE_LIST_NO_NAME    = 2100,
    HYTF_WHITE_LIST_NOT_FOUND  = 2102,
    HYTF_WHITE_LIST_NAME_USED  = 2103,
    HYTF_ADDR_USED_WHITE_LIST  = 2104,
    USER_SCHEDULE_USED_HYTF_WHITE_LIST = 2105,
    HYTF_WHITE_LIST_HOST_FORMAT = 2106,
    HYTF_WHITE_LIST_ADDRESSBOOK = 2107,
    HYTF_WHITE_LIST_ADDRESS_MODE = 2108,
    HYTF_WHITE_LIST_DOAMIN_GROUP = 2109,    

    HYTF_TC_RULE_NO_NAME    = 2110,
    HYTF_TC_RULE_NOT_FOUND  = 2112,
    HYTF_TC_RULE_NAME_USED  = 2113,
    HYTF_ADDR_USED_TC_RULE  = 2114,
    HYTF_TC_INSERT_NOT_FOUND = 2115,
    HYTF_TC_RULE_MAX         = 2116,
    URLCUSTOM_OBJECT_USED_TC_RULE = 2117,
    
    /*no pass auth policy*/
    NPAUTH_POLICY_SYSTEM_FAIL = 2200,
    NPAUTH_POLICY_RULE_MODE_ERROR,
    NPAUTH_POLICY_RULE_NO_NAME,
    NPAUTH_POLICY_RULE_NAME_USED,
    NPAUTH_POLICY_RULE_SERVICE,
	
    NPAUTH_POLICY_RULE_NO_SERVICE = 2205,
    NPAUTH_POLICY_RULE_NOT_FOUND,
    NPAUTH_POLICY_RULE_EXIST,
    NPAUTH_POLICY_RULE_SCHEDULE,
    NPAUTH_POLICY_RULE_HOST_FORMAT,
	
    NPAUTH_POLICY_RULE_HOST_ADDRBOOK = 2210,
    NPAUTH_POLICY_RULE_ADDRESS_MODE,
    NPAUTH_POLICY_RULE_INSERT_NOT_FOUND,
    NPAUTH_POLICY_RULE_MOVE_NOT_FOUND,
    NPAUTH_POLICY_RULE_MEMORY_FAIL,
	
    USER_SCHEDULE_USED_NPAUTH_POLICY = 2215,
    FIREWALL_ADDR_USED_NPAUTH_POLICY,
    FIREWALL_SVC_USED_NPAUTH_POLICY,
    FIREWALL_DOMAIN_USED_NPAUTH_POLICY,
    NPAUTH_POLICY_RULE_HOST_DOMAINBOOK,

    PROUTE_POLICY_RULE_USED_DOMAINBOOK = 2220,
    FIREWALL_DOMAIN_USED_NETWORK_ZONE,
    USER_SCHEDULE_USED_SERVICE_CONTROL,
    USER_SCHEDULE_USED_DNS_PROXY_POLICY,
    USER_SCHEDULE_USED_IPV6_ROUTE_POLICY,
    NPAUTH_POLICY_RULE_EXCEED = 2225,

	WHITE_LIST_POLICY_RULE_USED_DOMAINBOOK = 2230,

    HYTF_SERVICEPOOL_SYSTEM_FAIL = 2250,
    HYTF_SERVICEPOOL_RULE_EXIST,
    HYTF_SERVICEPOOL_RULE_NOT_FOUND,
    HYTF_SERVICEPOOL_RULE_OVERFLOW,

    HYTF_SERVICE_CONTROL_SYSTEM_FAIL = 2260,
    HYTF_SERVICE_CONTROL_RULE_EXIST,
    HYTF_SERVICE_CONTROL_RULE_NOT_FOUND,
    HYTF_SERVICE_CTRL_USER_NOT_FOUND,
    HYTF_SERVICE_CTRL_HOST_FORMAT_ERROR,

    HYTF_SSL_PROXY_CFG_SYSTEM_FAIL = 2270,

    HYTF_BALANCE_DNS_INTERFACE_FAIL = 2280,
    HYTF_BALANCE_DNS_INTERFACE_NOT_FOUND,
    HYTF_BALANCE_DNS_INTERFACE_EXIST,
    HYTF_BALANCE_DNS_INTERFACE_INUSE,
    HYTF_BALANCE_DNS_POLICY_FAIL,
    HYTF_BALANCE_DNS_POLICY_NOT_FOUND,
    HYTF_BALANCE_DNS_POLICY_EXIST,
    HYTF_BALANCE_DNS_POLICY_INUSE,
    HYTF_BALANCE_DNS_DOMAIN_FAIL,
    HYTF_BALANCE_DNS_DOMAIN_NOT_FOUND,
    HYTF_BALANCE_DNS_DOMAIN_EXIST,
    HYTF_BALANCE_DNS_DOMAIN_INUSE,
    HYTF_BALANCE_DNS_DOMAIN_TOOMUCH,
    HYTF_BALANCE_DNS_RR_A_TOOMUCH,
    HYTF_BALANCE_DNS_RR_CNAME_TOOMUCH,
    HYTF_BALANCE_DNS_RR_MX_TOOMUCH,
    HYTF_BALANCE_DNS_RR_A_FORMAT,
    HYTF_BALANCE_DNS_INTERFACE_TOOMUCH,
    HYTF_BALANCE_DNS_POLICY_TOOMUCH,

    HYTF_CHNGRP_BOND_IF_ATTR_DIFFERENT = 2310,
    HYTF_CHNGRP_BOND_NO_IF,
    HYTF_CHNGRP_BOND_IF_NAME_SAME,
    HYTF_CHNGRP_BOND_IF_PRIMARY_WRONG,
    HYTF_CHNGRP_BOND_MODE_WRONG,
    HYTF_CHNGRP_BOND_XMIT_HASH_WRONG,
    HYTF_CHNGRP_BOND_MASTER_CONFIG_FAILED,
    HYTF_CHNGRP_BOND_SLAVE_CONFIG_FAILED,
    HYTF_CHNGRP_BOND_PARAMETER_CONFIG_FAILED,
    HYTF_CHNGRP_BOND_NOT_FOUND,
    HYTF_CHNGRP_BOND_PHY_IF_USED,
    HYTF_CHNGRP_BOND_IP_TARGET_WRONG,
    HYTF_CHNGRP_BOND_USED_BY_BRIDGE,
    HYTF_CHNGRP_BOND_SLAVE_USED_BY_BRIDGE,

    /**Dynamic route protocol**/
    HYTF_DRP_ZEBRA_ERR_ID_MIN = 2350,
    /**ospf, rip, ospf6, ripngd:**/
    HYTF_DRP_ZEBRA_ERR_ID_MAX = 2449,

    /*access rule*/
    ACCESSRULE_OBJECT_SYSTEM_FAIL = 2480,
    ACCESSRULE_OBJECT_NOT_FIND    = 2481,
    ACCESSRULE_OBJECT_NAME_USED   = 2482,
    ACCESSRULE_OBJECT_MAX_NUMBER  = 2483,
    ACCESSRULE_OBJECT_NOT_EXIST   = 2484,
    ACCESSRULE_OBJECT_DEL_USED    = 2485,
    ACCESSRULE_OBJECT_CLEAR_USED  = 2486,
    ACCESSRULE_OBJECT_HOST_FORMAT  = 2487, 
    ACCESSRULE_USED_BLACKLIST_RULE = 2488,
    
    USER_WLAN_STATION_NO_NAME = 2650,
    USER_WLAN_STATION_IP_ERROR = 2651,
    USER_WLAN_STATION_NOT_EXIST = 2652,
    USER_WLAN_STATION_NO_HOT_IP = 2653,
    USER_WLAN_STATION_MAC_ERROR = 2654,
    USER_WLAN_STATION_DEFAULT_TYPE_ERROR = 2655,
    USER_WLAN_STATION_NAME_EXIST = 2656,
    USER_WLAN_STATION_MEM_ERR = 2657,
    USER_WLAN_STATION_NUM_OVERFLOW = 2658,
    USER_WLAN_STATION_NUM_OVERFLOW_DEFAULT = 2659,

    HYTF_NMC_ZONE_SITE_NAME_ERROR = 2700,
    HYTF_NMC_ZONE_SITE_NO_PARENT = 2701,
    HYTF_NMC_ZONE_SITE_SAME_NAME = 2702,
    HYTF_NMC_ZONE_SITE_WEB_NM_ERROR = 2703,
    HYTF_NMC_ZONE_SITE_REP_NM_ERROR = 2704,
    HYTF_NMC_ZONE_SITE_SSH_NM_ERROR = 2705,
    HYTF_NMC_ZONE_SITE_OTHER_ERROR = 2706,
    USER_MANAGEFRAME_WRONG_NAME = 2707,
    USER_MANAGEFRAME_NUMBER_OVERFLOW = 2708,

    PHP_SNMP_ERROR_PARAMET = 2750,
    PHP_SNMP_ERROR_FILE = 2751,
    PHP_ERROR_PARAMET = 2752,

    SSLVPN_MATERIAL_NO_NAME,//2753
    SSLVPN_MATERIAL_NO_GROUP,
    SSLVPN_MATERIAL_NO_ADDRESS,
    SSLVPN_MATERIAL_NOT_FOUND,
    SSLVPN_MATERIAL_NAME_USED,
    SSLVPN_MATERIAL_USED,
    SSLVPN_MATERIAL_ERROR,//2759

    SSLVPN_MATERIAL_GROUP_NO_NAME,//2760
    SSLVPN_MATERIAL_GROUP_NOT_FOUND,
    SSLVPN_MATERIAL_GROUP_NAME_USED,
    SSLVPN_MATERIAL_GROUP_USED,
    SSLVPN_MATERIAL_GROUP_ERROR,

	// IPS
    IPS_BASE_ERROR_ID = IPS_CONFIG, // 2810
    IPS_RULE_ADDRESS_FORMAT, // 2811
	IPS_RULE_PARAM, // 2812
	IPS_RULE_NAME_NULL = IPS_RULE_PARAM, 
	IPS_RULE_NAME_USED = IPS_RULE_PARAM, 
	IPS_RULE_MEM_FAIL = IPS_RULE_PARAM, 
	IPS_RULE_NOT_FOUND = IPS_RULE_PARAM, 
	IPS_RULE_INDEX_RANGE = IPS_RULE_PARAM, 
    IPS_RULE_REF_BY_SECURITY_PROTECT_POLICY = IPS_RULE_PARAM + 10, 

    
	// ATTACKER BLACKLIST
	ATBL_BASE_ERROR_ID = ATBL_CONFIG,
	ATBL_PARAM,
	ATBL_MEM_FAIL,
	ATBL_NOT_FOUND,
	ATBL_INDEX_RANGE,
	ATBL_BLOCK_RANGE,

    // AV
    AV_BASE_ERROR_ID = AV_CONFIG,
    AV_RULE_PARAM,
    AV_RULE_NAME_NULL,
    AV_RULE_NAME_USED,
    AV_RULE_MEM_FAIL,
    AV_RULE_NOT_FOUND,
    AV_RULE_INDEX_RANGE,
    AV_SIG_PARAM,
    AV_EXCLUDE_PARAM,
    AV_EXCLUDE_MEM_FAIL,
    
    // WAF
    WAF_BASE_ERROR_ID = WAF_CONFIG,
    WAF_RULE_PARAM,
    WAF_RULE_NAME_NULL,
    WAF_RULE_NAME_USED,
    WAF_RULE_MEM_FAIL,
    WAF_RULE_NOT_FOUND,
    WAF_RULE_INDEX_RANGE,
    
    //ATTACK DEFENCE
    AD_BASE_ERROR_ID = AD_CONFIG,//3000
    AD_ERR_POLICY_NO_NAME,
    AD_ERR_POLICY_EXIST,
    AD_ERR_POLICY_FULL,
    AD_ERR_POLICY_MEM,
    AD_ERR_POLICY_CACHE,//3005
    AD_ERR_POLICY_PARAM,
    AD_ERR_POLICY_NOT_FOUND,
    AD_ERR_POLICY_ZONE_NOT_FOUND,
    AD_ERR_POLICY_ADDRBOOK_NOT_FOUND,
    AD_ERR_INNER_POLICY_SRC_ADDR_ERROR,//3010
    AD_ERR_INNER_POLICY_EXC_ADDR_ERROR,
    
    
    //POLICY ROUTE
    POLICY_ROUTE_BASE_ERROR_ID = PROUTE_CONFIG,
    POLICY_ROUTE_NEXTHOP_NOT_WAN = 3201,
    POLICY_ROUTE_MAX_NUMBER,
    POLICY_ROUTE_NOT_FOUND,
    POLICY_ROUTE_SOURCE_IP,
    
    POLICY_ROUTE_DEST_IP,
    POLICY_ROUTE_DEST_ISP,
    POLICY_ROUTE_NAME_USED,
    POLICY_ROUTE_SYSTEM_FAIL,
    POLICY_ROUTE_SYSTEM_FAIL2,
    
    POLICY_ROUTE_SYSTEM_FAIL3,
    POLICY_ROUTE_SYSTEM_FAIL5,
    POLICY_ROUTE_RULE_SCHEDULE = 3212,
    ISP_POOL_NOT_FOUND,
    ISP_POOL_NAME_USED,
    
    ISP_POOL_USED,
    ISP_POOL_SYSTEM_FAIL,    
    PERSISTENT_ROUTE_NOT_FOUND,
    PERSISTENT_ROUTE_NAME_USED,
    PERSISTENT_ROUTE_USED,
    
    PERSISTENT_ROUTE_SYSTEM_FAIL,
    USER_SCHEDULE_USED_POLICY_ROUTE_RULE,
    POLICY_ROUTE_DEST_DOMAIN_GROUP = 3222,
    POLICY_PROUTE_DEST_OVERFLOW = 3223,
    POLICY_PROUTE_SOURCE_OVERFLOW = 3224,
    POLICY_PROUTE_SOURCE_EXCLUDE_OVERFLOW = 3225,
    POLICY_PROUTE_DEST_EXCLUDE_OVERFLOW = 3226,
    POLICY_PROUTE_SOURCE_EXCLUDE_IP = 3227,
    POLICY_PROUTE_DEST_EXCLUDE_IP = 3228,

	ISP_POOL_MAX_NUMBER = 3240,
	PERSISTENT_ROUTE_MAX_NUMBER = 3241,

    LO_ERROR_SYSTEM_FAIL = LINK_OBJ_CONFIG,
    LO_ERROR_NOT_EXIST,
    LO_ERROR_BUSY,
    LO_ERROR_CONFILCT,


    LOCATION_ERR_SYSTEM_FAIL = LOCATION_ERR_INFO,
    LOCATION_ERR_PARM,
    LOCATION_ERR_OVERFLOW,
    LOCATION_ERR_IP_CONFLICT,
    LOCATION_ERR_USED_BY_AUTH_POLICY,


    HYTF_MIRROR_ERROR_PARAM = HYTF_MIRROR_POLICY,
    HYTF_MIRROR_ERROR_SYSTEM,
    HYTF_MIRROR_ERROR_NOT_FOUND,
    HYTF_MIRROR_ERROR_EXIST,
    HYTF_MIRROR_ERROR_IFS_SAME,
	
	HYTF_SMP_ERROR_PARAM = HYTF_SMP_POLICY,
    HYTF_SMP_ERROR_SYSTEM,

    HYTF_EDR_ERROR_DOMAIN = HYTF_EDR,
    
    //threat intelligence
    TI_BASE_ERROR_ID = TI_CONFIG, //3800
    TI_POLICY_PARAM,
    TI_POLICY_NAME_NULL,
    TI_POLICY_NAME_USED,
    TI_POLICY_MEM_FAIL,
    TI_POLICY_NOT_FOUND, //3805
    
    TI_CUSTOM_RULE_ERR_PARAM,
    TI_CUSTOM_RULE_ERR_EXIST,
    TI_CUSTOM_RULE_ERR_NOT_FOUND,
    TI_CUSTOM_RULE_ERR_IP_EXCEED,
    TI_CUSTOM_RULE_ERR_MEMORY,//3810
    TI_CUSTOM_RULE_ERR_DOMAIN_EXCEED,

    TI_CUSTOM_CATEGORY_ERR_PARAM,//3812
    TI_CUSTOM_CATEGORY_ERR_EXIST,
    TI_CUSTOM_CATEGORY_ERR_NOT_FOUND,//3814
    TI_CUSTOM_CATEGORY_ERR_MEMORY,
    TI_CUSTOM_CATEGORY_ERR_NUM_EXCEED,
    TI_CUSTOM_CATEGORY_ERR_USED,//3817

    //illegal outreach protect
    IOP_BASE_ERROR_ID = IOP_CONFIG, // 3900
    IOP_SERVER_PARAM, // 3901
    IOP_SERVER_NAME_NULL, // 3902
    IOP_SERVER_NAME_USED, // 3903
    IOP_SERVER_MEM_FAIL, // 3904
    IOP_SERVER_NOT_FOUND, // 3905
    IOP_SERVER_INDEX_RANGE, // 3906
    IOP_SERVER_REFERENCED_BY_LEARN, // 3907
    
    IOP_POLICY_PARAM,// 3908
    IOP_POLICY_NAME_NULL,// 3909
    IOP_POLICY_NAME_USED,// 3910
    IOP_POLICY_MEM_FAIL,
    IOP_POLICY_NOT_FOUND,
    IOP_POLICY_INDEX_RANGE,
    IOP_POLICY_SERVER_EXCEED,
    IOP_POLICY_EXCLUDE_EXCEED,// 3915
    
    IOP_LEARN_PARAM,// 3916
    IOP_LEARN_NAME_NULL,
    IOP_LEARN_NAME_USED,// 3918
    IOP_LEARN_MEM_FAIL,
    IOP_LEARN_NOT_FOUND,// 3920
    IOP_LEARN_INDEX_RANGE,
    
    IOP_LEARN_LIST_PARAM,
    IOP_LEARN_LIST_NOT_FOUND,
    IOP_LEARN_LIST_IPADDR_ERROR,
    
    IOP_IPADDR_FORMAT,// 3925
    IOP_PORT_FORMAT,// 3926
    IOP_SERVER_REFERENCED_BY_POLICY, // 3927
    
    //risk analysis
    RA_BASE_ERROR_ID = RA_CONFIG,
    RA_IP_FORMAT_ERROR,
    RA_PORT_FORMAT_ERROR,
    RA_PREDEF_PORT_FORMAT_ERROR,
    RA_CUSTOM_DEFINE_ERROR,
    RA_IPADDR_ERROR,
    RA_SAME_IP_PORT_EXIST_ERROR,
    RA_POLICY_NOT_FOUND_ERROR,

    DNS_TPROXY_SYSTEM_FAIL      = DNS_TPROXY_CONFIG,
    DNS_TPROXY_HOST_FORMAT,
    DNS_TPROXY_ADDRESSBOOK,
    DNS_TPROXY_DOAMIN_GROUP,
    DNS_TPROXY_ADDRESS_MODE,

    DNS_TPROXY_SYSTEM_FAIL1,
    DNS_TPROXY_NOT_FOUND,
	DNS_TPROXY_SYSTEM_FAIL2,
    DNS_TPROXY_NAME_USED,    
    FIREWALL_ADDR_USED_DNS_TROXY,

    DNS_TPROXY_SCHEDULE_USED = 4111,
    DNS_TRPOXY_DOMAIN_OVERFLOW = 4112,

	NETWORK_ZONE_MEM_FAIL      = NETWORK_ZONE_CONFIG,
	NETWORK_ZONE_NAME_EMPTY,
	NETWORK_ZONE_ADDR_AND_DOMAIN_ENPTY,
	NETWORK_ZONE_ADDR_NOT_FOUND,
	NETWORK_ZONE_ADDR_DOMAIN_FOUND,
	
	NETWORK_ZONE_OVERFLOW,
	NETWORK_ZONE_NAME_EXIST,
	NETWORK_ZONE_NOT_FOUND,


    USER_RADIUS_USED_BY_ACCOUNT = COMM_ERR_OTHER,
    ASSET_SCAN_PARAMETER_ERROR, // 4301
    ASSET_SCAN_ADDR_RANGE_ERROR,    // 4302
    ASSET_SCAN_ACTIVE_ADDR_RANGE_ERROR, // 4303

    BLKLIST_FILTER_ERR_MEMORY = BLKLIST_FILTER_ERR,//4400
    BLKLIST_FILTER_ERR_PARAMETER,
    BLKLIST_FILTER_ERR_SINGLE_IP_EXCEED,
    BLKLIST_FILTER_ERR_RANGE_IP_EXCEED,
    BLKLIST_FILTER_ERR_DOMAIN_EXCEED,
    BLKLIST_FILTER_ERR_NOT_FOUND,//4405
    BLKLIST_FILTER_ERR_FULL_CONFIG, //memory error
    BLKLIST_FILTER_ERR_FULL_CONFIG_SAVE,
    BLKLIST_FILTER_ERR_FULL_CONFIG_RESTORE,
    BLKLIST_FILTER_ERR_FULL_CONFIG_EXPORT,
    BLKLIST_FILTER_ERR_FULL_CONFIG_IMPORT,//4410
    BLKLIST_FILTER_ERR_EXIST,
    BLKLIST_FILTER_ERR_TOO_MANY_TEMPORARY,

    FW_SEPERATE_POLICY_ERR_NAME_NULL = FW_SEPERATE_POLICY_ERR,//4500
    FW_SEPERATE_POLICY_ERR_NAME_EXIST,
    FW_SEPERATE_POLICY_ERR_NAME_NOT_FOUND,//4502
    FW_SEPERATE_POLICY_ERR_ADDRESS_NOT_FOUND,
    FW_SEPERATE_POLICY_ERR_MEM,//4504
    FW_SEPERATE_POLICY_ERR_SERVER_ADDRESS,
    FW_SEPERATE_POLICY_ERR_REFERENCED,//4506
    FW_SEPERATE_POLICY_ERR_ADDRESS_FORMAT,
    FW_SEPERATE_POLICY_ERR_SCHEDULE,//4508
    FW_SEPERATE_POLICY_ERR_IPS_TEMPLATE,
    FW_SEPERATE_POLICY_ERR_AV_TEMPLATE,//4510
    FW_SEPERATE_POLICY_ERR_WAF_TEMPLATE,
    FW_SEPERATE_POLICY_ERR_CS_TEMPLATE,//4512
    FW_SEPERATE_POLICY_ERR_USED_SCHEDULE,//4513
    FW_SEPERATE_POLICY_ERR_USED_ADDRESS_BOOK,//4514
    FW_SEPERATE_POLICY_ERR_USED_ZONE,//4515
    FW_SEPERATE_POLICY_ERR_USED_IPS,//4516
    FW_SEPERATE_POLICY_ERR_USED_WAF,//4517
    FW_SEPERATE_POLICY_ERR_USED_AV,//4518
    FW_SEPERATE_POLICY_ERR_USED_CS,//4519

    FW_APP_CONTROL_POLICY_ERR_USED_SCHEDULE,//4520
    FW_APP_CONTROL_POLICY_ERR_USED_ADDRESS_BOOK,//4521
    FW_WEB_KEYWORD_FILTER_ERR_USED_SCHEDULE,//4522
    FW_WEB_KEYWORD_FILTER_ERR_USED_ADDRESS_BOOK,//4523
    FW_PROTOCOL_CMD_CONTROL_ERR_USED_SCHEDULE,//4524
    FW_PROTOCOL_CMD_CONTROL_ERR_USED_ADDRESS_BOOK,//4525
    FW_PROXY_CONTROL_ERR_USED_SCHEDULE,//4526
    FW_PROXY_CONTROL_ERR_USED_ADDRESS_BOOK,//4527
    FW_DECRYPT_POLICY_ERR_USED_ADDRESS_BOOK,//4528
    FW_HOST_ACCESS_CONTROL_ERR_USED_ADDRESS_BOOK,//4529
    FW_HOST_ACCESS_CONTROL_ERR_USED_SERVICE,  //4530
    FW_HOST_ACCESS_CONTROL_ERR_USED_ZONE,  //4531

    FW_SEPERATE_POLICY_ERR_SERVICE,//4532
    FW_SEPERATE_POLICY_ERR_IP_STRING,//4533

    FW_SEPERATE_POLICY_ERR_NUM_EXCEED, //4534

    FW_SEPERATE_POLICY_ERR_IC_TEMPLATE, //4535 
        
    FIREWALL_SERVICE_USED_BY_SECURITY_POLICY, //4536
    FIREWALL_SERVICE_USED_BY_PFC, //4537
    FIREWALL_SERVICE_USED_BY_UFC, //4538
    FIREWALL_SCHEDULE_USED_BY_SESSION_CONTROL, //4539
    FIREWALL_ZONE_USED_BY_SESSION_CONTROL,  //4540
    FIREWALL_ADDRESSBOOK_USED_BY_SESSION_CONTROL,  //4541
    FIREWALL_SERVICE_USED_BY_SESSION_CONTROL,  //4542
    FIREWALL_SERVICE_USED_BY_SERVICE_CONTROL, //4543
    FIREWALL_SERVICE_USED_BY_BEHAVIOUR_WARNING, //4544
    FIREWALL_ADDRESSBOOK_USED_BY_ASSET_SCAN,  //4545
    FIREWALL_ADDRESSBOOK_USED_BY_TOPOLOGY_LEARNING,  //4546
    FIREWALL_ADDRESSBOOK_USED_BY_360_IP_DOMAIN_DB,  //4547

    SSLVPN_ERROR_START = SSLVPN_CONFIG_ERR,//4700
    SSLVPN_ERROR_PARAM, //4701
    SSLVPN_ERROR_MEM_FAIL,

    SSLVPN_ERROR_USER_GROUP_NAME_NULL,
    SSLVPN_ERROR_USER_GROUP_NAME_USED,
    SSLVPN_ERROR_USER_GROUP_NOT_FOUND, //4705
    SSLVPN_ERROR_USER_GROUP_PARENT_NOT_FOUND,
    SSLVPN_ERROR_USER_GROUP_SAME_EXIST,
    SSLVPN_ERROR_USER_GROUP_EXCEED_MAX,
    SSLVPN_ERROR_USER_GROUP_EXCEED_GROUP_MAX,
    SSLVPN_ERROR_USER_GROUP_LEVEL_EXCCED_MAX,//4710
    SSLVPN_ERROR_USER_GROUP_RELATE_ROLE,
    SSLVPN_ERROR_USER_GROUP_RELATE_ROLE_EXCEED,

    SSLVPN_ERROR_USER_NAME_NULL,
    SSLVPN_ERROR_USER_NAME_USED,
    SSLVPN_ERROR_USER_NOT_FOUND, //4715
    SSLVPN_ERROR_USER_PARENT_NOT_FOUND,
    SSLVPN_ERROR_USER_SAME_EXIST,
    SSLVPN_ERROR_USER_EXCEED_MAX,
    SSLVPN_ERROR_USER_RELATE_ROLE,
    SSLVPN_ERROR_USER_RELATE_ROLE_EXCEED, //4720

    SSLVPN_ERROR_RESOURCE_GROUP_NAME_NULL,
    SSLVPN_ERROR_RESOURCE_GROUP_NAME_USED,
    SSLVPN_ERROR_RESOURCE_GROUP_NOT_FOUND,
    SSLVPN_ERROR_RESOURCE_GROUP_PARENT_NOT_FOUND,
    SSLVPN_ERROR_RESOURCE_GROUP_SAME_EXIST, //4725
    SSLVPN_ERROR_RESOURCE_GROUP_EXCEED_MAX,
    SSLVPN_ERROR_RESOURCE_GROUP_EXCEED_GROUP_MAX,
    SSLVPN_ERROR_RESOURCE_GROUP_LEVEL_EXCCED_MAX,
    
    SSLVPN_ERROR_RESOURCE_NAME_NULL,
    SSLVPN_ERROR_RESOURCE_NAME_USED, //4730
    SSLVPN_ERROR_RESOURCE_NOT_FOUND,
    SSLVPN_ERROR_RESOURCE_PARENT_NOT_FOUND,
    SSLVPN_ERROR_RESOURCE_SAME_EXIST,
    SSLVPN_ERROR_RESOURCE_EXCEED_MAX,
    SSLVPN_ERROR_RESOURCE_IP_FORMAT, //4735
    SSLVPN_ERROR_RESOURCE_PORT_FORMAT,

    SSLVPN_ERROR_ROLE_NAME_NULL,
    SSLVPN_ERROR_ROLE_NAME_USED,
    SSLVPN_ERROR_ROLE_NOT_FOUND,
    SSLVPN_ERROR_ROLE_EXCEED_MAX, //4740
    SSLVPN_ERROR_ROLE_USER_FORMAT,
    SSLVPN_ERROR_ROLE_RESOURCE_FORMAT,
    SSLVPN_ERROR_ROLE_RELATE_RESOURCE_GROUP,
    SSLVPN_ERROR_ROLE_RELATE_RESOURCE,
    
    SSLVPN_ERROR_USER_USER_PASSWD, //4745
    SSLVPN_ERROR_USER_DOS,
    SSLVPN_ERROR_IP_DOS,
    SSLVPN_ERROR_USER_BLOCKED,
    SSLVPN_ERROR_USER_FREEZED,
    SSLVPN_ERROR_USER_TIME_INVALID, //4750
    SSLVPN_ERROR_USER_EXCEED_LIMIT,
    SSLVPN_ERROR_USER_EXCEED_KICKOFF,
    
    SSLVPN_ERROR_USER_IP_FORMAT,
    SSLVPN_ERROR_IP_NOT_FOUND, 
    SSLVPN_ERROR_ROLE_RELATE_USER_GROUP,//4755
    SSLVPN_ERROR_ROLE_RELATE_USER,

    SSLVPN_ERROR_MAX,

    INDUSTRIAL_CONTROL_TEMPLATE_ERR_NAME_NULL = INDUSTRIAL_CONTROL_ERR,//5000
    INDUSTRIAL_CONTROL_TEMPLATE_ERR_NAME_EXIST,
    INDUSTRIAL_CONTROL_TEMPLATE_ERR_NAME_NOT_FOUND,
    INDUSTRIAL_CONTROL_TEMPLATE_ERR_REFERENCED,
    INDUSTRIAL_CONTROL_TEMPLATE_ERR_NUM_EXCEED,

    INDUSTRIAL_CONTROL_ERR_ENTRY_NOT_FOUND,//5005
    INDUSTRIAL_CONTROL_ERR_ENTRY_EXIST,
    INDUSTRIAL_CONTROL_ERR_ENTRY_NUM_EXCEED,

    INDUSTRIAL_CONTROL_ERR_ADDRESS_FORMAT,
    INDUSTRIAL_CONTROL_ERR_VALUE_FIELD_FORMAT,

    INDUSTRIAL_CONTROL_ERR_PORT_EXIST,//5010
    INDUSTRIAL_CONTROL_ERR_PORT_STANDARD_EXIST,
    INDUSTRIAL_CONTROL_ERR_IP_FORMAT,
    INDUSTRIAL_CONTROL_ERR_PORT_FORMAT,

    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ERR_NAME_NULL,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ERR_NAME_EXIST,//5010
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ERR_NAME_NOT_FOUND,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ERR_REFERENCED,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ERR_NUM_EXCEED,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ERR_FIELD_ALL_NULL,


    INDUSTRIAL_CONTROL_ERR_PARAM,//5020
    INDUSTRIAL_CONTROL_ERR_MEM,    


    BFD_TRACK_NO_NAME = BFD_CONFIG_ERR,
    BFD_TRACK_NAME_USED,
    BFD_TRACK_WRONG_LOCAL_IP,
    BFD_TRACK_WRONG_REMOTE_IP,
    BFD_TRACK_WRONG_IFNAME,

    BFD_TRACK_WRONG_LOCAL_ID,
    BFD_TRACK_WRONG_PEER_ID,
    BFD_TRACK_WRONG_TX_INTRERVAL,
    BFD_TRACK_WRONG_RX_INTRERVAL,
    BFD_TRACK_WRONG_LOCAL_MULTI,

    BFD_TRACK_WRONG_ECHO_INTERVAL,
    BFD_TRACK_ENTRY_OVERFLOW,
    BFD_TRACK_NO_ENTRY,

    SSL_PROXY_LIB_ERR_PARAM = SSL_PROXY_LIB_ERR,
    SSL_PROXY_LIB_ERR_SHM,
    SSL_PROXY_LIB_ERR_EXIST,
    SSL_PROXY_LIB_ERR_ID,
    SSL_PROXY_LIB_ERR_OVERFLOW,
    SSL_PROXY_LIB_ERR_FS,
    SSL_PROXY_LIB_ERR_NO_EXIST,
    SSL_PROXY_LIB_ERR_PREDEFINED,


    HYNAD_ERR_EXIST_IP = HYNAD_ERR,
    HYNAD_ERR_PARAM,
    HYNAD_ERR_OVERFLOW,

    ACE_MROUTE_BASE_ERROR_ID = ACE_MROUTE,
    ACE_MROUTE_SYSTEM_FAIL,
    ACE_MROUTE_MEMORY_FAIL,
    ACE_MROUTE_NOT_FOUND,
    ACE_MROUTE_EXIST,
	
    ACE_MROUTE_HOST_FORMAT,
    ACE_MROUTE_HOST_ADDRBOOK,
    ACE_MROUTE_ADDRESS_MODE,
    ACE_MROUTE_MOVE_NOT_FOUND,
    ACE_MROUTE_DEV_NOT_FOUND,

    ACE_MROUTE_MOVE_ERROR,
    ACE_MROUTE_DEV_OVERFLOW,

    SERVICE_GROUP_ERR_SERVICE_NOTFOUND = SERVICE_GROUP,
    SERVICE_GROUP_ERR_SERVICE_EXCEED, //5501
    SERVICE_GROUP_ERR_NAME_NULL,//5502
    SERVICE_GROUP_ERR_NAME_EXIST,//5503
    SERVICE_GROUP_ERR_NAME_NOT_FOUND,//5504
    SERVICE_GROUP_ERR_NUM_EXCEED, //5505
    SERVICE_GROUP_ERR_MEM, //5506
    SERVICE_GROUP_OR_SERVICE_ERR_NOTFOUND, //5507
    SERVICE_GROUP_ERR_NAME_EXIST_PRE_SERVICE,  //5508
    SERVICE_GROUP_ERR_NAME_EXIST_CUSTOM_SERVICE,  //5509
    SERVICE_GROUP_ERR_NAME_EXIST_SERVICE_GROUP,  //5510
    SERVICE_GROUP_OR_SERVICE_ERR_REF_EXCEED,  //5511
        
    FIREWALL_SVC_USED_SERVICE_GROUP = 5564,//5564
    FIREWALL_SERVICE_GROUP_USED_BY_SECURITY_POLICY,//5565
    FIREWALL_SVC_USED_WHITE_LIST,//5566
    FIREWALL_SERVICE_GROUP_USED_WHITE_LIST,//5567
    FIREWALL_SVC_USED_PROUTE,//5568
    FIREWALL_SERVICE_GROUP_USED_PROUTE,//5569
    FIREWALL_SERVICE_GROUP_USED_NPAUTH_POLICY,//5570
    FIREWALL_SERVICE_GROUP_USED_PAT,//5571
    FIREWALL_SVC_USED_PAT64,//5572
    FIREWALL_SERVICE_GROUP_USED_PAT64,//5573
    FIREWALL_SERVICE_GROUP_USED_PAT66,//5574
    FIREWALL_SVC_USED_PROUTE6,//5575
    FIREWALL_SERVICE_GROUP_USED_PROUTE6,//5576
    FIREWALL_SERVICE_GROUP_USED_HOST_ACCESS_CONTROL,//5577
    FIREWALL_SERVICE_GROUPC_USED_WHITELIST,  //5578
    FIREWALL_SERVICE_GROUP_USED_SESSION_CONTROL,//5579
    FIREWALL_SERVICE_GROUP_USED_DNAT,//5580


	//#if defined(CONFIG_NSAS)
    NSAS_FILTER_RULE_USED = NSAS_FILTER_CONFIG_ERR,
    NSAS_FILTER_RULE_NAME_EXIST,
    NSAS_FILTER_RULE_OVERFLOW,
    NSAS_FILTER_RULE_NAME_NOTEXIST,
    NSAS_FILTER_RULE_GET_ERROR,

    NSAS_FILTER_RULE_MODE_ERROR,
    NSAS_FILTER_RULE_PARM_ERROR,
	//#endif
	DAS_OBJECT_FULL = DAS_OBJECT_CONFIG_ERR,
	DAS_OBJECT_PARAM_ERR,
	DAS_OBJECT_NAME_NULL,
	DAS_OBJECT_NAME_USED,
	DAS_OBJECT_MEM_FAIL,
	DAS_OBJECT_NOT_FOUND,
	DAS_OBJECT_SYSTEM_ERROR,
	DAS_OBJECT_USED_BY_OTHER,

	SIG_ASSET_FULL = SIG_CONFIG_ERR,
	SIG_ASSET_PARAM_ERR,
	SIG_ASSET_NAME_NULL,
	SIG_ASSET_NAME_USED,
	SIG_ASSET_MEM_FAIL,
	SIG_ASSET_NOT_FOUND,
	SIG_ASSET_SYSTEM_ERROR,
	SIG_ASSET_USED_BY_OTHER,
	SIG_ASSET_GROUP_IS_EMPTY,

	SIG_GEN_ERR_MEMORY = SIG_GEN_ERR,//5850
	SIG_GEN_ERR_PARAMETER,
	SIG_GEN_ERR_SINGLE_IP_EXCEED,
	SIG_GEN_ERR_RANGE_IP_EXCEED,
	SIG_GEN_ERR_DOMAIN_EXCEED,
	SIG_GEN_ERR_NOT_FOUND,//
	SIG_GEN_ERR_FULL_CONFIG, //memory error
	SIG_GEN_ERR_FULL_CONFIG_SAVE,
	SIG_GEN_ERR_FULL_CONFIG_RESTORE,
	SIG_GEN_ERR_FULL_CONFIG_EXPORT,
	SIG_GEN_ERR_FULL_CONFIG_IMPORT,//
	SIG_GEN_ERR_EXIST,
	SIG_GEN_ERR_TOO_MANY_TEMPORARY,
	
	ILL_OUTREACH_JS_ERR_NO_MEMORY = ILL_OUTREACH_JS_ERR,
	ILL_OUTREACH_JS_ERR_HOST_FORMAT,
	ILL_OUTREACH_JS_ERR_HOST_FORMAT_WIHTE,
};

#define ACEFW_BASE_CMD_ID               2000
#define ACENAT_BASE_CMD_ID              2100
#define ACEMANAGEWHITELIST_BASE_CMD_ID  2200
#define ACECAR_BASE_CMD_ID              2300
#define ACE_PFC_BASE_CMD_ID             2400
#define ACE_UFC_BASE_CMD_ID             2500
#define ACE_L7FILTER_BASE_CMD_ID        2600
#define ACE_P3SCAN_BASE_CMD_ID          2700
#define ACE_SYSTEMCFG_BASE_CMD_ID       2800
#define ACEFW6_BASE_CMD_ID              2900
#define ACE_USER_AUTH_BASE_ID           3000
#define ACE_NMC_BASE_CMD_ID             3200
#define ACE_SSL_BASE_ID                 10000
#define HYTFFW_ADDRBOOK_BASE_CMD_ID     11100
#define MAC_FILTER_OBJ_BASE_CMD_ID      11140
#define HYTFFW_DOMAIN_GROUP_BASE_CMD_ID 11150
#define HYTFFW_WHILTE_LIST_BASE_CMD_ID  11200
#define HYTFFW_TC_BASE_CMD_ID           11300
#define HYTF_NOPASSAUTH_BASE_CMD_ID     11400
#define HYTF_SERVICECTL_BASE_CMD_ID     11500
#define HYTFFW_ADDRBOOK6_BASE_CMD_ID    11600
#define HYTFFW_FEATURE_RULE_CMD_ID      11700
#define HYTF_SMS_CMD_ID                 11800
#define HYTF_LINK_OBJ_CMD_ID            11900
#define HYTF_MIRROR_POLICY_CMD_ID       12000
#define IPS_RULE_CMD_ID                 12100
#define AV_RULE_CMD_ID                  12200
#define SAFETY_ZONE_CMD_ID              12300
#define AD_POLICY_CMD_ID                12400
#define WAF_RULE_CMD_ID                 12500
#define ATBL_CMD_ID                     12600
#define VPN_CMD_ID                      12700
#define HYTF_SMP_POLICY_CMD_ID          13000
#define HYLAB_IF_CMD_ID                 13100
#define HYLAB_EDR_CMD_ID                13300
#define TI_CMD_ID                       13400
#define WPP_CMD_ID                      13500 //weak passwd protect
#define IOP_CMD_ID                      13600 //illegal outreach protect
#define RA_CMD_ID                       13700 //risk analysis
#define BLKLIST_FILTER_CMD_ID           13800 //blklist filter
#define DNS_TPROXY_BASE_CMD_ID          14100
#define DNS_CONTROL_BASE_CMD_ID         14200
#define SWITCH_CHIP_PORT_BASE_CMD_ID	14300

#define FW_SEPERATE_CMD_ID              14350
#define SESSION_CONTROL_CMD_ID          14550
#define SSLVPN_CMD_ID                   14600
#define SSLVPN_CMD_ID_MAX_RESERVE       14800

#define INDUSTRIAL_CONTROL_CMD_ID       14900
#define WEB_FILTER_URL_CMD_ID			15100
#define ACE_MROUTE_CMD_ID               15200

#define SERVICE_GROUP_CMD_ID            15300

#define TOPOLOGY_CMD_ID            15400
#define IPMAC_CMD_ID               15500

#define HYNAT66_BASE_CMD_ID       16000

#define HYLAB_EXT_BASE            17000
#define ACENAT_BASE2_CMD_ID              19000

enum _acefw_netlink_cmd_id
{
//    HYLAB_RPC_CALL_VPP2VPP          = 999,
    ACEFW_SCHEDULE_RELATE           = 1000,
    ACEFW_USER_GROUP_RELATE         = 1010,
    ACEFW_AUTH_PORT_CONFIG          = 1020,
    ACEFW_SET_DNS_SERVER_SWITCH     = 1021,
    ACEFW_GET_DNS_SERVER_SWITCH     = 1022,
    ACEFW_AUTH_TIME_OUT_CONFIG      = 1030,
    ACEFW_IP_TIMEOUT_NOTIFY         = 1040,
    ACE_WEB_IP_QUERY_REQ            = 1050,
    ACE_SET_MACBOOK_FLAG            = 1051,
    ACE_SET_MACBOOK_CLASSIFY        = 1052,
    ACE_SET_DINGDING_AUTH_PASS      = 1053,
    ACE_USER_IP_IN                  = 1060,
    ACE_USER_IP_FLOW                = 1061,
    ACE_USER_IP_OUT                 = 1062,
    ACE_USER_ADSSO_LOGOUT           = 1069,
    ACE_IPUSERMAPPING_AUTH_NOTIFY  = 1070,
    ACEFW_DNS_PING_PERMIT_CONFIG   = 1071,
    ACEFW_URLCUSTOM_OBJECT_RELATE  = 1072,
    ACE_PPPOE_SSO_IN                  = 1073,
    ACE_PPPOE_SSO_OUT               = 1074,
    ACE_PPPOE_SSO_AUTHFAIL      = 1075,
    ACEFW_PPPOE_SSO_FLAG_CONFIG          = 1076,
    ACE_PPPOE_SSO_IPACK                  = 1077,
    HYTF_VLAN_AUTH_FLAG_CONFIG   = 1078,
    ACEFW_AD_SSO_MIRROR_CONFIG          = 1079,
    ACEFW_SSO_MIRROR_PORT_CONFIG          = 1080,
    ACE_AD_MIRROR_SSO_FLOW                = 1081,
    ACE_REGEX_AUTH_IN = 1082,
    ACE_SET_AUTH_PAGE_CLOSE_LOGOUT_KERNEL          = 1083,
    ACE_PROXY_SSO_IN                  = 1084,
    ACEFW_PROXY_SSO_FLAG_CONFIG          = 1085,
    ACEFW_TRANSPROXY_FLAG_CONFIG          = 1086,
    ACE_FTP_SSO_IN                  = 1087,
    ACE_FTP_SSO_OUT               = 1088,
    ACEFW_FTP_SSO_FLAG_CONFIG          = 1089,
    HYTF_RADIUS_SSO_IN              = 1090,
    HYTF_RADIUS_SSO_CONFIG_SET      = 1091,
    HYTF_RADIUS_SSO_CONFIG_GET      = 1092,
    HYTF_AWIFI_AUTH_STATUS          = 1093,
    
    ACE_JMS_SSO_FLOW                = 1094,
    ACE_DHCP_OFFER_IN               = 1095,
    ACE_DHCP_HANDLE_STATUS          = 1096,

    ACEFW_MAC_CHANGE_REAUTH_FLAG_CONFIG      = 1097,
    ACEFW_HANGTING_STATUS_CONFIG             = 1098,
    ACEFW_RIP_STATUS_CONFIG                  = 1099,
    ACEFW_OSPF_STATUS_CONFIG                 = 1100,
    ACEFW_LANNETSHAREDENY_CTR_CONFIG         = 1101,

    ACEFW_SEUDO_IP_CONFIG = 1102,
    ACE_SET_AUTH_PAGE_CLOSE_LOGOUT_TIMEOUT   = 1103,
    ACEFW_HTTPLINK_SSO_CONFIG = 1104,

    HYTF_FW_TCP_STATE_CHECK_SET = 1105,
    HYTF_FW_TCP_STATE_CHECK_GET = 1106,
    HYTF_ASSET_SCAN_SET        = 1107,
	
	HYTF_JS_URL_NOTIFY          = 1108,
	ACEFW_HTTP_META_REFRESH_CONFIG = 1109,

//#ifdef L7_APP_TPROXY
    ACEFW_GET_L7_TPROXY_STATUS               = 1200, 
//#endif
    HYLAB_SET_HA_CONFIG     = 1300,
    HYLAB_SET_HA_PEER_ADDRS,
    HYLAB_SET_HA_SYNC_FLAGS,
    HYLAB_GET_HA_SYNC_PEERS,

    ACEFW_SSHD_BLACKLIST_ADD                 = 1888,
    HYTF_SESSION_AGEOUT_SYNC_NOTIFY          = 1889,

	//#if defined(CONFIG_NSAS)
	NSAS_AUDIT_DROP                 = 1890,
	//#endif
	
    ACEFW_SET_FW_ADDR_BOOK          = ACEFW_BASE_CMD_ID,
    ACEFW_SET_FW_ADDR_BOOK_ITEM     = 2001,
    ACEFW_DEL_FW_ADDR_BOOK          = 2002,
    ACEFW_GET_FW_ADDR_BOOK_SIZE     = 2003,
    ACEFW_GET_FW_ADDR_BOOK          = 2004,
    ACEFW_GET_FW_ADDR_BOOK_ITEM_SIZE= 2005,
    ACEFW_GET_FW_ADDR_BOOK_ITEM     = 2006,
    ACEFW_SET_FW_SERVICE            = 2007,
    ACEFW_SET_FW_SERVICE_ITEM       = 2008,
    ACEFW_DEL_FW_SERVICE            = 2009,
    ACEFW_GET_FW_SERVICE_SIZE       = 2010,
    ACEFW_GET_FW_SERVICE            = 2011,
    ACEFW_GET_FW_SERVICE_ITEM_SIZE  = 2012,
    ACEFW_GET_FW_SERVICE_ITEM       = 2013,
    ACEFW_ADD_FW_RULE               = 2014,
    ACEFW_DEL_FW_RULE               = 2015,
    ACEFW_MOD_FW_RULE               = 2016,
    ACEFW_MOVE_FW_RULE              = 2017,
    ACEFW_GET_FW_RULE_IF_SIZE       = 2018,
    ACEFW_GET_FW_RULE_IF            = 2019,
    ACEFW_GET_FW_RULE_DETAIL_SIZE   = 2020,
    ACEFW_GET_FW_RULE_DETAIL        = 2021,
    ACEFW_SET_REPORTER_NETLINK      = 2022,
    ACEFW_SCHEDULE_STATUS_CHANGE    = 2023,
    ACEFW_CLEAR_SERVICE             = 2024,
    ACEFW_CLEAR_ADDR_BOOK           = 2025,
    ACEFW_CLEAR_POLICY              = 2026,
    ACE_FW_QUERY_IP_STATUS          = 2027,
    ACE_FW_IP_AUTH_NOTIFY           = 2028,
    ACE_FW_GET_ONLINE_IP_NUM        = 2029,
    ACE_FW_GET_ONLINE_IP_ALL        = 2030,
    ACEFW_GET_RESTORE_SERVICE_SIZE  = 2031,
    ACEFW_GET_RESTORE_SERVICE       = 2032,
    ACEFW_SET_MANAGE_PORT           = 2033,
    ACEFW_GET_MANAGE_PORT           = 2034,
    ACEFW_GET_PERMIT_PORT           = 2035,
    ACEFW_GET_PPPOE_CONFIG          = 2036,
    ACEFW_SET_PPPOE_CONFIG          = 2037,
    ACEFW_DEL_PPPOE_CONFIG          = 2038,
    ACEFW_USER_GET_DEV_TYPE         = 2039,
    ACEFW_SET_IPSEC_RULE            = 2040,
    ACEFW_CLEAR_IPSEC_RULE          = 2041,
    ACEFW_TRANSLATE_IPSEC_ADDRESS  = 2042,
    ACEFW_IS_CUSTOML7SERVICE_USED  = 2043,
    ACE_FW_IP_BLACKLIST_NOTIFY     = 2044,
    ACE_FW_IP_GETMAC_NOTIFY        = 2045,
    ACEFW_CLEAR_POLICY_COUNTER     = 2046,
    ACEFW_INSERT_FW_RULE           = 2047,
    ACE_FW_IP_AUTH_NOTIFY_ALL      = 2048,

    ACEFW_SET_SNIFFER_MODE        = 2049,
    ACEFW_GET_SNIFFER_MODE        = 2050,

    ACEFW_ADD_FW_RULE_NMC               = 2051,
    ACEFW_GET_FW_RULE_IF_SIZE_NMC       = 2052,
    ACEFW_GET_FW_RULE_IF_NMC            = 2053,
    ACEFW_GET_FW_RULE_DETAIL_SIZE_NMC   = 2054,
    ACEFW_GET_FW_RULE_DETAIL_NMC        = 2055,
    ACEFW_CLEAR_POLICY_NMC              = 2056,
    ACE_FW_QUERY_IP_STATUS2             = 2057,
    ACEFW_CLEAR_KERNEL_NMC     = 2058,
    
    HYTF_ACCESS_RULE_NOTIFY             = 2059,
    ACEFW_PARSE_DNS_RESULT              = 2060,
    HYTF_NO_AUDIT_KEY_NOTIFY             = 2061,
    HYTF_PROTOCOL_OFFSET_SET                  = 2062,
    HYTF_PROTOCOL_OFFSET_GET                  = 2063,

    HYTF_IP_DEVICE_TYPE_NOTIFY           = 2064,
    HYTF_UAC_MACCHECKER_NOTIFY        = 2065,   /* lijian 2013-06-03 */
    HYTF_REACH_RATIO_BLACKLIST_NOTIFY  = 2066,
    HYTF_IP_WEIXIN_URL_PASS_NOTIFY      = 2067,

    HYTF_SESSION_AGEOUT_NOTIFY      = 2068,
    HYTF_SO_ORIGINAL_DST_GET = 2069,
    HYTF_SESSION_EXT_KEEP_NOTIFY      = 2070,
	HYTF_GET_MOBILE_TYPE_INFO = 2071,

    HYTF_FW_DEVICE_RELOAD_START = 2072,
    HYTF_FW_DEVICE_RELOAD_END = 2073,

    HYTF_UAC_TMP_PASS_NOTIRY = 2074,
    HYTF_AWIFI_PREAUTH_NOTIRY = 2075,
    ACEFW_GET_FW_PREDEFINED_SERVICE_SIZE = 2076,
    HYTF_SET_KERNEL_FLAG_BY_INDEX = 2077,
   	HYTF_GET_KERNEL_FLAG_BY_INDEX = 2078,
	ACEFW_SET_SESSION_ACCELERATE_AGING = 2079,

    HYTF_WQA_STATUS_SET   = 2080,
    HYTF_WQA_STATUS_GET   = 2081, 
    HYTF_BLR_POLICY_SET   = 2082,
    HYTF_BLR_POLICY_GET   = 2083, 

    HYTF_PROXY_CONNECTION_INFO_GET = 2084,

    HYTF_DYNAMIC_ROUTE_STATUS_SET = 2085,

    CLOUD_SECURITY_SET                 = 2086,
    CLOUD_SECURITY_GET                 = 2087,
    CLOUD_SECURITY_IP_UPDATE              = 2088,

    HYTF_GET_AUTH_ENTRY = 2089,
	HYTF_L7_QUEUE_NOTIFY = 2090,
    HYTF_GET_ALL_IPSEC_RULES = 2091,
	HYTF_GET_ALL_IPSEC_RULES_SIZE = 2092,

    HYTF_NETWORK_ZONE_NOTIRY = 2093,

    ACEFW_SET_CA_DISTRIBUTE        = 2094,
    ACEFW_GET_CA_DISTRIBUTE        = 2095,

    ACEFW_SET_CA_DISTRIBUTE_STATUS = 2096,
    ACEFW_GET_CA_DISTRIBUTE_STATUS = 2097,

    ALG_CFG_SET = 2098,
    ALG_CFG_GET = 2099,

    ACEFW_MAX_CMD_ID = 2099,

    ACENAT_ADD_PAT_RULE = ACENAT_BASE_CMD_ID,//2100
    ACENAT_DEL_PAT_RULE,
    ACENAT_GET_PAT_RULE_DETAIL_SIZE,
    ACENAT_GET_PAT_RULE_DETAIL,
    ACENAT_MOD_PAT_RULE,

    ACENAT_MOVE_PAT_RULE,//2105
    ACENAT_ADD_STATICNAT_RULE,
    ACENAT_DEL_STATICNAT_RULE,
    ACENAT_GET_STATICNAT_RULE_DETAIL_SIZE,
    ACENAT_GET_STATICNAT_RULE_DETAIL,

    ACENAT_ADD_SERVICEMAPPING_RULE,//2110
    ACENAT_DEL_SERVICEMAPPING_RULE,
    ACENAT_GET_SERVICEMAPPING_RULE_DETAIL_SIZE,
    ACENAT_GET_SERVICEMAPPING_RULE_DETAIL,
    ACENAT_GET_STATICNAT_RULE_BY_INDEX,

    ACENAT_GET_SERVICEMAPPING_RULE_BY_INDEX,//2115
    ACENAT_GET_PAT_RULE_BY_INDEX,
    ACENAT_CLEAR_PAT_RULE_COUNTER,
    ACENAT_CLEAR_NAT_RULE_COUNTER,
    ACENAT_CLEAR_SERVICEMAPPING_RULE_COUNTER,

    ACENAT_INSERT_PAT_RULE,/*2120*/
    ACENAT_MODIFY_STATICNAT_RULE,
    ACENAT_MODIFY_SERVICEMAPPING_RULE,    
    ACENAT_SYNC_CMMAPPING_RULE,
    ACENAT_CLEAR_CMMAPPING_RULE,

    ACENAT_INSERT_SERVICEMAPPING_RULE,//2125
    ACENAT_MOVE_SERVICEMAPPING_RULE,    
    ACENAT_INSERT_STATICNAT_RULE,   
    ACENAT_MOVE_STATICNAT_RULE, 
    ACENAT66_ADD_PAT_RULE,
    
    ACENAT66_DEL_PAT_RULE,/*2130*/
    ACENAT66_GET_PAT_RULE_DETAIL_SIZE,
    ACENAT66_GET_PAT_RULE_DETAIL,
    ACENAT66_MOD_PAT_RULE,
    ACENAT66_MOVE_PAT_RULE,
    
    ACENAT66_ADD_STATICNAT_RULE,/*2135*/
    ACENAT66_DEL_STATICNAT_RULE,
    ACENAT66_GET_STATICNAT_RULE_DETAIL_SIZE,
    ACENAT66_GET_STATICNAT_RULE_DETAIL,
    ACENAT66_ADD_SERVICEMAPPING_RULE,
 
    ACENAT66_DEL_SERVICEMAPPING_RULE,/*2140*/
    ACENAT66_GET_SERVICEMAPPING_RULE_DETAIL_SIZE,
    ACENAT66_GET_SERVICEMAPPING_RULE_DETAIL,
    ACENAT66_GET_STATICNAT_RULE_BY_INDEX,
    ACENAT66_GET_SERVICEMAPPING_RULE_BY_INDEX,
    
    ACENAT66_GET_PAT_RULE_BY_INDEX,/*2145*/
    ACENAT66_CLEAR_PAT_RULE_COUNTER,
    ACENAT66_CLEAR_NAT_RULE_COUNTER,
    ACENAT66_CLEAR_SERVICEMAPPING_RULE_COUNTER,
    ACENAT66_INSERT_PAT_RULE,
    
    ACENAT66_MODIFY_STATICNAT_RULE,/*2150*/
    ACENAT66_MODIFY_SERVICEMAPPING_RULE,    
    ACENAT66_SYNC_CMMAPPING_RULE,
    ACENAT66_CLEAR_CMMAPPING_RULE,
    ACENAT66_INSERT_SERVICEMAPPING_RULE,  
    
    ACENAT66_MOVE_SERVICEMAPPING_RULE,/*2155*/
    ACENAT66_INSERT_STATICNAT_RULE,   
    ACENAT66_MOVE_STATICNAT_RULE,
    HYLAB_NAT66_NDP_PROXY_GET,
    ACENAT64_ADD_PAT_RULE,

    ACENAT64_DEL_PAT_RULE,/*2160*/
    ACENAT64_GET_PAT_RULE_DETAIL_SIZE,
    ACENAT64_GET_PAT_RULE_DETAIL,
    ACENAT64_MOD_PAT_RULE,
    ACENAT64_MOVE_PAT_RULE,

    ACENAT64_ADD_STATICNAT_RULE,/*2165*/
    ACENAT64_DEL_STATICNAT_RULE,
    ACENAT64_GET_STATICNAT_RULE_DETAIL_SIZE,
    ACENAT64_GET_STATICNAT_RULE_DETAIL,
    ACENAT64_ADD_SERVICEMAPPING_RULE,
 
    ACENAT64_DEL_SERVICEMAPPING_RULE,/*2170*/
    ACENAT64_GET_SERVICEMAPPING_RULE_DETAIL_SIZE,
    ACENAT64_GET_SERVICEMAPPING_RULE_DETAIL,
    ACENAT64_GET_STATICNAT_RULE_BY_INDEX,
    ACENAT64_GET_SERVICEMAPPING_RULE_BY_INDEX,
    
    ACENAT64_GET_PAT_RULE_BY_INDEX,/*2175*/
    ACENAT64_CLEAR_PAT_RULE_COUNTER,
    ACENAT64_CLEAR_NAT_RULE_COUNTER,
    ACENAT64_CLEAR_SERVICEMAPPING_RULE_COUNTER,
    ACENAT64_INSERT_PAT_RULE,
    
    ACENAT64_MODIFY_STATICNAT_RULE,/*2180*/
    ACENAT64_MODIFY_SERVICEMAPPING_RULE,    
    ACENAT64_SYNC_CMMAPPING_RULE,
    ACENAT64_CLEAR_CMMAPPING_RULE,
    ACENAT64_INSERT_SERVICEMAPPING_RULE,  
    
    ACENAT64_MOVE_SERVICEMAPPING_RULE,/*2185*/
    ACENAT64_INSERT_STATICNAT_RULE,   
	ACENAT64_MOVE_STATICNAT_RULE,
	ACENAT64_PAT64_ADDR_PREFIX_SET,
	ACENAT64_PAT64_ADDR_PREFIX_GET,

    HYNAT44_ADD_NAT44_RULE,/*2190*/
    HYNAT44_DEL_NAT44_RULE,
    HYNAT44_GET_NAT44_RULE_DETAIL_SIZE,
    HYNAT44_GET_NAT44_RULE_DETAIL,
    HYNAT44_MOD_NAT44_RULE,

    HYNAT44_MOVE_NAT44_RULE,/*2195*/
    HYNAT44_GET_NAT44_RULE_BY_INDEX,
    HYNAT44_INSERT_NAT44_RULE,
    HYNAT44_CLEAR_NAT44_RULE_COUNTER,
    HYNAT44_DPORT_MAPPING_VAILD,
/*
    NAT_RULE_ADD,
    NAT_RULE_INSERT,
    NAT_RULE_DELETE,
    NAT_RULE_MODIFY,
    NAT_RULE_MOVE,
    NAT_RULE_GET_SIZE,
    NAT_RULE_GET,
    NAT_RULE_SET_STATUS,
    NAT_RULE_CLEAR_COUNTER, 
*/
    ACENAT_MAX_CMD_ID,

    ACEMANAGEWHITELIST_SET_RULE = ACEMANAGEWHITELIST_BASE_CMD_ID,
    ACEMANAGEWHITELIST_DEL_RULE,
    ACEMANAGEWHITELIST_GET_RULE_DETAIL_SIZE,
    ACEMANAGEWHITELIST_GET_RULE_DETAIL,
    ACEMANAGEWHITELIST_MOD_RULE,
    ACEMANAGEWHITELIST_GET_STATUS,
    ACEMANAGEWHITELIST_SET_STATUS,
    ACEMANAGEWHITELIST_GET_BY_INDEX,
    ACEMANAGEWHITELIST_INSERT_RULE,
    ACEMANAGEWHITELIST_MOVE_RULE,
    ACEMANAGEWHITELIST_CLEAN,
    ACEMANAGEWHITELIST_MAX_CMD_ID,

    ACECAR_SET_LINE_BANDWIDTH = ACECAR_BASE_CMD_ID,
    ACECAR_GET_LINE_BANDWIDTH,
    ACECAR_GET_LINE_NUMBER,
    ACECAR_SET_PORTTRUNK_BANDWIDTH,
    ACECAR_GET_PORTTRUNK_BANDWIDTH,
    ACECAR_MAX_CMD_ID,

    ACE_PFC_RULE_ADD = ACE_PFC_BASE_CMD_ID,
    ACE_PFC_RULE_DEL,
    ACE_PFC_RULE_MODIFY,
    ACE_PFC_RULE_INSERT,
    ACE_PFC_RULE_MOVE,
    ACE_PFC_RULE_CLEAR,
    ACE_PFC_RULE_SIZE,
    ACE_PFC_RULE_GET,
    ACE_PFC_RULE_GET_NEXT,
    ACE_PFC_RULE_CLEAR_COUNTER,
    ACE_PFC_RULE_ADD_NMC              = 2410,
    ACE_PFC_RULE_SIZE_NMC    = 2411,
    ACE_PFC_RULE_GET_NMC      = 2412,
    ACE_PFC_RULE_GET_NEXT_NMC   = 2413,
    ACE_PFC_RULE_CLEAR_NMC        = 2414,
    ACE_PFC_RULE_COPY = 2415,
    ACE_PFC_MAX_CMD_ID,

    ACE_UFC_RULE_ADD = ACE_UFC_BASE_CMD_ID,
    ACE_UFC_RULE_DEL,
    ACE_UFC_RULE_MODIFY,
    ACE_UFC_RULE_INSERT,
    ACE_UFC_RULE_MOVE,
    ACE_UFC_RULE_CLEAR,
    ACE_UFC_RULE_SIZE,
    ACE_UFC_RULE_GET,
    ACE_UFC_RULE_GET_NEXT,
    ACE_UFC_REPORTER_GET_SERVICE_BY_IP, 
    ACE_UFC_REPORTER_GET_IP_BY_SERVICE, 
    ACE_POLICY_OBJECT_KERNEL_ADD,
    ACE_POLICY_OBJECT_KERNEL_DEL,
    ACE_POLICY_OBJECT_KERNEL_MODIFY,
    ACE_POLICY_OBJECT_KERNEL_CLEAR,
    ACE_POLICY_OBJECT_KERNEL_SIZE,
    ACE_POLICY_OBJECT_KERNEL_GET,
    ACE_POLICY_OBJECT_KERNEL_GET_NEXT,
    ACE_UFC_RULE_CLEAR_COUNTER,
    ACE_UFC_RULE_ADD_NMC              = 2519,
    ACE_UFC_RULE_SIZE_NMC    = 2520,
    ACE_UFC_RULE_GET_NMC      = 2521,
    ACE_UFC_RULE_GET_NEXT_NMC   = 2522,
    ACE_UFC_RULE_CLEAR_NMC        = 2523,
    ACE_UFC_RULE_ADD_3PARTY              = 2524,
    ACE_UFC_RULE_SIZE_3PARTY    = 2525,
    ACE_UFC_RULE_GET_3PARTY      = 2526,
    ACE_UFC_RULE_GET_NEXT_3PARTY   = 2527,
    ACE_UFC_RULE_CLEAR_3PARTY        = 2528,
    ACE_UFC_MAX_CMD_ID,

 //   ACE_L7FILTER_SERVICE_GET = ACE_L7FILTER_BASE_CMD_ID,/*GET SERVICE*/
//  ACE_L7FILTER_SERVICE_GET_RSP,

    L7FILTER_CFG_MSG = ACE_L7FILTER_BASE_CMD_ID,
    /*below is service get*/
    L7FILTER_SERVICE_GET = 2600,
    L7FILTER_SERVICE_GET_RET = 2601,
    /*update pattern*/
    L7FILTER_UPDATE_PATTERN = 2619,
    L7FILTER_UPDATE_PATTERN_RET = 2620,
    L7FILTER_NOTIFY_L7_FEATURE = 2621,

    /*update bbs feature*/
    L7FILTER_UPDATE_BBS_FEATURE = 2635,
    L7FILTER_UPDATE_BBS_FEATURE_RET = 2636,

    /*reporter filesize upper*/
    L7FILTER_REPORTER_FILESIZE_UPPER = 2637,
    L7FILTER_REPORTER_FILESIZE_UPPER_RET = 2638,

    /*l7-filter debug*/
    L7FILTER_DEBUG = 2639,
    L7FILTER_DEBUG_RET = 2640,
    L7FILTER_SERVICE_GET_SIZE = 2641,
    L7FILTER_SERVICE_GET_SIZE_RET = 2642,

    L7FILTER_UPDATE_CUSTOM_PATTERN = 2643,
    L7FILTER_UPDATE_CUSTOM_PATTERN_RET = 2644,

    AUTO_TEST_ACCOUNT_NOTIFY = 2645,

    L7DEVICE_NOTIFY_L7_FEATURE = 2646,
    
    ACE_L7FILTER_MAX_CMD_ID,

    ACE_SYSTEMCFG_SAVE = ACE_SYSTEMCFG_BASE_CMD_ID,
    ACE_FDEBUG_CMD_ID, /*for fdebug command for busybox*/
    ACE_VDEBUG_CMD_ID, /*for vdebug command for busybox*/
    ACE_FDEBUG_SYS_CMD_ID, /*for fdebug command for busybox*/
    ACE_VDEBUG_SYS_CMD_ID, /*for vdebug command for busybox*/
    
    POLICY_ROUTE_CMD_ID, //2805
    POLICY_ROUTE_ADD = POLICY_ROUTE_CMD_ID, //2805
    POLICY_ROUTE_INSERT,
    POLICY_ROUTE_MODIFY,
    POLICY_ROUTE_MOVE,
    POLICY_ROUTE_DELETE,
    POLICY_ROUTE_DELETE_ALL,//2810
    
    POLICY_ROUTE_GET_SIZE, 
	POLICY_ROUTE_GET,
    POLICY_ROUTE_SET_STATUS,
	POLICY_ROUTE_CLEAR_COUNTER, //2814
	POLICY_ROUTE_CMD_ID_MAX = POLICY_ROUTE_CLEAR_COUNTER, //2814
    ACE_PORT_STATUS_NOTIFY, /*2815*/
    ACE_HOTEL_HOTPLUG_VLAN_GET, /*2816*/
    ACE_HOTEL_HOTPLUG_VLAN_SET, 
    POLIC_ROUTE_SET_GW_STATUS,

    ISP_POOL_CMD_ID, //2818
	ISP_POOL_ADD = ISP_POOL_CMD_ID, //2818
	ISP_POOL_MODIFY,
	ISP_POOL_DEL,
	ISP_POOL_DEL_ALL,
	ISP_POOL_GET_SIZE,
	ISP_POOL_GET,
	ISP_POOL_CLEAR_COUNTER, //2823
	ISP_POOL_CMD_ID_MAX = ISP_POOL_CLEAR_COUNTER, //2823

    PERSISTENT_ROUTE_ADD,/*2824*/
    PERSISTENT_ROUTE_MODIFY,
    PERSISTENT_ROUTE_GET_SIZE,
    PERSISTENT_ROUTE_GET_NEXT,
    PERSISTENT_ROUTE_DEL,
    PERSISTENT_ROUTE_DEL_ALL,/*2829*/

    PERSISTENT_ROUTE_TIMEOUT_SET,/*2830*/
    PERSISTENT_ROUTE_TIMEOUT_GET,/*2831*/
    ISP_POOL_RESET_COUNTERS,/*2832*/
    POLICY_ROUTE_RESET_COUNTERS,/*2833*/
    PERSISTENT_ROUTE_RESET_COUNTERS,/*2834*/

    ACE_FDEBUG_FREEDOM_CMD_ID, /*2862*/
    ACE_VDEBUG_FREEDOM_CMD_ID, /*2863*/

    ACEFW6_SET_FW_RULE               = ACEFW6_BASE_CMD_ID, /*2900*/
    ACEFW6_DEL_FW_RULE               = 2901,
    ACEFW6_MOD_FW_RULE               = 2902,
    ACEFW6_MOVE_FW_RULE              = 2903,
    ACEFW6_GET_FW_RULE_IF_SIZE       = 2904,
    ACEFW6_GET_FW_RULE_IF            = 2905,
    ACEFW6_GET_FW_RULE_DETAIL_SIZE   = 2906,
    ACEFW6_GET_FW_RULE_DETAIL        = 2907,
    ACEFW6_CLEAR_POLICY              = 2908,
    ACEFW6_CLEAN_COUNTERS        = 2909,
    ACEFW6_MAX_CMD_ID,

    ACE_SNMP_TRAP_NOTIFY,
    FILEFILTER_IS_REF_BY_PFC_NOTIFY,
    LINK_PPPOE_CONNECT_RESTART_NOTIFY = 2950,
    HYLAB_SOCKET_FULL_NOTIFY = 2951,

    ACE_USER_AUTH_SET_NETLINK_ID = ACE_USER_AUTH_BASE_ID, /*3000*/
    ACE_USER_AUTH_SET_POLICY,
    ACE_USER_AUTH_MOVE_POLICY,
    ACE_USER_AUTH_MODIFY_POLICY,
    ACE_USER_AUTH_DEL_POLICY,
    ACE_USER_AUTH_CLEAR_POLICY,
    ACE_USER_IP_LOGIN,
    ACE_USER_IP_LOGOUT,
    ACE_USER_AUTH_SET_NETLINK_NFLOG_ID,

    ACE_SMP_SET,
    ACE_SMP_DEL,
    ACE_SMP_CLEAR,
    ACE_SMP_DETECT,
	
	HYTF_NETWORK_ZONE_ADD,
    HYTF_NETWORK_ZONE_EDIT,
    HYTF_NETWORK_ZONE_DEL,
    HYTF_NETWORK_ZONE_GET_NUM,
    HYTF_NETWORK_ZONE_GET_BY_IDX,
    HYTF_NETWORK_ZONE_SET_STATUS,
    HYTF_NETWORK_ZONE_GET_STATUS,
    HYTF_NETWORK_ZONE_CLEAR,
    HYTF_NETWORK_ZONE_CURRENT_STATUS,
    HYTF_NETWORK_ZONE_CLEAR_NMC,

    ACE_PROXY_SERVER_SET_IP = 3100,
    ACEFW_SSL_PROXY_CFG_SET = 3101,
    ACEFW_SSL_PROXY_CACHE_CLEAR = 3102,

    ACE_SET_URL_CATAGORY_MATCH_REFER = 3103, 
    ACE_GET_URL_CATAGORY_MATCH_REFER = 3104,  
    
	IPV6_POLICY_ROUTE_ADD = 3235,
    IPV6_POLICY_ROUTE_INSERT,
    IPV6_POLICY_ROUTE_MODIFY,
    IPV6_POLICY_ROUTE_MOVE,
    IPV6_POLICY_ROUTE_DELETE,

    IPV6_POLICY_ROUTE_DELETE_ALL,
    IPV6_POLICY_ROUTE_GET_SIZE,
    IPV6_POLICY_ROUTE_GET_ENTRY_BY_INDEX,
    IPV6_POLICY_ROUTE_SET_STATUS,
    IPV6_POLICY_ROUTE_SET_GW_STATUS,

    IPV6_POLICY_ROUTE_RESET_COUNTERS,
    IPV6_ISP_POOL_ADD,
    IPV6_ISP_POOL_MODIFY,
    IPV6_ISP_POOL_GET_SIZE,
    IPV6_ISP_POOL_GET_NEXT,

    IPV6_ISP_POOL_DEL,
    IPV6_ISP_POOL_DEL_ALL,
    IPV6_ISP_POOL_RESET_COUNTERS,
    IPV6_PERSISTENT_ROUTE_ADD,
    IPV6_PERSISTENT_ROUTE_MODIFY,

    IPV6_PERSISTENT_ROUTE_GET_SIZE,
    IPV6_PERSISTENT_ROUTE_GET_NEXT,
    IPV6_PERSISTENT_ROUTE_DEL,
    IPV6_PERSISTENT_ROUTE_DEL_ALL,
    IPV6_PERSISTENT_ROUTE_TIMEOUT_SET,

	IPV6_PERSISTENT_ROUTE_TIMEOUT_GET,
	IPV6_PERSISTENT_ROUTE_RESET_COUNTERS,

    ACENAT46_ADD_PAT_RULE = 3300,
    ACENAT46_DEL_PAT_RULE,
    ACENAT46_GET_PAT_RULE_DETAIL_SIZE,
    ACENAT46_GET_PAT_RULE_DETAIL,
    ACENAT46_MOD_PAT_RULE,

    ACENAT46_MOVE_PAT_RULE,/*3305*/
    ACENAT46_ADD_STATICNAT_RULE,
    ACENAT46_DEL_STATICNAT_RULE,
    ACENAT46_GET_STATICNAT_RULE_DETAIL_SIZE,
    ACENAT46_GET_STATICNAT_RULE_DETAIL,

    ACENAT46_ADD_SERVICEMAPPING_RULE,/*3310*/
    ACENAT46_DEL_SERVICEMAPPING_RULE,
    ACENAT46_GET_SERVICEMAPPING_RULE_DETAIL_SIZE,
    ACENAT46_GET_SERVICEMAPPING_RULE_DETAIL,
    ACENAT46_GET_STATICNAT_RULE_BY_INDEX,

    ACENAT46_GET_SERVICEMAPPING_RULE_BY_INDEX,/*3315*/
    ACENAT46_GET_PAT_RULE_BY_INDEX,
    ACENAT46_CLEAR_PAT_RULE_COUNTER,
    ACENAT46_CLEAR_NAT_RULE_COUNTER,
    ACENAT46_CLEAR_SERVICEMAPPING_RULE_COUNTER,

    ACENAT46_INSERT_PAT_RULE,/*3320*/
    ACENAT46_MODIFY_STATICNAT_RULE,
    ACENAT46_MODIFY_SERVICEMAPPING_RULE,    
    ACENAT46_SYNC_CMMAPPING_RULE,
    ACENAT46_CLEAR_CMMAPPING_RULE,

    ACENAT46_INSERT_SERVICEMAPPING_RULE,/*3325*/    
    ACENAT46_MOVE_SERVICEMAPPING_RULE,
    ACENAT46_INSERT_STATICNAT_RULE,   
    ACENAT46_MOVE_STATICNAT_RULE,
    ACE_SSL_CFG_MSG = ACE_SSL_BASE_ID,/*10000*/
    ACE_SSLCTL_ADD,
    ACE_SSLCTL_ADD_RET,
    ACE_SSLCTL_DEL,
    ACE_SSLCTL_DEL_RET,
    ACE_SSLCTL_MODIFY,
    ACE_SSLCTL_MODIFY_RET,
    ACE_SSLCTL_INSERT,
    ACE_SSLCTL_INSERT_RET,
    ACE_SSLCTL_MOVE,
    ACE_SSLCTL_MOVE_RET,
    ACE_SSLCTL_CLEAR,
    ACE_SSLCTL_CLEAR_RET,
    ACE_SSLCTL_SIZE,
    ACE_SSLCTL_SIZE_RET,
    ACE_SSLCTL_GET,
    ACE_SSLCTL_GET_RET,
    ACE_SSLCTL_MODIFY_STAUTS,
    ACE_SSLCTL_MODIFY_STAUTS_RET,
    ACE_SSLLIB_GET,
    ACE_SSLLIB_GET_RET,
    ACE_SSLLIB_DEL,
    ACE_SSLLIB_DEL_RET,
    #ifdef SSL_CA
    ACE_SSLCA_GET,
    ACE_SSLCA_GET_RET,
    ACE_CA_DETAIL_GET,
    ACE_CA_DETAIL_GET_RET,
    #endif
    ACE_SSL_CTL_GET,
    ACE_SSL_CTL_GET_RET,
    ACE_LOAD_SSL,
    ACE_LOAD_SSL_RET,

    //扫描内网MAC
    ACE_NETBIOS_CFG_MSG = 11000,/*11000*/   
    ACE_NETBIOS_SCAN,
    ACE_NETBIOS_SCAN_RET,
    ACE_NETBIOS_SIZE,
    ACE_NETBIOS_SIZE_RET,
    ACE_NETBIOS_GET,
    ACE_NETBIOS_GET_RET,
    ACE_NETBIOS_DEL,
    ACE_NETBIOS_DEL_RET,
    ACE_NETSNMP_SET,
    ACE_NETSNMP_SET_RET,
    ACE_NETSNMP_GET,
    ACE_NETSNMP_GET_RET,

    ACE_NETBIOS_SCAN2,
    ACE_NETBIOS_SCAN_RET2,
    ACE_NETBIOS_SCAN3,
    ACE_NETBIOS_SCAN_RET3,

    ACE_USER_LOG_SET,
    ACE_USER_LOG_SET_RET,
    ACE_USER_LOG_GET,
    ACE_USER_LOG_GET_RET,
    ACE_USER_SNMP_SET,
    ACE_USER_SNMP_SET_RET,
    ACE_USER_SNMP_GET,
    ACE_USER_SNMP_GET_RET,
    ACE_USER_SMS_SET,
    ACE_USER_SMS_SET_RET,
    ACE_USER_SMS_GET,
    ACE_USER_SMS_GET_RET,

    HYTFFW_SYSTEM_DN_NAME_SET,
    HYTFFW_SYSTEM_DN_NAME_GET,

    ACE_NETSNMP_TEST,
    ACE_NETSNMP_TEST_RET,

    HYTFFW_SET_FW_ADDRBOOK = HYTFFW_ADDRBOOK_BASE_CMD_ID,//11100
    HYTFFW_GET_FW_ADDRBOOK_SIZE,
    HYTFFW_GET_FW_ADDRBOOK_NEXT,
    HYTFFW_DEL_FW_ADDRBOOK,
    HYTFFW_SET_FW_ADDRBOOK_GEOIP,//11104
    
    HYTFFW_RESET_FW_ADDRBOOK_GEOIP,
    HYTFFW_GET_FW_ADDRBOOK_GEOIP_SIZE,//11106
    HYTFFW_GET_FW_ADDRBOOK_GEOIP_NEXT,
    HYTFFW_GET_FW_ADDRBOOK_GEOIP_CUSTOM_SIZE,//11108
    HYTFFW_GET_FW_ADDRBOOK_GEOIP_CUSTOM_NEXT,//11109

    HYTFFW_ADDRBOOK_MAX_CMD_ID,

    HYTFFW_CLEAR_KERNEL_CONFIG = 11110,

    MAC_FILTER_OBJ_ADD = MAC_FILTER_OBJ_BASE_CMD_ID,
    MAC_FILTER_OBJ_EDIT,
    MAC_FILTER_OBJ_DEL,
    MAC_FILTER_OBJ_CLEAR,
    MAC_FILTER_OBJ_GET_SIZE,
    MAC_FILTER_OBJ_GET_BY_NAME,
    MAC_FILTER_OBJ_GET_BY_INDEX,
    MAC_FILTER_OBJ_CLEAR_NMC,
    MAC_FILTER_OBJ_CMD_ID_MAX,

    HYTFFW_NMC_FLAG_CONFIG = 11120,
    HYTFFW_SET_FW_DOMAIN_GROUP = HYTFFW_DOMAIN_GROUP_BASE_CMD_ID,//11150
	HYTFFW_GET_FW_DOMAIN_GROUP_SIZE,//11151
	HYTFFW_GET_FW_DOMAIN_GROUP_NEXT,//11152
	HYTFFW_DEL_FW_DOMAIN_GROUP,//11153
    HYTFFW_DOMAIN_GROUP_MAX_CMD_ID = 11160,

    HYLAB_ZTGW_REINIT_START = 11161,
    HYLAB_ZTGW_REINIT_COMPLETE,
    HYLAB_ZTGW_SET,
    HYLAB_ZTGW_DEL,
    HYLAB_ZTGW_GET_SIZE,
    HYLAB_ZTGW_GET_NEXT,

    HYLAB_ZTGW_MAX_CMD_ID = 11170,

    HYTFFW_SET_FW_CUSTOM_SERVICE_DOMAIN_GROUP = 11180,
	HYTFFW_GET_FW_CUSTOM_SERVICE_DOMAIN_GROUP_SIZE,
	HYTFFW_GET_FW_CUSTOM_SERVICE_DOMAIN_GROUP_NEXT,

    HYTFFW_WHITE_LIST_ADD = HYTFFW_WHILTE_LIST_BASE_CMD_ID,//11200
    HYTFFW_WHITE_LIST_GET_SIZE,
    HYTFFW_WHITE_LIST_GET_NEXT,
    HYTFFW_WHITE_LIST_DEL,
    HYTFFW_WHITE_LIST_DEL_ALL,
    HYTFFW_WHILTE_LIST_MAX_CMD_ID,

    HYTFFW_ARP_PROTECT_SET = 11220,
    HYTFFW_ARP_PROTECT_GET,
    HYTFFW_ARP_PROTECT_MANUAL_SET,
    HYTFFW_PKT_TRACE_SET,
    HYTFFW_PKT_TRACE_GET,
    UDP_MULTICAST_TO_LOCAL_SET, 
    UDP_MULTICAST_TO_LOCAL_GET,
        

    HYTFFW_TC_SET = HYTFFW_TC_BASE_CMD_ID,
    HYTFFW_TC_SIZE,
    HYTFFW_TC_MODIFY,
    HYTFFW_TC_GET,
    HYTFFW_TC_DEL,
    HYTFFW_TC_DEL_ALL,
    HYTFFW_TC_MOVE,
    HYTFFW_TC_INSERT,
    HYTFFW_TC_GET_ALL,
    HYTFFW_TC_CLEAR_COUNTER,
    HYTFFW_TC_SET_NMC,
    HYTFFW_TC_SIZE_NMC,
    HYTFFW_TC_GET_NMC,
    HYTFFW_TC_GET_ALL_NMC,
    HYTFFW_TC_PORTTRUNK_INIT,
    HYTFFW_TC_MAX_CMD_ID,

    HYTF_NPAUTH_PERMIT_RULE_ADD = HYTF_NOPASSAUTH_BASE_CMD_ID,
    HYTF_NPAUTH_PERMIT_RULE_DEL,
    HYTF_NPAUTH_PERMIT_RULE_MODIFY,
    HYTF_NPAUTH_PERMIT_RULE_INSERT,
    HYTF_NPAUTH_PERMIT_RULE_MOVE,
    HYTF_NPAUTH_PERMIT_RULE_CLEAR,
    HYTF_NPAUTH_PERMIT_RULE_SIZE,
    HYTF_NPAUTH_PERMIT_RULE_GET,
    HYTF_NPAUTH_PERMIT_RULE_GET_ALL,
    HYTF_NPAUTH_PERMIT_RULE_CLEAR_COUNTER,
    HYTF_NOPASSAUTH_MAX_CMD_ID,

    HYTF_SERVERPOOL_RULE_ADD     = 11450,
    HYTF_SERVERPOOL_RULE_MODIFY,
    HYTF_SERVERPOOL_RULE_DEL,
    HYTF_SERVERPOOL_RULE_SIZE,
    HYTF_SERVERPOOL_RULE_GET_NEXT,
    HYTF_SERVERPOOL_RULE_GET,
	HYTF_SERVERPOOL_RULE_CLEAR_COUNTER,
	HYTF_SERVERPOOL66_RULE_ADD,
	HYTF_SERVERPOOL66_RULE_MODIFY,
	HYTF_SERVERPOOL66_RULE_DEL,
	/*11460*/
	HYTF_SERVERPOOL66_RULE_SIZE,
	HYTF_SERVERPOOL66_RULE_GET_NEXT,
	HYTF_SERVERPOOL66_RULE_GET,
	HYTF_SERVERPOOL66_RULE_CLEAR_COUNTER,

    HYTF_SERVICECTL_RULE_ADD = HYTF_SERVICECTL_BASE_CMD_ID,
    HYTF_SERVICECTL_RULE_MODIFY,
    HYTF_SERVICECTL_RULE_DEL,
    HYTF_SERVICECTL_RULE_SIZE,
    HYTF_SERVICECTL_RULE_GET_NEXT,
    HYTF_SERVICECTL_RULE_GET,
    HYTF_CTRLUSER_SIZE,
    HYTF_CTRLUSER_GET_NEXT,
    HYTF_CTRLUSER_RELEASE,
    HYTF_SERVICECTL_RULE_MOVE,
    HYTF_CTRLUSER_QUOTA_ADD,
    HYTF_CTRLUSER_GET_BY_IP,
    HYTF_SERVICECTL_MAX_CMD_ID,

    HYTFFW_SET_FW6_ADDRBOOK6 = HYTFFW_ADDRBOOK6_BASE_CMD_ID,
    HYTFFW_DEL_FW6_ADDRBOOK6,
    HYTFFW_ADDRBOOK6_MAX_CMD_ID,

    HYTFFW_FEATURE_RULE_ADD = HYTFFW_FEATURE_RULE_CMD_ID,
    HYTFFW_FEATURE_RULE_CLEAN,
    HYTFFW_FEATURE_RULE_COMPLETE,
    HYTFFW_FEATURE_SIMU_PACKET,
    HYTFFW_URLCUSTOM_KERNEL_ADD,
    
    HYTFFW_URLCUSTOM_KERNEL_CLEAN,
    HYTFFW_URLLIB_KERNEL_SUMMARY,
    HYTFFW_URLLIB_KERNEL_ADD,
    HYTFFW_URLLIB_KERNEL_CLEAN,
    HYTFFW_URLLIB_KERNEL_GROUPNUM,

    HYTFFW_URLLIB_KERNEL_COMPLETE,  //11710
    HYTFFW_POLICY_OBJECT_KERNEL_ADD,
    HYTFFW_POLICY_OBJECT_KERNEL_CLEAN,
    HYTFFW_FEATURE_RULE_CUSTOM_CLEAN,
    HYTFFW_FEATURE_RULE_CUSTOM_COMPLETE,

    HYTFFW_FEATURE_RULE_CUSTOM_ADD,
    HYTFFW_FEATURE_L7_FEATURE_PID,
    HYTFFW_KEYWORD_KERNEL_ADD,
    HYTFFW_KEYWORD_KERNEL_CLEAN,
    HYTFFW_FILEFILTER_KERNEL_ADD,

    HYTFFW_FILEFILTER_KERNEL_CLEAN, //11720
    HYTFFW_HTTPAPP_KERNEL_SUMMARY,
    HYTFFW_HTTPAPP_KERNEL_ADD,
    HYTFFW_HTTPAPP_KERNEL_CLEAN,
    HYTFFW_HTTPAPP_KERNEL_COMPLETE,

    HYTFFW_URLWHITELIST_RULE_KERNEL_ADD,
    HYTFFW_URLWHITELIST_RULE_KERNEL_CLEAN,
    HYTFFW_HTTPAPP_FILTER_ADD,
    HYTFFW_HTTPAPP_FILTER_CLEAN,
    HYTFFW_HTTPAPP_FILTER_COMPLETE, 

    HYTFFW_CUSTOM_BBS_KERNEL_ADD, //11730
    HYTFFW_CUSTOM_BBS_KERNEL_CLEAN,
    HYTFFW_WEBSSO_KERNEL_SYNC,
    HYTFFW_AUDIT_RULE_URL_LOG_KERNEL_SYNC,
    HYTFFW_REPORTER_AUDIT_RULE_KERNEL_ADD,

    HYTFFW_REPORTER_AUDIT_RULE_KERNEL_CLEAN,
    HYTFFW_TERMINAL_PROMPT_OBJECT_KERNEL_ADD,
    HYTFFW_TERMINAL_PROMPT_OBJECT_KERNEL_CLEAN,
    HYTFFW_AUDIT_RULE_MAX_FILESIZE_KERNEL_SYNC,
    HYTFFW_AUDIT_RULE_CFG_KERNEL_SYNC,

    HYTFFW_WINDOWSSSO_KERNEL_SYNC, //11740
    HYTF_REGEX_AUTH_CFG,
    HYTFFW_WECHAT_AUTH_KERNEL_SYNC,    
    HYTFFW_GLOBALWHITELIST_RULE_KERNEL_ADD,
    HYTFFW_GLOBALWHITELIST_RULE_KERNEL_CLEAN,

    HYTFFW_FEATURE_FDEBUG_ID,
    HYTFFW_FEATURE_VDEBUG_ID,
	HYTFFW_MOBILE_USER_AGENT_KERNEL_ADD,
	HYTFFW_MOBILE_USER_AGENT_KERNEL_CLEAN,
    HYTF_URL_CATAGORY_GET_ID,

    CLOUD_SECURITY_URL_UPDATE, //11750
    AUTO_TEST_START_ID,
    AUTO_TEST_END_ID,
    HYTF_VPP_RESET_NOTIFY,
    HYTF_VPP_FEATURE_START,

    HYTF_CHECK_VPP_READY,
    HYTFFW_POLICY_OBJECT_KERNEL_COMPLETE,

	#ifdef CONFIG_NSAS
	AGENT_PACKET_TO_VPP=11757,
	#endif
    //warnning:I don't know where this ID was used.
    
    HYTFFW_GAME_DOMAIN_KERNEL_ADD = 11760,
    HYTFFW_GAME_DOMAIN_KERNEL_CLEAN,
    HYTFFW_XUNLEI_RESOURCE_DOMAIN_KERNEL_ADD,
	HYTFFW_XUNLEI_RESOURCE_DOMAIN_KERNEL_CLEAN,
	HYTFFW_SHAREIP_UA_FILTER_ADD,
	
	HYTFFW_SHAREIP_UA_FILTER_CLEAN,
	HYTFFW_SHAREIP_HOST_FILTER_ADD,
	HYTFFW_SHAREIP_HOST_FILTER_CLEAN,
	HYTFFW_FEATURE_RULE_MAX_CMD_ID,
    HYTFFW_JMS_SSO_KERNEL_SYNC,

    HYTFFW_CITYHOTSPOT_PORT_KERNEL_SYNC,//11770
    HYTFFW_SAAS_PATTERN_CLEAN,
	HYTFFW_SAAS_PATTERN_ADD,
    HYTFFW_SAAS_PATTERN_ADD_COMPLETE,
    HYTFFW_DHCP_SYSLOG_PORT_SYNC,
    HYTFFW_NCE_CAMPUS_SSO_PORT,


    HYTFFW_SHAREIP_BASE_ID = 11780,
    HYTFFW_SHAREIP_FDEBUG_ID,
    HYTFFW_SHAREIP_VDEBUG_ID,

    HYTFFW_SHAREIP_MAX_ID = 11790,
    
    HYTF_SMS_CMD_CFG = HYTF_SMS_CMD_ID,
    HYTF_SMS_CMD_SEND,
    HYTF_SMS_CMD_SEND_RESP,
    HYTF_HTTP_REDIRECT_WX_ADD,

    HYTF_OL_ADD = HYTF_LINK_OBJ_CMD_ID,
    HYTF_OL_EDIT,
    HYTF_OL_DEL,
    HYTF_OL_GET_NUM,
    HYTF_OL_GET,
    HYTF_OL_GET_BY_NAME,
    HYTF_OL_UPDATE_DHCP_GEATWAY,
    HYTF_OL_DEL_CHECK,
    HYTF_OL_PPPOE_CONNECT_PID_NOTIFY,
    HYTF_OL_DEL_CHECK2,
    HYTF_OL_CMD_ID_MAX,


    HYTF_MIRROR_POLICY_SET = HYTF_MIRROR_POLICY_CMD_ID,
    HYTF_MIRROR_POLICY_GET,
    HYTF_MIRROR_POLICY_GET_NEXT,
    HYTF_MIRROR_POLICY_DEL,
    HYTF_MIRROR_POLICY_CLEAR,
    HYTF_MIRROR_POLICY_CLEAR_COUNT,
    HYTF_MIRROR_POLICY_GET_NUMBER,
    HYTF_MIRROR_POLICY_ID_MAX,
	
	HYTF_SMP_POLICY_SET = HYTF_SMP_POLICY_CMD_ID,
    HYTF_SMP_POLICY_GET,
    HYTF_SMP_POLICY_GET_NEXT,
    HYTF_SMP_POLICY_DEL,
    HYTF_SMP_POLICY_CLEAR,
    HYTF_SMP_POLICY_CLEAR_COUNT,
    HYTF_SMP_POLICY_GET_NUMBER,
    HYTF_SMP_POLICY_SET_SERVER,
    HYTF_SMP_POLICY_GET_SERVER,
    HYTF_SMP_POLICY_ID_MAX,

    
    IPS_RULE_ADD = IPS_RULE_CMD_ID,//12100
    IPS_RULE_DEL,
    IPS_RULE_MODIFY,
    IPS_RULE_GET_SIZE,
    IPS_RULE_GET_BY_INDEX,
    IPS_RULE_GET_BY_NAME,
    IPS_RULE_SET_STATUS,//12106
    IPS_RULE_MOVE_TO,
    IPS_RULE_RESET_COUNT,
    IPS_RULE_INCREASE_COUNT,
    IPS_RULE_HEART_BEAT,
    IPS_RULE_SURICATA_RESULT,//12111
    IPS_RULE_GET_ZONE,
    IPS_RULE_CMD_ID_MAX,
    
    AV_RULE_ADD = AV_RULE_CMD_ID,//12200
    AV_RULE_DEL,
    AV_RULE_MODIFY,
    AV_RULE_GET_SIZE,
    AV_RULE_GET_BY_INDEX,
    AV_RULE_GET_BY_NAME,//12205
    AV_RULE_SET_STATUS,
    AV_RULE_MOVE_TO,
    AV_RULE_RESET_COUNT,
    AV_RULE_INCREASE_COUNT,
    AV_RULE_INSERT, //12210
    AV_EXCLUDE_ADDR_SET,
    AV_EXCLUDE_ADDR_GET,
    AV_RULE_CMD_ID_MAX,
    
    SAFETY_ZONE_ADD = SAFETY_ZONE_CMD_ID,
    SAFETY_ZONE_DEL,
    SAFETY_ZONE_MOD,
    SAFETY_ZONE_GET_SIZE,
    SAFETY_ZONE_GET_BY_INDEX,
    SAFETY_ZONE_GET_BY_NAME,
    HYTF_SZ_GET_LIST,
    SAFETY_ZONE_GET_ALL_SIZE,
    SAFETY_ZONE_GET_ALL_BY_INDEX,
    SAFETY_ZONE_CMD_ID_MAX,
        
    AD_POLICY_ADD = AD_POLICY_CMD_ID,
    AD_POLICY_MOD,
    AD_POLICY_DEL,
    AD_POLICY_GET_SIZE,
    AD_POLICY_GET_BY_IDX,
    AD_POLICY_GET_BY_NAME,
    AD_POLICY_CHANGE_STATUS,
    AD_POLICY_CLEAR,
    AD_INNER_POLICY_SET,
    AD_INNER_POLICY_GET,
    AD_POLICY_CLEAR_COUNT,
    AD_THRESHOLD_STUDY_SET,
    AD_THRESHOLD_STUDY_GET,
    AD_THRESHOLD_STUDY_RESULT_GET,
    AD_THRESHOLD_STUDY_APPLY_SET,
    AD_POLICY_ID_MAX,
    
    WAF_RULE_ADD = WAF_RULE_CMD_ID,//12500
    WAF_RULE_DEL,
    WAF_RULE_MODIFY,
    WAF_RULE_GET_SIZE,
    WAF_RULE_GET_BY_INDEX,
    WAF_RULE_GET_BY_NAME,
    WAF_RULE_SET_STATUS,//12506
    WAF_RULE_MOVE_TO,
    WAF_RULE_RESET_COUNT,
    WAF_RULE_HEART_BEAT,
    WAF_RULE_INCREASE_COUNT,     
    WAF_RULE_SSL_STATUS_SET, //12511
    WAF_RULE_SSL_STATUS_GET, //12512
    WAF_RULE_UPDATE_BY_SERVER,//12513
    WAF_RULE_MODE_SET, //12514
    WAF_RULE_MODE_GET, //12515
    WAF_RULE_CMD_ID_MAX,//12516

    
    TI_CMD_ID_START = TI_CMD_ID,//13400
    TI_POLICY_ADD = TI_CMD_ID_START,//for sake of multiple rules
    TI_POLICY_DEL,
    TI_POLICY_MODIFY,
    TI_POLICY_GET_SIZE,
    TI_POLICY_GET_BY_INDEX,
    TI_POLICY_GET_BY_NAME, // 13405
    TI_POLICY_SET_STATUS,
    TI_POLICY_MOVE,
    TI_POLICY_RESET_COUNT,
    TI_POLICY_INSERT,
    TI_POLICY_CLEAR, // 13410

    TI_RULE_CLEAN,
    TI_RULE_IP_ADD,
    TI_RULE_DOMAIN_ADD,
    TI_RULE_URL_ADD,
    TI_RULE_EMAIL_ADD,// 13415
    TI_RULE_MD5_ADD,
    TI_RULE_SHA1_ADD,
    TI_RULE_COMPLETE,
    
    TI_RULE_ID_CONFIG,
    
    TI_PLATFORM_STATUS_CONFIG,// 13420

    TI_DOMAIN_MATCH_TYPE_SET,
    TI_DOMAIN_MATCH_TYPE_GET,

    TI_CUSTOM_RULE_ADD,
    TI_CUSTOM_RULE_DEL,
    TI_CUSTOM_RULE_EDIT,// 13425
    TI_CUSTOM_RULE_IMPORT,
    TI_CUSTOM_RULE_ACTION,
    TI_CUSTOM_RULE_CLEAR,
    TI_CUSTOM_RULE_GET,
    TI_CUSTOM_RULE_CATEGORY_COUNT_GET,// 13430

    TI_CUSTOM_CATEGORY_ADD,
    TI_CUSTOM_CATEGORY_DEL,
    TI_CUSTOM_CATEGORY_GET,

    TI_NOT_TO_KERNEL_GET,// 13434
    TI_CUSTOM_CATEGORY_USED,

    TI_XFF_SET,
    TI_XFF_GET,// 13437
    TI_IP_REPUTATION_QUERY,// 13438

    TI_CMD_ID_MAX,

    WPP_CMD_ID_START = WPP_CMD_ID,//13500
    WPP_CMD_ID_SYNC_TO_KERNEL_CONFIG = WPP_CMD_ID_START,
    //WPP_CMD_ID_SYNC_TO_KERNEL_WEB_PARAMS,
    //WPP_CMD_ID_SYNC_TO_KERNEL_RULE_PARAMS,
    WPP_CMD_ID_MAX,
    
    IOP_CMD_ID_START = IOP_CMD_ID,//13600

    IOP_POLICY_ADD = IOP_CMD_ID_START,
    IOP_POLICY_DEL,
    IOP_POLICY_MODIFY,
    IOP_POLICY_GET_SIZE,
    IOP_POLICY_GET_BY_INDEX,
    IOP_POLICY_GET_BY_NAME,
    IOP_POLICY_SET_STATUS,
    IOP_POLICY_MOVE_TO,
    IOP_POLICY_RESET_COUNT,
    IOP_POLICY_CLEAR,

    IOP_SERVER_ADD,
    IOP_SERVER_DEL,
    IOP_SERVER_MODIFY,
    IOP_SERVER_GET_SIZE,
    IOP_SERVER_GET_BY_INDEX,
    IOP_SERVER_GET_BY_NAME,
    IOP_SERVER_CLEAR,

    IOP_LEARN_SET,
    IOP_LEARN_GET,

    IOP_LEARN_LIST_GET_SIZE,
    IOP_LEARN_LIST_GET_BY_INDEX,
    IOP_LEARN_LIST_DEL,
    IOP_LEARN_LIST_CLEAR,
    IOP_LEARN_LIST_VALID,

    IOP_CMD_ID_MAX,

    RA_CMD_ID_START = RA_CMD_ID,//13700
    RA_POLICY_ADD = RA_CMD_ID_START,
    RA_POLICY_DEL,
    RA_POLICY_EDIT,
    RA_POLICY_GET_SIZE,
    RA_POLICY_GET_BY_INDEX,
    RA_POLICY_SET_STATUS,
    RA_POLICY_RESET_COUNT,
    RA_CMD_ID_MAX,

    BLKLIST_FILTER_CMD_ID_START = BLKLIST_FILTER_CMD_ID,//13800
    BLKLIST_FILTER_ADD = BLKLIST_FILTER_CMD_ID_START,
    BLKLIST_FILTER_DEL,
    BLKLIST_FILTER_EDIT,
    BLKLIST_FILTER_GET, // get both size and entries
    BLKLIST_FILTER_CLEAR,
    BLKLIST_FILTER_SAVE,
    BLKLIST_FILTER_RESTORE,
    BLKLIST_FILTER_EXPORT,
    BLKLIST_FILTER_IMPORT,
    BLKLIST_FILTER_FULL_CONFIG_STATUS, // used to check if save, restore, export and import is finished(not used now)
    BLKLIST_FILTER_XFF_SET,
    BLKLIST_FILTER_XFF_GET,
    BLKLIST_FILTER_SDK_ADD,
    BLKLIST_FILTER_SDK_DEL,
    BLKLIST_FILTER_GET_STAT,
    BLKLIST_FILTER_GET_SIZE,
    BLKLIST_FILTER_SET_AUTO_DELETE_STATUS,
    BLKLIST_FILTER_GET_AUTO_DELETE_STATUS,
    BLKLIST_FILTER_CMD_ID_MAX,

    ATBL_ADD = ATBL_CMD_ID,
    ATBL_DEL,
    ATBL_GET_SIZE,
    ATBL_GET_BY_INDEX,
    ATBL_CLEAR_ALL,
    ATBL_SET_BLOCK_DURATION,
    ATBL_GET_BLOCK_DURATION,
    ATBL_RESTORE,
    ATBL_CMD_ID_MAX,

    VPN_SET_STATE = VPN_CMD_ID,
	VPN_GET_STATE,
    VPN_GET_EXIST,
	VPN_CMD_ID_MAX,

    HYLAB_IF_SET = HYLAB_IF_CMD_ID,
    HYLAB_IF_GET_SIZE,
    HYLAB_IF_GET_BY_ID,
    HYLAB_IF_GET_BY_NAME,
    HYLAB_IF_GET_LIST,
    HYLAB_BR_SET_BRIDGE,
    HYLAB_BR_DEL_BRIDGE,
    HYLAB_BR_GET_SIZE,
    HYLAB_BR_GET_BY_ID,
    HYLAB_IF_DEL,
    HYLAB_IF_CAR_SET,
    HYLAB_IF_CAR_GET,
    HYLAB_IF_CAR_DEL,
    HYLAB_IF_GET_LIST_BY_WORKMODE,
    HYLAB_IF_GET_SIZE2,
    HYLAB_IF_GET_BY_ID2,
    HYLAB_BR_SET_BRIDGE2,
    HYLAB_IF_SET_GATEWAY,
    HYLAB_IF_CLEAN_GATEWAY,
    HYLAB_IF_SET_HA_RESTORE,
    HYLAB_BR_SET_BRIDGE_HA_RESTORE,
    HYLAB_IF_USED_BY_PPPOE,
    HYLAB_IF_CMD_ID_MAX,

    HYTF_ID_ERROR_NOTIFY_CMD = 13200,

    HYLAB_EDR_CONNECT_STATUS = HYLAB_EDR_CMD_ID,
    HYLAB_EDR_CONFIG,
	HYLAB_SET_DER_DISTRIBUTE_STATUS,
    HYLAB_EDR_CMD_ID_MAX,

    HYTFFW_CUSTOM_L7SERVICE_PATTERN_CLEAN = 14000,
	HYTFFW_CUSTOM_L7SERVICE_PATTERN_ADD,
    HYTFFW_CUSTOM_L7SERVICE_PATTERN_ADD_COMPLETE,


    DNS_TPROXY_ADD = DNS_TPROXY_BASE_CMD_ID, //14100
	DNS_TPROXY_INSERT,
	DNS_TPROXY_MODIFY,
	DNS_TPROXY_MOVE,
	DNS_TPROXY_DELETE,
	
	DNS_TPROXY_DELETE_ALL, //14105
	DNS_TPROXY_GET_SIZE, 
	DNS_TPROXY_GET,
	DNS_TPROXY_SET_STATUS,
	DNS_TPROXY_CLEAR_COUNTER,
	
	DNS_TPROXY_DELETE_BY_NAME,//14110
	DNS_TPROXY_MODIFY_BY_NAME,
    DNS_TPROXY_SET_GW_STATUS,
	DNS_TPROXY_CMD_ID_MAX, 
	
	DNS_CONTROL_CMD_ID_START = DNS_CONTROL_BASE_CMD_ID,
    DNS_CONTROL_RULE_CLEAN,
    DNS_CONTROL_RULE_DOMAIN_ADD,
    DNS_CONTROL_RULE_COMPLETE,
    DNS_CONTROL_CMD_ID_MAX,

	SWITCH_CHIP_PORT_LINK_SET_ONE = SWITCH_CHIP_PORT_BASE_CMD_ID,
	SWITCH_CHIP_PORT_LINK_SET_ALL,
	SWITCH_CHIP_PORT_LINK_GET_ONE,
	SWITCH_CHIP_PORT_LINK_GET_ALL,
	SWITCH_CHIP_PORT_MTU_SET_ONE,
	SWITCH_CHIP_PORT_MTU_SET_ALL,
	SWITCH_CHIP_PORT_MTU_GET_ONE,
	SWITCH_CHIP_PORT_MTU_GET_ALL,
	SWITCH_CHIP_PORT_STATS_GET_ONE,
	SWITCH_CHIP_PORT_STATS_GET_ALL,
	SWITCH_CHIP_PORT_MAC_BASE_GET,
	SWITCH_CHIP_PORT_MAC_SET_ONE,
	SWITCH_CHIP_PORT_ENABLE_DISABLE,

	SWITCH_CHIP_CMD_ID_MAX,    


    FW_SEPERATE_CMD_ID_START = FW_SEPERATE_CMD_ID, //14350
    FW_GEO_ACCESS_CONTROL_POLICY_ADD = FW_SEPERATE_CMD_ID_START, 
    FW_GEO_ACCESS_CONTROL_POLICY_EDIT,
    FW_GEO_ACCESS_CONTROL_POLICY_DEL,
    FW_GEO_ACCESS_CONTROL_POLICY_CLEAR,
    FW_GEO_ACCESS_CONTROL_POLICY_GET_SIZE,
    FW_GEO_ACCESS_CONTROL_POLICY_GET_BY_NAME,//14355
    FW_GEO_ACCESS_CONTROL_POLICY_GET_BY_INDEX,
    FW_GEO_ACCESS_CONTROL_POLICY_INSERT,
    FW_GEO_ACCESS_CONTROL_POLICY_MOVE,
    FW_GEO_ACCESS_CONTROL_POLICY_RESET_COUNT,
    FW_GEO_ACCESS_CONTROL_POLICY_CHANGE_STATUS,//14360

    FW_IPS_TEMPLATE_ADD, 
    FW_IPS_TEMPLATE_EDIT,
    FW_IPS_TEMPLATE_DEL,
    FW_IPS_TEMPLATE_CLEAR,
    FW_IPS_TEMPLATE_GET_SIZE,//14365
    FW_IPS_TEMPLATE_GET_BY_NAME,
    FW_IPS_TEMPLATE_GET_BY_INDEX,
    
    FW_WAF_TEMPLATE_ADD, 
    FW_WAF_TEMPLATE_EDIT,
    FW_WAF_TEMPLATE_DEL,//14370
    FW_WAF_TEMPLATE_CLEAR,
    FW_WAF_TEMPLATE_GET_SIZE,
    FW_WAF_TEMPLATE_GET_BY_NAME,
    FW_WAF_TEMPLATE_GET_BY_INDEX,

    FW_AV_TEMPLATE_ADD, //14375
    FW_AV_TEMPLATE_EDIT,
    FW_AV_TEMPLATE_DEL,
    FW_AV_TEMPLATE_CLEAR,
    FW_AV_TEMPLATE_GET_SIZE,
    FW_AV_TEMPLATE_GET_BY_NAME,//14380
    FW_AV_TEMPLATE_GET_BY_INDEX,
    
    FW_CONTENT_SECURITY_TEMPLATE_ADD, 
    FW_CONTENT_SECURITY_TEMPLATE_EDIT,
    FW_CONTENT_SECURITY_TEMPLATE_DEL,
    FW_CONTENT_SECURITY_TEMPLATE_CLEAR,//14385
    FW_CONTENT_SECURITY_TEMPLATE_GET_SIZE,
    FW_CONTENT_SECURITY_TEMPLATE_GET_BY_NAME,
    FW_CONTENT_SECURITY_TEMPLATE_GET_BY_INDEX,

    FW_SECURITY_PROTECT_POLICY_ADD, 
    FW_SECURITY_PROTECT_POLICY_EDIT,//14390
    FW_SECURITY_PROTECT_POLICY_DEL,
    FW_SECURITY_PROTECT_POLICY_CLEAR,
    FW_SECURITY_PROTECT_POLICY_CLEAR_COUNTER,
    FW_SECURITY_PROTECT_POLICY_GET_SIZE,
    FW_SECURITY_PROTECT_POLICY_GET_BY_NAME,//14395
    FW_SECURITY_PROTECT_POLICY_GET_BY_INDEX,
    FW_SECURITY_PROTECT_POLICY_INSERT,
    FW_SECURITY_PROTECT_POLICY_MOVE, 
    FW_SECURITY_PROTECT_POLICY_CHANGE_STATUS,
    FW_SECURITY_PROTECT_POLICY_TEMPLATE_UPDATE, //14400

    FW_SECURITY_POLICY_TEMPLATE_REFERENCED,

    FW_SECURITY_PROTECT_POLICY_GET_SIZE_LOCAL_ONLY,
    FW_SECURITY_PROTECT_POLICY_GET_BY_INDEX_LOCAL_ONLY,
    FW_SECURITY_PROTECT_POLICY_GET_SIZE_NMC_ONLY,
    FW_SECURITY_PROTECT_POLICY_GET_BY_INDEX_NMC_ONLY,
    
    FW_APP_CONTROL_POLICY_ADD,
    FW_APP_CONTROL_POLICY_EDIT,
    FW_APP_CONTROL_POLICY_DEL,
    FW_APP_CONTROL_POLICY_CLEAR, //14405
    FW_APP_CONTROL_POLICY_GET_SIZE,
    FW_APP_CONTROL_POLICY_GET_BY_NAME,
    FW_APP_CONTROL_POLICY_GET_BY_INDEX,
    FW_APP_CONTROL_POLICY_INSERT,
    FW_APP_CONTROL_POLICY_MOVE, //14410
    FW_APP_CONTROL_POLICY_CHANGE_STATUS,

    
    FW_WEB_KEYWORD_FILTER_ADD,
    FW_WEB_KEYWORD_FILTER_EDIT,
    FW_WEB_KEYWORD_FILTER_DEL,
    FW_WEB_KEYWORD_FILTER_CLEAR, //14415
    FW_WEB_KEYWORD_FILTER_GET_SIZE,
    FW_WEB_KEYWORD_FILTER_GET_BY_NAME,
    FW_WEB_KEYWORD_FILTER_GET_BY_INDEX,
    FW_WEB_KEYWORD_FILTER_INSERT,
    FW_WEB_KEYWORD_FILTER_MOVE, //14420
    FW_WEB_KEYWORD_FILTER_CHANGE_STATUS,

    FW_PROTOCOL_CMD_CONTROL_ADD,
    FW_PROTOCOL_CMD_CONTROL_EDIT,
    FW_PROTOCOL_CMD_CONTROL_DEL,
    FW_PROTOCOL_CMD_CONTROL_CLEAR, //14425
    FW_PROTOCOL_CMD_CONTROL_GET_SIZE,
    FW_PROTOCOL_CMD_CONTROL_GET_BY_NAME,
    FW_PROTOCOL_CMD_CONTROL_GET_BY_INDEX,
    FW_PROTOCOL_CMD_CONTROL_INSERT,
    FW_PROTOCOL_CMD_CONTROL_MOVE, //14430
    FW_PROTOCOL_CMD_CONTROL_CHANGE_STATUS,

    FW_PROXY_CONTROL_ADD,
    FW_PROXY_CONTROL_EDIT,
    FW_PROXY_CONTROL_DEL,
    FW_PROXY_CONTROL_CLEAR, //14435
    FW_PROXY_CONTROL_GET_SIZE,
    FW_PROXY_CONTROL_GET_BY_NAME,
    FW_PROXY_CONTROL_GET_BY_INDEX,
    FW_PROXY_CONTROL_INSERT,
    FW_PROXY_CONTROL_MOVE, //14440
    FW_PROXY_CONTROL_CHANGE_STATUS,

    FW_DECRYPT_POLICY_ADD,
    FW_DECRYPT_POLICY_EDIT,
    FW_DECRYPT_POLICY_DEL,
    FW_DECRYPT_POLICY_CLEAR,//14445
    FW_DECRYPT_POLICY_GET_SIZE,
    FW_DECRYPT_POLICY_GET_BY_NAME,
    FW_DECRYPT_POLICY_GET_BY_INDEX,
    FW_DECRYPT_POLICY_INSERT,
    FW_DECRYPT_POLICY_MOVE, //14450
    FW_DECRYPT_POLICY_CHANGE_STATUS,

    FW_HOST_ACCESS_CONTROL_ADD,
    FW_HOST_ACCESS_CONTROL_EDIT,
    FW_HOST_ACCESS_CONTROL_DEL,
    FW_HOST_ACCESS_CONTROL_CLEAR,//14455
    FW_HOST_ACCESS_CONTROL_GET_SIZE,
    FW_HOST_ACCESS_CONTROL_GET_BY_NAME,
    FW_HOST_ACCESS_CONTROL_GET_BY_INDEX,
    FW_HOST_ACCESS_CONTROL_INSERT,
    FW_HOST_ACCESS_CONTROL_MOVE, //14460
    FW_HOST_ACCESS_CONTROL_CHANGE_STATUS,
    
    FW_HOST_ACCESS_CONTROL_POLICY_TYPE_SET,
    FW_HOST_ACCESS_CONTROL_POLICY_TYPE_GET,

    FW_USERSPACE_SERVICE_UPDATE,

    FW_AV_TEMPLATE_CLEAR_NMC,//14465
    FW_WAF_TEMPLATE_CLEAR_NMC,
    FW_SECURITY_PROTECT_POLICY_CLEAR_NMC,
    
    // start policy analyse
    FW_POLICY_ANALYSE_SETTING_SET,
    FW_POLICY_ANALYSE_SETTING_GET,
    FW_POLICY_ANALYSE_START,//14470
    FW_POLICY_ANALYSE_RESULT_SUMMARY_GET,
    FW_POLICY_ANALYSE_RESULT_LIST_GET_SIZE,
    FW_POLICY_ANALYSE_RESULT_LIST_GET_BY_INDEX,
    FW_POLICY_ANALYSE_RESULT_DETAIL_LIST_GET_SIZE,
    FW_POLICY_ANALYSE_RESULT_DETAIL_LIST_GET_BY_INDEX,//14475
    FW_POLICY_ANALYSE_IGNORE_LIST_GET_SIZE,
    FW_POLICY_ANALYSE_IGNORE_LIST_GET_BY_INDEX,
    FW_POLICY_ANALYSE_IGNORE_BY_NAME,//14478
    // end policy analyse

    FW_SEPERATE_CMD_ID_MAX,

    // start session control
    FW_SESSION_CONTROL_ADD = SESSION_CONTROL_CMD_ID,
    FW_SESSION_CONTROL_EDIT,
    FW_SESSION_CONTROL_DEL,
    FW_SESSION_CONTROL_CLEAR,//14482
    FW_SESSION_CONTROL_GET_SIZE,
    FW_SESSION_CONTROL_GET_BY_NAME,
    FW_SESSION_CONTROL_GET_BY_INDEX,
    FW_SESSION_CONTROL_INSERT,
    FW_SESSION_CONTROL_MOVE, //14487
    FW_SESSION_CONTROL_CHANGE_STATUS,
    FW_SESSION_CONTROL_CLEAR_LOCAL,//
    FW_SESSION_CONTROL_CLEAR_NMC,//
    FW_SESSION_CONTROL_GET_SIZE_LOCAL,
    FW_SESSION_CONTROL_GET_BY_INDEX_LOCAL,
    FW_SESSION_CONTROL_GET_SIZE_NMC,
    FW_SESSION_CONTROL_GET_BY_INDEX_NMC,
    FW_SESSION_CONTROL_CMD_ID_MAX,
    // end session control


    // user group
    SSLVPN_MSG_USER_GROUP_GET = SSLVPN_CMD_ID, // 14600
    SSLVPN_MSG_USER_GROUP_GET_ONE,
    SSLVPN_MSG_USER_GROUP_ADD,
    SSLVPN_MSG_USER_GROUP_EDIT,
    SSLVPN_MSG_USER_GROUP_DEL,
    SSLVPN_MSG_USER_GROUP_CLEAR_CURRENT,// 14605
    SSLVPN_MSG_USER_GROUP_MOVE,
    SSLVPN_MSG_USER_GROUP_CLEAR,
    SSLVPN_MSG_USER_GROUP_ROOT_GET,
    SSLVPN_MSG_USER_GROUP_ROOT_SET,

    // user
    SSLVPN_MSG_USER_GET,// 14610
    SSLVPN_MSG_USER_GET_ONE,
    SSLVPN_MSG_USER_GET_ONE_BY_NAME,
    SSLVPN_MSG_USER_GET_ONE_BY_OFFSET,
    SSLVPN_MSG_USER_ADD,
    SSLVPN_MSG_USER_EDIT,// 14615
    SSLVPN_MSG_USER_DEL,
    SSLVPN_MSG_USER_MOVE,
    SSLVPN_MSG_USER_CLEAR,

    // resource group
    SSLVPN_MSG_RESOURCE_GROUP_GET,
    SSLVPN_MSG_RESOURCE_GROUP_GET_ONE,// 14620
    SSLVPN_MSG_RESOURCE_GROUP_ADD,
    SSLVPN_MSG_RESOURCE_GROUP_EDIT,
    SSLVPN_MSG_RESOURCE_GROUP_DEL,
    SSLVPN_MSG_RESOURCE_GROUP_CLEAR_CURRENT,
    SSLVPN_MSG_RESOURCE_GROUP_MOVE, // 14625
    SSLVPN_MSG_RESOURCE_GROUP_CLEAR,
    SSLVPN_MSG_RESOURCE_GROUP_ROOT_GET,
    SSLVPN_MSG_RESOURCE_GROUP_ROOT_SET,
    
    // resource
    SSLVPN_MSG_RESOURCE_GET,
    SSLVPN_MSG_RESOURCE_GET_ONE,// 14630
    SSLVPN_MSG_RESOURCE_ID_GET_BY_USER,
    SSLVPN_MSG_RESOURCE_ID_GET_BY_OFFSET,
    SSLVPN_MSG_RESOURCE_ADD,
    SSLVPN_MSG_RESOURCE_EDIT,
    SSLVPN_MSG_RESOURCE_DEL,// 14635
    SSLVPN_MSG_RESOURCE_MOVE,
    SSLVPN_MSG_RESOURCE_CLEAR,

    // role
    SSLVPN_MSG_ROLE_GET_SIZE,
    SSLVPN_MSG_ROLE_GET_BY_INDEX,
    SSLVPN_MSG_ROLE_GET_BY_NAME,// 14640
    SSLVPN_MSG_ROLE_ADD,
    SSLVPN_MSG_ROLE_EDIT,
    SSLVPN_MSG_ROLE_DEL,
    SSLVPN_MSG_ROLE_MOVE,
    SSLVPN_MSG_ROLE_CLEAR,// 14645

    // import and export
    SSLVPN_MSG_IMPORT,
    SSLVPN_MSG_EXPORT,

    // online user msg
    SSLVPN_MSG_ONLINE_USER_INFO_GET,
    SSLVPN_MSG_KICKOFF_BY_IP,
    SSLVPN_MSG_BLOCK_USER,// 14650
    SSLVPN_MSG_UNBLOCK_USER,
    SSLVPN_MSG_UNBLOCK_REALIP,
    SSLVPN_MSG_BLOCKED_IP_USER_SET, // config restore use
    SSLVPN_MSG_BLOCKED_IP_USER_GET,
    SSLVPN_MSG_ONLINE_USER_CHECK,// 14655
    SSLVPN_MSG_SECURITY_SETTING_SET,
    SSLVPN_MSG_SECURITY_SETTING_GET,

    // sslvpn address pool
    SSLVPN_MSG_ADDRESS_POOL_SET,
    SSLVPN_MSG_ADDRESS_POOL_GET, // for save config
    PPTP_MSG_ADDRESS_POOL_SET,
    L2TP_MSG_ADDRESS_POOL_SET,

    // below is sslvpn process related msg:
    
    // online user msg from sslvpn process
    SSLVPN_MSG_ONLINE_USER_GET_SIZE_FROM_SSLVPN = SSLVPN_CMD_ID_MAX_RESERVE - 9,
    SSLVPN_MSG_ONLINE_USER_GET_FROM_SSLVPN = SSLVPN_CMD_ID_MAX_RESERVE - 8,
    
    // debug msg to sslvpn process
    SSLVPN_MSG_VDEBUG = SSLVPN_CMD_ID_MAX_RESERVE - 7,
    SSLVPN_MSG_FDEBUG = SSLVPN_CMD_ID_MAX_RESERVE - 6,

    // notify from kernel to sslvpn process
    SSLVPN_MSG_KICKOFF_TO_SSLPVN = SSLVPN_CMD_ID_MAX_RESERVE - 5,

    // notify from sslvpn process
    SSLVPN_MSG_TID = SSLVPN_CMD_ID_MAX_RESERVE - 4,
    SSLVPN_MSG_AUTH = SSLVPN_CMD_ID_MAX_RESERVE - 3,
    SSLVPN_MSG_ONLINE = SSLVPN_CMD_ID_MAX_RESERVE - 2,
    SSLVPN_MSG_OFFLINE = SSLVPN_CMD_ID_MAX_RESERVE - 1,

    SSLVPN_CMD_ID_MAX = SSLVPN_CMD_ID_MAX_RESERVE,

    INDUSTRIAL_CONTROL_BASE_ID = INDUSTRIAL_CONTROL_CMD_ID, // 14900
    INDUSTRIAL_CONTROL_FDEBUG_ID,
    INDUSTRIAL_CONTROL_VDEBUG_ID,

    // template rule start
    INDUSTRIAL_CONTROL_TEMPLATE_ADD,//14903 
    INDUSTRIAL_CONTROL_TEMPLATE_INSERT, 
    INDUSTRIAL_CONTROL_TEMPLATE_MOVE, 
    INDUSTRIAL_CONTROL_TEMPLATE_EDIT,
    INDUSTRIAL_CONTROL_TEMPLATE_DEL,
    INDUSTRIAL_CONTROL_TEMPLATE_CLEAR,
    INDUSTRIAL_CONTROL_TEMPLATE_GET,
    INDUSTRIAL_CONTROL_TEMPLATE_GET_FOR_SAVE,
    INDUSTRIAL_CONTROL_TEMPLATE_GET_ONE,
    INDUSTRIAL_CONTROL_TEMPLATE_APP_CHECK_SET, 
    INDUSTRIAL_CONTROL_TEMPLATE_APP_CHECK_SET_ALL, 
    INDUSTRIAL_CONTROL_TEMPLATE_GET_INFO, 
    INDUSTRIAL_CONTROL_TEMPLATE_CHANGE_STATUS, 
    INDUSTRIAL_CONTROL_TEMPLATE_CLEAR_COUNTER, 

    INDUSTRIAL_CONTROL_MODBUS_ADD, //14917
    INDUSTRIAL_CONTROL_MODBUS_EDIT,
    INDUSTRIAL_CONTROL_MODBUS_DEL,
    INDUSTRIAL_CONTROL_MODBUS_GET,
    INDUSTRIAL_CONTROL_MODBUS_GET_ONE,
    INDUSTRIAL_CONTROL_MODBUS_SUB_ITEM_ADD, 
    INDUSTRIAL_CONTROL_MODBUS_SUB_ITEM_EDIT,
    INDUSTRIAL_CONTROL_MODBUS_SUB_ITEM_DEL,
    INDUSTRIAL_CONTROL_MODBUS_SUB_ITEM_GET,
    INDUSTRIAL_CONTROL_MODBUS_SUB_ITEM_GET_ONE,
    INDUSTRIAL_CONTROL_MODBUS_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_MODBUS_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_CIP_ADD, //14929
    INDUSTRIAL_CONTROL_CIP_EDIT,
    INDUSTRIAL_CONTROL_CIP_DEL,
    INDUSTRIAL_CONTROL_CIP_GET,
    INDUSTRIAL_CONTROL_CIP_GET_ONE,
    INDUSTRIAL_CONTROL_CIP_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_CIP_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_S7_ADD, //14936
    INDUSTRIAL_CONTROL_S7_EDIT,
    INDUSTRIAL_CONTROL_S7_DEL,
    INDUSTRIAL_CONTROL_S7_GET,
    INDUSTRIAL_CONTROL_S7_GET_ONE,
    INDUSTRIAL_CONTROL_S7_SUB_ITEM_ADD, 
    INDUSTRIAL_CONTROL_S7_SUB_ITEM_EDIT,
    INDUSTRIAL_CONTROL_S7_SUB_ITEM_DEL,
    INDUSTRIAL_CONTROL_S7_SUB_ITEM_GET,
    INDUSTRIAL_CONTROL_S7_SUB_ITEM_GET_ONE,
    INDUSTRIAL_CONTROL_S7_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_S7_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_OPC_ADD, //14948
    INDUSTRIAL_CONTROL_OPC_EDIT,
    INDUSTRIAL_CONTROL_OPC_DEL,
    INDUSTRIAL_CONTROL_OPC_GET,
    INDUSTRIAL_CONTROL_OPC_GET_ONE,
    INDUSTRIAL_CONTROL_OPC_SUB_ITEM_ADD, 
    INDUSTRIAL_CONTROL_OPC_SUB_ITEM_EDIT,
    INDUSTRIAL_CONTROL_OPC_SUB_ITEM_DEL,
    INDUSTRIAL_CONTROL_OPC_SUB_ITEM_GET,
    INDUSTRIAL_CONTROL_OPC_SUB_ITEM_GET_ONE,
    INDUSTRIAL_CONTROL_OPC_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_OPC_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_DNP3_ADD, //14960
    INDUSTRIAL_CONTROL_DNP3_EDIT,
    INDUSTRIAL_CONTROL_DNP3_DEL,
    INDUSTRIAL_CONTROL_DNP3_GET,
    INDUSTRIAL_CONTROL_DNP3_GET_ONE,
    INDUSTRIAL_CONTROL_DNP3_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_DNP3_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_PROFINET_ADD, //14967
    INDUSTRIAL_CONTROL_PROFINET_EDIT,
    INDUSTRIAL_CONTROL_PROFINET_DEL,
    INDUSTRIAL_CONTROL_PROFINET_GET,
    INDUSTRIAL_CONTROL_PROFINET_GET_ONE,
    INDUSTRIAL_CONTROL_PROFINET_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_PROFINET_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_IEC104_ADD, //14974
    INDUSTRIAL_CONTROL_IEC104_EDIT,
    INDUSTRIAL_CONTROL_IEC104_DEL,
    INDUSTRIAL_CONTROL_IEC104_GET,
    INDUSTRIAL_CONTROL_IEC104_GET_ONE,
    INDUSTRIAL_CONTROL_IEC104_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_IEC104_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_ETHERNETIP_ADD, //14981
    INDUSTRIAL_CONTROL_ETHERNETIP_EDIT,
    INDUSTRIAL_CONTROL_ETHERNETIP_DEL,
    INDUSTRIAL_CONTROL_ETHERNETIP_GET,
    INDUSTRIAL_CONTROL_ETHERNETIP_GET_ONE,
    INDUSTRIAL_CONTROL_ETHERNETIP_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_ETHERNETIP_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_OPCUA_TCP_ADD, //14988
    INDUSTRIAL_CONTROL_OPCUA_TCP_EDIT,
    INDUSTRIAL_CONTROL_OPCUA_TCP_DEL,
    INDUSTRIAL_CONTROL_OPCUA_TCP_GET,
    INDUSTRIAL_CONTROL_OPCUA_TCP_GET_ONE,
    INDUSTRIAL_CONTROL_OPCUA_TCP_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_OPCUA_TCP_KEY_EVENT_GET,
        
    INDUSTRIAL_CONTROL_MQTT_ADD, //14995
    INDUSTRIAL_CONTROL_MQTT_EDIT,
    INDUSTRIAL_CONTROL_MQTT_DEL,
    INDUSTRIAL_CONTROL_MQTT_GET,
    INDUSTRIAL_CONTROL_MQTT_GET_ONE,
    INDUSTRIAL_CONTROL_MQTT_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_MQTT_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_FINS_ADD, //15002
    INDUSTRIAL_CONTROL_FINS_EDIT,
    INDUSTRIAL_CONTROL_FINS_DEL,
    INDUSTRIAL_CONTROL_FINS_GET,
    INDUSTRIAL_CONTROL_FINS_GET_ONE,
    INDUSTRIAL_CONTROL_FINS_SUB_ITEM_ADD, 
    INDUSTRIAL_CONTROL_FINS_SUB_ITEM_EDIT,
    INDUSTRIAL_CONTROL_FINS_SUB_ITEM_DEL,
    INDUSTRIAL_CONTROL_FINS_SUB_ITEM_GET,
    INDUSTRIAL_CONTROL_FINS_SUB_ITEM_GET_ONE,
    INDUSTRIAL_CONTROL_FINS_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_FINS_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_BACNET_ADD, 
    INDUSTRIAL_CONTROL_BACNET_EDIT,
    INDUSTRIAL_CONTROL_BACNET_DEL,
    INDUSTRIAL_CONTROL_BACNET_GET,
    INDUSTRIAL_CONTROL_BACNET_GET_ONE,
    INDUSTRIAL_CONTROL_BACNET_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_BACNET_KEY_EVENT_GET,

    INDUSTRIAL_CONTROL_MMS_ADD, 
    INDUSTRIAL_CONTROL_MMS_EDIT,
    INDUSTRIAL_CONTROL_MMS_DEL,
    INDUSTRIAL_CONTROL_MMS_GET,
    INDUSTRIAL_CONTROL_MMS_GET_ONE,
    INDUSTRIAL_CONTROL_MMS_KEY_EVENT_SET, 
    INDUSTRIAL_CONTROL_MMS_KEY_EVENT_GET,
        
    INDUSTRIAL_CONTROL_CUSTOM_ADD, 
    INDUSTRIAL_CONTROL_CUSTOM_EDIT,
    INDUSTRIAL_CONTROL_CUSTOM_DEL,
    INDUSTRIAL_CONTROL_CUSTOM_GET,
    INDUSTRIAL_CONTROL_CUSTOM_GET_ONE,
    INDUSTRIAL_CONTROL_CUSTOM_SUB_ITEM_ADD, 
    INDUSTRIAL_CONTROL_CUSTOM_SUB_ITEM_EDIT,
    INDUSTRIAL_CONTROL_CUSTOM_SUB_ITEM_DEL,
    INDUSTRIAL_CONTROL_CUSTOM_SUB_ITEM_GET,
    INDUSTRIAL_CONTROL_CUSTOM_SUB_ITEM_GET_ONE,

    INDUSTRIAL_CONTROL_GOOSE_ADD, 
    INDUSTRIAL_CONTROL_GOOSE_EDIT,
    INDUSTRIAL_CONTROL_GOOSE_DEL,
    INDUSTRIAL_CONTROL_GOOSE_GET,
    INDUSTRIAL_CONTROL_GOOSE_GET_ONE,
    
    INDUSTRIAL_CONTROL_SV_ADD, 
    INDUSTRIAL_CONTROL_SV_EDIT,
    INDUSTRIAL_CONTROL_SV_DEL,
    INDUSTRIAL_CONTROL_SV_GET,
    INDUSTRIAL_CONTROL_SV_GET_ONE,
    // template and rule end

    // learning start
    INDUSTRIAL_CONTROL_LEARNING_SET, //15048
    INDUSTRIAL_CONTROL_LEARNING_GET, 
    INDUSTRIAL_CONTROL_LEARNING_DEPTH_SET, 
    INDUSTRIAL_CONTROL_LEARNING_DEPTH_GET, 
    // learning end

    // custom start
    INDUSTRIAL_CONTROL_CUSTOM_PORT_SET, 
    INDUSTRIAL_CONTROL_CUSTOM_PORT_GET, 
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_ADD, 
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_EDIT,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_DEL,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_CLEAR,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_GET,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_GET_ONE,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_GET_INFO,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_CHANGE_STATUS,
    INDUSTRIAL_CONTROL_CUSTOM_PROTOCOL_GET_MARK_INFO,
    // custom end

    // no ip protocol policy start
    INDUSTRIAL_CONTROL_NON_IP_PROTOCOL_POLICY_SET, //15063
    INDUSTRIAL_CONTROL_NON_IP_PROTOCOL_POLICY_GET, 
    // no ip protocol policy end

    INDUSTRIAL_CONTROL_INSMOD_USERSPACE_NOTIFY, 
    INDUSTRIAL_CONTROL_RESTORE_FINISH_KERNEL_NOTIFY, 
#ifdef CONFIG_IFW
	INDUSTRIAL_CONTROL_AUTO_TEMPLATE_GET, //15067
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_DEL,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_GENERATE,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_GENERATED,// 15070
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_GET_LEARNING,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_ACTION_SET,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_ACTION_GET,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_APP_CHECK_SET,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_APP_CHECK_GET,// 15075
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_APP_CHECK_SET_ALL,
    INDUSTRIAL_CONTROL_AUTO_TEMPLATE_APP_CHECK_GET_ALL,// 15077
#endif

    INDUSTRIAL_CONTROL_AUDIT_CFG_GET,  // 15080
    INDUSTRIAL_CONTROL_AUDIT_CFG_SET,   // 15081

    INDUSTRIAL_CONTROL_MAX_CMD_ID,


    WEB_FILTER_URL_BASE_ID = WEB_FILTER_URL_CMD_ID, // 15100

    WEB_FILTER_URL_ADD,
    WEB_FILTER_URL_MOD,
    WEB_FILTER_URL_DEL,
    WEB_FILTER_URL_CLR,

    FW_SYNERGY_BLK_SET,
	
	ACE_MROUTE_ADD = ACE_MROUTE_CMD_ID,// 15200
    ACE_MROUTE_MOD,
    ACE_MROUTE_DEL,
    ACE_MROUTE_CLR,
    ACE_MROUTE_MOVE,
    
    ACE_MROUTE_SIZE,
    ACE_MROUTE_GET_NEXT,
    ACE_MROUTE_GET_BY_IDX,
    ACE_MROUTE_MAX,
    
    SERVICE_GROUP_ADD = SERVICE_GROUP_CMD_ID,// 15300
    SERVICE_GROUP_EDIT,
    SERVICE_GROUP_DEL,
    SERVICE_GROUP_CLEAR,
    SERVICE_GROUP_GET_SIZE,
    SERVICE_GROUP_GET_BY_NAME,
    SERVICE_GROUP_GET_BY_INDEX,
    SERVICE_GROUP_CMD_ID_MAX,

	TOPOLOGY_LEARNING_SET=TOPOLOGY_CMD_ID,//15400
	TOPOLOGY_LEARNING_GET,
	TOPOLOGY_LEARNING_CLEAR,
	TOPOLOGY_LEARNING_PERSISTENCE,
	TOPOLOGY_CMD_ID_MAX,

	IPMAC_BINDING_ADD=IPMAC_CMD_ID,//15500
	IPMAC_BINDING_DEL,
	IPMAC_BINDING_GET,
	IPMAC_BINDING_MOD,
	IPMAC_BINDING_GET_LIST,
	IPMAC_BINDING_CLEAR,
	IPMAC_CMD_ID_MAX,

    HYNAT66_ADD_NAT66_RULE = HYNAT66_BASE_CMD_ID,/*16000*/
    HYNAT66_DEL_NAT66_RULE,
    HYNAT66_GET_NAT66_RULE_DETAIL_SIZE,
    HYNAT66_GET_NAT66_RULE_DETAIL,
    HYNAT66_MOD_NAT66_RULE,

    HYNAT66_MOVE_NAT66_RULE,/*16005*/
    HYNAT66_GET_NAT66_RULE_BY_INDEX,
    HYNAT66_INSERT_NAT66_RULE,
    HYNAT66_CLEAR_NAT66_RULE_COUNTER = 16008,
    HYNAT66_CHANGE_RULE_STATUS,
    HYNAT66_DPORT_MAPPING_VAILD,

    HYNAT66_MAX_CMD_ID,

	DAS_AUDIT_RULE_ADD = 16500,
	DAS_AUDIT_RULE_MOD,
	DAS_AUDIT_RULE_DEL,
	DAS_AUDIT_RULE_GET_SIZE,
	DAS_AUDIT_RULE_GET_BY_INDEX,
	DAS_AUDIT_RULE_GET_BY_NAME,
	DAS_AUDIT_RULE_CLEAR,
	DAS_AUDIT_RULE_CMD_MAX,

	SIG_ASSET_ADD = 16600,
	SIG_ASSET_ADD_GROUP,
	SIG_ASSET_MOD,
	SIG_ASSET_MOD_GROUP,
	SIG_ASSET_DEL,
	SIG_ASSET_GET_SIZE,
	SIG_ASSET_GET_BY_PARENT,
	SIG_ASSET_GET_BY_NAME,
	SIG_ASSET_CLEAR,
	SIG_ASSET_RULE_SET,
	SIG_ASSET_RULE_GET_BY_NAME,
	SIG_ASSET_CHECK_CONFILCK,
	SIG_SDK_RELOAD,
	SIG_THREAT_IP_RELOAD,
	SIG_ASSET_CMD_MAX,

	SIG_BLK_REGION_ADD = 16650,
	SIG_BLK_REGION_MOD,
	SIG_BLK_REGION_DEL,
	SIG_BLK_REGION_GET_SIZE,
	SIG_BLK_REGION_GET_BY_INDEX,
	SIG_BLK_REGION_GET_BY_NAME,
	SIG_BLK_REGION_SEARCH,
	SIG_BLK_REGION_CLEAR,
	SIG_BLK_REGION_CMD_MAX,

	SIG_THREAT_TEMPLATE_ADD = 16700, 
    SIG_THREAT_TEMPLATE_EDIT,
    SIG_THREAT_TEMPLATE_DEL,
    SIG_THREAT_TEMPLATE_CLEAR,
    SIG_THREAT_TEMPLATE_GET_SIZE,
    SIG_THREAT_TEMPLATE_GET_BY_NAME,
    SIG_THREAT_TEMPLATE_GET_BY_INDEX,
    SIG_THREAT_TEMPLATE_GET_BY_PAGE,
    SIG_THREAT_TEMPLATE_MAX,

	SIG_PREVENT_BAN_ADD = 16750, 
	SIG_PREVENT_BAN_EDIT,
	SIG_PREVENT_BAN_DEL,
	SIG_PREVENT_BAN_CLEAR,
	SIG_PREVENT_BAN_GET_SIZE,
	SIG_PREVENT_BAN_GET_BY_PAGE,
	SIG_PREVENT_BAN_GET_BY_INDEX,
	SIG_PREVENT_BAN_MAX,

	SIG_MIX_WHITELIST_ADD = 16800, 
	SIG_MIX_WHITELIST_EDIT,
	SIG_MIX_WHITELIST_DEL,
	SIG_MIX_WHITELIST_CLEAR,
	SIG_MIX_WHITELIST_GET_SIZE,
	SIG_MIX_WHITELIST_GET_BY_PAGE,
	SIG_MIX_WHITELIST_GET_BY_NAME,
	SIG_MIX_WHITELIST_GET_BY_INDEX,
	SIG_MIX_WHITELIST_GET_QUERY_SIZE,
	SIG_MIX_WHITELIST_MAX,

    SIG_IPS_RULE_ADD = 16850,
    SIG_IPS_RULE_DEL,
    SIG_IPS_RULE_MODIFY,
    SIG_IPS_RULE_GET_SIZE,
    SIG_IPS_RULE_GET_BY_INDEX,
    SIG_IPS_RULE_GET_BY_NAME,
    SIG_IPS_RULE_SET_STATUS,//12106
    SIG_IPS_RULE_MOVE_TO,
    SIG_IPS_RULE_RESET_COUNT,
    SIG_IPS_RULE_INCREASE_COUNT,
    SIG_IPS_RULE_HEART_BEAT,
    SIG_IPS_RULE_SURICATA_RESULT,//12111
    SIG_IPS_RULE_GET_ZONE,
    SIG_IPS_RULE_CMD_ID_MAX,

	SIG_HOST_ISO_ADD = 16875,
	SIG_HOST_ISO_DEL,
	SIG_HOST_ISO_EDIT,
	SIG_HOST_ISO_GET,
	SIG_HOST_ISO_GET_SIZE, 
	SIG_HOST_ISO_CLEAR,
	SIG_HOST_ISO_GET_STATUS,
	SIG_HOST_ISO_CMD_ID_MAX,

	SIG_LABEL_FILTER_ADD = 16900, 
    SIG_LABEL_FILTER_EDIT,
    SIG_LABEL_FILTER_DEL,
    SIG_LABEL_FILTER_CLEAR,
    SIG_LABEL_FILTER_GET_SIZE,
    SIG_LABEL_FILTER_GET_BY_NAME,
    SIG_LABEL_FILTER_GET_BY_INDEX,
    SIG_LABEL_FILTER_GET_BY_PAGE,
    SIG_LABEL_FILTER_MAX,

	SIG_SYSLOG_ADD = 16925,
	SIG_SYSLOG_ADD_RET,
	SIG_SYSLOG_MOD,
	SIG_SYSLOG_MOD_RET,
	SIG_SYSLOG_DEL,
	SIG_SYSLOG_DEL_RET,
	SIG_SYSLOG_GET,
	SIG_SYSLOG_GET_RET,
	SIG_SYSLOG_SET_PORT_TO_VPP,
	SIG_SYSLOG_MAX,

	HYLAB_INTERFACE_DEL_CHECK = HYLAB_EXT_BASE,
	HYTF_ANTISHAREIPR_POLICY_SET   = 17001,
    HYTF_ANTISHAREIPR_POLICY_GET   = 17002,

#ifdef CONFIG_IFW
	FIREWALL_SET_WORK_MODE=18000,
	FIREWALL_GET_WORK_MODE=18001,
#endif

	HYSERVICEPOOL44_ADD_NAT44_RULE = ACENAT_BASE2_CMD_ID,
	HYSERVICEPOOL44_DEL_NAT44_RULE,
	HYSERVICEPOOL44_GET_NAT44_RULE_DETAIL_SIZE,
	HYSERVICEPOOL44_GET_NAT44_RULE_DETAIL,
	HYSERVICEPOOL44_MOD_NAT44_RULE,

	HYSERVICEPOOL44_MOVE_NAT44_RULE,
	HYSERVICEPOOL44_GET_NAT44_RULE_BY_INDEX,
	HYSERVICEPOOL44_INSERT_NAT44_RULE,
	HYSERVICEPOOL44_CLEAR_NAT44_RULE_COUNTER,
	HYSERVICEPOOL44_LINK_STATE_CHANGE,

	HYSERVICEPOOL66_ADD_NAT66_RULE = 19100,
	HYSERVICEPOOL66_DEL_NAT66_RULE,
	HYSERVICEPOOL66_GET_NAT66_RULE_DETAIL_SIZE,
	HYSERVICEPOOL66_GET_NAT66_RULE_DETAIL,
	HYSERVICEPOOL66_MOD_NAT66_RULE,

	HYSERVICEPOOL66_MOVE_NAT66_RULE,
	HYSERVICEPOOL66_GET_NAT66_RULE_BY_INDEX,
	HYSERVICEPOOL66_INSERT_NAT66_RULE,
	HYSERVICEPOOL66_CLEAR_NAT66_RULE_COUNTER,
	HYSERVICEPOOL66_LINK_STATE_CHANGE,
	ACENAT_MAX2_CMD_ID,

	HYLAB_VPN_ABROAD_360_SET = 19120,
	HYLAB_VPN_ABROAD_360_GET,
	HYLAB_VPN_ABROAD_360_DB_RELOAD,
	HYLAB_VPN_ABROAD_360_CMD_MAX,
};

#ifdef CONFIG_IFW
enum {
	FIREWALL_WORK_MODE_PROTECT=0,
	FIREWALL_WORK_MODE_PASSALL=1,
	FIREWALL_WORK_MODE_DEBUG=2,
	FIREWALL_WORK_MODE_MONITOR=3,
	FIREWALL_WORK_MODE_MAX,
};
#endif

#define HYTF_SSH_OR_TELNET_TEMP_OPEN_TIME (24 * 3600 * HZ)

enum hytf_kernel_flag_index_e
{
	/**kernel flag:**/
	HYTF_KERNEL_FLAG_SSLVPN = 0,
	HYTF_KERNEL_FLAG_SSH,
	HYTF_KERNEL_FLAG_TELNET,
	HYTF_KERNEL_FLAG_XL2TPD = 3,
	HYTF_KERNEL_FLAG_IPSEC = 4,

	/**last one:**/
	HYTF_KERNEL_FLAG_MAX
};

enum ace_schedule_user_space_id
{
    ACE_USER_SPACE_MSG_ID           = 3000,
    ACE_USER_SPACE_RESERVE          = 3001,
    ACE_SET_SCHEDULE                = 3002,
    ACE_SET_SCHEDULE_RESP           = 3003,
    ACE_GET_SCHEDULE_SIZE           = 3004,
    ACE_GET_SCHEDULE_SIZE_RESP      = 3005,
    ACE_GET_SCHEDULE_NEXT           = 3006,
    ACE_GET_SCHEDULE_NEXT_RESP      = 3007,
    ACE_DEL_SCHEDULE                = 3008,
    ACE_DEL_SCHEDULE_RESP           = 3009,
    ACE_CLEAR_SCHEDULE              = 3010,
    ACE_CLEAR_SCHEDULE_RESP         = 3011,
    ACE_SET_RADIUS                  = 3012,
    ACE_SET_RADIUS_RESP             = 3013,
    ACE_GET_RADIUS_SIZE             = 3014,
    ACE_GET_RADIUS_SIZE_RESP        = 3015,
    ACE_GET_RADIUS_NEXT             = 3016,
    ACE_GET_RADIUS_NEXT_RESP        = 3017,
    ACE_DEL_RADIUS                  = 3018,
    ACE_DEL_RADIUS_RESP             = 3019,
    ACE_CLEAR_RADIUS                = 3020,
    ACE_CLEAR_RADIUS_RESP           = 3021,
    ACE_SET_LDAP                    = 3022,
    ACE_SET_LDAP_RESP               = 3023,
    ACE_GET_LDAP_SIZE               = 3024,
    ACE_GET_LDAP_SIZE_RESP          = 3025,
    ACE_GET_LDAP_NEXT               = 3026,
    ACE_GET_LDAP_NEXT_RESP          = 3027,
    ACE_DEL_LDAP                    = 3028,
    ACE_DEL_LDAP_RESP               = 3029,
    ACE_CLEAR_LDAP                  = 3030,
    ACE_CLEAR_LDAP_RESP             = 3031,
    ACE_SET_POP3                    = 3032,
    ACE_SET_POP3_RESP               = 3033,
    ACE_GET_POP3_SIZE               = 3034,
    ACE_GET_POP3_SIZE_RESP          = 3035,
    ACE_GET_POP3_NEXT               = 3036,
    ACE_GET_POP3_NEXT_RESP          = 3037,
    ACE_DEL_POP3                    = 3038,
    ACE_DEL_POP3_RESP               = 3039,
    ACE_CLEAR_POP3                  = 3040,
    ACE_CLEAR_POP3_RESP             = 3041,
    ACE_SET_AD                      = 3042,
    ACE_SET_AD_RESP                 = 3043,
    ACE_GET_AD_SIZE                 = 3044,
    ACE_GET_AD_SIZE_RESP            = 3045,
    ACE_GET_AD_NEXT                 = 3046,
    ACE_GET_AD_NEXT_RESP            = 3047,
    ACE_DEL_AD                      = 3048,
    ACE_DEL_AD_RESP                 = 3049,
    ACE_CLEAR_AD                    = 3050,
    ACE_CLEAR_AD_RESP               = 3051,
    ACE_SET_AUTH_POLICY             = 3052,
    ACE_SET_AUTH_POLICY_RESP        = 3053,
    ACE_GET_AUTH_POLICY_SIZE        = 3054,
    ACE_GET_AUTH_POLICY_SIZE_RESP   = 3055,
    ACE_GET_AUTH_POLICY_NEXT        = 3056,
    ACE_GET_AUTH_POLICY_NEXT_RESP   = 3057,
    ACE_DEL_AUTH_POLICY             = 3058,
    ACE_DEL_AUTH_POLICY_RESP        = 3059,
    ACE_CLEAR_AUTH_POLICY           = 3060,
    ACE_CLEAR_AUTH_POLICY_RESP      = 3061,
    ACE_MOVE_AUTH_POLICY            = 3062,
    ACE_MOVE_AUTH_POLICY_RESP       = 3063,
    ACE_MODIFY_AUTH_POLICY          = 3064,
    ACE_MODIFY_AUTH_POLICY_RESP     = 3065,

    ACE_GET_IP_AUTH_SERVER          = 3066,
    ACE_GET_IP_AUTH_SERVER_RESP     = 3067,

    ACE_SET_AUTH_USER               = 3068,
    ACE_SET_AUTH_USER_RESP          = 3069,
    ACE_GET_AUTH_USER_SIZE          = 3070,
    ACE_GET_AUTH_USER_SIZE_RESP     = 3071,
    ACE_GET_AUTH_USER_NEXT          = 3072,
    ACE_GET_AUTH_USER_NEXT_RESP     = 3073,
    ACE_DEL_AUTH_USER               = 3074,
    ACE_DEL_AUTH_USER_RESP          = 3075,
    ACE_CLEAR_AUTH_USER             = 3076,
    ACE_CLEAR_AUTH_USER_RESP        = 3077,
    ACE_SET_USER_GROUP              = 3078,
    ACE_SET_USER_GROUP_RESP         = 3079,
    ACE_GET_USER_GROUP_SIZE         = 3080,
    ACE_GET_USER_GROUP_SIZE_RESP    = 3081,
    ACE_GET_USER_GROUP_NEXT         = 3082,
    ACE_GET_USER_GROUP_NEXT_RESP    = 3083,
    ACE_DEL_USER_GROUP              = 3084,
    ACE_DEL_USER_GROUP_RESP         = 3085,
    ACE_CLEAR_USER_GROUP            = 3086,
    ACE_CLEAR_USER_GROUP_RESP       = 3087,

    ACE_LOCAL_USER_AUTH             = 3088,
    ACE_LOCAL_USER_AUTH_RESP        = 3089,

    ACE_SET_AUTH_LANGUAGE           = 3090,
    ACE_SET_AUTH_LANGUAGE_RESP      = 3091,
    ACE_SET_AUTH_PORT               = 3092,
    ACE_SET_AUTH_PORT_RESP          = 3093,
    ACE_SET_AUTH_POSITION           = 3094,
    ACE_SET_AUTH_POSITION_RESP      = 3095,
    ACE_SET_AUTH_COLOR              = 3096,
    ACE_SET_AUTH_COLOR_RESP         = 3097,
    ACE_SET_AUTH_TIMEOUT            = 3098,
    ACE_SET_AUTH_TIMEOUT_RESP       = 3099,
    ACE_SET_AUTH_TITLE              = 3100,
    ACE_SET_AUTH_TITLE_RESP         = 3101,
    ACE_SET_AUTH_IMAGE              = 3102,
    ACE_SET_AUTH_IMAGE_RESP         = 3103,
    ACE_SET_AUTH_ERROR_MSG          = 3104,
    ACE_SET_AUTH_ERROR_MSG_RESP     = 3105,
    ACE_SET_AUTH_PORTAL_MSG         = 3106,
    ACE_SET_AUTH_PORTAL_MSG_RESP    = 3107,
    ACE_GET_AUTH_LANGUAGE           = 3108,
    ACE_GET_AUTH_LANGUAGE_RESP      = 3109,
    ACE_GET_AUTH_PORT               = 3110,
    ACE_GET_AUTH_PORT_RESP          = 3111,
    ACE_GET_AUTH_POSITION           = 3112,
    ACE_GET_AUTH_POSITION_RESP      = 3113,
    ACE_GET_AUTH_COLOR              = 3114,
    ACE_GET_AUTH_COLOR_RESP         = 3115,
    ACE_GET_AUTH_TIMEOUT            = 3116,
    ACE_GET_AUTH_TIMEOUT_RESP       = 3117,
    ACE_GET_AUTH_TITLE              = 3118,
    ACE_GET_AUTH_TITLE_RESP         = 3119,
    ACE_GET_AUTH_IMAGE              = 3120,
    ACE_GET_AUTH_IMAGE_RESP         = 3121,
    ACE_GET_AUTH_ERROR_MSG          = 3122,
    ACE_GET_AUTH_ERROR_MSG_RESP     = 3123,
    ACE_GET_AUTH_PORTAL_MSG         = 3124,
    ACE_GET_AUTH_PORTAL_MSG_RESP    = 3125,

    ACE_SET_DNS_PING_PERMIT          = 3126,
    ACE_SET_DNS_PING_PERMIT_RESP    = 3127,
    ACE_GET_DNS_PING_PERMIT          = 3128,
    ACE_GET_DNS_PING_PERMIT_RESP          = 3129,


    ACE_USER_LOGIN_NOTIFY           = 3130,
    ACE_USER_LOGIN_NOTIFY_RESP      = 3131,
    ACE_USER_LOGOUT_NOTIFY          = 3132,
    ACE_USER_LOGOUT_NOTIFY_RESP     = 3133,
    ACE_USER_KICKOFF_NOTIFY         = 3134,
    ACE_USER_KICKOFF_NOTIFY_RESP    = 3135,
    ACE_SINGLE_LOGIN_NOTIFY         = 3136,
    ACE_SINGLE_LOGIN_NOTIFY_RESP    = 3137,
    ACE_SINGLE_LOGOUT_NOTIFY        = 3138,
    ACE_SINGLE_LOGOUT_NOTIFY_RESP   = 3139,

    ACE_SET_BLACKLIST_PIC          = 3140,
    ACE_SET_BLACKLIST_PIC_RESP     = 3141,
    ACE_GET_BLACKLIST_PIC          = 3142,
    ACE_GET_BLACKLIST_PIC_RESP     = 3143,
    ACE_SET_BLACKLIST_MSG          = 3144,
    ACE_SET_BLACKLIST_MSG_RESP     = 3145,
    ACE_GET_BLACKLIST_MSG          = 3146,
    ACE_GET_BLACKLIST_MSG_RESP     = 3147,

    HYTF_ACCESS_RULE_QQ_NOTIFY        = 3148,
    HYTF_ACCESS_RULE_QQ_NOTIFY_RESP   = 3149,

    ACE_USER_KICKOFF_ALL_NOTIFY         = 3150,
    ACE_USER_KICKOFF_ALL_NOTIFY_RESP    = 3151,

    HYTF_NO_AUDIT_KEY                   = 3152,
    HYTF_NO_AUDIT_KEY_RESP          = 3153,
    HYTF_WEBSSO_USERNAME_NOTIFY         = 3154,
    HYTF_WEBSSO_USERNAME_NOTIFY_RESP    = 3155,
    HYTF_WEBSSO_LOGIN_OK_NOTIFY         = 3156,
    HYTF_WEBSSO_LOGIN_OK_NOTIFY_RESP    = 3157,
    HYTF_WEBSSO_LOGIN_FAIL_NOTIFY         = 3158,
    HYTF_WEBSSO_LOGIN_FAIL_NOTIFY_RESP    = 3159,
    HYTF_WEBSSO_SET         = 3160,
    HYTF_WEBSSO_SET_RESP         = 3161,
    HYTF_WEBSSO_GET         = 3162,
    HYTF_WEBSSO_GET_RESP         = 3163,
    HYTF_WEBSSO_WANGXING_SET         = 3164,
    HYTF_WEBSSO_WANGXING_SET_RESP   = 3165,

    ACE_SET_HENGBANG                  = 3166,
    ACE_SET_HENGBANG_RESP             = 3167,
    ACE_GET_HENGBANG_SIZE             = 3168,
    ACE_GET_HENGBANG_SIZE_RESP        = 3169,
    ACE_GET_HENGBANG_NEXT             = 3170,
    ACE_GET_HENGBANG_NEXT_RESP        = 3171,
    ACE_DEL_HENGBANG                  = 3172,
    ACE_DEL_HENGBANG_RESP             = 3173,
    ACE_CLEAR_HENGBANG                = 3174,
    ACE_CLEAR_HENGBANG_RESP           = 3175,
    
    HYTF_CITYHOTSPOT_LOGIN_NOTIFY        = 3176,
    HYTF_CITYHOTSPOT_LOGIN_NOTIFY_RESP   = 3177,
    HYTF_CITYHOTSPOT_LOGOUT_NOTIFY        = 3178,
    HYTF_CITYHOTSPOT_LOGOUT_NOTIFY_RESP   = 3179,

    HYTF_SET_PAIBO                  = 3180,
    HYTF_SET_PAIBO_RESP             = 3181,
    HYTF_GET_PAIBO_SIZE             = 3182,
    HYTF_GET_PAIBO_SIZE_RESP        = 3183,
    HYTF_GET_PAIBO_NEXT             = 3184,
    HYTF_GET_PAIBO_NEXT_RESP        = 3185,
    HYTF_DEL_PAIBO                  = 3186,
    HYTF_DEL_PAIBO_RESP             = 3187,
    HYTF_CLEAR_PAIBO                = 3188,
    HYTF_CLEAR_PAIBO_RESP           = 3189,

    HYTF_HTTPLINK_LOGIN_NOTIFY        = 3190,
    HYTF_HTTPLINK_LOGIN_NOTIFY_RESP   = 3191,
    HYTF_HTTPLINK_LOGOUT_NOTIFY        = 3192,
    HYTF_HTTPLINK_LOGOUT_NOTIFY_RESP   = 3193,

    HYTF_KICKOFF_USER_BY_USERNAME_NOTIFY         = 3194,
    HYTF_AUTH_POLICY_MATCH_AUTO_ADD = 3195,
    HYTF_AUTH_POLICY_MATCH_AUTO_ADD_RESP = 3196,
    HYTF_LAN_SHARE_IP_FLASH_COOKIE_SET = 3197,
    HYTF_LAN_SHARE_IP_FLASH_COOKIE_SET_RESP = 3198,

    ACE_ADSSO_LOG_LOGIN_NOTIFY = 3199,
    ACE_USER_SET_IPUSERMAPPING = 3200,
    ACE_USER_SET_IPUSERMAPPING_RESP = 3201,
    ACE_USER_DEL_IPUSERMAPPING = 3202,
    ACE_USER_DEL_IPUSERMAPPING_RESP = 3203,
    ACE_GET_IPUSERMAPPING_SIZE = 3204,
    ACE_GET_IPUSERMAPPING_SIZE_RESP = 3205,
    ACE_USER_QUERY_IPUSERMAPPING = 3206,
    ACE_USER_QUERY_IPUSERMAPPING_RESP = 3207,
    ACE_USER_CLEAR_IPUSERMAPPING = 3208,
    ACE_USER_CLEAR_IPUSERMAPPING_RESP = 3209,
    ACE_SET_USER_BLACKLIST              = 3210,
    ACE_SET_USER_BLACKLIST_RESP         = 3211,
    ACE_GET_USER_BLACKLIST_SIZE         = 3212,
    ACE_GET_USER_BLACKLIST_SIZE_RESP    = 3213,
    ACE_GET_USER_BLACKLIST_NEXT         = 3214,
    ACE_GET_USER_BLACKLIST_NEXT_RESP    = 3215,
    ACE_DEL_USER_BLACKLIST              = 3216,
    ACE_DEL_USER_BLACKLIST_RESP         = 3217,
    ACE_CLEAR_USER_BLACKLIST            = 3218,
    ACE_CLEAR_USER_BLACKLIST_RESP       = 3219,
    ACE_SET_MANUAL_USER_BLACKLIST       = 3220,
    ACE_SET_MANUAL_USER_BLACKLIST_RESP  = 3221,
    ACE_GET_MANUAL_USER_BLACKLIST_SIZE         = 3222,
    ACE_GET_MANUAL_USER_BLACKLIST_SIZE_RESP    = 3223,
    ACE_GET_MANUAL_USER_BLACKLIST_NEXT         = 3224,
    ACE_GET_MANUAL_USER_BLACKLIST_NEXT_RESP    = 3225,
    ACE_DEL_MANUAL_USER_BLACKLIST              = 3226,
    ACE_DEL_MANUAL_USER_BLACKLIST_RESP         = 3227,
    ACE_CLEAR_MANUAL_USER_BLACKLIST            = 3228,
    ACE_CLEAR_MANUAL_USER_BLACKLIST_RESP       = 3229,
    ACE_IS_POLICY_OBJECT_USED                   = 3230,
    ACE_IS_POLICY_OBJECT_USED_RESP             = 3231,
    ACE_MODIFY_AUTHUSER_PASSWD                  =3232,
    ACE_MODIFY_AUTHUSER_PASSWD_RET          =3233,
    ACE_GET_SYS_USER_AUTH_SERVER            =3234,
    ACE_GET_SYS_USER_AUTH_SERVER_RESP       =3235,
    ACE_SET_AUTH_OVER_LOGIN_ACTION              = 3236,
    ACE_SET_AUTH_OVER_LOGIN_ACTION_RESP         = 3237,
    ACE_GET_AUTH_OVER_LOGIN_ACTION              = 3238,
    ACE_GET_AUTH_OVER_LOGIN_ACTION_RESP         = 3239,
    ACE_SET_AUTH_SHARE_COUNT              = 3240,
    ACE_SET_AUTH_SHARE_COUNT_RESP         = 3241,
    ACE_GET_AUTH_SHARE_COUNT              = 3242,
    ACE_GET_AUTH_SHARE_COUNT_RESP         = 3243,
    ACE_SET_AUTH_PPPOE_SSO              = 3244,
    ACE_SET_AUTH_PPPOE_SSO_RESP         = 3245,
    ACE_GET_AUTH_PPPOE_SSO              = 3246,
    ACE_GET_AUTH_PPPOE_SSO_RESP         = 3247,
    ACE_SET_AUTH_AD_SSO              = 3248,
    ACE_SET_AUTH_AD_SSO_RESP         = 3249,
    ACE_SET_AUTH_AD_SSO_SERVER              = 3250,
    ACE_SET_AUTH_AD_SSO_SERVER_RESP         = 3251,
    ACE_GET_AUTH_AD_SSO              = 3252,
    ACE_GET_AUTH_AD_SSO_RESP         = 3253,
    ACE_GET_AUTH_AD_SSO_SERVER              = 3254,
    ACE_GET_AUTH_AD_SSO_SERVER_RESP         = 3255,

    MAILALARM_LEVEL_UPDATE=3256,
    MAILALARM_LEVEL_UPDATE_RET=3257,

    ACE_SET_AUTH_CITYHOTSPOT_SSO              = 3258,
    ACE_SET_AUTH_CITYHOTSPOT_SSO_RESP         = 3259,
    ACE_GET_AUTH_CITYHOTSPOT_SSO              = 3260,
    ACE_GET_AUTH_CITYHOTSPOT_SSO_RESP         = 3261,

    HYTF_SET_WLAN_STATION_POLICY                 = 3262,
    HYTF_SET_WLAN_STATION_POLICY_RESP       = 3263,
    HYTF_SET_WLAN_STATION_WHITELIST           = 3264,
    HYTF_SET_WLAN_STATION_WHITELIST_RESP = 3265,
    HYTF_GET_WLAN_STATION_POLICY                 = 3266,
    HYTF_GET_WLAN_STATION_POLICY_RESP       = 3267,
    HYTF_GET_WLAN_STATION_WHITELIST_SIZE = 3268,
    HYTF_GET_WLAN_STATION_WHITELIST_SIZE_RESP = 3269,
    HYTF_GET_WLAN_STATION_WHITELIST = 3270,
    HYTF_GET_WLAN_STATION_WHITELIST_RESP = 3271,
    HYTF_GET_WLAN_STATION_WHITELIST_NEXT         = 3272,
    HYTF_GET_WLAN_STATION_WHITELIST_NEXT_RESP    = 3273,
    HYTF_CLEAR_WLAN_STATION_WHITELIST            = 3274,
    HYTF_CLEAR_WLAN_STATION_WHITELIST_RESP       = 3275,
    HYTF_DEL_WLAN_STATION_WHITELIST              = 3276,
    HYTF_DEL_WLAN_STATION_WHITELIST_RESP         = 3277,
    HYTF_GET_WLAN_STATION_WHITELIST_IPPAIRE           = 3278,
    HYTF_GET_WLAN_STATION_WHITELIST_IPPAIRE_RESP = 3279,

    HYTF_SET_AUTH_HTTPLINK_SSO              = 3280,
    HYTF_SET_AUTH_HTTPLINK_SSO_RESP         = 3281,
    HYTF_GET_AUTH_HTTPLINK_SSO              = 3282,
    HYTF_GET_AUTH_HTTPLINK_SSO_RESP         = 3283,

    HYTF_SET_AUTH_HTTPLINK_SSO_OPTION              = 3284,
    HYTF_SET_AUTH_HTTPLINK_SSO_OPTION_RESP         = 3285,
    HYTF_GET_AUTH_HTTPLINK_SSO_OPTION              = 3286,
    HYTF_GET_AUTH_HTTPLINK_SSO_OPTION_RESP         = 3287,

    HYTF_SET_AUTH_PORTAL_FLAG              = 3288,
    HYTF_SET_AUTH_PORTAL_FLAG_RESP         = 3289,
    HYTF_GET_AUTH_PORTAL_FLAG              = 3290,
    HYTF_GET_AUTH_PORTAL_FLAG_RESP         = 3291,

    HYTF_VLAN_AUTH_FLAG = 3292,
    HYTF_VLAN_AUTH_REQUEST = 3293,
    HYTF_VLAN_AUTH_REPLY = 3294,

    HYTF_SET_ZHIAN                  = 3295,
    HYTF_SET_ZHIAN_RESP             = 3296,
    HYTF_GET_ZHIAN_SIZE             = 3297,
    HYTF_GET_ZHIAN_SIZE_RESP        = 3298,
    HYTF_GET_ZHIAN_NEXT             = 3299,
    HYTF_GET_ZHIAN_NEXT_RESP        = 3300,
    HYTF_DEL_ZHIAN                  = 3301,
    HYTF_DEL_ZHIAN_RESP             = 3302,
    HYTF_CLEAR_ZHIAN                = 3303,
    HYTF_CLEAR_ZHIAN_RESP           = 3304,

    HYTF_ZHIAN_WEB_AUTH_REQUEST = 3305,
    HYTF_ZHIAN_WEB_AUTH_REPLY = 3306,
    HYTF_CENTER_KEYWORD_UPDATE_REQUEST = 3307,
    HYTF_CENTER_KEYWORD_UPDATE_REPLY = 3308,

    HYTF_SET_AD_MIRROR_SSO              = 3309,
    HYTF_SET_AD_MIRROR_SSO_RESP         = 3310,
    HYTF_GET_AD_MIRROR_SSO              = 3311,
    HYTF_GET_AD_MIRROR_SSO_RESP         = 3312,

    HYTF_SET_MIRROR_PORT_SSO              = 3313,
    HYTF_SET_MIRROR_PORT_SSO_RESP         = 3314,
    HYTF_GET_MIRROR_PORT_SSO              = 3315,
    HYTF_GET_MIRROR_PORT_SSO_RESP         = 3316,

    HYTF_SYSAPP_DEBUG_PRINT_SHAREIP = 3317,

    ACE_SET_AUTH_H3CIMC_SSO              = 3318,
    ACE_SET_AUTH_H3CIMC_SSO_RESP         = 3319,
    ACE_GET_AUTH_H3CIMC_SSO              = 3320,
    ACE_GET_AUTH_H3CIMC_SSO_RESP         = 3321,

    HYTF_H3CIMC_LOGIN_NOTIFY        = 3322,
    HYTF_H3CIMC_LOGIN_NOTIFY_RESP   = 3323,
    HYTF_H3CIMC_LOGOUT_NOTIFY        = 3234,
    HYTF_H3CIMC_LOGOUT_NOTIFY_RESP   = 3325,

    HYTF_ACCESS_RULE_LOGOUT_NOTIFY        = 3326,
    HYTF_ACCESS_RULE_LOGOUT_NOTIFY_RESP   = 3327,

    HYTF_SET_AUTH_EVERYDAY_REAUTH              = 3328,
    HYTF_SET_AUTH_EVERYDAY_REAUTH_RESP         = 3329,
    HYTF_GET_AUTH_EVERYDAY_REAUTH              = 3330,
    HYTF_GET_AUTH_EVERYDAY_REAUTH_RESP         = 3331,

    HYTF_ZHIAN_LOCAL_USER_MSG_ID = 3332,
    HYTF_ZHIAN_LOCAL_USER_ADD = 3333,
    HYTF_ZHIAN_LOCAL_USER_ADD_RESP = 3334,
    HYTF_ZHIAN_LOCAL_USER_DEL = 3335,
    HYTF_ZHIAN_LOCAL_USER_DEL_RESP = 3336,
    HYTF_ZHIAN_LOCAL_USER_QUERY = 3337,
    HYTF_ZHIAN_LOCAL_USER_QUERY_RESP = 3338,
    HYTF_ZHIAN_LOCAL_USER_SIZE = 3339,
    HYTF_ZHIAN_LOCAL_USER_SIZE_RESP = 3340,
    HYTF_ZHIAN_LOCAL_USER_LOGOUT = 3341,
    HYTF_ZHIAN_LOCAL_USER_LOGOUT_RESP = 3342,

    HYTF_SET_PORTAL_SERVER                  = 3343,
    HYTF_SET_PORTAL_SERVER_RESP             = 3344,
    HYTF_GET_PORTAL_SERVER_SIZE             = 3345,
    HYTF_GET_PORTAL_SERVER_SIZE_RESP        = 3346,
    HYTF_GET_PORTAL_SERVER_NEXT             = 3347,
    HYTF_GET_PORTAL_SERVER_NEXT_RESP        = 3348,
    HYTF_DEL_PORTAL_SERVER                  = 3349,
    HYTF_DEL_PORTAL_SERVER_RESP             = 3350,
    HYTF_CLEAR_PORTAL_SERVER                = 3351,
    HYTF_CLEAR_PORTAL_SERVER_RESP           = 3352,
    HYTF_PORTAL_USER_LOGIN_NOTIFY           = 3353,
    HYTF_PORTAL_USER_LOGIN_RESP           = 3354,

    HYTF_SMS_USER_AUTH             = 3355,
    HYTF_SMS_USER_AUTH_RESP        = 3356,
    HYTF_SMS_USER_LOGIN_NOTIFY             = 3357,
    HYTF_SMS_USER_LOGIN_NOTIFY_RESP        = 3358,
    HYTF_SMS_USER_COOKIE_NOTIFY             = 3359,
    HYTF_SMS_USER_COOKIE_NOTIFY_RESP        = 3360,

    HYTF_CGI_GET_RADIUS_AUTH_SERVER          = 3361,
    HYTF_CGI_GET_RADIUS_AUTH_SERVER_RESP     = 3362,

    HYTF_HTTPLINK_WEIXIN_AUTH_NOTIFY        = 3363,
    HYTF_HTTPLINK_WEIXIN_AUTH_NOTIFY_RESP        = 3364,

    HYTF_WEIXIN_AUTH_MATCH_AUTO_LOGIN = 3365,
    HYTF_WEIXIN_AUTH_MATCH_AUTO_LOGIN_RESP = 3366,
    HYTF_GET_VALID_SCHEDULE_NEXT = 3367,
    HYTF_GET_VALID_SCHEDULE_NEXT_RESP = 3368,

    HYTF_DINGDING_AUTH_LOGIN = 3369,
    HYTF_DINGDING_AUTH_LOGIN_RESP = 3370,

    HYTF_ENTERPRISE_WECHAT_AUTH_LOGIN = 3371,
    HYTF_ENTERPRISE_WECHAT_AUTH_LOGIN_RESP = 3372,

    HYTF_SET_AUTH_PROXY_SSO              = 3373,
    HYTF_SET_AUTH_PROXY_SSO_RESP         = 3374,
    HYTF_GET_AUTH_PROXY_SSO              = 3375,
    HYTF_GET_AUTH_PROXY_SSO_RESP         = 3376,

    HYTF_WECHAT_AUTH_SET         = 3377,
    HYTF_WECHAT_AUTH_SET_RESP         = 3378,
    HYTF_WECHAT_AUTH_GET         = 3379,
    HYTF_WECHAT_AUTH_GET_RESP         = 3380,

    ACE_SET_SMP                    = 3381,
    ACE_SET_SMP_RESP               = 3382,
    ACE_GET_SMP_SIZE               = 3383,
    ACE_GET_SMP_SIZE_RESP          = 3384,
    ACE_GET_SMP_NEXT               = 3385,
    ACE_GET_SMP_NEXT_RESP          = 3386,
    ACE_DEL_SMP                    = 3387,
    ACE_DEL_SMP_RESP               = 3388,
    ACE_CLEAR_SMP                  = 3389,
    ACE_CLEAR_SMP_RESP             = 3390,

    HYTF_SMP_USER_LOGIN_NOTIFY         = 3391,
    HYTF_SMP_USER_LOGIN_RESP           = 3392,
    HYTF_SMP_USER_SSO_LOGIN_NOTIFY     = 3393,
    HYTF_SMP_USER_SSO_LOGIN_RESP       = 3394,
    HYTF_SMP_USER_SSO_LOGOUT_NOTIFY    = 3395,
    HYTF_SMP_USER_SSO_LOGOUT_RESP      = 3396,
    HYTF_CGI_GET_LADP_AUTH_SERVER      = 3397,
    HYTF_CGI_GET_LADP_AUTH_SERVER_RESP = 3398,

    ACE_SET_AUTH_FTP_SSO              = 3399,
    ACE_SET_AUTH_FTP_SSO_RESP         = 3400,
    ACE_GET_AUTH_FTP_SSO              = 3401,
    ACE_GET_AUTH_FTP_SSO_RESP         = 3402,
	HYTF_SAM_USER_SSO_LOGIN_NOTIFY     = 3403,
	HYTF_SAM_USER_SSO_LOGIN_RESP       = 3404,
	HYTF_SAM_USER_SSO_LOGOUT_NOTIFY    = 3405,
	HYTF_SAM_USER_SSO_LOGOUT_RESP      = 3406,

	HYTF_SET_AUTH_MAC_FREE_AUTH        = 3407,
	HYTF_SET_AUTH_MAC_FREE_AUTH_RESP   = 3408,
	HYTF_GET_AUTH_MAC_FREE_AUTH        = 3409,
	HYTF_GET_AUTH_MAC_FREE_AUTH_RESP   = 3410,

    HYTF_SMP_2009_SSO_LOGIN_NOTIFY     = 3411,
	HYTF_SMP_2009_SSO_LOGIN_RESP       = 3412,
	HYTF_SMP_2009_SSO_LOGOUT_NOTIFY    = 3413,
	HYTF_SMP_2009_SSO_LOGOUT_RESP      = 3414,

	ACE_MODIFY_AUTHUSER_PASSWD2        = 3415,

    HYTF_DDI_USER_SSO_LOGIN_NOTIFY     = 3420,
	HYTF_DDI_USER_SSO_LOGIN_RESP       = 3421,
	HYTF_DDI_USER_SSO_LOGOUT_NOTIFY    = 3422,
	HYTF_DDI_USER_SSO_LOGOUT_RESP      = 3423,

	HYTF_BLACKLIST_REMIND_RATIO_NOTIFY	= 3424,
    HYTF_BLACKLIST_REMIND_RATIO_RESP    = 3425,

    HYTF_THIRD_INTERFACE_SSO = 3426,

    HYTF_WECHAT_CONNECT_ATTENTION_UPDATE 		= 3428,
	HYTF_WECHAT_CONNECT_ATTENTION_UPDATE_RESP	= 3429,

    HYTF_MCP_USER_SSO_LOGIN_NOTIFY     = 3430,
	HYTF_MCP_USER_SSO_LOGIN_RESP       = 3431,
	HYTF_MCP_USER_SSO_LOGOUT_NOTIFY    = 3432,
	HYTF_MCP_USER_SSO_LOGOUT_RESP      = 3433,

    HYTF_CLEAR_AUTH_MAC_FREE_AUTH        = 3434,
	HYTF_CLEAR_AUTH_MAC_FREE_AUTH_RESP   = 3435,

	HYTF_ADSSO_LOG_ADD               = 3436,
	HYTF_ADSSO_LOG_ADD_RESP          = 3437,
    HYTF_ADSSO_LOG_DEL               = 3438,
    HYTF_ADSSO_LOG_DEL_RESP          = 3439,
    HYTF_ADSSO_LOG_MOD               = 3440,
    HYTF_ADSSO_LOG_MOD_RESP          = 3441,
	HYTF_ADSSO_LOG_GET_SIZE          = 3442,
	HYTF_ADSSO_LOG_GET_SIZE_RESP     = 3443,
    HYTF_ADSSO_LOG_GET               = 3444,
    HYTF_ADSSO_LOG_GET_RESP          = 3445,
    HYTF_ADSSO_LOG_GET_STATUS        = 3446,
    HYTF_ADSSO_LOG_GET_STATUS_RESP   = 3447,
    HYTF_ADSSO_LOG_SET_STATUS        = 3448,
    HYTF_ADSSO_LOG_SET_STATUS_RESP   = 3449,

    HYTF_METRO_USER_SSO_LOGIN_NOTIFY     = 3450,
	HYTF_METRO_USER_SSO_LOGIN_RESP       = 3451,
	HYTF_METRO_USER_SSO_LOGOUT_NOTIFY    = 3452,
	HYTF_METRO_USER_SSO_LOGOUT_RESP      = 3453,

    ACE_SET_AUTH_METRO_SSO              = 3454,
    ACE_SET_AUTH_METRO_SSO_RESP         = 3455,
    ACE_GET_AUTH_METRO_SSO              = 3456,
    ACE_GET_AUTH_METRO_SSO_RESP         = 3457,

    ACE_SET_AUTH_METRO_KEY              = 3458,
    ACE_SET_AUTH_METRO_KEY_RESP         = 3459,
    ACE_GET_AUTH_METRO_KEY              = 3460,
    ACE_GET_AUTH_METRO_KEY_RESP         = 3461,

    ACE_USER_HA_SYNC_NOTIFY             = 3464,
	ACE_USER_HA_SYNC_RESP               = 3465,

    ACE_SET_AUTH_SMART_CARD              = 3466,
    ACE_SET_AUTH_SMART_CARD_RESP         = 3467,

    HYTF_FINGERPRINT_RECORD_NOTIFY         = 3468,
    HYTF_FINGERPRINT_RECORD_RESP           = 3469,
    HYTF_FINGERPRINT_USER_LOGIN_NOTIFY     = 3470,
    HYTF_FINGERPRINT_USER_LOGIN_RESP       = 3471,
    HYTF_FINGERPRINT_GET_SIZE              = 3472,
    HYTF_FINGERPRINT_GET_SIZE_RESP         = 3473,
    HYTF_FINGERPRINT_GET                   = 3474,
    HYTF_FINGERPRINT_GET_RESP              = 3475,
    HYTF_FINGERPRINT_DELETE                = 3476,
    HYTF_FINGERPRINT_DELETE_RESP           = 3477,
    HYTF_FINGERPRINT_CLEAR                 = 3478,
    HYTF_FINGERPRINT_CLEAR_RESP            = 3479,

    HYTF_H3CIMC_SSO_RECORD_GET_SIZE              = 3480,
    HYTF_H3CIMC_SSO_RECORD_GET_SIZE_RESP         = 3481,
    HYTF_H3CIMC_SSO_RECORD_GET                   = 3482,
    HYTF_H3CIMC_SSO_RECORD_GET_RESP              = 3483,
    HYTF_H3CIMC_SSO_RECORD_DELETE                = 3484,
    HYTF_H3CIMC_SSO_RECORD_DELETE_RESP           = 3485,
    HYTF_H3CIMC_SSO_RECORD_CLEAR                 = 3486,
    HYTF_H3CIMC_SSO_RECORD_CLEAR_RESP            = 3487,
    HYTF_H3CIMC_SSO_RECORD_GET_BY_NAME           = 3488,
    HYTF_H3CIMC_SSO_RECORD_GET_BY_NAME_RESP      = 3489,
	HYTF_H3CIMC_SSO_RECORD_IMPORT				 = 3490,
	HYTF_H3CIMC_SSO_RECORD_IMPORT_RESP			 = 3491,
	HYTF_H3CIMC_SSO_RECORD_GET_BY_NAME2          = 3492,
    HYTF_H3CIMC_SSO_RECORD_GET_BY_NAME2_RESP     = 3493,

    HYTF_CSP_ESP_SSO_LOGIN_NOTIFY     = 3494,
	HYTF_CSP_ESP_SSO_LOGIN_RESP       = 3495,

    HYTF_LOGIN_CHECK_PASSWD_AGEOUT      = 3496,
	HYTF_LOGIN_CHECK_PASSWD_AGEOUT_RESP = 3497,

    HYTF_DKEY_LOGIN = 3498,
    HYTF_DKEY_LOGIN_RESP = 3499,

    ACE_GET_AUTH_CITYHOTSPOT_PORT             = 3500,
	ACE_GET_AUTH_CITYHOTSPOT_PORT_RESP        = 3501,
	ACE_SET_AUTH_CITYHOTSPOT_PORT             = 3502,
	ACE_SET_AUTH_CITYHOTSPOT_PORT_RESP        = 3503,

    HYTF_HANTING_HOTEL_LOGIN_NOTIFY        = 3504,
	HYTF_HANTING_HOTEL_LOGIN_NOTIFY_RESP   = 3505,
	HYTF_HANTING_HOTEL_LOGOUT_NOTIFY        = 3506,
	HYTF_HANTING_HOTEL_LOGOUT_NOTIFY_RESP   = 3507,

    ACE_SET_AUTH_HANTING_HOTEL_SSO              = 3508,
    ACE_SET_AUTH_HANTING_HOTEL_SSO_RESP         = 3509,
    ACE_GET_AUTH_HANTING_HOTEL_SSO              = 3510,
    ACE_GET_AUTH_HANTING_HOTEL_SSO_RESP         = 3511,

    ACE_SET_SMARTCARD_USER_RECORD          = 3512,
    ACE_SET_SMARTCARD_USER_RECORD_RESP     = 3513,
    ACE_SET_SMARTCARD_USER_LOGIN           = 3514,
    ACE_SET_SMARTCARD_USER_LOGIN_RESP      = 3515,
    ACE_SET_SMARTCARD_USER_LOGOUT          = 3516,
    ACE_SET_SMARTCARD_USER_LOGOUT_RESP     = 3517,
    ACE_SET_SMARTCARD_USER_KEEPALIVE       = 3518,
    ACE_SET_SMARTCARD_USER_KEEPALIVE_RESP  = 3519,

    ACE_GET_MAC_CHANGE_REAUTH = 3520,
    ACE_GET_MAC_CHANGE_REAUTH_RESP = 3521,
    ACE_SET_MAC_CHANGE_REAUTH = 3522,
    ACE_SET_MAC_CHANGE_REAUTH_RESP = 3523,

    ACE_GET_AUTH_FINGERPRINT_SSO        = 3524,
    ACE_GET_AUTH_FINGERPRINT_SSO_RESP   = 3525,
    ACE_SET_AUTH_FINGERPRINT_SSO        = 3526,
    ACE_SET_AUTH_FINGERPRINT_SSO_RESP   = 3527,

    HYTF_USER_MANAGE_REQUEST                    = 3518,
    HYTF_USER_MANAGE_REQUEST_RESP               = 3519,

    HYTF_GET_OUT_FROM_BLACKLIST_LOGOUT_USER           = 3528,
    HYTF_GET_OUT_FROM_BLACKLIST_LOGOUT_USER_RESP      = 3529,
	HYTF_SET_OUT_FROM_BLACKLIST_LOGOUT_USER           = 3530,
    HYTF_SET_OUT_FROM_BLACKLIST_LOGOUT_USER_RESP      = 3531,
    
	HYTF_JMS_SSO_SET              = 3536,
	HYTF_JMS_SSO_SET_RESP         = 3537,
	HYTF_JMS_SSO_GET              = 3538,
	HYTF_JMS_SSO_GET_RESP         = 3539,
    HYTF_SAM_WEBSERVICE_SSO_SET              = 3540,
    HYTF_SAM_WEBSERVICE_SSO_SET_RESP         = 3541,
    HYTF_SAM_WEBSERVICE_SSO_GET              = 3542,
    HYTF_SAM_WEBSERVICE_SSO_GET_RESP         = 3543,

    ACE_GET_CSP_ESP_SSO              = 3544,
    ACE_GET_CSP_ESP_SSO_RESP         = 3545,
    ACE_SET_CSP_ESP_SSO              = 3546,
    ACE_SET_CSP_ESP_SSO_RESP         = 3547,

	ACE_GET_USER_LIMIT_SIZE      = 3548,

    ACE_SET_AWIFI_USER_LOGOUT      = 3592,
    ACE_SET_AWIFI_USER_LOGOUT_RESP = 3593,

    ACE_SET_AWIFI_USER_LOGIN         = 3594,
    ACE_SET_AWIFI_USER_LOGIN_RESP    = 3595,

    ACE_SET_WANGXING_USER_LOGINOUT      = 3596,
    ACE_SET_WANGXING_USER_LOGINOUT_RESP = 3597,

    ACE_SET_WANGXING_USER_LOGIN         = 3598,
    ACE_SET_WANGXING_USER_LOGIN_RESP    = 3599,

    ACE_DANS_OBJECT_CONFIG              = 3600,
    KEYWORD_OBJECT_ADD = 3601,
    KEYWORD_OBJECT_ADD_RET = 3602,
    KEYWORD_OBJECT_DEL = 3603,
    KEYWORD_OBJECT_DEL_RET = 3604,
    KEYWORD_OBJECT_MODIFY = 3605,
    KEYWORD_OBJECT_MODIFY_RET = 3606,
    KEYWORD_OBJECT_CLEAR = 3607,
    KEYWORD_OBJECT_CLEAR_RET = 3608,
    KEYWORD_OBJECT_SIZE = 3609,
    KEYWORD_OBJECT_SIZE_RET = 3610,
    KEYWORD_OBJECT_GET = 3611,
    KEYWORD_OBJECT_GET_RET = 3612,
    KEYWORD_OBJECT_GET_BYINDEX = 3613,
    KEYWORD_OBJECT_GET_BYINDEX_RET = 3614,
    FILEFILTER_OBJECT_ADD = 3615,
    FILEFILTER_OBJECT_ADD_RET = 3616,
    FILEFILTER_OBJECT_DEL = 3617,
    FILEFILTER_OBJECT_DEL_RET = 3618,
    FILEFILTER_OBJECT_MODIFY = 3619,
    FILEFILTER_OBJECT_MODIFY_RET = 3620,
    FILEFILTER_OBJECT_CLEAR = 3621,
    FILEFILTER_OBJECT_CLEAR_RET = 3622,
    FILEFILTER_OBJECT_SIZE = 3623,
    FILEFILTER_OBJECT_SIZE_RET = 3624,
    FILEFILTER_OBJECT_GET = 3625,
    FILEFILTER_OBJECT_GET_RET = 3626,
    FILEFILTER_OBJECT_GET_BYINDEX = 3627,
    FILEFILTER_OBJECT_GET_BYINDEX_RET = 3628,
    URLCUSTOM_OBJECT_ADD = 3629,
    URLCUSTOM_OBJECT_ADD_RET = 3630,
    URLCUSTOM_OBJECT_DEL = 3631,
    URLCUSTOM_OBJECT_DEL_RET = 3632,
    URLCUSTOM_OBJECT_MODIFY = 3633,
    URLCUSTOM_OBJECT_MODIFY_RET = 3634,
    URLCUSTOM_OBJECT_CLEAR = 3635,
    URLCUSTOM_OBJECT_CLEAR_RET = 3636,
    URLCUSTOM_OBJECT_SIZE = 3637,
    URLCUSTOM_OBJECT_SIZE_RET = 3638,
    URLCUSTOM_OBJECT_GET = 3639,
    URLCUSTOM_OBJECT_GET_RET = 3640,
    URLCUSTOM_OBJECT_GET_BYINDEX = 3641,
    URLCUSTOM_OBJECT_GET_BYINDEX_RET = 3642,
    URLLIB_OBJECT_SIZE = 3643,
    URLLIB_OBJECT_SIZE_RET = 3644,
    URLLIB_OBJECT_GET = 3645,
    URLLIB_OBJECT_GET_RET = 3646,
    URLLIB_OBJECT_GET_BYINDEX = 3647,
    URLLIB_OBJECT_GET_BYINDEX_RET = 3648,
    WHITENAME_OBJECT_ADD = 3649,
    WHITENAME_OBJECT_ADD_RET = 3650,
    WHITENAME_OBJECT_DEL = 3651,
    WHITENAME_OBJECT_DEL_RET = 3652,
    WHITENAME_OBJECT_MODIFY = 3653,
    WHITENAME_OBJECT_MODIFY_RET = 3654,
    WHITENAME_OBJECT_CLEAR = 3655,
    WHITENAME_OBJECT_CLEAR_RET = 3656,
    WHITENAME_OBJECT_SIZE = 3657,
    WHITENAME_OBJECT_SIZE_RET = 3658,
    WHITENAME_OBJECT_GET = 3659,
    WHITENAME_OBJECT_GET_RET = 3660,
    WHITENAME_OBJECT_GET_BYINDEX = 3661,
    WHITENAME_OBJECT_GET_BYINDEX_RET = 3662,
    /*system object move and insert*/
    KEYWORD_OBJECT_MOVE = 3663,
    KEYWORD_OBJECT_MOVE_RET = 3664,
    KEYWORD_OBJECT_INSERT = 3665,
    KEYWORD_OBJECT_INSERT_RET = 3666,
    FILEFILTER_OBJECT_MOVE = 3667,
    FILEFILTER_OBJECT_MOVE_RET = 3668,
    FILEFILTER_OBJECT_INSERT = 3669,
    FILEFILTER_OBJECT_INSERT_RET = 3670,
    URLCUSTOM_OBJECT_MOVE = 3671,
    URLCUSTOM_OBJECT_MOVE_RET = 3672,
    URLCUSTOM_OBJECT_INSERT = 3673,
    URLCUSTOM_OBJECT_INSERT_RET = 3674,
    WHITENAME_OBJECT_MOVE = 3675,
    WHITENAME_OBJECT_MOVE_RET = 3676,
    WHITENAME_OBJECT_INSERT = 3677,
    WHITENAME_OBJECT_INSERT_RET = 3678,
    /*im whitelist*/
    IM_WHITELIST_RULE_ADD = 3679,
    IM_WHITELIST_RULE_ADD_RET = 3680,
    IM_WHITELIST_RULE_DEL = 3681,
    IM_WHITELIST_RULE_DEL_RET = 3682,
    IM_WHITELIST_RULE_MODIFY = 3683,
    IM_WHITELIST_RULE_MODIFY_RET = 3684,
    IM_WHITELIST_RULE_CLEAR = 3685,
    IM_WHITELIST_RULE_CLEAR_RET = 3686,
    IM_WHITELIST_RULE_SIZE = 3687,
    IM_WHITELIST_RULE_SIZE_RET = 3688,
    IM_WHITELIST_RULE_GET = 3689,
    IM_WHITELIST_RULE_GET_RET = 3690,
    IM_WHITELIST_RULE_GET_BYINDEX = 3691,
    IM_WHITELIST_RULE_GET_BYINDEX_RET = 3692,
    /*url flow control*/
    URLCUSTOM_OBJECT_GET_BY_URL_FC = 3693,
    URLCUSTOM_OBJECT_GET_BY_URL_FC_RET = 3694,
    /*url whitelist*/
    URL_WHITELIST_RULE_ADD = 3695,
    URL_WHITELIST_RULE_ADD_RET = 3696,
    URL_WHITELIST_RULE_DEL = 3697,
    URL_WHITELIST_RULE_DEL_RET = 3698,
    URL_WHITELIST_RULE_MODIFY = 3699,
    URL_WHITELIST_RULE_MODIFY_RET = 3700,
    URL_WHITELIST_RULE_CLEAR = 3701,
    URL_WHITELIST_RULE_CLEAR_RET = 3702,
    URL_WHITELIST_RULE_SIZE = 3703,
    URL_WHITELIST_RULE_SIZE_RET = 3704,
    URL_WHITELIST_RULE_GET = 3705,
    URL_WHITELIST_RULE_GET_RET = 3706,
    URL_WHITELIST_RULE_GET_BYINDEX = 3707,
    URL_WHITELIST_RULE_GET_BYINDEX_RET = 3708,

    HYTF_SET_TRANSPARENT_PROXY              = 3709,
	HYTF_SET_TRANSPARENT_PROXY_RESP         = 3710,
	HYTF_GET_TRANSPARENT_PROXY              = 3711,
	HYTF_GET_TRANSPARENT_PROXY_RESP         = 3712,

    HYTF_VISITOR_AUTH_MATCH_AUTO_LOGIN = 3713,
	HYTF_VISITOR_AUTH_MATCH_AUTO_LOGIN_RESP = 3714,

	HYTF_SET_WECHAT_SERVER                  = 3720,
    HYTF_SET_WECHAT_SERVER_RESP             = 3721,
    HYTF_GET_WECHAT_SERVER_SIZE             = 3722,
    HYTF_GET_WECHAT_SERVER_SIZE_RESP        = 3723,
    HYTF_GET_WECHAT_SERVER_NEXT             = 3724,
    HYTF_GET_WECHAT_SERVER_NEXT_RESP        = 3725,
    HYTF_DEL_WECHAT_SERVER                  = 3726,
    HYTF_DEL_WECHAT_SERVER_RESP             = 3727,
    HYTF_CLEAR_WECHAT_SERVER                = 3728,
    HYTF_CLEAR_WECHAT_SERVER_RESP           = 3729,
    HYTF_DEL_LOCATION_OBJ                = 3730,
    HYTF_DEL_LOCATION_OBJ_RESP           = 3731,

    HYTF_ONLINE_AP_MAC_LIST_GET          = 3732,
    HYTF_ONLINE_AP_MAC_LIST_GET_RESP     = 3733,
    HYTF_ONLINE_AP_MAC_LIST_CLEAR        = 3734,
    HYTF_ONLINE_AP_MAC_LIST_CLEAR_RESP   = 3735,

    HYTF_MODIFY_LOCATION_OBJ             = 3736,

    ACE_GET_AUTH_SYSLOG_SSO              = 3750,
    ACE_GET_AUTH_SYSLOG_SSO_RESP         = 3751,
    ACE_SET_AUTH_SYSLOG_SSO              = 3752,
    ACE_SET_AUTH_SYSLOG_SSO_RESP         = 3753,

    ACE_GET_AUTH_DDI_SSO              = 3754,
    ACE_GET_AUTH_DDI_SSO_RESP         = 3755,
    ACE_SET_AUTH_DDI_SSO              = 3756,
    ACE_SET_AUTH_DDI_SSO_RESP         = 3757,

    ACE_GET_RADIUS_SSO_PCRE              = 3758,
    ACE_GET_RADIUS_SSO_PCRE_RESP         = 3759,
    ACE_SET_RADIUS_SSO_PCRE              = 3760,
    ACE_SET_RADIUS_SSO_PCRE_RESP         = 3761,
    ACE_GET_AWIFI_SSO              = 3762,
    ACE_GET_AWIFI_SSO_RESP         = 3763,
    ACE_SET_AWIFI_SSO              = 3764,
    ACE_SET_AWIFI_SSO_RESP         = 3765,


    ACE_GET_HANMING_SSO              = 3766,
    ACE_GET_HANMING_SSO_RESP         = 3767,
    ACE_SET_HANMING_SSO              = 3768,
    ACE_SET_HANMING_SSO_RESP         = 3769,

    ACE_GET_JUTAI_SSO              = 3770,
    ACE_GET_JUTAI_SSO_RESP         = 3771,
    ACE_SET_JUTAI_SSO              = 3772,
    ACE_SET_JUTAI_SSO_RESP         = 3773,

    ACE_SET_NETWORK_QUARANTINE_ENABLE = 3774,
    ACE_SET_NETWORK_QUARANTINE_ENABLE_RESP = 3775,

    HYLAB_SEDUO_IP_SET = 3776,
    HYLAB_SEDUO_IP_SET_RESP = 3777,
    HYLAB_SEDUO_IP_GET = 3778,
    HYLAB_SEDUO_IP_GET_RESP = 3779,

    HYLAB_MULTIPLE_AUTH_LOGIN = 3780,
    HYLAB_MULTIPLE_AUTH_LOGIN_RESP = 3781,

	HYLAB_GET_USED_OAUTH_SERVERLIST = 3782,
	HYLAB_GET_USED_CAS_SERVERLIST = 3783,

    HYLAB_MANAGE_LOCAL_USER_SET = 3784,
    HYLAB_MANAGE_LOCAL_USER_DEL = 3785,
    HYLAB_MANAGE_LOCAL_USER_ADD = 3786,
	HYTF_MAC_AUTH_FREE_CLEAR         = 3789,
	HYTF_MAC_AUTH_FREE_ADD           = 3790,
	HYTF_MAC_AUTH_FREE_COMPLETE      = 3791,
	
	HYTF_SYSLOG_SSO_RULE_SET         = 3787,
    HYTF_SYSLOG_SSO_RULE_GET         = 3788,

	ACE_SET_MANUAL_USER_ANTI_SHARING       = 3795,
	ACE_SET_MANUAL_USER_ANTI_SHARING_RESP       = 3796,

    HYTF_5GAAA_USER_SSO_LOGIN_NOTIFY     = 3800,
	HYTF_5GAAA_USER_SSO_LOGIN_RESP       = 3801,
	HYTF_5GAAA_USER_SSO_LOGOUT_NOTIFY    = 3802,
	HYTF_5GAAA_USER_SSO_LOGOUT_RESP      = 3803,

	HYLAB_HTTP_META_REFRESH_SET = 3804,
	HYLAB_HTTP_META_REFRESH_SET_RESP = 3805,
	HYLAB_HTTP_META_REFRESH_GET = 3806,
	HYLAB_HTTP_META_REFRESH_GET_RESP = 3807,

	ACE_SET_NCE_SSO_PORT = 3808,

	HYTF_SMP_2009_SSO_TIMEOUT_SET = 3797,
	HYTF_SMP_2009_SSO_TIMEOUT_GET = 3798,

    ACE_CONFIG_SAVE_REQ             = 4000,
    ACE_CONFIG_SAVE_RESP            = 4001,
    ACE_TEMPLATE_LOAD_REQ             = 4002,
    ACE_TEMPLATE_LOAD_RESP            = 4003,
    ACE_TEMPLATE_SAVE_REQ             = 4004,
    ACE_TEMPLATE_SAVE_RESP            = 4005,
    ACE_CONFIG_HA_SAVE_REQ            = 4006,
    ACE_CONFIG_HA_SAVE_RESP           = 4007,
    ACE_CONFIG_HA_RESTORE_REQ         = 4008,
    ACE_CONFIG_HA_RESTORE_RESP        = 4009,
    ACE_HA_SWITCH_REQ                 = 4010,
    ACE_HA_SWITCH_RESP                = 4011,
    ACE_HA_CONFIG_FILES_CHECK_REQ     = 4012,
    ACE_HA_CONFIG_FILES_CHECK_RESP    = 4013,

    ACE_MANAGE_FRAME_MSG_ID                     =4500,
    ACE_MANAGE_FRAME_ADD                        =4500,
    ACE_MANAGE_FRAME_ADD_RET                    =4501,
    ACE_MANAGE_FRAME_DEL                        =4502,
    ACE_MANAGE_FRAME_DEL_RET                    =4503,
    ACE_MANAGE_FRAME_MOVE                       =4504,
    ACE_MANAGE_FRAME_MOVE_RET                   =4505,
    ACE_MANAGE_FRAME_MODIFY                 =4506,
    ACE_MANAGE_FRAME_MODIFY_RET             =4507,
    ACE_MANAGE_FRAME_GET                        =4508,
    ACE_MANAGE_FRAME_GET_RET                    =4509,
    ACE_MANAGE_FRAME_COVER                      =4510,
    ACE_MANAGE_FRAME_COVER_RET                  =4511,
    ACE_MANAGE_FRAME_SEARCH                     =4512,
    ACE_MANAGE_FRAME_SEARCH_RET                 =4513,
    ACE_MANAGE_FRAME_EXPORT                     =4514,
    ACE_MANAGE_FRAME_EXPORT_RET                 =4515,
    ACE_MANAGE_FRAME_IMPORT                     =4516,
    ACE_MANAGE_FRAME_IMPORT_RET                 =4517,
    ACE_MANAGE_FRAME_LDAP_IMPORT                =4518,
    ACE_MANAGE_FRAME_LDAP_IMPORT_RET            =4519,
    ACE_MANAGE_FRAME_GET_ALL_SIMPLE     =4520,
    ACE_MANAGE_FRAME_GET_ALL_SIMPLE_RET =4521,
    ACE_MANAGE_FRAME_GET_ALL_GROUP_SIMPLE       =4522,
    ACE_MANAGE_FRAME_GET_ALL_GROUP_SIMPLE_RET   =4523,
    ACE_MANAGE_FRAME_GET_TREE_GROUP_SIMPLE      =4524,
    ACE_MANAGE_FRAME_GET_TREE_GROUP_SIMPLE_RET  =4525,
    ACE_MANAGE_FRAME_GET_CHILD_GROUP_SIZE       =4526,
    ACE_MANAGE_FRAME_GET_CHILD_GROUP_SIZE_RET   =4527,
    ACE_MANAGE_FRAME_GET_CHILD_USER_SIZE        =4528,
    ACE_MANAGE_FRAME_GET_CHILD_USER_SIZE_RET    =4529,
    ACE_MANAGE_FRAME_GET_LDAP_IMPORT_CFG        =4530,
    ACE_MANAGE_FRAME_GET_LDAP_IMPORT_CFG_RET    =4531,
    ACE_MANAGE_FRAME_GET_GROUP_DIRECT_MEMBER    =4532,
    ACE_MANAGE_FRAME_GET_GROUP_DIRECT_MEMBER_RET=4533,
    ACE_MANAGE_FRAME_SEARCH_IN_GROUP            = 4534,
    ACE_MANAGE_FRAME_SEARCH_IN_GROUP_RET        = 4535,
    ACE_MANAGE_FRAME_MODIFY_BINDTEXT            = 4536,
    ACE_MANAGE_FRAME_MODIFY_BINDTEXT_RET        = 4537,
    ACE_MANAGE_FRAME_BINDTEXT_ALL               = 4538,
    ACE_MANAGE_FRAME_BINDTEXT_ALL_RET           = 4539,
    ACE_MANAGE_FRAME_RESET                  = 4540,
    ACE_MANAGE_FRAME_RESET_RET              = 4541,
    ACE_MANAGE_FRAME_GET_INVALID_OBJECT_ID  =4542,
    ACE_MANAGE_FRAME_GET_INVALID_OBJECT_ID_RET  =4543,
    ACE_MANAGE_FRAME_GET_TODAY                      =4544,
    ACE_MANAGE_FRAME_GET_RET_TODAY              =4545,
    ACE_MANAGE_FRAME_SEARCH_USER                        =4546,
    ACE_MANAGE_FRAME_SEARCH_USER_RET                    =4547,
    ACE_MANAGE_FRAME_GET_ALL_SIMPLE_FOR_COLLECTOR               =4548,
    ACE_MANAGE_FRAME_GET_ALL_SIMPLE_FOR_COLLECTOR_RET           =4549,
    ACE_MANAGE_FRAME_GET_ALL_GROUP_SIMPLE_FOR_COLLECTOR     =4550,
    ACE_MANAGE_FRAME_GET_ALL_GROUP_SIMPLE_FOR_COLLECTOR_RET =4551,
    ACE_MANAGE_FRAME_GET_TREE_GROUP_SIMPLE_FOR_COLLECTOR        =4552,
    ACE_MANAGE_FRAME_GET_TREE_GROUP_SIMPLE_FOR_COLLECTOR_RET    =4553,
    ACE_MANAGE_FRAME_CLEAR                      =4554,
    ACE_MANAGE_FRAME_CLEAR_RET                  =4555,
    ACE_MANAGE_FRAME_SECURITY_GROUP_USER        =4556,
    ACE_MANAGE_FRAME_SECURITY_GROUP_USER_RET    =4557,
    ACE_MANAGE_FRAME_GET_ALL_GROUP_SIMPLE_FILTER    =4558,
    ACE_MANAGE_FRAME_GET_ALL_GROUP_SIMPLE_FILTER_RET    =4559,
    ACE_MANAGE_FRAME_LDAP_DO_IMPORT             =4560,
    ACE_MANAGE_FRAME_LDAP_DO_IMPORT_RET         =4561,

    ACE_MANAGE_FRAME_GROUP_GET_PATH_BY_NAME     =4562,
    ACE_MANAGE_FRAME_GROUP_GET_PATH_BY_NAME_RET =4563,

    ACE_MANAGE_FRAME_ADD_INTERFACE2             =4564,
    ACE_MANAGE_FRAME_ADD_INTERFACE2_RET         =4565,
    
    ACE_SSLVPN_MATERIAL_SET                     = 4566,
    ACE_SSLVPN_MATERIAL_SET_RET                 = 4567,
    ACE_SSLVPN_MATERIAL_GET                     = 4568,
    ACE_SSLVPN_MATERIAL_GET_RET                 = 4569,
    ACE_SSLVPN_MATERIAL_SIZE                    = 4570,
    ACE_SSLVPN_MATERIAL_SIZE_RET                = 4571,
    ACE_SSLVPN_MATERIAL_DEL                     = 4572,
    ACE_SSLVPN_MATERIAL_DEL_RET                 = 4573,
    ACE_SSLVPN_MATERIAL_CLEAR                   = 4574,
    ACE_SSLVPN_MATERIAL_CLEAR_RET               = 4575,
    ACE_SSLVPN_MATERIAL_GET_BY_INDEX            = 4576,
    ACE_SSLVPN_MATERIAL_GET_BY_INDEX_RET        = 4577,
	
    ACE_SSLVPN_MATERIAL_GROUP_SET              = 4580,
    ACE_SSLVPN_MATERIAL_GROUP_SET_RET          = 4581,
    ACE_SSLVPN_MATERIAL_GROUP_GET              = 4582,
    ACE_SSLVPN_MATERIAL_GROUP_GET_RET          = 4583,
    ACE_SSLVPN_MATERIAL_GROUP_SIZE             = 4584,
    ACE_SSLVPN_MATERIAL_GROUP_SIZE_RET         = 4585,
    ACE_SSLVPN_MATERIAL_GROUP_DEL              = 4586,
    ACE_SSLVPN_MATERIAL_GROUP_DEL_RET          = 4587,
    ACE_SSLVPN_MATERIAL_GROUP_CLEAR            = 4588,
    ACE_SSLVPN_MATERIAL_GROUP_CLEAR_RET        = 4589,
    ACE_SSLVPN_MATERIAL_GROUP_GET_BY_INDEX     = 4590,
    ACE_SSLVPN_MATERIAL_GROUP_GET_BY_INDEX_RET = 4591,

    ACE_AUTH_SUCCESS_ACTION                    = 4596,
    ACE_AUTH_SUCCESS_ACTION_RET                = 4597,
    
    ACE_AUTH_METHOD_GET                        = 4598,
    ACE_AUTH_METHOD_GET_RET                    = 4599,
    ACE_AUTH_MASK_GET                          = 4600,
    ACE_AUTH_MASK_GET_RET                      = 4601,

    ACE_AUTH_METHOD_GET2                        = 4602,
    ACE_AUTH_METHOD_GET2_RET                    = 4603,

	ACE_MANAGE_FRAME_GET_USERNAME_BY_EMAIL_OR_PHONE = 4604,

	ACE_MANAGE_FRAME_IMPORT_FROM_OUTER = 4605,
	
    ACE_USE_SPACE_FDEBUG            = 5000,
    ACE_USE_SPACE_FDEBUG_RESP       = 5001,
    ACE_USE_SPACE_VDEBUG            = 5002,
    ACE_USE_SPACE_VDEBUG_RESP       = 5003,

    HYLAB_EDR_SET_CLIENT = 5010,
    HYLAB_EDR_SET_CLIENT_RET = 5011,
    HYLAB_EDR_CLEAR_CLIENT = 5012,
    HYLAB_EDR_CLEAR_CLIENT_RET = 5013,
    HYLAB_EDR_SET_CONFIG = 5014,
    HYLAB_EDR_SET_CONFIG_RET = 5015,
    HYLAB_EDR_GET_CONFIG = 5016,
    HYLAB_EDR_GET_CONFIG_RET = 5017,

    HYLAB_IPSCAN_SET_CONFIG = 5018,
    HYLAB_IPSCAN_GET_CONFIG = 5019,

    HYLAB_ZTGW_SET_CONFIG = 5020,
    HYLAB_ZTGW_GET_CONFIG = 5021,

    HYLAB_IPSCAN_SET_SCAN_NOW = 5022,

    ACE_USR_URLLIB_SPACE_MSG_ID     = 6210,     /* 与这个宏 ACE_URLLIB_SPACE_MSG_ID 相等 */
    ACE_USR_DANS_CMD_FETCHDATA      = 6214,
    ACE_USR_DANS_CMD_DEBUG          = 6215,
    ACE_USR_URLLIB_SPACE_MSGTYPE_ID = 6310,

    HYTF_BYZORO_WEIXIN_AUTH_NOTIFY        = 6350,
    HYTF_BYZORO_WEIXIN_AUTH_NOTIFY_RESP   = 6351,
    
    ISPTRACK_OBJECT_ADD = 6501,
    ISPTRACK_OBJECT_ADD_RET = 6502,
    ISPTRACK_OBJECT_DEL = 6503,
    ISPTRACK_OBJECT_DEL_RET = 6504,
    ISPTRACK_OBJECT_MODIFY = 6505,
    ISPTRACK_OBJECT_MODIFY_RET = 6506,
    ISPTRACK_OBJECT_CLEAR = 6507,
    ISPTRACK_OBJECT_CLEAR_RET = 6508,
    ISPTRACK_OBJECT_SIZE = 6509,
    ISPTRACK_OBJECT_SIZE_RET = 6510,
    ISPTRACK_OBJECT_GET = 6511,
    ISPTRACK_OBJECT_GET_RET = 6512,
    ISPTRACK_OBJECT_GET_BYINDEX = 6513,
    ISPTRACK_OBJECT_GET_BYINDEX_RET = 6514,
    ISPTRACK_OBJECT_RESET_COUNTERS = 6515,
    ISPTRACK_OBJECT_RESET_COUNTERS_RET = 6516,
    ISPTRACK_OBJECT_DHCP_CHANGE_GW = 6517,
    ISPTRACK_OBJECT_DHCP_CHANGE_GW_RET = 6518,

    IPV6_ISPTRACK_OBJECT_ADD = 6521,
    IPV6_ISPTRACK_OBJECT_ADD_RET = 6522,
    IPV6_ISPTRACK_OBJECT_DEL = 6523,
    IPV6_ISPTRACK_OBJECT_DEL_RET = 6524,
    IPV6_ISPTRACK_OBJECT_MODIFY = 6525,
    IPV6_ISPTRACK_OBJECT_MODIFY_RET = 6526,
    IPV6_ISPTRACK_OBJECT_CLEAR = 6527,
    IPV6_ISPTRACK_OBJECT_CLEAR_RET = 6528,
    IPV6_ISPTRACK_OBJECT_SIZE = 6529,
    IPV6_ISPTRACK_OBJECT_SIZE_RET = 6530,
    IPV6_ISPTRACK_OBJECT_GET = 6531,
    IPV6_ISPTRACK_OBJECT_GET_RET = 6532,
    IPV6_ISPTRACK_OBJECT_GET_BYINDEX = 6533,
    IPV6_ISPTRACK_OBJECT_GET_BYINDEX_RET = 6534,
    IPV6_ISPTRACK_OBJECT_RESET_COUNTERS = 6535,
    IPV6_ISPTRACK_OBJECT_RESET_COUNTERS_RET = 6536,

    REP_WEB_BASE_MSG_ID                = 7000,
    REP_WEB_SET_AUDIT_RULE             = 7001,
    REP_WEB_SET_AUDIT_RULE_RESP        = 7002,
    REP_WEB_MODIFY_AUDIT_RULE          = 7003,
    REP_WEB_MODIFY_AUDIT_RULE_RESP     = 7004,
    REP_WEB_MOVE_AUDIT_RULE            = 7005,
    REP_WEB_MOVE_AUDIT_RULE_RESP       = 7006,
    REP_WEB_DEL_AUDIT_RULE             = 7007,
    REP_WEB_DEL_AUDIT_RULE_RESP        = 7008,
    REP_WEB_CLEAR_AUDIT_RULE           = 7009,
    REP_WEB_CLEAR_AUDIT_RULE_RESP      = 7010,
    REP_WEB_GET_AUDIT_RULE_SIZE        = 7011,
    REP_WEB_GET_AUDIT_RULE_SIZE_RESP   = 7012,
    REP_WEB_GET_AUDIT_RULE_NEXT        = 7013,
    REP_WEB_GET_AUDIT_RULE_NEXT_RESP   = 7014,
    REP_WEB_SET_AUDIT_RULE_ENABLE       = 7015,
    REP_WEB_SET_AUDIT_RULE_ENABLE_RESP  = 7016,
    REP_WEB_GET_AUDIT_RULE_ENABLE       = 7017,
    REP_WEB_GET_AUDIT_RULE_ENABLE_RESP  = 7018,
    REP_WEB_SET_SYSLOG_SERVER       = 7019,
    REP_WEB_SET_SYSLOG_SERVER_RESP  = 7020,
    REP_WEB_GET_SYSLOG_SERVER       = 7021,
    REP_WEB_GET_SYSLOG_SERVER_RESP      = 7022,

    REP_WEB_SET_OUTER       = 7101,
    REP_WEB_SET_OUTER_RESP  = 7102,
    REP_WEB_GET_OUTER       = 7103,
    REP_WEB_GET_OUTER_RESP  = 7104,
    
    ACE_PROXY_SERVER_CONFIG     = 7200,
    PROXY_SERVER_IP_SET = 7201,
    PROXY_SERVER_IP_RET = 7202,
    PROXY_SERVER_IP_GET = 7203,
    PROXY_SERVER_IP_GET_RET = 7204,

    HYTF_APP_GATEWAY_CONFIG     = 7300,
    APP_GATEWAY_SWITCH_SET      = 7301,
    APP_GATEWAY_SWITCH_SET_RET  = 7302,
    APP_GATEWAY_SWITCH_GET      = 7303,
    APP_GATEWAY_SWITCH_GET_RET  = 7304,

    HYTF_SNMPTRAP_CONFIG     = 7305,
    SNMPTRAP_SET      = 7306,
    SNMPTRAP_SET_RET  = 7307,
    SNMPTRAP_GET      = 7308,
    SNMPTRAP_GET_RET  = 7309,


    HYTF_DDNS_CONFIG        = 7310,
    DDNS_SWITCH_SET         = 7311,
    DDNS_SWITCH_SET_RET     = 7312,
    DDNS_SWITCH_GET         = 7313,
    DDNS_SWITCH_GET_RET     = 7314,

    HYTF_GET_LDAP_IMPORT_SIZE        = 7320,
    HYTF_GET_LDAP_IMPORT_SIZE_RESP   = 7321,
    HYTF_GET_LDAP_IMPORT_QUERY       = 7322,
    HYTF_GET_LDAP_IMPORT_QUERY_RESP  = 7323,
    HYTF_DEL_LDAP_IMPORT_CFG         = 7324,
    HYTF_DEL_LDAP_IMPORT_CFG_RESP    = 7325,

    HYTF_SSL_PROXY_CONFIG       = 7350,
    HYTF_SSL_PROXY_CFG_SET      = 7351,
    HYTF_SSL_PROXY_CFG_SET_RET  = 7352,
    HYTF_SSL_PROXY_CFG_GET      = 7353,
    HYTF_SSL_PROXY_CFG_GET_RET  = 7354,

    HYTF_BALANCE_INTERFACE_ADD  = 7380,
    HYTF_BALANCE_INTERFACE_ADD_RET = 7381,
    HYTF_BALANCE_INTERFACE_DELETE = 7382,
    HYTF_BALANCE_INTERFACE_DELETE_RET = 7383,
    HYTF_BALANCE_INTERFACE_MODIFY = 7384,
    HYTF_BALANCE_INTERFACE_MODIFY_RET = 7385,
    HYTF_BALANCE_INTERFACE_CLEAR = 7386,
    HYTF_BALANCE_INTERFACE_CLEAR_RET = 7387,
    HYTF_BALANCE_INTERFACE_SIZE = 7388,
    HYTF_BALANCE_INTERFACE_SIZE_RET = 7389,
    HYTF_BALANCE_INTERFACE_GET_BY_IFNAME = 7390,
    HYTF_BALANCE_INTERFACE_GET_BY_IFNAME_RET = 7391,
    HYTF_BALANCE_INTERFACE_GET_BY_INDEX = 7392,
    HYTF_BALANCE_INTERFACE_GET_BY_INDEX_RET = 7393,
    HYTF_BALANCE_INTERFACE_RESET_COUNTERS = 7394,
    HYTF_BALANCE_INTERFACE_RESET_COUNTERS_RET = 7395,

    HYTF_BALANCE_DNS_STATUS_SET = 7400,
    HYTF_BALANCE_DNS_STATUS_SET_RET = 7401,
    HYTF_BALANCE_DNS_STATUS_GET = 7402,
    HYTF_BALANCE_DNS_STATUS_GET_RET = 7403,

    HYTF_BALANCE_POLICY_ADD  = 7410,
    HYTF_BALANCE_POLICY_ADD_RET = 7411,
    HYTF_BALANCE_POLICY_DELETE = 7412,
    HYTF_BALANCE_POLICY_DELETE_RET = 7413,
    HYTF_BALANCE_POLICY_MODIFY = 7414,
    HYTF_BALANCE_POLICY_MODIFY_RET = 7415,
    HYTF_BALANCE_POLICY_CLEAR = 7416,
    HYTF_BALANCE_POLICY_CLEAR_RET = 7417,
    HYTF_BALANCE_POLICY_SIZE = 7418,
    HYTF_BALANCE_POLICY_SIZE_RET = 7419,
    HYTF_BALANCE_POLICY_GET_BY_NAME = 7420,
    HYTF_BALANCE_POLICY_GET_BY_NAME_RET = 7421,
    HYTF_BALANCE_POLICY_GET_BY_INDEX = 7422,
    HYTF_BALANCE_POLICY_GET_BY_INDEX_RET = 7423,
    HYTF_BALANCE_POLICY_RESET_COUNTERS = 7424,
    HYTF_BALANCE_POLICY_RESET_COUNTERS_RET = 7425,
#if 1
    HYTF_BALANCE_DOMAIN_ADD  = 7440,
    HYTF_BALANCE_DOMAIN_ADD_RET = 7441,
    HYTF_BALANCE_DOMAIN_DELETE = 7442,
    HYTF_BALANCE_DOMAIN_DELETE_RET = 7443,
    HYTF_BALANCE_DOMAIN_MODIFY = 7444,
    HYTF_BALANCE_DOMAIN_MODIFY_RET = 7445,
    HYTF_BALANCE_DOMAIN_CLEAR = 7446,
    HYTF_BALANCE_DOMAIN_CLEAR_RET = 7447,
    HYTF_BALANCE_DOMAIN_SIZE = 7448,
    HYTF_BALANCE_DOMAIN_SIZE_RET = 7449,
    HYTF_BALANCE_DOMAIN_GET_BY_NAME = 7450,
    HYTF_BALANCE_DOMAIN_GET_BY_NAME_RET = 7451,
    HYTF_BALANCE_DOMAIN_GET_BY_INDEX = 7452,
    HYTF_BALANCE_DOMAIN_GET_BY_INDEX_RET = 7453,
    HYTF_BALANCE_DOMAIN_ABBR_GET_BY_INDEX = 7454,
    HYTF_BALANCE_DOMAIN_ABBR_GET_BY_INDEX_RET = 7455,
    HYTF_BALANCE_DOMAIN_INSERT = 7456,
    HYTF_BALANCE_DOMAIN_INSERT_RET = 7457,
    HYTF_BALANCE_DOMAIN_MOVE = 7458,
    HYTF_BALANCE_DOMAIN_MOVE_RET = 7459,
    HYTF_BALANCE_DOMAIN_CHANGE_STATUS = 7460,
    HYTF_BALANCE_DOMAIN_CHANGE_STATUS_RET = 7461,
    HYTF_BALANCE_DOMAIN_RESET_COUNTERS = 7462,
    HYTF_BALANCE_DOMAIN_RESET_COUNTERS_RET = 7463,
#endif

    HYTF_CHNGRP_BONDING_ADD = 7500,
    HYTF_CHNGRP_BONDING_ADD_RET = 7501,
    HYTF_CHNGRP_BONDING_DELETE = 7502,
    HYTF_CHNGRP_BONDING_DELETE_RET = 7503,

    /*access rule*/
    ACCESSRULE_OBJECT_SIZE = 7550,
    ACCESSRULE_OBJECT_SIZE_RET = 7551,
    ACCESSRULE_OBJECT_GET_BYINDEX = 7552,
    ACCESSRULE_OBJECT_GET_BYINDEX_RET = 7553,
    ACCESSRULE_OBJECT_GET_BYNAME = 7554,
    ACCESSRULE_OBJECT_GET_BYNAME_RET = 7555,
    ACCESSRULE_OBJECT_ADD = 7556,
    ACCESSRULE_OBJECT_ADD_RET = 7557,
    ACCESSRULE_OBJECT_MODIFY = 7558,
    ACCESSRULE_OBJECT_MODIFY_RET = 7559,
    ACCESSRULE_OBJECT_DEL  = 7560,
    ACCESSRULE_OBJECT_DEL_RET  = 7561,  
    ACCESSRULE_OBJECT_CLEAR  = 7562,    
    ACCESSRULE_OBJECT_CLEAR_RET  = 7563,    

    //auth page close ,logout maoan add 11.13
    ACE_SET_AUTH_PAGE_CLOSE_LOGOUT = 7564,
    ACE_SET_AUTH_PAGE_CLOSE_LOGOUT_RESP       = 7565,
    ACE_GET_AUTH_PAGE_CLOSE_LOGOUT = 7566,
    ACE_GET_AUTH_PAGE_CLOSE_LOGOUT_RESP       = 7567,

    ACE_SET_AUTH_LOCAL_USER_PASSWD_AGEOUT_TIME            = 7568,
    ACE_SET_AUTH_LOCAL_USER_PASSWD_AGEOUT_TIME_RESP       = 7569,
    ACE_GET_AUTH_LOCAL_USER_PASSWD_AGEOUT_TIME            = 7570,
    ACE_GET_AUTH_LOCAL_USER_PASSWD_AGEOUT_TIME_RESP       = 7571,

    ACE_SET_AUTH_LOCAL_USER_PASSWD_POLICY            = 7572,
    ACE_SET_AUTH_LOCAL_USER_PASSWD_POLICY_RESP       = 7573,
    ACE_GET_AUTH_LOCAL_USER_PASSWD_POLICY            = 7574,
    ACE_GET_AUTH_LOCAL_USER_PASSWD_POLICY_RESP       = 7575,


    ACE_SET_AUTH_CITYHOTSPOT_GROUP_SELECT = 7576,
    ACE_SET_AUTH_CITYHOTSPOT_GROUP_SELECT_RESP = 7577,
    ACE_GET_AUTH_CITYHOTSPOT_GROUP_SELECT = 7578,
    ACE_GET_AUTH_CITYHOTSPOT_GROUP_SELECT_RESP = 7579,
    
    HYTF_DBSSO_TEST         = 7658, 
    HYTF_DBSSO_TEST_RESP    = 7659,
    HYTF_DBSSO_SET         = 7660,
    HYTF_DBSSO_SET_RESP         = 7661,
    HYTF_DBSSO_GET         = 7662,
    HYTF_DBSSO_GET_RESP         = 7663,
#if 0
    HYTF_CONF_SYNC_SET         = 7664,
	HYTF_CONF_SYNC_SET_RESP         = 7665,
	HYTF_CONF_SYNC_GET         = 7666,
	HYTF_CONF_SYNC_GET_RESP         = 7667,
#endif
    HYTF_PWDPOLICY_SET         = 7670,
    HYTF_PWDPOLICY_SET_RESP         = 7671,
    HYTF_PWDPOLICY_GET         = 7672,
    HYTF_PWDPOLICY_GET_RESP         = 7673,

    HYTF_BRIDGE_STATUS_SET         = 7674,
	HYTF_BRIDGE_STATUS_SET_RESP         = 7675,
	HYTF_BRIDGE_STATUS_GET         = 7676,
	HYTF_BRIDGE_STATUS_GET_RESP         = 7677,

	//BFD
	BFDTRACK_OBJECT_ADD = 7700,
    BFDTRACK_OBJECT_ADD_RET,
    BFDTRACK_OBJECT_DEL,
    BFDTRACK_OBJECT_DEL_RET,
    BFDTRACK_OBJECT_MODIFY,
    BFDTRACK_OBJECT_MODIFY_RET,
    BFDTRACK_OBJECT_CLEAR,
    BFDTRACK_OBJECT_CLEAR_RET,
    BFDTRACK_OBJECT_SIZE,
    BFDTRACK_OBJECT_SIZE_RET,
    BFDTRACK_OBJECT_GET,
    BFDTRACK_OBJECT_GET_RET,
    BFDTRACK_OBJECT_GET_BYINDEX,
    BFDTRACK_OBJECT_GET_BYINDEX_RET,

    //static route
    STATIC_ROUTE_MSG = 7800,
    STATIC_ROUTE_ADD,
    STATIC_ROUTE_ADD_RET,
    STATIC_ROUTE_DEL,
    STATIC_ROUTE_DEL_RET,
    STATIC_ROUTE_MOD,
    STATIC_ROUTE_MOD_RET,
    STATIC_ROUTE_GET_SIZE,
    STATIC_ROUTE_GET_SIZE_RET,
    STATIC_ROUTE_GET,
    STATIC_ROUTE_GET_RET,
    STATIC_ROUTE_GET_BY_GW_NET,
    STATIC_ROUTE_GET_BY_GW_NET_RET,
    STATIC_ROUTE_MSG_MAX,

	//ips_rule
	IPS_RULE_APP_MSG=7900,
	IPS_RULE_APP_ADD,
	IPS_RULE_APP_ADD_RET,
	IPS_RULE_APP_DEL,
	IPS_RULE_APP_DEL_RET,
	IPS_RULE_APP_MODIFY,//7905
	IPS_RULE_APP_MODIFY_RET,
	IPS_RULE_APP_RESET_COUNT,
	IPS_RULE_APP_RESET_COUNT_RET,
	IPS_RULE_APP_MOVE_UP,
	IPS_RULE_APP_MOVE_UP_RET,//7910
	IPS_RULE_APP_MOVE_DOWN,
	IPS_RULE_APP_MOVE_DOWN_RET,
	IPS_RULE_APP_MOVE_TO,
	IPS_RULE_APP_MOVE_TO_RET,
	IPS_RULE_APP_GET_SIZE,//7915
	IPS_RULE_APP_GET_SIZE_RET,
	IPS_RULE_APP_GET_BY_INDEX,
	IPS_RULE_APP_GET_BY_INDEX_RET,
	IPS_RULE_APP_SET_STATUS,
	IPS_RULE_APP_SET_STATUS_RET,//7920
	IPS_RULE_APP_GET_BY_NAME,
	IPS_RULE_APP_GET_BY_NAME_RET,
	IPS_RULE_APP_SYNC,
	IPS_RULE_APP_SYNC_RET,
	

    HYTF_L7FILTER_MSG_ID = 8000,

	//external conn
	EXTERNAL_CONN_RULE_MSG = 8300,
	EXTERNAL_CONN_RULE_ADD,
	EXTERNAL_CONN_RULE_DEL,
	EXTERNAL_CONN_RULE_GET,
	EXTERNAL_CONN_RULE_AUTO_DEL,
	EXTERNAL_CONN_RULE_MAX,

	ILLEGAL_OUTREACH2_MSG = 8430,
	ILLEGAL_OUTREACH2_SERVER,
	ILLEGAL_OUTREACH2_SERVER_SET,
	ILLEGAL_OUTREACH2_SERVER_SET_RET,
	ILLEGAL_OUTREACH2_SERVER_GET,
	ILLEGAL_OUTREACH2_SERVER_GET_RET,
	ILLEGAL_OUTREACH2_TASK,
	ILLEGAL_OUTREACH2_TASK_ADD,
	ILLEGAL_OUTREACH2_TASK_ADD_RET,
	ILLEGAL_OUTREACH2_TASK_DEL,
	ILLEGAL_OUTREACH2_TASK_DEL_RET,
	ILLEGAL_OUTREACH2_TASK_GET_BY_INDEX,
	ILLEGAL_OUTREACH2_TASK_GET_BY_INDEX_RET,
	ILLEGAL_OUTREACH2_TASK_SIZE,
	ILLEGAL_OUTREACH2_TASK_SIZE_RET,
	ILLEGAL_OUTREACH2_TASK_RUN,
	ILLEGAL_OUTREACH2_TASK_RUN_RET,
	ILLEGAL_OUTREACH2_TASK_DEL_ALL,
	ILLEGAL_OUTREACH2_TASK_DEL_ALL_RET,
	ACE_ILL_OUTREACH_JS_SET,
	ACE_ILL_OUTREACH_JS_SET_RET,
	ACE_ILL_OUTREACH_JS_GET,
	ACE_ILL_OUTREACH_JS_GET_RET,
	HYTF_KERNEL_JS_FILE_NOTIFY,


    HYTF_COLLECTOR_MSG_ID = 9000,

    HYTF_SHAREIP_NOTIFY_CFG_GET = 10000,
    HYTF_SHAREIP_NOTIFY_CFG_GET_RET = 10001,
    HYTF_SHAREIP_NOTIFY_CFG_SET = 10002,
    HYTF_SHAREIP_NOTIFY_CFG_SET_RET = 10003,

    ACE_DOT1X_GET_USER_INFO = 11000,
    ACE_DOT1X_GET_USER_INFO_RET = 11001,
    ACE_DOT1X_AUTH_SUCC = 11002,
    ACE_DOT1X_AUTH_SUCC_RET = 11003,
    ACE_DOT1X_LOGOUT = 11004,
    ACE_DOT1X_LOGOUT_RET = 11005,

	ACE_SET_802DOT1X = 11100,
	ACE_SET_802DOT1X_RESP = 11101,
	ACE_GET_802DOT1X = 11102,
	ACE_GET_802DOT1X_RESP = 11103,

	ACE_ADD_VLANID = 11200,
	ACE_ADD_VLANID_RESP = 11201,
	ACE_DELTE_VLANID = 11202,
	ACE_DELTE_VLANID_RESP = 11203,
	ACE_MODIFY_VLANID = 11204,
	ACE_MODIFY_VLANID_RESP = 11205,
	ACE_CLEAR_VLANID = 11206,
	ACE_CLEAR_VLANID_RESP = 11207,
	ACE_GET_VLANID_SIZE = 11208,
	ACE_GET_VLANID_SIZE_RESP = 11209,
	ACE_GET_VLANID_NEXT = 11210,
	ACE_GET_VLANID_NEXT_RESP = 11211,

	ACE_GET_IP_MAC_TABLE_SIZE = 11300,
	ACE_GET_IP_MAC_TABLE_SIZE_RET = 11301,
	ACE_GET_IP_MAC_TABLE_NEXT = 11302,
	ACE_GET_IP_MAC_TABLE_NEXT_RET = 11303,

    RISK_ANALYSIS_CONFIG = 11400,
	RISK_ANALYSIS_SET = 11401,
	RISK_ANALYSIS_SET_RET = 11402,
	RISK_ANALYSIS_GET = 11403,
	RISK_ANALYSIS_GET_RET = 11404,

    FW_SECURITY_PROTECT_POLICY_USERSPACE_COUNTER_GET = 11460,
    FW_SECURITY_PROTECT_POLICY_USERSPACE_COUNTER_GET_RET = 11461,

	HYTF_ADD_WLAN_STATION_WHITELIST = 11462,

	//20250512
	HYLAB_IPSEC_DIAGNOSTIC_CONN = 11470,
	HYLAB_IPSEC_DIAGNOSTIC_CONN_RESP,

	ACE_GET_SHARE_MANAGE_ENTRY_INFO = 23000,
	ACE_GET_SHARE_MANAGE_ENTRY_RESP,
	ACE_DOUBLE_STACK_AUTH_SYNC,


	KICKOFF_MAC_NOAUTH = 23100,
	MAC_NOAUTH_ADD = 23101,
	MAC_NOAUTH_IMPORT = 23102,


	EXTERNAL_CONN_PARSER_REQ = 30100,
	EXTERNAL_CONN_PARSER_REQ_RESP,


	HYTF_NCE_CAMPUS_SSO_LOGOUT = 30200,
	HYTF_NCE_CAMPUS_SSO_LOGIN = 30201,
};

enum HYTF_L7FILTER_IPC_ID
{
    /*1-5*/
    HYTF_L7FILTER_IPC_VDEBUG = 1,
    HYTF_L7FILTER_IPC_VDEBUG_RESP,
    HYTF_L7FILTER_IPC_FDEBUG,
    HYTF_L7FILTER_IPC_FDEBUG_RESP
};

enum HYTF_COLLECTOR_IPC_ID
{
    /*1-5*/
    HYTF_COLLECTOR_IPC_VDEBUG = 1,
    HYTF_COLLECTOR_IPC_VDEBUG_RESP,
    HYTF_COLLECTOR_IPC_FDEBUG,
    HYTF_COLLECTOR_IPC_FDEBUG_RESP,
    HYTF_COLLECTOR_IPC_IPLOCATION_GET,
    HYTF_COLLECTOR_IPC_IPLOCATION_GET_RESP,
};

enum HYTF_LICENSE_MSG_ID
{
    /*1-5*/
    ACE_LIC_MSG_ID                  = 1,
    ACE_LIC_USED_TIME_RECORD        = 2,
    ACE_LIC_UPDATE_LICENSE,
    ACE_LIC_INIT_USE_TIME,
    ACE_LIC_UPDATE_LICENSE_RESP,
    /*6-10*/
    ACE_LIC_FDEBUG,
    ACE_LIC_FDEBUG_RESP,
    ACE_LIC_VDEBUG,
    ACE_LIC_VDEBUG_RESP,
    ACE_LIC_GET_LICENSE,
    /*11-15*/
    ACE_LIC_GET_LICENSE_RESP,
    ACE_LIC_GET_STATE,
    ACE_LIC_GET_STATE_RESP,
    ACE_LIC_GET_UPDATE_FIRMWARE,
    ACE_LIC_GET_UPDATE_FIRMWARE_RESP,
    /*16-20*/
    ACE_LIC_GET_UPDATE_AISLIB,
    ACE_LIC_GET_UPDATE_AISLIB_RESP,
    ACE_LIC_GET_UPDATE_URLLIB,
    ACE_LIC_GET_UPDATE_URLLIB_RESP,
    ACE_LIC_GET_RESET_TIME,
    /*21-25*/
    ACE_LIC_GET_RESET_TIME_RESP,
    ACE_LIC_GET_FIRMWARE_RESET_TIME,
    ACE_LIC_GET_FIRMWARE_RESET_TIME_RESP,
    ACE_LIC_GET_AIS_RESET_TIME,
    ACE_LIC_GET_AIS_RESET_TIME_RESP,
    /*26-30*/
    ACE_LIC_GET_URL_RESET_TIME,
    ACE_LIC_GET_URL_RESET_TIME_RESP,
    ACE_LIC_GET_LANGUAGE,
    ACE_LIC_GET_LANGUAGE_RESP,
    HYTF_LIC_GET_MOD_EXT_EXPIRE,
    /**31-35**/
    HYTF_LIC_GET_MOD_EXT_EXPIRE_RESP,
    HYTF_LIC_GET_LICENSE_INFO,
    HYTF_LIC_GET_LICENSE_INFO_RESP,
    HYTF_LIC_GET_MOD_EXT,
    HYTF_LIC_GET_MOD_EXT_RESP,
	/**36-40**/
    HYTF_LIC_MGT1_LAN1_RENAME,
    HYTF_LIC_MGT1_LAN1_RENAME_RESP,
    HYTF_LIC_VPP_RESET_NOTIFY,
    HYTF_LIC_VPP_RESET_NOTIFY_RESP,
    HYTF_LIC_GET_EXPIRE,
    /**41-25**/
    HYTF_LIC_GET_EXPIRE_RESP,
    HYTF_LIC_GET_MYHW_LIC_STATUS,
    HYTF_LIC_GET_MYHW_LIC_STATUS_RESP,
    HYTF_LIC_ACTIVATE_LICENSE_II,
    HYTF_LIC_ACTIVATE_LICENSE_II_RESP,
#ifdef Hillstone
    HYTF_LIC_HS_LMS_DISABLE = 100,
    HYTF_LIC_HS_LMS_DISABLE_RESP,
    HYTF_LIC_HS_LMS_ENABLE,
    HYTF_LIC_HS_LMS_ENABLE_RESP,
    HYTF_LIC_HS_LMS_CANCEL,
    HYTF_LIC_HS_LMS_CANCEL_RESP,
#endif
};

enum ACE_LIC_LOG_ID
{
    /*0-4*/
    LOG_LICENSE_UPDATE_OK = 0,
    LOG_LICENSE_UPDATE_FILE_INVALID,
    LOG_LICENSE_GET_QUEUE_ERROR,
    LOG_LICENSE_DEMON_RESPONSE_TIMEOUT,
    LOG_LICENSE_READ_ERROR,

    /*5-9*/
    LOG_LICENSE_UPDATE_ERROR,   
    LOG_LICENSE_MODEL_ERROR,
    LOG_LICENSE_SN_ERROR,
    LOG_LICENSE_DATE_ERROR,
    LOG_LICENSE_DEMO_TIMEOUT_ERROR,

    /*10-14*/
    LOG_LICENSE_IMP2P_TIMEOUT_ERROR,
    LOG_LICENSE_SYS_TIMEOUT_ERROR,
    LOG_LICENSE_DATE_UPDATE_ERROR,
    LOG_LICENSE_RECHEKING_OK,
    LOG_LICENSE_FORMAT_ERROR,

    /*15-19*/
    LOG_LICENSE_EXCEED_AUTH,
    LOG_LICENSE_NO_MEMORY,
    LOG_LICENSE_INVALID_SYSTEM_TIME,
    LOG_LICENSE_INVALID_LICENSE_ISSUED_DATE,
    LOG_LICENSE_UPDATE_SAME_LICENSE_ERROR,

    /*20-24*/
    LOG_LICENSE_SAVE_LICENSE_FILE_ERROR,
    LOG_LICENSE_READ_DATE_FILE_ERROR,
    LOG_LICENSE_DATE_FILE_FORMAT_ERROR,
    LOG_LICENSE_SAVE_DATE_FILE_ERROR,
    LOG_LICENSE_REITERATE,

    /*25-29*/
    LOG_LICENSE_START_TIME_VALID,
    LOG_LICENSE_IN_BLACKLIST,
    LOG_LICENSE_NO_SYSTEM_LICENSE_ERROR,
    LOG_LICENSE_MAX_BANDWIDTH_OVERFLOW,    
    LOG_LICENSE_MAX_MODULE_PARAMETERS_OVERFLOW,
    LOG_LICENSE_MAX,
};

#define LICENSE_CTRL_BIT_SESSIONS		(0x00000001)
#define LICENSE_CTRL_BIT_USERS			(0x00000002)

/**LIC_VER_XXX definition:**/
#define LIC_VER_MAIN        2/*(<=255)*/
#define LIC_VER_SUB         0/*(<=255)*/
#define LIC_VER_EXT         0/*(<=255)*/

/**version string format:main.sub.ext;  example:1.0.0**/
#define LIC_VER_VALUE(main, sub, ext)   ((main<<16) + (sub<<8) + ext)

#define ACE_MAX_SUPPORT_PORT              ((MAX_CAR_TUNNEL_NUM-1) * 2)
#if defined(ARCH_ARM64) || defined(CONFIG_HYTF_FW)
#ifndef ETH0_MAC_INFO_FILE
#define ETH0_MAC_INFO_FILE                "/tmp/.eth0_mac_info.dat"
#endif
#endif


#ifndef ACE_PORT_INFO_FILE
#define ACE_PORT_INFO_FILE                "/home/ace_port_info.dat"
#endif

#ifndef ACE_PORT_MAP_FILE
#define ACE_PORT_MAP_FILE                 "/home/ace_port_map.dat"
#endif

#ifndef ACE_BRIDGE_MAP_FILE
#define ACE_BRIDGE_MAP_FILE               "/home/ace_bridge_map.dat"
#endif

#ifndef ACE_SYSTEM_MAP_FILE
#define ACE_SYSTEM_MAP_FILE               "/boot/System.map"
#endif

#ifndef ACE_VPP_PORT_MAP_FILE
#define ACE_VPP_PORT_MAP_FILE             "/home/linux2vpp.dat"
#endif

#ifndef ACE_DEFAULT_PORT_NUM
#define ACE_DEFAULT_PORT_NUM              4
#endif

#define PPPOE_PORT_MAX  8
#define HYTF_ARP_PROTECT_NUM    (4)

enum ACE_WORK_MODE
{
    ACE_NETWORK_WORKMODE_ROUTE = 0,
    ACE_NETWORK_WORKMODE_BRIDGE,
    ACE_NETWORK_WORKMODE_BYPASS,
};

#define ACE_NETWORK_WORKMODE_MAX          ACE_NETWORK_WORKMODE_BYPASS
#define ACE_NETWORK_WORKMODE_DEFAULT      0x101

#ifndef ACE_WORK_MODE_FILE
#define ACE_WORK_MODE_FILE            "/home/config/current/ace_work_mode.conf"
#define ACE_WORK_MODE_FILE_SHORT      "ace_work_mode.conf"
#endif

#ifndef ACE_SYSTEM_CONFIG_FILE
#define ACE_SYSTEM_CONFIG_FILE        "/home/config/current/ace_system_config.conf"
#define ACE_SYSTEM_CONFIG_FILE_SHORT  "ace_system_config.conf"
#define ACE_BACKUP_SYSTEM_CONFIG_FILE        "/usr/local/etc/ace_system_config.conf"
#endif

#ifndef HYLAB_VPP_CMD_FILE
#define HYLAB_VPP_CMD_FILE        "/home/config/current/hylab_vpp_cmd.conf"
#define HYLAB_VPP_CMD_FILE_SHORT  "hylab_vpp_cmd.conf"
#define HYLAB_BACKUP_VPP_CMD_FILE        "/usr/local/etc/hylab_vpp_cmd.conf"
#endif

#ifndef HYLAB_POST_VPP_CMD_FILE
#define HYLAB_POST_VPP_CMD_FILE        "/home/config/current/hylab_post_vpp_cmd.conf"
#define HYLAB_POST_VPP_CMD_FILE_SHORT  "hylab_post_vpp_cmd.conf"
#define HYLAB_BACKUP_POST_VPP_CMD_FILE        "/usr/local/etc/hylab_post_vpp_cmd.conf"
#endif

#ifndef ACE_PPPOE_CONFIG_FILE
#define ACE_PPPOE_CONFIG_FILE        "/home/config/current/ace_pppoe_config.conf"
#define ACE_PPPOE_CONFIG_FILE_SHORT  "ace_pppoe_config.conf"
#endif
#ifndef ACE_VLAN_CONFIG_FILE
#define ACE_VLAN_CONFIG_FILE          "/home/vlan-tmp-cfg.conf"
#define ACE_VLAN_CONFIG_FILE_SHORT    "vlan-tmp-cfg.conf"
#endif
#ifndef ACE_MACVLAN_CONFIG_FILE
#define ACE_MACVLAN_CONFIG_FILE          "/home/macvlan-tmp-cfg.conf"
#define ACE_MACVLAN_CONFIG_FILE_SHORT    "macvlan-tmp-cfg.conf"
#endif
#ifndef HYTF_CHANNEL_GROUP_CONFIG_FILE
#define HYTF_CHANNEL_GROUP_CONFIG_FILE          "/home/config/current/hytf_changrp_config.conf"
#define HYTF_CHANNEL_GROUP_CONFIG_FILE_SHORT    "hytf_changrp_config.conf"
#endif
#ifndef ACE_CONFIG_FILE
#define ACE_CONFIG_INI_FILE           "/home/config/current/ace_private_config.conf"
#define ACE_CONFIG_FILE               "/home/config/current/ace_private_config.conf"
#define ACE_CONFIG_FILE_SHORT         "ace_private_config.conf"
#define HYTF_MANAGE_FRAME_DIR   "/home/config/current/manage_frame_edit"
#define TEMP_CONFIG_FILE      "/home/config/current/ace_private_config.tmp"
#define ACE_DEFAULT_POLICY_FILE               "/home/config/current/ace_default_config.conf"
#endif

#ifndef ACE_HA_FILE
#define ACE_HA_FILE               "/home/config/current/ace_ha_config.conf"
#endif

#ifndef ACE_USER_CONFIG_FILE
#define ACE_USER_CONFIG_FILE          "/home/config/current/user.conf" /*/usr/local/lighttpd/*/
#define ACE_USER_CONFIG_FILE_SHORT    "user.conf"
#endif

#ifndef ACE_ROOT_PASSWD_FILE
#define ACE_ROOT_PASSWD_FILE          "/home/config/default/passwd"
#define ACE_ROOT_PASSWD_FILE_SHORT    "passwd"
#endif

#ifndef ACE_NET_MANAGER_FILE
#define ACE_NET_MANAGER_FILE          "/home/config/current/netmanager.conf" /*/usr/local/lighttpd/*/
#define ACE_NET_MANAGER_FILE_SHORT    "netmanager.conf"
#endif

#ifndef ACE_SSH_CONFIG_FILE
#define ACE_SSH_CONFIG_FILE          "/etc/ssh/sshd_config" /*/etc/ssh/*/
#define ACE_SSH_CONFIG_FILE_SHORT    "sshd_config"
#endif

#ifndef ACE_CHECKOUT_FILE_SHORT
#define ACE_CHECKOUT_FILE_SHORT       "Config.conf"
#endif

#define ACE_USER_FTOK_NAME            "/tmp/.ftok_user.dat"
#define HYTF_RADIUS_MACCHECKER_FTOK_NAME            "/tmp/.ftok_radius_macchecker.dat"
#define REP_WEB_FTOK_NAME             "/tmp/.ftok_rep.dat"

#ifndef ACE_FEATURE_LIB_PATH
#define ACE_FEATURE_LIB_PATH          "/usr/private/featureV/"
#define ACE_FEATURE_LIB_KEY           "L7filter5*P1W2D3"
#endif

#ifndef ACE_FEATURE_LIB_VERSION
#define ACE_FEATURE_LIB_VERSION       5
#endif


#ifndef ACE_FEATURE_LIB_VER
#define ACE_FEATURE_LIB_VER           "/usr/private/featureV-lib-ver"
#define ACE_FEATURE_LIB_VER_SHORT     "featureV-lib-ver"
#endif

#ifndef ACE_FEATURE_LIB_RULE
#define ACE_FEATURE_LIB_RULE           "/usr/private/l7-filter_rule.pat"
#define ACE_FEATURE_LIB_RULE_SHORT     "l7-filter_rule.pat"
#endif

#ifndef ACE_URL_LIB_VER
#define ACE_URL_LIB_VER               "/usr/private/url-lib-ver"
#define ACE_URL_LIB_VER_SHORT         "url-lib-ver"
#endif

#ifndef ACE_TEMPLATE_FILE
#define ACE_TEMPLATE_FILE               "/home/config/current/ace_template_config.conf"
#define ACE_TEMPLATE_FILE_SHORT         "ace_template_config.conf"
#endif

#ifndef NMC_TEMPLATE_DIR
#define NMC_TEMPLATE_DIR                "/etc/template"
#define NMC_TEMPLATE_INFO_FILE          NMC_TEMPLATE_DIR"/template_info"
#define NMC_TEMPLATE_INFO_FILE_SHORT    "template_info"
#define NMC_TEMPLATE_ID_FILE        "/tmp/.template_id_table"
#endif


#define ACE_SYS_INIT_FLAG             "/etc/init.d/ace_sys_init"
#define ACE_SYS_BOOT_MODE             "/boot/grub/ace_boot_mode"

#define ACE_LIC_FTOK_NAME             "/usr/.lic/.ace_lic_ftok.dat"
#define ACE_LIC_UPDATE_FILE           "/usr/download/.ace_fsec.dat"
#define ACE_LIC_UPDATE_FILE_SHORT     ".ace_fsec.dat"

#define ACE_IPTK_FTOK_NAME            "/home/.ace_iptk_ftok.dat"
#define IPTK_CFG_FTOK_NAME            "/home/iptk_ftok.dat"

#define HYLAB_NAC_LICENSE_V2_FILE   "/usr/mysys/.nac_license_v2"

#define SOCKET_UDP_DPORT_TO_TCP_SYSLOG     (65488)
#define SOCKET_UDP_DPORT_TO_ASSET_SCANNING (65489)

#define SOCKET_UDP_DPORT_TO_RYZ_PORTAL (65496)
#define SOCKET_UDP_DPORT_TO_STRONGSWAN_MONITOR (65497)
#define SOCKET_UDP_DPORT_TO_SAM_SSO     (65498)
#define SOCKET_UDP_DPORT_TO_TRACING_SOURCE    (65499)

#define SOCKET_UDP_DPORT_TO_L7FILTER    (65500)
// keep a range(65500-65514) for multiple l7dpi socket 
#define SOCKET_UDP_DPORT_TO_L7FILTER_END    (65514)

#define UDP_SOCKET_DPORT_DEVICE 65515
#define SOCKET_UDP_DPORT_TO_COLLECTOR_NL (65516)

#if 0
#define SOCKET_UDP_DPORT_TO_L7DEVICE	(65516)
#endif
#define SOCKET_UDP_DPORT_TO_USERSPACE3  (65518)
#define SOCKET_UDP_DPORT_TO_USERSPACE2  (65519)
#define SOCKET_UDP_DPORT_TO_LICENSED	(65524)
#define SOCKET_UDP_DPORT_TO_COLLECTOR82_SESSION_START (65525)
#define SOCKET_UDP_DPORT_TO_COLLECTOR82_SESSION_END (65527)
#define SOCKET_UDP_DPORT_TO_COLLECTOR82 (65528)
#define SOCKET_UDP_DPORT_TO_USERSPACE   (65529)
#define SOCKET_UDP_DPORT_TO_COLLECTOR   (65530)
#define SOCKET_UDP_DPORT_TO_SYSTEM_CALL (65531)
#define SOCKET_UDP_DPORT_TO_CLIENT82    (65532)
//#define SOCKET_UDP_DPORT_TO_NAT_FINDER	(65533)
#define SOCKET_UDP_DPORT_HA_SYNC    	(65533)
#define SOCKET_UDP_DPORT_TO_RAPID_SYSCALL     (65534)
#define SOCKET_UDP_DPORT_TO_JSON_LICENSED (65424)

#define SOCKET_UDP_DPORT_TO_FAKE_USER     (65520)
#define SOCKET_UDP_DPORT_TO_FAKE_KERNEL   (65521)
#define SOCKET_UDP_DPORT_L7PATTERN	(65400)

#define SURI_COMM_UDP_PORT   (65300)
#define AV_L7_UDP_PORT       (65301)
#define AV_MAIL_UDP_PORT     (65303)
#define WAF_COMM_UDP_PORT    (65302)

// same as ips rule_level
// 0. Critical, 1. High, 2. Medium, 3. Low, 4. Informational
typedef enum {
    SECURITY_RISK_LEVEL_CRITICAL = 0,
    SECURITY_RISK_LEVEL_HIGH,
    SECURITY_RISK_LEVEL_MEDIUM,
    SECURITY_RISK_LEVEL_LOW,
    SECURITY_RISK_LEVEL_INFORMATIONAL
} SecurityRiskLevel;

typedef enum {
    THREAT_LOG_TYPE_AV = 1,
    THREAT_LOG_TYPE_IPS = 2,
    THREAT_LOG_TYPE_TI = 3,
    THREAT_LOG_TYPE_WAF = 4,
    THREAT_LOG_TYPE_AD = 5,
    THREAT_LOG_TYPE_BLKLIST = 6
} THREAT_LOG_TYPE;


#define HA_PROXY_WEB_NM_DPORT    (19090)

#define MANAGE_FRAME_EXPORT_FILE    "/home/config/current/manage_frame_export.conf"
#define MANAGE_FRAME_IMPORT_FILE    "/home/config/current/manage_frame_import.conf"

#define MANAGE_FRAME_SHM_FILE       "/home/manage_frame_shm.dat"
#define MANAGE_FRAME_CFG_SHM_FILE   "/home/manage_frame_cfg_shm.dat"
#define MANAGE_FRAME_GROUP_SHM_FILE "/home/manage_frame_group_shm.dat"

#define HYTF_VLAN_AUTH_FTOK_NAME "/home/vlan_auth.dat"

#define SESSION_CONTROL_SHM_FILE       "/home/session_control_shm.dat"
typedef enum {
    SESSION_AGEOUT_REASON_AGEOUT = 0,                       // ageout but not because of tcp fin/rst
    SESSION_AGEOUT_REASON_AGEOUT_TCP_FIN = 1,               // ageout by tcp fin
    SESSION_AGEOUT_REASON_AGEOUT_TCP_RST = 2,               // ageout by tcp rst
    SESSION_AGEOUT_REASON_AGEOUT_SESSION_LIMIT = 3,         // ageout but not because of tcp fin/rst, also match session limit warning
    SESSION_AGEOUT_REASON_AGEOUT_TCP_FIN_SESSION_LIMIT = 4, // ageout by tcp fin, also match session limit warning
    SESSION_AGEOUT_REASON_AGEOUT_TCP_RST_SESSION_LIMIT = 5, // ageout by tcp rst, also match session limit warning
    SESSION_AGEOUT_REASON_BLOCK = 6,                        // block but not because of session limit
    SESSION_AGEOUT_REASON_BLOCK_SESSION_LIMIT = 7,          // block because of session limit
    SESSION_AGEOUT_REASON
} session_ageout_reason;


#ifndef HOTAL_VLAN_ID_STR
#define HOTAL_VLAN_ID_STR           ".4094"
#endif

#ifndef HOTAL_VLAN_ID
#define HOTAL_VLAN_ID               4094
#endif 

#define HA_MAX_INSTANCE 32
#define HA_LOCAL_CONFIG_FILE        "/etc/keepalived/local_ha_config.conf"
#define HA_LOCAL_CONFIG_FILE_SAVED  "/home/config/current/local_ha_config.conf"
#define HA_SYNC_CONFIG_FILE         "/etc/keepalived/ha_auto_syn.conf"

#define CLOUD_SECURITY_IP_FILE      "/home/config/current/cloud_security_iplib"
#define CLOUD_SECURITY_URL_FILE     "/home/config/current/cloud_security_urllib"
#define CLOUD_SECURITY_CONTENT_FILE     "/home/config/current/cloud_security_contlib"

// rename in blklist_filter_ini_save
#define BLKLIST_FILTER_CONFIG_ID_FILE     "/home/config/current/blklist_filter_config_id.conf"
#define BLKLIST_FILTER_CONFIG_ID_FILE_HA     "/home/config/current/blklist_filter_config_id_ha.conf"
#define BLKLIST_FILTER_CONFIG_ID_FILE_TMP     "/tmp/blklist_filter_config_id.conf"
#define BLKLIST_FILTER_CONFIG_NODE_FILE     "/home/config/current/blklist_filter_config_node.conf"
#define BLKLIST_FILTER_CONFIG_NODE_FILE_TMP     "/tmp/blklist_filter_config_node.conf"
#define BLKLIST_FILTER_CONFIG_NODE_FILE_HA     "/home/config/current/blklist_filter_config_node_ha.conf"

// rename in PHP_FUNCTION(blklist_filter_export)
#define BLKLIST_FILTER_CONFIG_IO_FILE     "/mnt/blklist_filter_config_io.conf"
#define BLKLIST_FILTER_CONFIG_IO_FILE_TMP     "/tmp/blklist_filter_config_io.conf"

#define BLKLIST_FILTER_FIX_16G_FILE     "/mnt/blklist_filter_fix_16g"


enum HA_INSTANCE_STATE
{
    HA_STATE_NONE = 0, //not ha
    HA_STATE_MASTER = 'M',   //master
    HA_STATE_BACKUP = 'B',   //bakup
    HA_STATE_FAULT = 'F',    //fault
};

#define MAX_DEVICEPATTERN_GROUP 128
#define MAX_DEVICEPATTERN_GROUP_MAP_UCHAR_LEN 16 //MAX_DEVICEPATTERN_GROUP / 8
#define MAX_DEVICEPATTERN_ITEM_NUM 4096
#define MAX_DEVICEPATTERN_OPENPORT_NUM 10
#define MAX_DEVICEPATTERN_OSEXCLUDE_NUM 10
#define MAX_DEVICEPATTERN_LEVEL 4

enum ACE_DEBUG_MODULE
{
    /*0~4*/
    ADBG_K_BZIMAGE = 0,
    ADBG_K_NETWORK,
    ADBG_K_EUM,
    ADBG_K_FIREWALL,
    ADBG_U_NETWORK,

    /*5~9*/
    ADBG_U_EUM,
    ADBG_U_FIREWALL,
    ADBG_U_L7FILTER,
    ADBG_U_PHP,
    ADBG_U_URL,

    /*10~14*/
    ADBG_U_P3SCAN,
    ADBG_U_IPTK,


    ADBG_MODULE_MAX,
};

enum ACE_DEBUG_LEVEL
{
    ADBG_L0 = 0,
    ADBG_L1,
    ADBG_L2,
    ADBG_L3,
    ADBG_LEVEL_MAX,
};

typedef enum policy_cfg_mode
{
    POLICY_APPEND,
    POLICY_FIRST,
    POLICY_INSERT,
    POLICY_MODIFY,
    POLICY_MOVE_BEFORE,
    POLICY_MOVE_AFTER,
}POLICY_CFG_MODE;

enum _ace_dev_type
{
    ACE_DEV_ETHER_TYPE = 0,
    ACE_DEV_PPP_TYPE,
    ACE_DEV_IPGRE_TYPE,
    ACE_DEV_OTHER_TYPE,
};

enum _cloud_security_type
{
    CLOUD_SECURITY_IP_TYPE = 0,
    CLOUD_SECURITY_URL_TYPE,
};

extern U32 g_adbg_enable[ADBG_MODULE_MAX];
extern U32 g_adbg_level[ADBG_MODULE_MAX];

#define dbg_printf( dbgmod, dbglevel, fmt... ) \
{ \
    if ( ( dbgmod < ADBG_MODULE_MAX ) && g_adbg_enable[dbgmod] && ( g_adbg_level[dbgmod] > dbglevel ) ) \
    { \
        printf( fmt ); \
        syslog( LOG_USER | LOG_INFO, fmt ); \
    } \
}


#define ace_printf( fmt... ) syslog( LOG_USER | LOG_INFO,  ##fmt )
#define log_error(fmt, args...) do{ace_printf("%s,%d:",__FUNCTION__, __LINE__);ace_printf(fmt,##args);}while(0)
#define log_info(fmt, args...) ace_printf(fmt, ##args)

#define SHARE_MEM_SCAN_INTERVAL     10000

#if 0
#define ACE_MAX_USER_SERVICE_NUM   3
#define MAX_SERVICE_TYPE		1024 /* 1024*8 = 4096*/  
#define MAX_SERVICE_TYPE_NUM	8192 /* 1024*8 = 4096*/  
#define MAX_SERVICE_GROUP_NUM   16
#endif

#define ACE_HOST_MANAGE_HOST_HASH_NEW_BUCKET 4
#define ACE_HOST_MANAGE_HOST_HASH_NEW_BUCKET_BIT 2

#define ACE_HOST_MANAGE_HOST_HASH_CONFLICT_MAX 32


#define ACE_HOST_MANAGE_SERVICE_HASH_NEW_BUCKET 8
#define ACE_HOST_MANAGE_SERVICE_HASH_NEW_BUCKET_BIT 3
#define ACE_HOST_MANAGE_SERVICE_HASH_METHOD_NUM 3

#define HOST_HASH_MAX 3
#define UNIX_SOCK_PATH_FAKE_KERNEL	"/run/vpp/.fake_kernel"
#define UNIX_SOCK_PATH_SYSAPP	    "/run/vpp/.sysapp"
#define UNIX_SOCK_PATH_DATA_CONTROL	"/run/vpp/.data_control"
#define UNIX_SOCK_PATH_SWITCH_IFS	"/run/vpp/.switch_ifs"
#define UNIX_SOCK_PATH_NORMAL_IFS	"/run/vpp/.normal_ifs"
#define UNIX_SOCK_PATH_IPS_UNIX	"/run/vpp/.ips_unix"
#define SOCKET_UDP_DPORT_L7PATTERN_CHAN_MAX	(16)
#define SOCKET_UDP_DPORT_TO_L7FEATURE	(SOCKET_UDP_DPORT_L7PATTERN + SOCKET_UDP_DPORT_L7PATTERN_CHAN_MAX)
#define SOCKET_UDP_DPORT_TO_L7DPI_START (SOCKET_UDP_DPORT_TO_L7FEATURE + 1)
#define SOCKET_UDP_DPORT_TO_L7DPI_MAX   4
#define SOCKET_UDP_DPORT_TO_DPI    (SOCKET_UDP_DPORT_TO_L7DPI_START)
#define SOCKET_UDP_DPORT_TO_MAIL_FILTER (SOCKET_UDP_DPORT_TO_L7DPI_START + SOCKET_UDP_DPORT_TO_L7DPI_MAX + 1)
#define SOCKET_UDP_DPORT_FROM_L7PATTERN_TO_COLLECTOR    (SOCKET_UDP_DPORT_TO_MAIL_FILTER + 1) 


#ifndef INDUSTRIAL_CONTROL_CONFIG_FILE
#define INDUSTRIAL_CONTROL_CONFIG_FILE          "/home/config/current/industrial_control_config.conf"
#define TEMP_INDUSTRIAL_CONTROLCONFIG_FILE      "/home/config/current/industrial_control_config.tmp"
#endif


#ifdef COMPILING_IN_VPP
#if 0
#include <rte_config.h>
#include <rte_spinlock.h>
#include <rte_rwlock.h>
#if 0
typedef rte_rwlock_t 	hylab_rwlock_t;
#define hylab_rwlock_read_lock(x)	rte_rwlock_read_lock(x)
#define hylab_rwlock_read_unlock(x)	rte_rwlock_read_unlock(x)
#define hylab_rwlock_write_lock(x)	rte_rwlock_write_lock(x)
#define hylab_rwlock_write_unlock(x)	rte_rwlock_write_unlock(x)
#define hylab_rwlock_init(x)	rte_rwlock_init(x)

#define DEFINE_RWLOCK(x) hylab_rwlock_t x = RTE_RWLOCK_INITIALIZER
#else
typedef clib_rwlock_t 	hylab_rwlock_t;
#define hylab_rwlock_read_lock(x)	clib_rwlock_reader_lock(x)
#define hylab_rwlock_read_unlock(x)	clib_rwlock_reader_unlock(x)
#define hylab_rwlock_write_lock(x)	clib_rwlock_writer_lock(x)
#define hylab_rwlock_write_unlock(x)	clib_rwlock_writer_unlock(x)
#define hylab_rwlock_init(x)	clib_rwlock_init(x)

#define DEFINE_RWLOCK(x) hylab_rwlock_t x = NULL;
#endif

#if 0
typedef rte_spinlock_t  hylab_spinlock_t;
#define hylab_spinlock_lock(x)	rte_spinlock_lock(x)
#define hylab_spinlock_unlock(x)	rte_spinlock_unlock(x)
#define hylab_spinlock_init(x)	rte_spinlock_init(x)
#define HYLAB_SPINLOCK_INITIALIZER	RTE_SPINLOCK_INITIALIZER
#else
typedef clib_spinlock_t  hylab_spinlock_t;
#define hylab_spinlock_lock(x)	clib_spinlock_lock(x)
#define hylab_spinlock_unlock(x)	clib_spinlock_unlock(x)
#define hylab_spinlock_init(x)	clib_spinlock_init(x)
#define HYLAB_SPINLOCK_INITIALIZER	NULL
#endif

#define DEFINE_SPINLOCK(x)	hylab_spinlock_t x = HYLAB_SPINLOCK_INITIALIZER
#define DEFINE_MUTEX(x) DEFINE_SPINLOCK(x)
#define mutex_unlock(x) hylab_spinlock_unlock(x) 
#define mutex_lock(x) hylab_spinlock_lock(x) 
#endif

/* Plain integer GFP bitmasks. Do not use this directly. */
#define ___GFP_DMA		0x01u
#define ___GFP_HIGHMEM		0x02u
#define ___GFP_DMA32		0x04u
#define ___GFP_MOVABLE		0x08u
#define ___GFP_WAIT		0x10u
#define ___GFP_HIGH		0x20u
#define ___GFP_IO		0x40u
#define ___GFP_FS		0x80u
#define ___GFP_COLD		0x100u

/* This equals 0, but use constants in case they ever change */
#define GFP_NOWAIT	(GFP_ATOMIC & ~___GFP_HIGH)
/* GFP_ATOMIC means both !wait (__GFP_WAIT not set) and use emergency pool */
#define GFP_ATOMIC	(___GFP_HIGH)
#define GFP_NOIO	(___GFP_WAIT)
#define GFP_NOFS	(___GFP_WAIT | ___GFP_IO)
#define GFP_KERNEL	(___GFP_WAIT | ___GFP_IO | ___GFP_FS)

#define IP_OFFSET	0x1FFF		/* "Fragment Offset" part	*/

#define NF_DROP 0
#define NF_ACCEPT 1
#define NF_STOLEN 2
#define NF_QUEUE 3
#define NF_REPEAT 4
#define NF_STOP 5
#define NF_IMQ_QUEUE 6
#define NF_CONNTRACK 7

/* we overload the higher bits for encoding auxiliary data such as the queue
 * number or errno values. Not nice, but better than additional function
 * arguments. */
#define NF_VERDICT_MASK 0x000000ff

/* extra verdict flags have mask 0x0000ff00 */
#define NF_VERDICT_FLAG_QUEUE_BYPASS	0x00008000

/* queue number (NF_QUEUE) or errno (NF_DROP) */
#define NF_VERDICT_QMASK 0xffff0000
#define NF_VERDICT_QBITS 16

#define NF_QUEUE_NR(x) ((((x) << 16) & NF_VERDICT_QMASK) | NF_QUEUE)

#define NF_DROP_ERR(x) (((-x) << 16) | NF_DROP)
#define VLAN_HLEN	4		/* The additional bytes (on top of the Ethernet header)*/

#define VLAN_VID_MASK		0x0fff /* VLAN Identifier */
#define vlan_tx_tag_present(x)	(clib_host_to_net_u16(ETHERNET_TYPE_VLAN) == ethernet_buffer_get_header(x)->type)
#define vlan_tx_vlan_tci(x)		(clib_net_to_host_u16(((ethernet_vlan_header_t *)(ethernet_buffer_get_header(x) + 1))->priority_cfi_and_id))
#define vlan_tx_tag_get(x)	(vlan_tx_vlan_tci(x)&VLAN_VID_MASK)
static_always_inline unsigned compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
	const u16 *a = (const u16 *) addr1;
	const u16 *b = (const u16 *) addr2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}

#define IP_MF		0x2000		/* Flag: "More Fragments"	*/
static_always_inline int ip_is_fragment(const struct iphdr *iph)
{
	return (iph->frag_off & htons(IP_MF | IP_OFFSET)) != 0;
}


/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */
enum ip_conntrack_info {
        /* Part of an established connection (either direction). */
        IP_CT_ESTABLISHED,

        /* Like NEW, but related to an existing connection, or ICMP error
           (in either direction). */
        IP_CT_RELATED,

        /* Started a new connection to track (only
           IP_CT_DIR_ORIGINAL); may be a retransmission. */
        IP_CT_NEW,

        /* >= this indicates reply direction */
        IP_CT_IS_REPLY,

        IP_CT_ESTABLISHED_REPLY = IP_CT_ESTABLISHED + IP_CT_IS_REPLY,
        IP_CT_RELATED_REPLY = IP_CT_RELATED + IP_CT_IS_REPLY,
        IP_CT_NEW_REPLY = IP_CT_NEW + IP_CT_IS_REPLY,
        /* Number of distinct IP_CT types (no NEW in reply dirn). */
        IP_CT_NUMBER = IP_CT_IS_REPLY * 2 - 1
};

enum ip_conntrack_dir {
        IP_CT_DIR_ORIGINAL,
        IP_CT_DIR_REPLY,
        IP_CT_DIR_MAX
};

enum nf_nat_manip_type {
	IP_NAT_MANIP_SRC,
	IP_NAT_MANIP_DST
};

#define CTINFO2DIR(ctinfo) ((ctinfo) >= IP_CT_IS_REPLY ? IP_CT_DIR_REPLY : IP_CT_DIR_ORIGINAL)


union nf_inet_addr {
	u32		all[4];
	u32		ip;
	u32		ip6[4];
	struct in_addr	in;
	struct in6_addr	in6;
};

/* expectation flags */
#define NF_CT_EXPECT_PERMANENT		0x1
#define NF_CT_EXPECT_INACTIVE		0x2
#define NF_CT_EXPECT_USERSPACE		0x4

#define CONFIG_NF_CONNTRACK_FTP
#define CONFIG_NF_CONNTRACK_PPTP
#define CONFIG_NF_NAT_PPTP
#define CONFIG_NF_CONNTRACK_H323
#define CONFIG_NF_CONNTRACK_SIP

#define NUM_SEQ_TO_REMEMBER 2

/*
struct list_head {
        struct list_head *next, *prev;
};
*/
/*
struct hlist_head {
        struct hlist_node *first;
};

struct hlist_node {
        struct hlist_node *next, **pprev;
};
*/
#define kfree(x)	free(x)
#define vfree(x)	free(x)

struct nf_conn
{
	void* unused;
};

struct nf_conn_help
{
	void* unused;
};

typedef unsigned int gfp_t;

#define EXPORT_SYMBOL(x)
#define EXPORT_SYMBOL_GPL(x)
#ifndef unlikely
#define unlikely(x) PREDICT_FALSE(x)
#endif
#ifndef likely
#define likely(x) PREDICT_TRUE(x)
#endif
#ifndef __read_mostly
#define __read_mostly
#endif
#undef malloc
//clib_mem_alloc(x)
#define malloc(x) clib_mem_alloc_or_null(x)
//#define malloc(x) hylab_clib_mem_alloc_or_null(x, __FILE__, __LINE__)
#undef free
#define free(x)	clib_mem_free(x)
//#define free(x)	hylab_clib_mem_free(x, __FILE__, __LINE__)
#define KERN_EMERG      "<0>"   /* system is unusable                   */
#define KERN_ALERT      "<1>"   /* action must be taken immediately     */
#define KERN_CRIT       "<2>"   /* critical conditions                  */
#define KERN_ERR        "<3>"   /* error conditions                     */
#define KERN_WARNING    "<4>"   /* warning conditions                   */
#define KERN_NOTICE     "<5>"   /* normal but significant condition     */
#define KERN_INFO       "<6>"   /* informational                        */
#define KERN_DEBUG      "<7>"   /* debug-level messages                 */

extern int hylab_util_log_printf(int level, const char * fmt, ...);
#define printk(fmt, args...) hylab_util_log_printf(0,"%s %s %d->" fmt,__FUNCTION__,__FILE__,__LINE__,##args)
#define WARN_ONCE(level, fmt, args...)	{static char once=0;if(!once){once=1;hylab_util_log_printf(level,"%s %s %d->" fmt,__FUNCTION__,__FILE__,__LINE__,##args);}}
#define SKB_TRACE_DROP (0x01)
#define SKB_TRACE_PATH (0x02)
#define SKB_TRACE_PACKET (0x04)
#define SKB_TRACE_ONEWAY (0x08)
#define vzalloc(x)  ({void* __m__ = malloc(x); if (__m__)memset(__m__,0x00,x); __m__;})//calloc(1, (x))
#define vmalloc(x)	malloc(x)
#define kmalloc(x,y)	malloc(x)
#define vmalloc_user(x)	malloc(x)
#define pskb_may_pull(x, y)	(x)
#define skb_is_nonlinear(x)	0
#define skb_has_frag_list(x) 0
//#define DEFINE_RWLOCK(x) clib_rwlock_t x = NULL

static_always_inline unsigned char __tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static_always_inline unsigned char __toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({	type __dummy; \
	typeof(x) __dummy2; \
	(void)(&__dummy == &__dummy2); \
	1; \
})

#define time_after(a,b)         \
        (((long)((b) - (a)) < 0))
#define time_before(a,b)        time_after(b,a)

#define time_after_eq(a,b)      \
        (((long)((a) - (b)) >= 0))
#define time_before_eq(a,b)     time_after_eq(b,a)

#define get_seconds()	time(NULL)
enum tcp_conntrack {
	TCP_CONNTRACK_NONE,
	TCP_CONNTRACK_SYN_SENT,
	TCP_CONNTRACK_SYN_RECV,
	TCP_CONNTRACK_ESTABLISHED,
	TCP_CONNTRACK_FIN_WAIT,
	TCP_CONNTRACK_CLOSE_WAIT,
	TCP_CONNTRACK_LAST_ACK,
	TCP_CONNTRACK_TIME_WAIT,
	TCP_CONNTRACK_CLOSE,
	TCP_CONNTRACK_LISTEN,	/* obsolete */
#define TCP_CONNTRACK_SYN_SENT2	TCP_CONNTRACK_LISTEN
	TCP_CONNTRACK_MAX,
	TCP_CONNTRACK_IGNORE
};

#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/socket.h>

struct tcphdr {
        __be16  source;
        __be16  dest;
        __be32  seq;
        __be32  ack_seq;
#if defined(__LITTLE_ENDIAN_BITFIELD)
        __u16   res1:4,
                doff:4,
                fin:1,
                syn:1,
                rst:1,
                psh:1,
                ack:1,
                urg:1,
                ece:1,
                cwr:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
        __u16   doff:4,
                res1:4,
                cwr:1,
                ece:1,
                urg:1,
                ack:1,
                psh:1,
                rst:1,
                syn:1,
                fin:1;
#else
#error  "Adjust your <asm/byteorder.h> defines"
#endif
        __be16  window;
        __sum16 check;
        __be16  urg_ptr;
};

/*
 *	TCP option
 */
 
#define TCPOPT_NOP		1	/* Padding */
#define TCPOPT_EOL		0	/* End of options */
#define TCPOPT_MSS		2	/* Segment size negotiating */
#define TCPOPT_WINDOW		3	/* Window scaling */
#define TCPOPT_SACK_PERM        4       /* SACK Permitted */
#define TCPOPT_SACK             5       /* SACK Block */
#define TCPOPT_TIMESTAMP	8	/* Better RTT estimations/PAWS */
#define TCPOPT_MD5SIG		19	/* MD5 Signature (RFC2385) */
#define TCPOPT_COOKIE		253	/* Cookie extension (experimental) */

/* But this is what stacks really send out. */
#define TCPOLEN_TSTAMP_ALIGNED		12
#define TCPOLEN_WSCALE_ALIGNED		4
#define TCPOLEN_SACKPERM_ALIGNED	4
#define TCPOLEN_SACK_BASE		2
#define TCPOLEN_SACK_BASE_ALIGNED	4
#define TCPOLEN_SACK_PERBLOCK		8
#define TCPOLEN_MD5SIG_ALIGNED		20
#define TCPOLEN_MSS_ALIGNED		4

/*
 *     TCP option lengths
 */

#define TCPOLEN_MSS            4
#define TCPOLEN_WINDOW         3
#define TCPOLEN_SACK_PERM      2
#define TCPOLEN_TIMESTAMP      10
#define TCPOLEN_MD5SIG         18
#define TCPOLEN_COOKIE_BASE    2	/* Cookie-less header extension */
#define TCPOLEN_COOKIE_PAIR    3	/* Cookie pair header extension */
#define TCPOLEN_COOKIE_MIN     (TCPOLEN_COOKIE_BASE+TCP_COOKIE_MIN)
#define TCPOLEN_COOKIE_MAX     (TCPOLEN_COOKIE_BASE+TCP_COOKIE_MAX)


union tcp_word_hdr { 
	struct tcphdr hdr;
	__be32 		  words[5];
}; 

#define tcp_flag_word(tp) ( ((union tcp_word_hdr *)(tp))->words [3]) 

/*
 *	IPv6 fixed header
 *
 *	BEWARE, it is incorrect. The first 4 bits of flow_lbl
 *	are glued to priority now, forming "class".
 */

struct ipv6hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8			priority:4,
				version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8			version:4,
				priority:4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8			flow_lbl[3];

	__be16			payload_len;
	__u8			nexthdr;
	__u8			hop_limit;

	struct	in6_addr	saddr;
	struct	in6_addr	daddr;
};

struct ipv6_opt_hdr {
	__u8 		nexthdr;
	__u8 		hdrlen;
	/* 
	 * TLV encoded option data follows.
	 */
} __attribute__((packed));	/* required for some archs */
#define ipv6_optlen(p)  (((p)->hdrlen+1) << 3)

#define NEXTHDR_HOP		0	/* Hop-by-hop option header. */
#define NEXTHDR_TCP		6	/* TCP segment. */
#define NEXTHDR_UDP		17	/* UDP message. */
#define NEXTHDR_IPV6		41	/* IPv6 in IPv6 */
#define NEXTHDR_ROUTING		43	/* Routing header. */
#define NEXTHDR_FRAGMENT	44	/* Fragmentation/reassembly header. */
#define NEXTHDR_ESP		50	/* Encapsulating security payload. */
#define NEXTHDR_AUTH		51	/* Authentication header. */
#define NEXTHDR_ICMP		58	/* ICMP for IPv6. */
#define NEXTHDR_NONE		59	/* No next header */
#define NEXTHDR_DEST		60	/* Destination options header. */
#define NEXTHDR_MOBILITY	135	/* Mobility header. */

#define NEXTHDR_MAX		255

static_always_inline int ipv6_addr_cmp(const struct in6_addr *a1, const struct in6_addr *a2)
{
	return memcmp(a1, a2, sizeof(struct in6_addr));
}

static_always_inline int ipv6_ext_hdr(u8 nexthdr)
{
        /*
         * find out if nexthdr is an extension header or a protocol
         */
        return   (nexthdr == NEXTHDR_HOP)       ||
                 (nexthdr == NEXTHDR_ROUTING)   ||
                 (nexthdr == NEXTHDR_FRAGMENT)  ||
                 (nexthdr == NEXTHDR_AUTH)      ||
                 (nexthdr == NEXTHDR_NONE)      ||
                 (nexthdr == NEXTHDR_DEST);
}

/*
 *	fragmentation header
 */

struct frag_hdr {
	__u8	nexthdr;
	__u8	reserved;
	__be16	frag_off;
	__be32	identification;
};

#define	IP6_MF	0x0001

#define IP_NAT_RANGE_MAP_IPS 1
#define IP_NAT_RANGE_PROTO_SPECIFIED 2
#define IP_NAT_RANGE_PROTO_RANDOM 4
#define IP_NAT_RANGE_PERSISTENT 8

/* The protocol-specific manipulable parts of the tuple. */
union nf_conntrack_man_proto {
	/* Add other protocols here. */
	__be16 all;

	struct {
		__be16 port;
	} tcp;
	struct {
		__be16 port;
	} udp;
	struct {
		__be16 id;
	} icmp;
	struct {
		__be16 port;
	} dccp;
	struct {
		__be16 port;
	} sctp;
	struct {
		__be16 key;	/* GRE key is 32bit, PPtP only uses 16bit */
	} gre;
};

/* Single range specification. */
struct nf_nat_range {
	/* Set to OR of flags above. */
	unsigned int flags;

	/* Inclusive: network order. */
	__be32 min_ip, max_ip;

	/* Inclusive: network order */
	union nf_conntrack_man_proto min, max;
};

/* For backwards compat: don't use in modern code. */
struct nf_nat_multi_range_compat {
	unsigned int rangesize; /* Must be 1. */

	/* hangs off end. */
	struct nf_nat_range range[1];
};

#define nf_nat_multi_range nf_nat_multi_range_compat

static_always_inline void skb_reset_network_header(vlib_buffer_t *skb)
{
	vnet_buffer (skb)->l3_hdr_offset = skb->current_data;
}

static_always_inline void skb_reset_mac_header(vlib_buffer_t *skb)
{
	vnet_buffer (skb)->l2_hdr_offset = skb->current_data;
}

static_always_inline unsigned char *skb_set_current(vlib_buffer_t *skb, i16 zero_based_offset)
{
    i16 current_based_offset = zero_based_offset - skb->current_data;
	skb->current_data = zero_based_offset;
	skb->current_length -= current_based_offset;
	return skb->data + skb->current_data;
}

static_always_inline unsigned char *skb_push(vlib_buffer_t *skb, unsigned int len)
{

	if (VLIB_BUFFER_PRE_DATA_SIZE + skb->current_data < len)
	{
		WARN_ONCE(1,"%s %s %d --- wrong current_data %d len %d pre_data_size %d\r\n", __FUNCTION__,__FILE__,__LINE__,skb->current_data,len,VLIB_BUFFER_PRE_DATA_SIZE);
		return NULL;
	}

	skb->current_data -= len;
	skb->current_length += len;
	return skb->data + skb->current_data;
}

static_always_inline unsigned char *skb_pull(vlib_buffer_t *skb, unsigned int len)
{
	if (skb->current_length < len)
	{
		WARN_ONCE(1,"%s %s %d --- wrong current_length %d len %d\r\n", __FUNCTION__,__FILE__,__LINE__,skb->current_length,len);
		return NULL;
	}
	skb->current_data += len;
	skb->current_length -= len;
	return skb->data + skb->current_data;
}

static_always_inline struct ethhdr *eth_hdr(const vlib_buffer_t *skb)
{
	return (struct ethhdr *)(skb->data + vnet_buffer (skb)->l2_hdr_offset);
}

static_always_inline struct iphdr *ip_hdr(const vlib_buffer_t *skb)
{
	return (struct iphdr *)(skb->data + vnet_buffer (skb)->l3_hdr_offset);
}

static_always_inline struct udphdr* udp_hdr(const vlib_buffer_t *skb)
{
	return (struct udphdr *)(skb->data + vnet_buffer (skb)->l4_hdr_offset);
}

static_always_inline struct tcphdr* tcp_hdr(const vlib_buffer_t *skb)
{
	return (struct tcphdr *)(skb->data + vnet_buffer (skb)->l4_hdr_offset);
}

static_always_inline struct icmphdr* icmp_hdr(const vlib_buffer_t *skb)
{
    return (struct icmphdr *)(skb->data + vnet_buffer (skb)->l4_hdr_offset);
}

static_always_inline struct icmp6hdr* icmp6_hdr(const vlib_buffer_t *skb)
{
    return (struct icmp6hdr *)(skb->data + vnet_buffer (skb)->l4_hdr_offset);
}

static_always_inline void* l4_hdr(const vlib_buffer_t *skb)
{
	return (void *)(skb->data + vnet_buffer (skb)->l4_hdr_offset);
}

#define skb_network_header_len(b) (vnet_buffer(b)->l4_hdr_offset - vnet_buffer(b)->l3_hdr_offset)
#define skb_mac_header_len(b) (vnet_buffer(b)->l3_hdr_offset - vnet_buffer(b)->l2_hdr_offset)

static_always_inline unsigned int ip_hdrlen(const vlib_buffer_t *skb)
{
	return ip_hdr(skb)->ihl * 4;
}

static_always_inline unsigned char *skb_network_header(const vlib_buffer_t *skb)
{
	return (unsigned char*)ip_hdr(skb);
}

static_always_inline unsigned char *skb_mac_header(const vlib_buffer_t *skb)
{
	return (unsigned char *)eth_hdr(skb);
}

static_always_inline unsigned char *skb_tail_pointer(vlib_buffer_t *skb)
{
	return skb->data+skb->current_data+skb->current_length;
}

//in common we use skb in l3,so offset is 0
static_always_inline int skb_network_offset(const vlib_buffer_t *skb)
{
	return skb->current_data-vnet_buffer (skb)->l3_hdr_offset;
}

static_always_inline unsigned char *skb_put(vlib_buffer_t *skb, unsigned int len)
{
    unsigned char *tmp = skb_tail_pointer(skb);
    skb->current_length += len;
    return tmp;
}

static_always_inline void __skb_trim(vlib_buffer_t *skb, unsigned int len)
{
    skb->current_length = len;
}

static_always_inline unsigned char *skb_transport_body(const vlib_buffer_t *skb, int *len)
{
#if 0
    unsigned char *body = NULL;
    struct iphdr *iph = ip_hdr(skb);
    if (len)
        *len = 0;
/*
    if (skb->skb_trace)
        trace_log_printf(TRACE_LOG_LEVEL_ERROR, "%s %d skp %p hytf_ct %p 0x%8.8x->0x%8.8x current_data %i l3_hdr_offset %d",
                __FUNCTION__,__LINE__,skb,skb->hytf_ct,iph->saddr,iph->daddr,skb->current_data,vnet_buffer (skb)->l3_hdr_offset);
*/
    if (!iph || skb->current_data != vnet_buffer (skb)->l3_hdr_offset)
    {
        return NULL;
    }
    else
    {
        unsigned short iplen = ntohs(iph->tot_len);
        unsigned short iphlen = iph->ihl * 4;
/*
        if (skb->skb_trace)
            trace_log_printf(TRACE_LOG_LEVEL_ERROR, "%s %d skp %p hytf_ct %p 0x%8.8x->0x%8.8x iplen %u iphlen %u current_length %u",
                    __FUNCTION__,__LINE__,skb,skb->hytf_ct,iph->saddr,iph->daddr,iplen,iphlen,skb->current_length);
*/
        if (iplen > skb->current_length)
        {
            return NULL;
        }
   
        if (iph->protocol == IPPROTO_TCP)
        {
            struct tcphdr *th = (struct tcphdr *)((char *)iph + iphlen);
            
            body = (unsigned char *)th + th->doff * 4;
            if (len)
                *len = iplen - iphlen - th->doff * 4;                
        }
        else if (iph->protocol == IPPROTO_UDP)
        {
            body = (unsigned char *)iph + iphlen + 8;
            if (len)
                *len = iplen - iphlen - 8; 
        }
        return body;
    }
#else
    if(len)
    {
        *len = (int)(skb->l5_content_len);
    }
    return (skb->l5_content_len) ? (unsigned char*)(skb->data + skb->l5_hdr_offset) : NULL;
#endif
}

static_always_inline int skb_after_network_data_len(const vlib_buffer_t *skb)
{
	if (skb->ipv6)
	{
		struct ipv6hdr *iph = (struct ipv6hdr *)skb_network_header(skb);
		
		if (unlikely((!iph || skb->current_data != vnet_buffer (skb)->l3_hdr_offset)))
	    {
#if 0
#ifdef included_vlib_trace_h
	        eth_pkt_trace_printf((vlib_buffer_t*)skb,__FUNCTION__,"",__LINE__, "skb_after_network_data_len");
	        eth_pkt_printf((void*)skb->data, skb->current_length);
	        trace_coordinate_stack();
#endif
#endif
	        return 0;
	    }

		return ntohs(iph->payload_len) + sizeof(struct ipv6hdr) < skb->current_length ? ntohs(iph->payload_len) + sizeof(struct ipv6hdr) : skb->current_length;
	}
	else
	{
	    struct iphdr *iph = ip_hdr(skb);

	    if (unlikely((!iph || skb->current_data != vnet_buffer (skb)->l3_hdr_offset)))
	    {
#if 0
#ifdef included_vlib_trace_h
	        eth_pkt_trace_printf((vlib_buffer_t*)skb,__FUNCTION__,"",__LINE__, "skb_after_network_data_len");
	        eth_pkt_printf((void*)skb->data, skb->current_length);
	        trace_coordinate_stack();
#endif
#endif
	        return 0;
	    }

	    return ntohs(iph->tot_len) < skb->current_length ? ntohs(iph->tot_len) : skb->current_length;
	}
}

static_always_inline int skb_data_len(const vlib_buffer_t *skb)
{
	static __thread vlib_buffer_t *local_skb;
	static __thread int len;

	if (skb != local_skb)
	{
		local_skb = (vlib_buffer_t *)skb;

		if (skb->l5_hdr_offset)
		{
			len = skb->l5_content_len + skb->l5_hdr_offset - vnet_buffer (skb)->l2_hdr_offset;
			
			if (skb->map_dev_vlan)
				len -= 4;

			if (len < 0)
				len = 0;
		}
		else if (skb->current_data == vnet_buffer (skb)->l3_hdr_offset)
		{
			len = skb_after_network_data_len(skb)+14;
		}
		else
		{
			len = skb->current_length + 14;
		}
	}

	return len;
}


static_always_inline void *skb_header_pointer(const vlib_buffer_t *skb, int offset,
				       int len, void *buffer)
{
	void* p_ip = ip_hdr(skb);
	return p_ip + offset;
}

static_always_inline struct ipv6hdr *ipv6_hdr(const vlib_buffer_t *skb)
{
	return (struct ipv6hdr *)skb_network_header(skb);
}

#define skb_make_writable(x,y) 1

static_always_inline struct vnet_hw_interface_t *dev_get_by_name(vnet_main_t *vnet, const char *name)
{
#if 0
	vnet_hw_interface_t *hi = NULL;
	u32 hw_if_index;
	uword *p;
	u8* if_name = format(0, "%s", name);
	if (!(p = hash_get (vnet->interface_main.hw_interface_by_name, if_name)))
	{
//printk("%s %s %d --- name %s hash_p %p\r\n",__FUNCTION__,__FILE__,__LINE__,name,vnet->interface_main.hw_interface_by_name);
		goto error_out;
	}
	hw_if_index = p[0];
	hi = vnet_get_hw_interface (vnet, hw_if_index);
//printk("%s %s %d --- name %s hi %p\r\n",__FUNCTION__,__FILE__,__LINE__,name,hi);
error_out:
	if (if_name)
		vec_free(if_name);
	return hi;
#else
    vnet_hw_interface_t *hi = NULL;
    u32 hw_if_index;
    uword *p;
    vec_header_t *vh;
    u8 *v;
    int name_len;
    char name_buffer[128];
    /* Find the user vector pointer */
    v = (u8 *) (name_buffer + _vec_round_size(sizeof(vec_header_t)));
    name_len = strlen(name);
    if (name_len >= sizeof(name_buffer) - _vec_round_size(sizeof(vec_header_t)))
    {
        goto error_out;
    }
    /* Finally, the vector header */
    vh = _vec_find (v);
    vh->len = name_len;
    strncpy((char*)v, name, name_len);
    v[name_len] = 0;
    if (!(p = hash_get (vnet->interface_main.hw_interface_by_name, v)))
    {
        goto error_out;
    }
    hw_if_index = p[0];
    hi = vnet_get_hw_interface (vnet, hw_if_index);
error_out:
    return hi;
#endif
}

static_always_inline vnet_sw_interface_t *sw_get_by_name (vnet_main_t *vnm, const char *name)
{
  vnet_hw_interface_t *hi;
  u32 hw_if_index, id;
  u32 sw_if_index;
  u8 *if_name;
  uword *p;
  int name_len;
  char name_buffer[128];
  vec_header_t *vh;
  char* p_tmp;

  id = ~0;
  if_name = (u8*)(name_buffer + _vec_round_size(sizeof(vec_header_t)));
  strncpy((char*)if_name, name, sizeof(name_buffer) - (_vec_round_size(sizeof(vec_header_t))));
  if_name[sizeof(name_buffer) - (_vec_round_size(sizeof(vec_header_t))) -1] = 0;
  p_tmp = strchr((char*)if_name, '.');
  if (p_tmp)
  {
    *p_tmp = 0;
    ++p_tmp;
    id = atoi(p_tmp);
  }
  name_len = strlen((char*)if_name);
  vh = _vec_find (if_name);
  vh->len = name_len;
  if_name[name_len] = 0;
  if ((p = hash_get (vnm->interface_main.hw_interface_by_name, if_name)))
  {
    hw_if_index = p[0];
  }
  else
  {
    goto error;
  }

  hi = vnet_get_hw_interface (vnm, hw_if_index);
  if (~0 == id)
    {
      sw_if_index = hi->sw_if_index;
    }
  else
    {
      if (!(p = hash_get (hi->sub_interface_sw_if_index_by_id, id)))
	goto error;
      sw_if_index = p[0];
    }
  if (!vnet_sw_interface_is_api_visible (vnm, sw_if_index))
    goto error;
  return vnet_get_sw_interface (vnm, sw_if_index);
error:
  return NULL;
}

static_always_inline struct vnet_hw_interface_t *dev_get_by_name_by_sw(vnet_main_t *vnet, const char *name)
{
	struct vnet_sw_interface_t *si;
	
	si = sw_get_by_name(vnet, name);
    if (si)
    {
		struct vnet_sw_interface_t *si_sup;
		vnet_hw_interface_t *hi = NULL;
		
		si_sup = vnet_get_sup_sw_interface (vnet, si->sw_if_index);
	    ASSERT (si_sup->type == VNET_SW_INTERFACE_TYPE_HARDWARE);
	    hi = vnet_get_hw_interface (vnet, si_sup->hw_if_index);
		return hi;
    }

	return NULL;
}

struct vlan_ethhdr {
	unsigned char	h_dest[ETH_ALEN];
	unsigned char	h_source[ETH_ALEN];
	__be16		h_vlan_proto;
	__be16		h_vlan_TCI;
	__be16		h_vlan_encapsulated_proto;
};

static_always_inline struct vlan_ethhdr *vlan_eth_hdr(const vlib_buffer_t *skb)
{
	return (struct vlan_ethhdr *)skb_mac_header(skb);
}

#define PPP_IP      0x21    /* Internet Protocol */
static_always_inline __be16 vlan_proto(const vlib_buffer_t *skb)
{
    if (ethernet_buffer_get_header(((vlib_buffer_t*)skb))->type == htons(ETH_P_8021Q))
        return vlan_eth_hdr(skb)->h_vlan_encapsulated_proto;
    else
        return 0;
}
static_always_inline __be16 pppoe_proto(const vlib_buffer_t *skb)
{
    return *((__be16 *)(skb_mac_header(skb) + ETH_HLEN + sizeof(struct pppoe_hdr)));
}

struct qinq_ethhdr {
   unsigned char    h_dest[ETH_ALEN];      /* destination eth addr  */
   unsigned char    h_source[ETH_ALEN];    /* source ether addr */
   __be16           h_vlan_proto;              /* Should always be 0x8100 */
   __be16           h_vlan_TCI;                /* Encapsulates priority and VLAN ID */
   unsigned short   h_vlan_encapsulated_proto; /* packet type ID field (or len) */
   __be16           h_qinq_TCI;                /* Encapsulates priority and VLAN ID */
   unsigned short   h_qinq_encapsulated_proto; /* packet type ID field (or len) */
};

static_always_inline struct qinq_ethhdr *qinq_eth_hdr(const vlib_buffer_t *skb)
{
    return (struct qinq_ethhdr *)skb_mac_header(skb);
}

static_always_inline __be16 qinq_proto(const vlib_buffer_t *skb)
{
    return qinq_eth_hdr(skb)->h_qinq_encapsulated_proto;
}

static_always_inline __be16 pppoe_vlan_proto(const vlib_buffer_t *skb)
{
    return *((__be16 *)(skb_mac_header(skb) + ETH_HLEN + 4 + sizeof(struct pppoe_hdr)));
}

static_always_inline __be16 pppoe_qinq_proto(const vlib_buffer_t *skb)
{
    return *((__be16 *)(skb_mac_header(skb) + ETH_HLEN + 8 + sizeof(struct pppoe_hdr)));
}

#define IS_QINQ_PPPOE_IP(skb) \
	(vlan_proto(skb) == htons(ETH_P_8021Q) && 	\
 	 qinq_proto(skb) == htons(ETH_P_PPP_SES) && 	\
 	 pppoe_qinq_proto(skb) == htons(PPP_IP))

#define IS_QINQ_PPPOE_NEGO(skb) \
    (vlan_proto(skb) == htons(ETH_P_8021Q) &&   \
     qinq_proto(skb) == htons(ETH_P_PPP_SES) &&     \
     pppoe_qinq_proto(skb) != htons(PPP_IP))

#define IS_QINQ_IP(skb) \
    (vlan_proto(skb) == htons(ETH_P_8021Q) && \
     qinq_proto(skb) == htons(ETH_P_IP))

#define IS_PPPOE_IP(skb) \
    (ethernet_buffer_get_header(skb)->type == htons(ETH_P_PPP_SES) && \
     pppoe_proto(skb) == htons(PPP_IP))

#define IS_PPPOE_IPV6(skb) \
	(ethernet_buffer_get_header(skb)->type == htons(ETH_P_PPP_SES) && \
	 pppoe_proto(skb) == htons(PPP_IP))

#define IS_VLAN_PPPOE_IP(skb) \
        (vlan_proto(skb) == htons(ETH_P_PPP_SES) && \
         pppoe_vlan_proto(skb) == htons(PPP_IP))

#define IS_VLAN_IP(skb) \
    (vlan_proto(skb) == htons(ETH_P_IP))

#define IS_QINQ_PPPOE_IPV6(skb) \
	(vlan_proto(skb) == htons(ETH_P_8021Q) && 	\
 	 qinq_proto(skb) == htons(ETH_P_PPP_SES) && 	\
 	 pppoe_qinq_proto(skb) == htons(PPP_IPV6))

#define IS_QINQ_PPPOE_NEGO_IPV6(skb) \
	(vlan_proto(skb) == htons(ETH_P_8021Q) && 	\
 	 qinq_proto(skb) == htons(ETH_P_PPP_SES) && 	\
 	 pppoe_qinq_proto(skb) != htons(PPP_IPV6))

#define IS_QINQ_IPV6(skb) \
	(vlan_proto(skb) == htons(ETH_P_8021Q) && \
 	 qinq_proto(skb) == htons(ETH_P_IPV6))

#ifndef PAGE_SIZE
#define PAGE_SIZE	(4096)
#endif

static_always_inline unsigned long total_pages_get()
{
	FILE *fp = fopen("/proc/meminfo", "r");
	unsigned long total = 0;
	if (fp == NULL) 
	{
		total = 256000;
	}
	else
	{
		char buf[80];
		if (2 != fscanf(fp, "MemTotal: %lu %s\n", &total, buf))
		{
            total = 0;
		}
		fclose(fp);
		/*MemTotal:        3876496 kB*/
		total <<= 10;
	}
	return (total / PAGE_SIZE);
}
#define totalram_pages	(total_pages_get())
#define dev_put(x)	do{}while(0)
extern int g_net_ratelimit_count;
extern int g_net_ratelimit_per_second;
#define net_ratelimit()	((g_net_ratelimit_per_second && (g_net_ratelimit_count>0))?(g_net_ratelimit_count--):(0))
#define local_bh_disable() do{}while(0)
#define local_bh_enable() do{}while(0)

#define skb_is_gso(x)	1
typedef unsigned short sk_buff_data_t;
#endif//COMPILING_IN_VPP

struct ace_list_head {
    struct ace_list_head *next, *prev;
};

#define LIST_SEARCH(head, cmpfn, type, args...)     \
({                          \
    const struct ace_list_head *__i = (head);       \
                            \
    do {                        \
        __i = __i->next;            \
        if (__i == (head)) {            \
            __i = NULL;         \
            break;              \
        }                   \
    } while (!cmpfn((const type)__i , ## args));    \
    (type)__i;                  \
})

#ifndef static_always_inline
#define static_always_inline static inline
#endif

static_always_inline void ACE_INIT_LIST_HEAD(struct ace_list_head *list)
{
    list->next = list;
    list->prev = list;
}

#define LIST_EACH(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

static_always_inline void __ace_list_add(struct ace_list_head *newone,
                  struct ace_list_head *prev,
                  struct ace_list_head *next)
{
    next->prev = newone;
    newone->next = next;
    newone->prev = prev;
    prev->next = newone;
}

#define LIST_FOR_EACH_SAFE(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)


static_always_inline void LIST_ADD(struct ace_list_head *newone, struct ace_list_head *head)
{
    __ace_list_add(newone, head->prev, head);
}

static_always_inline void LIST_ADD_AFTER(struct ace_list_head *newone, struct ace_list_head *head)
{
    __ace_list_add(newone, head, head->next);
}

static_always_inline void __ace_list_del(struct ace_list_head * prev, struct ace_list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static_always_inline void LIST_DEL(struct ace_list_head *entry)
{
    __ace_list_del(entry->prev, entry->next);
    entry->next = 0;
    entry->prev = 0;
}

#define IPTK_CHECK_NAME_LEN       32
#define IPTK_TARGET_LEN           64
#define IPTK_TARGET_DNS_LEN       64
#define IPTK_MAX_TARGET_NUM       20    

#define ISPTRACK_TARGET_STRING_CFG_MAX (512)
#define ISPTRACK_STATIC_ROUTE_SUBNET_NUM (128)
#define ISPTRACK_STATIC_ROUTE_SUBNET_MAX (2048)

#define NMC_TITLE "NMC_"

#define MAX_MD5_LEN       64

extern unsigned int g_module_debug_flag1;

#define MODULE_DEBUG_FIREWALL                             0x00000001
#define MODULE_DEBUG_PFC                                  0x00000002
#define	MODULE_DEBUG_UFC                                  0x00000004
#define	MODULE_DEBUG_L7_QUEUE                             0x00000008
#define MODULE_DEBUG_AUTH                                 0x00000010
#define MODULE_DEBUG_L7_QUEUE_SNED                        0x00000020
#define MODULE_DEBUG_L7_QUEUE_COPY                        0x00000040
#define MODULE_DEBUG_SESSION_END                          0x00000080
#define MODULE_DEBUG_STATISTICS                           0x00000100
#define MODULE_DEBUG_FEATURE                              0x00000200
#define MODULE_DEBUG_HTTP                                 0x00000400
#define MODULE_DEBUG_SSL                                  0x00000800
#define MODULE_DEBUG_MAIL                                 0x00001000
#define MODULE_DEBUG_FTP                                  0x00002000
#define MODULE_DEBUG_BYPASS_SKIP_UDP                      0x00004000
#define MODULE_DEBUG_BYPASS_SKIP_RX                       0x00008000
#define MODULE_DEBUG_BYPASS_SKIP_NO80                     0x00010000
#define MODULE_DEBUG_BYPASS_SKIP_MONITOR                  0x00020000
#define MODULE_DEBUG_SKIP_MAIL_PROC                       0x00040000
#define MODULE_DEBUG_SKIP_MAIL_IMAP_PROC                  0x00080000
#define MODULE_DEBUG_SKIP_MAIL_POP3_PROC                  0x00100000
#define MODULE_DEBUG_SKIP_MAIL_SMTP_PROC                  0x00200000
#define MODULE_DEBUG_HOTEL_VLAN_ARP_RESPONSE              0x00400000
#define MODULE_DEBUG_PROUTE_TCP                           0x00800000
#define MODULE_DEBUG_HOTEL_LEARN_VLAN                     0x01000000
#define MODULE_DEBUG_HOTEL_HH_DMAC                        0x02000000
#define MODULE_DEBUG_HOTEL_DHCP_VLAN                      0x04000000
#define MODULE_DEBUG_IPV6_PROUTE_ISP_OPTIMUM              0x08000000
#define MODULE_DEBUG_IPV6_PROUTE_REPLACE_DST              0x10000000
#define MODULE_DEBUG_IPV6_PROUTE_STATISTICS               0x20000000
#define MODULE_DEBUG_CNT_STAT                             0x40000000
#define MODULE_DEBUG_ROUTE_MAIN_DIABLE                    0x80000000	

#define MODULE_DEBUG_CTL1(_ctrl) (g_module_debug_flag1 & _ctrl)

extern unsigned int g_module_debug_flag2;

#define MODULE_DEBUG_SKIP_CLASSIFY                        0x00000001
#define MODULE_DEBUG_SKIP_DO_MATCH_HTTP_RESPONSE          0x00000002
#define MODULE_DEBUG_SKIP_DO_MATCH_HTTP_REQUST            0x00000004
#define MODULE_DEBUG_SKIP_WM_HTTP_SEARCH                  0x00000008
#define MODULE_DEBUG_SKIP_DO_MATCH_SSL_RESPONSE           0x00000010
#define MODULE_DEBUG_SKIP_WM_SSL_SEARCH                   0x00000020
#define MODULE_DEBUG_SKIP_DO_MATCH_SSL_REQUEST            0x00000040
#define MODULE_DEBUG_SMB                                  0x00000080
#define MODULE_DEBUG_HYTF_REDIECT                         0x00000100
#define MODULE_DEBUG_SKIP_BRIDGE     0x200
#define MODULE_DEBUG_SKIP_PPPOE_SO     0x400
#define MODULE_DEBUG_SKIP_POLICY_ROUTE     0x800
#define MODULE_DEBUG_SKIP_ISP_OPTIMUM     0x1000
#define MODULE_DEBUG_SKIP_DST_NAT     0x2000
#define MODULE_DEBUG_SKIP_SRC_NAT     0x4000
#define MODULE_DEBUG_SKIP_MGT_WHITELIST     0x8000
#define MODULE_DEBUG_SKIP_QUEUE 0x10000
#define MODULE_DEBUG_SKIP_SSO_MIRROR 0x20000
#define MODULE_DEBUG_SKIP_SSO_JMS 0x40000
#define MODULE_DEBUG_SKIP_POST_ROUTE     0x80000
#define MODULE_DEBUG_SKIP_PRE_ROUTE     0x100000
#define MODULE_DEBUG_ONLY_BUILD_SESSION 0x200000
#define MODULE_DEBUG_SKIP_LICENSE_CAR   0x400000
#define MODULE_DEBUG_SKIP_CONFIRM       0x800000
#define MODULE_DEBUG_ONLY_FW_POLICY     0x1000000
#define MODULE_DEBUG_DO_DNAT_PKT     0x2000000
#define MODULE_DEBUG_SKIP_WM_HTTP_HIGH_SEARCH 0x4000000
#define MODULE_DEBUG_SKIP_COMMON_VM_SEARCH 0x8000000
#define MODULE_DEBUG_CLASSIFY_SIMU 0x10000000

#define MODULE_DEBUG_CTL2(_ctrl) (g_module_debug_flag2 & _ctrl)

#define CLOUD_SECURITY_KEY_LEN			64

/* defined in compilation parameters start */
//#define MANAGE_FRAME_SECURITY_NAME_LEN  512
//#define MANAGE_FRAME_SECURITY_ONE_LEN   128
/* defined in compilation parameters end */

#ifndef MANAGE_FRAME_SECURITY_NAME_LEN
#define MANAGE_FRAME_SECURITY_NAME_LEN 512
#endif
#ifndef MANAGE_FRAME_SECURITY_ONE_LEN
#define MANAGE_FRAME_SECURITY_ONE_LEN 128
#endif

#define MANAGE_FRAME_NAME_LEN           64
#define MANAGE_FRAME_PATH_LEN           512
#define MANAGE_FRAME_GROUP_USER_LEN     10240
#define MANAGE_FRAME_GROUP_USER_MAX     128
#define MANAGE_FRAME_BIND_TEXT_LEN      512
#define HYTF_COMMON_NAME_LEN            32
#define MANAGE_FRAME_SAVE_STRING_LEN    2048

#define USER_FINGERPRINT_NAME_LEN    	64
#define USER_FINGERPRINT_LEN			512
#define USER_FINGERPRINT_MAX_FINGER_NUM	2
#define USER_FINGERPRINT_MAX	        1000

#define HYLAB_IPDB_DATA_SHM_NAME   "/hylab_ipdb_data_shm"
#define HYLAB_IPDB_CITYCODE_SHM_NAME "/hylab_ipdb_citycode_shm"
#define HYTF_MAX_STRING_CONFIG_LEN  (3072)
#define HYTF_ADDRBOOK_IP_MAXLEN         3072
#define HYTF_ADDRBOOK_NAME_LEN          32
#define HYTF_ADDRBOOK_IP_GROUP_MAX_NUM  512
#define HYTF_USER_NL_RECV_MAX_LEN       65535
#define HYTF_USER_SOCKET_RECV_MAX_LEN   65535
#define HYTF_COUNTRY_NAME_LEN           64
#define HYTF_LONGITUDE_LATITUDE_LEN     32
#define HYTF_WLAN_STATION_WHITELIST_NUM 512
#define HYTF_WLAN_STATION_WHITELIST_LEN 10240

#define FW_SECURITY_PROTECT_POLICY_ANALYSE_IP_GROUP_MAX_NUM     (HYTF_ADDRBOOK_IP_GROUP_MAX_NUM * 2)
#define FW_SECURITY_PROTECT_POLICY_ANALYSE_IP_GROUP_MAX_STRING  (HYTF_ADDRBOOK_IP_MAXLEN * 2)

#define HYTF_DOMAIN_GROUP_MAXLEN         3072
#define HYTF_DOMAIN_GROUP_NAME_LEN       32
#define HYTF_DOMAIN_GROUP_MAX_NUM  512

#define DOMAIN_GROUP_DOMAIN_MAX_NUM      128
#define DOMAIN_GROUP_DOMAIN_MAX_LEN       64

#define LIGHT_DOMAIN_GROUP_MAX_HASH 0x10
#define LIGHT_DOMAIN_GROUP_MAX_MASK (LIGHT_DOMAIN_GROUP_MAX_HASH - 1)

#define DOMAIN_GROUP_MAX_HASH  0x100 /* 256 */
#define DOMAIN_GROUP_MAX_MASK  (DOMAIN_GROUP_MAX_HASH - 1)
#define DOMAIN_GROUP_HASH_ENTRY_MAX 16000


#define SERVER_INFO_STRING_LEN          1024

#define MANAGE_SAVE_CONFIG_FILE         "/home/config/current/manage_save_config.conf"

#define LDAP_INFO_DEBUG     0
#define MAX_SERVER_NAME_LEN         32
#define MAX_SERVER_DN_NAME_LEN      128
#define MAX_SERVER_SEARCH_DN_LEN      128
#define MAX_SERVER_DN_FILTER_LEN      1024
#define MAX_SERVER_GROUP_FILTER_NUM   12

#define MAX_ACCESS_DEVICE_LIST	256

#define MAX_BIG_MSG_BLOCK_NUM 16
#define MAX_BIG_MSG_BLOCK_SIZE 65536

#define MAX_TERMINAL_NAME_LEN 32

#define MAX_SHARE_GEOIP_NUM_MACRO             256
#define MAX_SHARE_ADDRBOOK_NUM_MACRO          512
#define MAX_SHARE_ADDRBOOK_NUMBER_MACRO       (MAX_SHARE_ADDRBOOK_NUM_MACRO+MAX_SHARE_GEOIP_NUM_MACRO)

typedef enum tag_msgq_type_t
{
	MSGQ_TYPE_HA_SWITCH = 0,
    MSGQ_TYPE_HTTP_URL = 1,
    MSGQ_TYPE_HTTP_SEARCH,
    MSGQ_TYPE_EMAIL,
    MSGQ_TYPE_IM,
    MSGQ_TYPE_HTTP_BBS,
    
    MSGQ_TYPE_HTTP_WEBMAIL,//6
    MSGQ_TYPE_HTTP_UPLOAD_FILE,
    MSGQ_TYPE_FTP,
    MSGQ_TYPE_EVENT,
    MSGQ_TYPE_AUTH,//10
    
    MSGQ_TYPE_BLACKLIST,
    MSGQ_TYPE_DATA_DELETE,
    MSGQ_TYPE_TELNET,
    MSGQ_TYPE_SECURITY,
    MSGQ_TYPE_HTTP_CONTENT_FILTER,//15
    
    MSGQ_TYPE_HTTP_LOGIN,
    MSGQ_TYPE_HTTP_SEARCH_CENTER,
    MSGQ_TYPE_HTTP_BBS_CENTER,
    MSGQ_TYPE_IM_CENTER,
    MSGQ_TYPE_HTTP_WEBMAIL_CENTER,//20
    
    MSGQ_TYPE_EMAIL_CENTER,
    MSGQ_TYPE_HTTP_POST,
    MSGQ_TYPE_SQL_SERVER,
    MSGQ_TYPE_SHAREIP_LOG,
    MSGQ_TYPE_SHAREIP_TO_BK_LOG,//25
    
    MSGQ_TYPE_IPS,
    MSGQ_TYPE_AV,
    MSGQ_TYPE_WAF,//28
    MSGQ_TYPE_SQUID_HTTPS_WEBMAIL_DROP,
    MSGQ_TYPE_PPPOE_LOG,

    MSGQ_TYPE_HTTP_DOWNLOAD_FILE,//31
    MSGQ_TYPE_NETWORK_ZONE,	
	MSGQ_TYPE_SMB,
	MSGQ_TYPE_ACCESS_RULE,
	MSGQ_TYPE_SSL_CONNTENT_FILTER,

	MSGQ_TYPE_EXTERNEL_CONN,//36
	MSGQ_TYPE_VPN_ABROAD_LOG,
    MSGQ_TYPE_WPP,
    MSGQ_TYPE_EG_NAT_LOG,
    MSGQ_TYPE_EG_URL_LOG,
    
    MSGQ_TYPE_EG_IM_LOG,//41
    MSGQ_TYPE_EG_BBS_LOG,
    MSGQ_TYPE_EG_MAIL_LOG,
    MSGQ_TYPE_EG_SEARCH_LOG,
    MSGQ_TYPE_EG_FILE_HTTP_URL,
 
    MSGQ_TYPE_EG_FILE_HTTP_SEARCH,//46
    MSGQ_TYPE_EG_FILE_EMAIL,
    MSGQ_TYPE_EG_FILE_HTTP_BBS,
    MSGQ_TYPE_EG_FILE_AUTH,
    MSGQ_TYPE_EG_FILE_HTTP_LOGIN,

#ifdef CONFIG_NSAS
	MSGQ_TYPE_NSAS_MAIL_LOG,
    MSGQ_TYPE_NSAS_TFTP_LOG,
    MSGQ_TYPE_NSAS_SMB_LOG,
    MSGQ_TYPE_NSAS_NFS_LOG,
    MSGQ_TYPE_NSAS_DB_LOG,
#endif

	MSGQ_TYPE_ILLEGAL_OUTREACH_JS,

#ifdef CONFIG_IFW
	MSGQ_TYPE_COMMAND,
#endif

    MSGQ_TYPE_MAX
}msgq_type_t;

enum _auth_policy_info_tag
{
    AUTH_POLICY_SERVER_COUNT,
    AUTH_POLICY_SERVER_TYPE,
    AUTH_POLICY_SERVER_IP,
    AUTH_POLICY_SERVER_PORT,
    AUTH_POLICY_SERVER_SECRET,
    
    AUTH_POLICY_SERVER_DNS,
    AUTH_POLICY_SERVER_DN,
    AUTH_POLICY_SERVER_CN,
    AUTH_POLICY_SERVER_DOMAIN,
    AUTH_POLICY_SERVER_USER,
    
    AUTH_POLICY_SERVER_PASSWORD,
    AUTH_POLICY_TYPE_LOCAL,//11
    AUTH_POLICY_TYPE_RADIUS,
    AUTH_POLICY_TYPE_LDAP,
    AUTH_POLICY_TYPE_POP3,
    
    AUTH_POLICY_TYPE_AD,
    AUTH_POLICY_TYPE_NONE,
    AUTH_POLICY_SERVER_NAME,
    AUTH_POLICY_SEARCH_DN,
	AUTH_POLICY_SEARCH_PWD,
	AUTH_POLICY_SSL_TLS,
	AUTH_POLICY_CHK_SVR_CERT,//21
    
    AUTH_ACCT_SERVER_NAME,
    AUTH_RADIUS_SERVER_ID,
    AUTH_POLICY_TYPE_HENGBANG,
    AUTH_POLICY_SERVER_URL,
    AUTH_POLICY_SERVER_SSL,
    
    AUTH_POLICY_TYPE_PAIBO,
    AUTH_RADIUS_GROUPATTR_ID,
    AUTH_POLICY_TYPE_ZHIAN,
    AUTH_POLICY_TYPE_PORTAL,
    AUTH_POLICY_SERVER_ACCT_PORT,//30
    
    AUTH_POLICY_TYPE_DATABASE,
    AUTH_POLICY_TYPE_SMP,
    AUTH_POLICY_TYPE_SSO_SAM,
    AUTH_POLICY_TYPE_CUSTOM,
    AUTH_POLICY_TYPE_SSO_MCP,
    
    AUTH_POLICY_TYPE_DDI,    //nouse
    AUTH_POLICY_SERVER_PORTAL_IP,
    AUTH_POLICY_SERVER_PORTAL_SECRET,
    AUTH_POLICY_AC_IP,
    AUTH_POLICY_TYPE_SSO_BYZORO,//40
    
    AUTH_POLICY_TYPE_SSO_AWIFI,
    AUTH_POLICY_TYPE_SSO_RADIUS,
    AUTH_POLICY_TYPE_SSO_METRO,
    AUTH_POLICY_TYPE_SSO_AD,
    AUTH_POLICY_TYPE_SSO_CITYHOTSPOT,
    
    AUTH_POLICY_TYPE_SSO_H3CMIC,
    AUTH_POLICY_TYPE_SSO_PPPOE,
    AUTH_POLICY_TYPE_SSO_PROXY,
    AUTH_POLICY_TYPE_SSO_HTTPLINK,
    AUTH_POLICY_TYPE_SSO_SMART_CARD,

    AUTH_POLICY_TYPE_SSO_HANTING,
    AUTH_POLICY_TYPE_SSO_JMS,
    AUTH_POLICY_TYPE_SSO_CSPESP,
    AUTH_POLICY_TYPE_VISITOR,
    AUTH_POLICY_TYPE_SSO_DB,

    AUTH_POLICY_TYPE_SMP_2009,
    AUTH_POLICY_TYPE_SNMP,
    AUTH_POLICY_TYPE_SYSLOG,
    AUTH_POLICY_TYPE_DINGDING,
    AUTH_POLICY_TYPE_SSO_TSM,//60

    AUTH_POLICY_PORTAL_AUTO_GROUP,
    AUTH_POLICY_TYPE_DKEY,
    AUTH_POLICY_TYPE_OUTER_GENERIC,
    AUTH_POLICY_MULTIPLE_BIT_MAP,
    AUTH_POLICY_TYPE_WIFIDOG_BIGUIYUAN,
	AUTH_POLICY_IMPORT_GROUP_NAME,
	AUTH_POLICY_THIRD_INTERFACE,

	AUTH_POLICY_DOUBLE_FACTOR,
	
    AUTH_POLICY_TYPE_SSO_5GAAA,
    AUTH_POLICY_TYPE_SSO_NCE_CAMPUS,
};

enum HA_USER_TYPE
{
    HA_USER_TYPE_NO_CARE = 0,
    HA_USER_TYPE_AUTH,
    HA_USER_TYPE_SSO
};  


enum HTTP_TYPE{
    HTTP_TYPE_GET=1,
    HTTP_TYPE_POST,
    HTTP_TYPE_RESPONSE_GET,
    HTTP_TYPE_RESPONSE_POST,
    HTTP_TYPE_MAX
};  

enum HTTP_APP_TYPE
{
    HTTP_APP_TYPE_SUMMARTY=-1,
    HTTP_APP_TYPE_SEARCH=0, 
    HTTP_APP_TYPE_BBS,
    HTTP_APP_TYPE_CUSTOM_BBS,   
    HTTP_APP_TYPE_WEBMAIL,
    HTTP_APP_TYPE_UPLOAD,   
    HTTP_APP_TYPE_LOGIN,
    HTTP_APP_TYPE_PROXY,
    HTTP_APP_TYPE_FILETYPE,
    
    HTTP_APP_TYPE_MAX
};

/*HTTP log msg define*/
#define MAX_ENGINEER_HOST_LEN 64
#define MAX_ENGINEER_METHOD_LEN 128
#define MAX_ENGINEER_KEY_LEN 128
#define MAX_ENGINEER_CHARSET_LEN 16
#define MAX_ENGINEER_PATTERN_SHORT_LEN 64
#define MAX_ENGINEER_PATTERN_LONG_LEN 128

#define HTTP_FILETYPE_LEN   16
#define DGDIAN_HTTP_URL_LEN         512
#define DGDIAN_HTTP_HOST_LEN        64
#define DGDIAN_HTTP_DOMAIN_LEN      48
#define DGDIAN_HTTP_CATEGORY_LEN      32
#define DGDIAN_HTTP_TITILE_LEN        256
#define DGDIAN_HTTP_CHARSET_LEN   16
#define DGDIAN_HTTP_KEYWORD_LEN     1024 
#define KW_GROUP_NAME_LEN 32
#define FILE_TYPE_NAME_LEN 32
#define DGDIAN_HTTP_POST_LEN DGDIAN_HTTP_TITILE_LEN*7-2


#define BBS_CHARSET_LEN  20
#define BBS_SUBJECT_LEN  512
#define BBS_CONTENT_LEN     4096
#define BBS_FILENAME_LEN  512

#define WEBMAIL_ID_LEN 128
#define WEBMAIL_USER_LEN 64
#define WEBMAIL_DATE_LEN 64
#define WEBMAIL_FROM_LEN            512
#define WEBMAIL_TO_LEN              1024
#define WEBMAIL_SUBJECT_LEN     512
#define WEBMAIL_FILENAME_LEN    512 
#define WEBMAIL_CONTENT_LEN     MAX_BIG_MSG_BLOCK_SIZE

#define MSG_UPLOAD_FILE_LEN MANAGE_FRAME_SECURITY_NAME_LEN
#define MSG_UPLOAD_FILE_UID_LEN 1024

#define LOGIN_HOST_LEN 128
#define LOGIN_USER_LEN 64
#define LOGIN_PWD_LEN 64

typedef enum tag_email_protocol_e{
    EMAIL_PROTOCOL_POP3,
    EMAIL_PROTOCOL_SMTP,
    EMAIL_PROTOCOL_IMAP,
}email_protocol_e;

#define EMAIL_USER_LEN          64
#define EMAIL_FROM_LEN          512
#define EMAIL_TO_LEN            512
#define EMAIL_DATE_LEN          64
#define EMAIL_SUBJECT_LEN       512
#define MAX_MAIL_CONTENT_LEN    MAX_BIG_MSG_BLOCK_SIZE 
#define MAX_ATTACH_NAME_LEN     512

#if defined(CONFIG_NSAS)
#define NSAS_EMAIL_FROM_LEN          256
#define NSAS_EMAIL_TO_LEN            256
#define NSAS_EMAIL_Cc_LEN            256
#define NSAS_EMAIL_Bcc_LEN           256
#define NSAS_EMAIL_SUBJECT_LEN       1024
#define NSAS_EMAIL_CONTENT_LEN       1024
#define NSAS_EMAIL_CONTENT_BUF_LEN   2048
#endif

/*IM log msg define*/
#define MAX_TF_NAME_LEN         64  // MSN 的e-mail 地址比较长, 64 不要更改，不然要更改REPORT_TRAFFIC_IMP2P_SELFID_LENGTH 等定义
#define MAX_TF_TYPE_NAME_LEN        32  // "msn", "qq"
#define MAX_TF_CLASS_NAME_LEN       16  // "im", "p2p", "proxy", "malware"
#define MAX_TF_CHAT_LEN 256

#define FTP_USER_MAX_LEN 64
#define FTP_VALUE_MAX_LEN   256

#define TELNET_VALUE_MAX_LEN 64
#define TELNET_COMMAND_MAX_LEN 512
#define TELNET_RESULT_MAX_LEN 4096

/* ruijie, EG to UAC log-manage Module */
#define EG_AUTH_ACCOUNT_LEN                32
#define EG_NETWORK_APP_LEN                 8
#define EG_ENGINE_TYPE_LEN                 16
#define EG_KEYWORD_LEN                     64
#define EG_URL_LEN                         256
#define EG_MAC_LEN                         20
#define EG_AP_MAC_LEN                      20
#define EG_SESSIONID_LEN                   48
#define EG_CAPTURE_TIME_LEN                12
#define EG_COLLECTION_EQUIPMENTID_LEN      32
#define EG_NETBAR_WACODE_LEN               32
#define EG_IP_ADDRESS_LEN                  20
#define EG_PORT_LEN                        8
#define EG_SRC_IP_LEN                      20
#define EG_SRC_PORT_START_LEN              8
#define EG_SRC_PORT_END_LEN                8
#define EG_DST_IP_LEN                      20
#define EG_DST_PORT_LEN                    8
#define EG_LAYER3PROTOCOL_LEN              8
#define EG_AUTH_TYPE_LEN                   24
#define EG_HTTP_TYPE_LEN                   24
#define EG_HOST_LEN                        64
#define EG_TITLE_LEN                       128
#define EG_CONTENT_TYPE_LEN                64
#define EG_USERNAME_LEN                    64
#define EG_PASSWORD_LEN                    64
#define EG_REFERER_LEN                     32
#define EG_MAINFILE_LEN                    256
#define EG_FILESIZE_LEN                    12
#define EG_FILE_MD5_LEN                    32
#define EG_OTHER_FILE_LEN                  128
#define EG_OTHER_FILE_MD5_LEN              32
#define EG_SRC_IPV6_LEN                    64
#define EG_SRC_PORT_START_V6_LEN           8
#define EG_SRC_PORT_END_V6_LEN             8
#define EG_DST_IPV6_LEN                    64
#define EG_DST_PORT_V6_LEN                 8
#define EG_EMAIL_TYPE_LEN                  64
#define EG_MAIL_FROM_LEN                   128
#define EG_RCPT_TO_LEN                     512
#define EG_MAIL_SUBJECT_LEN                128
#define EG_ACTION_LEN                      24
#define EG_CC_LEN                          256
#define EG_BCC_LEN                         256
#define EG_MAIN_TEXT_LENGTH_LEN            12
#define EG_MAIL_SEND_TIME_LEN              12
#define EG_WEBBBS_TYPE_LEN                 24
#define EG_DOMAIN_LEN                      64
#define EG_BBS_TOPIC_LEN                   24
#define EG_CONTENT_LEN                     12
#define EG_NETSITE_TYPE_LEN                24
#define EG_CERTIFICATE_TYPE_LEN            12
#define EG_CERTIFICATE_CODE_LEN            24
#define EG_START_TIME_LEN                  12
#define EG_END_TIME_LEN                    12
#define EG_LONGITUDE_LEN                   12
#define EG_LATITUDE_LEN                    12
#define EG_TERMINAL_FIED_STRENGTH_LEN      12
#define EG_X_COORDINATE_LEN                24
#define EG_Y_COORDINATE_LEN                24
#define EG_COLLECTION_EQUIPMENT_ID_LEN     32
#define EG_SESSION_ID_LEN                  64
#define EG_SERVICE_CODE_LEN                32
#define EG_ACCOUNT_TYPE_LEN                12
#define EG_ACCOUNT_LEN                     64
#define EG_ACTION_TYPE_LEN                 32
#define EG_MAC_ADDRESS_LEN                 20
#define EG_NETSERVERPORT_WACODE_LEN        32
#define EG_APP_NAME_LEN                    32
/* ruijie, EG to UAC log-manage Module */

enum tag_nlmsg_type_t
{
    REPORTER_NLMSG_TYPE_FTP_LOG,
    REPORTER_NLMSG_TYPE_EVENT,
    REPORTER_NLMSG_TYPE_DROP_FIREWALL,
    REPORTER_NLMSG_TYPE_DROP_POLICY_FC,
    REPORTER_NLMSG_TYPE_DROP_BEHAVIOR_MANAGE,
    
    REPORTER_NLMSG_TYPE_RLM_IP_CLEAR,
    REPORTER_NLMSG_TYPE_URL_LOG,//6
    REPORTER_NLMSG_TYPE_TITLE_LOG,
    REPORTER_NLMSG_TYPE_EMAIL_LOG,
    REPORTER_NLMSG_TYPE_POP3_LOGIN_LOG,
    
    REPORTER_NLMSG_TYPE_SHAREIP_LOG,//10
    REPORTER_NLMSG_TYPE_DROP_CLOUD_SECURITY_IP,
    REPORTER_NLMSG_TYPE_DROP_CLOUD_SECURITY_URL,
    REPORTER_NLMSG_TYPE_RLM_QOS_FLOW,
    REPORTER_NLMSG_TYPE_HTTP_DOWNLOAD_LOG,
    
    REPORTER_NLMSG_TYPE_DROP_MANAGE,
    REPORTER_NLMSG_TYPE_AD_LOG,//16
    REPORTER_NLMSG_TYPE_ATBL_LOG,
    REPORTER_NLMSG_BR_LOOP_WARN_LOG,

    REPORTER_NLMSG_TI_LOG, //21
    REPORTER_NLMSG_WPP_LOG,
    REPORTER_NLMSG_IOP_LOG,
    REPORTER_NLMSG_IOP_LEARN_LOG,

	REPORTER_NLMSG_DNS_CONTROL_LOG,
    
    REPORTER_NLMSG_TYPE_DROP_RISK_ANALYSIS,//26
    REPORTER_NLMSG_TYPE_DROP_BLKLIST_FILTER,

    REPORTER_NLMSG_TYPE_DROP_GEO_ACCCESS_CONTROL_POLICY,
    REPORTER_NLMSG_TYPE_DROP_HOST_ACCESS_CONTROL,
    REPORTER_NLMSG_TYPE_SSLVPN,
    REPORTER_NLMSG_INDUSTRIAL_CONTROL_SECURITY_LOG,//31
    REPORTER_NLMSG_INDUSTRIAL_CONTROL_AUDIT_LOG,

    REPORTER_NLMSG_TYPE_VPN_ABROAD,
    REPORTER_NLMSG_TYPE_360_ABROAD,
    REPORTER_NLMSG_TYPE_L2_DROP,
    
    REPORTER_NLMSG_TYPE_LOG_MAX,   
    REPORTER_NLMSG_TYPE_SESSION_FLOW,//unused
    REPORTER_NLMSG_TYPE_SESSION_END,//unused
    REPORTER_NLMSG_TYPE_RLM_IP_IN,//unused
    
    REPORTER_NLMSG_TYPE_RLM_IP_FLOW,//unused
    REPORTER_NLMSG_TYPE_RLM_IP_OUT,//unused
    REPORTER_NLMSG_TYPE_RLM_SERVICE_IN,//unused
    REPORTER_NLMSG_TYPE_RLM_SERVICE_FLOW,//unused
    REPORTER_NLMSG_TYPE_RLM_SERVICE_OUT,//unused
    
    REPORTER_NLMSG_TYPE_RLM_USER_IN,//unused
    REPORTER_NLMSG_TYPE_RLM_USER_OUT,//unused
    REPORTER_NLMSG_TYPE_IPV6_FLOW,//unused
    REPORTER_NLMSG_TYPE_IPV6_END,//unused
    REPORTER_NLMSG_TYPE_IPV6_FW_DROP,//unused
    
    REPORTER_NLMSG_TYPE_URL_IP6_LOG,//unused
    REPORTER_NLMSG_TYPE_TITLE_IP6_LOG,//unused
    REPORTER_NLMSG_TYPE_EMAIL_IP6_LOG,//unused
    REPORTER_NLMSG_TYPE_FTP_IP6_LOG,//unused

#ifdef CONFIG_NSAS
	REPORTER_NLMSG_NSAS_HTTP_LOG,
	REPORTER_NLMSG_NSAS_FTP_LOG,
	REPORTER_NLMSG_NSAS_DNS_LOG,
	REPORTER_NLMSG_NSAS_SSL_LOG,
	REPORTER_NLMSG_NSAS_RADIUS_LOG,
	REPORTER_NLMSG_NSAS_SSH_LOG,
	REPORTER_NLMSG_NSAS_LDAP_LOG,
	REPORTER_NLMSG_NSAS_ADDOMAIN_LOG,
	REPORTER_NLMSG_NSAS_TELNET_LOG,
	REPORTER_NLMSG_NSAS_RDP_LOG,
	REPORTER_NLMSG_NSAS_NETBIOS_NS_LOG,
	REPORTER_NLMSG_NSAS_NETBIOS_DS_LOG,
	REPORTER_NLMSG_NSAS_RLOGIN_LOG,
	REPORTER_NLMSG_NSAS_NFS_LOG,
	REPORTER_NLMSG_NSAS_SYSLOG_LOG,
	REPORTER_NLMSG_NSAS_EMAIL_LOG,
	REPORTER_NLMSG_NSAS_DB_LOG,
	REPORTER_NLMSG_NSAS_SNMP_LOG,
#endif
#ifdef CONFIG_DAS
    REPORTER_NLMSG_TYPE_DB_LOG,
#endif
#ifdef CONFIG_SIG
	REPORTER_NLMSG_TYPE_SIG_LOG,
#endif

    REPORTER_NLMSG_TYPE_DROP_IPMAC,
};

enum tag_shareip_type_t
{
    SHAREIP_TYPE_OS_NOTIFY,
    SHAREIP_TYPE_WECHAT_NOTIFY,
    SHAREIP_TYPE_MOBILE_NOTIFY,
    SHAREIP_TYPE_COOKIE_NOTIFY,
    SHAREIP_TYPE_GPS_NOTIFY,
    SHAREIP_TYPE_SEX_AND_AGE_NOTIFY,
    SHAREIP_TYPE_FIND_ANTI_ANTI_SHARE_ROUTER_NOTIFY,
    SHAREIP_TYPE_FIND_360WIFI,
    SHAREIP_TYPE_NTP_SERVER,
    SHAREIP_TYPE_WECHAT_LONG_CONNECTION,
    SHAREIP_TYPE_YXLM_LONG_CONNECTION,
    SHAREIP_TYPE_JDQS_LONG_CONNECTION,
    SHAREIP_TYPE_YUANSHEN_LONG_CONNECTION,
    SHAREIP_TYPE_FIND_HIWIFI,
    SHAREIP_TYPE_VIRTUAL_ID,
    SHAREIP_TYPE_FIND_UNKNOWN_ROUTER,
};

enum industrial_control_event_level
{
	INDUSTRIAL_CONTROL_RISK_NO,
	INDUSTRIAL_CONTROL_RISK_LOW,
	INDUSTRIAL_CONTROL_RISK_MID,
	INDUSTRIAL_CONTROL_RISK_HIGH,
	INDUSTRIAL_CONTROL_RISK_PANIC,
};

#define REPORTER_THREAD_POOLS_MIN_CPU 4

#define WECHAT_SCAN_HOST_STRING						"open.weixin.qq.com"
#define WECHAT_SCAN_URL_STRING						"http://open.weixin.qq.com/wechat_scanning.htm"//"http://mp.weixin.qq.com/wechat_scanning.htm"

//自定义BBS custom 特征文件
#define MAX_CUSTOM_BBS_ENGINEER_NUM  100

//HTTP_POST 数据记录最大长度
#define MSG_HTTP_POST_DATA_LEN   512

#define MSG_SQL_QUERY_DATA_LEN   1024

#define MAX_GAME_DOMAIN_NUM  2048
#define MAX_XUNLEI_DOMAIN_NUM  2048


enum CHARSET_TYPE{
    CHARSET_UTF_8=0,
    CHARSET_GBK,
    CHARSET_GB2312,
    CHARSET_BIG5,   
    CHARSET_TYPE_MAX,
    CHARSET_US_ASCII
};  

#define     FTP_CONTROL_PROTOCOL 1036

#define     MAIL_POP3_PROTOCOL 1033
#define     MAIL_IMAP_PROTOCOL 1035
#define MAIL_SMTP_PROTOCOL 1032

#define MAX_BBS_PATTERN_ITEMS 10000 /* 1,im_mark, whitelist_type */
#define isWebIM( id )  ( id > MAX_BBS_PATTERN_ITEMS )

typedef enum ip_version
{   
    IP_V4,
    IP_V6,
    IP_V4V6
}IP_VERSION;

typedef enum if_family
{
    IF_PHY = 0,
    IF_SUB = 1,
    IF_BOND = 2,
    IF_GRE = 3,
    IF_PPTP_CLIENT = 4,
    
    IF_PPTP_SERVER = 5,
    IF_L2TP_SERVER = 6,
    IF_PPPOE_CLIENT = 7,
    IF_SSL_SERVER = 8,
    IF_BR = 9,
    
    IF_IPSEC_VPN = 10,
    IF_PPPOE_MULI = 11,
    IF_VLANIF = 12,
    IF_VLANIF_SUB = 13,
}IF_FAMILY;

#define MSGQ_HEAD_LEN sizeof(long) /*linux msgq head len*/
#define NL_HEAD_LEN sizeof(struct nlmsghdr) /*linux netlink msg head len*/

#define MAX_HYLAB_SAFE_ZONE_NUM 4096
#define HYLAB_POLICY_MAP_MAX 1024

#define COMMON_USERNAME_LEN_MAX 32
#define COMMON_PASSWORD_LEN_MAX 32
#define COMMON_NAME_LEN_MAX 32
#define COMMON_DESC_LEN_MAX 256
#define COMMON_PATH_LEN_MAX 256

#define COMMON_DESC_LEN_256 256 // should only used for comment

#define COMMON_256_LEN      256
#define COMMON_512_LEN      512

#define MAX_HTTP_VIRUS_URL_HASH 32

#define MAC_STR_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_STR_LEN_MAX 24 //17+7(for align)
#define IP_STR_LEN_MAX 16 //15+1(for align)
#define IP_RANGE_STR_LEN_MAX 40 //34+6(for align)
#define IPV6_STR_LEN_MAX 40 //39+1(for align)
#define IPV6_RANGE_STR_LEN_MAX 88//83+5(for align)
#define PORT_RANGE_STR_LEN_MAX 16//11+5(for align)

#define IF_NAME_LEN_MAX 16
#define IP_STR_NUM_MAX 16
#define VLAN_RANGE_STR_LEN_MAX 16
#define ZONE_IF_NUM_MAX 64
#define PHY_IF_NUM_MAX 32
#define WAF_RULE_NUM_MAX 64
#define WAF_MAX_REF_SERVER_NUM 32
#define WAF_HTTP_HEADER_OVERFLOW_NUM_MAX 32
#define WAF_CC_URL_LIMIT_MAX_LEN 128
#define WAF_CC_URL_LIMIT_MAX_NUM 128
#define WAF_CC_URL_LIMIT_MAX_STRING (WAF_CC_URL_LIMIT_MAX_LEN * WAF_CC_URL_LIMIT_MAX_NUM + 2 * WAF_CC_URL_LIMIT_MAX_NUM) // extra \r\n at the end of each line
#define WAF_KEY_MAX_LEN 128
#define WAF_HOTLINK_PROTECT_CONTENT_LEN (4096 + 256)

#define WAF_SERVER_NUM_MAX 64
#define WAF_LISTENING_START_PORT 2998
#define WAF_LISTENING_PORT_NUM WAF_SERVER_NUM_MAX * 2
#define WAF_LISTENING_END_PORT (WAF_LISTENING_START_PORT + WAF_LISTENING_PORT_NUM)

#define WAF_MAX_DOMAIN_NUM 16
#define WAF_MAX_PUBLIC_IP_NUM 16
#define WAF_MAX_CERT_PATH_LEN 256
#define WAF_MAX_IP_LEN 64
#define WAF_MAX_NAME_LEN 64
#define WAF_MAX_CRS_NUM 32

#define WAF_MAX_SERVER_GROUP 8
#define WAF_MAX_OUTER_IP_GROUP 8
#define WAF_PORT_RANGE_LEN 16
#define VLANID_MAX_NUMBER 128

#define WAF_LOW_RISK_SCORE      1
#define WAF_MIDDLE_RISK_SCORE   4
#define WAF_HIGH_RISK_SCORE     5

typedef enum{
    WAF_MODE_REVERSE_PROXY = 0, // client access local ip in L3 mode, but no dnat exist. also comment 'proxy_bind $remote_addr transparent' in the config file
    WAF_MODE_TRANSPARENT_PROXY = 1, // the traditional mode, transparent proxy in L2/L3 mode
    WAF_MODE_TRANSPARENT_BRIDGE = 2, // parse the http plain packet in l2/l3 mode, no proxy 
    WAF_MODE_BYPASS = 3 // parse the http plain packet in bypass mode, no proxy 
}WAF_MODE;

typedef enum waf_hotlink_protect_mode_eum
{
	HOTLINK_PROTECT_WHOLE_SITE = 0,
    HOTLINK_PROTECT_FILE_TYPE = 1,
	HOTLINK_PROTECT_MAX	
}waf_hotlink_protect_mode_eum;



#define CHKSUM_LENGTH 32
#define MAX_SERVICE_TYPE 1024 /* 1024*8 = 8192 种*/
#define MAX_SERVICE_TYPE_NUM (MAX_SERVICE_TYPE * 8)
#define MAX_SERVICE_TYPE_DIV_32 (MAX_SERVICE_TYPE / 4)
#define ACE_MAX_USER_SERVICE_NUM 3
#define MAX_SERVICE_GROUP_NUM   16
#define MAX_JSON_CONFIG_SIZE	2048

#define MAX_SAAS_LEVEL_ID_BIT 9
#define MAX_SAAS_LEVEL_NUM 3

#define MAX_SAAS_LEVEL_ID_MASK ((1LL << MAX_SAAS_LEVEL_ID_BIT) - 1)
#define MAX_SAAS_LEVEL_TYPE (1LL << (MAX_SAAS_LEVEL_ID_BIT - 3))
#define MAX_SAAS_LEVEL_TYPE_NUM (1LL << MAX_SAAS_LEVEL_ID_BIT)

#define MAX_SAAS_LEVEL_ID_BIT_OFFSET(_level) (MAX_SAAS_LEVEL_ID_BIT * (MAX_SAAS_LEVEL_NUM - 1 - (_level)))
#define MAX_SAAS_LEVEL_TYPE_OFFSET(_level) (MAX_SAAS_LEVEL_TYPE * (_level))
#define MAX_SAAS_LEVEL_TYPE_NUM_OFFSET(_level) (MAX_SAAS_LEVEL_TYPE_NUM * (_level))

#define MAX_SAAS_TYPE (MAX_SAAS_LEVEL_TYPE * MAX_SAAS_LEVEL_NUM)
#define MAX_SAAS_TYPE_NUM (MAX_SAAS_LEVEL_TYPE_NUM * MAX_SAAS_LEVEL_NUM)

#define COMMON_ITEM_MAX 50
#define SIZE_32KB_IN_4_BYTES 8192   //(32 * 1024) / 4
#define SIZE_10MB_IN_4_BYTES 327680 // (10 * 1024 * 1024) / 8 / 4
#define SIZE_10MB_BIT SIZE_10MB_IN_4_BYTES * 4 * 8
//#define AV_CRS_CNT_MAX 600000//60000

#define L7DPI_AV_FILE_PATH "/mnt/reporter/store/fwlog/log_files/"
#define L7DPI_AV_FILE_SIZE 20000000 //20M


#define SCAN_HTTP 0x08
#define SCAN_FTP  0x04
#define SCAN_POP3 0x02
#define SCAN_SMTP 0x01
#define SCAN_IMAP 0x10
#define SCAN_NFS  0x20

typedef enum attack_log_type_eum
{
	ATTACK_LOG_DOS,
	ATTACK_LOG_IPS,
	ATTACK_LOG_AV,
	ATTACK_LOG_WAF,
	ATTACK_LOG_TYPE_MAX	
}attack_log_type_eum;

#define IPS_GROUP_NUM_MAX 48
#define IPS_GROUP_NAME_LEN 64
#define IPS_LEVEL_NODE 24
typedef enum ips_level_eum
{
	IPS_LEVEL_INFO,
	IPS_LEVEL_LOW,
	IPS_LEVEL_MID,
	IPS_LEVEL_HIGH,
    IPS_LEVEL_FATAL,
	IPS_LEVEL_MAX
}ips_level_eum;

typedef enum session_mark_type
{
    SESSION_MARK_FTP_UPLOAD = 100,
    SESSION_MARK_FTP_DOWNLOAD = 101,
}SESSION_MARK_TYPE;

#if defined(ARCH_ARM64) && defined(CS_M3720_PLATFORM)
#define MAX_CAR_TUNNEL_NUM         (ARM64_CAR_TUNNEL_NUM + 1)  /*link car , 系统最大线路数目*/
#else
#define MAX_CAR_TUNNEL_NUM         (NORMAL_CAR_TUNNEL_NUM + 1)  /*link car , 系统最大线路数目*/
#endif

/* PROXY SSO define*/
#define PROXY_SSO_TMG 0x01
#define PROXY_SSO_HTTP_BASIC 0x02

#define HA_LINK_STATUS_PATH "/tmp/.keepalived"

#define MAX_SQL_ONE_RECORD	2048

//threat intelligence
#define TI_MAX_IP_CONFIG_NUM 128
#define TI_MAX_DOMAIN_CONFIG_NUM 64
#define TI_MAX_DOMAIN_LEN 256
#define TI_MAX_URL_CONFIG_NUM 64
#define TI_MAX_URL_LEN 512
#define TI_MAX_IP_STRING 64
#define TI_MAX_GEO_STRING 64

#define TI_CUSTOM_RULE_MAX_DOMAIN_STRING 256
#define TI_CUSTOM_RULE_MAX_QEURY_NUM 50

#define TI_CATEGORY_INDEX_MAX 32 // custom and predefine both have seperate 32 index
#define TI_CATEGORY_NAME_ALL_MAX 256 // include more than one category in predefined rules

//dns control
#define DNS_CONTROL_MAX_IP_CONFIG_NUM 128
#define DNS_CONTROL_MAX_DOMAIN_CONFIG_NUM 64
#define DNS_CONTROL_MAX_DOMAIN_LEN 256

#define MAC_FILTER_ENTRY_MAX 64
#define MAC_STR_NUM_MAX 256
#define MAC_FILTER_STRING_MAX (MAC_STR_LEN_MAX * MAC_STR_NUM_MAX)

//weak passwd protect
#define WPP_WEB_SERVER_URL_LEN 128
#define WPP_REGEX_EXPRESSION_LEN 128
#define WPP_CUSTOM_RULE_LIST_MAX 100
#define WPP_INNER_RULE_LIST_MAX 100
#define WPP_RULE_LEN_MAX 32
#define WPP_WEB_PRAMS_MAX 128
#define WPP_INNER_RULE_FILE   "/home/config/cfg-scripts/weak_passwd_inner_rule"
#define WPP_CURRENT_DAY_LOG_FILE   "/var/log/weak_passwd_protect"
#define ILLEGAL_OUTREAC_LEARNED_FILE   "/var/log/illegal_outreac_learned"
#define ILLEGAL_OUTREAC_LEARNED_FILE_0   "/var/log/illegal_outreac_learned_0"

#define WPP_RULES_MAX_HASH   128
#define WPP_RULES_HASH_MASK  (WPP_RULES_MAX_HASH - 1)
#define WPP_RULES_MAX_ENTRY  (WPP_RULES_MAX_HASH * 2)

enum _wpp_rule_class
{
    WPP_CLASS_EMPTY_PASSWD = 1,
    WPP_CLASS_ACCOUNT_SAME_AS_PASSWD = 2,
    WPP_CLASS_ST8_AND_DICTIONARY_SEQ = 3,
    WPP_CLASS_ST8_AND_ONLY_DIGIT = 4,
    WPP_CLASS_ST8_AND_ONLY_ALPHABET = 5,
    WPP_CLASS_ST8_AND_ONLY_DIGIT_ALPHABET = 6,
    WPP_CLASS_CUSTOM_RULE = 7,
    WPP_CLASS_CUSTOM_WEAK_PASSWD = 8,
    WPP_CLASS_INNER_WEAK_PASSWD = 9
};

enum _wpp_protocol
{
    WPP_PROTOCOL_FTP = 1<<0,
    WPP_PROTOCOL_TELNET = 1<<1,
    WPP_PROTOCOL_SMTP = 1<<2,
    WPP_PROTOCOL_IMAP = 1<<3,
    WPP_PROTOCOL_POP3 = 1<<4,
    WPP_PROTOCOL_WEB = 1<<5
};

enum _wpp_smtp_login_flag
{
    WPP_SMTP_LOGIN_FLAG_LOGIN = 1<<0,
    WPP_SMTP_LOGIN_FLAG_USERNAME = 1<<1,
    WPP_SMTP_LOGIN_FLAG_PASSWD = 1<<2
};

// illegal outreach protect
#define IOP_MAX_SERVER_NUM 64
#define IOP_MAX_SERVER_IP_LEN 1024
#define IOP_MAX_SERVER_IP_PAIR 32
#define IOP_MAX_IP_DOMAIN_LEN 64
#define IOP_MAX_PORT_RANGE_LEN 128
#define IOP_MAX_SERVER_PORT_PAIR 32
#define IOP_MAX_EXCLUDE_NUM 128
#define IOP_MAX_EXCLUDE_IP_PAIR 32
#define IOP_MAX_EXCLUDE_PORT_PAIR 32


// risk analysis
#define RISK_ANALYSIS_MAX_IP_STR 128
#define RISK_ANALYSIS_MAX_PORT_STR 128
#define RISK_ANALYSIS_MAX_APP_STR 128
#define RISK_ANALYSIS_MAX_USERNAME_STR 256
#define RISK_ANALYSIS_MAX_PASSWD_STR 3200
#define RISK_ANALYSIS_MAX_COMMON_STR 32

#define RISK_ANALYSIS_MAX_PORT_DIGIT_STR 256

#define RISK_ANALYSIS_MAX_IP_PAIR 2
#define RISK_ANALYSIS_MAX_PORT_PAIR 64

#define RISK_ANALYSIS_MAX_IP_STRING 64

#define BLKLIST_FILTER_MAX_DOMAIN_STRING 256
#define BLKLIST_FILTER_MAX_CONFIG_ENTRY 1024
#define BLKLIST_FILTER_MAX_CONFIG_STRING (BLKLIST_FILTER_MAX_CONFIG_ENTRY * BLKLIST_FILTER_MAX_DOMAIN_STRING)
#ifdef CONFIG_SIG
#define BLKLIST_FILTER_MAX_QEURY_NUM 10
#else
#define BLKLIST_FILTER_MAX_QEURY_NUM 50
#endif
#define BLKLIST_FILTER_MAX_QEURY_STRING 256 //same as BLKLIST_FILTER_MAX_DOMAIN_STRING

#define BLKLIST_FILTER_MAX_BUF_LEN 2048

#define TI_CUSTOM_RULE_MAX_DOMAIN_STRING 256
#define TI_CUSTOM_RULE_MAX_QEURY_NUM 50

#define TI_CATEGORY_INDEX_MAX 32 // custom and predefine both have seperate 32 index
#define TI_CATEGORY_NAME_ALL_MAX 256 // include more than one category in predefined rules

// fw seperate config start
//#define  ADDRESSBOOK_MULTIPLE_SELECT_STRING_LEN_MAX (COMMON_NAME_LEN_MAX * 512)
#define  ADDRESSBOOK_MULTIPLE_SELECT_STRING_LEN_MAX (COMMON_NAME_LEN_MAX * 256)
#define  GEO_MULTIPLE_SELECT_STRING_LEN_MAX (8 * 256)
#define  MAC_MULTIPLE_SELECT_STRING_LEN_MAX (COMMON_NAME_LEN_MAX * 32)

#define FW_SECURITY_PROTECT_POLICY_NUM_MAX 128

// fw seperate config end

//dns control
#define DNS_CONTROL_MAX_IP_CONFIG_NUM 128
#define DNS_CONTROL_MAX_DOMAIN_CONFIG_NUM 64
#define DNS_CONTROL_MAX_DOMAIN_LEN 256

#define MAC_FILTER_ENTRY_MAX 64
#define MAC_STR_NUM_MAX 256
#define MAC_FILTER_STRING_MAX (MAC_STR_LEN_MAX * MAC_STR_NUM_MAX)

enum _mail_flag
{
    MAIL_FLAG_MIN = 0,
    MAIL_FLAG_WPP_DROP = 1
};

enum _report_audit_flag
{
    IM_LOGIN  = 1,
    IM_CONTENT = 2,
    EMAIL_BASE = 3,
    EMAIL_CONTENT = 4,
    EMAIL_ATTACH = 5,
    WEB_URL = 6,
    WEB_TITLE = 7,
    WEB_SENDINFO = 8,
    WEB_SEARCHKEY = 9,
    WEB_FILE_TYPE = 10,
    FTP_LOG = 11,
    SESSION_LOG = 12,
    TELNET_LOG = 13,
    WEB_CONTENT_FILTER = 14,
    WEB_LOGIN = 15,
    WEB_POST = 16,
    IM_FILE_AUDIT = 17,
    FTP_FILE_LOG = 18, 
    FLOW_LOG = 19,
    ACCESS_NUM_LOG = 20,
    ONLINE_LOG = 21,
    DATABASE_LOG = 22,
    SMB_LOG = 23,
    //24, alarm
    //25, alarm
    //26, alarm
    WEB_SNAPSHOT_FILTER = 27,
    ADFILTER_LOG = 28,
    VPN_ABROAD_LOG = 29,    
#if 0
    // asas add, but not use macro CONFIG_NSAS
    SMB_FILE_LOG = 32,
    DNS_LOG = 33,
    HTTP_LOG = 34,
    HTTP_FILE_LOG = 35,
    TFTP_LOG = 36,
    TFTP_FILE_LOG = 37,
    NFS_LOG = 38,
    NFS_FILE_LOG = 39,
    SSL_LOG = 40,
    RDP_LOG = 41,
    SSH_LOG = 42,
    RLOGIN_LOG = 43,
    NETBIOS_NS_LOG = 44,
    NETBIOS_DS_LOG = 45,
    NETBIOS_SS_LOG = 46
    #endif
};
	
typedef enum ace_warn_level
{
	ACE_WARN_LEVEL_EMERG = 5,		//jinji
	ACE_WARN_LEVEL_ALERT = 1,		//jingbao
	ACE_WARN_LEVEL_CRUX = 4,		//guanjian
	ACE_WARN_LEVEL_ERROR = 7,		//cuowu
	ACE_WARN_LEVEL_WARN = 3,		//gaojing
	ACE_WARN_LEVEL_NOTICE = 6, 	//tongzhi
	ACE_WARN_LEVEL_INFO = 2,		//xinxi
	ACE_WARN_LEVEL_DEBUG = 8,		//tiaoshi
	ACE_WARN_LEVEL_MAX,
}ACE_WARN_LEVEL_E;

enum industrial_control_app_mark
{
    INDUSTRIAL_CONTROL_APP_MARK_MODBUS = 3098,
    INDUSTRIAL_CONTROL_APP_MARK_CIP = 3067,
    INDUSTRIAL_CONTROL_APP_MARK_S7 = 3058,
    INDUSTRIAL_CONTROL_APP_MARK_OPC = 5978,  // 2036, 2036 is custom mark, 1793 -- 2048
    INDUSTRIAL_CONTROL_APP_MARK_DNP3 = 2285,
    INDUSTRIAL_CONTROL_APP_MARK_PROFINET = 3068,
    INDUSTRIAL_CONTROL_APP_MARK_IEC104 = 3057,
    INDUSTRIAL_CONTROL_APP_MARK_ETHERNETIP = 2463, // ifw skip ethernetip
    INDUSTRIAL_CONTROL_APP_MARK_OPCUA_TCP = 3061,
    INDUSTRIAL_CONTROL_APP_MARK_MQTT = 3060,
    INDUSTRIAL_CONTROL_APP_MARK_FINS = 3059,
    INDUSTRIAL_CONTROL_APP_MARK_BACNET = 3056,
    INDUSTRIAL_CONTROL_APP_MARK_MMS = 746,
    INDUSTRIAL_CONTROL_APP_MARK_GOOSE = 3100,
    INDUSTRIAL_CONTROL_APP_MARK_SV = 3066
};

#if defined(CONFIG_NSAS)
#define NSAS_AUDIT_RULE_WARN_DESC   (1024)
#define NSAS_AUDIT_RULE_CONTENT     (2048)
#define NSAS_AUDIT_RULE_MAX    (64)
#define NSAS_AUDIT_RULE_ITEM_MAX    (64)
#define NSAS_FILTER_TIME_CNT_MAX    (600)
#define NSAS_FILTER_POTENTIAL_RISK      (800)

typedef enum nsas_potentail_risk_type
{
	NSAS_PR_ASSOC = 0,    
	NSAS_PR_HAZARD,    
	NSAS_PR_ANOMALY,    
	NSAS_PR_MAX
}NSAS_POTENTIAL_RISK_TYPE;

typedef enum nsas_match_mode
{
	NSAS_MATCH_MODE_EQUAL = 0,    // ==
	NSAS_MATCH_MODE_NOTEQUAL,     // !=
	NSAS_MATCH_MODE_GEQUAL,       // >=
	NSAS_MATCH_MODE_LEQUAL,       // <=
	NSAS_MATCH_MODE_L,			  // <
	NSAS_MATCH_MODE_G,            // >
	NSAS_MATCH_MODE_CONTAIN,      // baohan
	NSAS_MATCH_MODE_MAX,
}NSAS_MATCH_MODE_E;

typedef enum nsas_warn_method
{
	NSAS_WARN_METHOD_NONE = 0,  
	NSAS_WARN_METHOD_VOICE = 1, 
	NSAS_WARN_METHOD_EMAIL = 2, 
	NSAS_WARN_METHOD_MAX,
}NSAS_WARN_METHOD_E;
	
typedef enum nsas_audit_rule_action
{
	NSAS_AUDIT_RULE_A_ACCEPT = 0,	
	NSAS_AUDIT_RULE_A_DROP, 
	NSAS_AUDIT_RULE_A_MAX,
}NSAS_AUDIT_RULE_ACTION_E;
	
typedef enum nsas_audit_rule_type
{
	NSAS_AUDIT_RULE_HTTP, // 0     
	NSAS_AUDIT_RULE_FTP,       
	NSAS_AUDIT_RULE_TFTP,      
	NSAS_AUDIT_RULE_RLOGIN,  // 3   

	NSAS_AUDIT_RULE_DNS,    // 4  
	NSAS_AUDIT_RULE_TELNET,       
	NSAS_AUDIT_RULE_SSH,     
	NSAS_AUDIT_RULE_SSL,       
	NSAS_AUDIT_RULE_RDP,   // 8   

	NSAS_AUDIT_RULE_SMB,      // 9    
	NSAS_AUDIT_RULE_NETBIOS_SS,
	NSAS_AUDIT_RULE_NETBIOS_NS,       
	NSAS_AUDIT_RULE_NETBIOS_DS,     
	NSAS_AUDIT_RULE_NFS,      // 13

	NSAS_AUDIT_RULE_ADDOMAIN, // 14  
	NSAS_AUDIT_RULE_LDAP,       
	NSAS_AUDIT_RULE_RADIUS,     
	NSAS_AUDIT_RULE_EMAIL,       
	NSAS_AUDIT_RULE_DB, // 18
	
	NSAS_AUDIT_RULE_SYSLOG, // 19
	NSAS_AUDIT_RULE_SNMP, //20
	NSAS_AUDIT_RULE_TYPE_MAX = 64,
	NSAS_AUDIT_RULE_COMM_PARAM, // 65 
}NSAS_AUDIT_RULE_TYPE;

typedef enum nsas_audit_rule_match
{
	NSAS_AUDIT_RULE_M_SIP = 0,  
	NSAS_AUDIT_RULE_M_DIP,      
	NSAS_AUDIT_RULE_M_SPORT,       
	NSAS_AUDIT_RULE_M_DPORT,      
	NSAS_AUDIT_RULE_M_SMAC,   

	NSAS_AUDIT_RULE_M_DMAC,     
	NSAS_AUDIT_RULE_M_APP_PROTOCOL,      //yingyong xieyi 
	NSAS_AUDIT_RULE_M_TRANS_PROTOCOL,     //cuangshu xieyi
	NSAS_AUDIT_RULE_M_S_LOCATION,   //source weizhi    
	NSAS_AUDIT_RULE_M_D_LOCATION,    //dest weizhi

	NSAS_AUDIT_RULE_M_SIP_CREDIT, //sip xinyu      
	NSAS_AUDIT_RULE_M_DIP_CREDIT, //dip xinyu
	NSAS_AUDIT_RULE_M_AUTH_USER,   //renzheng yonghuming     
	NSAS_AUDIT_RULE_M_APP_PROTOCOL_GROUP,  //yingyong xieyi fenzu   

    NSAS_AUDIT_RULE_M_COMM_MAX = 64,

	NSAS_AUDIT_RULE_M_HTTP_REF = NSAS_AUDIT_RULE_M_COMM_MAX,
	NSAS_AUDIT_RULE_M_HTTP_ACCESS,   //http fangwen xingwei
	NSAS_AUDIT_RULE_M_HTTP_HOST,     //http fangwen yuming
	NSAS_AUDIT_RULE_M_HTTP_URL,
	NSAS_AUDIT_RULE_M_HTTP_RESP_TYPE,   // http xiangying neixing

	NSAS_AUDIT_RULE_M_HTTP_REQ_TYPE,    // http qingqiu neixing
	NSAS_AUDIT_RULE_M_HTTP_REQ_FILE,
	NSAS_AUDIT_RULE_M_HTTP_REQ_PARAM,
	NSAS_AUDIT_RULE_M_HTTP_ACCESS_BROSER,
	NSAS_AUDIT_RULE_M_HTTP_RESP_CODE,

	NSAS_AUDIT_RULE_M_HTTP_SERVER,
	NSAS_AUDIT_RULE_M_HTTP_COOKIE,
	NSAS_AUDIT_RULE_M_HTTP_TRANS_FILE,
	NSAS_AUDIT_RULE_M_HTTP_EVENT_ID,
	NSAS_AUDIT_RULE_M_HTTP_DOMAIN_CLASS,

	NSAS_AUDIT_RULE_M_HTTP_DOMAIN_THREAT_TYPE,
	NSAS_AUDIT_RULE_M_HTTP_PROXY_IP,
	NSAS_AUDIT_RULE_M_HTTP_PROXY_SURF,
	NSAS_AUDIT_RULE_M_HTTP_RESP_LENGTH,
	NSAS_AUDIT_RULE_M_HTTP_RESP_TIME,
	
	NSAS_AUDIT_RULE_M_HTTP_VER,

	NSAS_AUDIT_RULE_M_FTP_USER_NAME = 96,
    NSAS_AUDIT_RULE_M_FTP_FILE_NAME,
    NSAS_AUDIT_RULE_M_FTP_DIR_NAME,
    NSAS_AUDIT_RULE_M_FTP_DIRECTION,  /* 1 upload; 2 download */
    NSAS_AUDIT_RULE_M_FTP_FILE_SIZE,

    NSAS_AUDIT_RULE_M_FTP_CMD,
    NSAS_AUDIT_RULE_M_FTP_CMD_RET,

    NSAS_AUDIT_RULE_M_TFTP_FILE_NAME = 112,
    NSAS_AUDIT_RULE_M_TFTP_MODE,
    NSAS_AUDIT_RULE_M_TFTP_DIR_NAME,
    NSAS_AUDIT_RULE_M_TFTP_DIRECTION,

    NSAS_AUDIT_RULE_M_RLOGIN_USER_NAME = 128,
    NSAS_AUDIT_RULE_M_RLOGIN_CMD,
    NSAS_AUDIT_RULE_M_RLOGIN_RET,

    NSAS_AUDIT_RULE_M_DNS_QUERY_IP = 144,
    NSAS_AUDIT_RULE_M_DNS_PKG_DIR,   // 0: request 1:response
    NSAS_AUDIT_RULE_M_DNS_QUERY_DOMAIN,
    NSAS_AUDIT_RULE_M_DNS_QUERY_STATUS,
    NSAS_AUDIT_RULE_M_DNS_REQ_ID,

    NSAS_AUDIT_RULE_M_DNS_QUERY_CLASS,
    NSAS_AUDIT_RULE_M_DNS_MX_SERVER,
    NSAS_AUDIT_RULE_M_DNS_DOMAIN_TXT,
    NSAS_AUDIT_RULE_M_DNS_AN_COUNT,

    NSAS_AUDIT_RULE_M_DNS_NS_COUNT,
    NSAS_AUDIT_RULE_M_DNS_AR_COUNT,
    NSAS_AUDIT_RULE_M_DNS_AN_CONTENT,
    NSAS_AUDIT_RULE_M_DNS_NS_CONTENT,
    NSAS_AUDIT_RULE_M_DNS_AR_CONTENT,

    NSAS_AUDIT_RULE_M_DNS_TTL,
	NSAS_AUDIT_RULE_M_DNS_QUERY_TYPE,
    NSAS_AUDIT_RULE_M_DNS_AA,
    NSAS_AUDIT_RULE_M_DNS_TC,
    NSAS_AUDIT_RULE_M_DNS_RD,
    NSAS_AUDIT_RULE_M_DNS_RA,

    NSAS_AUDIT_RULE_M_DNS_RCODE,
    NSAS_AUDIT_RULE_M_REQ_LEN,
    NSAS_AUDIT_RULE_M_REP_LEN,
    NSAS_AUDIT_RULE_M_CNAME,
    NSAS_AUDIT_RULE_M_TXTRECORD,

    NSAS_AUDIT_RULE_M_DOMAIN_CATEGORY,
    NSAS_AUDIT_RULE_M_DOMAIN_THREAT_TYPE,
    NSAS_AUDIT_RULE_M_DGA_PROBABILITY,

    NSAS_AUDIT_RULE_M_TELNET_USER_NAME = 176,
    NSAS_AUDIT_RULE_M_TELNET_CMD,
    NSAS_AUDIT_RULE_M_TELNET_RET,

    NSAS_AUDIT_RULE_M_SSH_C_VERSION = 192,
    NSAS_AUDIT_RULE_M_SSH_C_INFO,
    NSAS_AUDIT_RULE_M_SSH_S_VERSION,
    NSAS_AUDIT_RULE_M_SSH_S_INFO,

    NSAS_AUDIT_RULE_M_SSL_S_VERSION = 208,
    NSAS_AUDIT_RULE_M_SSL_COUNTRY,
    NSAS_AUDIT_RULE_M_SSL_ORGANIZE,
    NSAS_AUDIT_RULE_M_SSL_UORGANIZE,
    NSAS_AUDIT_RULE_M_SSL_CNAME,

    NSAS_AUDIT_RULE_M_SSL_UCNAME,
    NSAS_AUDIT_RULE_M_SSL_SERVER_NAME,
    NSAS_AUDIT_RULE_M_SSL_CIPHER_SUITE,
    NSAS_AUDIT_RULE_M_SSL_FINGERPRINT,
    NSAS_AUDIT_RULE_M_SSL_SKEY_ALGORITHM_TYPE,

    NSAS_AUDIT_RULE_M_SSL_SKEY_SIGNATURE_ALGORITHM,
    NSAS_AUDIT_RULE_M_SSL_SKEY_EXCHANGE_PUBKEY,
    NSAS_AUDIT_RULE_M_SSL_CKEY_ALGORITHM_TYPE,
    NSAS_AUDIT_RULE_M_SSL_CKEY_EXCHANGE_PUBKEY,

    NSAS_AUDIT_RULE_M_RDP_S_VERSION = 240,
    NSAS_AUDIT_RULE_M_RDP_COUNTRY,
    NSAS_AUDIT_RULE_M_RDP_ORGANIZE,
    NSAS_AUDIT_RULE_M_RDP_UORGANIZE,
    NSAS_AUDIT_RULE_M_RDP_CNAME,

    NSAS_AUDIT_RULE_M_RDP_UCNAME,
    NSAS_AUDIT_RULE_M_RDP_SERVER_NAME,
    NSAS_AUDIT_RULE_M_RDP_CIPHER_SUITE,
    NSAS_AUDIT_RULE_M_RDP_FINGERPRINT,
    NSAS_AUDIT_RULE_M_RDP_SKEY_ALGORITHM_TYPE,

    NSAS_AUDIT_RULE_M_RDP_SKEY_SIGNATURE_ALGORITHM,
    NSAS_AUDIT_RULE_M_RDP_SKEY_EXCHANGE_PUBKEY,
    NSAS_AUDIT_RULE_M_RDP_CKEY_ALGORITHM_TYPE,
    NSAS_AUDIT_RULE_M_RDP_CKEY_EXCHANGE_PUBKEY,

    NSAS_AUDIT_RULE_M_SMB_ACCOUNT = 272,
    NSAS_AUDIT_RULE_M_SMB_FILE_NAME,
    NSAS_AUDIT_RULE_M_SMB_DIR_NAME,
    NSAS_AUDIT_RULE_M_SMB_FILE_TYPE,
    NSAS_AUDIT_RULE_M_SMB_CMD,

    NSAS_AUDIT_RULE_M_SMB_DIRECTION,

    NSAS_AUDIT_RULE_M_NETBIOS_NS_DEVICE_NAME = 288,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_GROUP_NAME,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_SERVICE_NAME,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_IDENTIFIER_STR_CN,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_GROUP_TYPE,

    NSAS_AUDIT_RULE_M_NETBIOS_NS_SERVICE_COUNT,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_REQ_TYPE,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_RESP_STATUS,
    NSAS_AUDIT_RULE_M_NETBIOS_NS_MAC,

    NSAS_AUDIT_RULE_M_NETBIOS_DS_SOURCE_NAME = 304,
    NSAS_AUDIT_RULE_M_NETBIOS_DS_SOURCE_IDENTIFIER,
    NSAS_AUDIT_RULE_M_NETBIOS_DS_DEST_NAME,
    NSAS_AUDIT_RULE_M_NETBIOS_DS_DEST_IDENTIFIER,
    NSAS_AUDIT_RULE_M_NETBIOS_DS_OS_VERSION,

    NSAS_AUDIT_RULE_M_NETBIOS_DS_BROWSER_VERSION,
    NSAS_AUDIT_RULE_M_NETBIOS_DS_MAILSLOT_NAME,
    
	NSAS_AUDIT_RULE_M_NETBIOS_SS_ACCOUNT = 320,
	NSAS_AUDIT_RULE_M_NETBIOS_SS_CMD,
	NSAS_AUDIT_RULE_M_NETBIOS_SS_FILE_NAME,
	NSAS_AUDIT_RULE_M_NETBIOS_SS_FILE_TYPE,
	NSAS_AUDIT_RULE_M_NETBIOS_SS_FILE_PATH,
	
	NSAS_AUDIT_RULE_M_NETBIOS_SS_DIR_FILE_NAME,

    NSAS_AUDIT_RULE_M_NFS_MSG_TYPE = 336,
    NSAS_AUDIT_RULE_M_NFS_CMD,
    NSAS_AUDIT_RULE_M_NFS_RESP_STATUS,
	NSAS_AUDIT_RULE_M_NFS_FILE_TYPE,
    NSAS_AUDIT_RULE_M_NFS_FILE_NAME,

    NSAS_AUDIT_RULE_M_NFS_FLID,
    NSAS_AUDIT_RULE_M_NFS_RPC_VERSION,
    NSAS_AUDIT_RULE_M_NFS_PROG_ID,
    NSAS_AUDIT_RULE_M_NFS_PROG_NAME,
    NSAS_AUDIT_RULE_M_NFS_PROG_VERSION,

    NSAS_AUDIT_RULE_M_ADDOMAIN_CNAME = 352,
    NSAS_AUDIT_RULE_M_ADDOMAIN_DOMAIN,
    NSAS_AUDIT_RULE_M_ADDOMAIN_HOST_NAME,

    NSAS_AUDIT_RULE_M_LDAP_CN = 368,
    NSAS_AUDIT_RULE_M_LDAP_DC,

    NSAS_AUDIT_RULE_M_RADIUS_USER_NAME = 384,
    NSAS_AUDIT_RULE_M_RADIUS_NAS_IP,
    NSAS_AUDIT_RULE_M_RADIUS_NAS_PORT,
    NSAS_AUDIT_RULE_M_RADIUS_CALL_MAC,
    NSAS_AUDIT_RULE_M_RADIUS_CALLED_MAC,
    
    NSAS_AUDIT_RULE_M_RADIUS_PWD,
    
    NSAS_AUDIT_RULE_M_DB_CLIENT_APP = 390,
    NSAS_AUDIT_RULE_M_DB_DATABASE,
    NSAS_AUDIT_RULE_M_DB_ACCOUNT,
    NSAS_AUDIT_RULE_M_DB_SRC_HOST,
    NSAS_AUDIT_RULE_M_DB_DST_HOST,

    NSAS_AUDIT_RULE_M_DB_CLIENT_USER,
    NSAS_AUDIT_RULE_M_DB_TABLE,
    NSAS_AUDIT_RULE_M_DB_OBJECT,
    NSAS_AUDIT_RULE_M_DB_COMMAND,
    NSAS_AUDIT_RULE_M_DB_SQL,
	
    NSAS_AUDIT_RULE_M_DB_DIR,
    NSAS_AUDIT_RULE_M_DB_ERR_CODE,
    NSAS_AUDIT_RULE_M_DB_COLUM,
    NSAS_AUDIT_RULE_M_DB_INSTANCE,
    NSAS_AUDIT_RULE_M_DB_AFFECT_ROWS,

    NSAS_AUDIT_RULE_M_SYSLOG_MSG = 422,
    
	NSAS_AUDIT_RULE_M_EMAIL_FROM = 430,
	NSAS_AUDIT_RULE_M_EMAIL_TO,
	NSAS_AUDIT_RULE_M_EMAIL_CC,
	NSAS_AUDIT_RULE_M_EMAIL_BCC,
	NSAS_AUDIT_RULE_M_EMAIL_SUBJECT,
	
	NSAS_AUDIT_RULE_M_EMAIL_CONTENT,
	NSAS_AUDIT_RULE_M_EMAIL_ATTACH_NAME,
	NSAS_AUDIT_RULE_M_EMAIL_ATTACH_SIZE,

	NSAS_AUDIT_RULE_M_SNMP_VERSION = 450,
	NSAS_AUDIT_RULE_M_SNMP_COMMUNITY,
	NSAS_AUDIT_RULE_M_SNMP_REQUEST_ID,
	NSAS_AUDIT_RULE_M_SNMP_ERROR_STATUS,
	NSAS_AUDIT_RULE_M_SNMP_ERROR_INDEX,
	NSAS_AUDIT_RULE_M_SNMP_OID,
	NSAS_AUDIT_RULE_M_SNMP_VALUE,
	NSAS_AUDIT_RULE_M_SNMP_ENTERPRISE_OID,
	NSAS_AUDIT_RULE_M_SNMP_GENERIC_TRAP,
	NSAS_AUDIT_RULE_M_SNMP_SPECIFIC_TRAP,
	NSAS_AUDIT_RULE_M_SNMP_MSG_ID,
	NSAS_AUDIT_RULE_M_SNMP_USERNAME,
}NSAS_AUDIT_RULE_MATCH;
	
typedef enum nsas_audit_rule_cond
{
	NSAS_AUDIT_RULE_CON_OR = 0,	// and
	NSAS_AUDIT_RULE_CON_AND,			// or
	NSAS_AUDIT_RULE_CON_MAX, 	
}NSAS_AUDIT_RULE_COND;


// log type stat use, do not change the numbers, need to be same as php(web)
// deleted numbers shoud not used again
// max is U16
typedef enum nsas_log_stat_type
{
    // audit log
	NSAS_LOG_STAT_TYPE_HTTP = 1,
	NSAS_LOG_STAT_TYPE_FTP,
	NSAS_LOG_STAT_TYPE_TFTP,
	NSAS_LOG_STAT_TYPE_RLOGIN,
	NSAS_LOG_STAT_TYPE_DNS,     //5

    NSAS_LOG_STAT_TYPE_TELNET,
	NSAS_LOG_STAT_TYPE_SSH,
	NSAS_LOG_STAT_TYPE_SSL,
	NSAS_LOG_STAT_TYPE_RDP,
	NSAS_LOG_STAT_TYPE_SMB,   //10

    NSAS_LOG_STAT_TYPE_NETBIOS_SS,
	NSAS_LOG_STAT_TYPE_NETBIOS_NS,
	NSAS_LOG_STAT_TYPE_NETBIOS_DS,
	NSAS_LOG_STAT_TYPE_NFS,
	NSAS_LOG_STAT_TYPE_ADDOMAIN,    //15

    NSAS_LOG_STAT_TYPE_LDAP,
	NSAS_LOG_STAT_TYPE_RADIUS,
	NSAS_LOG_STAT_TYPE_EMAIL,
	NSAS_LOG_STAT_TYPE_DB,
	NSAS_LOG_STAT_TYPE_SYSLOG, //20

	NSAS_LOG_STAT_TYPE_SNMP,


    // safety log
	NSAS_LOG_STAT_TYPE_AD = 256, //256
    NSAS_LOG_STAT_TYPE_IPS,
    NSAS_LOG_STAT_TYPE_AV,
    NSAS_LOG_STAT_TYPE_WAF, // nouse
    NSAS_LOG_STAT_TYPE_TI,

    NSAS_LOG_STAT_TYPE_WPP, // only log, no warning
    NSAS_LOG_STAT_TYPE_DGA,

	NSAS_LOG_STAT_TYPE_TITLE = 512,
	NSAS_LOG_STAT_TYPE_SEARCH,
	NSAS_LOG_STAT_TYPE_IM,
	NSAS_LOG_STAT_TYPE_BBS,
	NSAS_LOG_STAT_TYPE_APP_LOGIN,
	NSAS_LOG_STAT_TYPE_DROP,

    NSAS_LOG_STAT_TYPE_MAX
}NSAS_LOG_STAT_TYPE;

#endif

#define NRADIO_CALLED_STATION_ID_LEN (256+18)
#define NRADIO_STA_ID_LEN 18
#define REPORT_AUDIT_FLAG_MARK(_flag) (1 << _flag)

#define MAX_USER_AGENT_STRING_LEN 128
#define MAX_USER_AGENT_STRING_NUM 1024
#define MAX_USER_AGENT_BIT_MASK (MAX_USER_AGENT_STRING_NUM / 32)
#define IOS_USER_AGENT_START_WORD 0
#define ANDROID_USER_AGENT_START_WORD 3
#define IOS_USER_AGENT_START_WORD2 13
#define AI_ANDROID_USER_AGENT_START_WORD 16

//#define MAX_LOCATION_NUM 4096

#define MAX_SERVICE_GROUP 128

#define MAX_MAIL_ADDRESS_ITEM_NUMBER 32
#define MAX_MAIL_ADDRESS_STRING_LEN 64


#define ACE_KERNEL_NETLINK_ID           30
#define NETLINK_REPORTER                31     

#define PENDING_PKT_CHECK_FIELDA 0x0A0A0A0A
#define PENDING_PKT_CHECK_FIELDB 0x0B0B0B0B

#if MANAGE_FRAME_SECURITY_ONE_LEN > 256
MANAGE_FRAME_SECURITY_ONE_LEN must be <= 256
#endif

#if MANAGE_FRAME_SECURITY_NAME_LEN < 512
MANAGE_FRAME_SECURITY_NAME_LEN must be >= 512
#endif

#define SECURITY_GROUP_MATCH(_GROUP_USER_MAX, _shost_user_group, _host_entry_path, _security_group, _enable, _find) \
{																					   \
	int macro_max_string; 																   \
	int macro_policy_group_len;															   \
	int macro_group_len;																	   \
	char *macro_p = NULL, *macro_q = NULL;														   \
	int macro_i = 0;																		   \
																					   \
	memset(_security_group, 0, MANAGE_FRAME_SECURITY_ONE_LEN);						   \
																					   \
	macro_max_string = strlen(_host_entry_path);						   \
	_find = 0;																		   \
																					   \
	for(macro_i = 0; macro_i < _GROUP_USER_MAX; macro_i++)											   \
	{																				   \
		if(_shost_user_group[macro_i][0] == '\0') 										   \
		{																			   \
			break;																	   \
		}																			   \
																					   \
		macro_p = _host_entry_path; 									   \
		macro_policy_group_len = strlen(_shost_user_group[macro_i]);							   \
																					   \
		if ( _enable )								   \
		{																			   \
			macro_q = strchr(macro_p, '\n');													   \
			while(macro_q)																   \
			{																		   \
				*macro_q = 0; 															   \
				macro_group_len = macro_q - macro_p;													   \
				_security_group[0] = 0;												   \
				macro_group_len = manage_frame_uncompress_security_group(_security_group,	   \
					MANAGE_FRAME_SECURITY_ONE_LEN, macro_p, macro_group_len,					   \
					_host_entry_path, macro_max_string);				   \
				if (!strncmp(_security_group, _shost_user_group[macro_i], macro_policy_group_len))  \
				{																	   \
					if ((macro_policy_group_len >= macro_group_len) 							   \
						|| (_security_group[macro_policy_group_len]=='/')) 				   \
					{																   \
						*macro_q = '\n';													   \
						_find = 1;													   \
						break;														   \
					}																   \
				}																	   \
				*macro_q = '\n';															   \
				macro_p = macro_q + 1;															   \
				macro_q = strchr(macro_p, '\n');												   \
			}																		   \
			if (_find)																   \
			{																		   \
				break;																   \
			}																		   \
		}																			   \
		else																		   \
		{																			   \
			if(!strncmp(macro_p, _shost_user_group[macro_i], macro_policy_group_len)) 				   \
			{																		   \
				if ((macro_policy_group_len >= macro_max_string)								   \
					|| (macro_p[macro_policy_group_len]=='/'))									   \
				{																	   \
					_find = 1;														   \
					break;															   \
				}																	   \
			}																		   \
		}																			   \
	}																				   \
}

#define HYLAB_SHARE_IP_MATCH_ASSET(_entry, _ip) \
    hytf_inet_addr_cmp(&_entry.ip_u3, &_ip)

#define HYLAB_SHARE_IP_MATCH(_entry, _ip) \
    _entry.authed && hytf_inet_addr_cmp(&_entry.ip_u3, &_ip)

#define HYLAB_SHARE_IPV4_MATCH(_entry, _ip) \
    _entry.authed && !_entry.ipv6 && _entry.ip_u3.ip4.ip == _ip

typedef enum _SYSLOG_FORMAT_T
{
	SFT_DEFAULT,
	SFT_Q_GDW,            //Q_GDW 11802-2018
	SFT_GB_31992,         //GB/T 31992
	SFT_CGW,              //CGW-GREATWALL
	SFT_RAILWAY,          //RAILWAY
	SFT_RFC5424,          //RFC5424
	SFT_MAX
}SYSLOG_FORMAT_T;

enum ASSET_ACCESS_RULE
{
	ASSET_ACCESS_NONE,
	ASSET_ACCESS_PRIVATE_ALLOW,
	ASSET_ACCESS_PRIVATE_DENY,
	ASSET_ACCESS_COUNTERFEIT_ALLOW,
	ASSET_ACCESS_COUNTERFEIT_DENY,
	ASSET_ACCESS_MAC_BIND_ALLOW,
	ASSET_ACCESS_MAC_BIND_DENY,
	ASSET_ACCESS_PORT_BIND_ALLOW,
	ASSET_ACCESS_PORT_BIND_DENY,
	ASSET_ACCESS_EXTERNAL_CONN_DENY,
	ASSET_ACCESS_EXTERNAL_CONN_ALARM,
	ASSET_ACCESS_SKIP,
};

#ifndef __KERNEL__
#define rcu_assign_pointer(x,y) x=y
#define RCU_INIT_POINTER(x,y) x=y
#define rcu_barrier()
#define rcu_read_lock()
#define rcu_read_unlock()
#define rcu_dereference(x) x
#define rcu_dereference_protected(x,y) x
#define lockdep_is_held(x) x
#define NF_CT_ASSERT ASSERT
#define kzalloc(x,y) ({void* __m__ = malloc(x); if (__m__)memset(__m__,0x00,x); __m__;})//malloc(x)
#ifndef BUG_ON
#define BUG_ON(x) ASSERT(!(x))
#endif

#ifndef ALIGN
#define ALIGN(x, a) (((x) + ((__typeof__(x))(a) - 1)) & \
			~((__typeof__(x))(a) - 1))
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef __h2ns
#define __h2ns(x)	((U16)((((U16)x)<<8)|((((U16)x)&0xff00)>>8)))
#endif

#pragma pack(pop)
#ifndef __KERNEL__
#ifndef DEFAULT_CONFIG_FILE
#include <execinfo.h>
#include <syslog.h>
#include <signal.h>
static_always_inline void hylab_segfault_print_stack(int sig)
{
	void *buffer[32];
	char **strings;
	unsigned long nnn;
	unsigned long nptrs = backtrace(buffer, sizeof(buffer)/sizeof(buffer[0]));
	strings = backtrace_symbols(buffer, nptrs);
	if (NULL == strings)
	{
		return;
	}
	if (nptrs <= 0)
	{
		goto free_symbols;
	}
	if (nptrs > sizeof(buffer)/sizeof(buffer[0]))
	{
		nptrs = sizeof(buffer)/sizeof(buffer[0]);
	}
	syslog( LOG_USER | LOG_INFO, "=====stack start=====");
	for (nnn = 0; nnn < nptrs; nnn++)
	{
		syslog( LOG_USER | LOG_INFO, "%s", strings[nnn]);
	}
	syslog( LOG_USER | LOG_INFO, "=====stack end=====");

free_symbols:
	free(strings);
	if (SIGPIPE != sig)
	{
		signal(sig, SIG_DFL);
		raise(sig);
	}
}

static_always_inline void hylab_signal_init(void)
{
	signal(SIGSEGV, hylab_segfault_print_stack);
    //signal(SIGPIPE, hylab_segfault_print_stack);
    signal(SIGFPE, hylab_segfault_print_stack);
    signal(SIGABRT, hylab_segfault_print_stack);
}
#endif
#endif
#endif /*__ACE_COMMON_MACRO_H__*/
