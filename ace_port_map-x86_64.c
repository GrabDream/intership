
#define _GNU_SOURCE         /* See feature_test_macros(7) */

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
#include "ext_card_common.h"
#include "ace_init_common.h"

ACE_PORT_INFO_ST  g_port_info[ACE_MAX_SUPPORT_PORT_BUSYBOX];
static char bypass_switch_config[256]   = "/home/config/cfg-scripts/bypass_switch_config";

#define ACE_PORT_ORDER_LAN_WAN 1

#define MAX_LOGFILE_SIZE (2000*1024)
#define TIME_LEN 100
#define SYSAPPLOG            "/var/log/ace_init"

extern int g_iUpdateVppCmdConf;

extern int ace_get_physical_port_num( void );

int ruijie_check_change_ext_card( char* pOldFile, char* pNewFile );

const char* hylab_init_get_current_date(long * ioSec) 
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

long hylab_init_get_file_size( char * filename )
{
    struct stat f_stat = { 0 };

    if( stat( filename, &f_stat ) == -1 )
    {
        return -1;
    }

   return (long)f_stat.st_size;
}

int hylab_init_debug_print(const char * fmt, ...)
{ 
    char curtime[TIME_LEN]    = { 0 }; 
    FILE *fp                  = NULL;
    long ace_time             = 0;
    long file_size            = 0;
    int  n                    = 0;
    va_list ap; 

    strcpy( curtime, hylab_init_get_current_date( &ace_time) );
    file_size = hylab_init_get_file_size(SYSAPPLOG);

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

static char *hylab_trim(char *s)
{
	int n;
    for (n = strlen(s)-1; n >= 0; n--)
		if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n' && s[n] != '\r')
			break;
    s[n+1] = '\0';
    return s;
}

static int check_dev_content( char* pFile, char* pDstContent )
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

static int get_i2c_buf_address( char* pFilter )
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

static int get_chuangshi_slot_info( unsigned int iArrSlot[], int iArrSizes, int iSlots )
{
	int i = 0;
	int iRet = 0;
	char szCmd[256] = {0};
	char szLine[256] = {0};

	if ( iSlots <= 6 )
	{
		int iI2c = get_i2c_buf_address( "I801" );

		if ( -1 != iI2c )
		{
			for ( i=0; i<iSlots; i++ )
			{
				if ( ( 0 == i ) || ( 2 == i ) )
				{
					snprintf( szCmd, sizeof( szCmd ) - 1, "/usr/sbin/i2cset -f -y %d 0x70 0x0; /usr/sbin/i2cset -f -y %d 0x71 0x0", iI2c, iI2c );

					system( szCmd );

					//ext_card_debug_print( "[%s:%d] ==> Info, slot%d, cmd [%s]", __FUNCTION__, __LINE__, i, szCmd );
				}

				if ( i < 2 )
				{
					snprintf( szCmd, sizeof( szCmd ) - 1, "/usr/sbin/i2cset -f -y %d 0x70 0x%d", iI2c, i + 1 );

					system( szCmd );

					//ext_card_debug_print( "[%s:%d] ==> Info, slot%d, cmd [%s]", __FUNCTION__, __LINE__, i, szCmd );
				}
				else
				{
					int iReg = 1 << ( i - 2 );

					snprintf( szCmd, sizeof( szCmd ) - 1, "/usr/sbin/i2cset -f -y %d 0x71 0x%d", iI2c, iReg );

					system( szCmd );

					//ext_card_debug_print( "[%s:%d] ==> Info, slot%d, cmd [%s]", __FUNCTION__, __LINE__, i, szCmd );
				}

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

				//ext_card_debug_print( "[%s:%d] ==> Info, slot%d, card info 0x%X", __FUNCTION__, __LINE__, i, iArrSlot[i] );
			}
		}
	}

	return 0;
}

static int get_eth_slot_dpdk_value( int iType, char* pEth , char* pOut, int iOut )
{
	int iRet = 0;
	FILE* fp = NULL;
	char szLine[256] = {0};
	char szEthInfo[128] = {0};

	if ( pOut && pEth && iOut > 0 )
	{
		if ( 1 == iType )
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
#if 0
#if (defined (CS_NZ02))
						snprintf( pOut, iOut - 1, "d%u/enp%d/s%d/f%d", iDomain, iVendor, iDev, iFunc );
#else
						snprintf( pOut, iOut - 1, "enp%d/s%d/f%d", iVendor, iDev, iFunc );
#endif
#else
						snprintf( pOut, iOut - 1, "d%u/enp%d/s%d/f%d", iDomain, iVendor, iDev, iFunc );
#endif
						break;
					}

					memset( szLine, 0, sizeof( szLine ) );
				}

				fclose( fp );
			}
		}
		else if ( 2 == iType ) // NT09, board
		{
			snprintf( szEthInfo, sizeof( szEthInfo ) - 1, "/sys/class/net/%s/device/uevent", pEth );

			if ( ( fp = fopen( szEthInfo, "r" ) ) )
			{
				while( 0 != fgets( szLine, sizeof( szLine ) - 1, fp ) )
				{
					if ( 0 == strncmp( szLine, "OF_FULLNAME=/soc/ethernet@", strlen( "OF_FULLNAME=/soc/ethernet@" ) ) )
					{
						char* pSlot = szLine + strlen( "OF_FULLNAME=/soc/ethernet@" );

						char szVenvor[16] = {0};

						sscanf( pSlot, "%s", szVenvor );

						snprintf( pOut, iOut - 1, "%s.ethernet", szVenvor );

						break;
					}

					memset( szLine, 0, sizeof( szLine ) );
				}

				fclose( fp );
			}
		}
	}
	else
	{
		iRet = -1;
	}

	return iRet;
}

int get_vpp_eth_config_info( char pOut[128][64], int* pRLine )
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

static int have_i_pci_device (const char* device)
{
	int result;
	char  cmdbuf [128]={0};
	sprintf( cmdbuf , "cat /proc/bus/pci/devices | grep %s", device);
	result = !(system ( cmdbuf ));
	return result;
}

int ace_get_port_num( void )
{
    FILE*   fp                            = NULL;
    char    buffer[512]  = { 0 };
    int     portnum                       = 0;

#if defined(RK_PLATFORM_MT) || defined(RK_PLATFORM_CS03) || defined(E2000Q_PLATFORM) || defined(CS_E2000Q_PLATFORM)
	fp = popen("/sbin/ifconfig -a|grep Ethernet|grep -E '^ge|^eth' |awk '{print$1}'", "r");
#else
    fp = popen("lspci | grep 'Ethernet controller'", "r");
#endif
    if ( !fp )
    {
        init_coordinate_log(" error");
        return 0;
    }

    while ( 0 != fgets(buffer, sizeof(buffer), fp ) )
    {
        portnum++;
    }

    pclose( fp );
    return portnum;
}

int ace_get_boardtype( void )
{
    FILE*   fp                            = NULL;
    char    buffer[512]  = { 0 };
    int     boardtype                       = 0;

    fp = popen("cat /proc/device-tree/model", "r");
    if ( !fp )
    {
        init_coordinate_log(" error");
        return 0;
    }

    if ( 0 != fgets(buffer, sizeof(buffer), fp ) )
    {
        if ( 0 == strncmp(buffer, "NT06-E2000Q", strlen("NT06-E2000Q")) )
        {
            boardtype = 6;
        }
        else if ( 0 == strncmp(buffer, "NT09-E2000Q", strlen("NT09-E2000Q")) )
        {
            boardtype = 9;
        }
        else if ( 0 == strncmp( buffer, "NT03", strlen( "NT03" ) ) )
        {
            boardtype = 3;
        }
    }

    pclose( fp );

	if ( 0 == boardtype )
	{
		FILE * fdmi = popen( "/usr/sbin/dmidecode -t 1 | grep 'Product Name: ' | awk '{if (NR==1){print $0}}'", "r" );

		if ( fdmi )
		{
			char dmi_product[128] = { 0 };

			if ( NULL != fgets( dmi_product, sizeof( dmi_product ), fdmi ) )
			{
				char *p = strstr( dmi_product, "Product Name: " );
				if ( p )
				{
					p += sizeof("Product Name: ")-1;

					if ( !strcmp( p, "NK03\n" ) )
					{
						boardtype = 103;
					}
				}
			}

			pclose( fdmi ); fdmi = NULL;
		}
	}

	return boardtype;
}

static char* ace_find_first_mgt_port( U32* retIndex )
{
    int mgt_index = -1;

    U32 portIndex = 0;

    for ( portIndex = 0; portIndex < ACE_MAX_SUPPORT_PORT_BUSYBOX; portIndex++ )
    {
        if (g_port_info[portIndex].isTrust != 2)
            continue;

		if ( strncmp( g_port_info[portIndex].newPhyname, "MGT", 3 ) && strncmp( g_port_info[portIndex].newPhyname, "MGMT", 4 ) )
			continue;

        if (mgt_index == -1)
            mgt_index = portIndex;
        else if (strcmp(g_port_info[portIndex].newPhyname, g_port_info[mgt_index].newPhyname) < 0)
            mgt_index = portIndex;

        //printf("%s %d portIndex %u mgt_idx %d\n", __FUNCTION__, __LINE__, portIndex, mgt_index);
    }

    if (mgt_index != -1)
    {
        *retIndex = mgt_index;
        //printf("%s %d mgt_idx %d phy %s\n", __FUNCTION__, __LINE__, mgt_index, g_port_info[mgt_index].newPhyname);
        return g_port_info[mgt_index].newPhyname;
    }

    return NULL;
}


static char* ace_find_port_by_index(  U32 ifindex, U32 isTrust, U32* retIndex )
{
    U32 portIndex = 0;

    for ( portIndex = 0; portIndex < ACE_MAX_SUPPORT_PORT_BUSYBOX; portIndex++ )
    {
        if ( ( ifindex == g_port_info[portIndex].ifIndex ) &&
             ( isTrust == g_port_info[portIndex].isTrust ) )
        {
            *retIndex = portIndex;
            return g_port_info[portIndex].newPhyname;
        }
    }

    return NULL;
}

static unsigned int ruijie_x_serial_ext_card_number = 0;
static struct ruijie_ext_card_struct ruijie_x_serial_ext_card_entry[MAX_CARD_ONE_DEV];

static const char* ruijie_rename_attribute_name[] = 
{
	"TRUST",
	"UNTRUST",
	"MANAGE",
};

static unsigned int ruijie_x_serial_ext_card_sorted[MAX_CARD_ONE_DEV];

static int have_i_pci_device (const char* device);

/**RUIJIE rename:**/
#if 1
int ruijie_check_change_ext_card( char* pOldFile, char* pNewFile )
{
	int iRet = 0;
	int iOldEthTotal = 0;
	int iNewEthTotal = 0;
	int iNewNameTotal = 0;
	int iLinuxEthTotal = 0;
	char szLine[256] = {0};
	char szCachOldEthInfos[128][32] = {0};
	char szCachNewEthInfos[128][32] = {0};
	char szCachNewNameInfos[128][32] = {0};
	char szCachLinuxEthInfos[128][32] = {0};

	if ( pOldFile && pNewFile )
	{
		// ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE
		if ( ( F_OK == access( pOldFile, F_OK ) ) && ( F_OK == access( pNewFile, F_OK ) ) )
		{
			FILE* fpOld = NULL;
			FILE* fpNew = NULL;
			FILE* fpLinux = NULL;

			do
			{
				fpOld = fopen( pOldFile, "r" );

				if ( NULL == fpOld )
				{
					init_coordinate_log( "Error, open file [%s] failed, error:%d,%s.", pOldFile, errno, strerror( errno ) );

					break;
				}

				while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpOld ) )
				{
					if ( strncmp( szLine, "##ExtMap##", strlen( "##ExtMap##" ) ) )
					{
						continue;
					}

					char* pEnd = strstr( szLine, " LAN" );

					if ( NULL == pEnd )
					{
						pEnd = strstr( szLine, " WAN" );
					}

					if ( pEnd && ( pEnd - szLine < 32 ) )
					{
						if ( iOldEthTotal >= 128 )
						{
							iRet = -1;

							init_coordinate_log( "Error, the eth number is more than %d.", 128 );

							break;
						}

						strncpy( szCachOldEthInfos[iOldEthTotal], szLine, pEnd - szLine );

						iOldEthTotal++;
					}
				}

				fclose( fpOld ); fpOld = NULL;

				if ( iRet )
				{
					break;
				}

				fpNew = fopen( pNewFile, "r" );

				if ( NULL == fpNew )
				{
					init_coordinate_log( "Error, open file [%s] failed, error:%d,%s.", pNewFile, errno, strerror( errno ) );

					break;
				}

				while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpNew ) )
				{
					if ( strncmp( szLine, "##ExtMap##", strlen( "##ExtMap##" ) ) )
					{
						if ( ( 0 == strncmp( szLine, "eth", strlen( "eth" ) ) ) && ( iNewNameTotal < 128 ) )
						{
							int iPair = 0;
							char szEthName[16] = {0};
							char szNewName[16] = {0};
							char szTempItem1[32] = {0};
							char szTempItem2[32] = {0};

							int iItem = sscanf( szLine, "%s %s %d %s %s", szEthName, szNewName, &iPair, szTempItem1, szTempItem2 );

							if ( 5 == iItem )
							{
								if ( ( 0 == strncmp( szNewName, "LAN", strlen( "LAN" ) ) ) || ( 0 == strncmp( szNewName, "WAN", strlen( "WAN" ) ) ) )
								{
									strcpy( szCachNewNameInfos[iNewNameTotal], szNewName );

									iNewNameTotal++;
								}
							}
						}
					
						continue;
					}

					char* pEnd = strstr( szLine, " LAN" );

					if ( NULL == pEnd )
					{
						pEnd = strstr( szLine, " WAN" );
					}

					if ( pEnd && ( pEnd - szLine < 32 ) )
					{
						if ( iNewEthTotal >= 128 )
						{
							iRet = -2;

							init_coordinate_log( "Error, the eth number is more than %d.", 128 );

							break;
						}

						strncpy( szCachNewEthInfos[iNewEthTotal], szLine, pEnd - szLine );

						iNewEthTotal++;
					}
				}

				fclose( fpNew ); fpNew = NULL;

				if ( iRet )
				{
					break;
				}

				if ( iNewEthTotal != iOldEthTotal )
				{
					iRet = 1;

					break;
				}
				else
				{
					int i = 0, j = 0;

					for ( i=0; i<iNewEthTotal; i++ )
					{
						int iFind = 0;

						for ( j=0; j<iOldEthTotal; j++ )
						{
							if ( 0 == strncmp( szCachNewEthInfos[i], szCachOldEthInfos[j], 32 ) )
							{
								iFind = 1;

								break;
							}
						}

						if ( 0 == iFind )
						{
							iRet = 2;

							init_coordinate_log( "Info, the new eth [%d,%s] is not find in old config.", i, szCachNewEthInfos[i] );

							break;
						}
					}
				}

				if ( ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) ) && !iRet )
				{
					fpLinux = fopen( ACE_VPP_PORT_MAP_FILE, "r" );

					if ( NULL == fpLinux )
					{
						init_coordinate_log( "Error, open file [%s] failed, error:%d,%s.", ACE_VPP_PORT_MAP_FILE, errno, strerror( errno ) );

						break;
					}

					while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpLinux ) )
					{
						if ( strstr( szLine, "enp" ) && ( iLinuxEthTotal < 128 ) )
						{
							int iPair = 0;
							char szEthName[16] = {0};
							char szNewName[16] = {0};
							char szTempItem1[32] = {0};
							char szTempItem2[32] = {0};

							int iItem = sscanf( szLine, "%s %s %d %s %s", szEthName, szNewName, &iPair, szTempItem1, szTempItem2 );

							if ( 5 == iItem )
							{
								if ( ( 0 == strncmp( szNewName, "LAN", strlen( "LAN" ) ) ) || ( 0 == strncmp( szNewName, "WAN", strlen( "WAN" ) ) ) )
								{
									strcpy( szCachLinuxEthInfos[iLinuxEthTotal], szNewName );

									iLinuxEthTotal++;
								}
							}
						}
					}

					fclose( fpLinux ); fpLinux = NULL;

					if ( iNewNameTotal != iLinuxEthTotal )
					{
						iRet = 1;

						break;
					}
					else
					{
						int i = 0, j = 0;

						for ( i=0; i<iNewNameTotal; i++ )
						{
							int iFind = 0;

							for ( j=0; j<iLinuxEthTotal; j++ )
							{
								if ( 0 == strncmp( szCachNewNameInfos[i], szCachLinuxEthInfos[j], 32 ) )
								{
									iFind = 1;

									break;
								}
							}

							if ( 0 == iFind )
							{
								iRet = 2;

								init_coordinate_log( "Info, the new eth [%d,%s] is not find in old config.", i, szCachNewNameInfos[i] );

								break;
							}
						}
					}
				}
			} while ( 0 );

			if ( fpOld )
			{
				fclose( fpOld ); fpOld = NULL;
			}

			if ( fpNew )
			{
				fclose( fpNew ); fpNew = NULL;
			}

			if ( fpLinux )
			{
				fclose( fpLinux ); fpLinux = NULL;
			}
		}
	}

	return iRet;
}

int ruijie_check_change_ext_card2( char* pOldFile, char* pNewFile )
{
	int i = 0;
	int iRet = 0;
	int iOldEthTotal = 0;
	int iNewEthTotal = 0;
	char szLine[256] = {0};
	int iOldCardEth[32] = {0};
	int iNewCardEth[32] = {0};
	char szOldVppInfos[4096] = {0};
	char szOldInterInfos[4096] = {0};
	char szCachNewEthInfos[128][32] = {0};

	if ( pOldFile && pNewFile )
	{
		// ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE
		if ( ( F_OK == access( pOldFile, F_OK ) ) && ( F_OK == access( pNewFile, F_OK ) ) )
		{
			FILE* fpOld = NULL;
			FILE* fpNew = NULL;

			do
			{
				fpOld = fopen( pOldFile, "r" );

				if ( NULL == fpOld )
				{
					iRet = -1;

					init_coordinate_log( "Error, open file [%s] failed, error:%d,%s.", pOldFile, errno, strerror( errno ) );

					break;
				}

				while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpOld ) )
				{
					if ( 0 == strncmp( szLine, "ifAutoCfgMode", strlen( "ifAutoCfgMode" ) ) )
					{
						continue;
					}

					if ( 0 == strncmp( szLine, "##ExtMap##", strlen( "##ExtMap##" ) ) )
					{
						char szExt[32] = {0};
						char szEthIndex[32] = {0};
						char szEthTotal[32] = {0};

						sscanf( szLine + strlen( "##ExtMap##" ), "%[^ ]%*[ ]%[^/]%*[/]%[^ ]", szExt, szEthIndex, szEthTotal );

						int iExt = atoi( szExt );
						int iEthTotal = atoi( szEthTotal );

						if ( ( iExt > 0 ) && ( iExt <= 32 ) && ( iEthTotal > 0 ) && ( iEthTotal <= 16 ) && ( 0 == iEthTotal % 2 ) )
						{
							if ( 0 == iOldCardEth[iExt - 1] )
							{
								iOldCardEth[iExt - 1] = iEthTotal;
							}
						}

						continue;
					}

					if ( '#' == szLine[0] )
					{
						continue;
					}

					char szPair[8] = {0};
					char szEthName[16] = {0};
					char szNewName[16] = {0};
					char szEthFlag[32] = {0};
					char szCacheLine[128] = {0};

					//eth0  MGT1  1  MANAGE
					int iItem = sscanf( szLine, "%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]", szEthName, szNewName, szPair, szEthFlag );

					if ( 4 == iItem )
					{
						snprintf( szCacheLine, sizeof( szCacheLine ) - 1, "%s %s %s %s\n", szEthName, szNewName, szPair, szEthFlag );

						if ( strlen( szOldInterInfos ) + strlen( szCacheLine ) >= sizeof( szOldInterInfos ) )
						{
							iRet = -2;

							init_coordinate_log( "Error, the cache string is small, current size %d.", sizeof( szOldInterInfos ) );

							break;
						}
						else
						{
							strcat( szOldInterInfos, szCacheLine );
						}
					}
					else
					{
						iRet = -3;

						init_coordinate_log( "Error, split string [%s] failed, item %d.", szLine, iItem );

						break;
					}

					iOldEthTotal++;
				}

				fclose( fpOld ); fpOld = NULL;

				if ( iRet )
				{
					break;
				}

				fpNew = fopen( pNewFile, "r" );

				if ( NULL == fpNew )
				{
					iRet = -4;

					init_coordinate_log( "Error, open file [%s] failed, error:%d,%s.", pNewFile, errno, strerror( errno ) );

					break;
				}

				while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpNew ) )
				{
					if ( 0 == strncmp( szLine, "ifAutoCfgMode", strlen( "ifAutoCfgMode" ) ) )
					{
						continue;
					}

					if ( 0 == strncmp( szLine, "##ExtMap##", strlen( "##ExtMap##" ) ) )
					{
						char szExt[32] = {0};
						char szEthIndex[32] = {0};
						char szEthTotal[32] = {0};

						sscanf( szLine + strlen( "##ExtMap##" ), "%[^ ]%*[ ]%[^/]%*[/]%[^ ]", szExt, szEthIndex, szEthTotal );

						int iExt = atoi( szExt );
						int iEthTotal = atoi( szEthTotal );

						if ( ( iExt > 0 ) && ( iExt <= 32 ) && ( iEthTotal > 0 ) && ( iEthTotal <= 16 ) && ( 0 == iEthTotal % 2 ) )
						{
							if ( 0 == iNewCardEth[iExt - 1] )
							{
								iNewCardEth[iExt - 1] = iEthTotal;
							}
						}

						continue;
					}

					if ( '#' == szLine[0] )
					{
						continue;
					}

					char szPair[8] = {0};
					char szEthName[16] = {0};
					char szNewName[16] = {0};
					char szEthFlag[32] = {0};
					char szCacheLine[128] = {0};

					//eth0  MGT1  1  MANAGE
					int iItem = sscanf( szLine, "%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]", szEthName, szNewName, szPair, szEthFlag );

					if ( 4 == iItem )
					{
						snprintf( szCacheLine, sizeof( szCacheLine ) - 1, "%s %s %s %s", szEthName, szNewName, szPair, szEthFlag );

						if ( NULL == strstr( szOldInterInfos, szCacheLine ) )
						{
							iRet = 1;

							init_coordinate_log( "Info, item [%s] is not in old config, is new.", szCacheLine );
						
							break;
						}
					}
					else
					{
						iRet = -5;

						init_coordinate_log( "Error, split string [%s] failed, item %d.", szLine, iItem );

						break;
					}

					iNewEthTotal++;
				}

				fclose( fpNew ); fpNew = NULL;

				if ( iRet )
				{
					break;
				}

				if ( iNewEthTotal != iOldEthTotal )
				{
					iRet = 2;

					break;
				}

				for ( i = 0; i < 32; i++ )
				{
					if ( iOldCardEth[i] != iNewCardEth[i] )
					{
						iRet = 4;

						break;
					}
				}
			} while ( 0 );
		}
		else if ( ( F_OK != access( pOldFile, F_OK ) ) && ( F_OK == access( pNewFile, F_OK ) ) )
		{
			iRet = 3;
		}
	}

	return iRet;
}

static void ruijie_auto_rename_x_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, unsigned int uiMgt)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	unsigned int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));
/*
-+-[0000:80]-+-00.0-[81]--
 |           +-01.0-[82]--
 |           +-02.0-[b0]--
 |           +-02.2-[83]--
 |           +-03.0-[84]--
 |           +-05.0  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 VTd/Memory Map/Misc
 |           +-05.2  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 IIO RAS
 |           \-05.4  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 IOAPIC
 \-[0000:00]-+-00.0  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 DMI2
             +-01.0-[01-06]----00.0-[02-06]--+-01.0-[03-04]--+-00.0  Intel Corporation I350 Gigabit Network Connection
             |                               |               +-00.1  Intel Corporation I350 Gigabit Network Connection
             |                               |               +-00.2  Intel Corporation I350 Gigabit Network Connection
             |                               |               \-00.3  Intel Corporation I350 Gigabit Network Connection
             |                               \-03.0-[05-06]--+-00.0  Intel Corporation I350 Gigabit Network Connection
             |                                               +-00.1  Intel Corporation I350 Gigabit Network Connection
             |                                               +-00.2  Intel Corporation I350 Gigabit Network Connection
             |                                               \-00.3  Intel Corporation I350 Gigabit Network Connection
             +-02.0-[30]--
             +-02.2-[07-0c]----00.0-[08-0c]--+-01.0-[09-0a]--+-00.0  Intel Corporation I350 Gigabit Network Connection
             |                               |               +-00.1  Intel Corporation I350 Gigabit Network Connection
             |                               |               +-00.2  Intel Corporation I350 Gigabit Network Connection
             |                               |               \-00.3  Intel Corporation I350 Gigabit Network Connection
             |                               \-03.0-[0b-0c]--+-00.0  Intel Corporation I350 Gigabit Network Connection
             |                                               +-00.1  Intel Corporation I350 Gigabit Network Connection
             |                                               +-00.2  Intel Corporation I350 Gigabit Network Connection
             |                                               \-00.3  Intel Corporation I350 Gigabit Network Connection
             +-03.0-[0d]--
             +-03.2-[0e]--
             +-05.0  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 VTd/Memory Map/Misc
             +-05.2  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 IIO RAS
             +-05.4  Intel Corporation Xeon E7 v2/Xeon E5 v2/Core i7 IOAPIC
             +-11.0-[0f]--
             +-1a.0  Intel Corporation C600/X79 series chipset USB2 Enhanced Host Controller #2
             +-1c.0-[10]----00.0  Intel Corporation 82583V Gigabit Network Connection
             +-1c.1-[11]----00.0  Intel Corporation 82583V Gigabit Network Connection
             +-1c.7-[12-13]----00.0-[13]----00.0  ASPEED Technology, Inc. ASPEED Graphics Family
             +-1d.0  Intel Corporation C600/X79 series chipset USB2 Enhanced Host Controller #1
             +-1e.0-[14]--
             +-1f.0  Intel Corporation C600/X79 series chipset LPC Controller
             +-1f.2  Intel Corporation C600/X79 series chipset 6-Port SATA AHCI Controller
             \-1f.3  Intel Corporation C600/X79 series chipset SMBus Host Controller

 */
	unlink( ACE_PORT_MAP_FILE ".old" );

	if ( F_OK == access( ACE_PORT_MAP_FILE, F_OK ) )
	{
		rename( ACE_PORT_MAP_FILE, ACE_PORT_MAP_FILE ".old" );
	}

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );

		fgets( line, sizeof( line ), fp );

		fclose( fp );

		fp = NULL;

		if ( strstr( line, "####ManMakeExtMap" ) && ( sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make )
		{
			init_coordinate_log( "Info, use defined interface." );

			goto error_out;
		}
	}

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while ( 0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
                p_char = strcasestr(line, "Intel Corporation");
                if (p_char)
                    *p_char = 0;
                
				p_char = strrchr(line, '-');
				if (p_char)
				{
					if (eth_index > MAX_ETH_ONE_CARD)
					{
						goto error_out;
					}
					if (ruijie_x_serial_ext_card_number)
					{
						/**0000:b2:00.0**/
						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
						/**8086:1521**/
						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
                        if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
                        {
                            if (card_id[card_index][1] != '\0')
                            {
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
                            }
                            else
                            {
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
                            }
                        }
					}
					eth_index++;
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose( fp );
		fp = NULL;
	}
	if (ruijie_x_serial_ext_card_number)
	{
        //printf("%s %s %d ruijie_x_serial_ext_card_number%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_number);

		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);

			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = (eth_num&1);
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", (eth_num&1)?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
                    if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
                        snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
                    else
					    snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
					nm_index++;
				}
                //printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                //	ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName);
			}
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
		if (fp)
		{
			sprintf( line, "ifAutoCfgMode %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
			sprintf( line, "####ManMakeExtMap %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
		}
#if 0
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", ruijie_x_serial_ext_card_entry[sorted_num].cardId, eth_index + 1, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );

				sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );
			}
		}
#else
		int x = 0, y = 0;
		int iMgtPort = 1;
		int iPortIndex = 0;
		char iCardIndex = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			int iFind = 0;
			char szMgt[16] = {0};

			if ( iMgtPort <= uiMgt )
			{
				snprintf( szMgt, sizeof( szMgt ) - 1, "MGT%d", iMgtPort ); iMgtPort++;
			}
			else
			{
				iCardIndex += 1;
			}

			for ( x = 0; x < ruijie_x_serial_ext_card_number; ++x )
			{
				for ( y = 0; y < ruijie_x_serial_ext_card_entry[x].ethNum; ++y )
				{
					if ( szMgt[0] )
					{
						if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[x].ethCard[y].ethNewName, szMgt ) )
						{
							iFind = 1;
						}
					}
					else
					{
						if ( '1' <= ruijie_x_serial_ext_card_entry[x].cardId && ruijie_x_serial_ext_card_entry[x].cardId <= '9' )
						{
							if ( iCardIndex == ( ruijie_x_serial_ext_card_entry[x].cardId - '0' ) )
							{
								iFind = 1;
							}
						}
						else if ( 'A' <= ruijie_x_serial_ext_card_entry[x].cardId && ruijie_x_serial_ext_card_entry[x].cardId <= 'F' )
						{
							if ( iCardIndex == ( ruijie_x_serial_ext_card_entry[x].cardId - 'A' + 10 ) )
							{
								iFind = 1;
							}
						}
					}

					if ( iFind )
					{
						int iPair = ruijie_x_serial_ext_card_entry[x].ethCard[y].ethPair;

						if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[x].ethCard[y].ethNewName, "MGT", 3 ) )
						{
							iPair = 1;
						}

						sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", ruijie_x_serial_ext_card_entry[x].cardId, y + 1, ruijie_x_serial_ext_card_entry[x].ethNum,
							ruijie_x_serial_ext_card_entry[x].ethCard[y].ethOldName, 
							ruijie_x_serial_ext_card_entry[x].ethCard[y].ethNewName, 
							iPair,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[x].ethCard[y].ethAttribute]));
						fwrite(line, strlen( line ), sizeof( char ), fp );

						sprintf(line, "##PortMap##%d %s %s\n",
							iPortIndex,
							ruijie_x_serial_ext_card_entry[x].ethCard[y].ethOldName, 
							ruijie_x_serial_ext_card_entry[x].ethCard[y].ethNewName
						);

						fwrite(line, strlen( line ), sizeof( char ), fp );

						sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[x].ethCard[y].ethOldName, 
							ruijie_x_serial_ext_card_entry[x].ethCard[y].ethNewName, 
							iPair,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[x].ethCard[y].ethAttribute]));
						fwrite(line, strlen( line ), sizeof( char ), fp );

						iPortIndex++;
					}
				}

				if ( iFind )
				{
					break;
				}
			}
		}
#endif
		fclose(fp);
		fp = NULL;
#endif
	}

	// check ext change
	{
		int iChanged = ruijie_check_change_ext_card( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );

		if ( iChanged > 0 )
		{
			unlink( ACE_PORT_MAP_FILE ".old" );

			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is changed." );
		}
		else
		{
			if ( F_OK == access( ACE_PORT_MAP_FILE ".old", F_OK ) )
			{
				unlink( ACE_PORT_MAP_FILE );

				rename( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is not changed." );
		}
	}

error_out:
	if (fp)
	{
		pclose(fp);
	}
}


/**2015-01-20 zhang.huanan for new Hardware:**/
static const char* ruijie_x60_ext_card_feature_string[] = 
{
	"       +-01.0",
	"       +-02.0",
	"       +-02.2",
	"       +-1c.0-",
	"       +-1c.1-",
};
const static char ruijie_x60_ext_card_id_char[][3] = 
{
	"1",
	"3",
	"2",
	"n",
	"n",
};

static void ruijie_auto_rename_x60_ext_card(unsigned int eth_total)
{
	ruijie_auto_rename_x_serial_ext_card(ruijie_x60_ext_card_feature_string, ruijie_x60_ext_card_id_char, sizeof(ruijie_x60_ext_card_id_char)/sizeof(ruijie_x60_ext_card_id_char[0]), eth_total, 2);
}

/**2015-01-20 zhang.huanan for new Hardware:**/
static const char* ruijie_x20_ext_card_feature_string[] = 
{
	"       +-01.0",
	"       +-02.0",
	"       +-02.2",
	"       +-1c.0-",
	"       +-1c.1-",
};
const static char ruijie_x20_ext_card_id_char[][3] = 
{
	"1",
	"3",
	"2",
	"n",
	"n",
};

static void ruijie_auto_rename_x20_ext_card(unsigned int eth_total)
{
	ruijie_auto_rename_x_serial_ext_card(ruijie_x20_ext_card_feature_string, ruijie_x20_ext_card_id_char, sizeof(ruijie_x20_ext_card_id_char) / sizeof(ruijie_x20_ext_card_id_char[0]), eth_total, 2);
}

static const char* ruijie_x20_new_ext_card_feature_string[] = 
{
	"       +-01.0-",
	"       +-01.1-",
	"       +-1c.0-",
	"       +-1c.4-",
	"       +-1b.0-",
	"       +-1b.1-",
};
const static char ruijie_x20_new_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"n2",
	"n1",
};

static void ruijie_auto_rename_x20_new_ext_card(unsigned int eth_total)
{
	ruijie_auto_rename_x_serial_ext_card(ruijie_x20_new_ext_card_feature_string, ruijie_x20_new_ext_card_id_char, sizeof(ruijie_x20_new_ext_card_id_char) / sizeof(ruijie_x20_new_ext_card_id_char[0]), eth_total, 2);
}

static const char* ruijie_x60_new_ext_card_feature_string[] = 
{
	"       +-01.0-",
	"       +-02.0-",
	"       +-02.2-",
	"       +-03.0-",
	"       +-1c.1-",
	"       +-1c.2-",
};
const static char ruijie_x60_new_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"n1",
	"n2",
};

static void ruijie_auto_rename_x60_new_ext_card(unsigned int eth_total)
{
	ruijie_auto_rename_x_serial_ext_card(ruijie_x60_new_ext_card_feature_string, ruijie_x60_new_ext_card_id_char, sizeof(ruijie_x60_new_ext_card_id_char) / sizeof(ruijie_x60_new_ext_card_id_char[0]), eth_total, 2);
}

#ifndef ARCH_ARM64
static void ruijie_auto_rename_x200_ext_card(unsigned int eth_total)
{
    const static int ruijie_x200_ext_card_id[] = {1, 2, 0, 0, 3, 4};     //0 means single port, other means card id
    const static char ruijie_x200_ext_card_order[][3] = {"1", "2", "n2", "n1", "3", "4"}; //- means mgt, number means order

	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	unsigned int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int man_make = 0;

	unlink( ACE_PORT_MAP_FILE ".old" );

	if ( F_OK == access( ACE_PORT_MAP_FILE, F_OK ) )
	{
		rename( ACE_PORT_MAP_FILE, ACE_PORT_MAP_FILE ".old" );
	}

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );

		fgets( line, sizeof( line ), fp );

		fclose( fp );

		fp = NULL;

		if ( strstr( line, "####ManMakeExtMap" ) && ( sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make )
		{
			init_coordinate_log( "Info, use defined interface." );

			goto error_out;
		}
	}

    ruijie_x_serial_ext_card_number = 0;
    for (card_index = 0; card_index < sizeof(ruijie_x200_ext_card_id) / sizeof(ruijie_x200_ext_card_id[0]); card_index++)
    {
        if ( ruijie_x200_ext_card_id[card_index] == 0 )
        {
			int iCardId = 0;

			if ( 'n' != ruijie_x200_ext_card_order[card_index][0] )
			{
				iCardId = ruijie_x200_ext_card_order[card_index][0];
			}
			else
			{
				iCardId += (ruijie_x200_ext_card_order[card_index][1] - '0');
			}

			ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = iCardId;
			ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].ethNum = 1;
            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].ethCard[eth_index].ethMgtIdx = ruijie_x200_ext_card_order[card_index][1] - '0';
        }
        else
        {
            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = ruijie_x200_ext_card_order[card_index][0];
			ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].ethNum = x200_get_eth_num_by_cardid(ruijie_x200_ext_card_id[card_index]);
        }
#if 0
		if ( ruijie_x200_ext_card_id[card_index] == 0 )
			printf( "%d, cardid:%c, ethNum:%d, ethMgtIdx:%d\n", ruijie_x_serial_ext_card_number, ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId,
				ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].ethNum, ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].ethCard[0].ethMgtIdx );
		else
			printf( "%d, cardid:%c, ethNum:%d, ethMgtIdx:ext\n", ruijie_x_serial_ext_card_number, ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId,
				ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].ethNum );
#endif
        ++ruijie_x_serial_ext_card_number;
    }
    
	fp = popen("lspci | grep Ethernet | awk {'print $1'}", "r");
	if ( fp )
	{
        card_index = 0;
        eth_num = 0;
		memset(line, 0x00, sizeof(line));
		while ( 0 != fgets(line, sizeof(line), fp ) )
		{
            while (eth_num >= ruijie_x_serial_ext_card_entry[card_index].ethNum)
            {
                eth_num = 0;
                card_index++;
                
                if (card_index >= ruijie_x_serial_ext_card_number)
                    break;
            }

            if (card_index >= ruijie_x_serial_ext_card_number)
                break;

            sscanf(line, "%[^\r\n]", ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_num].ethPciName);
          	//printf("%s %d cardid %d eth %d ethPciName=%s\n", __FUNCTION__, __LINE__, 
            //    ruijie_x_serial_ext_card_entry[card_index].cardId, eth_num, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_num].ethPciName);

            eth_num++;
		}
		pclose( fp );
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%d Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%d Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}

		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%d Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);

			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > '0')
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = (eth_num&1);
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", (eth_num&1)?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
                    snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
					nm_index++;
				}
                //printf("%s %s %d sort %d Ext%d No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                //	ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName,
                //	ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
			}
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}

		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
		if ( fp )
		{
			sprintf( line, "ifAutoCfgMode %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
			sprintf( line, "####ManMakeExtMap %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
		}

		int iPortIndex = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				char chCardId = ruijie_x_serial_ext_card_entry[sorted_num].cardId;

				if ( chCardId < '0' )
				{
					chCardId = 'n';
				}

				sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", chCardId, eth_index + 1, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );
#if 1
				sprintf(line, "##PortMap##%d %s %s\n",
					iPortIndex,
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
				);

				fwrite(line, strlen( line ), sizeof( char ), fp );
#endif
				sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );

				iPortIndex++;
			}
		}

		fclose( fp );

		fp = NULL;
	}

	// check ext change
	{
		int iChanged = ruijie_check_change_ext_card( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );

		if ( iChanged > 0 )
		{
			unlink( ACE_PORT_MAP_FILE ".old" );

			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is changed." );
		}
		else
		{
			if ( F_OK == access( ACE_PORT_MAP_FILE ".old", F_OK ) )
			{
				unlink( ACE_PORT_MAP_FILE );

				rename( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is not changed." );
		}
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}
#endif


static const char* ruijie_C3558_ext_card_feature_string[] = 
{
	"       +-09.0-",
	"       +-0a.0-",
	"       +-0b.0-",
	"       +-0c.0-",
	"       +-0e.0-",
	"       +-0f.0-",
	"       +-16.0-",
	"       +-17.0-",
};
const static char ruijie_C3558_ext_card_id_char[][3] = 
{
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"1",
	"2",
};

static void ruijie_C3558_auto_rename_x_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int iType )
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	unsigned int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));
/*
-[0000:00]-+-00.0  Intel Corporation Device 1980
           +-04.0  Intel Corporation Device 19a1
           +-05.0  Intel Corporation Device 19a2
           +-06.0-[01]----00.0  Intel Corporation Device 19e2
           +-09.0-[02]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0a.0-[03]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0b.0-[04]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0c.0-[05]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0e.0-[06]--
           +-0f.0-[07-0a]----00.0-[08-0a]--+-02.0-[09]--+-00.0  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            +-00.1  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            +-00.2  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            \-00.3  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               \-03.0-[0a]--+-00.0  Intel Corporation I350 Gigabit Network Connection
           |                                            +-00.1  Intel Corporation I350 Gigabit Network Connection
           |                                            +-00.2  Intel Corporation I350 Gigabit Network Connection
           |                                            \-00.3  Intel Corporation I350 Gigabit Network Connection
           +-10.0-[0b-0c]----00.0-[0c]----00.0  ASPEED Technology, Inc. ASPEED Graphics Family
           +-12.0  Intel Corporation Device 19ac
           +-14.0  Intel Corporation Device 19c2
           +-15.0  Intel Corporation Device 19d0
           +-16.0-[0d]--+-00.0  Intel Corporation Device 15c4
           |            \-00.1  Intel Corporation Device 15c4
           +-17.0-[0e]--+-00.0  Intel Corporation Device 15e5
           |            \-00.1  Intel Corporation Device 15e5
           +-18.0  Intel Corporation Device 19d3
           +-1f.0  Intel Corporation Device 19dc
           +-1f.2  Intel Corporation Device 19de
           +-1f.4  Intel Corporation Device 19df
           \-1f.5  Intel Corporation Device 19e0
 */
#if 0
	if ( 132 == iType )
	{
		unlink( ACE_PORT_MAP_FILE ".old" );

		if ( F_OK == access( ACE_PORT_MAP_FILE, F_OK ) )
		{
			rename( ACE_PORT_MAP_FILE, ACE_PORT_MAP_FILE ".old" );
		}
	}
#endif
	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if (fp)
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose(fp);
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			init_coordinate_log( "Info, use defined interface." );
			goto error_out;
		}
	}
	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while ( 0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
                if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Intel Corporation Device 15c4")
                    || strcasestr(line, "Intel Corporation Device 15e5")
                    || strcasestr(line, "Device 8086:1533")
                    || strcasestr(line, "Device 8086:15c4")
                    || strcasestr(line, "Device 8086:15e5")
                    )
				{
                    p_char = strcasestr(line, "Intel Corporation");
                    if (p_char)
                        *p_char = 0;
                    
    				p_char = strrchr(line, '-');
    				if (p_char)
    				{
    					if (eth_index > MAX_ETH_ONE_CARD)
    					{
    						goto error_out;
    					}
    					if (ruijie_x_serial_ext_card_number)
    					{
    						/**0000:b2:00.0**/
    						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
    						/**8086:1521**/
    						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
                            if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
                            {
                                if (card_id[card_index][1] != '\0')
                                {
                                    ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
                                }
                                else
                                {
                                    ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
                                }
                            }
    					}
    					eth_index++;
    					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
    				}
                }
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose( fp );
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;

		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}

			sorted_num = eth_index;

			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}

				if (sorted_num == card_index)
				{
					continue;
				}

				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		eth_num = 0;

		nm_index = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( eth_num >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ( eth_num & 1 );

					if ( ( 132 == iType ) && ( ruijie_x_serial_ext_card_entry[sorted_num].cardId == '7' ) )
					{
						snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "HA" );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
					}
					else
					{
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", (eth_num&1)?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					}

					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;

					if ( 132 == iType )
					{
						snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%s", "MGMT" );
					}
					else
					{
						if ( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0 )
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%s%d", "MGT", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
						else
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%s%d", "MGT", nm_index + 1);
					}

					nm_index++;
				}
			}
		}

		{
			DIR *dir;
			struct dirent *entry;
			
			dir = opendir("/sys/class/net");
			if (dir) {
				while ((entry = readdir(dir)) != NULL) {
					if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
						continue;
					}
					
					if (strncmp(entry->d_name, "eth", 3)) {
						continue;
					}
							
					snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/%s/device", entry->d_name);
					fp = popen(tmpstr, "r");
					if ( fp )
					{
						memset(line, 0x00, sizeof(line));
						if (0 != fgets(line, sizeof(line), fp ))
						{
							line_size = strlen(line);
							/**remove '\n'**/
							line[line_size - 1] = 0;
							--line_size;

							p_char = strrchr(line, '/');
							if (p_char)
							{
								p_char += 6;
								//printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

								for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
								{
									for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
									{
										//printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
										//	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

										if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
										{
											snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "%s", entry->d_name);
											//printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
											//	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
											break;
										}
									}
								}
							}
						}
						pclose(fp);
						fp = NULL;
					}
				}
				closedir(dir);
			}
		}
#if 1
		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
		if (fp)
		{
			sprintf( line, "ifAutoCfgMode %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
			sprintf( line, "####ManMakeExtMap %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
		}

		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
#if 1
				if ( 132 == iType )
				{
					int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair;
					int iPairIndex = 0;

					if ( 5 == iPair )
					{
						iPair = 0;

						if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}
					}
					else
					{
						if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}
					}

					sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", '1', iPair * 2 + iPairIndex, eth_total,
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );

					sprintf(line, "##PortMap##%d %s %s\n",
						iPair * 2 + iPairIndex,
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
					);

					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				else
				{
					sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", ruijie_x_serial_ext_card_entry[sorted_num].cardId, eth_index + 1, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
#endif
				sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );
			}
		}
		fclose(fp);
		fp = NULL;
#endif
	}

	// check ext change
#if 0
	if ( 132 == iType )
	{
		int iChanged = ruijie_check_change_ext_card( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );

		if ( iChanged > 0 )
		{
			unlink( ACE_PORT_MAP_FILE ".old" );

			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is changed." );
		}
		else
		{
			if ( F_OK == access( ACE_PORT_MAP_FILE ".old", F_OK ) )
			{
				unlink( ACE_PORT_MAP_FILE );

				rename( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is not changed." );
		}
	}
#endif
error_out:
	if (fp)
	{
		pclose(fp);
	}

	return;
}


static void ruijie_auto_rename_C3558_ext_card(unsigned int eth_total)
{
	ruijie_C3558_auto_rename_x_serial_ext_card(ruijie_C3558_ext_card_feature_string, ruijie_C3558_ext_card_id_char, sizeof(ruijie_C3558_ext_card_id_char)/sizeof(ruijie_C3558_ext_card_id_char[0]), eth_total, 0);
}

static const char* ruijie_nis132_c3558_ext_card_feature_string[] = 
{
	"       +-09.0-",
	"       +-0a.0-",
	"       +-0b.0-",
	"       +-0c.0-",
	"       +-0e.0-",
	"       +-0f.0-",
	"       +-16.0-",
	"       +-17.0-",
};
const static char ruijie_nis132_c3558_ext_card_id_char[][3] = 
{
	"n",
	"7",
	"1",
	"2",
	"3",
	"4",
	"6",
	"5",
};

static void ruijie_auto_rename_nis132_c3558_ext_card(unsigned int eth_total)
{
	ruijie_C3558_auto_rename_x_serial_ext_card(ruijie_nis132_c3558_ext_card_feature_string, ruijie_nis132_c3558_ext_card_id_char, sizeof(ruijie_nis132_c3558_ext_card_id_char) / sizeof(ruijie_nis132_c3558_ext_card_id_char[0]), eth_total, 132);

	return;
}

static const char* ruijie_nis_128_C3558_ext_card_feature_string[] = 
{
	"       +-09.0-",
	"       +-0a.0-",
	"       +-0b.0-",
	"       +-0c.0-",
	"       +-0e.0-",
	"       +-0f.0-",
	"       +-16.0-",
	"       +-17.0-",
};

const static char ruijie_nis_128_C3558_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"7",
	"6",
	"5",
	"8",
};

static void ruijie_nis_128_C3558_auto_rename_x_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	unsigned int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));
/*
-[0000:00]-+-00.0  Intel Corporation Device 1980
           +-04.0  Intel Corporation Device 19a1
           +-05.0  Intel Corporation Device 19a2
           +-06.0-[01]----00.0  Intel Corporation Device 19e2
           +-09.0-[02]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0a.0-[03]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0b.0-[04]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0c.0-[05]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0e.0-[06]--
           +-0f.0-[07-0a]----00.0-[08-0a]--+-02.0-[09]--+-00.0  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            +-00.1  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            +-00.2  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            \-00.3  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               \-03.0-[0a]--+-00.0  Intel Corporation I350 Gigabit Network Connection
           |                                            +-00.1  Intel Corporation I350 Gigabit Network Connection
           |                                            +-00.2  Intel Corporation I350 Gigabit Network Connection
           |                                            \-00.3  Intel Corporation I350 Gigabit Network Connection
           +-10.0-[0b-0c]----00.0-[0c]----00.0  ASPEED Technology, Inc. ASPEED Graphics Family
           +-12.0  Intel Corporation Device 19ac
           +-14.0  Intel Corporation Device 19c2
           +-15.0  Intel Corporation Device 19d0
           +-16.0-[0d]--+-00.0  Intel Corporation Device 15c4
           |            \-00.1  Intel Corporation Device 15c4
           +-17.0-[0e]--+-00.0  Intel Corporation Device 15e5
           |            \-00.1  Intel Corporation Device 15e5
           +-18.0  Intel Corporation Device 19d3
           +-1f.0  Intel Corporation Device 19dc
           +-1f.2  Intel Corporation Device 19de
           +-1f.4  Intel Corporation Device 19df
           \-1f.5  Intel Corporation Device 19e0
 */
	unlink( ACE_PORT_MAP_FILE ".old" );

	if ( F_OK == access( ACE_PORT_MAP_FILE, F_OK ) )
	{
		rename( ACE_PORT_MAP_FILE, ACE_PORT_MAP_FILE ".old" );
	}

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if (fp)
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose(fp);
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			init_coordinate_log( "Info, use defined interface." );
			goto error_out;
		}
	}
	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while ( 0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					
					if ( strcasestr(line, " I350 Gigabit ") )
					{
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 1;
					}
					else if ( strcasestr(line, " 82599ES 10-Gigabit SFI/SFP+ ") )
					{
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 2;
					}
					else
					{
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
                if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Intel Corporation Device 15c4")
                    || strcasestr(line, "Intel Corporation Device 15e5")
                    || strcasestr(line, "Device 8086:1533")
                    || strcasestr(line, "Device 8086:15c4")
                    || strcasestr(line, "Device 8086:15e5")
                    )
				{
                    p_char = strcasestr(line, "Intel Corporation");
                    if (p_char)
                        *p_char = 0;
                    
    				p_char = strrchr(line, '-');
    				if (p_char)
    				{
    					if (eth_index > MAX_ETH_ONE_CARD)
    					{
    						goto error_out;
    					}
    					if (ruijie_x_serial_ext_card_number)
    					{
    						/**0000:b2:00.0**/
    						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
    						/**8086:1521**/
    						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
                            if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
                            {
                                if (card_id[card_index][1] != '\0')
                                {
                                    ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
                                }
                                else
                                {
                                    ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
                                }
                            }
    					}
    					eth_index++;
    					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
    				}
                }
				
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose( fp );
		fp = NULL;
	}
	if (ruijie_x_serial_ext_card_number)
	{
        //printf("%s %s %d ruijie_x_serial_ext_card_number%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_number);

		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);

			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if ( 1 == ruijie_x_serial_ext_card_entry[sorted_num].mode && ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum ) )
				{
					int iEthNum = 0;
					const static unsigned int map[4] = {2, 3, 0, 1};

					for ( iEthNum=0; iEthNum < 4; ++iEthNum )
					{
						int ethAttribute = ( ( eth_num + map[iEthNum] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethPair = 1 + ( ( eth_num + map[iEthNum] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethPair );
					}

					eth_num += 4;
					eth_index = 4;
				}
				else if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].mode && ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum ) )
				{
					int iEthNum = 0;
					const static unsigned int map[2] = {1, 0};

					for ( iEthNum=0; iEthNum < 2; ++iEthNum )
					{
						int ethAttribute = ( ( eth_num + map[iEthNum] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethPair = 1 + ( ( eth_num + map[iEthNum] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[iEthNum].ethPair );
					}

					eth_num += 2;
					eth_index = 2;
				}
				else if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != '8')
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = (eth_num&1);
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", (eth_num&1)?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ((eth_num)>>1);
                    if (eth_index != 0)
					{
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "HA");
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
					}
                    else
					{
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%s", "MGMT");
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
					}
					eth_num++;
				}
                //printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                //	ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName);
			}
		}

		{
			DIR *dir;
			struct dirent *entry;
			
			dir = opendir("/sys/class/net");
			if (dir) {
				while ((entry = readdir(dir)) != NULL) {
					if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
						continue;
					}
					
					if (strncmp(entry->d_name, "eth", 3)) {
						continue;
					}
							
					snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/%s/device", entry->d_name);
					fp = popen(tmpstr, "r");
					if ( fp )
					{
						memset(line, 0x00, sizeof(line));
						if (0 != fgets(line, sizeof(line), fp ))
						{
							line_size = strlen(line);
							/**remove '\n'**/
							line[line_size - 1] = 0;
							--line_size;

							p_char = strrchr(line, '/');
							if (p_char)
							{
								p_char += 6;
								//printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

								for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
								{
									for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
									{
										//printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
										//	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

										if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
										{
											snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "%s", entry->d_name);
											//printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
											//	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
											break;
										}
									}
								}
							}
						}
						pclose(fp);
						fp = NULL;
					}
				}
				closedir(dir);
			}
		}
#if 1
		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
		if (fp)
		{
			sprintf( line, "ifAutoCfgMode %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
			sprintf( line, "####ManMakeExtMap %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
		}

		int iExtCardIndex = 0;

		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			
			if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == 1 && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				int eth_count = 0;
				const static unsigned int map[4] = {2, 3, 0, 1};

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", '2', iExtCardIndex + 1, 8,
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
								(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
							fwrite(line, strlen( line ), sizeof( char ), fp );

#if 1
							int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair;
							int iPairIndex = 0;
		
							if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
							{
								iPairIndex = 1;
							}
		
							sprintf(line, "##PortMap##%d %s %s\n",
								iPair * 2 + iPairIndex,
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
							);
		
							fwrite(line, strlen( line ), sizeof( char ), fp );
#endif
							sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
								(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
							fwrite(line, strlen( line ), sizeof( char ), fp );

							iExtCardIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == 2 && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				int eth_count = 0;
				const static unsigned int map[2] = {1, 0};

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", '2', iExtCardIndex + 1, 4,
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
								(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
							fwrite(line, strlen( line ), sizeof( char ), fp );
#if 1
							int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair;
							int iPairIndex = 0;
		
							if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
							{
								iPairIndex = 1;
							}
		
							sprintf(line, "##PortMap##%d %s %s\n",
								iPair * 2 + iPairIndex,
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
							);
		
							fwrite(line, strlen( line ), sizeof( char ), fp );
#endif

							sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
								ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
								(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
							fwrite(line, strlen( line ), sizeof( char ), fp );

							iExtCardIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			else if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
#if 1
					int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair;
					int iPairIndex = 0;

					if ( ( 'M' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] ) || ( 'H' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] ) )
					{
						iPair = 0;
					
						if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}
					}
					else
					{
						if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}
					}

					sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", '1', iPair * 2 + iPairIndex, 8,
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );

					sprintf(line, "##PortMap##%d %s %s\n",
						iPair * 2 + iPairIndex,
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
					);

					fwrite(line, strlen( line ), sizeof( char ), fp );
#endif
					sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
			}
		}
		fclose(fp);
		fp = NULL;
#endif
	}

	// check ext change
	{
		int iChanged = ruijie_check_change_ext_card( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );

		if ( iChanged > 0 )
		{
			unlink( ACE_PORT_MAP_FILE ".old" );

			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is changed." );
		}
		else
		{
			if ( F_OK == access( ACE_PORT_MAP_FILE ".old", F_OK ) )
			{
				unlink( ACE_PORT_MAP_FILE );

				rename( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is not changed." );
		}
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static const char* ruijie_C3758_ext_card_feature_string[] = 
{
	"       +-09.0-",
	"       +-0a.0-",
	"       +-0b.0-",
	"       +-0c.0-",
	"       +-0e.0-",
	"       +-0f.0-",
	"       +-16.0-",
	"       +-17.0-",
};

const static char ruijie_C3758_ext_card_id_char[][3] = 
{
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"1",
	"2",
};

static void ruijie_C3758_auto_rename_x_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	unsigned int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));
/*
-[0000:00]-+-00.0  Intel Corporation Device 1980
           +-04.0  Intel Corporation Device 19a1
           +-05.0  Intel Corporation Device 19a2
           +-06.0-[01]----00.0  Intel Corporation Device 19e2
           +-09.0-[02]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0a.0-[03]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0b.0-[04]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0c.0-[05]----00.0  Intel Corporation I210 Gigabit Network Connection
           +-0e.0-[06]--
           +-0f.0-[07-0a]----00.0-[08-0a]--+-02.0-[09]--+-00.0  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            +-00.1  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            +-00.2  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               |            \-00.3  Intel Corporation I350 Gigabit Fiber Network Connection
           |                               \-03.0-[0a]--+-00.0  Intel Corporation I350 Gigabit Network Connection
           |                                            +-00.1  Intel Corporation I350 Gigabit Network Connection
           |                                            +-00.2  Intel Corporation I350 Gigabit Network Connection
           |                                            \-00.3  Intel Corporation I350 Gigabit Network Connection
           +-10.0-[0b-0c]----00.0-[0c]----00.0  ASPEED Technology, Inc. ASPEED Graphics Family
           +-12.0  Intel Corporation Device 19ac
           +-14.0  Intel Corporation Device 19c2
           +-15.0  Intel Corporation Device 19d0
           +-16.0-[0d]--+-00.0  Intel Corporation Device 15c4
           |            \-00.1  Intel Corporation Device 15c4
           +-17.0-[0e]--+-00.0  Intel Corporation Device 15e5
           |            \-00.1  Intel Corporation Device 15e5
           +-18.0  Intel Corporation Device 19d3
           +-1f.0  Intel Corporation Device 19dc
           +-1f.2  Intel Corporation Device 19de
           +-1f.4  Intel Corporation Device 19df
           \-1f.5  Intel Corporation Device 19e0
 */
	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if (fp)
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose(fp);
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			goto error_out;
		}
	}
	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while ( 0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
                if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Intel Corporation Device 15c4")
                    || strcasestr(line, "Intel Corporation Device 15e4"))
				{
                    p_char = strcasestr(line, "Intel Corporation");
                    if (p_char)
                        *p_char = 0;
                    
    				p_char = strrchr(line, '-');
    				if (p_char)
    				{
    					if (eth_index > MAX_ETH_ONE_CARD)
    					{
    						goto error_out;
    					}
    					if (ruijie_x_serial_ext_card_number)
    					{
    						/**0000:b2:00.0**/
    						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
    						/**8086:1521**/
    						snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
                            if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
                            {
                                if (card_id[card_index][1] != '\0')
                                {
                                    ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
                                }
                                else
                                {
                                    ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
                                }
                            }
    					}
    					eth_index++;
    					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
    				}
                }
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose( fp );
		fp = NULL;
	}
	if (ruijie_x_serial_ext_card_number)
	{
        //printf("%s %s %d ruijie_x_serial_ext_card_number%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_number);

		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);

			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = (eth_num&1);
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", (eth_num&1)?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
                    if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
                        snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
                    else
					    snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
					nm_index++;
				}
                //printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                //	ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName);
			}
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
		if (fp)
		{
			sprintf( line, "ifAutoCfgMode %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
			sprintf( line, "####ManMakeExtMap %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
		}

		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				sprintf(line, "##ExtMap##%c %d/%d %s %s %d %s\n", ruijie_x_serial_ext_card_entry[sorted_num].cardId, eth_index + 1, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );

				sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );
			}
		}
		fclose(fp);
		fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static void ruijie_auto_rename_C3758_ext_card(unsigned int eth_total)
{
	ruijie_C3758_auto_rename_x_serial_ext_card(ruijie_C3758_ext_card_feature_string, ruijie_C3758_ext_card_id_char, sizeof(ruijie_C3758_ext_card_id_char)/sizeof(ruijie_C3758_ext_card_id_char[0]), eth_total);
}

static const char* ruijie_rename_e10_13t_5e_4c_4f[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
	"LAN6",
	"WAN6",
	"MGT1"
};
static int ruijie_sorted_e10_13t_5e_4c_4f[] = {9, 10, 11, 12, 0, 1, 2, 3, 4, 5, 6, 7, 8};
static int ruijie_pair_e10_13t_5e_4c_4f[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7};
static int ruijie_attribute_e10_13t_5e_4c_4f[] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2};

static const char* ruijie_rename_e10_5t_5e_no_ext[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"MGT1"
};
static int ruijie_sorted_e10_5t_5e_no_ext[] = {1, 2, 3, 4, 0};
static int ruijie_pair_e10_5t_5e_no_ext[] = {1, 1, 2, 2, 3};
static int ruijie_attribute_e10_5t_5e_no_ext[] = {0, 1, 0, 1, 2};

static const char* ruijie_rename_e10_13t_5e_8c[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
	"LAN6",
	"WAN6",
	"MGT1"
};
static int ruijie_sorted_e10_13t_5e_8c[] = {9, 10, 11, 12, 0, 1, 2, 3, 4, 5, 6, 7, 8};
static int ruijie_pair_e10_13t_5e_8c[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7};
static int ruijie_attribute_e10_13t_5e_8c[] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2};
static const char* ruijie_rename_e50_14t_6e_4c_4f[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
	"LAN6",
	"WAN6",
	"LAN7",
	"WAN7",
};

static int ruijie_sorted_e50_14t_6e_4c_4f[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static int ruijie_pair_e50_14t_6e_4c_4f[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
static int ruijie_attribute_e50_14t_6e_4c_4f[] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

static const char* ruijie_rename_e50_6t_6e_no_ext[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",

};

static int ruijie_sorted_e50_6t_6e_no_ext[] = {0, 1, 2, 3, 4, 5};
static int ruijie_pair_e50_6t_6e_no_ext[] = {1, 1, 2, 2, 3, 3,};
static int ruijie_attribute_e50_6t_6e_no_ext[] = {0, 1, 0, 1, 0, 1};

static const char* ruijie_rename_e50_14t_6e_8c[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
	"LAN6",
	"WAN6",
	"LAN7",
	"WAN7",
};

static int ruijie_sorted_e50_14t_6e_8c[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
static int ruijie_pair_e50_14t_6e_8c[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
static int ruijie_attribute_e50_14t_6e_8c[] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

static const char* ruijie_rename_e10_6e[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3"
};

static int ruijie_sorted_e10_6e[] = {0, 1, 2, 3, 4, 5};
static int ruijie_pair_e10_6e[] = {1, 1, 2, 2, 3, 3};
static int ruijie_attribute_e10_6e[] = {0, 1, 0, 1, 0, 1};

static int ruijie_sorted_dsk159_c3558r[] = {0, 1, 2, 3, 4, 5, 8, 9, 6, 7};

static const char* ruijie_rename_dsk159_c3558r[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
};

static int is_this_ruijie_x_serial(void)
{
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	unsigned int cpu_num = 0;
	fp = fopen( bypass_switch_config, "r" );
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
	}

	if (fp)
	{
		fclose(fp);
	}

	if (!strstr(tmpstr, "RUIJIE"))
	{
		return 0;
	}
	cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	/**X20:8 CPUs; X60:12 CPUs;**/
	if (cpu_num >= 8)
	{
		return 1;
	}
	/**E10:2 CPUs;E20:2 CPUs;E50:4 CPUs;**/
	else
	{
		return 0;
	}
}

static const char* rename_sorted_ruijie_z8680_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:0b.0/",
	" ../../devices/pci0000:00/0000:00:16.0/",
	" ../../devices/pci0000:00/0000:00:17.0/",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:08.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:00.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:0c.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:04.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:0e.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:06.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:0f.",
	" ../../devices/pci0000:00/0000:00:0e.0/0000:06:00.0/0000:07:07."
};

const static char rename_sorted_ruijie_z8680_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"A",
	"B"
};

const static char rename_sorted_ruijie_z8680_card_mode_char[][3] = 
{
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0"
};

static const char* rename_sorted_ruijie_z8680_name_string[] = 
{
	"onboard1",
	"onboard2",
	"onboard3",
	"slot1",
	"slot2",
	"slot3",
	"slot4",
	"slot5",
	"slot6",
	"slot7",
	"slot8"
};

static void  ruijie_z8680_ext_card_by_sys_class_net(const char* card_feature[], const char card_id[][3], const char card_mode[][3], const char *card_name[], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int switch_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	unsigned int iDpdkEth = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose( fp );
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			goto error_out;
		}
	}

#if 1
	unlink( ACE_PORT_MAP_FILE ".old" );

	if ( F_OK == access( ACE_PORT_MAP_FILE, F_OK ) )
	{
		rename( ACE_PORT_MAP_FILE, ACE_PORT_MAP_FILE ".old" );
	}
#endif

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp))
		{
retry:
			eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					//ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = card_mode[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
						{
							goto retry;
						}

						char *p = strrchr(line, '/');
						if (p)
						{
							strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1),
								sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);
							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					}while(0 != fgets(line, sizeof(line), fp));
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}

		eth_num = 28;

		switch_index = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1572" );

					if ( iDriver && iDevType )
					{
						// x, x-1, x-2 ...
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
				}
			}

			// MGMT
			if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) ) )
			{
				if ( ruijie_x_serial_ext_card_entry[sorted_num].ethNum > 0 )
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethPair = 1;
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethAttribute = 2;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "%s", "MGMT" );
				}
			}
			// switch
			else if ( ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2", strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) ) )
				|| ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard3", strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) ) )
				)
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( switch_index & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 0;
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "switch%d", switch_index );
					get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );
					switch_index++;
					iDpdkEth++;
				}
			}
			// x, x-1, x-2 ...
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
			{
				for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; eth_index-- )
				{
					int iRealIndex = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1 - eth_index;

					int ethAttribute = ( eth_num + iRealIndex ) & 0x01;

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + iRealIndex ) >> 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;

					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, 
						"%cAN%d", ethAttribute ? 'W' : 'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );
					iDpdkEth++;
				}

				eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			// x, x+1, x+2 ...
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( eth_num & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( eth_num >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );
					eth_num++;
					iDpdkEth++;
				}
			}
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
	if ( fp )
	{
		sprintf( line, "ifAutoCfgMode %d\n", 0 );
		fwrite( line, strlen( line ), sizeof( char ), fp );
	}

	int iSwitch = 0;

	for( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
	{
		int iExtIndex = 1;

		sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

		if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
		{
			for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; eth_index-- )
			{
				if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
				{
					int iExt = 1;

					if ( 's' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
					{
						if ( ( ruijie_x_serial_ext_card_entry[sorted_num].cardId >= 'A' ) && ( ruijie_x_serial_ext_card_entry[sorted_num].cardId <= 'Z' ) )
						{
							iExt = 10 + ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - 'A' ) - 3;
						}
						else
						{
							iExt = ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - '0' ) - 3;
						}

						sprintf( line, "##ExtMap##%d %d/%d %s %s %d %s\n", iExt, iExtIndex, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );
						fwrite( line, strlen( line ), sizeof( char ), fp );

						int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1;
						int iPairIndex = 0;

						if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}

						sprintf(line, "##PortMap##%d %s %s\n",
							iPair * 2 + iPairIndex + 1,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
						);

						fwrite(line, strlen( line ), sizeof( char ), fp );

						iExtIndex++;
					}
					else if ( 0 == iSwitch )
					{
						sprintf( line, "##PortMap##%d %s %s\n", 1,  "101", "LAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //1
						sprintf( line, "##PortMap##%d %s %s\n", 2,  "201", "WAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //2
						sprintf( line, "##PortMap##%d %s %s\n", 3,  "102", "LAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //3
						sprintf( line, "##PortMap##%d %s %s\n", 4,  "202", "WAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //4
						sprintf( line, "##PortMap##%d %s %s\n", 5,  "103", "LAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //5
						sprintf( line, "##PortMap##%d %s %s\n", 6,  "203", "WAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //6
						sprintf( line, "##PortMap##%d %s %s\n", 7,  "104", "LAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //7
						sprintf( line, "##PortMap##%d %s %s\n", 8,  "204", "WAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //8
						sprintf( line, "##PortMap##%d %s %s\n", 9,  "105", "LAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //9
						sprintf( line, "##PortMap##%d %s %s\n", 10, "205", "WAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //10
						sprintf( line, "##PortMap##%d %s %s\n", 11, "106", "LAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //11
						sprintf( line, "##PortMap##%d %s %s\n", 12, "206", "WAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //12
						sprintf( line, "##PortMap##%d %s %s\n", 13, "107", "LAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //13
						sprintf( line, "##PortMap##%d %s %s\n", 14, "207", "WAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //14
						sprintf( line, "##PortMap##%d %s %s\n", 15, "108", "LAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //15
						sprintf( line, "##PortMap##%d %s %s\n", 16, "208", "WAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //16
						sprintf( line, "##PortMap##%d %s %s\n", 17, "109", "LAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //17
						sprintf( line, "##PortMap##%d %s %s\n", 18, "209", "WAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //18
						sprintf( line, "##PortMap##%d %s %s\n", 19, "110", "LAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //19
						sprintf( line, "##PortMap##%d %s %s\n", 20, "210", "WAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //20
						sprintf( line, "##PortMap##%d %s %s\n", 21, "111", "LAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //21
						sprintf( line, "##PortMap##%d %s %s\n", 22, "211", "WAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //22
						sprintf( line, "##PortMap##%d %s %s\n", 23, "112", "LAN12" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //23
						sprintf( line, "##PortMap##%d %s %s\n", 24, "212", "WAN12" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //24
						sprintf( line, "##PortMap##%d %s %s\n", 25, "113", "LAN13" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //25
						sprintf( line, "##PortMap##%d %s %s\n", 26, "213", "WAN13" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //26
						sprintf( line, "##PortMap##%d %s %s\n", 27, "114", "LAN14" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //27
						sprintf( line, "##PortMap##%d %s %s\n", 28, "214", "WAN14" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //28

						iSwitch = 1;
					}
				}

				sprintf( line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

				fwrite( line, strlen( line ), sizeof( char ), fp );
			}
		}
		else
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
			{
				if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
				{
					int iExt = 1;

					if ( 's' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
					{
						if ( ( ruijie_x_serial_ext_card_entry[sorted_num].cardId >= 'A' ) && ( ruijie_x_serial_ext_card_entry[sorted_num].cardId <= 'Z' ) )
						{
							iExt = 10 + ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - 'A' ) - 3;
						}
						else
						{
							iExt = ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - '0' ) - 3;
						}

						sprintf( line, "##ExtMap##%d %d/%d %s %s %d %s\n", iExt, iExtIndex, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );
						fwrite( line, strlen( line ), sizeof( char ), fp );

						int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1;
						int iPairIndex = 0;

						if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}

						sprintf(line, "##PortMap##%d %s %s\n",
							iPair * 2 + iPairIndex + 1,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
						);

						fwrite(line, strlen( line ), sizeof( char ), fp );

						iExtIndex++;
					}
					else if ( 0 == iSwitch )
					{
						sprintf( line, "##PortMap##%d %s %s\n", 1,  "101", "LAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //1
						sprintf( line, "##PortMap##%d %s %s\n", 2,  "201", "WAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //2
						sprintf( line, "##PortMap##%d %s %s\n", 3,  "102", "LAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //3
						sprintf( line, "##PortMap##%d %s %s\n", 4,  "202", "WAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //4
						sprintf( line, "##PortMap##%d %s %s\n", 5,  "103", "LAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //5
						sprintf( line, "##PortMap##%d %s %s\n", 6,  "203", "WAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //6
						sprintf( line, "##PortMap##%d %s %s\n", 7,  "104", "LAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //7
						sprintf( line, "##PortMap##%d %s %s\n", 8,  "204", "WAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //8
						sprintf( line, "##PortMap##%d %s %s\n", 9,  "105", "LAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //9
						sprintf( line, "##PortMap##%d %s %s\n", 10, "205", "WAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //10
						sprintf( line, "##PortMap##%d %s %s\n", 11, "106", "LAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //11
						sprintf( line, "##PortMap##%d %s %s\n", 12, "206", "WAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //12
						sprintf( line, "##PortMap##%d %s %s\n", 13, "107", "LAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //13
						sprintf( line, "##PortMap##%d %s %s\n", 14, "207", "WAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //14
						sprintf( line, "##PortMap##%d %s %s\n", 15, "108", "LAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //15
						sprintf( line, "##PortMap##%d %s %s\n", 16, "208", "WAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //16
						sprintf( line, "##PortMap##%d %s %s\n", 17, "109", "LAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //17
						sprintf( line, "##PortMap##%d %s %s\n", 18, "209", "WAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //18
						sprintf( line, "##PortMap##%d %s %s\n", 19, "110", "LAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //19
						sprintf( line, "##PortMap##%d %s %s\n", 20, "210", "WAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //20
						sprintf( line, "##PortMap##%d %s %s\n", 21, "111", "LAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //21
						sprintf( line, "##PortMap##%d %s %s\n", 22, "211", "WAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //22
						sprintf( line, "##PortMap##%d %s %s\n", 23, "112", "LAN12" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //23
						sprintf( line, "##PortMap##%d %s %s\n", 24, "212", "WAN12" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //24
						sprintf( line, "##PortMap##%d %s %s\n", 25, "113", "LAN13" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //25
						sprintf( line, "##PortMap##%d %s %s\n", 26, "213", "WAN13" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //26
						sprintf( line, "##PortMap##%d %s %s\n", 27, "114", "LAN14" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //27
						sprintf( line, "##PortMap##%d %s %s\n", 28, "214", "WAN14" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //28

						iSwitch = 1;
					}
				}

				sprintf( line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

				fwrite( line, strlen( line ), sizeof( char ), fp );
			}
		}
	}

	fclose( fp );

	fp = NULL;

#if 1
	// check ext change
	{
		int iChanged = ruijie_check_change_ext_card2( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );

		if ( iChanged > 0 )
		{
			unlink( ACE_PORT_MAP_FILE ".old" );

			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is changed, ret:%d.", iChanged );
		}
		else
		{
			if ( F_OK == access( ACE_PORT_MAP_FILE ".old", F_OK ) )
			{
				unlink( ACE_PORT_MAP_FILE );

				rename( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is not changed, ret:%d.", iChanged );
		}

		// check slot value
		if ( iChanged <= 0 )
		{
			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				int x = 0;
				int iVppEth = 0;
				char szLine[128] = {0};
				char szVppEthCache[128][64] = {0};

				int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

				if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
				{
					if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
					{
						unlink( ACE_VPP_PORT_MAP_FILE );

						init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
					}
					else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
					{
						int iEnd = 0;

						for( card_index = 0; card_index < ruijie_x_serial_ext_card_number && !iEnd; ++card_index )
						{
							sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

							for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum && !iEnd; eth_index++ )
							{
								if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
								{
									int iFind = 0;

									snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId,
										ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
										( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

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

										init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif

error_out:
	if ( fp )
	{
		pclose(fp);
	}

	return;
}

static const char* rename_sorted_ruijie_z8620_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:0b.0/",
	" ../../devices/pci0000:00/0000:00:16.0/",
	" ../../devices/pci0000:00/0000:00:10.0/",
	" ../../devices/pci0000:00/0000:00:0e.0/"
};
const static char rename_sorted_ruijie_z8620_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4"
};

const static char rename_sorted_ruijie_z8620_card_mode_char[][3] = 
{
	"0",
	"0",
	"0",
	"0"
};

static const char* rename_sorted_ruijie_z8620_name_string[] = 
{
	"onboard1",
	"onboard2",
	"slot1",
	"slot2"
};

static void  ruijie_z8620_ext_card_by_sys_class_net(const char* card_feature[], const char card_id[][3], const char card_mode[][3], const char *card_name[], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int switch_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	unsigned int iDpdkEth = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose( fp );
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			goto error_out;
		}
	}

#if 1
	unlink( ACE_PORT_MAP_FILE ".old" );

	if ( F_OK == access( ACE_PORT_MAP_FILE, F_OK ) )
	{
		rename( ACE_PORT_MAP_FILE, ACE_PORT_MAP_FILE ".old" );
	}
#endif

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp))
		{
retry:
			eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					//ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = card_mode[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
						{
							goto retry;
						}

						char *p = strrchr(line, '/');
						if (p)
						{
							strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1),
								sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);
							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					}while(0 != fgets(line, sizeof(line), fp));
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}

		eth_num = 22;

		switch_index = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1572" );

					if ( iDriver && iDevType )
					{
						// x, x-1, x-2 ...
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
				}
			}

			// MGMT
			if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) ) )
			{
				if ( ruijie_x_serial_ext_card_entry[sorted_num].ethNum > 0 )
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethPair = 1;
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethAttribute = 2;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "%s", "MGMT" );
				}
			}
			// switch
			else if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2", strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) ) )
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( switch_index & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 0;
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "switch%d", switch_index );
					get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );
					switch_index++;
					iDpdkEth++;
				}
			}
			// x, x-1, x-2 ...
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
			{
				for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; eth_index-- )
				{
					int iRealIndex = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1 - eth_index;

					int ethAttribute = ( eth_num + iRealIndex ) & 0x01;

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + iRealIndex ) >> 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;

					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, 
						"%cAN%d", ethAttribute ? 'W' : 'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );
					iDpdkEth++;
				}

				eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			// x, x+1, x+2 ...
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( eth_num & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( eth_num >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );
					eth_num++;
					iDpdkEth++;
				}
			}
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
	if ( fp )
	{
		sprintf( line, "ifAutoCfgMode %d\n", 0 );
		fwrite( line, strlen( line ), sizeof( char ), fp );
	}

	int iSwitch = 0;

	for( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
	{
		int iExtIndex = 1;

		sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

		if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
		{
			for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; eth_index-- )
			{
				if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
				{
					int iExt = 1;

					if ( 's' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
					{
						if ( ( ruijie_x_serial_ext_card_entry[sorted_num].cardId >= 'A' ) && ( ruijie_x_serial_ext_card_entry[sorted_num].cardId <= 'Z' ) )
						{
							iExt = 10 + ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - 'A' ) - 3;
						}
						else
						{
							iExt = ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - '0' ) - 2;
						}

						sprintf( line, "##ExtMap##%d %d/%d %s %s %d %s\n", iExt, iExtIndex, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );
						fwrite( line, strlen( line ), sizeof( char ), fp );

						int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1;
						int iPairIndex = 0;

						if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}

						sprintf(line, "##PortMap##%d %s %s\n",
							iPair * 2 + iPairIndex + 1,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
						);

						fwrite(line, strlen( line ), sizeof( char ), fp );

						iExtIndex++;
					}
					else if ( 0 == iSwitch )
					{
						sprintf( line, "##PortMap##%d %s %s\n", 1,  "101", "LAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //1
						sprintf( line, "##PortMap##%d %s %s\n", 2,  "201", "WAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //2
						sprintf( line, "##PortMap##%d %s %s\n", 3,  "102", "LAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //3
						sprintf( line, "##PortMap##%d %s %s\n", 4,  "202", "WAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //4
						sprintf( line, "##PortMap##%d %s %s\n", 5,  "103", "LAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //5
						sprintf( line, "##PortMap##%d %s %s\n", 6,  "203", "WAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //6
						sprintf( line, "##PortMap##%d %s %s\n", 7,  "104", "LAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //7
						sprintf( line, "##PortMap##%d %s %s\n", 8,  "204", "WAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //8
						sprintf( line, "##PortMap##%d %s %s\n", 9,  "105", "LAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //9
						sprintf( line, "##PortMap##%d %s %s\n", 10, "205", "WAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //10
						sprintf( line, "##PortMap##%d %s %s\n", 11, "106", "LAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //11
						sprintf( line, "##PortMap##%d %s %s\n", 12, "206", "WAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //12
						sprintf( line, "##PortMap##%d %s %s\n", 13, "107", "LAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //13
						sprintf( line, "##PortMap##%d %s %s\n", 14, "207", "WAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //14
						sprintf( line, "##PortMap##%d %s %s\n", 15, "108", "LAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //15
						sprintf( line, "##PortMap##%d %s %s\n", 16, "208", "WAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //16
						sprintf( line, "##PortMap##%d %s %s\n", 17, "109", "LAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //17
						sprintf( line, "##PortMap##%d %s %s\n", 18, "209", "WAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //18
						sprintf( line, "##PortMap##%d %s %s\n", 19, "110", "LAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //19
						sprintf( line, "##PortMap##%d %s %s\n", 20, "210", "WAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //20
						sprintf( line, "##PortMap##%d %s %s\n", 21, "111", "LAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //21
						sprintf( line, "##PortMap##%d %s %s\n", 22, "211", "WAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //22

						iSwitch = 1;
					}
				}

				sprintf( line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

				fwrite( line, strlen( line ), sizeof( char ), fp );
			}
		}
		else
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
			{
				if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
				{
					int iExt = 1;

					if ( 's' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
					{
						if ( ( ruijie_x_serial_ext_card_entry[sorted_num].cardId >= 'A' ) && ( ruijie_x_serial_ext_card_entry[sorted_num].cardId <= 'Z' ) )
						{
							iExt = 10 + ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - 'A' ) - 3;
						}
						else
						{
							iExt = ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - '0' ) - 2;
						}

						sprintf( line, "##ExtMap##%d %d/%d %s %s %d %s\n", iExt, iExtIndex, ruijie_x_serial_ext_card_entry[sorted_num].ethNum,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );
						fwrite( line, strlen( line ), sizeof( char ), fp );

						int iPair = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1;
						int iPairIndex = 0;

						if ( 'W' == ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
						{
							iPairIndex = 1;
						}

						sprintf(line, "##PortMap##%d %s %s\n",
							iPair * 2 + iPairIndex + 1,
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName
						);

						fwrite(line, strlen( line ), sizeof( char ), fp );

						iExtIndex++;
					}
					else if ( 0 == iSwitch )
					{
						sprintf( line, "##PortMap##%d %s %s\n", 1,  "101", "LAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //1
						sprintf( line, "##PortMap##%d %s %s\n", 2,  "201", "WAN1" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //2
						sprintf( line, "##PortMap##%d %s %s\n", 3,  "102", "LAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //3
						sprintf( line, "##PortMap##%d %s %s\n", 4,  "202", "WAN2" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //4
						sprintf( line, "##PortMap##%d %s %s\n", 5,  "103", "LAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //5
						sprintf( line, "##PortMap##%d %s %s\n", 6,  "203", "WAN3" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //6
						sprintf( line, "##PortMap##%d %s %s\n", 7,  "104", "LAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //7
						sprintf( line, "##PortMap##%d %s %s\n", 8,  "204", "WAN4" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //8
						sprintf( line, "##PortMap##%d %s %s\n", 9,  "105", "LAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //9
						sprintf( line, "##PortMap##%d %s %s\n", 10, "205", "WAN5" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //10
						sprintf( line, "##PortMap##%d %s %s\n", 11, "106", "LAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //11
						sprintf( line, "##PortMap##%d %s %s\n", 12, "206", "WAN6" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //12
						sprintf( line, "##PortMap##%d %s %s\n", 13, "107", "LAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //13
						sprintf( line, "##PortMap##%d %s %s\n", 14, "207", "WAN7" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //14
						sprintf( line, "##PortMap##%d %s %s\n", 15, "108", "LAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //15
						sprintf( line, "##PortMap##%d %s %s\n", 16, "208", "WAN8" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //16
						sprintf( line, "##PortMap##%d %s %s\n", 17, "109", "LAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //17
						sprintf( line, "##PortMap##%d %s %s\n", 18, "209", "WAN9" );  fwrite(line, strlen( line ), sizeof( char ), fp ); //18
						sprintf( line, "##PortMap##%d %s %s\n", 19, "110", "LAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //19
						sprintf( line, "##PortMap##%d %s %s\n", 20, "210", "WAN10" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //20
						sprintf( line, "##PortMap##%d %s %s\n", 21, "111", "LAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //21
						sprintf( line, "##PortMap##%d %s %s\n", 22, "211", "WAN11" ); fwrite(line, strlen( line ), sizeof( char ), fp ); //22

						iSwitch = 1;
					}
				}

				sprintf( line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

				fwrite( line, strlen( line ), sizeof( char ), fp );
			}
		}
	}

	fclose( fp );

	fp = NULL;

#if 1
	// check ext change
	{
		int iChanged = ruijie_check_change_ext_card2( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );

		if ( iChanged > 0 )
		{
			unlink( ACE_PORT_MAP_FILE ".old" );

			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is changed, ret:%d.", iChanged );
		}
		else
		{
			if ( F_OK == access( ACE_PORT_MAP_FILE ".old", F_OK ) )
			{
				unlink( ACE_PORT_MAP_FILE );

				rename( ACE_PORT_MAP_FILE ".old", ACE_PORT_MAP_FILE );
			}

			init_coordinate_log( "Info, the ext card is not changed, ret:%d.", iChanged );
		}

		// check slot value
		if ( iChanged <= 0 )
		{
			if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
			{
				int x = 0;
				int iVppEth = 0;
				char szLine[128] = {0};
				char szVppEthCache[128][64] = {0};

				int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

				if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
				{
					if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
					{
						unlink( ACE_VPP_PORT_MAP_FILE );

						init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
					}
					else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
					{
						int iEnd = 0;

						for( card_index = 0; card_index < ruijie_x_serial_ext_card_number && !iEnd; ++card_index )
						{
							sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

							for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum && !iEnd; eth_index++ )
							{
								if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] )
								{
									int iFind = 0;

									snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId,
										ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
										( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

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

										init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif

error_out:
	if ( fp )
	{
		pclose(fp);
	}

	return;
}


static void ruijie_auto_rename_if(int eth_num)
{
/*
# 1: watchdog 0: bypass default: bypass
METHOD = 0
# LANNER_7580 LANNER_8755 LANNER_8892 LANNER_7565 #MSI_G41
TYPE = NEUSOFT
*/
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     i                           = 0;
	int     fixed_eth_num = 0;
	int     ext_copper_num              = 0;
	int     ext_fibre_num               = 0;

	fp = fopen( bypass_switch_config, "r" );
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
	}

	if (fp)
	{
		fclose(fp);
	}

	if (!strstr(tmpstr, "RUIJIE"))
	{
		return;
	}

#if 0
	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( ( !fp ) 
		|| ( fgets( line, sizeof( line ), fp ) == 0 ) 
		|| ( sscanf( line, "%s %d", cmdBuf, &autoCfgMode ) != 2 ) 
		|| ( autoCfgMode ) )
	{
		/**auto config:**/
	}
	else
	{
		return;
	}
#endif
    //zhanghuanan 2024/01/06 Ruijie Z8620 hardware(2 Switch chips conneting 16 coppers(1G) and 6 fibre(10G)):
    if (!access("/usr/mysys/z8620_config.bcm", F_OK))
    {
#if 0
        if (access(ACE_VPP_PORT_MAP_FILE, F_OK))
        {
            fp = fopen( ACE_VPP_PORT_MAP_FILE, "wt+" );
            if (fp)
            {
                i = sprintf( line, "ifAutoCfgMode 0\n" );
                fwrite(line, i, sizeof( char ), fp );
                i = sprintf( line, "d0/enp8/s0/f0 switch0 1 TRUST no-ignore\n" );
                fwrite(line, i, sizeof( char ), fp );
                i = sprintf( line, "d0/enp8/s0/f1 switch1 1 UNTRUST no-ignore\n" );
                fwrite(line, i, sizeof( char ), fp );
                fclose(fp);
            }
        }
#else
		ruijie_z8620_ext_card_by_sys_class_net( rename_sorted_ruijie_z8620_feature_string, rename_sorted_ruijie_z8620_card_id_char, rename_sorted_ruijie_z8620_card_mode_char, rename_sorted_ruijie_z8620_name_string, sizeof( rename_sorted_ruijie_z8620_card_id_char ) / sizeof( rename_sorted_ruijie_z8620_card_id_char[0] ), eth_num );
#endif
        return;
    }

	// wyx 2024/03/05 Ruijie Z8680 hardware
	if ( !access( "/usr/mysys/z8680_config.bcm", F_OK ) )
	{
		ruijie_z8680_ext_card_by_sys_class_net( rename_sorted_ruijie_z8680_feature_string, rename_sorted_ruijie_z8680_card_id_char, rename_sorted_ruijie_z8680_card_mode_char, rename_sorted_ruijie_z8680_name_string, sizeof( rename_sorted_ruijie_z8680_card_id_char ) / sizeof( rename_sorted_ruijie_z8680_card_id_char[0] ), eth_num );	

		return;
	}

    fp = popen("cat /proc/cpuinfo | grep \"model name\" | awk 'NR==1'", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		fgets(line, sizeof(line), fp );
		pclose( fp );
        fp = NULL;
		if (!strncmp(line, "model name\t: Intel(R) Xeon(R) CPU E3-1275 v5 @ 3.60GHz",
            sizeof("model name\t: Intel(R) Xeon(R) CPU E3-1275 v5 @ 3.60GHz")-1)
            || !strncmp(line, "model name\t: Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz",
            sizeof("model name\t: Intel(R) Core(TM) i7-7700 CPU @ 3.60GHz")-1))
		{
            //x20-M 超线程已开
            ruijie_auto_rename_x20_new_ext_card(eth_num);
            return;
        }
        else if (!strncmp(line, "model name\t: Intel(R) Celeron(R) CPU  J1900  @ 1.99GHz",
            sizeof("model name\t: Intel(R) Celeron(R) CPU  J1900  @ 1.99GHz")-1))
        {
            //E10
            if (eth_num == 6)
            {
                fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
    			if (fp)
    			{
    				sprintf( line, "ifAutoCfgMode %d\n", 0 );
    				fwrite( line, strlen( line ), sizeof( char ), fp );
    				for (i = 0; i < eth_num; i++)
    				{
    					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e10_6e[i], ruijie_rename_e10_6e[i], ruijie_pair_e10_6e[i],  (ruijie_rename_attribute_name[ruijie_attribute_e10_6e[i]]));
    					fwrite(line, strlen( line ), sizeof( char ), fp );
    				}
    				fclose(fp);
    			}
    			return;
            }
        }
        else if (!strncmp(line, "model name\t: Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz",
            sizeof("model name\t: Intel(R) Xeon(R) CPU E5-2620 v4 @ 2.10GHz")-1))
        {
            if (sysconf(_SC_NPROCESSORS_CONF) == 16)
            {
                //x60-M 超线程已开
                ruijie_auto_rename_x60_new_ext_card(eth_num);
                return;
            }
        }
		// DSK-159
		else if (!strncmp(line, "model name\t: Intel(R) Atom(TM) CPU C3558R @ 2.40GHz",
				sizeof("model name\t: Intel(R) Atom(TM) CPU C3558R @ 2.40GHz")-1))
		{
			if ( have_i_pci_device("80861980") && have_i_pci_device("808619a1") && have_i_pci_device("8086125c") && ( 10 == eth_num ) )
			{
				fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
				if (fp)
				{
					sprintf( line, "ifAutoCfgMode %d\n", 0 );
					fwrite( line, strlen( line ), sizeof( char ), fp );
					for (i = 0; i < eth_num; i++)
					{
						sprintf(line, "eth%d  %s %d  %s no-ignore\n", ruijie_sorted_dsk159_c3558r[i], ruijie_rename_dsk159_c3558r[i], 1 + (i>>1),  (i & 1) ? "UNTRUST" : "TRUST");
						fwrite(line, strlen( line ), sizeof( char ), fp );
					}
					fclose(fp);
				}
			}
		}
		// U3210 or U3100
		else if ( !strncmp( line, "model name\t: Intel(R) Atom(TM) CPU C3558 @ 2.20GHz",
				sizeof("model name\t: Intel(R) Atom(TM) CPU C3558 @ 2.20GHz")-1))
		{
			// U3100
			if ( have_i_pci_device("80861980") && have_i_pci_device("808619a1") && have_i_pci_device("808619db") && ( 10 == eth_num ) )
			{
				ruijie_auto_rename_nis132_c3558_ext_card( eth_num );
			}
			// U3210
			else if ( have_i_pci_device("80861980") && have_i_pci_device("808619a1") && have_i_pci_device("808619e2") && ( eth_num >= 8 ) )
			{
				ruijie_nis_128_C3558_auto_rename_x_serial_ext_card(ruijie_nis_128_C3558_ext_card_feature_string, ruijie_nis_128_C3558_ext_card_id_char, sizeof(ruijie_nis_128_C3558_ext_card_id_char)/sizeof(ruijie_nis_128_C3558_ext_card_id_char[0]), eth_num);
			}
		}
	}

    FILE * fdmi = popen("/usr/sbin/dmidecode | grep \"Product Name: \" | awk '{if (NR==1){print $0}}'", "r");
    if (fdmi)
    {
        char dmi_product[128] = { 0 };
        if (NULL != fgets(dmi_product, sizeof(dmi_product), fdmi))
        {
            char *p = strstr(dmi_product, "Product Name: ");
            if (p)
            {
                p += sizeof("Product Name: ")-1;
                if (!strcmp(p, "C3558\n"))
                {
                    ruijie_auto_rename_C3558_ext_card(eth_num);
                    return;
                }
                else if (!strcmp(p, "C3758\n"))
                {
                    ruijie_auto_rename_C3758_ext_card(eth_num);
                    return;
                }
            }
        }

        pclose(fdmi);
    }
    
	fp = popen("mpstat | awk -F '(' '{if(NR==1)print $3}' | awk -F ' ' '{print $1}'", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		fgets(line, sizeof(line), fp );
		pclose( fp );
        //x20 超线程已开
		if (line[0] == '8')
		{
			ruijie_auto_rename_x20_ext_card(eth_num);
			return;
		}
        //x60-32位型号灌装64位系统
        else if (line[0] == '1' && line[1] == '2')
        {
            ruijie_auto_rename_x60_ext_card(eth_num);
            return;
        }
		//x100
        else if(line[0] == '2' && line[1] == '4')
		{
			ruijie_auto_rename_x60_ext_card(eth_num);
			return;
		}
        //定型使用，未采用
        else if(line[0] == '3' && line[1] == '2')
		{
			#ifndef ARCH_ARM64
            ruijie_auto_rename_x200_ext_card(eth_num);
			#endif
            return;
		}
        //x200 x100-S 超线程已开
        else if (line[0] == '4' && line[1] == '0')
        {
			#ifndef ARCH_ARM64
            ruijie_auto_rename_x200_ext_card(eth_num);
			#endif
            return;
        }
	}

 	fixed_eth_num = 0;
	fp = popen("lspci -n | grep '8086:150c'", "r");
	if ( fp )
	{
		while ( 0 != fgets(line, sizeof(line), fp ) )
		{
			fixed_eth_num++;
		}
		pclose( fp );
	}

	/**RG-UAC 6000 E10 --- D525**/
	if (5 == fixed_eth_num)
	{
		/**4Copper+4Fibre:**/
		ext_copper_num = 0;
		fp = popen("lspci -n | grep '8086:150e'", "r");
		if ( fp )
		{
			while ( 0 != fgets(line, sizeof(line), fp ) )
			{
				ext_copper_num++;
			}
			pclose( fp );
		}
		ext_fibre_num = 0;
		fp = popen("lspci -n | grep '8086:150f'", "r");
		if ( fp )
		{
			while ( 0 != fgets(line, sizeof(line), fp ) )
			{
				ext_fibre_num++;
			}
			pclose( fp );
		}
		if (0 == ext_copper_num && 0 == ext_fibre_num)
		{
			ext_copper_num = 0;
			fp = popen("lspci -n | grep '8086:1521'", "r");
			if ( fp )
			{
				while ( 0 != fgets(line, sizeof(line), fp ) )
				{
					ext_copper_num++;
				}
				pclose( fp );
			}
			ext_fibre_num = 0;
			fp = popen("lspci -n | grep '8086:1522'", "r");
			if ( fp )
			{
				while ( 0 != fgets(line, sizeof(line), fp ) )
				{
					ext_fibre_num++;
				}
				pclose( fp );
			}
		}
		/**RG-UAC 6000 E10 --- Total ethnet 13; Fixed 5; Ext copper 4; Ext fibre 4;**/
		if (13 == eth_num && 4 == ext_copper_num && 4 == ext_fibre_num)
		{
			fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
			if (fp)
			{
				sprintf( line, "ifAutoCfgMode %d\n", 0 );
				fwrite( line, strlen( line ), sizeof( char ), fp );
				for (i = 0; i < eth_num; i++)
				{
					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e10_13t_5e_4c_4f[i], ruijie_rename_e10_13t_5e_4c_4f[i], ruijie_pair_e10_13t_5e_4c_4f[i],  (ruijie_rename_attribute_name[ruijie_attribute_e10_13t_5e_4c_4f[i]]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				fclose(fp);
			}
			return;
		}

		/**8Copper:**/
		ext_copper_num = 0;
/**zhang.huanan 2015-04-17 replaced:**/
#if 0
		fp = popen("lspci -n | grep '8086:1521'", "r");
#else
		fp = popen("lspci -n | grep '8086:150e'", "r");
#endif
		if ( fp )
		{
			while ( 0 != fgets(line, sizeof(line), fp ) )
			{
				ext_copper_num++;
			}
			pclose( fp );
		}
		/**RG-UAC 6000 E10 --- Total ethnet 13; Fixed 5; Ext copper 8;**/
		if (13 == eth_num && 8 == ext_copper_num && 0 == ext_fibre_num)
		{
			fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
			if (fp)
			{
				sprintf( line, "ifAutoCfgMode %d\n", 0 );
				fwrite( line, strlen( line ), sizeof( char ), fp );
				for (i = 0; i < eth_num; i++)
				{
					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e10_13t_5e_8c[i], ruijie_rename_e10_13t_5e_8c[i], ruijie_pair_e10_13t_5e_8c[i],  (ruijie_rename_attribute_name[ruijie_attribute_e10_13t_5e_8c[i]]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				fclose(fp);
			}
			return;
		}
		
		/**RG-UAC 6000 E10 --- Total ethnet 5; Fixed 5; Ext copper 0; Ext fibre 0;**/
		if (5 == eth_num && 0 == ext_copper_num && 0 == ext_fibre_num)
		{
			fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
			if (fp)
			{
				sprintf( line, "ifAutoCfgMode %d\n", 0 );
				fwrite( line, strlen( line ), sizeof( char ), fp );
				for (i = 0; i < eth_num; i++)
				{
					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e10_5t_5e_no_ext[i], ruijie_rename_e10_5t_5e_no_ext[i], ruijie_pair_e10_5t_5e_no_ext[i],  (ruijie_rename_attribute_name[ruijie_attribute_e10_5t_5e_no_ext[i]]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				fclose(fp);
			}
			return;
		}
//		remove(ACE_PORT_MAP_FILE);
	}
	/**RG-UAC 6000 E50 --- I5-2400**/
	/**RG-UAC 6000 E20 --- G850-3000 the same as RG-UAC6000 E50**/
	if (6 == fixed_eth_num)
	{
		/**4Copper+4Fibre:**/
		ext_copper_num = 0;
		fp = popen("lspci -n | grep '8086:150e'", "r");
		if ( fp )
		{
			while ( 0 != fgets(line, sizeof(line), fp ) )
			{
				ext_copper_num++;
			}
			pclose( fp );
		}
		ext_fibre_num = 0;
		fp = popen("lspci -n | grep '8086:150f'", "r");
		if ( fp )
		{
			while ( 0 != fgets(line, sizeof(line), fp ) )
			{
				ext_fibre_num++;
			}
			pclose( fp );
		}
		if (0 == ext_copper_num && 0 == ext_fibre_num)
		{
			ext_copper_num = 0;
			fp = popen("lspci -n | grep '8086:1521'", "r");
			if ( fp )
			{
				while ( 0 != fgets(line, sizeof(line), fp ) )
				{
					ext_copper_num++;
				}
				pclose( fp );
			}
			ext_fibre_num = 0;
			fp = popen("lspci -n | grep '8086:1522'", "r");
			if ( fp )
			{
				while ( 0 != fgets(line, sizeof(line), fp ) )
				{
					ext_fibre_num++;
				}
				pclose( fp );
			}
		}
		/**RG-UAC 6000 E20 --- Total ethnet 14; Fixed 6; Ext copper 4; Ext fibre 4;**/
		if (14 == eth_num && 4 == ext_copper_num && 4 == ext_fibre_num)
		{
			fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
			if (fp)
			{
				sprintf( line, "ifAutoCfgMode %d\n", 0 );
				fwrite( line, strlen( line ), sizeof( char ), fp );
				for (i = 0; i < eth_num; i++)
				{
					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e50_14t_6e_4c_4f[i], ruijie_rename_e50_14t_6e_4c_4f[i], ruijie_pair_e50_14t_6e_4c_4f[i],  (ruijie_rename_attribute_name[ruijie_attribute_e50_14t_6e_4c_4f[i]]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				fclose(fp);
			}
			return;
		}
		/**8Copper:**/
		ext_copper_num = 0;
		/**zhang.huanan 2015-04-17 replaced:**/
#if 0
		fp = popen("lspci -n | grep '8086:1521'", "r");
#else
		fp = popen("lspci -n | grep '8086:150e'", "r");
#endif
		if ( fp )
		{
			while ( 0 != fgets(line, sizeof(line), fp ) )
			{
				ext_copper_num++;
			}
			pclose( fp );
		}
		/**RG-UAC 6000 E10 --- Total ethnet 13; Fixed 5; Ext copper 8;**/
		if (14 == eth_num && 8 == ext_copper_num && 0 == ext_fibre_num)
		{
			fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
			if (fp)
			{
				sprintf( line, "ifAutoCfgMode %d\n", 0 );
				fwrite( line, strlen( line ), sizeof( char ), fp );
				for (i = 0; i < eth_num; i++)
				{
					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e50_14t_6e_8c[i], ruijie_rename_e50_14t_6e_8c[i], ruijie_pair_e50_14t_6e_8c[i],  (ruijie_rename_attribute_name[ruijie_attribute_e50_14t_6e_8c[i]]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				fclose(fp);
			}
			return;
		}
		
		/**RG-UAC 6000 E20 --- Total ethnet 14; Fixed 6; Ext copper 0; Ext fibre 0;**/
		if (6 == eth_num && 0 == ext_copper_num && 0 == ext_fibre_num)
		{
			fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
			if (fp)
			{
				sprintf( line, "ifAutoCfgMode %d\n", 0 );
				fwrite( line, strlen( line ), sizeof( char ), fp );
				for (i = 0; i < eth_num; i++)
				{
					sprintf(line, "eth%d %s %d %s no-ignore\n", ruijie_sorted_e50_6t_6e_no_ext[i], ruijie_rename_e50_6t_6e_no_ext[i], ruijie_pair_e50_6t_6e_no_ext[i],  (ruijie_rename_attribute_name[ruijie_attribute_e50_6t_6e_no_ext[i]]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
				fclose(fp);
			}
			return;
		}
	}
}
#endif

#if 4
static const char* topsec_t5e_ext_card_feature_string[] = 
{
	"       +-01.0-",
	"       +-01.1-",
	"       +-1b.0-",
	"       +-1c.0-",
	"       +-1c.1-",
	"       +-1c.2-",
	"       +-1c.3-",
	"       +-1c.4-",
	"       +-1c.5-",
};

const static char topsec_t5e_ext_card_id_char[][3] = 
{
	"9",
	"8",
	"7",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
};

static const char* topsec_t5e_2u_ext_card_feature_string[] = 
{
	"       +-01.0-",
	"       +-01.1-",
	"       +-1b.0-",
	"       +-1c.0-",
	"       +-1c.4-",
};

const static char topsec_t5e_2u_ext_card_id_char[][3] = 
{
	"5",
	"4",
	"3",
	"2",
	"1",
};

static const char* topsec_t2e_1u_ext_card_feature_string[] = 
{
	"       +-1c.0-",
	"       +-1c.2-",
	"       +-1c.3-",
};

const static char topsec_t2e_1u_ext_card_id_char[][3] = 
{
	"2",
	"1",
	"3",
};


static const char* topsec_t7e_ext_card_feature_string[] = 
{
	"       +-1c.1-",
	"       +-1c.2-",
	"       +-01.0-",
	"       +-01.1-",
	"       +-02.0-",
	"       +-02.2-",
	"       +-03.0-",
	"       +-03.2-",
};
const static char topsec_t7e_ext_card_id_char[][3] = 
{
	"2",
	"1",
	"5",
	"8",
	"4",
	"3",
	"7",
	"6",
};

static const char* topsec_t7e_ext_card_name_string[] = 
{
	"onboard",
	"onboard",
	"slot3",
	"slot6",
	"slot2",
	"slot1",
	"slot5",
	"slot4",
};

static void topsec_t7e_auto_rename_x_serial_ext_card(const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	unsigned int iDpdkEth = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "--+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet Controller"))
				{
                    int x710 = 0;
                
                    if (strcasestr(line, "X710"))
                        x710 = 1;
                
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
						*p_char = 0;
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}

		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
            if (force_reverse
                || (ruijie_x_serial_ext_card_entry[sorted_num].ethNum >= 2
                && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == ruijie_x_serial_ext_card_entry[sorted_num].have_x710_num))
            {
                for (eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; --eth_index)
				{
					if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
					{
						int ethAttribute = (eth_num&1);

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
#if 0
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
#else
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1);
#endif
						eth_num++;
					}
					else
					{
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
						if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
						else
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
						nm_index++;
					}
				}
            }
            else
            {
    			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
    			{
    				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
    				{
    					int ethAttribute = (eth_num&1);

    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
#if 0
    					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
#else
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1);
#endif
    					eth_num++;
    				}
    				else
    				{
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
    					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
    					else
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
    					nm_index++;
    				}
    				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
    					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
    			}
            }
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

		char slot_name[CARD_SLOD_NAME_LEN]={0};
		int idx_in_slot = 0;
		int ethPair = 1;
        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			int used_eth_num = 0;
			int trust = 1;

			if (0 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum)
				continue;

			if (ethPair > NORMAL_CAR_TUNNEL_NUM)
				break;

			for (used_eth_num = 0; used_eth_num < ruijie_x_serial_ext_card_entry[sorted_num].ethNum;)
			{
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
	            {
	            	int trustA = trust;
					
					if (strcmp(ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard") && 8 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum)
						trustA = !trustA;
					
					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].used)
						continue;

					if (ethPair != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair
						|| !((trustA && !ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute) || (!trustA && ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute)))
						continue;
					
					if (strcmp(slot_name, ruijie_x_serial_ext_card_entry[sorted_num].card_slot))
					{
						idx_in_slot = 0;
						strncpy(slot_name, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, sizeof(slot_name)-1);
					}
#if 0
					if (!strcmp(slot_name, "onboard"))
					{
						sprintf(line, "#panel_info/%s/%s/%s\n",
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, slot_name, trust?"HA":"MGMT");
					}
					else
					{
		            	sprintf(line, "#panel_info/%s/%s/%d\n",
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, slot_name, idx_in_slot);
					}
					fwrite(line, strlen( line ), sizeof( char ), fp );
	                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
	                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
	                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
	                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
	                fwrite(line, strlen( line ), sizeof( char ), fp );
#else
					if ( !strcmp( slot_name, "onboard" ) )
					{
						sprintf( line, "#panel_info/%s/%s/%s\n",
							trust ? "HA" : "MGMT", slot_name, trust ? "HA" : "MGMT" );

						fwrite( line, strlen( line ), sizeof( char ), fp );

						sprintf( line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							trust ? "HA" : "MGMT", 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
							(ruijie_rename_attribute_name[2] ) );

						fwrite( line, strlen( line ), sizeof( char ), fp );
					}
					else
					{
						sprintf( line, "#panel_info/%s/%s/%d\n",
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, slot_name, idx_in_slot );

						fwrite( line, strlen( line ), sizeof( char ), fp );

						sprintf( line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair - 1,
							(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

						fwrite( line, strlen( line ), sizeof( char ), fp );

						get_eth_slot_dpdk_value( 1, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, sizeof( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId ) );

						iDpdkEth++;
					}
#endif
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].used=1;
					used_eth_num++;
					idx_in_slot++;
	            }

				if (!trust)
				{
					ethPair++;
					if (ethPair > NORMAL_CAR_TUNNEL_NUM)
						break;
				}
				trust = !trust;
			}
        }
        fclose(fp);
        fp = NULL;
#endif
		if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
		{
			int x = 0;
			int iVppEth = 0;
			char szLine[128] = {0};
			char szVppEthCache[128][64] = {0};

			int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

			if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
			{
				if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
				{
					unlink( ACE_VPP_PORT_MAP_FILE );

					init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
				}
				else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
				{
					int iEnd = 0;

					for( card_index = 0; card_index < ruijie_x_serial_ext_card_number && !iEnd; ++card_index )
					{
						sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

						for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum && !iEnd; eth_index++ )
						{
							if ( 'M' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0]
								&& 'H' != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName[0] 
								)
							{
								int iFind = 0;

								snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId,
									ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
									( ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute] ) );

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

									init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
								}
							}
						}
					}
				}
			}
		}
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}


static void topsec_auto_rename_t2e_1u_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "--+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet Controller")
                    || strcasestr(line, "Device 8088:0107")
                    || strcasestr(line, "Device 8088:0103")
                    || strcasestr(line, "Device 8088:0101")
                    || strcasestr(line, "Device 8088:2001"))
				{
                    int x710 = 0;
                
                    if (strcasestr(line, "X710"))
                        x710 = 1;
                
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
					{
						*p_char = 0;
					}
                    else
                    {
                        p_char = strcasestr(line, "Device 8088:");
                        if (p_char)
    					{
    						*p_char = 0;
    					}
                    }
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
            if (ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8)
            {
                for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
    			{
    				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
    				{
    					int ethAttribute = (eth_num&1);
    										
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'L':'W', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    					eth_num++;
    				}
    				else
    				{
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
    					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
    					else
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
    					nm_index++;
    				}
    				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
    					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
    			}
            }
            else
            {
    			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
    			{
    				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
    				{
    					int ethAttribute = (eth_num&1);
    										
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    					eth_num++;
    				}
    				else
    				{
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
    					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
    					else
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
    					nm_index++;
    				}
    				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
    					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
    			}
            }
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
            {
                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
                fwrite(line, strlen( line ), sizeof( char ), fp );
            }
        }
        fclose(fp);
        fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}


static void topsec_auto_rename_t5e_2u_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "--+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet Controller")
                    || strcasestr(line, "Device 8088:0107")
                    || strcasestr(line, "Device 8088:0103")
                    || strcasestr(line, "Device 8088:0101")
                    || strcasestr(line, "Device 8088:2001"))
				{
                    int x710 = 0;
                
                    if (strcasestr(line, "X710") || strcasestr(line, "Device 8088:2001"))
                        x710 = 1;
                
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
					{
						*p_char = 0;
					}
                    else
                    {
                        p_char = strcasestr(line, "Device 8088:");
                        if (p_char)
    					{
    						*p_char = 0;
    					}
                    }
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
            if (force_reverse
                || (ruijie_x_serial_ext_card_entry[sorted_num].ethNum >= 2
                && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == ruijie_x_serial_ext_card_entry[sorted_num].have_x710_num))
            {
                for (eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; --eth_index)
				{
					if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
					{
						int ethAttribute = (eth_num&1);
												
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
						eth_num++;
					}
					else
					{
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
						if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
						else
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
						nm_index++;
					}		
				}
            }
            else
            {
    			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
    			{
    				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
    				{
    					int ethAttribute = (eth_num&1);
    										
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    					eth_num++;
    				}
    				else
    				{
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
    					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
    					else
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
    					nm_index++;
    				}
    				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
    					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
    			}
            }
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
            {
                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
                fwrite(line, strlen( line ), sizeof( char ), fp );
            }
        }
        fclose(fp);
        fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static void topsec_auto_rename_x_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "--+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet Controller"))
				{
                    int x710 = 0;
                
                    if (strcasestr(line, "X710"))
                        x710 = 1;
                
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
						*p_char = 0;
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
            if (force_reverse
                || (ruijie_x_serial_ext_card_entry[sorted_num].ethNum >= 2
                && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == ruijie_x_serial_ext_card_entry[sorted_num].have_x710_num))
            {
                for (eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; --eth_index)
				{
					if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
					{
						int ethAttribute = (eth_num&1);
												
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
						eth_num++;
					}
					else
					{
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
						if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
						else
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
						nm_index++;
					}		
				}
            }
            else
            {
    			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
    			{
    				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
    				{
    					int ethAttribute = (eth_num&1);
    										
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    					eth_num++;
    				}
    				else
    				{
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
    					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
    					else
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
    					nm_index++;
    				}
    				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
    					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
    			}
            }
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
            {
                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
                fwrite(line, strlen( line ), sizeof( char ), fp );
            }
        }
        fclose(fp);
        fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static const char* topsec_t3e_1u_ext_card_feature_string[] = 
{
	"       +-01.0-",
	"       +-03.0-",
	"       +-04.0-",
	"       +-14.0",
	"       +-14.1",
	"       +-14.2",
	"       +-14.3",
};
const static char topsec_t3e_1u_ext_card_id_char[][3] = 
{
	"5",
	"6",
	"7",
	"1",
	"2",
	"3",
	"4",
};

static void topsec_auto_rename_x2_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                next_feature:
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;

                    while (!(strcasestr(line, "Network Connection") || strcasestr(line, "Ethernet ")))
                    {
                        if(0 == fgets(line, sizeof(line), fp ) )
                        {
                            fclose(fp);
                            goto end;
                        }

                        line_size = strlen(line);
            			/**remove '\n'**/
            			line[line_size - 1] = 0;
            			--line_size;
                        //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);

                        if (card_index < card_max - 1)
                        {
                            if ((card_feature[card_index+1][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index + 1]))
                            {
                                card_index++;
                                ruijie_x_serial_ext_card_number--;
                                goto next_feature;
                            }
                        }
                    }
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}
                else if (feature_match && p_char)
                {
                    strcpy(device_prefix, "00");
                }
			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet "))
				{
                    int x710 = 0;
                    int fiber = 0;
                
                    if (strcasestr(line, "X710"))
                        x710 = 1;

                    if (strcasestr(line, "Fiber"))
                        fiber = 1;
                
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
						*p_char = 0;
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
                            if (fiber)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_fiber_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || (!device_match && feature_match) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}

end:
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
            if (ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 
                && ruijie_x_serial_ext_card_entry[sorted_num].have_fiber_num == 8)
            {
                int ethPairStart = 1 + (eth_num>>1);
    			for (eth_index = 1; eth_index < 8; eth_index += 2)
    			{
                    int ethAttribute = ((eth_index / 2)&1);
    				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = ethPairStart + (eth_index / 4);
    				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    				snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    				eth_num++;
                    //printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
                }
                ethPairStart += 2;
                for (eth_index = 0; eth_index < 8; eth_index += 2)
    			{
                    int ethAttribute = ((eth_index / 2)&1);
    				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = ethPairStart + (eth_index / 4);
    				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    				snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    				eth_num++;
                    //printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
                }
            }
            else
            {
    			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
    			{
    				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
    				{
    					int ethAttribute = (eth_num&1);
    										
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
    					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
    					eth_num++;
    				}
    				else
    				{
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
    					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
    					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
    					else
    						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
    					nm_index++;
    				}
    				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName,
                    //    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
    			}
            }
		}

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
            {
                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
                fwrite(line, strlen( line ), sizeof( char ), fp );
            }
        }
        fclose(fp);
        fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static const char* topsec_t3e_2u_ext_card_feature_string[] = 
{
	"       +-01.0-",
	"       +-03.0-",
	"       +-04.0-",
	"       +-14.0 ",
	"       +-14.1 ",
	"       +-14.2 ",
	"       +-14.3 ",
};
const static char topsec_t3e_2u_ext_card_id_char[][3] = 
{
	"5",
	"6",
	"7",
	"1",
	"2",
	"3",
	"4",
};

static void topsec_auto_rename_t3e_2u_serial_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "--+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

            if (!device_prefix[0] 
				&& feature_match 
				&& !device_match 
				&& strstr(line, "Intel Corporation Ethernet Connection I354")
				&& strstr(line, "+-14."))
			{
				strcpy(device_prefix, "00");
			}

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet Controller")
                    || strcasestr(line, "Device 8088:0107")
                    || strcasestr(line, "Device 8088:0103")
                    || strcasestr(line, "Device 8088:0101")
                    || strcasestr(line, "Device 8088:2001")
                    || strcasestr(line, "Device 8088:1001")
                    || strcasestr(line, "Ethernet Connection")
					)
				{
                    int x710 = 0;
                
                    if (strcasestr(line, "X710"))
                        x710 = 1;
                
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
					{
						*p_char = 0;
					}
                    else
                    {
                        p_char = strcasestr(line, "Device 8088:");
                        if (p_char)
    					{
    						*p_char = 0;
    					}
                    }
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_char + 7);
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
            
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
				{
					int ethAttribute = (eth_num&1);
										
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
					else
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
					nm_index++;
				}
				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
			}
        }

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
            {
                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
                fwrite(line, strlen( line ), sizeof( char ), fp );
            }
        }
        fclose(fp);
        fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static void topsec_auto_rename_t3e_2u_serial_ext_card_with_Wangxun_RP1000(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char* p_pciId = NULL;
	char* p_pciId2 = NULL;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
            //printf("%s %s %d line %s(size=%d) tail %c%c\r\n", __FUNCTION__,__FILE__,__LINE__,line,line_size,line[line_size-1],line[line_size-2]);
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
				{
                    //printf("%s %s %d feature %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_feature[card_index]);

					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					++ruijie_x_serial_ext_card_number;
					feature_match = 1;
					eth_index= 0;
					break;
				}
			}
			if (line[line_size - 1] != '-' && strstr(line, "      "))
			{
                //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

				p_left = strrchr(line, '[');
				p_right = strrchr(line, ']');
				p_char = strstr(line, "--+-");
				memset(tmpstr, 0x00, sizeof(tmpstr));

				if (!feature_match && p_char && (p_char < p_left))
				{
					/**This is other PCI device**/
				}
				else if (p_left && p_right && p_right - p_left > 2)
				{
					memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
					p_char = strchr(tmpstr, '-');
					if (p_char)
					{
						*p_char = 0;
					}
					strncpy(device_prefix, tmpstr, sizeof(device_prefix));
					device_match = 1;
				}

			}
            //printf("%s %s %d device_prefix %s\r\n", __FUNCTION__,__FILE__,__LINE__,device_prefix);

            if (!device_prefix[0] 
				&& feature_match 
				&& !device_match 
				&& strstr(line, "Intel Corporation Ethernet Connection I354")
				&& strstr(line, "+-14."))
			{
				strcpy(device_prefix, "00");
			}

			if (device_prefix[0])
			{
				if (strcasestr(line, "Network Connection")
                    || strcasestr(line, "Ethernet Controller")
                    || strcasestr(line, "Device 8088:0107")
                    || strcasestr(line, "Device 8088:0103")
                    || strcasestr(line, "Device 8088:0101")
                    || strcasestr(line, "Device 8088:2001")
                    || strcasestr(line, "Device 8088:1001")
                    || strcasestr(line, "Ethernet Connection")
					|| strcasestr(line, "Beijing Wangxun Technology Co., Ltd.")
					)
				{
                    int x710 = 0;
                
                    if (strcasestr(line, "X710"))
                        x710 = 1;

					p_pciId = NULL;
					p_pciId2 = NULL;
					
					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
					{
						*p_char = 0;
					}
                    else
                    {
                        p_char = strcasestr(line, "Device 8088:");
                        if (p_char)
    					{
    						*p_char = 0;

							p_pciId = p_char;
    					}
						else
						{
							p_char = strcasestr(line, "Beijing Wangxun Technology Co., Ltd. Ethernet Controller RP1000 for 10GbE SFP+");
							
							if (p_char)
							{
								*p_char = 0;

								p_pciId2 = p_char;
							}
						}
                    }
					
					// not RP1000, other Wangxun
					if ( !p_pciId && !p_pciId2 && !p_char )
					{
						p_char = strcasestr(line, "Beijing Wangxun Technology Co., Ltd.");
						if (p_char)
						{
							*p_char = 0;
						}
					}
					
					p_char = strrchr(line, '-');
					if (p_char)
					{
						if (eth_index > MAX_ETH_ONE_CARD)
						{
							goto error_out;
						}
						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);
							/**8086:1521**/
							if ( p_pciId )
							{
								snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%9s", p_pciId + 7);
							}
							else if ( p_pciId2 )
							{
								snprintf( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId, 16, "%s", "8088:1001" );
							}
							if ('n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
							{
								if (card_id[card_index][1] != '\0')
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = card_id[card_index][1];
								}
								else
								{
									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethMgtIdx = 0;
								}
							}
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].x710 = x710;
                            if (x710)
                                ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].have_x710_num++;
						}
						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
				p_char = strrchr(line, '\\');
				/**ONE eth in a PCI:*/
				if ((!device_match && p_char) || 'n' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].cardId)
				{
					memset(device_prefix, 0x00, sizeof(device_prefix));
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
            //printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
            //printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            //printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);

			int iEthNum = ruijie_x_serial_ext_card_entry[sorted_num].ethNum;

			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if ( ( 2 == iEthNum ) && ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciId, "8088:1001", strlen( "8088:1001" ) ) ) )
				{
					int ethAttribute = ( ( iEthNum - 1 - eth_index ) & 1 );
					
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (( iEthNum - 1 - eth_index )>>1) + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
				{
					int ethAttribute = (eth_num&1);

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
				else
				{
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1) + (nm_index >> 1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = 2;
					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
					else
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
					nm_index++;
				}
				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
					//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
			}
        }

		for (eth_num = 0; eth_num < eth_total; ++eth_num)
		{
			snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/eth%d/device", eth_num);
			fp = popen(tmpstr, "r");
			if ( fp )
			{
				memset(line, 0x00, sizeof(line));
				if (0 != fgets(line, sizeof(line), fp ))
				{
					line_size = strlen(line);
					/**remove '\n'**/
					line[line_size - 1] = 0;
					--line_size;

					p_char = strrchr(line, '/');
					if (p_char)
					{
						p_char += 6;
                        //printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

						for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
						{
							for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
							{
                                //printf("%s %s %d eth%d p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
                                //	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

								if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
								{
									snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "eth%d", eth_num);
                                    //printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
                                    //	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
									break;
								}
							}
						}
					}
				}
				pclose(fp);
				fp = NULL;
			}
		}
#if 1
        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			int iEthNum = ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			if ( ( 2 == iEthNum ) && ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethPciId, "8088:1001", strlen( "8088:1001" ) ) ) )
			{
				for (eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1; eth_index >= 0; --eth_index)
				{
					sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
			}
			else
			{
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
					sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
				}
			}
        }
        fclose(fp);
        fp = NULL;
#endif
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static const char* topsec_feiteng_ft_2000_ext_card_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:01.0/0000:02:00.0/0000:03:00.0/",
	" ../../devices/pci0000:00/0000:00:01.0/0000:02:00.0/0000:03:01.0/",
	" ../../devices/pci0000:00/0000:00:03.0/",
	" ../../devices/pci0000:00/0000:00:04.0/",
	" ../../devices/platform/PHYT0004:00/",
	" ../../devices/platform/PHYT0004:01/",
};
const static char topsec_feiteng_ft_2000_ext_card_id_char[][3] = 
{
	"5",
    "6",
	"3",
	"4",
	"1",
	"2",
};

static void  topsec_feiteng_ft_2000_ext_card(const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int eth_total, int force_reverse)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp))
		{
        retry:
            eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
                    ++ruijie_x_serial_ext_card_number;
					
					do
					{
                        if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
                        {
                            goto retry;
                        }
                        
                        char *p = strrchr(line, '/');
                        if (p)
                        {
                            strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1),
                                sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
                        }
                    }while(0 != fgets(line, sizeof(line), fp));
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				int ethAttribute = (eth_num&1);
										
				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
				snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
				eth_num++;
			}

        }

        fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
        if (fp)
        {
            sprintf( line, "ifAutoCfgMode %d\n", 0 );
            fwrite( line, strlen( line ), sizeof( char ), fp );
        }

        for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
        {
            sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
            {
                sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
                    ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
                    (ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
                fwrite(line, strlen( line ), sizeof( char ), fp );
            }
        }
        fclose(fp);
        fp = NULL;
    }

error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static const char* rename_sorted_gongfeng_c3k_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:16.0/",
	" ../../devices/pci0000:00/0000:00:17.0/",
	" ../../devices/pci0000:00/0000:00:0e.0/",
	" ../../devices/pci0000:00/0000:00:09.0/",
};
const static char rename_sorted_gongfeng_c3k_card_id_char[][3] = 
{
	"1",
    "2",
	"3",
	"4",
};

const static char rename_sorted_gongfeng_c3k_card_mode_char[][3] = 
{
	"1",
    "0",
	"2",
	"2",
};

static const char* rename_sorted_gongfeng_c3k_name_string[] = 
{
	"onboard",
	"onboard",
	"slot2",
	"slot3",
};

static void  topsec_3k_ext_card_by_sys_class_net(const char* card_feature[], const char card_id[][3], const char card_mode[][3], const char *card_name[], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp))
		{
        retry:
            eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = card_mode[card_index][0];
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
                    ++ruijie_x_serial_ext_card_number;
					
					do
					{
                        if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
                        {
                            goto retry;
                        }
                        
                        char *p = strrchr(line, '/');
                        if (p)
                        {
                            strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1),
                                sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
                        }
                    }while(0 != fgets(line, sizeof(line), fp));
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}
    
	if (ruijie_x_serial_ext_card_number)
	{
        eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}
        
		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if (ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 20)
			{
				const static unsigned int map[20] = {0, 1, 2, 3, 4, 5, 6, 7, 18, 16, 8, 9, 10, 11, 12, 13, 14, 15, 19, 17};
				int min_value = 100;
				for (eth_index = 0; eth_index < 20; ++eth_index)
				{
					int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]);
					if (a < min_value)
						min_value = a;					
				}

				for (eth_index = 0; eth_index < 20; ++eth_index)
				{
					int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]) - min_value;
					if (a >= 0 && a < 20)
					{
						int ethAttribute = (map[a]&1);

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ((eth_num+map[a])>>1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					}
				}
				eth_num+=20;
			}
			else if (ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 10)
			{
				const static unsigned int map[10] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 8};
				int min_value = 100;
				for (eth_index = 0; eth_index < 10; ++eth_index)
				{
					int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]);
					if (a < min_value)
						min_value = a;					
				}

				for (eth_index = 0; eth_index < 10; ++eth_index)
				{
					int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]) - min_value;
					if (a >= 0 && a < 10)
					{
						int ethAttribute = (map[a]&1);

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ((eth_num+map[a])>>1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					}
				}
				eth_num+=10;
			}
			else if (ruijie_x_serial_ext_card_entry[sorted_num].mode == '2' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2)
			{
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
					int ethAttribute = !(eth_num&1);
												
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
					eth_num++;
				}
			}
			else
			{
	            for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
	                if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
	                {
	                	int ethAttribute = (eth_num&1);
											
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + (eth_num>>1);
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
						eth_num++;
	                }
	                else
	                {
						if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
						else
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
						nm_index++;
	                }
				}
			}
        }
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
	if (fp)
	{
		sprintf( line, "ifAutoCfgMode %d\n", 0 );
		fwrite( line, strlen( line ), sizeof( char ), fp );
	}

	char slot_name[CARD_SLOD_NAME_LEN]={0};
	int idx_in_slot = 0;
	int ethPair = 1;
	for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
	{
		sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
		int used_eth_num = 0;
		int trust = 1;
		
		if (ethPair > NORMAL_CAR_TUNNEL_NUM)
			break;
		
		for (used_eth_num = 0; used_eth_num < ruijie_x_serial_ext_card_entry[sorted_num].ethNum;)
		{
			//printf("%s %d used_eth_num %d ethNum %d card_slot %s trust %d ethPair %d\n",
			//	__FUNCTION__, __LINE__, used_eth_num, ruijie_x_serial_ext_card_entry[sorted_num].ethNum, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, trust, ethPair);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].used)
					continue;
				
				//printf("%s %d eth_index %d ethPair %d-%d trust %d-%d\n",
				//	__FUNCTION__, __LINE__, eth_index, ethPair, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair, trust, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute);
					
				if (ethPair != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair
					|| !((trust && !ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute) || (!trust && ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute)))
					continue;
				
				if (strcmp(slot_name, ruijie_x_serial_ext_card_entry[sorted_num].card_slot))
				{
					idx_in_slot = 0;
					strncpy(slot_name, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, sizeof(slot_name)-1);
				}
				
				sprintf(line, "#panel_info/%s/%s/%s%d\n",
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, slot_name, !strcmp(slot_name, "onboard")?"eth":"", idx_in_slot);
						
				fwrite(line, strlen( line ), sizeof( char ), fp );
				sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
					(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
				fwrite(line, strlen( line ), sizeof( char ), fp );
				ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].used=1;
				//printf("%s %d eth_index %d ethPair %d trust %d %s->%s\n",
				//	__FUNCTION__, __LINE__, eth_index, ethPair, trust, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName);
				used_eth_num++;
				idx_in_slot++;
			}
			
			if (!trust)
			{
				ethPair++;
				if (ethPair > NORMAL_CAR_TUNNEL_NUM)
					break;
			}
			trust = !trust;
		}
	}
	fclose(fp);
	fp = NULL;
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static int check_eth_type( int iIndex, const char* dstType )
{
	int iRet = 0;
	
	if ( dstType )
	{
		char szTemp[256] = { 0 };
		
		snprintf( szTemp , sizeof( szTemp ), "ethtool eth%d | grep '%s'", iIndex, dstType );

		iRet = !( system ( szTemp ) );
	}
	
	return iRet;
}

// Device 8088:1001
// Ethernet controller: Beijing Wangxun Technology Co., Ltd. Ethernet Controller RP1000 for 10GbE SFP+
static int check_wx_RP1000P2SFP ( int iEthNum )
{
	int i = 0;
	int iRet = 0;
	int iPicCount = 0;
	int iEthCount = 0;
	char szCommand[256] = {0};
	char szReadLine[512] = {0};
	char szPicNames[32][16] = {0};
	//char szEthNames[32][16] = {0};
	
	if ( iEthNum > 0 )
	{
		FILE * fPci = popen( "lspci | grep -E 'Device 8088:1001|Beijing Wangxun Technology Co., Ltd. Ethernet Controller RP1000 for 10GbE SFP+' | awk '{print $1}'", "r" );
		if ( fPci )
		{
			// 04:00.0
			while ( NULL != fgets( szReadLine, sizeof( szReadLine ), fPci ) )
			{
				if ( iPicCount >= 32 )
				{
					printf( "[%s:%d] ==> error, Find more than 32 net device.\n", __FUNCTION__, __LINE__ );
					
					iPicCount = 0;
					
					break;
				}
				
				snprintf( szPicNames[iPicCount], 16, "%s" , szReadLine );
				
				szPicNames[iPicCount][7] = 0;
				
				iPicCount++;
			}

			pclose( fPci );
			
			fPci = NULL;
		}
		
		if ( ( iPicCount > 0 ) && ( 0 == iPicCount % 2 ) )
		{
			int eth_num = 0;
			int line_size = 0;

			for ( eth_num = 0; eth_num < iEthNum; ++eth_num )
			{
				snprintf( szCommand, sizeof( szCommand ), "readlink /sys/class/net/eth%d/device", eth_num );
				FILE *fpLink = popen( szCommand, "r" );
				if ( fpLink )
				{
					memset( szReadLine, 0x00, sizeof( szReadLine ) );

					if ( 0 != fgets( szReadLine, sizeof( szReadLine ), fpLink ) )
					{
						line_size = strlen( szReadLine );
						/**remove '\n'**/
						szReadLine[line_size - 1] = 0;
						--line_size;

						// /0000:04:00.0
						char* p_char = strrchr( szReadLine, '/' );

						if ( p_char )
						{
							if ( strlen( p_char ) > 6 )
							{
								// 04:00.0
								p_char += 6;

								for ( i=0; i<iPicCount; i++ )
								{
									if ( 0 == strcmp( p_char, szPicNames[i] ) )
									{
										if ( check_eth_type( eth_num, "FIBRE" ) )
										{
											//snprintf( szEthNames[iEthCount], 16, "eth%d" , eth_num );

											iEthCount++;
										}
										
										break;
									}
								}
							}
						}
					}

					pclose( fpLink );

					fpLink = NULL;
				}
			}
			
			if ( iEthCount == iPicCount )
			{
				iRet = 1;
			}
		}
	}
	
	return iRet;
}

static const char* topsec_t5h_ext_card_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:1d.7/",
	" ../../devices/pci0000:00/0000:00:1d.0/",
	" ../../devices/pci0000:00/0000:00:01.0/",
	" ../../devices/pci0000:00/0000:00:01.1/",
	" ../../devices/pci0000:00/0000:00:1c.0/",
	" ../../devices/pci0000:00/0000:00:1c.4/"
};

const static char topsec_t5h_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6"
};

static const char* topsec_t5h_ext_card_name_string[] = 
{
	"onboard1",
	"onboard2",
	"slot1",
	"slot2",
	"slot3",
	"slot4"
};

static void topsec_auto_rename_t5h_serial_ext_card( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	int   i                           = 0;
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	int eth_index = 0;
	char device_prefix[32] = {0};
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int iMgtEth = 0;
	unsigned int iDpdkEth = 0;
	char szPanelInfos[ACE_MAX_SUPPORT_PORT_BUSYBOX][64] = {0};
	char szExtCard[256] = {0};

	memset( device_prefix, 0x00, sizeof( device_prefix ) );

	fp = popen( "ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
retry:
			eth_index = 0;

			for ( card_index = 0; card_index < card_max; ++card_index )
			{
				if ( !strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if ( strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
						{
							goto retry;
						}

						char *p = strrchr( line, '/' );

						if ( p )
						{
							strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim( p + 1 ), sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethOldName ) - 1 );

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while( 0 != fgets( line, sizeof( line ), fp ) );
				}
			}
		}

		pclose( fp ); fp = NULL;
	}

	if ( ruijie_x_serial_ext_card_number )
	{
		eth_num = 0;

		while ( eth_num < ruijie_x_serial_ext_card_number )
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index )
			{
				if ( !is_sorted[eth_index] )
				{
					break;
				}
			}

			sorted_num = eth_index;

			for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
			{
				if ( is_sorted[card_index] )
				{
					continue;
				}

				if ( sorted_num == card_index )
				{
					continue;
				}

				if ( ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId )
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		eth_num = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = { 0 };

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1581" );

					if ( iDriver && iDevType )
					{
						// 1 0
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1572" );

					if ( iDriver && iDevType )
					{
						// 3 2 1 0
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';
					}
				}
				else if ( 8 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1521" );

					if ( iDriver && iDevType )
					{
						// 1 0 3 2 5 4 7 6
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1522" );

						if ( iDriver && iDevType )
						{
							// 1,0,3,2,5,4,7,6
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
						}
					}
				}
			}

			if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "HA" );

				sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[0].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
				snprintf( szPanelInfos[0], sizeof( szPanelInfos[0] ) - 1, "#panel_info/%s/%s/%s\n", "HA", "onboard", "HA" );

				iMgtEth++;
			}
			else if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "MGMT" );

				sprintf( g_port_info[1].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[1].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
				snprintf( szPanelInfos[1], sizeof( szPanelInfos[1] ) - 1, "#panel_info/%s/%s/%s\n", "MGMT", "onboard", "MGMT" );

				iMgtEth++;
			}
			// 1 0 3 2 ...
			else if ( '1' == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index + ( ( 0 == ( eth_index % 2 ) ) ? 1 : -1 );

					int ethAttribute = ( ( iEthIndex - iMgtEth ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtEth ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						snprintf( szPanelInfos[iEthIndex], sizeof( szPanelInfos[iEthIndex] ) - 1, "#panel_info/%s/%s/%d\n", 
							g_port_info[iEthIndex].newPhyname, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			// 3 2 1 0 ...
			else if ( '2' == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				int iTempIndex = 0;

				for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1, iTempIndex = 0; eth_index >= 0; eth_index--, iTempIndex++ )
				{
					int iEthIndex = eth_num + iTempIndex;

					int ethAttribute = ( ( iEthIndex - iMgtEth ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtEth ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						snprintf( szPanelInfos[iEthIndex], sizeof( szPanelInfos[iEthIndex] ) - 1, "#panel_info/%s/%s/%d\n", 
							g_port_info[iEthIndex].newPhyname, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index;

					int ethAttribute = ( ( iEthIndex - iMgtEth ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtEth ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						snprintf( szPanelInfos[iEthIndex], sizeof( szPanelInfos[iEthIndex] ) - 1, "#panel_info/%s/%s/%d\n", 
							g_port_info[iEthIndex].newPhyname, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}

			if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "slot", strlen( "slot" ) ) )
			{
				if ( !szExtCard[0] )
				{
					snprintf( szExtCard, sizeof( szExtCard ) - 1, "#%s", ruijie_x_serial_ext_card_entry[sorted_num].card_slot );
				}
				else
				{
					strcat( szExtCard, "," );

					strcat( szExtCard, ruijie_x_serial_ext_card_entry[sorted_num].card_slot );
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}

		iDpdkEth = eth_num - iMgtEth;

		if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
		{
			int x = 0;
			int iVppEth = 0;
			char szLine[128] = {0};
			char szVppEthCache[128][64] = {0};
		
			int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );
		
			if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
			{
				if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
				{
					unlink( ACE_VPP_PORT_MAP_FILE );
		
					init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
				}
				else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
				{
					int iEnd = 0;
		
					for( i = 0; i < iDpdkEth && !iEnd; ++i )
					{
						if ( ( 's' != g_port_info[i].newPhyname[0] ) && ( 'M' != g_port_info[i].newPhyname[0] ) && ( 'H' != g_port_info[i].newPhyname[0] ) )
						{
							int iFind = 0;
		
							int iPair = 1;
							int iEthAttribute = 0;
		
							if ( ( 'L' == g_port_info[i].newPhyname[0] ) || ( 'W' == g_port_info[i].newPhyname[0] ) )
							{
								iPair = atoi( g_port_info[i].newPhyname + 3 );
							}
		
							if ( 'W' == g_port_info[i].newPhyname[0] )
							{
								iEthAttribute = 1;
							}
							else if ( 'M' == g_port_info[i].newPhyname[0] )
							{
								iEthAttribute = 2;
							}
		
							snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname, iPair,
								( ruijie_rename_attribute_name[iEthAttribute] ) );
		
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
		
								init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
							}
						}
					}
				}
			}
		}

		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

		if ( fp )
		{
			sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

			fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

			for ( eth_index = 0; eth_index < eth_num; eth_index++ )
			{
				int iPair = 1;
				int ethAttribute = 0;

				if ( ( 'L' == g_port_info[eth_index].newPhyname[0] ) || ( 'W' == g_port_info[eth_index].newPhyname[0] ) )
				{
					iPair = atoi( g_port_info[eth_index].newPhyname + 3 );
				}

				if ( g_port_info[eth_index].newPhyname[0] == 'W' )
				{
					ethAttribute = 1;
				}
				else if ( ( g_port_info[eth_index].newPhyname[0] == 'M' ) || ( g_port_info[eth_index].newPhyname[0] == 'H' ) )
				{
					ethAttribute = 2;
				}

				fwrite( szPanelInfos[eth_index], strlen( szPanelInfos[eth_index] ), sizeof( char ), fp );

				sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[eth_index].oldPhyname, g_port_info[eth_index].newPhyname, iPair,
					( ruijie_rename_attribute_name[ethAttribute] ) );

				fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
			}

			fclose( fp ); fp = NULL;
		}

		if ( strlen( szExtCard ) )
		{
			fp = fopen( "/home/ace_bypass_map_t5h_ext.dat", "w+" );

			if ( NULL != fp )
			{
				fwrite( szExtCard, 1, strlen( szExtCard ), fp );

				fclose( fp ); fp = NULL;
			}
		}
	}

error_out:
	if ( fp )
	{
		pclose( fp );
	}

	return;
}

static const char* rename_sorted_c2350_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:01.4/",
	" ../../devices/pci0000:00/0000:00:01.2/",
	" ../../devices/pci0000:00/0000:00:03.2/",
	" ../../devices/pci0000:00/0000:00:03.1/",
	" ../../devices/pci0000:00/0000:00:01.1/"
};

const static char rename_sorted_c2350_card_id_char[][3] = 
{
	"1", // board
	"2", // slot1
	"3", // slot2
	"4", // slot3
	"5"  // slot4
};

static const char* rename_sorted_c2350_name_string[] = 
{
	"onboard",
	"slot1",
	"slot2",
	"slot3",
	"slot4"
};

static void topsec_c3250_ext_card_by_sys_class_net( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char  device_prefix[32] = { 0 };
	int   eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;

	memset( device_prefix, 0x00, sizeof( device_prefix ) );

	fp = popen( "ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
retry:
			eth_index = 0;

			for ( card_index = 0; card_index < card_max; ++card_index )
			{
				if ( !strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot ) - 1 );
					++ruijie_x_serial_ext_card_number;

					do
					{
						if ( strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
						{
							goto retry;
						}

						char *p = strrchr( line, '/' );

						if ( p )
						{
							strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim( p + 1 ), sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName ) - 1 );

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while ( 0 != fgets( line, sizeof( line ), fp ) );
				}
			}
		}

		pclose( fp );

		fp = NULL;
	}

	if ( ruijie_x_serial_ext_card_number )
	{
		eth_num = 0;

		while ( eth_num < ruijie_x_serial_ext_card_number )
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index )
			{
				if ( !is_sorted[eth_index] )
				{
					break;
				}
			}

			sorted_num = eth_index;

			for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
			{
				if ( is_sorted[card_index] )
				{
					continue;
				}

				if (sorted_num == card_index)
				{
					continue;
				}

				if ( ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId )
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		eth_num = 0;
		int iPanelShow = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			iPanelShow = 0;
		
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// 10g
					if ( iDriver && iDevType )
					{
						// 0 1
						iPanelShow = 1;
					}
					else if ( check_dev_content( szInfoFile, "DRIVER=rnp" )
						&& check_dev_content( szInfoFile, "PCI_ID=8848:1000" ) )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = 'a';
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
				}					
			}

			// HA & MGMT
			if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard" ) )
			{
				sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[0].newPhyname, "%s", "HA" );
				sprintf( g_port_info[0].m_szPanel, "%s", "onboard" );
				sprintf( g_port_info[1].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[1].ethOldName );
				sprintf( g_port_info[1].newPhyname, "%s", "MGMT" );
				sprintf( g_port_info[1].m_szPanel, "%s", "onboard" );
			}
			// 3 2 1 0 ...
			//TNS-CARD-STD-4X-1
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
			{
				int iTempIndex = 0;

				for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1, iTempIndex = 0; eth_index >= 0; eth_index--, iTempIndex++ )
				{
					int iEthIndex = eth_num + iTempIndex;

					int ethAttribute = ( ( iTempIndex) % 2 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1/* start with LAN1/WAN1*/ + ( ( iEthIndex - 2/* HA and MGMt*/ ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, iTempIndex );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == 'a' )
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + eth_index ) & 1 );

					int iEthIndex = eth_num + eth_index;

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = ( ( eth_num + eth_index ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );
					}
				}
			}			
			// 2,1,4,3...
			// 8086:1533, 2 port ( board, 2电-千兆 )
			// 8088:0103, 8 port ( 8电-千兆-S82WG-8TB_VER_A-4bypass, 4电4光-千兆-S82WG-4TB4F_VER_A-2bypass, 8光-千兆-S82WG-8F_VER_A )
			// 8088:1001, 2 port ( 2光-万兆-S83WX-2F_VER_A )
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + ( ( eth_index + 1 ) % 2 + ( eth_index / 2 ) * 2 );

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - 2 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;

					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						if ( 1 == iPanelShow )
						{
							sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, ( eth_index + 1 ) % 2 + ( eth_index / 2 ) * 2 );
						}
						else
						{
							sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );
						}
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );

		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		int ethPair = 1;
		int portIndex = 0;

		for ( portIndex = 0; portIndex < eth_num; portIndex++ )
		{
			int ethAttribute = 0;

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' )  )
			{
				ethAttribute = 2;
			}

			if ( 0 == strcmp( g_port_info[portIndex].m_szPanel, "onboard" ) )
			{
				sprintf( tmpstr, "#panel_info/%s/%s/%s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].m_szPanel, g_port_info[portIndex].newPhyname );
			}
			else
			{
				sprintf( tmpstr, "#panel_info/%s/%s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].m_szPanel );
			}

			if ( ( 'M' == g_port_info[portIndex].newPhyname[0] ) || ( 'H' == g_port_info[portIndex].newPhyname[0] ) )
			{
				ethPair = 1;
			}
			else if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				ethPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, ethPair, 
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

error_out:
	if ( fp )
	{
		pclose( fp );
	}

	return;
}

static void topsec_auto_rename_if(int eth_num)
{
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	int   i                           = 0;
	int   fixed_eth_num               = 0;
	int   ext_copper_num              = 0;
	int   ext_fibre_num               = 0;
	int   man_make                    = 0;
	char  szDevType[128]              = { 0 };

	fp = fopen( bypass_switch_config, "r" );
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
	}

	if ( fp )
	{
		fclose(fp); fp = NULL;
	}

	if ( !strstr( tmpstr, "TOPSEC" ) )
	{
		return;
	}

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	if ( 1 == is_user_define_interface() )
	{
		return;
	}

	get_dev_type_interface( szDevType, sizeof( szDevType ) );

#ifdef ARCH_ARM64
    if (have_i_pci_device("17cddc16") && have_i_pci_device("17cddc08") && have_i_pci_device("17cddc01"))
    { 
        FILE *fpxxx = popen("cat /proc/cpuinfo | grep \"model name\" | awk 'NR==1'", "r");
	    if ( fpxxx )
    	{
    		memset(line, 0x00, sizeof(line));
    		fgets(line, sizeof(line), fpxxx );
    		pclose( fpxxx );

    		if (!strncmp(line, "model name\t: FT-2000/4",
    			sizeof("model name\t: FT-2000/4")-1))
    		{
                topsec_feiteng_ft_2000_ext_card(topsec_feiteng_ft_2000_ext_card_feature_string, topsec_feiteng_ft_2000_ext_card_id_char, sizeof(topsec_feiteng_ft_2000_ext_card_id_char) / sizeof(topsec_feiteng_ft_2000_ext_card_id_char[0]), eth_num, 0);
    		}
    	}
    }
#else
    if (have_i_pci_device("8086591f") && have_i_pci_device("80861901"))//T5E 2U
    {
        FILE * fdmi = popen("/usr/sbin/dmidecode -t 2 | grep \"Product Name: \" | awk '{if (NR==1){print $0}}'", "r");
        if (fdmi)
        {
            char dmi_product[128] = { 0 };
            if (NULL != fgets(dmi_product, sizeof(dmi_product), fdmi))
            {
                char *p = strstr(dmi_product, "Product Name: ");
                if (p)
                {
                    p += sizeof("Product Name: ")-1;
                    if (!strncmp(p, "TNS-WX-T5E-2U-", sizeof("TNS-WX-T5E-2U-")-1))// 2U
                    {
                        system("/usr/private/t5e_lcm");
                        topsec_auto_rename_t5e_2u_serial_ext_card(topsec_t5e_2u_ext_card_feature_string, topsec_t5e_2u_ext_card_id_char, sizeof(topsec_t5e_2u_ext_card_id_char) / sizeof(topsec_t5e_2u_ext_card_id_char[0]), eth_num, 0);
                    }
                    else
                    {
                        system("/usr/private/t5e_lcm");
						if (1 || access("/usr/mysys/.force_use_t5e_2u_config", 0) == 0)
							topsec_auto_rename_t5e_2u_serial_ext_card(topsec_t5e_ext_card_feature_string, topsec_t5e_ext_card_id_char, sizeof(topsec_t5e_ext_card_id_char) / sizeof(topsec_t5e_ext_card_id_char[0]), eth_num, 0);
						else
							topsec_auto_rename_x_serial_ext_card(topsec_t5e_ext_card_feature_string, topsec_t5e_ext_card_id_char, sizeof(topsec_t5e_ext_card_id_char) / sizeof(topsec_t5e_ext_card_id_char[0]), eth_num, 0);
                    }
                }
            }
            pclose(fdmi);
        }
    }
    else if (have_i_pci_device("80866f81") && have_i_pci_device("80866f36"))//T7L
    {
        system("/usr/private/t7e_lcm");
        topsec_t7e_auto_rename_x_serial_ext_card(topsec_t7e_ext_card_feature_string, topsec_t7e_ext_card_id_char, topsec_t7e_ext_card_name_string, sizeof(topsec_t7e_ext_card_id_char) / sizeof(topsec_t7e_ext_card_id_char[0]), eth_num, 1);
    }
    else if (have_i_pci_device("80861f08") && have_i_pci_device("80861f10"))
    {
		int iWxRP1000P2SFPExit = check_wx_RP1000P2SFP( eth_num );

        FILE * fdmi = popen("/usr/sbin/dmidecode | grep \"Product Name: \" | awk '{if (NR==1){print $0}}'", "r");
        if (fdmi)
        {
            char dmi_product[128] = { 0 };
            if (NULL != fgets(dmi_product, sizeof(dmi_product), fdmi))
            {
                char *p = strstr(dmi_product, "Product Name: ");
                if (p)
                {
                    p += sizeof("Product Name: ")-1;
                    if (!strcmp(p, "T3E\n"))// 2U
                    {
						// with Beijing Wangxun Technology Co., Ltd. Ethernet Controller RP1000 for 10GbE SFP+ (rev 03)
						if ( iWxRP1000P2SFPExit )
						{
							topsec_auto_rename_t3e_2u_serial_ext_card_with_Wangxun_RP1000( topsec_t3e_2u_ext_card_feature_string, topsec_t3e_2u_ext_card_id_char, sizeof(topsec_t3e_2u_ext_card_id_char) / sizeof(topsec_t3e_2u_ext_card_id_char[0]), eth_num, 0 );
						}
						else
						{
							topsec_auto_rename_t3e_2u_serial_ext_card(topsec_t3e_2u_ext_card_feature_string, topsec_t3e_2u_ext_card_id_char, sizeof(topsec_t3e_2u_ext_card_id_char) / sizeof(topsec_t3e_2u_ext_card_id_char[0]), eth_num, 0);
						}
                    }
                    else if(!strcmp(p, "To be filled by O.E.M.\n"))// 1U
                    {
                        topsec_auto_rename_x2_serial_ext_card(topsec_t3e_1u_ext_card_feature_string, topsec_t3e_1u_ext_card_id_char, sizeof(topsec_t3e_1u_ext_card_id_char) / sizeof(topsec_t3e_1u_ext_card_id_char[0]), eth_num);
                    }
                }
            }

            pclose(fdmi);
        }
    }
    else if (have_i_pci_device("80860f00") && have_i_pci_device("80860f31") && have_i_pci_device("80860f23") && have_i_pci_device("80860f18"))
    {
        FILE * fdmi = popen("/usr/sbin/dmidecode | grep \"Product Name: \" | awk '{if (NR==1){print $0}}'", "r");
		if (fdmi)
		{
			char dmi_product[128] = { 0 };
			if (NULL != fgets(dmi_product, sizeof(dmi_product), fdmi))
			{
				char *p = strstr(dmi_product, "Product Name: ");
				if (p)
				{
					p += sizeof("Product Name: ")-1;
                    if (strstr(p, "T2C-DT\n"))
					{
					}
                    else if (strstr(p, "T2C-1U-6C\n"))
                    {
                    }
                    else if (strstr(p, "T2E-1U-6C\n"))
                    {
                        topsec_auto_rename_t2e_1u_serial_ext_card(topsec_t2e_1u_ext_card_feature_string, topsec_t2e_1u_ext_card_id_char, sizeof(topsec_t2e_1u_ext_card_id_char) / sizeof(topsec_t2e_1u_ext_card_id_char[0]), eth_num, 0);
                    }
                    else if (strstr(p, "T2C-1U-4C\n"))
                    {
                    }
				}
			}

			pclose(fdmi);
		}
	}
	else if (have_i_pci_device("80861980") && have_i_pci_device("808619a1") && have_i_pci_device("808619a2"))
	{
		FILE * fdmi = popen("/usr/sbin/dmidecode -t 2 | grep \"Product Name: \" | awk '{if (NR==1){print $0}}'", "r");
		if (fdmi)
		{
			char dmi_product[128] = { 0 };
			if (NULL != fgets(dmi_product, sizeof(dmi_product), fdmi))
			{
				char *p = strstr(dmi_product, "Product Name: ");
				if (p)
				{
					//TNS-T2H-2U-16C4X2F2E-B-RB-H4M16-GF-1
					//TNS-T3H-2U-16C6X2E-B-RB-H4M32-GF-1
					//TNS-T2H-2U-8C2X2F2E-B-RB-H4-GF-C5516-1
					p += sizeof("Product Name: ")-1;
                    if (!strncmp(p, "TNS-T2H-2U-16C4X2F2E", sizeof("TNS-T2H-2U-16C4X2F2E")-1)
						|| !strncmp(p, "TNS-T3H-2U-16C6X2E", sizeof("TNS-T3H-2U-16C6X2E")-1)
						|| !strncmp(p, "TNS-T2H-2U-8C2X2F2E", sizeof("TNS-T2H-2U-8C2X2F2E")-1))
                    {
						topsec_3k_ext_card_by_sys_class_net(rename_sorted_gongfeng_c3k_feature_string, rename_sorted_gongfeng_c3k_card_id_char, rename_sorted_gongfeng_c3k_card_mode_char, rename_sorted_gongfeng_c3k_name_string, sizeof(rename_sorted_gongfeng_c3k_card_id_char)/sizeof(rename_sorted_gongfeng_c3k_card_id_char[0]), eth_num);
                    }
				}
			}

			pclose(fdmi);
		}
	}
	else if ( have_i_pci_device( "80863e30" ) && have_i_pci_device( "80861901" ) && have_i_pci_device( "80861905" ) )
	{
		FILE * fdmi = popen( "/usr/sbin/dmidecode -t 1 | grep 'Product Name: ' | awk '{if (NR==1){print $0}}'", "r" );

		if ( fdmi )
		{
			char dmi_product[128] = { 0 };

			if ( NULL != fgets( dmi_product, sizeof( dmi_product), fdmi ) )
			{
				char *p = strstr( dmi_product, "Product Name: " );

				if ( p )
				{
					p += sizeof( "Product Name: " ) - 1;

					if ( strstr( p, "T5H\n" ) )
					{
#ifndef ARCH_ARM64
						led_control_t5h( 0x72, 0 );
#endif
						remove( "/home/ace_bypass_map_t5h_ext.dat" );

						topsec_auto_rename_t5h_serial_ext_card( topsec_t5h_ext_card_feature_string, topsec_t5h_ext_card_id_char, 
							topsec_t5h_ext_card_name_string, sizeof( topsec_t5h_ext_card_id_char ) / sizeof( topsec_t5h_ext_card_id_char[0] ) );
					}
				}
			}

			pclose( fdmi );
		}
	}
	else if ( have_i_pci_device( "1d941450" ) && have_i_pci_device( "1d941451" ) && have_i_pci_device( "1d941452" ) )
	{
		FILE * fdmi = popen( "/usr/sbin/dmidecode -t 2 | grep 'Product Name: ' | awk '{if (NR==1){print $0}}'", "r" );

		if ( fdmi )
		{
			char dmi_product[128] = { 0 };

			if ( NULL != fgets( dmi_product, sizeof( dmi_product ), fdmi ) )
			{
				char *p = strstr( dmi_product, "Product Name: " );

				if ( p )
				{
					// TNS-HG-2U-GF2C4E
					p += sizeof( "Product Name: " ) - 1;

					// TNS-HGSTD-2U-4E-RB-C3250-4M16-H4MS16-GF-1
					if ( !strncmp( p, "TNS-HG-2U-GF2C4E", sizeof( "TNS-HG-2U-GF2C4E" ) - 1 ) || !strncmp( p, "TNS-HGSTD-2U-4E-RB-C3250", sizeof( "TNS-HGSTD-2U-4E-RB-C3250" ) - 1 ) )
					{
						topsec_c3250_ext_card_by_sys_class_net( rename_sorted_c2350_feature_string, rename_sorted_c2350_card_id_char, 
							rename_sorted_c2350_name_string, sizeof( rename_sorted_c2350_card_id_char ) / sizeof( rename_sorted_c2350_card_id_char[0] ) );
					}
				}
			}

			pclose( fdmi );
		}
	}
#endif

	return;
}
#endif

static int get_phy_name_by_mac(char *name, char *mac)
{
    FILE*   fp                            = NULL;
    char    buffer[512]  = { 0 };
    int     i;
    char command[256] = { 0 };

    snprintf(command, sizeof(command)-1, "/sbin/ifconfig -a|grep Ethernet|grep %s|awk \'{print$1}\'", mac);

    fp = popen(command, "r");
    if ( !fp )
    {
        return 0;
    }

    while ( 0 != fgets(buffer, sizeof(buffer), fp ) )
    {
        
        for (i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] == '\r' || buffer[i] == '\n')
                break;
            name[i] = buffer[i];
        }

        pclose( fp );
        return 0;
    }

    return -1;
}

#if 57
static int rename_sorted_chuangshi_nk05_ports[] = {1,0,3,2,5,4};

static const char* zhuoyuexintong_chuangshi_nk05_map[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3"
};

static int rename_sorted_chuangshi_nk02_ports[] = {2,1,3,4,5,6,7,8,9,10,0};

static const char* zhuoyuexintong_chuangshi_nk02_map[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
	"MGT1"
};

static int rename_sorted_chuangshi_nk06_ports[] = {0,1,2,3,9,8,7,4,5,6};

static const char* chuangshi_nk06_ports_map[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
};

static const char* rename_sorted_chuangshi_nk06_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:0f.0/",
	" ../../devices/pci0000:00/0000:00:10.0/",
	" ../../devices/pci0000:00/0000:00:11.0/",
	" ../../devices/pci0000:00/0000:00:16.0",
	" ../../devices/pci0000:00/0000:00:17.0/",
	" ../../devices/pci0000:00/0000:00:0b.0/",
	" ../../devices/pci0000:00/0000:00:09.0/"
};
const static char rename_sorted_chuangshi_nk06_card_id_char[][3] = 
{
	"3",
	"2",
	"1",
	"4",
	"5",
	"6",
	"7"
};

const static char rename_sorted_chuangshi_nk06_card_mode_char[][3] = 
{
	"1",
	"0",
	"0",
	"0",
	"0",
	"0",
	"0"
};


static void  ext_card_by_sys_class_net(const char* card_feature[], const char card_id[][3], const char card_mode[][3], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp))
		{
        retry:
            eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
					{
						goto error_out;
					}
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = card_mode[card_index][0];
                    ++ruijie_x_serial_ext_card_number;
					
					do
					{
                        if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
                        {
                            goto retry;
                        }
                        
                        char *p = strrchr(line, '/');
                        if (p)
                        {
                            strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1),
                                sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);
                            ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
                       }
                    }while(0 != fgets(line, sizeof(line), fp));
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}

		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if (ruijie_x_serial_ext_card_entry[sorted_num].cardId != 'n')
			{
				int min_value = 1000;
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
					int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]);
					if (a < min_value)
						min_value = a;
				}

				if (ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4)
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]) - min_value;
						const static unsigned int map[4] = {1, 2, 3, 0};

						if (a >= 0 && a < ruijie_x_serial_ext_card_entry[sorted_num].ethNum)
						{
							int ethAttribute = ((eth_num+map[a])&1);

							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ((eth_num+map[a])>>1);
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
						}
					}
				}
				else
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						int a = atoi(&ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName[3]) - min_value;
						if (a >= 0 && a < ruijie_x_serial_ext_card_entry[sorted_num].ethNum)
						{
							int ethAttribute = ((eth_num+a)&1);

							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ((eth_num+a)>>1);
							ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
							snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);
						}
					}
				}
				eth_num+=ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			else
			{
				if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx != 0)
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethMgtIdx);
				else
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "MGT%d", nm_index + 1);
				nm_index++;
			}
		}

		fp = fopen( ACE_PORT_MAP_FILE, "wt+" );
		if (fp)
		{
			sprintf( line, "ifAutoCfgMode %d\n", 0 );
			fwrite( line, strlen( line ), sizeof( char ), fp );
		}

		int ethPair = 1;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			int used_eth_num = 0;
			int trust = 1;

			if (ethPair > NORMAL_CAR_TUNNEL_NUM)
				break;

			for (used_eth_num = 0; used_eth_num < ruijie_x_serial_ext_card_entry[sorted_num].ethNum;)
			{
				//printf("%s %d used_eth_num %d ethNum %d card_slot %s trust %d ethPair %d\n",
				//	__FUNCTION__, __LINE__, used_eth_num, ruijie_x_serial_ext_card_entry[sorted_num].ethNum, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, trust, ethPair);
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
					if (ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].used)
						continue;

					//printf("%s %d eth_index %d ethPair %d-%d trust %d-%d\n",
					//	__FUNCTION__, __LINE__, eth_index, ethPair, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair, trust, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute);

					if (ethPair != ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair
						|| !((trust && !ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute) || (!trust && ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute)))
						continue;

					sprintf(line, "%s %s %d %s no-ignore\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,
						(ruijie_rename_attribute_name[ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute]));
					fwrite(line, strlen( line ), sizeof( char ), fp );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].used=1;
					//printf("%s %d eth_index %d ethPair %d trust %d %s->%s\n",
					//	__FUNCTION__, __LINE__, eth_index, ethPair, trust, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName);
					used_eth_num++;
				}
				if (!trust)
				{
					ethPair++;
					if (ethPair > NORMAL_CAR_TUNNEL_NUM)
						break;
				}
				trust = !trust;
			}
		}
		fclose(fp);
		fp = NULL;
	}

error_out:
	if (fp)
	{
		pclose(fp);
	}
}


// NK06
static const char* chuangshi_c3000_nk06_ext_card_feature_string[] = 
{
	"       +-09.0-",
	"       +-0b.0-",
	"       +-0f.0-",
	"       +-10.0-",
	"       +-11.0-",
	"       +-16.0-",
	"       +-17.0-",
};

const static char chuangshi_c3000_nk06_ext_card_id_char[][3] = 
{
	"7",
	"6",
	"3",
	"2",
	"1",
	"4",
	"5",
};

const static char chuangshi_c3000_nk06_ext_card_mode_char[][3] = 
{
	"0",
	"0",
	"1",
	"0",
	"0",
	"0",
	"0",
};

static void chuangshi_auto_rename_c3000_nk06_serial_ext_card(const char* card_feature[], const char card_id[][3], const char card_mode[][3], unsigned int card_max, unsigned int eth_total)
{
	FILE*  fp                          = NULL;
	char   line[256]                   = { 0 };
	char   tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_char1;
	char* p_left;
	char* p_right;
	char* p_pciId = NULL;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	int iSlotFlag = 0;
	int iCardModel[2] = {0};
	int iCardType[2] = {0};
	char* pSlotName = NULL;
	int iExtCardBypassLen = 0;
	char szSlot1BypassName[32] = {0};
	char szSlot2BypassName[32] = {0};
	char szExtCardBypass[1024] = {0};
	int iSaveEthIndex = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	chuangshi_auto_rename_c3000_nk06_solt_info( &iSlotFlag, &iCardModel[0], &iCardModel[1], &iCardType[0], &iCardType[1] );

	fp = popen("lspci -tv", "r");
	if ( fp )
	{
		memset(line, 0x00, sizeof(line));
		while (0 != fgets(line, sizeof(line), fp ) )
		{
			device_match = 0;
			//feature_match = 0;
			line_size = strlen(line);
			/**remove '\n'**/
			line[line_size - 1] = 0;
			--line_size;
			//printf( "[%s:%d] ==> line:%s\n", __FUNCTION__, __LINE__, line );

			// new card
			if ( ( strstr( line, "     +-" ) || strstr( line, "     \\-" ) ) && !strstr( line, " |" ) )
			{
				if ( device_prefix[0] )
				{
					memset( device_prefix, 0x00, sizeof( device_prefix ) );
				}

				feature_match = 0;
			}

			if ( !feature_match )
			{
				for (card_index = 0; card_index < card_max; ++card_index)
				{
					if ((card_feature[card_index][0] == '|' || !strstr(line, "|")) && strstr(line, card_feature[card_index]))
					{
						if (ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV)
						{
							goto error_out;
						}

						char chMod = '0';
						pSlotName = NULL;

						// ext card slot
						if ( 0 == card_index || 1 == card_index )
						{
							// no card
							if ( iSlotFlag == NO_SOLT_ONLINE )
							{
								break;
							}
							// slot 1, no card
							else if ( ( 1 == card_index ) && ( SOLT_1_ONLINE != ( iSlotFlag & SOLT_1_ONLINE ) ) )
							{
								break;
							}
							// slot 2, no card
							else if ( ( 0 == card_index ) && ( SOLT_2_ONLINE != ( iSlotFlag & SOLT_2_ONLINE ) ) )
							{
								break;
							}
							// HPC06, TYPE_8_1G_FIBER
							else
							{
								// slot 1
								int iCardCur = 0;

								// slot 2
								if ( 0 == card_index )
								{
									iCardCur = 1;
								}

								// HPC01
								if ( HPC01 == iCardModel[iCardCur] )
								{
									// TYPE_4_1G_COPPER_4_1G_FIBER
									if ( TYPE_4_1G_COPPER_4_1G_FIBER == iCardType[iCardCur] )
									{
										chMod = 1 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC03
								else if ( HPC03 == iCardModel[iCardCur] )
								{
									// TYPE_4_10G_FIBER
									if ( TYPE_4_10G_FIBER == iCardType[iCardCur] )
									{
										chMod = 3 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC06
								else if ( HPC06 == iCardModel[iCardCur] )
								{
									// TYPE_8_1G_FIBER
									if ( TYPE_8_1G_FIBER == iCardType[iCardCur] )
									{
										chMod = 6 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC07
								else if ( HPC07 == iCardModel[iCardCur] )
								{
									// TYPE_8_1G_COPPER_BP
									if ( ( TYPE_8_1G_COPPER_BP == iCardType[iCardCur] ) || ( TYPE_8_1G_COPPER == iCardType[iCardCur] ) )
									{
										chMod = 7 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC08
								else if ( HPC08 == iCardModel[iCardCur] )
								{
									// TYPE_2_10G_FIBER
									if ( TYPE_2_10G_FIBER == iCardType[iCardCur] )
									{
										chMod = 8 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC09
								else if ( HPC09 == iCardModel[iCardCur] )
								{
									// TYPE_4_1G_COPPER_4_1G_FIBER_COMBO
									if ( TYPE_4_1G_COPPER_4_1G_FIBER_COMBO == iCardType[iCardCur] )
									{
										chMod = 9 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC11
								else if ( HPC11 == iCardModel[iCardCur] )
								{
									// TYPE_4_1G_COPPER_BP or TYPE_4_1G_FIBER
									if ( ( TYPE_4_1G_COPPER_BP == iCardType[iCardCur] ) || ( TYPE_4_1G_FIBER == iCardType[iCardCur] ) )
									{
										chMod = 11 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC12
								else if ( HPC12 == iCardModel[iCardCur] )
								{
									// TYPE_2_40G_FIBER
									if ( TYPE_2_40G_FIBER == iCardType[iCardCur] )
									{
										chMod = 12 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC13
								else if ( HPC13 == iCardModel[iCardCur] )
								{
									// TYPE_4_10G_FIBER_BP
									if ( TYPE_4_10G_FIBER_BP == iCardType[iCardCur] )
									{
										chMod = 13 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// HPC22
								else if ( HPC22 == iCardModel[iCardCur]
								{
									// TYPE_4_10G_FIBER
									if ( TYPE_4_10G_FIBER == iCardType[iCardCur] )
									{
										chMod = 22 + 'A';
									}
									// unknown
									else
									{
										break;
									}
								}
								// unknown card
								else
								{
									break;
								}

								pSlotName = ( 0 == iCardCur ) ? "slot1" : "slot2";
							}
						}

						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = ( '0' != chMod ) ? chMod : card_mode[card_index][0];
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot[0] = 0;

						if ( NULL != pSlotName )
							snprintf( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot ) - 1, "%s", pSlotName );

						//printf( "[%s:%d] ==> card_number:%d, feature:%s, CardId:%s, mode:%c\n", __FUNCTION__, __LINE__, ruijie_x_serial_ext_card_number + 1, card_feature[card_index], card_id[card_index], ( '0' != chMod ) ? chMod : card_mode[card_index][0] );
						++ruijie_x_serial_ext_card_number;
						feature_match = 1;
						eth_index= 0;
						break;
					}
				}
			}

			if ( !feature_match )
			{
				continue;
			}

			if ( !device_prefix[0] )
			{
				if ( line[line_size - 1] != '-' && strstr( line, "      " ) )
				{
					p_left = strrchr(line, '[');
					p_right = strrchr(line, ']');

					if ( p_left && p_right && p_right - p_left > 2 )
					{
						memset(tmpstr, 0x00, sizeof(tmpstr));

						memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
						p_char = strchr(tmpstr, '-');
						if (p_char)
						{
							*p_char = 0;
						}
						strncpy(device_prefix, tmpstr, sizeof(device_prefix));
						device_match = 1;
					}
				}
			}
			else
			{
				if ( line[line_size - 1] != '-' && strstr( line, "      " ) )
				{
					p_left = strrchr(line, '[');
					p_right = strrchr(line, ']');

					if ( p_left && p_right && p_right - p_left > 2 )
					{
						memset(tmpstr, 0x00, sizeof(tmpstr));
						memset( device_prefix, 0x00, sizeof( device_prefix ) );

						memcpy(tmpstr, p_left + 1, p_right - p_left - 1);
						p_char = strchr(tmpstr, '-');
						if (p_char)
						{
							*p_char = 0;
						}
						strncpy(device_prefix, tmpstr, sizeof(device_prefix));
						device_match = 1;
					}
				}
			}

			if ( device_prefix[0] )
			{
				if (strcasestr(line, "Intel Corporation Device ")
					|| strcasestr(line, "Network Connection")
					|| strcasestr(line, "Ethernet Controller")
					|| strcasestr( line, "Device 8088:0101" )
					|| strcasestr( line, "Device 8088:0103" )
					|| strcasestr( line, "Device 8088:1001" )
					|| strcasestr( line, "Device 8086:15f3" )
					|| strcasestr( line, "Device 8086:15ce" )
					|| strcasestr( line, "Device 8086:15c4" )
					|| strcasestr( line, "Device 8086:1521" )
					|| strcasestr( line, "Device 8086:1581" )
					|| strcasestr( line, "Device 8086:1583" )
					|| strcasestr( line, "Device 8086:1522" )
					)
				{
					p_pciId = NULL;

					p_char = strcasestr(line, "Intel Corporation");
					if (p_char)
					{
						*p_char = 0;
					}
					else
					{
						p_char = strcasestr(line, "Device 8088:");
						if (p_char)
						{
							*p_char = 0;
						}
						else
						{
							p_char = strcasestr(line, "Device 8086:");
							if (p_char)
							{
								*p_char = 0;
							}
						}
					}

					p_char = strrchr(line, '-');
					if ( p_char )
					{
						if ( eth_index > MAX_ETH_ONE_CARD )
						{
							goto error_out;
						}

						if (ruijie_x_serial_ext_card_number)
						{
							/**0000:b2:00.0**/
							snprintf(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 32, "%s:%4.4s", device_prefix, p_char + 1);

							//printf( "\t[%s:%d] ==> card_number:%d, eth_index:%d, ethPci:%s, pciId:%s\n", __FUNCTION__, __LINE__, 
							//	ruijie_x_serial_ext_card_number - 1, eth_index + 1, ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciName, 
							//		ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethPciId );
						}

						eth_index++;
						ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethNum = eth_index;
					}
				}
			}
		}
		pclose(fp );
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			//printf("%s %s %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,ruijie_x_serial_ext_card_entry[eth_index].cardId,ruijie_x_serial_ext_card_entry[eth_index].ethNum);
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			//printf("%s %s %d index %d sort_num %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,sorted_num, ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			eth_num++;
		}

		eth_num = 0;
		nm_index = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];
			//printf("%s %s %d sort %d Ext%c Num%d\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,ruijie_x_serial_ext_card_entry[sorted_num].ethNum);
			int iEthNum = ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
			{
				if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
				{
					const static unsigned int map[4] = {1, 2, 3, 0};

					for ( eth_index = 0; eth_index < 4; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 4;
				}
				// HPC03 TYPE_4_10G_FIBER
				else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 3 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
				{
					const static unsigned int map[4] = {3, 2, 1, 0};

					for ( eth_index = 0; eth_index < 4; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 4;
				}
				// HPC06 TYPE_8_1G_FIBER or HPC01 TYPE_4_1G_COPPER_4_1G_FIBER or HPC07 TYPE_8_1G_COPPER_BP
				else if ( ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 1 ) ) 
						|| ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 6 ) )
						|| ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 7 ) ) 
						) 
					&& ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 )
				{
					const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

					for ( eth_index = 0; eth_index < 8; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 8;
				}
				// HPC08 TYPE_2_10G_FIBER
				else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 8 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
				{
					const static unsigned int map[2] = {1, 0};

					for ( eth_index = 0; eth_index < 2; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 2;
				}
				// HPC09 TYPE_4_1G_COPPER_4_1G_FIBER_COMBO
				else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 9 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
				{
					const static unsigned int map[4] = {1, 0, 3, 2};

					for ( eth_index = 0; eth_index < 4; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 4;
				}
				// HPC11 TYPE_4_1G_COPPER_BP or TYPE_4_1G_FIBER
				else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 11 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
				{
					const static unsigned int map[4] = {0, 1, 3, 2};

					for ( eth_index = 0; eth_index < 4; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 4;
				}
				// HPC13 TYPE_4_10G_FIBER_BP
				else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 13 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
				{
					const static unsigned int map[4] = {2, 3, 0, 1};

					for ( eth_index = 0; eth_index < 4; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 4;
				}
				// HPC22 TYPE_4_10G_FIBER
				else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 22 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
				{
					const static unsigned int map[4] = {0, 1, 2, 3};

					for ( eth_index = 0; eth_index < 4; ++eth_index )
					{
						int ethAttribute = ( ( eth_num + map[eth_index] )  & 1 );

						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
						ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );
					}

					eth_num += 4;
				}
				else
				{
					int ethAttribute = ( eth_num & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( eth_num >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);

					eth_num ++;
				}

				//printf("%s %s %d sort %d Ext%c No%d Pair%d NewName %s ethPciName %s\r\n", __FUNCTION__,__FILE__,__LINE__,sorted_num,ruijie_x_serial_ext_card_entry[sorted_num].cardId,eth_index,
				//ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPciName);
			}
		}

		struct dirent *pEntry = NULL;
		DIR *pDir = opendir( "/sys/class/net" );

		if ( pDir )
		{
			while ( ( pEntry = readdir( pDir ) ) != NULL )
			{
				if ( strncmp( pEntry->d_name, "eth", 3 ) && strncmp( pEntry->d_name, "rename", 6 ) )
				{
					continue;
				}

				snprintf(tmpstr, sizeof(tmpstr), "readlink /sys/class/net/%s/device", pEntry->d_name);

				fp = popen(tmpstr, "r");
				if ( fp )
				{
					memset(line, 0x00, sizeof(line));
					if (0 != fgets(line, sizeof(line), fp ))
					{
						line_size = strlen(line);
						/**remove '\n'**/
						line[line_size - 1] = 0;
						--line_size;

						p_char = strrchr(line, '/');
						if (p_char)
						{
							if ( strlen( p_char ) > 6 )
							{
								p_char += 6;
								//printf("%s %s %d eth%d device %s p_char %s\r\n", __FUNCTION__,__FILE__,__LINE__,eth_num,line,p_char);

								for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
								{
									for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[card_index].ethNum; ++eth_index)
									{
										//printf("%s %s %d %s p_char %s(size=%d) card_index %d eth_index %d ethPciName %s(size=%d)\r\n", __FUNCTION__,__FILE__,__LINE__,pEntry->d_name,p_char,strlen(p_char),card_index,eth_index,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName,
										//	strlen(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName));

										if (0 == strcmp(p_char, ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPciName))
										{
											snprintf(ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName, 8, "%s", pEntry->d_name);
											//printf("%s %s %d card_index %d Ext%c No%d Pair%d NewName %s OldName %s\r\n", __FUNCTION__,__FILE__,__LINE__,card_index,ruijie_x_serial_ext_card_entry[card_index].cardId,eth_index,
											//	ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethPair,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethNewName,ruijie_x_serial_ext_card_entry[card_index].ethCard[eth_index].ethOldName);
											break;
										}
									}
								}
							}
						}
					}
					pclose(fp);
					fp = NULL;
				}
			}

			closedir( pDir );
		}

		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				int eth_count = 0;
				const static unsigned int map[4] = {1, 2, 3, 0};

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			// HPC03 TYPE_4_10G_FIBER
			else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 3 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				int eth_count = 0;
				const static unsigned int map[4] = {3, 2, 1, 0};

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			// HPC06 TYPE_8_1G_FIBER or HPC01 TYPE_4_1G_COPPER_4_1G_FIBER or HPC07 TYPE_8_1G_COPPER_BP
			else if ( ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 1 ) ) 
						|| ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 6 ) )
						|| ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 7 ) ) 
						) 
					&& ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 )
			{
				int iEthIndex = 0;
				int eth_count = 0;
				int iExtBypass = 0;
				int iEthBypassCount = 0;
				char szLineContent[64] = {0};
				const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

				// HPC07
				if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 7 ) )
				{
					if ( strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) )
					{
						if ( '1' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
						{
							if ( iCardType[0] == TYPE_8_1G_COPPER_BP )
							{
								iExtBypass = 1;
					
								snprintf( szSlot1BypassName, sizeof( szSlot1BypassName ) - 1, "#slot1,HPC07,%d,2\n", TYPE_8_1G_COPPER_BP );
							}
						}
						else if ( '2' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
						{
							if ( iCardType[1] == TYPE_8_1G_COPPER_BP )
							{
								iExtBypass = 1;
					
								snprintf( szSlot2BypassName, sizeof( szSlot2BypassName ) - 1, "#slot2,HPC07,%d,2\n", TYPE_8_1G_COPPER_BP );
							}
						}
					}

					if ( iExtBypass )
					{
						iEthBypassCount = 4;
					}
				}

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							if ( iExtBypass && ( iEthIndex < iEthBypassCount ) )
							{
								if ( 0 == iEthIndex % 2 )
									snprintf( szLineContent, sizeof( szLineContent ) - 1, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
								else
									snprintf( szLineContent + strlen( szLineContent ), sizeof( szLineContent ) - 1 - strlen( szLineContent ), "<=>%s\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

								if ( iExtCardBypassLen + strlen( szLineContent ) < sizeof( szExtCardBypass ) && strstr( szLineContent, "<=>" ) )
								{
									iExtCardBypassLen += snprintf( szExtCardBypass + iExtCardBypassLen, sizeof( szExtCardBypass ) - 1 - iExtCardBypassLen, "%s", szLineContent );
								}
							}
							iEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			// HPC08 TYPE_2_10G_FIBER
			else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 8 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				int eth_count = 0;
				const static unsigned int map[2] = {1, 0};

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			// HPC09 TYPE_4_1G_COPPER_4_1G_FIBER_COMBO
			else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 9 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				int iEthIndex = 0;
				int eth_count = 0;
				int iExtBypass = 0;
				int iEthBypassCount = 0;
				char szLineContent[64] = {0};
				const static unsigned int map[4] = {1, 0, 3, 2};

				if ( strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) )
				{
					if ( '1' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
					{
						if ( iCardType[0] == TYPE_4_1G_COPPER_4_1G_FIBER_COMBO )
						{
							iExtBypass = 1;

							snprintf( szSlot1BypassName, sizeof( szSlot1BypassName ) - 1, "#slot1,HPC09,%d,2\n", TYPE_4_1G_COPPER_4_1G_FIBER_COMBO );
						}
					}
					else if ( '2' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
					{
						if ( iCardType[1] == TYPE_4_1G_COPPER_4_1G_FIBER_COMBO )
						{
							iExtBypass = 1;

							snprintf( szSlot2BypassName, sizeof( szSlot2BypassName ) - 1, "#slot2,HPC09,%d,2\n", TYPE_4_1G_COPPER_4_1G_FIBER_COMBO );
						}
					}
				}

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							if ( iExtBypass )
							{
								if ( 0 == iEthIndex % 2 )
									snprintf( szLineContent, sizeof( szLineContent ) - 1, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
								else
									snprintf( szLineContent + strlen( szLineContent ), sizeof( szLineContent ) - 1 - strlen( szLineContent ), "<=>%s\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

								if ( iExtCardBypassLen + strlen( szLineContent ) < sizeof( szExtCardBypass ) && strstr( szLineContent, "<=>" ) )
								{
									iExtCardBypassLen += snprintf( szExtCardBypass + iExtCardBypassLen, sizeof( szExtCardBypass ) - 1 - iExtCardBypassLen, "%s", szLineContent );
								}
							}
							iEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			// HPC11 TYPE_4_1G_COPPER_BP or TYPE_4_1G_FIBER
			else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 11 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				int iEthIndex = 0;
				int eth_count = 0;
				int iExtBypass = 0;
				char szLineContent[64] = {0};
				const static unsigned int map[4] = {0, 1, 3, 2};

				if ( strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) )
				{
					if ( '1' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
					{
						if ( iCardType[0] == TYPE_4_1G_COPPER_BP )
						{
							iExtBypass = 1;

							snprintf( szSlot1BypassName, sizeof( szSlot1BypassName ) - 1, "#slot1,HPC11,%d,2\n", TYPE_4_1G_COPPER_BP );
						}
					}
					else if ( '2' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
					{
						if ( iCardType[1] == TYPE_4_1G_COPPER_BP )
						{
							iExtBypass = 1;

							snprintf( szSlot2BypassName, sizeof( szSlot2BypassName ) - 1, "#slot2,HPC11,%d,2\n", TYPE_4_1G_COPPER_BP );
						}
					}
				}

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							if ( iExtBypass )
							{
								if ( 0 == iEthIndex % 2 )
									snprintf( szLineContent, sizeof( szLineContent ) - 1, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
								else
									snprintf( szLineContent + strlen( szLineContent ), sizeof( szLineContent ) - 1 - strlen( szLineContent ), "<=>%s\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

								if ( iExtCardBypassLen + strlen( szLineContent ) < sizeof( szExtCardBypass ) && strstr( szLineContent, "<=>" ) )
								{
									iExtCardBypassLen += snprintf( szExtCardBypass + iExtCardBypassLen, sizeof( szExtCardBypass ) - 1 - iExtCardBypassLen, "%s", szLineContent );
								}
							}
							iEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			// HPC13 TYPE_4_10G_FIBER_BP
			else if ( ( ruijie_x_serial_ext_card_entry[sorted_num].mode == ( 'A' + 13 ) ) && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				int iEthIndex = 0;
				int eth_count = 0;
				int iExtBypass = 0;
				char szLineContent[64] = {0};

				const static unsigned int map[4] = {2, 3, 0, 1};

				if ( strlen( ruijie_x_serial_ext_card_entry[sorted_num].card_slot ) )
				{
					if ( '1' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
					{
						if ( iCardType[0] == TYPE_4_10G_FIBER_BP )
						{
							iExtBypass = 1;

							snprintf( szSlot1BypassName, sizeof( szSlot1BypassName ) - 1, "#slot1,HPC13,%d,2\n", TYPE_4_10G_FIBER_BP );
						}
					}
					else if ( '2' == ruijie_x_serial_ext_card_entry[sorted_num].card_slot[4] )
					{
						if ( iCardType[1] == TYPE_4_10G_FIBER_BP )
						{
							iExtBypass = 1;

							snprintf( szSlot2BypassName, sizeof( szSlot2BypassName ) - 1, "#slot2,HPC13,%d,2\n", TYPE_4_10G_FIBER_BP );
						}
					}
				}

				while ( eth_count < ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
					{
						if ( eth_count == map[eth_index] )
						{
							sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
							sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

							get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

							iSaveEthIndex++;

							if ( iExtBypass )
							{
								if ( 0 == iEthIndex % 2 )
									snprintf( szLineContent, sizeof( szLineContent ) - 1, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
								else
									snprintf( szLineContent + strlen( szLineContent ), sizeof( szLineContent ) - 1 - strlen( szLineContent ), "<=>%s\n", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

								if ( iExtCardBypassLen + strlen( szLineContent ) < sizeof( szExtCardBypass ) && strstr( szLineContent, "<=>" ) )
								{
									iExtCardBypassLen += snprintf( szExtCardBypass + iExtCardBypassLen, sizeof( szExtCardBypass ) - 1 - iExtCardBypassLen, "%s", szLineContent );
								}
							}
							iEthIndex++;

							break;
						}
					}

					eth_count++;
				}
			}
			else
			{
				for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index)
				{
					sprintf( g_port_info[iSaveEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
					sprintf( g_port_info[iSaveEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

					get_eth_slot_dpdk_value( 1, g_port_info[iSaveEthIndex].oldPhyname, g_port_info[iSaveEthIndex].ethPciId, sizeof( g_port_info[iSaveEthIndex].ethPciId ) );

					iSaveEthIndex++;
				}
			}

			g_portNum += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}

		if ( strlen( szExtCardBypass ) > 0 )
		{
			FILE* fp = fopen( "/home/ace_bypass_map_nk06_ext.dat", "w" );

			if ( fp )
			{
				if ( szSlot1BypassName[0] )
				{
					fwrite( szSlot1BypassName, 1, strlen( szSlot1BypassName ), fp );
				}

				if ( szSlot2BypassName[0] )
				{
					fwrite( szSlot2BypassName, 1, strlen( szSlot2BypassName ), fp );
				}

				fwrite( szExtCardBypass, 1, strlen( szExtCardBypass ), fp );
			
				fclose( fp );
			}
		}
	}
error_out:
	if (fp)
	{
		pclose(fp);
	}
}

static const char* rename_sorted_chuangshi_nk03_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:15.0/",
	" ../../devices/pci0000:00/0000:00:16.0/",
	" ../../devices/pci0000:00/0000:00:17.0/",
	" ../../devices/pci0000:00/0000:00:0e.0/",
	" ../../devices/pci0000:00/0000:00:09.0/"
};

const static char rename_sorted_chuangshi_nk03_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5"
};

static const char* rename_sorted_chuangshi_nk03_name_string[] = 
{
	"onboard1",
	"onboard2",
	"onboard3",
	"slot1",
	"slot2"
};

static void changshi_nk03_ext_card_by_sys_class_net( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char  device_prefix[32] = {0};
	int   eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	int iSlotFlag = 0;
	int iCardModel[2] = {0};
	int iCardType[2] = {0};
	int iSlot1Eths = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	chuangshi_auto_rename_c3000_nk06_solt_info( &iSlotFlag, &iCardModel[0], &iCardModel[1], &iCardType[0], &iCardType[1] );

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth|switch' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		while (0 != fgets(line, sizeof(line), fp))
		{
retry:
			eth_index = 0;

			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
						{
							goto retry;
						}

						char *p = strrchr(line, '/');

						if ( p )
						{
#if 0
							// switch port
							if ( '2' == ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].cardId )
							{
								if ( 0 == strncmp( p + 1, "switch", strlen( "switch" ) ) )
								{
									strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1), sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);

									ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
								}
							}
							else
							{
								strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1), sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);

								ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
							}
#else
							strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1), sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
#endif

						}
					} while( 0 != fgets(line, sizeof(line), fp) );
				}
			}
		}

		pclose( fp );

		fp = NULL;
	}

	int iTotalMgtEth = 0;
	int iTotalDpdkEth = 0;

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;

		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}

			sorted_num = eth_index;

			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}

				if (sorted_num == card_index)
				{
					continue;
				}

				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2", strlen( "onboard2" ) ) )
				{
					ruijie_x_serial_ext_card_entry[sorted_num].mode = 'a';
				}
				else if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );

					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// HPC08
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );

						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1583" );

						// HPC12
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
						}
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=ngbe" );

					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:0103" );

					// HPC11
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );

						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1581" );

						// HPC03
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '4';

							if ( HPC13 == iCardModel[0] )
							{
								ruijie_x_serial_ext_card_entry[sorted_num].mode = '5';
							}
						}
					}
				}
				else if ( 8 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );

					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1521" );

					// HPC01 or HP07
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );

						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1522" );

						// HPC06
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
						}
					}
				}
			}

			if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				const static unsigned int map[2] = {1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + map[eth_index];

					int ethAttribute = ( ( iEthIndex + 17 ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex + 17 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, hhh, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '2' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {0, 1, 3, 2};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + map[eth_index];

					int ethAttribute = ( ( iEthIndex + 17 ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex + 17 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );


					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, ggg, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '3' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 )
			{
				const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + map[eth_index];

					int ethAttribute = ( ( iEthIndex + 17 ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex + 17 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, fff, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '4' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {3, 2, 1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + map[eth_index];

					int ethAttribute = ( ( iEthIndex + 17 ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex + 17 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, eee, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '5' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {2, 3, 0, 1};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + map[eth_index];

					int ethAttribute = ( ( iEthIndex + 17 ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex + 17 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, ddd, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			// board2 ( switch )
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == 'a' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + eth_index;
#if 0
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
#else
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "switch%d", eth_index );

					if ( strcmp( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName ) )
					{
						char szCmd[256] = {0};

						snprintf( szCmd, sizeof( szCmd ), "ip link set %s name %s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						system( szCmd );

						snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName, 8, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
					}
#endif

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, aaa, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );						
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
			// MGT
			else if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( "onboard1" ) ) )
			{
				snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "%s", "MGT" );

				sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

				sprintf( g_port_info[0].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );

				init_coordinate_log( "Info, the dpdk_eth:%d, bbb, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
					iTotalMgtEth, 0, g_port_info[0].oldPhyname, g_port_info[0].newPhyname );

				iTotalMgtEth++;
			}
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = iTotalDpdkEth + iTotalMgtEth + eth_index;

					int ethAttribute = ( ( iEthIndex + 17 ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex + 17 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						init_coordinate_log( "Info, the dpdk_eth:%d, ccc, the mgt num:%d, eth_index:%d, old:%s, new:%s.", iTotalDpdkEth, 
							iTotalMgtEth, iEthIndex, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].newPhyname );
					}
				}

				iTotalDpdkEth += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			}
		}
	}

	g_portNum = iTotalDpdkEth;

	g_iMgtNum = iTotalMgtEth;

error_out:
	if ( fp )
	{
		pclose( fp ); fp = NULL;
	}

	return;
}

static void chuangshi_c3000_auto_rename_if(int eth_num)
{
/*
METHOD = 3
TYPE = CHUANGSHI_C3000
*/
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     i                           = 0;
	int     board_gh_7135 = 0;
	int     board_gh_7131 = 0;
	int     portIndex;
	int     nmPortNum = 0;
	int     man_make = 0;
	int     iDpdkEth = 0;
	char    szDevType[256] = {0};

	remove( "/home/ace_bypass_map_nk03_ext.dat" );

	remove( "/home/ace_bypass_map_nk06_ext.dat" );

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	get_dev_type_interface( szDevType, sizeof( szDevType ) );

	if ( strcmp( szDevType, "CS-NK02" ) && strcmp( szDevType, "CS-NK03" ) && strcmp( szDevType, "CS-NK05" ) 
		&& strcmp( szDevType, "CS-NK06" ) && strcmp( szDevType, "CS-NK08" )
		)
	{
		return;
	}

	fp = fopen( ACE_PORT_MAP_FILE, "r" );

	if ( fp )
	{
		fgets( line, sizeof( line ), fp );

		fgets( line, sizeof( line ), fp );

		fclose( fp ); fp = NULL;

		if ( strstr( line, "####ManMakeExtMap") )
		{
			if ( 2 != sscanf( line, "%s %d", tmpstr, &man_make ) )
			{
				man_make = 0;
			}
		}

		if ( 1 == man_make )
		{
			return;
		}
	}
	else
	{
		unlink( ACE_VPP_PORT_MAP_FILE );

		unlink( "/home/config/current/inband_mgt_config" );

		init_coordinate_log( "Info, the eth name rule may be change." );
	}

	if ( !strcmp( szDevType, "CS-NK03" ) )
	{
		system( "/usr/sbin/i2cset -f -y 0 0x40 0x74 0" );

		changshi_nk03_ext_card_by_sys_class_net( rename_sorted_chuangshi_nk03_feature_string, rename_sorted_chuangshi_nk03_card_id_char, 
			rename_sorted_chuangshi_nk03_name_string, sizeof( rename_sorted_chuangshi_nk03_card_id_char ) / sizeof( rename_sorted_chuangshi_nk03_card_id_char[0] ) );

		iDpdkEth = g_portNum;

		init_coordinate_log( "Info, the dpdk_eth:%d, the mgt num:%d.", iDpdkEth, g_iMgtNum );

		goto will_done;
	}
	else if ( !strcmp( szDevType, "CS-NK06" ) )
	{
		chuangshi_auto_rename_c3000_nk06_serial_ext_card( chuangshi_c3000_nk06_ext_card_feature_string, chuangshi_c3000_nk06_ext_card_id_char, chuangshi_c3000_nk06_ext_card_mode_char, sizeof(chuangshi_c3000_nk06_ext_card_id_char) / sizeof(chuangshi_c3000_nk06_ext_card_id_char[0]), eth_num );	

		iDpdkEth = g_portNum;

		g_iMgtNum = 0;

		init_coordinate_log( "Info, the dpdk_eth:%d, the mgt num:%d.", iDpdkEth, g_iMgtNum );

		goto will_done;
	}

	remove( ACE_PORT_MAP_FILE );

	return;

will_done:
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( i = 0; i < iDpdkEth && !iEnd; ++i )
				{
					if ( 'M' != g_port_info[i].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[i].newPhyname[0] ) || ( 'W' == g_port_info[i].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[i].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 1;
						}
						else if ( 'M' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 2;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < ( g_portNum + g_iMgtNum ); portIndex++ )
		{
			int iPair = 1;
			int ethAttribute = 0;

			if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, iPair,
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif

#ifdef CS_E2000Q_PLATFORM
static const char* rename_sorted_chuangshi_nt06_ports[] = 
{
	"eth4",
	"eth5",
	"eth0",
	"eth1",
	"eth3",
	"eth2"
};

static const char* rename_sorted_chuangshi_nt06_ports_new[] = 
{
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3"
};

const static char rename_sorted_nt06_card_mode_char[][3] = 
{
	"0",
	"0",
	"0",
	"0",
	"0",
	"0"
};

static const char* rename_sorted_nt06_feature_string[] = 
{
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:04.0/0000:03:00.0/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:05.0/0000:04:00.0/",
	" ../../devices/platform/soc/3200c000.ethernet/",
	" ../../devices/platform/soc/3200e000.ethernet/",
	" ../../devices/platform/soc/32012000.ethernet/",
	" ../../devices/platform/soc/32010000.ethernet/"
};

const static char rename_sorted_nt06_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6"
};

static void chuangshi_nt06_auto_rename_if( int eth_num )
{
/*
METHOD = 3
TYPE = CHUANGSHI-NT06
*/
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     i                           = 0;
	int     board_gh_7135 = 0;
	int     board_gh_7131 = 0;
	int     portIndex;
	int     nmPortNum = 0;
	int     man_make = 0;
	int     iDpdkEth = 0;

	if (access("/usr/mysys/skip_auto_interface", 0) == 0)
		return;

	fp = fopen( bypass_switch_config, "r" );
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

		fclose(fp);
	}

	if ( !strstr( tmpstr, "CHUANGSHI-NT06" ) )
	{
		return;
	}

	FILE* fpMap = fopen( ACE_PORT_MAP_FILE, "r" );

	if ( fpMap )
	{
		fgets( line, sizeof( line ), fp );

		fgets( line, sizeof( line ), fp );

		fclose( fpMap );

		fpMap = NULL;

		if ( strstr( line, "####ManMakeExtMap" ) && ( sscanf( line, "%s %d", tmpstr, &man_make ) == 2 ) && man_make )
		{
			return;
		}
	}

	if ( 6 == eth_num )
	{
		for ( i = 0; i < 6; i++ )
		{
			sprintf( g_port_info[i].oldPhyname, "%s", rename_sorted_chuangshi_nt06_ports[i] );

			sprintf( g_port_info[i].newPhyname, "%s", rename_sorted_chuangshi_nt06_ports_new[i] );
		}

		iDpdkEth = 6;

		goto will_done;
	}
	else if ( eth_num > 6 )
	{
	}

	remove( ACE_PORT_MAP_FILE );

	return;

will_done:
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

    if ( fp )
    {
        sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
        fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		int ethPair = 1;

        for ( portIndex = 0; portIndex < eth_num; portIndex++ )
        {
        	int ethAttribute = 0;

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

        	sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, 
                    g_port_info[portIndex].newPhyname, ethPair, ( ruijie_rename_attribute_name[ethAttribute] ) );

            fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

			if ( ( g_port_info[portIndex].newPhyname[0] != 'L' ) && ( g_port_info[portIndex].newPhyname[0] != 'M' ) && ( g_port_info[portIndex].newPhyname[0] != 'H' ) )
			{
				ethPair++;
			}
        }

		fclose( fp ); fp = NULL;
    }
}

static const char* rename_sorted_chuangshi_nt09_ports[] = 
{
	"eth3",
	"eth2",
	"eth0",
	"eth1"
};

static const char* rename_sorted_chuangshi_nt09_ports_new[] = 
{
	"switch0",
	"switch1",
	"LAN7",
	"WAN7"
};

static const char* rename_sorted_nt09_feature_string[] = 
{
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:00.0/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:02.0/"
};

const static char rename_sorted_nt09_card_id_char[][3] = 
{
	"2",
	"1"
};

void changshi_nt09_ext_card_fiber_light_on( int iType, int iSlot, int iEthNum )
{
	int i = 0;
	char szCmd[256] = {0};
	char szLine[256] = {0};

	if ( ( 11 == iType ) || ( 12 == iType ) )
	{
		if ( iSlot > 0 )
		{
			int iGpioNum = 0;
			int iArrGpios[2] = {0};

			FILE* fpHpc = popen( "ls -l /sys/class/gpio/ | grep 28026000.i2c | awk -F'gpiochip' '{print $3}'", "r" );

			if ( fpHpc )
			{
				memset( szLine, 0x00, sizeof( szLine ) );

				while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpHpc ) )
				{
					int iStartGpio = atoi( szLine );

					if ( ( iStartGpio > 0 ) && ( iGpioNum < 2 ) )
					{
						iArrGpios[iGpioNum] = iStartGpio;

						iGpioNum++;
					}
				}

				pclose( fpHpc );
			}

			init_coordinate_log( "[%s:%d] ==> HPC, type:%d, gpio num:%d, gpio_1:%d, gpio_2:%d, slot:%d, eth_num:%d.", __FUNCTION__, __LINE__, iType, iGpioNum, iArrGpios[0], iArrGpios[1], iSlot, iEthNum );

			if ( iGpioNum > 0 && iGpioNum <= 2 )
			{
				if ( 1 == iGpioNum )
				{
					for ( i=0; i<iEthNum; i++ )
					{
						snprintf( szCmd, sizeof( szCmd ) - 1, "echo %d > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio%d/direction && echo 0 > /sys/class/gpio/gpio%d/value", iArrGpios[0] + i, iArrGpios[0] + i, iArrGpios[0] + i );
						system( szCmd );
						init_coordinate_log( "[%s:%d] ==> HPC, 111, cmd [%s].", __FUNCTION__, __LINE__, szCmd );
					}
				}
				else
				{
					// slot1
					if ( 0x01 == ( 0x01 & iSlot ) )
					{
						for ( i=0; i<iEthNum; i++ )
						{
							snprintf( szCmd, sizeof( szCmd ) - 1, "echo %d > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio%d/direction && echo 0 > /sys/class/gpio/gpio%d/value", iArrGpios[0] + i, iArrGpios[0] + i, iArrGpios[0] + i );
							system( szCmd );
							init_coordinate_log( "[%s:%d] ==> HPC, 222, cmd [%s].", __FUNCTION__, __LINE__, szCmd );
						}
					}

					// slot2
					if ( 0x02 == ( 0x02 & iSlot ) )
					{
						for ( i=0; i<iEthNum; i++ )
						{
							snprintf( szCmd, sizeof( szCmd ) - 1, "echo %d > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio%d/direction && echo 0 > /sys/class/gpio/gpio%d/value", iArrGpios[1] + i, iArrGpios[1] + i, iArrGpios[1] + i );
							system( szCmd );
							init_coordinate_log( "[%s:%d] ==> HPC, 333, cmd [%s].", __FUNCTION__, __LINE__, szCmd );
						}
					}
				}
			}
			else
			{
				init_coordinate_log( "[%s:%d] ==> Error, get hpc gpio error.", __FUNCTION__, __LINE__ );
			}
		}
	}

	return;
}

static void changshi_nt09_ext_card_by_sys_class_net( const char* card_feature[], const char card_id[][3], unsigned int card_max, unsigned int iStartIndex, unsigned int iStart )
{
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;
	int iHpc11Fiber = 0;
	int iHpc11Solt = 0;
	int iHpc11Eth = 0;
	int iHpc12Fiber = 0;
	int iHpc12Solt = 0;
	int iHpc12Eth = 0;
	int iSlotCount = 0;
	int iHpcType1 = 0, iHpcType2 = 0, iHpcExist = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	chuangshi_get_slot_info( 9, "28026000.i2c", &iHpcType1, &iHpcType2, &iSlotCount );

	init_coordinate_log( "Info, nt09, slot_count:%d, slot1:0x%X, slot2:0x%X.", iSlotCount, iHpcType1, iHpcType2 );

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );
		while (0 != fgets(line, sizeof(line), fp))
		{
retry:
			eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					++ruijie_x_serial_ext_card_number;

					do
					{
						if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
						{
							goto retry;
						}

						char *p = strrchr(line, '/');
						if (p)
						{
							strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1), sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while( 0 != fgets(line, sizeof(line), fp) );
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}
		eth_num = iStart;
		int iNewIndex = iStartIndex;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// HPC08
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1583" );

						// HPC12
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';

							iHpc12Fiber = 1;

							iHpc12Eth = 2;

							iHpc12Solt |= ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - '0' );
						}
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=ngbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:0103" );

					// HPC11
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';

						// check Fiber ?
						iHpc11Fiber = 1;

						iHpc11Eth = 4;

						iHpc11Solt |= ( ruijie_x_serial_ext_card_entry[sorted_num].cardId - '0' );
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1581" );

						// HPC03
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '4';

							int iHpcType = 0;

							// slot1
							if ( ( '5' == ruijie_x_serial_ext_card_entry[sorted_num].cardId ) && iHpcType1 )
							{
								iHpcType = iHpcType1 >> 8;
							}
							// slot2
							else if ( ( '6' == ruijie_x_serial_ext_card_entry[sorted_num].cardId ) && iHpcType2 )
							{
								iHpcType = iHpcType2 >> 8;
							}

							// HPC13
							if ( 0x0D == iHpcType )
							{
								ruijie_x_serial_ext_card_entry[sorted_num].mode = '5';
							}
						}
					}
				}
				else if ( 8 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1521" );

					// HPC01 or HP07
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1522" );

						// HPC06
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
						}
					}
				}
			}

			if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				const static unsigned int map[2] = {1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + map[eth_index] ) & 1 );

					int iEthIndex = iNewIndex + map[eth_index];

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '2' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {0, 1, 3, 2};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + map[eth_index] ) & 1 );

					int iEthIndex = iNewIndex + map[eth_index];

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '3' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 )
			{
				const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + map[eth_index] ) & 1 );

					int iEthIndex = iNewIndex + map[eth_index];

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '4' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {3, 2, 1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + map[eth_index] ) & 1 );

					int iEthIndex = iNewIndex + map[eth_index];

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '5' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {2, 3, 0, 1};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + map[eth_index] ) & 1 );

					int iEthIndex = iNewIndex + map[eth_index];

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + map[eth_index] ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int ethAttribute = ( ( eth_num + eth_index ) & 1 );

					int iEthIndex = iNewIndex + eth_index;

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( eth_num + eth_index ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute?'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair);

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
			iNewIndex += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}
	}

	if ( 1 == iHpc11Fiber )
	{
		changshi_nt09_ext_card_fiber_light_on( 11, iHpc11Solt, iHpc11Eth );
	}

	if ( 1 == iHpc12Fiber )
	{
		changshi_nt09_ext_card_fiber_light_on( 12, iHpc12Solt, iHpc12Eth );
	}

error_out:
	if (fp)
	{
		pclose(fp);
	}

	return;
}

static void chuangshi_nt09_auto_rename_if(int eth_num)
{
/*
METHOD = 3
TYPE = CHUANGSHI-NT06 or TYPE = CHUANGSHI-NT09
*/
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	char    szCmd[256]                  = { 0 };
	int     i                           = 0;
	int     board_gh_7135 = 0;
	int     board_gh_7131 = 0;
	int     portIndex;
	int     nmPortNum = 0;
	int     man_make = 0;
	int     iDpdkEth = 0;

	if (access("/usr/mysys/skip_auto_interface", 0) == 0)
		return;

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose( fp );
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			return;
		}
	}

	// nt09
	if ( eth_num >= 16 )
	{
		//system( "insmod /usr/private/xspeed_nt09_fpga.ko" );
		//system( "echo 0x0200 > /sys/class/fpga/cpld0/regs" ); // board bypass
		//system( "echo 0x0300 > /sys/class/fpga/cpld0/regs" ); // ext bypass
		system( "echo 1 > /sys/class/leds/led2/brightness" );
		system( "echo 0 > /sys/class/leds/run/brightness" );
		system( "ifconfig eth1 down" );

		system( "ifconfig eth2 mtu 10000" );
		system( "ifconfig eth3 mtu 10000" );

		for ( i = 0; i < 4; i++ )
		{
			sprintf( g_port_info[i].oldPhyname, "%s", rename_sorted_chuangshi_nt09_ports[i] );
			sprintf( g_port_info[i].newPhyname, "%s", rename_sorted_chuangshi_nt09_ports_new[i] );
		}

		// have ext card
		if ( eth_num > 16 )
		{
			changshi_nt09_ext_card_by_sys_class_net(rename_sorted_nt09_feature_string, rename_sorted_nt09_card_id_char, sizeof(rename_sorted_nt09_card_id_char)/sizeof(rename_sorted_nt09_card_id_char[0]), 4, 14 );
		}

		eth_num -= 12;

		iDpdkEth = eth_num;

		goto will_done;
	}

	remove( ACE_PORT_MAP_FILE );

	return;

will_done:
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( i = 0; i < iDpdkEth && !iEnd; ++i )
				{
					if ( 's' != g_port_info[i].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[i].newPhyname[0] ) || ( 'W' == g_port_info[i].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[i].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 1;
						}
						else if ( 'M' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 2;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < eth_num; portIndex++ )
		{
			int iPair = 1;
			int ethAttribute = 0;

			if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, iPair,
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif

#if 58
static int rename_sorted_chuangshi_nr01_ports[] = {8,6,5,4,7,2,1,0,3,10,9,12,11,14,13,16,15};

static const char* chuangshi_nr01_map[] = 
{
	"MGT",
	"LAN1",
	"WAN1",
	"LAN2",
	"WAN2",
	"LAN3",
	"WAN3",
	"LAN4",
	"WAN4",
	"LAN5",
	"WAN5",
	"LAN6",
	"WAN6",
	"LAN7",
	"WAN7",
	"LAN8",
	"WAN8"
};

static const char* rename_sorted_chuangshi_nr01_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:17.0/",
	" ../../devices/pci0000:00/0000:00:16.0/",
	" ../../devices/pci0000:00/0000:00:15.0/",
	" ../../devices/pci0000:f3/0000:f3:04.0/",
	" ../../devices/pci0000:d0/0000:d0:02.0/",
	" ../../devices/pci0000:d0/0000:d0:04.0/",
	" ../../devices/pci0000:00/0000:00:11.0/",
	" ../../devices/pci0000:00/0000:00:10.0/",
	" ../../devices/pci0000:00/0000:00:0b.0/",
	" ../../devices/pci0000:00/0000:00:09.0/"
};

const static char rename_sorted_chuangshi_nr01_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"A"
};

static const char* rename_sorted_chuangshi_nr01_name_string[] = 
{
	"onboard1",
	"onboard2",
	"onboard3",
	"onboard4",
	"slot1",
	"slot2",
	"slot3",
	"slot4",
	"slot5",
	"slot6"
};

static void changshi_nr01_ext_card_by_sys_class_net( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	int   i                           = 0;
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int uiSlots[6] = {0};
	unsigned int uiSlotEths[6] = {0};

	memset(device_prefix, 0x00, sizeof(device_prefix));

	get_chuangshi_slot_info( uiSlots, sizeof( uiSlots ) / sizeof( unsigned int ), 6 );

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );
		while (0 != fgets(line, sizeof(line), fp))
		{
retry:
			eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
						{
							goto retry;
						}

						char *p = strrchr(line, '/');
						if (p)
						{
							strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1), sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while( 0 != fgets(line, sizeof(line), fp) );
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}

	if (ruijie_x_serial_ext_card_number)
	{
		eth_num = 0;
		while(eth_num < ruijie_x_serial_ext_card_number)
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}
		eth_num = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( "onboard1" ) ) ) || ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard6", strlen( "onboard6" ) ) ) )
				{
					ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
				}
				else if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// HPC08
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1583" );

						// HPC12
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
						}
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=ngbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:0103" );

					// HPC11
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1581" );

						// HPC03
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '4';

							int iSlot = atoi( ruijie_x_serial_ext_card_entry[sorted_num].card_slot + strlen( "slot" ) );

							if ( 0xd01 == uiSlots[iSlot-1] )
							{
								ruijie_x_serial_ext_card_entry[sorted_num].mode = '5';
							}
						}
					}
				}
				else if ( 8 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1521" );

					// HPC01 or HP07
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1522" );

						// HPC06
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
						}
					}
				}
			}

			if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "MGT" );

				sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[0].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
			}
			else if ( ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2" ) ) || ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard3" ) ) )
			{
				const static unsigned int map[4] = {2, 1, 0, 3};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard4" ) )
			{
				const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				const static unsigned int map[2] = {1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '2' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {0, 1, 3, 2};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '3' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 )
			{
				const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '4' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {3, 2, 1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '5' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {2, 3, 0, 1};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index;

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( iEthIndex >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}
	}

error_out:
	if ( fp )
	{
		pclose( fp );
	}

	return;
}

static void changting_rename_if( int eth_num )
{
/*
TYPE = CT
*/
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     nmPortNum                   = 0;
	int     portIndex                   = 0;
	int     i                           = 0;
	int     man_make                    = 0;
	int     iDpdkEth                    = 0;

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	fp = fopen( bypass_switch_config, "r" );

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

		fclose(fp);
	}

	if ( !strstr( tmpstr, "CT" ) )
	{
		return;
	}

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose( fp );
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			return;
		}
	}

	FILE * fdmi = popen("/usr/sbin/dmidecode -t 1 | grep 'Product Name: ' | awk '{if (NR==1){print $0}}'", "r");
	if ( fdmi )
	{
		char dmi_product[128] = { 0 };
		if ( NULL != fgets( dmi_product, sizeof( dmi_product ), fdmi ) )
		{
			char *p = strstr( dmi_product, "Product Name: " );
			if ( p )
			{
				p += sizeof( "Product Name: " ) - 1;

				if ( !strcmp( p, "NR01\n" ) )
				{
					system( "/sbin/insmod /usr/private/auxiliary.ko" );

					system( "/sbin/insmod /usr/private/ice.ko" );
#ifndef ARCH_ARM64
					if ( F_OK == access( "/usr/private/led", F_OK ) )
					{
						system( "/usr/private/led sys on" );
					}
#endif
					// get eth num again
					eth_num = ace_get_port_num();

					if ( 17 == eth_num )
					{
						pclose( fdmi );

						nmPortNum = 1;

						for ( i = 0; i < 17; i++ )
						{
							sprintf( g_port_info[i].oldPhyname, "eth%d", rename_sorted_chuangshi_nr01_ports[i] );

							sprintf( g_port_info[i].newPhyname, "%s", chuangshi_nr01_map[i] );
						}

						goto will_done;
					}
					else if ( eth_num > 17 )
					{
						pclose( fdmi );

						nmPortNum = 1;

						changshi_nr01_ext_card_by_sys_class_net( rename_sorted_chuangshi_nr01_feature_string, rename_sorted_chuangshi_nr01_card_id_char, 
							rename_sorted_chuangshi_nr01_name_string, sizeof( rename_sorted_chuangshi_nr01_card_id_char ) / sizeof( rename_sorted_chuangshi_nr01_card_id_char[0] ) );

						goto will_done;
					}
				}
			}
		}

		pclose( fdmi );
	}

	remove( ACE_PORT_MAP_FILE );

	return;

will_done:
	iDpdkEth = eth_num - nmPortNum;

	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( i = 0; i < iDpdkEth && !iEnd; ++i )
				{
					if ( 's' != g_port_info[i].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[i].newPhyname[0] ) || ( 'W' == g_port_info[i].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[i].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 1;
						}
						else if ( 'M' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 2;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < eth_num; portIndex++ )
		{
			int iPair = 1;
			int ethAttribute = 0;

			if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, iPair,
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif

#ifdef ARCH_ARM64
#if 59
static const char* chuangshi_nt03_ext_card_feature_string[] = 
{
	" ../../devices/platform/soc/28210000.eth/",
	" ../../devices/platform/soc/2820c000.eth/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:05.0/", // 1, 0
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:03.0/", // 1, 0, 3, 2
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:04.0/", // 1, 0, 3, 2
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:01.0/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:00.0/"
};

const static char chuangshi_nt03_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7"
};

static const char* chuangshi_nt03_ext_card_mode_char[] = 
{
	"onboard1",
	"onboard2",
	"onboard3",
	"onboard4",
	"onboard5",
	"slot1",
	"slot2"
};

void changshi_nt03_ext_card_fiber_light_on( int iType, int iSlot, int iEthNum )
{
	int i = 0;
	char szCmd[256] = {0};
	char szLine[256] = {0};

	if ( ( 11 == iType ) || ( 12 == iType ) )
	{
		if ( iSlot > 0 )
		{
			int iGpioNum = 0;
			int iArrGpios[2] = {0};
		
			FILE* fpHpc = popen( "ls -l /sys/class/gpio/ | grep 28007000.i2c | awk -F'gpiochip' '{print $3}'", "r" );

			if ( fpHpc )
			{
				memset( szLine, 0x00, sizeof( szLine ) );

				while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpHpc ) )
				{
					int iStartGpio = atoi( szLine );

					if ( ( iStartGpio > 0 ) && ( iGpioNum < 2 ) )
					{
						iArrGpios[iGpioNum] = iStartGpio;

						iGpioNum++;
					}
				}

				pclose( fpHpc );
			}

			init_coordinate_log( "HPC, type:%d, gpio num:%d, gpio_1:%d, gpio_2:%d, slot:%d, eth_num:%d.", iType, iGpioNum, iArrGpios[0], iArrGpios[1], iSlot, iEthNum );

			if ( iGpioNum > 0 && iGpioNum <= 2 )
			{
				if ( 1 == iGpioNum )
				{
					for ( i=0; i<iEthNum; i++ )
					{
						snprintf( szCmd, sizeof( szCmd ) - 1, "echo %d > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio%d/direction && echo 0 > /sys/class/gpio/gpio%d/value", iArrGpios[0] + i, iArrGpios[0] + i, iArrGpios[0] + i );
						system( szCmd );
						init_coordinate_log( "HPC, 111, cmd [%s].", szCmd );
					}
				}
				else
				{
					// slot1
					if ( 0x01 == ( 0x01 & iSlot ) )
					{
						for ( i=0; i<iEthNum; i++ )
						{
							snprintf( szCmd, sizeof( szCmd ) - 1, "echo %d > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio%d/direction && echo 0 > /sys/class/gpio/gpio%d/value", iArrGpios[0] + i, iArrGpios[0] + i, iArrGpios[0] + i );
							system( szCmd );
							init_coordinate_log( "HPC, 222, cmd [%s].", szCmd );
						}
					}

					// slot2
					if ( 0x02 == ( 0x02 & iSlot ) )
					{
						for ( i=0; i<iEthNum; i++ )
						{
							snprintf( szCmd, sizeof( szCmd ) - 1, "echo %d > /sys/class/gpio/export && echo out > /sys/class/gpio/gpio%d/direction && echo 0 > /sys/class/gpio/gpio%d/value", iArrGpios[1] + i, iArrGpios[1] + i, iArrGpios[1] + i );
							system( szCmd );
							init_coordinate_log( "HPC, 333, cmd [%s].", szCmd );
						}
					}
				}
			}
			else
			{
				init_coordinate_log( "Error, get hpc gpio error." );
			}
		}
	}

	return;
}

int changshi_nt03_ext_get_card_type( char* pI2cAddr )
{
	int iRet = 0;

	if ( pI2cAddr )
	{
		char szCmd[128] = {0};
		char szLine[128] = {0};

		// i2cget -f -y 0x00 0x57 0xa0 => 0x02
		// i2cget -f -y 0x00 0x57 0xa1 => 0x0X
		snprintf( szCmd, sizeof( szCmd ) - 1, "i2cget -f -y %s 0x57 0xa1", pI2cAddr );

		FILE* fpHpcType = popen( szCmd, "r" );

		if ( fpHpcType )
		{
			memset( szLine, 0x00, sizeof( szLine ) );

			while ( 0 != fgets( szLine, sizeof( szLine ) - 1, fpHpcType ) )
			{
				szLine[4] = 0;

				int iHpcType = strtoul( szLine, NULL, 16 );

				if ( ( iHpcType > 0 ) && ( iHpcType < 32 ) )
				{
					iRet = iHpcType;

					break;
				}
			}

			pclose( fpHpcType );
		}
	}

	return iRet;
}

static void chuangshi_nt03_auto_rename_ext_card( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	int   i                           = 0;
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int iMgtNum = 0;
	int iHpc11Fiber = 0;
	int iHpc11Solt = 0;
	int iHpc11Eth = 0;
	int iHpc12Fiber = 0;
	int iHpc12Solt = 0;
	int iHpc12Eth = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen("ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r");
	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );
		while (0 != fgets(line, sizeof(line), fp))
		{
retry:
			eth_index = 0;
			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if (!strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if (strncmp(line, card_feature[card_index], strlen(card_feature[card_index])))
						{
							goto retry;
						}

						char *p = strrchr(line, '/');
						if (p)
						{
							strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim(p+1), sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName)-1);

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while( 0 != fgets(line, sizeof(line), fp) );
				}
			}
		}
		pclose(fp);
		fp = NULL;
	}

	if ( ruijie_x_serial_ext_card_number )
	{
		eth_num = 0;

		while ( eth_num < ruijie_x_serial_ext_card_number )
		{
			for (eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index)
			{
				if (!is_sorted[eth_index])
				{
					break;
				}
			}
			sorted_num = eth_index;
			for (card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
			{
				if (is_sorted[card_index])
				{
					continue;
				}
				if (sorted_num == card_index)
				{
					continue;
				}
				if (ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId)
				{
					sorted_num = card_index;
				}
			}
			is_sorted[sorted_num] = 1;
			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;
			eth_num++;
		}
		eth_num = 0;
		for(card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index)
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( "onboard1" ) ) ) || ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard6", strlen( "onboard6" ) ) ) )
				{
					ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
				}
				else if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// HPC08
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1583" );

						// HPC12
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';

							iHpc12Fiber = 1;
						
							iHpc12Eth = 2;
						
							if ( '6' == ruijie_x_serial_ext_card_entry[sorted_num].cardId )
							{
								iHpc12Solt |= 0x01;
							}
							else if ( '7' == ruijie_x_serial_ext_card_entry[sorted_num].cardId )
							{
								iHpc12Solt |= 0x02;
							}
						}
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=ngbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:0103" );

					// HPC11
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';

						// check Fiber ?
						iHpc11Fiber = 1;
						
						iHpc11Eth = 4;
						
						if ( '6' == ruijie_x_serial_ext_card_entry[sorted_num].cardId )
						{
							iHpc11Solt |= 0x01;
						}
						else if ( '7' == ruijie_x_serial_ext_card_entry[sorted_num].cardId )
						{
							iHpc11Solt |= 0x02;
						}
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1581" );

						// HPC03
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '4';

							int iHpcType = 0;
							
							// slot1
							if ( '6' == ruijie_x_serial_ext_card_entry[sorted_num].cardId )
							{
								iHpcType = changshi_nt03_ext_get_card_type( "0x04" );
							}
							// slot2
							else if ( '7' == ruijie_x_serial_ext_card_entry[sorted_num].cardId )
							{
								iHpcType = changshi_nt03_ext_get_card_type( "0x03" );
							}
							
							// HPC13
							if ( 0x0D == iHpcType )
							{
								ruijie_x_serial_ext_card_entry[sorted_num].mode = '5';
							}
						}
						// HPC22
						else if(check_dev_content( szInfoFile, "DRIVER=rnpm" ) && check_dev_content( szInfoFile, "PCI_ID=8848:1020" ))
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '6';					
						}
					}
				}
				else if ( 8 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1521" );

					// HPC01 or HP07
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=igb" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1522" );

						// HPC06
						if ( iDriver && iDevType )
						{
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '3';
						}
					}
				}
			}

			if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "MGT0" );
			
				sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[0].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
				g_port_info[0].isTrust = 2;

				iMgtNum++;
			}
			else if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "MGT1" );
			
				sprintf( g_port_info[1].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[1].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
				g_port_info[1].isTrust = 2;

				iMgtNum++;
			}
			else if ( ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard4" ) ) || ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard5" ) ) )
			{
				const static unsigned int map[4] = {1, 0, 3, 2};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard3" ) )
			{
				const static unsigned int map[2] = {1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 2 )
			{
				const static unsigned int map[2] = {1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '2' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {0, 1, 3, 2};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '3' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 8 )
			{
				const static unsigned int map[8] = {1, 0, 3, 2, 5, 4, 7, 6};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '4' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {3, 2, 1, 0};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '5' && ruijie_x_serial_ext_card_entry[sorted_num].ethNum == 4 )
			{
				const static unsigned int map[4] = {2, 3, 0, 1};

				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '6')
			{
				const static unsigned int map[4] = {0, 1, 2, 3};
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + map[eth_index];

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_mac_from_kernel(g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ));
					}
				}
			}						
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index;

					int ethAttribute = ( ( iEthIndex - iMgtNum ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtNum ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}

		g_portNum = eth_num;

		if ( 1 == iHpc11Fiber )
		{
			changshi_nt03_ext_card_fiber_light_on( 11, iHpc11Solt, iHpc11Eth );
		}

		if ( 1 == iHpc12Fiber )
		{
			changshi_nt03_ext_card_fiber_light_on( 12, iHpc12Solt, iHpc12Eth );
		}
	}

error_out:
	if ( fp )
	{
		pclose( fp );
	}

	return;
}

static void chuangshi_nt03_auto_rename_if(int eth_num)
{
/*
METHOD = 3
TYPE = CHUANGSHI_NT03
*/
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	int   i                           = 0;
	int   portIndex                   = 0;
	int   nmPortNum                   = 0;
	int   iDpdkEth                    = 0;
	int   man_make                    = 0;

	fp = fopen( ACE_PORT_MAP_FILE, "r" );
	if ( fp )
	{
		fgets( line, sizeof( line ), fp );
		fgets( line, sizeof( line ), fp );
		fclose( fp );
		fp = NULL;
		if (strstr(line, "####ManMakeExtMap") && (sscanf( line, "%s %d", tmpstr, &man_make ) == 2) && man_make)
		{
			return;
		}
	}

	fp = fopen( bypass_switch_config, "r" );
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

	if ( !strstr( tmpstr, "CHUANGSHI_NT03" ) )
	{
		return;
	}

	if ( have_i_pci_device( "17cddc16" ) && have_i_pci_device( "17cddc08" ) && have_i_pci_device( "17cddc01" ) )
	{
		system( "echo 1 > /sys/class/leds/sys/brightness" );

		system( "/usr/private/xspeed_nt03_reset &" );	
	}

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	if ( have_i_pci_device( "17cddc16" ) && have_i_pci_device( "17cddc08" ) && have_i_pci_device( "17cddc01" ) )
	{
		nmPortNum = 2;

		chuangshi_nt03_auto_rename_ext_card( chuangshi_nt03_ext_card_feature_string, chuangshi_nt03_ext_card_id_char, 
			chuangshi_nt03_ext_card_mode_char, sizeof( chuangshi_nt03_ext_card_id_char ) / sizeof( chuangshi_nt03_ext_card_id_char[0] ) );

		com_generate_linux2vpp();
		goto will_done;
	}

	return;

will_done:
	iDpdkEth = eth_num - nmPortNum;
#if 0
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( iDpdkEth != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", iDpdkEth, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( iDpdkEth == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( i = 0; i < iDpdkEth && !iEnd; ++i )
				{
					if ( 's' != g_port_info[i].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[i].newPhyname[0] ) || ( 'W' == g_port_info[i].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[i].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 1;
						}
						else if ( 'M' == g_port_info[i].newPhyname[0] )
						{
							iEthAttribute = 2;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[i].ethPciId, g_port_info[i].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}
#endif
	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < eth_num; portIndex++ )
		{
			int iPair = 1;
			int ethAttribute = 0;

			if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, iPair,
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif
#endif

#if 60
static const char* rename_sorted_leyan_d2000_feature_string[] = 
{
	" ../../devices/platform/PHYT0004:00/",
	" ../../devices/platform/PHYT0004:01/",
	" ../../devices/pci0000:00/0000:00:03.0/",
	" ../../devices/pci0000:00/0000:00:04.0/",
	" ../../devices/pci0000:00/0000:00:01.0/",
	" ../../devices/pci0000:00/0000:00:00.0/"
};

const static char rename_sorted_leyan_d2000_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6"
};

static const char* rename_sorted_leyan_d2000_name_string[] = 
{
	"onboard1",
	"onboard2",
	"slot1",
	"slot2",
	"slot3",
	"slot4"
};

static void leyan_d2000_ext_card( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	FILE* fp						  = NULL;
	char  line[256] 				  = { 0 };
	char  tmpstr[256]				  = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char device_prefix[32];
	int eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;

	memset(device_prefix, 0x00, sizeof(device_prefix));

	fp = popen( "ls -al /sys/class/net/ | grep eth | awk -F\\-\\> {'print $2'} | sort", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
retry:
			eth_index = 0;

			for (card_index = 0; card_index < card_max; ++card_index)
			{
				if ( !strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot ) - 1 );
					++ruijie_x_serial_ext_card_number;

					do
					{
						if ( strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
						{
							goto retry;
						}

						char *p = strrchr( line, '/' );

						if ( p )
						{
							strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim( p + 1 ), sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName ) - 1 );

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while ( 0 != fgets( line, sizeof( line ), fp ) );
				}
			}
		}

		pclose( fp ); fp = NULL;
	}

	if ( ruijie_x_serial_ext_card_number )
	{
		eth_num = 0;

		while ( eth_num < ruijie_x_serial_ext_card_number )
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index )
			{
				if ( !is_sorted[eth_index] )
				{
					break;
				}
			}

			sorted_num = eth_index;

			for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
			{
				if ( is_sorted[card_index] )
				{
					continue;
				}

				if ( sorted_num == card_index )
				{
					continue;
				}

				if ( ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId )
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		eth_num = 0;
		int iMgtPort = 0;
		int iNormalPort = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// 2光, 万兆，ABN-793S Rev1.1
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
				}
				else if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// 4光, 万兆，ABN-793S-4X Rev1.1
					if ( iDriver && iDevType )
					{
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
				}
			}

			// HA & MGMT
			if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard", strlen( "onboard" ) ) )
			{
				if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( "onboard1" ) ) )
				{
					sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
					sprintf( g_port_info[0].newPhyname, "%s", "HA" );
					sprintf( g_port_info[0].m_szPanel, "%s", "onboard" );

					iMgtPort++;
				}
				else
				{
					sprintf( g_port_info[1].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
					sprintf( g_port_info[1].newPhyname, "%s", "MGT" );
					sprintf( g_port_info[1].m_szPanel, "%s", "onboard" );

					iMgtPort++;
				}
			}
			// 1 0 3 2 ...
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iRealIndex = eth_index + ( ( 0 == eth_index % 2 ) ? 1 : -1 );

					int iEthIndex = eth_num + iRealIndex;
				
					int ethAttribute = iEthIndex & 1;

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = iEthIndex >> 1;
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W' : 'L', iEthIndex >> 1 );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						g_port_info[iEthIndex].ifIndex = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair;
						g_port_info[iEthIndex].isTrust = ethAttribute ? 0 : 1;
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, iRealIndex + 1 );

						iNormalPort++;
					}
				}
			}
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index;
				
					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = iEthIndex >> 1;
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W' : 'L', iEthIndex >> 1 );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						g_port_info[iEthIndex].ifIndex = ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair;
						g_port_info[iEthIndex].isTrust = ethAttribute ? 0 : 1;
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index + 1 );

						iNormalPort++;
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}

		g_iMgtNum = iMgtPort;

		g_portNum = iNormalPort;
	}

error_out:
	if ( fp )
	{
		pclose( fp ); fp = NULL;
	}

	return;
}

static void leyan_auto_rename_if( int eth_num )
{
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     portIndex                   = 0;
	char    szBoardType[128]            = { 0 };
	char    szProduct[128]              = { 0 };

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	if ( 1 == is_user_define_interface() )
	{
		return;
	}

	get_dev_type_interface( szBoardType, sizeof( szBoardType ) );

	if ( !strncmp( szBoardType, "LEYAN-RIS5172", strlen( "LEYAN-RIS5172" ) ) )
	{
		leyan_d2000_ext_card( rename_sorted_leyan_d2000_feature_string, rename_sorted_leyan_d2000_card_id_char, 
			rename_sorted_leyan_d2000_name_string, sizeof( rename_sorted_leyan_d2000_card_id_char ) / sizeof( rename_sorted_leyan_d2000_card_id_char[0] ) );

		goto will_done;
	}

	return;

will_done:
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( g_portNum != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", g_portNum, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( g_portNum == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( portIndex = 0; portIndex < g_portNum + g_iMgtNum && !iEnd; ++portIndex )
				{
					if ( ( 's' != g_port_info[portIndex].newPhyname[0] ) && ( 'M' != g_port_info[portIndex].newPhyname[0] ) && ( 'H' != g_port_info[portIndex].newPhyname[0] ) )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[portIndex].newPhyname[0] )
						{
							iEthAttribute = 1;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[portIndex].ethPciId, g_port_info[portIndex].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	get_product_info( szProduct, sizeof( szProduct ) );

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );
		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < g_portNum + g_iMgtNum; portIndex++ )
		{
			int iPair = 1;
			int ethAttribute = 0;

			if ( !strncmp( szProduct, "topsec", 6 ) )
			{
				if ( g_port_info[portIndex].m_szPanel[0] )
				{
					if ( 0 == strcmp( g_port_info[portIndex].m_szPanel, "onboard" ) )
					{
						sprintf( tmpstr, "#panel_info/%s/%s/%s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].m_szPanel, g_port_info[portIndex].newPhyname );
					}
					else
					{
						sprintf( tmpstr, "#panel_info/%s/%s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].m_szPanel );
					}

					fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
				}
			}

			if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, iPair,
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif

#if 61
static const char* topsec_t7h_ext_card_feature_string[] = 
{
	" ../../devices/pci0000:00/0000:00:1b.1/",
	" ../../devices/pci0000:00/0000:00:1b.0/",
	" ../../devices/pci0000:16/0000:16:00.0/",
	" ../../devices/pci0000:16/0000:16:02.0/",
	" ../../devices/pci0000:64/0000:64:00.0/",
	" ../../devices/pci0000:64/0000:64:02.0/"
};

const static char topsec_t7h_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6"
};

static const char* topsec_t7h_ext_card_name_string[] = 
{
	"onboard1",
	"onboard2",
	"slot1",
	"slot2",
	"slot3",
	"slot4"
};

static void qiyang_t7h_ext_card_auto_rename( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	int   i                           = 0;
	FILE* fp                          = NULL;
	char  line[256]                   = { 0 };
	char  tmpstr[256]                 = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	int eth_index = 0;
	char device_prefix[32] = {0};
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0,};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int iMgtEth = 0;
	unsigned int iNormalEth = 0;

	memset( device_prefix, 0x00, sizeof( device_prefix ) );

	fp = popen( "ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
retry:
			eth_index = 0;

			for ( card_index = 0; card_index < card_max; ++card_index )
			{
				if ( !strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					strncpy(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof(ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot)-1);
					++ruijie_x_serial_ext_card_number;

					do
					{
						if ( strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
						{
							goto retry;
						}

						char *p = strrchr( line, '/' );

						if ( p )
						{
							strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim( p + 1 ), sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number - 1].ethCard[eth_index].ethOldName ) - 1 );

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while( 0 != fgets( line, sizeof( line ), fp ) );
				}
			}
		}

		pclose( fp ); fp = NULL;
	}

	if ( ruijie_x_serial_ext_card_number )
	{
		eth_num = 0;

		while ( eth_num < ruijie_x_serial_ext_card_number )
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index )
			{
				if ( !is_sorted[eth_index] )
				{
					break;
				}
			}

			sorted_num = eth_index;

			for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
			{
				if ( is_sorted[card_index] )
				{
					continue;
				}

				if ( sorted_num == card_index )
				{
					continue;
				}

				if ( ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId )
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		eth_num = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 4 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1572" );

					// 10g
					if ( iDriver && iDevType )
					{
						// 3 2 1 0
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';
					}
				}
				else if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1583" );

					// 40g
					if ( iDriver && iDevType )
					{
						// 0 1
						ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
					}
					else
					{
						iDriver = check_dev_content( szInfoFile, "DRIVER=i40e" );
						iDevType = check_dev_content( szInfoFile, "PCI_ID=8086:1581" );

						// 10g
						if ( iDriver && iDevType )
						{
							// 0 1
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '1';
						}
						else if(check_dev_content( szInfoFile, "DRIVER=txgbe" ))
						{
							// 1 0
							ruijie_x_serial_ext_card_entry[sorted_num].mode = '2';							
						}
					}
				}
			}

			if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "HA" );

				sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[0].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
				snprintf( g_port_info[0].m_szPanel, sizeof( g_port_info[0].m_szPanel ) - 1, "#panel_info/%s/%s/%s\n", "HA", "onboard", "HA" );

				iMgtEth++;
			}
			else if ( 0 == strcmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard2" ) )
			{
				snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName, 8, "MGMT" );

				sprintf( g_port_info[1].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
				sprintf( g_port_info[1].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethNewName );
				snprintf( g_port_info[1].m_szPanel, sizeof( g_port_info[1].m_szPanel ) - 1, "#panel_info/%s/%s/%s\n", "MGMT", "onboard", "MGMT" );

				iMgtEth++;
			}
			// 0 1 2 3 ...
			else if ( '1' == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index;

					int ethAttribute = ( ( iEthIndex - iMgtEth ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtEth ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W' : 'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						snprintf( g_port_info[iEthIndex].m_szPanel, sizeof( g_port_info[iEthIndex].m_szPanel ) - 1, "#panel_info/%s/%s/%d\n", 
							g_port_info[iEthIndex].newPhyname, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						iNormalEth++;
					}
				}
			}
			// 3 2 1 0 ...
			else if ( '2' == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				int iTempIndex = 0;

				for ( eth_index = ruijie_x_serial_ext_card_entry[sorted_num].ethNum - 1, iTempIndex = 0; eth_index >= 0; eth_index--, iTempIndex++ )
				{
					int iEthIndex = eth_num + iTempIndex;

					int ethAttribute = ( ( iEthIndex - iMgtEth ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtEth ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W' : 'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						snprintf( g_port_info[iEthIndex].m_szPanel, sizeof( g_port_info[iEthIndex].m_szPanel ) - 1, "#panel_info/%s/%s/%d\n", 
							g_port_info[iEthIndex].newPhyname, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, iTempIndex );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						iNormalEth++;
					}
				}
			}
			// 1 0 3 2 ...
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + eth_index + ( ( 0 == ( eth_index % 2 ) ) ? 1 : -1 );

					int ethAttribute = ( ( iEthIndex - iMgtEth ) & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - iMgtEth ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;
					snprintf( ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W' : 'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT_BUSYBOX )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );
						snprintf( g_port_info[iEthIndex].m_szPanel, sizeof( g_port_info[iEthIndex].m_szPanel ) - 1, "#panel_info/%s/%s/%d\n", 
							g_port_info[iEthIndex].newPhyname, ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );

						get_eth_slot_dpdk_value( 1, g_port_info[iEthIndex].oldPhyname, g_port_info[iEthIndex].ethPciId, sizeof( g_port_info[iEthIndex].ethPciId ) );

						iNormalEth++;
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}

		g_iMgtNum = iMgtEth;

		g_portNum = iNormalEth;
	}

error_out:
	if ( fp )
	{
		pclose( fp ); fp = NULL;
	}

	return;
}

static void qiyang_auto_rename_if( int eth_num )
{
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     portIndex                   = 0;
	char    szBoardType[128]            = { 0 };
	char    szProduct[128]              = { 0 };

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	// skip userdefine
	if ( 1 == is_user_define_interface() )
	{
		return;
	}

	// get device type
	get_dev_type_interface( szBoardType, sizeof( szBoardType ) );

	if ( !strncmp( szBoardType, "TOPSEC-T7H-QIYANG-C6248R-1", strlen( "TOPSEC-T7H-QIYANG-C6248R-1" ) ) )
	{
		qiyang_t7h_ext_card_auto_rename( topsec_t7h_ext_card_feature_string, topsec_t7h_ext_card_id_char, 
			topsec_t7h_ext_card_name_string, sizeof( topsec_t7h_ext_card_id_char ) / sizeof( topsec_t7h_ext_card_id_char[0] ) );

		goto will_done;
	}

	return;

will_done:
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( g_portNum != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", g_portNum, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( g_portNum == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( portIndex = 0; portIndex < g_portNum + g_iMgtNum && !iEnd; ++portIndex )
				{
					if ( 's' != g_port_info[portIndex].newPhyname[0] && 'M' != g_port_info[portIndex].newPhyname[0] && 'H' != g_port_info[portIndex].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[portIndex].newPhyname[0] )
						{
							iEthAttribute = 1;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[portIndex].ethPciId, g_port_info[portIndex].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	// topsec
	get_product_info( szProduct, sizeof( szProduct ) );

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );

		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		for ( portIndex = 0; portIndex < g_portNum + g_iMgtNum; portIndex++ )
		{
			int iPair = 1;
			int ethAttribute = 0;

			if ( !strncmp( szProduct, "topsec", 6 ) )
			{
				if ( g_port_info[portIndex].m_szPanel[0] )
				{
					fwrite( g_port_info[portIndex].m_szPanel, strlen( g_port_info[portIndex].m_szPanel ), sizeof( char ), fp );
				}
			}

			if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' ) )
			{
				ethAttribute = 2;
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, iPair,
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif

#if 62
static const char* gongfeng_d2000_ext_card_feature_string[] = 
{
	" ../../devices/platform/soc/28210000.eth/",
	" ../../devices/platform/soc/2820c000.eth/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:03.0/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:04.0/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:01.0/",
	" ../../devices/platform/soc/40000000.pcie/pci0000:00/0000:00:00.0/"
};

const static char gongfeng_d2000_ext_card_id_char[][3] = 
{
	"1",
	"2",
	"3",
	"4",
	"5",
	"6"
};

static const char* gongfeng_d2000_ext_card_name_string[] = 
{
	"onboard1",
	"onboard2",
	"slot1",
	"slot2",
	"slot3",
	"slot4"
};

static void gongfeng_d2000_ext_card_auto_rename( const char* card_feature[], const char card_id[][3], const char *card_name[], unsigned int card_max )
{
	FILE* fp						  = NULL;
	char  line[256] 				  = { 0 };
	char  tmpstr[256]				  = { 0 };
	char* p_char;
	char* p_left;
	char* p_right;
	char  device_prefix[32] = { 0 };
	int   eth_index = 0;
	unsigned int card_index = 0;
	unsigned int eth_num = 0;
	unsigned int sorted_num = 0;
	unsigned int nm_index = 0;
	unsigned int is_sorted[MAX_CARD_ONE_DEV] = {0};
	unsigned int line_size = 0;
	unsigned int device_match = 0;
	unsigned int feature_match = 0;
	unsigned int man_make = 0;

	memset( device_prefix, 0x00, sizeof( device_prefix ) );

	fp = popen( "ls -al /sys/class/net/ | grep -E 'eth' | awk -F\\-\\> {'print $2'} | sort", "r" );

	if ( fp )
	{
		memset( line, 0x00, sizeof( line ) );

		while ( 0 != fgets( line, sizeof( line ), fp ) )
		{
retry:
			eth_index = 0;

			for ( card_index = 0; card_index < card_max; ++card_index )
			{
				if ( !strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
				{
					if ( ruijie_x_serial_ext_card_number > MAX_CARD_ONE_DEV )
					{
						goto error_out;
					}

					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].cardId = card_id[card_index][0];
					ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].mode = 0;
					strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot, card_name[card_index], sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number].card_slot ) - 1 );
					++ruijie_x_serial_ext_card_number;

					do
					{
						if ( strncmp( line, card_feature[card_index], strlen( card_feature[card_index] ) ) )
						{
							goto retry;
						}

						char *p = strrchr( line, '/' );

						if ( p )
						{
							strncpy( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName, hylab_trim( p + 1 ), sizeof( ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethCard[eth_index].ethOldName ) - 1 );

							ruijie_x_serial_ext_card_entry[ruijie_x_serial_ext_card_number-1].ethNum = ++eth_index;
						}
					} while ( 0 != fgets( line, sizeof( line ), fp ) );
				}
			}
		}

		pclose( fp );

		fp = NULL;
	}

	if ( ruijie_x_serial_ext_card_number )
	{
		eth_num = 0;

		while ( eth_num < ruijie_x_serial_ext_card_number )
		{
			for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_number; ++eth_index )
			{
				if ( !is_sorted[eth_index] )
				{
					break;
				}
			}

			sorted_num = eth_index;

			for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
			{
				if ( is_sorted[card_index] )
				{
					continue;
				}

				if (sorted_num == card_index)
				{
					continue;
				}

				if ( ruijie_x_serial_ext_card_entry[sorted_num].cardId > ruijie_x_serial_ext_card_entry[card_index].cardId )
				{
					sorted_num = card_index;
				}
			}

			is_sorted[sorted_num] = 1;

			ruijie_x_serial_ext_card_sorted[eth_num] = sorted_num;

			eth_num++;
		}

		eth_num = 0;

		int iMgtEth = 0;
		int iNormalEth = 0;
		int iPanelShow = 0;

		for ( card_index = 0; card_index < ruijie_x_serial_ext_card_number; ++card_index )
		{
			iPanelShow = 0;
		
			sorted_num = ruijie_x_serial_ext_card_sorted[card_index];

			if ( 0 == ruijie_x_serial_ext_card_entry[sorted_num].mode )
			{
				ruijie_x_serial_ext_card_entry[sorted_num].mode = '0';

				if ( 2 == ruijie_x_serial_ext_card_entry[sorted_num].ethNum )
				{
					char szInfoFile[256] = {0};

					snprintf( szInfoFile, sizeof( szInfoFile ) - 1, "/sys/class/net/%s/device/uevent", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );

					int iDriver = check_dev_content( szInfoFile, "DRIVER=txgbe" );
					int iDevType = check_dev_content( szInfoFile, "PCI_ID=8088:1001" );

					// 10g
					if ( iDriver && iDevType )
					{
						// 0 1
						iPanelShow = 1;
					}
				}
			}

			// HA & MGMT
			if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard", strlen( "onboard" ) ) )
			{
				if ( 0 == strncmp( ruijie_x_serial_ext_card_entry[sorted_num].card_slot, "onboard1", strlen( "onboard1" ) ) )
				{
					sprintf( g_port_info[0].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
					sprintf( g_port_info[0].newPhyname, "%s", "HA" );
					sprintf( g_port_info[0].m_szPanel, "%s", "onboard" );

					iMgtEth++;
				}
				else
				{
					sprintf( g_port_info[1].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[0].ethOldName );
					sprintf( g_port_info[1].newPhyname, "%s", "MGMT" );
					sprintf( g_port_info[1].m_szPanel, "%s", "onboard" );

					iMgtEth++;
				}
			}
			else if ( ruijie_x_serial_ext_card_entry[sorted_num].mode == '1' )
			{
			}
			// 2,1,4,3...
			// 8086:1533, 2 port ( board, 2电-千兆 )
			// 8088:0103, 8 port ( 8电-千兆-S82WG-8TB_VER_A-4bypass, 4电4光-千兆-S82WG-4TB4F_VER_A-2bypass, 8光-千兆-S82WG-8F_VER_A )
			// 8088:1001, 2 port ( 2光-万兆-S83WX-2F_VER_A )
			else
			{
				for ( eth_index = 0; eth_index < ruijie_x_serial_ext_card_entry[sorted_num].ethNum; ++eth_index )
				{
					int iEthIndex = eth_num + ( ( eth_index + 1 ) % 2 + ( eth_index / 2 ) * 2 );

					int ethAttribute = ( iEthIndex & 1 );

					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair = 1 + ( ( iEthIndex - 2 ) >> 1 );
					ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethAttribute = ethAttribute;

					snprintf(ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName, 8, "%cAN%d", ethAttribute ? 'W':'L', ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethPair );

					if ( iEthIndex >= ACE_MAX_SUPPORT_PORT )
					{
						goto error_out;
					}
					else
					{
						sprintf( g_port_info[iEthIndex].oldPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethOldName );
						sprintf( g_port_info[iEthIndex].newPhyname, "%s", ruijie_x_serial_ext_card_entry[sorted_num].ethCard[eth_index].ethNewName );

						if ( 1 == iPanelShow )
						{
							sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, ( eth_index + 1 ) % 2 + ( eth_index / 2 ) * 2 );
						}
						else
						{
							sprintf( g_port_info[iEthIndex].m_szPanel, "%s/%d", ruijie_x_serial_ext_card_entry[sorted_num].card_slot, eth_index );
						}

						iNormalEth++;
					}
				}
			}

			eth_num += ruijie_x_serial_ext_card_entry[sorted_num].ethNum;
		}

		g_iMgtNum = iMgtEth;

		g_portNum = iNormalEth;
	}

error_out:
	if ( fp )
	{
		pclose( fp );
	}

	return;
}

static void gongfeng_auto_rename_if( int eth_num )
{
	FILE*   fp                          = NULL;
	char    line[256]                   = { 0 };
	char    tmpstr[256]                 = { 0 };
	int     portIndex                   = 0;
	char    szBoardType[128]            = { 0 };
	char    szProduct[128]              = { 0 };

	if ( access( "/usr/mysys/skip_auto_interface", 0 ) == 0 )
		return;

	// skip userdefine
	if ( 1 == is_user_define_interface() )
	{
		return;
	}

	// get device type
	get_dev_type_interface( szBoardType, sizeof( szBoardType ) );

	if ( !strncmp( szBoardType, "TOPSEC-TNS-GONGFENG-FT2K8C-1", strlen( "TOPSEC-TNS-GONGFENG-FT2K8C-1" ) ) )
	{
		gongfeng_d2000_ext_card_auto_rename( gongfeng_d2000_ext_card_feature_string, gongfeng_d2000_ext_card_id_char, 
			gongfeng_d2000_ext_card_name_string, sizeof( gongfeng_d2000_ext_card_id_char ) / sizeof( gongfeng_d2000_ext_card_id_char[0] ) );

		goto will_done;
	}

	return;

will_done:
	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		int x = 0;
		int iVppEth = 0;
		char szLine[128] = {0};
		char szVppEthCache[128][64] = {0};

		int iRVpp = get_vpp_eth_config_info( szVppEthCache, &iVppEth );

		if ( ( 0 == iRVpp ) && ( iVppEth >= 0 ) )
		{
			if ( ( g_portNum != iVppEth ) || ( 0 == iVppEth ) )
			{
				unlink( ACE_VPP_PORT_MAP_FILE );

				init_coordinate_log( "Info, the ext card positon maybe changed 1, %d:%d, delete [%s].", g_portNum, iVppEth, ACE_VPP_PORT_MAP_FILE );
			}
			else if ( ( g_portNum == iVppEth ) && ( 0 != iVppEth ) )
			{
				int iEnd = 0;

				for( portIndex = 0; portIndex < g_portNum + g_iMgtNum && !iEnd; ++portIndex )
				{
					if ( 's' != g_port_info[portIndex].newPhyname[0] && 'M' != g_port_info[portIndex].newPhyname[0] && 'H' != g_port_info[portIndex].newPhyname[0] )
					{
						int iFind = 0;

						int iPair = 1;
						int iEthAttribute = 0;

						if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
						{
							iPair = atoi( g_port_info[portIndex].newPhyname + 3 );
						}

						if ( 'W' == g_port_info[portIndex].newPhyname[0] )
						{
							iEthAttribute = 1;
						}

						snprintf( szLine, sizeof( szLine ) - 1, "%s %s %d %s no-ignore", g_port_info[portIndex].ethPciId, g_port_info[portIndex].newPhyname, iPair,
							( ruijie_rename_attribute_name[iEthAttribute] ) );

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

							init_coordinate_log( "Info, the ext card positon maybe changed 2, new vpp eth info [%s], delete [%s].", szLine, ACE_VPP_PORT_MAP_FILE );
						}
					}
				}
			}
		}
	}

	// topsec
	get_product_info( szProduct, sizeof( szProduct ) );

	fp = fopen( ACE_PORT_MAP_FILE, "wt+" );

	if ( fp )
	{
		sprintf( tmpstr, "ifAutoCfgMode %d\n", 0 );

		fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );

		fwrite( "####ManMakeExtMap 0\n", strlen( "####ManMakeExtMap 0\n" ), sizeof( char ), fp );

		int ethPair = 1;
		int portIndex = 0;

		for ( portIndex = 0; portIndex < g_portNum + g_iMgtNum; portIndex++ )
		{
			int ethAttribute = 0;

			if ( g_port_info[portIndex].newPhyname[0] == 'W' )
			{
				ethAttribute = 1;
			}
			else if ( ( g_port_info[portIndex].newPhyname[0] == 'M' ) || ( g_port_info[portIndex].newPhyname[0] == 'H' )  )
			{
				ethAttribute = 2;
			}

			if ( !strncmp( szProduct, "topsec", 6 ) )
			{
				if ( g_port_info[portIndex].m_szPanel[0] )
				{
					if ( 0 == strcmp( g_port_info[portIndex].m_szPanel, "onboard" ) )
					{
						sprintf( tmpstr, "#panel_info/%s/%s/%s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].m_szPanel, g_port_info[portIndex].newPhyname );
					}
					else
					{
						sprintf( tmpstr, "#panel_info/%s/%s\n", g_port_info[portIndex].newPhyname, g_port_info[portIndex].m_szPanel );
					}

					fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
				}
			}

			if ( ( 'M' == g_port_info[portIndex].newPhyname[0] ) || ( 'H' == g_port_info[portIndex].newPhyname[0] ) )
			{
				ethPair = 1;
			}
			else if ( ( 'L' == g_port_info[portIndex].newPhyname[0] ) || ( 'W' == g_port_info[portIndex].newPhyname[0] ) )
			{
				ethPair = atoi( g_port_info[portIndex].newPhyname + 3 );
			}

			sprintf( tmpstr, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname, ethPair, 
				( ruijie_rename_attribute_name[ethAttribute] ) );

			fwrite( tmpstr, strlen( tmpstr ), sizeof( char ), fp );
		}

		fclose( fp ); fp = NULL;
	}

	return;
}
#endif

int ace_port_map( void )
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

	readPortNum = ace_get_port_num();

	int iPhyNum = ace_get_physical_port_num();

	init_coordinate_log( "readPortNum %d iPhyNum %d", readPortNum, iPhyNum );

	if ( iPhyNum > 0 )
	{
		ruijie_auto_rename_if(iPhyNum);

		topsec_auto_rename_if(readPortNum);

		chuangshi_c3000_auto_rename_if(readPortNum);

#if defined(CS_E2000Q_PLATFORM)
		int boardtype = ace_get_boardtype();

		if ( 6 == boardtype )
		{
			chuangshi_nt06_auto_rename_if( readPortNum );
		}
		else if ( 9 == boardtype )
		{
			chuangshi_nt09_auto_rename_if( readPortNum );

			system( "echo 0x0901 > /sys/class/fpga/cpld0/regs" );
		}
#endif

#ifdef ARCH_ARM64
		chuangshi_nt03_auto_rename_if( iPhyNum );
#endif

		changting_rename_if( readPortNum );

		leyan_auto_rename_if( iPhyNum );

		qiyang_auto_rename_if( iPhyNum );

		gongfeng_auto_rename_if( iPhyNum );
	}

    memset( g_port_info, 0, sizeof( g_port_info ) );

    fp = fopen( ACE_PORT_MAP_FILE, "r" );
    if ( ( !fp ) 
      || ( fgets( line, sizeof( line ), fp ) == 0 ) 
      || ( sscanf( line, "%s %d", cmdBuf, &autoCfgMode ) != 2 ) 
      || ( autoCfgMode ) )
    {
		init_coordinate_log(" here");

		if ( NULL != fp )
		{
			fgets( line, sizeof( line ), fp );

			if ( strstr( line, "####ManMakeExtMap" ) )
			{
				sscanf( line, "%s %d", tmpstr, &man_make );
			}
		}

		if ( !man_make )
		{
			goto PORT_CFG;
		}
	}

    portNum = 0;
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

        if ( sscanf( line, "%s %s %d %s %s", mac, newname, &ifindex, property, noIgnore ) != 5 )
        {
		    init_coordinate_log(" invalid line %s",line);
            continue;
        }
        
        if ( mac[2] == ':' && mac[5] == ':' && mac[8] == ':' && mac[11] == ':' && mac[14] == ':' )
        {
            if (0 != get_phy_name_by_mac(oldname, mac))
                continue;
        }
        else
        {
            strcpy(oldname, mac);
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

    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }

    if ( !portNum )
    {
        goto PORT_CFG;
    }

    #if 0
    if ( ( ignoreNum + portNum ) != readPortNum )
    {
        printf( "\n %s %d, readPortNum=%d, ignoreNum=%d, portNum=%d, port config error!!! \n", __FUNCTION__, __LINE__, readPortNum, ignoreNum, portNum );
        goto PORT_CFG;
    }
    #endif

    g_portNum = portNum;
	g_iMgtNum = nmPortNum;
    init_coordinate_log(" g_portNum %d nmPortNum %d",g_portNum,nmPortNum);
#if 0
    fp = fopen( ACE_PORT_INFO_FILE, "wt+" );
    if ( g_portNum && fp )
    {
        sprintf( cmdBuf, "ethernetportnum  %d\n", g_portNum - nmPortNum);
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );

        sprintf( cmdBuf, "[LAN]\n" );
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
        for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
        {
            if ( g_port_info[portIndex].isTrust==1 )
            {
                sprintf( cmdBuf, "LAN%d=>%s\n", g_port_info[portIndex].ifIndex, g_port_info[portIndex].newPhyname );
                fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
            }
        }

        sprintf( cmdBuf, "[WAN]\n" );
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
        for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
        {
            if ( !g_port_info[portIndex].isTrust )
            {
                sprintf( cmdBuf, "WAN%d=>%s\n", g_port_info[portIndex].ifIndex, g_port_info[portIndex].newPhyname );
                fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
            }
        }
        
#if 0
        sprintf( cmdBuf, "[MGT]\n" );
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
        for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
        {
            if ( g_port_info[portIndex].isTrust ==2)
            {
                sprintf( cmdBuf, "MGT%d=>%s\n", g_port_info[portIndex].ifIndex, g_port_info[portIndex].newPhyname );
                fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
            }
        }
#endif
    }

    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }
#endif
    return 0;

PORT_CFG:
    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }

    g_portNum = iPhyNum;

    for ( portIndex = 0; ( portIndex < g_portNum ) && ( portIndex < ACE_MAX_SUPPORT_PORT_BUSYBOX ); portIndex++ )
    {
        sprintf( g_port_info[portIndex].oldPhyname, "eth%d", portIndex );

#ifdef ACE_PORT_ORDER_LAN_WAN
		if ( 1 == CONFIG_HYTF_FW_NEW )
		{
			sprintf( g_port_info[portIndex].newPhyname, "GE0-%d", portIndex );

			if ( !( portIndex % 2 ) )
			{
				lanIndex++;
				g_port_info[portIndex].ifIndex = lanIndex;
				g_port_info[portIndex].isTrust = 1;
			}
			else
			{
				wanIndex++;
				g_port_info[portIndex].ifIndex = wanIndex;
				g_port_info[portIndex].isTrust = 0;
			}
		}
		else
		{
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
		}
#else
		if ( 1 == CONFIG_HYTF_FW_NEW )
		{
			sprintf( g_port_info[portIndex].newPhyname, "GE0-%d", portIndex );

			if ( !( portIndex % 2 ) )
			{
	            lanIndex++;
	            g_port_info[portIndex].ifIndex = lanIndex;
	            g_port_info[portIndex].isTrust = 1;
			}
			else
			{
	            wanIndex++;
	            g_port_info[portIndex].ifIndex = wanIndex;
	            g_port_info[portIndex].isTrust = 0;
			}
		}
		else
		{
	        if ( portIndex < g_portNum / 2 )
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
			if ( 1 == CONFIG_HYTF_FW_NEW )
			{
	            sprintf( cmdBuf, "%s %s %d %s no-ignore inband\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname,
	                g_port_info[portIndex].ifIndex, g_port_info[portIndex].isTrust ? "TRUST" : "UNTRUST" );
			}
			else
			{
	            sprintf( cmdBuf, "%s %s %d %s no-ignore\n", g_port_info[portIndex].oldPhyname, g_port_info[portIndex].newPhyname,
	                g_port_info[portIndex].ifIndex, g_port_info[portIndex].isTrust ? "TRUST" : "UNTRUST" );
			}

            fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
        }
    }

    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }
#if 0
    fp = fopen( ACE_PORT_INFO_FILE, "wt+" );
    if ( g_portNum && fp )
    {
        sprintf( cmdBuf, "ethernetportnum  %d\n", g_portNum - nmPortNum);
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );

        sprintf( cmdBuf, "[LAN]\n" );
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
        for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
        {
            if ( g_port_info[portIndex].isTrust )
            {
                sprintf( cmdBuf, "LAN%d=>%s\n", g_port_info[portIndex].ifIndex, g_port_info[portIndex].newPhyname );
                fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
            }
        }

        sprintf( cmdBuf, "[WAN]\n" );
        fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
        for ( portIndex = 0; portIndex < g_portNum; portIndex++ )
        {
            if ( !g_port_info[portIndex].isTrust )
            {
                sprintf( cmdBuf, "WAN%d=>%s\n", g_port_info[portIndex].ifIndex, g_port_info[portIndex].newPhyname );
                fwrite( cmdBuf, strlen( cmdBuf ), sizeof( char ), fp );
            }
        }
    }

    if ( fp )
    {
        fclose( fp );
        fp = NULL;
    }
#endif

    return 0;
}

int ace_is_firewall()
{
	return 0;
}


