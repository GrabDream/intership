#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#if !defined(ARCH_ARM64)
#include <sys/io.h>
#endif
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include "ext_card_common.h"
#include "../../ace_include/ace_common_macro.h"

/* chuangshi C3000 NK06 */
static uint16_t smbus_io_base = 0xE000;

static int available = 0;

#define SMBUS_HSTS_REG (smbus_io_base + 0x00)
#define SMBUS_HCTL_REG (smbus_io_base + 0x02)
#define SMBUS_HCMD_REG (smbus_io_base + 0x03)
#define SMBUS_TSA_REG  (smbus_io_base + 0x04)
#define SMBUS_HD0_REG  (smbus_io_base + 0x05)
#define SMBUS_HD1_REG  (smbus_io_base + 0x06)
#define SMBUS_HBD_REG  (smbus_io_base + 0x07)

#define SMBUS_AUXS_REG  (smbus_io_base + 0x0C)
#define SMBUS_SMBC_REG  (smbus_io_base + 0x0F)

static int smbus_complete(void)
{
#if !defined(ARCH_ARM64)
	uint8_t status = inb(SMBUS_HSTS_REG);
	if (status & SMBUS_HSTS_INTR_BIT)
	{
		return 1;
	}
#endif
	return 0;
}

static int smbus_check_transaction(void)
{
	int i;
	for(i = 0; i < 100; i++)
	{
		if (smbus_complete())
		{
			return 0;
		}
		usleep(100);
	}
	return -1;
}

static int smbus_busy(void)
{
#if !defined(ARCH_ARM64)
	uint8_t status = inb(SMBUS_HSTS_REG);
	if (status & SMBUS_HSTS_HBSY_BIT)
	{
		return 1;
	}
#endif
	return 0;
}

static int smbus_reset(void)
{
#if !defined(ARCH_ARM64)
	uint8_t status;
    int i;
    for (i = 0; i < 10; i++) 
    {
        status = inb(SMBUS_HSTS_REG);
        outb(status, SMBUS_HSTS_REG);

        if (status & (SMBUS_HSTS_FAIL_BIT | SMBUS_HSTS_BERR_BIT | SMBUS_HSTS_DERR_BIT))
		{
			if (i == 10)
			{
				return -1;
			}
			else
			{
				continue;
			}
		}
		break;
    }
#endif
    return 0;
}

static int smbus_transmit_byte(uint8_t is_read, uint8_t slave_dev_addr, uint8_t smbus_command, uint8_t *read_data)
{
#if !defined(ARCH_ARM64)
	if (smbus_reset() < 0)
	{
		printf("smbus reset failed\n");
		return -1;
	}
	if (smbus_busy())
	{
		printf("smbus is busy\n");
		return -1;
	}

	slave_dev_addr <<= 1;
	if (is_read)
	{
		slave_dev_addr += 1;
	}

	outb(slave_dev_addr, SMBUS_TSA_REG);
	outb(smbus_command, SMBUS_HCMD_REG);

	if (!is_read)
	{
		outb(*read_data, SMBUS_HD0_REG);
		usleep(5000);
	}

	outb(0x40 | (0x2 << 2),SMBUS_HCTL_REG);

	if (smbus_check_transaction())
	{
		return -1;
	}
	if (is_read)
	{
		*read_data = inb(SMBUS_HD0_REG);
	}
#endif
	return 0;
}

static int smbus_read_byte(uint8_t slave_dev_addr, unsigned int smbus_command, uint8_t *read_data)
{
	if (available)
	{
		return smbus_transmit_byte(1, slave_dev_addr, smbus_command, read_data);
	}
	//printf("available is 0\n");
	return -1;
}

static int smbus_write_byte(uint8_t slave_dev_addr, unsigned int smbus_command, uint8_t write_data)
{
	if (available)
	{
		return smbus_transmit_byte(0, slave_dev_addr, smbus_command, &write_data);
	}
	//printf("available is 0\n");
	return -1;
}

static inline int read_pci_config_from_proc(uint16_t reg_num, uint32_t *reg_value)
{
    char path[32];
    int read_len;
    int try_cnt;
	int fd,iret = 0;
	int read_cnt;
	off_t curr_pos;
    snprintf(path, sizeof(path), "/proc/bus/pci/00/1f.4");
    if (access(path, F_OK | R_OK) < 0)
        return -1;

    fd = open(path, O_RDONLY);
    if (fd < 0)
    	return -1;

    read_len = 4;
    try_cnt = 5;
    while (try_cnt > 0)
    {
		curr_pos = lseek(fd, reg_num * 4, SEEK_SET);
		try_cnt--;
		if (curr_pos < 0)
		{
	        if (try_cnt == 0)
	        {
	        	iret = -1;
				goto out;
	        }
	        else
	            continue;
		}

		read_cnt = read(fd, reg_value, read_len);
		if (read_cnt < 0)
		{
	        if (try_cnt == 0)
	        {
				iret = -1;
				goto out;
	        }
	        else
	            continue;
		}

		if (read_cnt == read_len)
		{
		    break;
		}
		else
		{
	        if (try_cnt == 0)
	        {
				iret = -1;
				goto out;
	        }
		}
    }
out:
	close(fd);
    return iret;
}


static int read_pci_config_reg(uint16_t reg_num, uint32_t *reg_value)
{
        int ret = 0;
	ret = read_pci_config_from_proc(reg_num, reg_value);
        if (ret < 0)
        {
                *reg_value = 0x0;
        }
        return ret;
}

static int read_pci_config_word(uint16_t offset, uint16_t *value)
{
        uint8_t reg_num = offset / 4;
        uint8_t byte_offset = offset % 4;
        uint32_t reg_data;
        int ret = 0;
	ret = read_pci_config_reg(reg_num, &reg_data);
        *value = ((reg_data >> (byte_offset * 8)) & 0xFFFF);
        return ret;
}


static int smbus_init(void)
{
	int ret = 0;
	uint16_t offset = 0;
	uint32_t pci_id;
	uint16_t vid ;
	uint16_t did ;

	ret = read_pci_config_reg(offset, &pci_id);
	if (ret < 0)
	{
		return -1;
	}

	vid = pci_id;
	did = (pci_id >> 16);
	if (vid != 0x8086)
	{
		return -1;
	}

	offset = 0x20;

	ret = read_pci_config_word(offset, &smbus_io_base);
	if (ret < 0)
	{
		return -1;
	}

	if (smbus_io_base & 0x01)
	{
		smbus_io_base &= ~0x01;
	}
	else
	{
		printf("smbus controller is not I/O mapped!\n");
	}

	if (did == 0x19DF)
	{
		available = 1;
	}
	else
	{
		available = 0;
		return -1;
	}

	return 0;
}

static int read_exp_card_child_type(uint8_t type, uint8_t value)
{
	int child_type = UNKNOW_TYPE;

	switch(type)
	{
		case HPC01:
			if(value == 0x01)
				child_type = TYPE_4_1G_COPPER_4_1G_FIBER_BP;
			else if(value == 0x02)
				child_type = TYPE_4_1G_COPPER_4_1G_FIBER;
			break;

		case HPC02:
			if(value == 0x01)
				child_type = TYPE_4_1G_FIBER;
			else if(value == 0x02)
				child_type = TYPE_4_1G_COPPER;
			else if(value == 0x03)
				child_type = TYPE_4_1G_COPPER_BP;
			break;

		case HPC03:
			if(value == 0x01)
				child_type = TYPE_4_10G_FIBER;
			break;

		case HPC04:
			if(value == 0x02)
				child_type = TYPE_4_1G_COPPER_BP;
			break;

		case HPC05:
			if(value == 0x01)
				child_type = TYPE_2_1G_COPPER_2_1G_FIBER_BP;
			else if(value == 0x02)
				child_type = TYPE_2_1G_COPPER_2_1G_FIBER;
			break;

		case HPC06:
			if(value == 0x01)
				child_type = TYPE_8_1G_FIBER;
			break;

		case HPC07:
			if(value == 0x01)
				child_type = TYPE_8_1G_COPPER_BP;
			else if(value == 0x02)
				child_type = TYPE_8_1G_COPPER;
			break;

		case HPC08:
			if(value == 0x01)
				child_type = TYPE_2_10G_FIBER;
			break;

		case HPC09:
			if(value == 0x01)
				child_type = TYPE_4_1G_COPPER_4_1G_FIBER_COMBO;
			break;

		case HPC10:
			if(value == 0x01)
				child_type = TYPE_4_10G_FIBER;
			break;

		case HPC11:
			if(value == 0x01)
				child_type = TYPE_4_1G_FIBER;
			else if(value == 0x02)
				child_type = TYPE_4_1G_COPPER_BP;
			else if(value == 0x03)
				child_type = TYPE_4_1G_COPPER;
			break;

		case HPC12:
			if(value == 0x01)
				child_type = TYPE_2_40G_FIBER;
			break;

		case HPC13:
			if(value == 0x01)
				child_type = TYPE_4_10G_FIBER_BP;
			else if(value == 0x02)
				child_type = TYPE_4_10G_FIBER_BP;
			break;

		default:
			child_type = UNKNOW_TYPE;
			break;
	}
	return child_type;
}

static void matching_exp_card(int solt_numb, SoltInfo_t *info)
{
	uint8_t val;

	smbus_read_byte(0x57, 0xa0, &val);
	if(val != 0x02)
		printf("Unknown expansion card type!\n");
	smbus_read_byte(0x57, 0xa1, &val);
	if(val == Unknow)
		printf("Unknown expansion card type!\n");
	else
	{
		if ( solt_numb == 1 )
		{
			info->solt1.type = val;
			sprintf(info->solt1.name, "HPC%02d", val);
			smbus_read_byte(0x57, 0xa2, &val);
			info->solt1.child_type = read_exp_card_child_type(info->solt1.type, val);
		}
		else
		{
			info->solt2.type = val;
			sprintf(info->solt2.name, "HPC%02d", val);
			smbus_read_byte(0x57, 0xa2, &val);
			info->solt2.child_type = read_exp_card_child_type(info->solt2.type, val);
		}
	}
}

static void read_exp_card_info(SoltInfo_t *info)
{
	uint8_t val;
	if (info->solt_state == ALL_SOLT_ONLINE)
	{
		/* solt 1 */
		smbus_read_byte(0x70, 0x02, &val);
		matching_exp_card(1, info);
		/* solt 2 */
		smbus_read_byte(0x70, 0x01, &val);
		matching_exp_card(2, info);
	}
	else if(info->solt_state == SOLT_1_ONLINE) /* solt 1 */
	{
		smbus_read_byte(0x70, 0x02, &val);
		matching_exp_card(1, info);
	}
	else if(info->solt_state == SOLT_2_ONLINE) /* solt 2 */
	{
		smbus_read_byte(0x70, 0x01, &val);
		matching_exp_card(2, info);
	}
}

static int exp_card_online_monitoring(SoltInfo_t *info)
{
	uint8_t val;
	smbus_write_byte(0x20, 0x06, 0xfc);
	smbus_read_byte(0x20, 0x00, &val);

	switch(val & 0x3)
	{
		case 0:
			info->solt_state = ALL_SOLT_ONLINE;
			break;
		case 1:
			info->solt_state = SOLT_1_ONLINE;
			break;
		case 2:
			info->solt_state = SOLT_2_ONLINE;
			break;
		default:
			info->solt_state = NO_SOLT_ONLINE;
			break;
	}
	read_exp_card_info(info);
	return info->solt_state;
}

void chuangshi_auto_rename_c3000_nk06_solt_info( int* iSlotFlag, int* iCardModel1, int* iCardModel2, int* iCardType1, int* iCardType2 )
{
#if !defined(ARCH_ARM64)
	int iRet = 0;
	SoltInfo_t info = {0};

	iRet = iopl( 3 );
	if ( iRet < 0 )
	{
		printf( "[%s:%d] ==> iopl set priprity error, %d,%d,%s\n", __FUNCTION__, __LINE__, iRet, errno, strerror( errno ) );

		return;
	}

    iRet = smbus_init();
   	if ( iRet )
	{
        printf( "[%s:%d] ==> smbus init error, %d,%d,%s\n", __FUNCTION__, __LINE__, iRet, errno, strerror( errno ) );

        return;
    }

	memset( &info, 0x00, sizeof(SoltInfo_t) );

	switch ( exp_card_online_monitoring( &info ) )
	{
		case NO_SOLT_ONLINE:
			*iSlotFlag = NO_SOLT_ONLINE;
			printf( "no card in the expansion slot! \n" );
			break;
		case SOLT_1_ONLINE:
			*iSlotFlag = SOLT_1_ONLINE;
			*iCardModel1 = info.solt1.type;
			*iCardType1 = info.solt1.child_type;
			//printf("%s in expansion slot 1 is already in place, child type:[%d] \n",info.solt1.name, info.solt1.child_type);
			break;
		case SOLT_2_ONLINE:
			*iSlotFlag = SOLT_2_ONLINE;
			*iCardModel2 = info.solt2.type;
			*iCardType2 = info.solt2.child_type;
			//printf("%s in expansion slot 2 is already in place, child type:[%d] \n",info.solt2.name, info.solt2.child_type);
			break;
		case ALL_SOLT_ONLINE:
			*iSlotFlag = ALL_SOLT_ONLINE;
			*iCardModel1 = info.solt1.type;
			*iCardModel2 = info.solt2.type;
			*iCardType1 = info.solt1.child_type;
			*iCardType2 = info.solt2.child_type;
			//printf("%s in expansion slot 1 is already in place, child type:[%d] \n",info.solt1.name, info.solt1.child_type);
			//printf("%s in expansion slot 2 is already in place, child type:[%d] \n",info.solt2.name, info.solt2.child_type);
			break;
		default:
			printf("check error!\n");
	}
#endif
	return;
}

#ifndef ARCH_ARM64
#define ERR_DEVICE_TABLE_T5H	-4

#define NCT6791D_START_CHAR_T5H				0x87
#define NCT6791D_END_CHAR_T5H				0xAA
#define NCT6791D_WRITE_FLAG_T5H				1
#define NCT6791D_READ_FLAG_T5H				0
#define NCT6791D_LND_REG_T5H				0x07

#define NCT6791D_GPIO_EN_REG_T5H			0x30

#define NCT6791D_HWMonitor_LND_T5H			0x0B
#define NCT6791D_HWMonitor_EN_REG_T5H		0x30
#define NCT6791D_HWMonitor_EN_T5H			0x01
#define NCT6791D_HWMonitor_ADDR_HI_REG_T5H	0x60
#define NCT6791D_HWMonitor_ADDR_LO_REG_T5H	0x61
#define NCT6791D_HWMonitor_BANK_SEL_REG_T5H	0x4E
#define NCT6791D_HWMonitor_DATA_BANK_T5H	4
#define NCT6791D_GPIO_PUSH_PULL_OD_LND_T5H	0xF

unsigned char NCT6791D_GPIO_LND_T5H[]  = {8,8,9,9,9,9,7,7,7};
unsigned char NCT6791D_GPIO_EN_BIT_T5H[] = {1,	7,	  0,	1,	  2,	3,	  0,	1,	  2};
unsigned char NCT6791D_GPIO_OUTPUT_EN_REG_T5H[] = {0xe0, 0xf0, 0xe0, 0xe4, 0xf0, 0xf4, 0xf4, 0xe0, 0xe4};
unsigned char NCT6791D_GPIO_DATA_REG_T5H[] = {0xe1, 0xf1, 0xe1, 0xe5, 0xf1, 0xf5, 0xf5, 0xe1, 0xe5};
unsigned char NCT6791D_GPIO_MULTIFUNC_REG_T5H[]	= {0xe4, 0xf4, 0xe9, 0xeA, 0xee, 0xeb, 0xf8, 0xec, 0xed};
unsigned char NCT6791D_GPIO_MULTIFUNC_VALUE_T5H[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char NCT6791D_GPIO_PUSH_PULL_OD_REG_T5H[] = {0xE9, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7};
unsigned char NCT6791D_GPIO_PUSH_PULL_OD_VALUE_T5H[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

unsigned char NCT6791D_ucLDN_T5H = 0xff;
unsigned long NCT6791D_HWMonitor_addr_T5H;

void delay_t5h(int secs)
{
	usleep(secs * 1000);
}

void outportb_t5h(unsigned long ulport, int data)
{
	outb_p(data, ulport);
}

int inportb_t5h(unsigned long ulport)
{
	int data_rw8 = 0;
	data_rw8 = inb_p(ulport);
	return data_rw8;
}

void NCT6791D_Enter_SIO_t5h(unsigned long ulportIdx)
{
	outportb_t5h(ulportIdx, NCT6791D_START_CHAR_T5H);
	delay_t5h(1);
	outportb_t5h(ulportIdx, NCT6791D_START_CHAR_T5H);
}

void Write_SIO_t5h(unsigned long ulportIdx, unsigned long ulportData, int reg, int val)
{
	outportb_t5h(ulportIdx, reg);

	outportb_t5h(ulportData, val);
}

unsigned long Read_SIO_t5h(unsigned long ulportIdx, unsigned long ulportData, int reg)
{
	outportb_t5h(ulportIdx, reg);

	return inportb_t5h(ulportData);
}

void NCT6791D_Exit_SIO_t5h(unsigned long ulportIdx)
{
	outportb_t5h(ulportIdx, NCT6791D_END_CHAR_T5H);
}

int NCT6791D_GPIO_get_data_t5h(unsigned char outputFlag, unsigned long ulReg, unsigned long *ucValue, int argc, unsigned long argv[])
{
	unsigned char ucInterface;
	unsigned char ucSlaveID;
	unsigned long ulportIdx;
	unsigned long ulportData;
	unsigned char ucData;
	int index;

	ucInterface = argv[2];
	ucSlaveID = argv[3];
	ulportIdx = argv[4];
	ulportData = argv[5];

	index = argv[0] / 0x10;

	//if(NCT6791D_ucLDN != NCT6791D_GPIO_LND[index])
	{
		Write_SIO_t5h(ulportIdx, ulportData, NCT6791D_LND_REG_T5H, NCT6791D_GPIO_LND_T5H[index]);
		NCT6791D_ucLDN_T5H = NCT6791D_GPIO_LND_T5H[index];
	}

	switch(ucInterface)
	{
		case 'P':
			if(outputFlag)
				Write_SIO_t5h(ulportIdx, ulportData, ulReg, *ucValue);
			else
				*ucValue = Read_SIO_t5h(ulportIdx, ulportData, ulReg);
			break;

		default:
			//break;
			return ERR_DEVICE_TABLE_T5H;
	}

	return 0;
}

int NCT6791D_init_t5h()
{
	unsigned long ulAddrHI, ulAddrLO;
	unsigned char ucData;
	int i, rc;

	iopl(3);

	NCT6791D_Enter_SIO_t5h( 0x2e );

	Write_SIO_t5h( 0x2e, 0x2f, NCT6791D_LND_REG_T5H, NCT6791D_HWMonitor_LND_T5H);

	Write_SIO_t5h( 0x2e, 0x2f, NCT6791D_HWMonitor_EN_REG_T5H, NCT6791D_HWMonitor_EN_T5H);

	ulAddrHI = Read_SIO_t5h( 0x2e, 0x2f, NCT6791D_HWMonitor_ADDR_HI_REG_T5H);

	ulAddrLO = Read_SIO_t5h( 0x2e, 0x2f, NCT6791D_HWMonitor_ADDR_LO_REG_T5H);

	NCT6791D_HWMonitor_addr_T5H = ulAddrHI * 0x100 + ulAddrLO;

	Write_SIO_t5h( NCT6791D_HWMonitor_addr_T5H + 5, NCT6791D_HWMonitor_addr_T5H + 6, NCT6791D_HWMonitor_BANK_SEL_REG_T5H, NCT6791D_HWMonitor_DATA_BANK_T5H);

	return 0;
}

int NCT6791D_uninit_t5h()
{
	NCT6791D_Exit_SIO_t5h( 0x2e );

	return 0;
}

int NCT6791D_GPIO_set_enable_t5h(int argc, unsigned long argv[])
{
	unsigned long ulReg;
	unsigned long ulData, ulTempData, ulEnable;
	unsigned long ulIdx;

	ulEnable = argv[1];
	ulIdx = argv[0] / 0x10;
	ulReg = NCT6791D_GPIO_EN_REG_T5H;

	NCT6791D_GPIO_get_data_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
	ulTempData = (1 << NCT6791D_GPIO_EN_BIT_T5H[ulIdx]);
	ulData &= (~ulTempData);
	if(ulEnable > 0)
		ulData |= ulTempData;

	NCT6791D_GPIO_get_data_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

	return 0;
}

int NCT6791D_GPIO_get_configure_t5h(unsigned char outputFlag, unsigned long ulReg, unsigned long *ucValue, int argc, unsigned long argv[])
{
	unsigned char ucInterface;
	unsigned char ucSlaveID;
	unsigned long ulportIdx;
	unsigned long ulportData;

	ucInterface = argv[2];
	ucSlaveID = argv[3];
	ulportIdx = argv[4];
	ulportData = argv[5];

	switch(ucInterface)
	{
		case 'P':
			if(outputFlag)
				Write_SIO_t5h(ulportIdx, ulportData, ulReg, *ucValue);
			else
				*ucValue = Read_SIO_t5h(ulportIdx, ulportData, ulReg);
			break;

		default:
			//break;
			return ERR_DEVICE_TABLE_T5H;
	}

	//ESDK_printf("GPIO - *ucValue : 0x%x \r\n", *ucValue);

	return 0;
}

int NCT6791D_GPIO_set_push_pull_mode_function_t5h(int argc, unsigned long argv[])
{
	unsigned long ulReg;
	unsigned long ulData;
	unsigned long ulIdx, ulTempData;
	int i;

	// Set Logic device F
	ulReg = NCT6791D_LND_REG_T5H;
	ulData = NCT6791D_GPIO_PUSH_PULL_OD_LND_T5H;
	NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

	// Set push_pull mode
	ulIdx = argv[0]%0x10;
	ulReg = NCT6791D_GPIO_PUSH_PULL_OD_REG_T5H[argv[0]/0x10];
	NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);

	ulData &= ~(1 << ulIdx);
	ulTempData = NCT6791D_GPIO_PUSH_PULL_OD_VALUE_T5H[ulIdx] & (1 << ulIdx);
	ulData |= ulTempData;

	NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

}

int NCT6791D_GPIO_set_IO_t5h(int argc, unsigned long argv[])
{
	unsigned long ulReg;
	unsigned long ulData;
	unsigned long ulTempData;
	int i;

	ulReg = NCT6791D_GPIO_OUTPUT_EN_REG_T5H[argv[0] / 0x10];
	ulTempData = argv[1];

	NCT6791D_GPIO_get_data_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
	i = argv[0] % 0x10;
	ulData = ulData & (~(1 << i));		// 0: Output
	if(ulTempData == 0)
		ulData |= (1 << i);				// 1: Input

	NCT6791D_GPIO_get_data_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

	return 0;
}

int NCT6791D_GPIO_switch_default_function_t5h(int argc, unsigned long argv[])
{
	unsigned long ulReg;
	unsigned long ulData;
	unsigned long ulIdx;
	int i;

	//ucInterface = argv[2];
	//ucSlaveID = argv[3];
	//ulportIdx = argv[4];
	//ulportData = argv[5];

	// GPIO0X
	if((argv[0] / 0x10) == 0x0)
	{
		ulReg = 0x1c;
		if(argv[0] < 0x03)
		{
			// CR1C[0]=1, CR1C[1]=1, CR1C[2]=1
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulIdx = argv[0]%0x10;
			ulData |= (1 << ulIdx);
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x03)
		{
			//CR1C[4:3]=00
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xe7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] < 0x07)
		{
			// CR1C[5]=1, CR1C[6]=1, CR1C[7]=1
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulIdx = argv[0]%0x10;
			ulIdx++;
			ulData |= (1 << ulIdx);
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}
	// GPIO1X
	else if((argv[0] / 0x10) == 0x1)
	{
		// CR2A[6]=1
		ulReg = 0x2a;
		NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
		ulData |= 0x40;
		NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

		if((argv[0] == 0x12)||(argv[0] == 0x13))
		{
			// CR2A[6]=1, CR27[2]=0
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfb;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}
	// GPIO2X
	else if((argv[0] / 0x10) == 0x2)
	{
		if((argv[0] == 0x20)||(argv[0] == 0x21))
		{
			// CR2A[0]=1
			ulReg = 0x2a;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x1;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if((argv[0] == 0x22)||(argv[0] == 0x23))
		{
			// CR2A[1]=1
			ulReg = 0x2a;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x2;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x24)
		{
			// CR1B[4]=0, CR27[3]=0
			ulReg = 0x1b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xef;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

		}
		else if(argv[0] == 0x25)
		{
			// CR2A[3]=0, CR27[3]=0
			ulReg = 0x2a;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

		}
		else if(argv[0] == 0x26)
		{
			// CR2C[0]=0
			ulReg = 0x2c;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfe;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x27)
		{
			// CR2C[4:3]=00
			ulReg = 0x2c;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xe7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}
	// GPIO3X
	else if((argv[0] / 0x10) == 0x3)
	{
		if(argv[0] == 0x30)
		{
			// CR1A[7:6]=01
			ulReg = 0x1a;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0x3f;
			ulData |= 0x40;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		// p320
		if((argv[0] == 0x31) || (argv[0] == 0x32))
		{
			// CR1A[4]=1, CR1B[0]=0
			ulReg = 0x1a;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x10;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

			ulReg = 0x1b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfe;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x33)
		{
			// CR2C[6:5]=01
			ulReg = 0x2c;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0x9f;
			ulData |= 0x20;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if((argv[0] == 0x34)||(argv[0] == 0x35)||(argv[0] == 0x36))
		{
			// CR27[4]=0
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xef;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}
	// GPIO4X
	else if((argv[0] / 0x10) == 0x4)
	{
		if(argv[0] == 0x40)
		{
			// CR1B[3]=1
			ulReg = 0x1b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x08;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x41)
		{
			// CR1A[3:2]=10, CR27[4]=0
			ulReg = 0x1a;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf3;
			ulData |= 0x80;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xef;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x42)
		{
			// CR1B[2:1]=11, CR27[4]=0
			ulReg = 0x1b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf9;
			ulData |= 0x60;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xef;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x43)
		{
			// CR27[4]=0
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xef;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if((argv[0] == 0x44)||(argv[0] == 0x45))
		{
			// CR1B[6]=0, CR27[4]=0
			ulReg = 0x1b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xbf;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xef;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x46)
		{
			// CR27[5:4]=00
			ulReg = 0x27;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xcf;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

		}
		else if(argv[0] == 0x47)
		{
			// CR1B[7]=1
			ulReg = 0x1b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x80;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}
	// GPIO5X
	else if((argv[0] / 0x10) == 0x5)
	{
		if(argv[0] == 0x50)
		{
			// LDB CRE6[2]=0
			ulReg = 0xe6;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfb;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x51)
		{
			// CR2D[1]=0
			ulReg = 0x2d;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfd;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x52)
		{
			// LDB CRE6[1]=0
			ulReg = 0xe6;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfd;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x53)
		{
			// CR2D[0]=0
			ulReg = 0x2d;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfe;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x54)
		{
			// CR1D[3]=0
			ulReg = 0x1d;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x55)
		{
			// LDB CRE6[3]=0
			ulReg = 0xe6;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xf7;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if((argv[0] == 0x56)||(argv[0] == 0x57))
		{
			// Strapping by AMDPWR_EN or CR2F[5]
			// CR2F[5]=0
			ulReg = 0x2f;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xdf;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}
	// GPIO6X
	else if((argv[0] / 0x10) == 0x6)
	{
		// CR27[4]=0
		ulReg = 0x27;
		NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
		ulData &= 0xef;
		NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
	}
	// GPIO7X
	else if((argv[0] / 0x10) == 0x7)
	{
		if(argv[0] < 0x74)
		{
			// Strapping by TEST2_MODE_EN or CR2F[2]
			// CR2F[2]=0
			ulReg = 0x2f;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData &= 0xfb;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x74)
		{
			// CR2B[5]=1
			ulReg = 0x2b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x20;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x75)
		{
			// CR2B[6]=1
			ulReg = 0x2b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x40;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
		else if(argv[0] == 0x76)
		{
			// CR2B[7]=1
			ulReg = 0x2b;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
			ulData |= 0x80;
			NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
		}
	}

	// GPIO8X
	else if((argv[0] / 0x10) == 0x8)
	{
		// CR2A[7]=1
		ulReg = 0x2a;
		NCT6791D_GPIO_get_configure_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
		ulData |= 0x80;
		NCT6791D_GPIO_get_configure_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);
	}

	return 0;
}

int NCT6791D_GPIO_set_multifunction_t5h(int argc, unsigned long argv[])
{
	unsigned long ulReg;
	unsigned long ulData;
	unsigned long ulTempData, ulIO;
	int i;

	ulIO = argv[1];

	//Switch default function to GPIO
	NCT6791D_GPIO_switch_default_function_t5h(argc, argv);

	//Enable GPIO
	argv[1] = 1;
	NCT6791D_GPIO_set_enable_t5h(argc, argv);

	// Set multifunctions.
	ulReg = NCT6791D_GPIO_MULTIFUNC_REG_T5H[argv[0] / 0x10];
	ulTempData = NCT6791D_GPIO_MULTIFUNC_VALUE_T5H[argv[0] / 0x10];
	NCT6791D_GPIO_get_data_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);
	i = argv[0] % 0x10;
	ulData = ulData & (~(1 << i));
	ulTempData = ulTempData & (1 << i);
	ulData |= ulTempData;
	NCT6791D_GPIO_get_data_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

	NCT6791D_GPIO_set_push_pull_mode_function_t5h(argc, argv);

	//Reset GPIO input/output
	argv[1] = ulIO;
	NCT6791D_GPIO_set_IO_t5h(argc, argv);

	return 0;
}

int NCT6791D_GPIO_set_output_value_t5h(int argc, unsigned long argv[])
{
	unsigned long ulReg;
	unsigned long ulData;
	unsigned long ulTempData;
	int i;

	ulReg = NCT6791D_GPIO_DATA_REG_T5H[ argv[0] / 0x10 ];
	ulTempData = argv[1];

	NCT6791D_GPIO_get_data_t5h( NCT6791D_READ_FLAG_T5H, ulReg, &ulData, argc, argv);

	i = argv[0] % 0x10;

	ulData = ( ulData & (~(1 << i))) | (ulTempData << i);

	NCT6791D_GPIO_get_data_t5h( NCT6791D_WRITE_FLAG_T5H, ulReg, &ulData, argc, argv);

	return 0;
}

void led_control_t5h( char chLed, char chOn )
{
	unsigned long szArgv[6] = {0};

	NCT6791D_init_t5h();

	szArgv[1] = 0x1;
	szArgv[0] = 0x54;
	szArgv[2] = 'P';
	szArgv[3] = 0x0;
	szArgv[4] = 0x2e;
	szArgv[5] = 0x2f;
	NCT6791D_GPIO_set_multifunction_t5h(6, szArgv);
	szArgv[1] = 0x1;
	szArgv[0] = 0x71;
	szArgv[2] = 'P';
	szArgv[3] = 0x0;
	szArgv[4] = 0x2e;
	szArgv[5] = 0x2f;
	NCT6791D_GPIO_set_multifunction_t5h(6, szArgv);
	szArgv[1] = 0x1;
	szArgv[0] = 0x72;
	szArgv[2] = 'P';
	szArgv[3] = 0x0;
	szArgv[4] = 0x2e;
	szArgv[5] = 0x2f;
	NCT6791D_GPIO_set_multifunction_t5h(6, szArgv);
	szArgv[1] = 0x1;
	szArgv[0] = 0x73;
	szArgv[2] = 'P';
	szArgv[3] = 0x0;
	szArgv[4] = 0x2e;
	szArgv[5] = 0x2f;
	NCT6791D_GPIO_set_multifunction_t5h(6, szArgv);

	szArgv[1] = chOn; szArgv[0] = chLed; szArgv[2] = 'P';
	szArgv[3] = 0x0; szArgv[4] = 0x2e; szArgv[5] = 0x2f;

	NCT6791D_GPIO_set_output_value_t5h( 6, szArgv );

	NCT6791D_uninit_t5h();

	iopl( 0 );

	return;
}
#endif

#ifndef ARCH_ARM64
#include "NSA7135.h"
#include "lan_module.h"

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
 
#define NI140C          0
#define NI180C          1
#define NI140F          2
#define NI180F          3
#define NI180C4F4       4
#define NX140F          5
#define NI142C          6
#define NI142F          7
#define NI184C          8
#define NX142F          9
#define NI182C4F4       10
#define NX599F2         11
#define NX599F2B        12
#define	NX120F          13
#define NX121F          14
#define NI120F          15
#define NI121F          16
#define NI140CI210      17
#define NI142CI210      18
#define MGT1            19
#define MGT2            20
#define CTCK50G         21
#define NQ120F          22

#if 1
#define NI120C          23
#define NI121C          24
#define NI142_C2F2      25

#define NT140F          26
#define NT142F          27
#define NT140C          28
#define NT142C          29
#define NT120C          30
#define NT121C          31
#define NT120F          32
#define NT121F          33
#define NX120C          34
#define NX121C          35
#define NI140C2F2       36
#define NI141C2F2       37

#define LAE4000         38
#define LAE4020         39
#define LAE0400         40
#define LAE0420         41
#define NP120F          42
#define NP121F          43
#define NP140F          44
#define NP142F          45

#define LAI4400         46
#define LAI4420         47
#define LAI8000         48
#define LAI8040         49
#define LAI0800         50
#define LAE4400         51
#define LAE4420         52
#define LAE8000         53
#define LAE8040         54
#define LAE0800         55
#define NE180C4F4       56
#define NE182C4F4       57


#define LAI4401         58
#define LAI4421         59
#define LAI8001         60
#define LAI8041         61
#define LAI0801         62
#define LAE4401         63
#define LAE4421         64
#define LAE8001         65
#define LAE8041         66
#define LAE0801         67


#define LAI4405         68
#define LAI4425         69
#define LAI8005         70
#define LAI8045         71
#define LAI0805         72
#define LAE4405         73
#define LAE4425         74
#define LAE8005         75
#define LAE8045         76
#define LAE0805         77

#define NC220FIS4       78
#define NC220Q28M       79
#define NC220FMM3       80
#endif

#define SLOT_NONE	    99
#define SLOT_UNKNOWN	88



int bypass_num(int slot_n)
{
    int andy = 0;
    unsigned int *CRm = malloc(sizeof(int));
    char PCA9548_result = 0;
    SMBus_Port = Get_SMBus_Port();
    PCA9548_result = PCA9548_Setting(slot_n);
    if(PCA9548_result == 0x1)
        goto end;
end:
    PCA9548_Setting(slot_n);
    andy = Ct_I2CReadByte(LAN_MODULE_INDEX,SMBUS_OFFSET,CRm);
	free(CRm);
    //  printf("bypass = %x\n",andy);
    return andy;
}

int bypass_num_B(int slot)
{
	int andy;
	unsigned int *CRb = malloc(sizeof(int));
	char PCA9548_result = 0;
	SMBus_Port = Get_SMBus_Port();
	SMBus_Port &= 0x0580;
	PCA9548_result = PCA9548_Setting_B(slot);
	if(PCA9548_result == 0x1)
	goto end;
	PCA9548_Setting_B(slot);
	andy = Ct_I2CReadByte3(LAN_MODULE_INDEX,SMBUS_OFFSET,CRb);
end:
	free(CRb);
	return andy;
}

//uint8_t i2c_eep(uint8_t slot_n);
static uint32_t read_pci_config32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
	uint32_t addr;
	
	addr = (((bus << 5) | dev) << 3) | func;
	addr = (addr << 8) | (reg & ~3);
	outl(0x80000000 | addr , 0xCF8);
	return inl(0xCFC);
}


static uint8_t read_pci_config8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
        uint32_t addr;
        
        addr = (((bus << 5) | dev) << 3) | func;
	addr = (addr << 8) | (reg & ~3);
        outl(0x80000000 | addr , 0xCF8);
        return inb(0xCFC + (reg & 3));
}

uint8_t get_slot1_module_id(void)
{
	uint8_t	bus;
	/* get slot1 LAN module P2P controller Vendor/Device ID */
	bus = read_pci_config8(0, 1, 0, 0x19);
	return bus;
}

uint8_t get_slot1_module_id_B(void)
{
    uint8_t bus;
    /* get slot1 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0, 1, 1, 0x19);
    return bus;
}

uint8_t get_slot2_module_id(void)
{
    uint8_t bus;
    /* get slot2 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0, 0x02, 0x02, 0x19);
    return bus;
}

uint8_t get_slot2_module_id_B(void)
{
    uint8_t bus;
    /* get slot2 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0, 2, 3, 0x19);
    return bus;
}

uint8_t get_slot3_module_id(void)
{
    uint8_t bus;
    /* get slot3 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x80, 0x01, 0x0, 0x19);
    return bus;
}

uint8_t get_slot3_module_id_B(void)
{
    uint8_t bus;
    /* get slot3 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x80, 1, 1, 0x19);
    return bus;
}

uint8_t get_slot4_module_id(void)
{
    uint8_t bus;
    /* get slot4 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x80, 0x02, 0x02, 0x19);
    return bus;
}

uint8_t get_slot4_module_id_B(void)
{
    uint8_t bus;
    /* get slot4 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x80, 2, 3, 0x19);
    return bus;
}

uint8_t get_slot5_module_id(void)
{
    uint8_t         bus, bus_a,bus_b;
    uint32_t        vendid;
    /* get slot5 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x0, 0x02, 0, 0x19);
    return bus;
	vendid = read_pci_config32(bus, 0, 0, 0);
	if(vendid == 0x806c111d)
	{
		bus_a = read_pci_config8(bus, 0, 0, 0x19);
		bus_b = read_pci_config8(bus_a, 5, 0, 0x19);
		return bus_b;
	}
	else
	{
		return 0xff;
	}
}

uint8_t get_slot5_module_id_B(void)
{
    uint8_t bus,bus1,bus2;
    /* get slot5 LAN module P2P controller Vendor/Device ID */
    //bus = read_pci_config8(0, 3, 2, 0x19);
    //bus1 = read_pci_config8(bus, 0, 0, 0x19);
    bus2 = read_pci_config8(0, 2, 1, 0x19);
    return bus2;
}

uint8_t get_slot6_module_id(void)
{
    uint8_t         bus, bus_a,bus_b;
    uint32_t        vendid;
    /* get slot6 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0, 0x03, 0x02, 0x19);
    return bus;
    vendid = read_pci_config32(bus, 0, 0, 0);
    if(vendid == 0x862410B5)
    {
        bus_a = read_pci_config8(bus, 0, 0, 0x19);
        bus_b = read_pci_config8(bus_a, 6, 0, 0x19);
        return bus_b;
    }
    else
    {
        return 0xff;
    }
}

uint8_t get_slot6_module_id_B(void)
{
    uint8_t bus,bus1,bus2;
    /* get slot6 LAN module P2P controller Vendor/Device ID */
    //bus = read_pci_config8(0, 3, 2, 0x19);
    //bus1 = read_pci_config8(bus, 0, 0, 0x19);
    bus2 = read_pci_config8(0, 3, 3, 0x19);
    return bus2;
}

uint8_t get_slot7_module_id(void)
{
    uint8_t         bus, bus_a,bus_b;
    uint32_t        vendid;
    /* get slot7 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x80, 0x02, 0x0, 0x19);
    return bus;
    vendid = read_pci_config32(bus, 0, 0, 0);
    if(vendid == 0x862410B5)
    {
        bus_a = read_pci_config8(bus, 0, 0, 0x19);
        bus_b = read_pci_config8(bus_a, 8, 0, 0x19);
        return bus_b;
    }
    else
    {
        return 0xff;
    }
}

uint8_t get_slot7_module_id_B(void)
{
    uint8_t bus,bus1,bus2;
    /* get slot7 LAN module P2P controller Vendor/Device ID */
    //bus = read_pci_config8(0, 3, 2, 0x19);
    //bus1 = read_pci_config8(bus, 0, 0, 0x19);
    bus2 = read_pci_config8(0x80, 2, 1, 0x19);
    return bus2;
}

uint8_t get_slot8_module_id(void)
{
    uint8_t         bus, bus_a,bus_b;
    uint32_t        vendid;
    /* get slot8 LAN module P2P controller Vendor/Device ID */
    bus = read_pci_config8(0x80, 0x03, 0x02, 0x19);
    return bus;
    vendid = read_pci_config32(bus, 0, 0, 0);
    if(vendid == 0x862410B5)
    {
        bus_a = read_pci_config8(bus, 0, 0, 0x19);
        bus_b = read_pci_config8(bus_a, 9, 0, 0x19);
        return bus_b;
    }
    else
    {
        return 0xff;
    }
}

uint8_t get_slot8_module_id_B(void)
{
    uint8_t bus,bus1,bus2;
    /* get slot8 LAN module P2P controller Vendor/Device ID */
    //bus = read_pci_config8(0, 3, 2, 0x19);
    //bus1 = read_pci_config8(bus, 0, 0, 0x19);
    bus2 = read_pci_config8(0x80, 3, 3, 0x19);
    return bus2;
}

uint8_t get_module_id(uint8_t bus_num,int slot)
{
    uint8_t		bus, bus_a,bus_b,bus_c,bus_d;
    uint32_t	vendid,vendid_a,vendid_b;
    uint16_t        sys_vendid;
    /* check LAN module P2P controller Vendor/Device ID */
    vendid = read_pci_config32(bus_num, 0, 0, 0);
    //	printf("vendid = %x\n",vendid);
    if (vendid == 0xffffffff) return SLOT_NONE;
	printf( "[%s:%d] ==> vendid:0x%X\n", __FUNCTION__, __LINE__, vendid );

    if ( (vendid != 0x15338086) && (vendid != 0x15838086) && (vendid != 0x260812D8) 
		&& (vendid != 0x807B111D) && (vendid != 0x15228086) && (vendid != 0x15218086) 
		&& (vendid != 0x15728086) && (vendid != 0x10FB8086) && (vendid != 0x806C111D) 
		&& (vendid != 0x872410b5)
		)
		return SLOT_UNKNOWN;   //unknown

    if(vendid == 0x806C111D)
    {
        bus = read_pci_config8(bus_num, 0, 0, 0x19);
        bus_a = read_pci_config8(bus, 2, 0, 0x19);
        bus_b = read_pci_config8(bus, 3, 0, 0x19);
        vendid_a = read_pci_config32(bus_a, 0, 0, 0);
        vendid_b = read_pci_config32(bus_b, 0, 0, 0);
        if((vendid_a == 0x15218086) && (vendid_b == 0x15218086))
        {
            return NI180C;
        }
        if((vendid_a == 0x15228086) && (vendid_b == 0x15228086))
        {
            return NI180F;
        }
        if((vendid_a == 0x15228086) && (vendid_b == 0x15218086))
        {
            return NI180C4F4;
        }
    }
    if(vendid == 0x15728086)
    {
        vendid = read_pci_config32(bus_num, 0, 2, 0);
        if(vendid == 0x15728086)
        {
            return NX140F;
        }
        else
        {
            return NX120F;
        }
    }
    if(vendid == 0x15228086)
    {
        vendid = read_pci_config32(bus_num, 0, 0, 0);
        if(vendid == 0x15228086)
        {
            return NI140F;
        }
        else 
        {
            return NI120F;
        }
    }
    if(vendid == 0x15218086)
    {
        return NI140C;
    }
    if(vendid == 0x872410b5)
    {
        bus = read_pci_config8(bus_num, 0, 0, 0x19);
        bus_a = read_pci_config8(bus, 0, 0, 0x19);
        vendid = read_pci_config32(bus_a, 0, 0, 0);
        if(vendid == 0x04358086)
        {
            return CTCK50G;
        }
    }
    if(vendid == 0x15838086)
    {
        return NQ120F;
    }
    if(vendid == 0x10FB8086)
    {
        return NX599F2;
    }	
    if(vendid == 0x260812D8)
    {
        bus = read_pci_config8(bus_num, 0, 0, 0x19);
        bus_a = read_pci_config8(bus, 1, 0, 0x19);
        bus_b = read_pci_config8(bus, 2, 0, 0x19);
        bus_c = read_pci_config8(bus, 3, 0, 0x19);
        bus_d = read_pci_config8(bus, 4, 0, 0x19);
        vendid = read_pci_config32(bus_a, 0, 0, 0);
        if(vendid == 0x15338086)
        {
            return NI140CI210;
        }
    }

	return 0;
}

int read_portnum(int slot,int port,int busnum)
{
	if ( port == 0 )
	{
		printf("Slot%xPort%x: %.2x:00.0 \n",slot,port,busnum);
	}

	if ( port == 1 )
	{
		printf("Slot%xPort%x: %.2x:00.1 \n",slot,port,busnum);
	}

	if ( port == 2 )
	{
		printf("Slot%xPort%x: %.2x:00.2 \n",slot,port,busnum);
	}

    if ( port == 3 )
    {
		printf("Slot%xPort%x: %.2x:00.3 \n",slot,port,busnum);
    }

    if ( port == 4 )
    {
		printf("Slot%xPort%x: %.2x:00.0 \n",slot,port,busnum);
    }

    if ( port == 5 )
    {
		printf("Slot%xPort%x: %.2x:00.1 \n",slot,port,busnum);
    }

    if ( port == 6 )
    {
		printf("Slot%xPort%x: %.2x:00.2 \n",slot,port,busnum);
    }

    if ( port == 7 )
    {
		printf("Slot%xPort%x: %.2x:00.3 \n",slot,port,busnum);
    }

	return 0;
}

uint8_t get_module_id_B(uint8_t bus_num,uint8_t bus_num1,int slot,int port)
{
	uint8_t		bus, bus_a,bus_b,bus_c,bus_d;
	uint32_t	vendid,vendid1,vendid_a,vendid_b;
	uint16_t    sys_vendid;
	/* check LAN module P2P controller Vendor/Device ID */
	vendid = read_pci_config32(bus_num, 0, 0, 0);
	vendid1 = read_pci_config32(bus_num1, 0, 0 ,0);
	//	printf("vendid = %x\n",vendid);
	//	printf("vendid1 = %x\n",vendid1);
	if((vendid == 0xffffffff) && (vendid1 == 0xffffffff))
	{
		return SLOT_NONE;
	}
	else if(((vendid != 0x18241B21) && (vendid != 0x101715b3) && (vendid != 0x101315b3) && (vendid != 0x15928086)
			&& (vendid != 0x01078088) && (vendid != 0x01038088) && (vendid != 0x10018088) && (vendid != 0x861912D8) 
			&& (vendid != 0x122412D8) && (vendid != 0x15338086) && (vendid != 0x15838086) && (vendid != 0x260812D8)
			&& (vendid != 0x807B111D) && (vendid != 0x15228086) && (vendid != 0x15218086) && (vendid != 0x15728086)
			&& (vendid != 0x10FB8086) && (vendid != 0x806C111D) && (vendid != 0x872410b5) && (vendid != 0x15638086)
			&& (vendid != 0x150E8086) && (vendid != 0x150F8086) && (vendid != 0xc01612d8))
			&&((vendid1 != 0x18241B21) && (vendid1 != 0x101715b3) && (vendid1 != 0x101315b3) && (vendid1 != 0x15928086)
			&& (vendid1 != 0x01078088) && (vendid1 != 0x01038088) && (vendid1 != 0x10018088) && (vendid1 != 0x861912D8) 
			&& (vendid1 != 0x122412D8) && (vendid1 != 0x15338086) && (vendid1 != 0x15838086) && (vendid1 != 0x260812D8)
			&& (vendid1 != 0x807B111D) && (vendid1 != 0x15228086) && (vendid1 != 0x15218086) && (vendid1 != 0x15728086)
			&& (vendid1 != 0x10FB8086) && (vendid1 != 0x806C111D) && (vendid1 != 0x872410b5) && (vendid1 != 0x15638086)
			&& (vendid1 != 0x150E8086) && (vendid1 != 0x150F8086) && (vendid1 != 0xc01612d8)))
	{
		return SLOT_UNKNOWN;   //unknown
	}
	else if(vendid == 0x101715b3)
	{
	   if(port >=0 && port <= 1)
	   {
		   read_portnum(slot,port,bus_num);
	   }
	   else if(port == 9)
	   {
		 return NC220FMM3;
	   }
	   else
	   {
		  printf("slot%d only 2 port\n",slot);
	   }
	}
	else if(vendid == 0x101315b3)
	{
	   if(port >=0 && port <= 1)
	   {
		   read_portnum(slot,port,bus_num);
	   }
	   else if(port == 9)
		 return NC220Q28M;
	   else
	   {
		  printf("slot%d only 2 port\n",slot);
	   }
	}
	else if(vendid == 0x15928086)
	{
	   if(port >=0 && port <= 1)
	   {
		   read_portnum(slot,port,bus_num);
	   }
	   else if(port == 9)
	   {
		 return NC220FIS4;
	   }
	   else
	   {
		  printf("slot%d only 2 port\n",slot);
	   }
	}
	//old switch
	else if(vendid == 0x806C111D)
	{
		bus = read_pci_config8(bus_num, 0, 0, 0x19);
		bus_a = read_pci_config8(bus, 2, 0, 0x19);
		bus_b = read_pci_config8(bus, 3, 0, 0x19);
		vendid_a = read_pci_config32(bus_a, 0, 0, 0);
		vendid_b = read_pci_config32(bus_b, 0, 0, 0);
		if((vendid_a == 0x15218086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return NI180C;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		if((vendid_a == 0x15228086) && (vendid_b == 0x15228086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return NI180F;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		if((vendid_a == 0x15228086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return NI180C4F4;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
	}
	else if(vendid == 0x10018088)
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x10018088)
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NP140F;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
		else
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NP120F;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}	
		
	}
	//DIODE 2.0
	else if(vendid == 0x861912D8)
	{
		bus = read_pci_config8(bus_num, 0, 0, 0x19);
		bus_a = read_pci_config8(bus, 1, 0, 0x19);
		bus_b = read_pci_config8(bus, 3, 0, 0x19);
		vendid_a = read_pci_config32(bus_a, 0, 0, 0);
		vendid_b = read_pci_config32(bus_b, 0, 0, 0);
		if((vendid_a == 0x15218086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI8000;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x15228086) && (vendid_b == 0x15228086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI0800;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x15228086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI4400;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01038088) && (vendid_b == 0x01078088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return NE180C4F4;
				//NSFcard
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01078088) && (vendid_b == 0x01038088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE4400;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01038088) && (vendid_b == 0x01038088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE8000;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01078088) && (vendid_b == 0x01078088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE0800;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
	}
	//ASMEDIA 3.0
	else if(vendid == 0x18241B21)
	{
		bus = read_pci_config8(bus_num, 0, 0, 0x19);
		bus_a = read_pci_config8(bus, 0, 0, 0x19);
		bus_b = read_pci_config8(bus, 8, 0, 0x19);
		
		vendid_a = read_pci_config32(bus_a, 0, 0, 0);
		vendid_b = read_pci_config32(bus_b, 0, 0, 0);
		//printf("vendid = %x\n",vendid_a);
		//printf("vendid = %x\n",vendid_b);
		if((vendid_a == 0x15218086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI8001;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x15228086) && (vendid_b == 0x15228086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI0801;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x15228086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI4401;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01078088) && (vendid_b == 0x01038088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE4401;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01038088) && (vendid_b == 0x01038088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE8001;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01078088) && (vendid_b == 0x01078088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE0801;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
	}
	//DIODE 3.0
	else if(vendid == 0xc01612d8)
	{
		bus = read_pci_config8(bus_num, 0, 0, 0x19);
		bus_a = read_pci_config8(bus, 4, 0, 0x19);
		bus_b = read_pci_config8(bus, 6, 0, 0x19);

		vendid_a = read_pci_config32(bus_a, 0, 0, 0);
		vendid_b = read_pci_config32(bus_b, 0, 0, 0);
		if((vendid_a == 0x15218086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI8005;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x15228086) && (vendid_b == 0x15228086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI0805;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x15228086) && (vendid_b == 0x15218086))
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
				read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAI4405;
			}
			else
			{
				printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01078088) && (vendid_b == 0x01038088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE4405;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01038088) && (vendid_b == 0x01038088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE8005;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
		else if((vendid_a == 0x01078088) && (vendid_b == 0x01078088))
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_a);
			}
			else if((port >= 4) && (port <= 7))
			{
			    read_portnum(slot,port,bus_b);
			}
			else if(port == 9)
			{
				return LAE0805;
			}
			else
			{
			    printf("slot%x only 8 port \n",slot);
			}
		}
	}
	else if(vendid == 0x15638086)
	{
		if((port >= 0) && (port <2))
		{
			read_portnum(slot,port,bus_num);
		}
		else if(port == 9)
		{
			return NX120C;
		}
		else 
		{
			printf("slot%x only 2 port\n",slot);
		}
	}
	else if(vendid == 0x15728086)
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x15728086)
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NX140F;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
		else
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NX120F;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
	}
	else if(vendid == 0x150E8086)
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x150E8086)
		{
			if((port >= 0) && (port <= 3))
			{
					read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
					return NT140C;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
		else 
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NT120C;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
	}
	else if(vendid == 0x150F8086)
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x150F8086)
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NT140F;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
		else 
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NT120F;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
	}
	else if((vendid == 0x15228086) && (vendid1 == 0xffffffff))
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x15228086)
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NI140F;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
		else 
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NI120F;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
	}
	else if((vendid == 0x15218086) && (vendid1 == 0xffffffff))
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x15218086)
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NI140C;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
		else if(vendid == 0xffffffff)
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NI120C;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
		else 
		{
			if((port >= 0) && (port <= 3))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NI140C2F2;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
	}
	else if((vendid == 0x15228086) && (vendid1 == 0x15228086))
	{
		if((port >= 0) && (port <= 3))
		{
			read_portnum(slot,port,bus_num);
		}
		else if((port >= 4) && (port <= 7))
		{
			read_portnum(slot,port,bus_num1);
		}
		else if(port == 9)
		{
			return NI180F;
		}
		else
		{
			printf("slot%x only 8 port \n",slot);
		}
	}
	else if((vendid == 0x15218086) && (vendid1 == 0x15218086))
	{
		if((port >= 0) && (port <= 3))
		{
			read_portnum(slot,port,bus_num);
		}
		else if((port >= 4) && (port <= 7))
		{
			read_portnum(slot,port,bus_num1);
		}
		else if(port == 9)
		{
			return NI180C;
		}
		else
		{
			printf("slot%x only 8 port \n",slot);
		}
	}
	else if((vendid == 0x15228086) && (vendid1 == 0x15218086))
	{
		if((port >= 0) && (port <= 3))
		{
			read_portnum(slot,port,bus_num);
		}
		else if((port >= 4) && (port <= 7))
		{
			read_portnum(slot,port,bus_num1);
		}
		else if(port == 9)
		{
			return NI180C4F4;
		}
		else
		{
			printf("slot%x only 8 port \n",slot);
		}
	}
	else if(vendid == 0x872410b5)
	{
		bus = read_pci_config8(bus_num, 0, 0, 0x19);
		bus_a = read_pci_config8(bus, 0, 0, 0x19);
		vendid = read_pci_config32(bus_a, 0, 0, 0);
		if(vendid == 0x04358086)
		{
			if((port >= 0) && (port < 1))
			{
				read_portnum(slot,port,bus_a);
			}
			else if(port == 9)
			{
				return CTCK50G;
			}
			else
			{
				printf("slot%x only 1 port \n",slot);
			}
		}
	}
	else if(vendid == 0x15838086)
	{
		if((port >= 0) && (port <= 1))
		{
			read_portnum(slot,port,bus_num);
		}
		else if(port == 9)
		{
			return NQ120F;
		}
		else
		{
			printf("slot%x only 2 port \n",slot);
		}
	}
	else if((vendid == 0x10FB8086) || (vendid1 == 0x10FB8086))
	{
		if(vendid == 0x10FB8086)
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return NX599F2;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
		else if(vendid1 == 0x10FB8086)
		{
			if((port >= 0) && (port <= 1))
			{
				read_portnum(slot,port,bus_num1);
			}
			else if(port == 9)
			{
				return NX599F2;
			}
			else
			{
				printf("slot%x only 2 port \n",slot);
			}
		}
	}		
	else if(vendid == 0x260812D8)
	{
		bus = read_pci_config8(bus_num, 0, 0, 0x19);
		bus_a = read_pci_config8(bus, 1, 0, 0x19);
		bus_b = read_pci_config8(bus, 2, 0, 0x19);
		bus_c = read_pci_config8(bus, 3, 0, 0x19);
		bus_d = read_pci_config8(bus, 4, 0, 0x19);
		vendid = read_pci_config32(bus_a, 0, 0, 0);
		if(vendid == 0x15338086)
		{
			if(port == 0)
			{
				printf("Slot%xPort%x: %.2x:00.0 \n",slot,port,bus_a);
			}
			else if(port == 1)
			{
				printf("Slot%xPort%x: %.2x:00.0 \n",slot,port,bus_b);
			}
			else if(port == 2)
			{
				printf("Slot%xPort%x: %.2x:00.0 \n",slot,port,bus_c);
			}
			else if(port == 3)
			{
				printf("Slot%xPort%x: %.2x:00.0 \n",slot,port,bus_d);
			}
			else if(port == 9)
			{
				return NI140CI210;
			}
			else
			{
				printf("slot%x only 4 port \n",slot);
			}
		}
	}
	else if(vendid == 0x01038088)
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x01038088)
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return LAE4000;
			}
			else
			{
			    printf("slot%x only 4 port \n",slot);
			}			
		}
	}	
	else if(vendid == 0x01078088)
	{
		vendid = read_pci_config32(bus_num, 0, 2, 0);
		if(vendid == 0x01078088)
		{
			if((port >= 0) && (port <= 3))
			{
			    read_portnum(slot,port,bus_num);
			}
			else if(port == 9)
			{
				return LAE0400;
			}
			else
			{
			    printf("slot%x only 4 port \n",slot);
			}
		}
	}
	else
	{
		return SLOT_UNKNOWN;
	}
}

uint8_t modules_state(int slot,int port)
{
	uint8_t state;

    if(slot == 1)
    {
		state = get_module_id_B(get_slot1_module_id(),get_slot1_module_id_B(),slot,port);
    }
    else if(slot == 2)
    {
		state = get_module_id_B(get_slot2_module_id(),get_slot2_module_id_B(),slot,port);
    }
    else if(slot == 3)
    {
		state = get_module_id_B(get_slot3_module_id(),get_slot3_module_id_B(),slot,port);
    }
    else if(slot == 4)
    {
		state = get_module_id_B(get_slot4_module_id(),get_slot4_module_id_B(),slot,port);			 
    }
    else if(slot == 5)
    {
		state = get_module_id_B(get_slot5_module_id(),get_slot5_module_id_B(),slot,port);
    }
    else if(slot == 6)
    {
		state = get_module_id_B(get_slot6_module_id(),get_slot6_module_id_B(),slot,port);
    }
    else if(slot == 7)
    {
		state = get_module_id_B(get_slot7_module_id(),get_slot7_module_id_B(),slot,port);
    }
    else if(slot == 8)
    {
		state = get_module_id_B(get_slot8_module_id(),get_slot8_module_id_B(),slot,port);
    }

	return state;
}

int display_module_name_bypass(uint8_t lan_module,uint8_t bypass,int slot)
{
    int port_num = 0;

    if((lan_module == NI180C) && (bypass != 0))
    {
        printf("Slot%x: NI184C \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI180C) && (bypass == 0))
    {
        printf("Slot%x: NI180C \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI140C) && (bypass != 0))
    {
        printf("Slot%x: NI142C \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI140C) && (bypass == 0))
    {
        printf("Slot%x: NI140C \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI180F) && (bypass != 0))
    {
        printf("Slot%x: NI184F \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI180F) && (bypass == 0))
    {
        printf("Slot%x: NI180F \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI140F) && (bypass != 0))
    {
        printf("Slot%x: NI142F \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI140F) && (bypass == 0))
    {
        printf("Slot%x: NI140F \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI180C4F4) && (bypass != 0))
    {
        printf("Slot%x: NI182C4F4 \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI180C4F4) && (bypass == 0))
    {
        printf("Slot%x: NI180C4F4 \n",slot);
        port_num = 8;
    }
    else if((lan_module == NX140F) && (bypass != 0))
    {
        printf("Slot%x: NX142F \n",slot);
        port_num = 4;
    }
    else if((lan_module == NX140F) && (bypass == 0))
    {
        printf("Slot%x: NX140F \n",slot);
        port_num = 4;
    }
    else if((lan_module == NX120F) && (bypass != 0))
    {
        printf("Slot%x: NX121F \n",slot);
        port_num = 2;
    }
    else if((lan_module == NX120F) && (bypass == 0))
    {
        printf("Slot%x: NX120F \n",slot);
        port_num = 2;
    }
    else if((lan_module == NI120F) && (bypass != 0))
    {
        printf("Slot%x: NX121F \n",slot);
    }
    else if((lan_module == NI120F) && (bypass == 0))
    {
        printf("Slot%x: NI120F \n",slot);
    }
    else if((lan_module == NX599F2) && (bypass == 0))
    {
        printf("Slot%x: NX599F2 \n",slot);
    }
    else if((lan_module == NX599F2) && (bypass != 0))
    {
        printf("Slot%x: NX599F2B \n",slot);
    }
    else if((lan_module == NI140CI210) && (bypass == 0))
    {
        printf("Slot%x: NI140CI210 \n",slot);
        port_num = 8;
    }
    else if((lan_module == NI140CI210) && (bypass != 0))
    {
        printf("Slot%x: NI142CI210 \n",slot);
        port_num = 8;
    }
    else if((lan_module == NQ120F) && (bypass == 0))
    {
        printf("Slot%x: NQ120F \n",slot);
		port_num = 2;
    }
    else if((lan_module == NQ120F) && (bypass != 0))
    {
        printf("Slot%x: NQ121F \n",slot);
    }
    else if(lan_module == MGT1)
    {
        ;
    }
    else if(lan_module == CTCK50G)
    {
         printf("Slot%x: CTCK50G \n",slot);
    }
    else if(lan_module == SLOT_UNKNOWN)
    {
        printf("Unknown \n");
    }
    else if(lan_module == SLOT_NONE)
    {
        printf("None \n");
    } 
    else
    {
        printf("None \n");
    }
    
    printf("%s %d lan_module %d bypass %d slot %d port_num %d\n", __FUNCTION__, __LINE__, lan_module, bypass, slot, port_num);

    return port_num;
}

int display_module_name_bypass_B( uint8_t lan_module, uint8_t bypass, int sorm, int slot, int port )
{
	int port_num = 0;

	if((lan_module == NI140C2F2) && (bypass != 0))
	{
		printf("Slot%x: NI141C2F2 \n",slot);
	}
	else if((lan_module == NI140C2F2) && (bypass == 0))
	{
		printf("Slot%x: NI140C2F2 \n",slot);
	}
	else if((lan_module == NI180C) && (bypass != 0))
	{
		printf("Slot%x: NI184C \n",slot);
	}
    else if((lan_module == NI180C) && (bypass == 0))
    {
		printf("Slot%x: NI180C \n",slot);
    }
    else if((lan_module == NI142_C2F2))
    {
		printf("Slot%x: NI142-C2F2 \n",slot);
    }
    else if((lan_module == NT140C) && (bypass != 0))
    {
		printf("Slot%x: NT142C \n",slot);
    }
    else if((lan_module == NT140C) && (bypass == 0))
    {
		printf("Slot%x: NT140C \n",slot);
    }
    else if((lan_module == NI140C) && (bypass != 0))
    {
		printf("Slot%x: NI142C \n",slot);
    }
    else if((lan_module == NI140C) && (bypass == 0))
    {
		printf("Slot%x: NI140C \n",slot);
    }
	else if((lan_module == NT120C) && (bypass != 0))
    {
		printf("Slot%x: NT121C \n",slot);
    }
    else if((lan_module == NT120C) && (bypass == 0))
    {
		printf("Slot%x: NT120C \n",slot);
    }
	else if((lan_module == NT120F) && (bypass != 0))
    {
		printf("Slot%x: NT121F \n",slot);
    }
    else if((lan_module == NT120F) && (bypass == 0))
    {
		printf("Slot%x: NT120F \n",slot);
    }
	else if((lan_module == NI120C) && (bypass != 0))
    {
		printf("Slot%x: NI121C \n",slot);
    }
    else if((lan_module == NI120C) && (bypass == 0))
    {
		printf("Slot%x: NI120C \n",slot);
    }
    else if((lan_module == NI180F) && (bypass != 0))
    {
		printf("Slot%x: NI184F \n",slot);
    }
    else if((lan_module == NI180F) && (bypass == 0))
    {
		printf("Slot%x: NI180F \n",slot);
    }
    else if((lan_module == NT140F) && (bypass != 0) &&(sorm!=0x01)&&(sorm!=0x02))
    {
		printf("Slot%x: NT142F \n",slot);
    }
    else if((lan_module == NT140F) && (bypass != 0) &&(sorm==0x01))
    {
		printf("Slot%x: NT142FL \n",slot);
    }
    else if((lan_module == NT140F) && (bypass != 0) &&(sorm==0x02))
    {
		printf("Slot%x: NT142FS \n",slot);
    }
    else if((lan_module == NT140F) && (bypass == 0))
    {
		printf("Slot%x: NT140F \n",slot);
    }
    else if((lan_module == NI140F) && (bypass != 0) &&(sorm!=0x01)&&(sorm!=0x02))
    {
		printf("Slot%x: NI142F \n",slot);
    }
    else if((lan_module == NI140F) && (bypass != 0) &&(sorm==0x01))
    {
		printf("Slot%x: NI142FL \n",slot);
    }
    else if((lan_module == NI140F) && (bypass != 0) &&(sorm==0x02))
    {
		printf("Slot%x: NI142FS \n",slot);
    }
    else if((lan_module == NI140F) && (bypass == 0))
    {
		printf("Slot%x: NI140F \n",slot);
    }
    else if((lan_module == NI180C4F4) && (bypass != 0))
    {
		printf("Slot%x: NI182C4F4 \n",slot);
    }
    else if((lan_module == NI180C4F4) && (bypass == 0))
    {
		printf("Slot%x: NI180C4F4 \n",slot);
    }
    else if((lan_module == NX140F) && (bypass != 0)&&(sorm!=0x01)&&(sorm!=0x02))
    {
		printf("Slot%x: NX142F \n",slot);
    }
    else if((lan_module == NX140F) && (bypass == 0))
    {
		printf("Slot%x: NX140F \n",slot);
    }
    else if((lan_module == NX140F) && (bypass != 0) && (sorm == 0x01))
    {
		printf("Slot%x: NX142FL \n",slot);
    }
    else if((lan_module == NX140F) && (bypass != 0) && (sorm == 0x02))
    {
		printf("Slot%x: NX142FS \n",slot);
    }
    else if((lan_module == NX120C) && (bypass != 0))
    {
		printf("Slot%x: NX121C \n",slot);
    }
    else if((lan_module == NX120C) && (bypass == 0))
    {
		printf("Slot%x: NX120C \n",slot);
    }
    else if((lan_module == NX120F) && (bypass != 0)&&(sorm!=0x01)&&(sorm!=0x02))
    {
		printf("Slot%x: NX121F \n",slot);
    }
    else if((lan_module == NX120F) && (bypass == 0))
    {
		printf("Slot%x: NX120F \n",slot);
    }
    else if((lan_module == NX120F) && (bypass != 0) && (sorm == 0x01))
    {
		printf("Slot%x: NX121FL \n",slot);
    }
    else if((lan_module == NX120F) && (bypass != 0) && (sorm == 0x02))
    {
		printf("Slot%x: NX121FS \n",slot);
    }
    else if((lan_module == NI120F) && (bypass != 0))
    {
		printf("Slot%x: NI121F \n",slot);
    }
    else if((lan_module == NI120F) && (bypass == 0)&&(sorm!=0x01)&&(sorm!=0x02))
    {
		printf("Slot%x: NI120F \n",slot);
    }
    else if((lan_module == NI120F) && (bypass != 0) && (sorm == 0x01))
    {
		printf("Slot%x: NI121FL \n",slot);
    }
    else if((lan_module == NI120F) && (bypass != 0) && (sorm == 0x02))
    {
		printf("Slot%x: NI121FS \n",slot);
    }
    else if((lan_module == NX599F2) && (bypass == 0))
    {
		printf("Slot%x: NX599F2 \n",slot);
    }
    else if((lan_module == NX599F2) && (bypass != 0))
    {
		printf("Slot%x: NX599F2 \n",slot);
    }
    else if((lan_module == NI140CI210) && (bypass == 0))
    {
		printf("Slot%x: NI140CI210 \n",slot);
    }
    else if((lan_module == NI140CI210) && (bypass != 0))
    {
		printf("Slot%x: NI142CI210 \n",slot);
    }
    else if((lan_module == NQ120F) && (bypass == 0))
    {
		printf("Slot%x: NQ120F \n",slot);
    }
    else if((lan_module == NQ120F) && (bypass != 0) && (sorm == 0x01))
    {
		printf("Slot%x: NQ121FL \n",slot);
    }
	else if((lan_module == NQ120F) && (bypass != 0) && (sorm == 0x02))
    {
		printf("Slot%x: NQ121FS \n",slot);
    }
    else if((lan_module == NQ120F) && (bypass != 0))
    {
		printf("Slot%x: NQ121F \n",slot);
    }
	else if(lan_module == CTCK50G)
	{
		printf("Slot%x: CTCK50G \n",slot);
	}
	else if((lan_module == NP120F) && (bypass == 0))
	{
		printf("Slot%x: NP120F \n",slot);
	}
	else if((lan_module == NP120F) && (bypass != 0))
	{
		printf("Slot%x: NP121F \n",slot);
	}
	else if((lan_module == NP140F) && (bypass == 0))
	{
		printf("Slot%x: NP140F \n",slot);
	}
	else if((lan_module == NP140F) && (bypass != 0))
	{
		printf("Slot%x: NP142F \n",slot);
	}
	else if((lan_module == LAE0400) && (bypass == 0))
	{
		printf("Slot%x: LAE0400 \n",slot);
	}
	else if((lan_module == LAE0400) && (bypass != 0))
	{
		printf("Slot%x: LAE0420 \n",slot);
	}
	else if((lan_module == LAE4000) && (bypass == 0))
	{
		printf("Slot%x: LAE4000 \n",slot);
	}
	else if((lan_module == LAE4000) && (bypass != 0))
	{
		printf("Slot%x: LAE4020 \n",slot);
	}
//----------------------------------------------------------------------------------------------//
//LAXXXX0 DIODE 2.0-----------------------------------------------------------------------------//
//INTEL//
	else if((lan_module == LAI4400) && (bypass != 0))
	{
		port_num = 8;
	
		printf("Slot%x: LAI4420 \n",slot);
	}
	else if((lan_module == LAI4400) && (bypass == 0))
	{
		printf("Slot%x: LAI4400 \n",slot);
	}
	else if((lan_module == LAI8000) && (bypass != 0))
	{
		printf("Slot%x: LAI8040 \n",slot);
	}
	else if((lan_module == LAI8000) && (bypass == 0))
	{
		printf("Slot%x: LAI8000 \n",slot);
	}
	else if((lan_module == LAI0800) && (bypass == 0))
	{
		printf("Slot%x: LAI0800 \n",slot);
	}
//WANGXUN//
	else if((lan_module == NE180C4F4) && (bypass == 0))
	{
		printf("Slot%x: NE180C4F4 \n",slot);
	}
	else if((lan_module == NE180C4F4) && (bypass != 0))
	{
		printf("Slot%x: NE182C4F4 \n",slot);
	}
	else if((lan_module == LAE4400) && (bypass != 0))
	{
		printf("Slot%x: LAE4420 \n",slot);
	}
	else if((lan_module == LAE4400) && (bypass == 0))
	{
		printf("Slot%x: LAE4400 \n",slot);
	}
	else if((lan_module == LAE8000) && (bypass != 0))
	{
		printf("Slot%x: LAE8040 \n",slot);
	}
	else if((lan_module == LAE8000) && (bypass == 0))
	{
		printf("Slot%x: LAE8000 \n",slot);
	}
	else if((lan_module == LAE0800) && (bypass == 0))
	{
		printf("Slot%x: LAE0800 \n",slot);
	}
//----------------------------------------------------------------------------------------------//
//LAXXXX1 ASMEDIA 3.0-----------------------------------------------------------------------------//
//INTEL//
	else if((lan_module == LAI4401) && (bypass != 0))
	{
		printf("Slot%x: LAI4421 \n",slot);
	}
	else if((lan_module == LAI4401) && (bypass == 0))
	{
		printf("Slot%x: LAI4401 \n",slot);
	}
	else if((lan_module == LAI8001) && (bypass != 0))
	{
		printf("Slot%x: LAI8041 \n",slot);
	}
	else if((lan_module == LAI8001) && (bypass == 0))
	{
		printf("Slot%x: LAI8001 \n",slot);
	}
	else if((lan_module == LAI0801) && (bypass == 0))
	{
		printf("Slot%x: LAI0801 \n",slot);
	}
//WANGXUN//
	else if((lan_module == LAE4401) && (bypass != 0))
	{
		printf("Slot%x: LAE4421 \n",slot);
	}
	else if((lan_module == LAE4401) && (bypass == 0))
	{
		printf("Slot%x: LAE4401 \n",slot);
	}
	else if((lan_module == LAE8001) && (bypass != 0))
	{
		printf("Slot%x: LAE8041 \n",slot);
	}
	else if((lan_module == LAE8001) && (bypass == 0))
	{
		printf("Slot%x: LAE8001 \n",slot);
	}
	else if((lan_module == LAE0801) && (bypass == 0))
	{
		printf("Slot%x: LAE0801 \n",slot);
	}
//----------------------------------------------------------------------------------------------//
//LAXXXX5 DIODE 3.0--------------------------------------------------------------------------------//
//INTEL//
	else if((lan_module == LAI4405) && (bypass != 0))
	{
		printf("Slot%x: LAI4425 \n",slot);
	}
	else if((lan_module == LAI4405) && (bypass == 0))
	{
		printf("Slot%x: LAI4405 \n",slot);
	}
	else if((lan_module == LAI8005) && (bypass != 0))
	{
		printf("Slot%x: LAI8045 \n",slot);
	}
	else if((lan_module == LAI8005) && (bypass == 0))
	{
		printf("Slot%x: LAI8005 \n",slot);
	}
	else if((lan_module == LAI0805) && (bypass == 0))
	{
		printf("Slot%x: LAI0805 \n",slot);
	}
//WANGXUN//
	else if((lan_module == LAE4405) && (bypass != 0))
	{
		printf("Slot%x: LAE4425 \n",slot);
	}
	else if((lan_module == LAE4405) && (bypass == 0))
	{
		printf("Slot%x: LAE4405 \n",slot);
	}
	else if((lan_module == LAE8005) && (bypass != 0))
	{
		printf("Slot%x: LAE8045 \n",slot);
	}
	else if((lan_module == LAE8005) && (bypass == 0))
	{
		printf("Slot%x: LAE8005 \n",slot);
	}
	else if((lan_module == LAE0805) && (bypass == 0))
	{
		printf("Slot%x: LAE0805 \n",slot);
	}
//----------------------------------------------------------------------------------------------//
//100G 25G//
	else if((lan_module == NC220FIS4) && (bypass != 0))
	{
		printf("Slot%x: NC220FIS4 \n",slot);
	}
	else if((lan_module == NC220FIS4) && (bypass == 0))
	{
		printf("Slot%x: NC220FIS4 \n",slot);
	}
	else if(lan_module == NC220FMM3)
	{
		printf("Slot%x: NC220FMM3 \n",slot);
	}
	else if(lan_module == NC220Q28M)
	{
		printf("Slot%x: NC220Q28M \n",slot);
	}
//----------------------------------------------------------------------------------------------//		
	else if(lan_module == SLOT_NONE)
	{
		printf("Slot%x: None \n",slot);
		return 0;
	}
	else if(lan_module == SLOT_UNKNOWN)
	{
		printf("Slot%x: Unknown \n",slot);
		return 0;
	}
	else
	{
		printf("Slot%x: Unknown \n",slot);
		return 0;
	}

	printf("%s %d lan_module %d bypass %d slot %d port_num %d\n", __FUNCTION__, __LINE__, lan_module, bypass, slot, port_num);

	return port_num;
}

int x200_get_eth_num_by_cardid(int slot)
{
    int num = 0;

    iopl(3);

    switch(slot)
	{
	case 1:
        num = display_module_name_bypass(get_module_id(get_slot1_module_id(),slot),bypass_num(1),slot);
		break;
    case 2:
        num = display_module_name_bypass(get_module_id(get_slot2_module_id(),slot),bypass_num(2),slot);
		break;
    case 3:
        num = display_module_name_bypass(get_module_id(get_slot3_module_id(),slot),bypass_num(5),slot);
		break;
    case 4:
        num = display_module_name_bypass(get_module_id(get_slot4_module_id(),slot),bypass_num(6),slot);
        break;
    case 5:
        num = display_module_name_bypass(get_module_id(get_slot5_module_id(),slot),bypass_num(3),slot);
        break;
    case 6:
        num = display_module_name_bypass(get_module_id(get_slot6_module_id(),slot),bypass_num(4),slot);
        break;
    case 7:
        num = display_module_name_bypass(get_module_id(get_slot7_module_id(),slot),bypass_num(7),slot);
        break;
    case 8:
        num = display_module_name_bypass(get_module_id(get_slot8_module_id(),slot),bypass_num(8),slot);
		break;
	default:
		return 0;
	}

	if ( 0 == num )
	{
		if ( modules_state( slot, 9 ) == SLOT_NONE )
		{
			printf("Slot%x: None \n",slot);
		}
		else if(modules_state( slot, 9 ) == SLOT_UNKNOWN)
		{
			printf("Slot%x: Unknown \n",slot);
		}
		else if(slot == 1)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot1_module_id(),get_slot1_module_id_B(),slot,9),bypass_num_B(1),0,slot,9);
		}
		else if(slot == 2)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot2_module_id(),get_slot2_module_id_B(),slot,9),bypass_num_B(2),0,slot,9);
		}
		else if(slot == 3)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot3_module_id(),get_slot3_module_id_B(),slot,9),bypass_num_B(3),0,slot,9);
		}
		else if(slot == 4)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot4_module_id(),get_slot4_module_id_B(),slot,9),bypass_num_B(4),0,slot,9);
		}
		else if(slot == 5)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot5_module_id(),get_slot5_module_id_B(),slot,9),bypass_num_B(5),0,slot,9);
		}
		else if(slot == 6)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot6_module_id(),get_slot6_module_id_B(),slot,9),bypass_num_B(6),0,slot,9);
		}
		else if(slot == 7)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot7_module_id(),get_slot7_module_id_B(),slot,9),bypass_num_B(7),0,slot,9);
		}
		else if(slot == 8)
		{
			num = display_module_name_bypass_B(get_module_id_B(get_slot8_module_id(),get_slot8_module_id_B(),slot,9),bypass_num_B(8),0,slot,9);
		}
		else
		{
			printf("slot setup error \n");
		}
	}

	return num;
}
#endif

#ifdef ARCH_ARM64
void get_dev_type_by_cmd( char* pDevType, int iMaxLen )
{
	FILE* fp		  = NULL;
	char  line[256]   = { 0 };

	do
	{
		// chuangshi device
		fp = fopen("/proc/device-tree/model", "r");

		if ( fp )
		{
			fgets( line, sizeof( line ) - 1, fp );

			fclose( fp ); fp = NULL;
		}

		if ( !strncmp( line, "NH10", sizeof( "NH10" ) - 1 ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NH10" );
		else if ( !strncmp( line, "NH31", sizeof( "NH31" ) - 1 ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NH31" );
		else if ( !strncmp( line, "NZ02", sizeof( "NZ02" ) - 1 ) || !strncmp( line, "NZ10", sizeof( "NZ10" ) - 1 ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NZ02" );
		else if ( strstr( line, "NM59" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NM59" );
		else if ( strstr( line, "NM15" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NM15" );
		else if ( strstr( line, "NM32" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NM32" );
		else if ( strstr( line, "NT03" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NT03" );
		else if ( strstr( line, "NT06" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NT06" );
		else if ( strstr( line, "NT09" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NT09" );
		else if ( strstr( line, "NT13" ) )
			snprintf( pDevType, iMaxLen, "%s", "CS-NT13" );

		if ( pDevType[0] )
		{
			break;
		}

		fp = fopen( "/sys/devices/virtual/dmi/id/board_name", "r" );
		
		if ( fp )
		{
			fgets( line, sizeof( line ) - 1, fp );
		
			fclose( fp ); fp = NULL;
		}

		// leyan, royaltech
		if ( !strncmp( line, "RIS-5172-8", sizeof( "RIS-5172-8" ) - 1 ) )
		{
			snprintf( pDevType, iMaxLen, "%s", "LEYAN-RIS5172" );
		}

		if ( pDevType[0] )
		{
			break;
		}

		fp = fopen( "/sys/devices/virtual/dmi/id/product_name", "r" );
		
		if ( fp )
		{
			fgets( line, sizeof( line ) - 1, fp );
		
			fclose( fp ); fp = NULL;
		}

		// qiyang
		if ( !strncmp( line, "T7H", sizeof( "T7H" ) - 1 ) )
		{
			snprintf( pDevType, iMaxLen, "%s", "TOPSEC-T7H-QIYANG-C6248R-1" );
		}
		else if ( !strncmp( line, "RIS-297", sizeof( "RIS-297" ) - 1 ) )
		{
			snprintf( pDevType, iMaxLen, "%s", "LEYAN-RIS297" );
		}

		if ( pDevType[0] )
		{
			break;
		}
	} while ( 0 );

	return;
}
#else
void get_dev_type_by_cmd( char* pDevType, int iMaxLen )
{
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
					snprintf( pDevType, iMaxLen, "%s", "CS-NR01" );
				}
				else if ( !strcmp( p, "NK03\n" ) )
				{
					snprintf( pDevType, iMaxLen, "%s", "CS-NK03" );
				}
				else if ( !strcmp( p, "NK02\n" ) )
				{
					snprintf( pDevType, iMaxLen, "%s", "CS-NK02" );
				}
				else if ( !strcmp( p, "NK06\n" ) )
				{
					snprintf( pDevType, iMaxLen, "%s", "CS-NK06" );
				}
				else if ( !strcmp( p, "NK05\n" ) )
				{
					snprintf( pDevType, iMaxLen, "%s", "CS-NK05" );
				}
				else if ( !strcmp( p, "NK08\n" ) )
				{
					snprintf( pDevType, iMaxLen, "%s", "CS-NK08" );
				}
			}
		}

		pclose( fdmi );
	}

	return;
}
#endif

#define FIRMWARE_FILE "/usr/private/Firmware_Board_Type"

void get_dev_type( char* pDevType, int iMaxLen )
{
	FILE *fp = NULL;
	char szLine[256] = {0};

	// open file
	fp = fopen( FIRMWARE_FILE, "r" );

	if ( fp == NULL )
	{
		printf( "[%s:%d] ==> Error, open file [%s] failed, error:%d,%s.", __FUNCTION__, __LINE__, FIRMWARE_FILE, errno, strerror( errno ) );

		return;
	}

	// read one line
	if ( fgets( szLine, sizeof( szLine ), fp ) == NULL )
	{
		printf( "[%s:%d] ==> Error, read file [%s] failed, error:%d,%s.", __FUNCTION__, __LINE__, FIRMWARE_FILE, errno, strerror( errno ) );

		fclose( fp );

		fp  = NULL;

		return;
	}

	fclose( fp );

	fp = NULL;

	char* hardware_model = strtok( szLine, ":" );

	if ( hardware_model )
	{
		snprintf( pDevType, iMaxLen, "%s", hardware_model );
	}

	return;
}

void get_dev_type_interface( char* pDevType, int iMaxLen )
{
	if ( pDevType && iMaxLen > 0 )
	{
		get_dev_type_by_cmd( pDevType, iMaxLen );

		if ( !pDevType[0] )
			get_dev_type( pDevType, iMaxLen );
	}

	return;
}

int is_user_define_interface()
{
	int iManInter = 0;
	char szTemp[128] = {0};
	char szLine[256] = {0};

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

int get_product_info( char* pRet, int iMax )
{
	FILE* fp = fopen( "/usr/private/product_info", "r" );

	if ( fp )
	{
		fgets( pRet, iMax, fp );

		fclose( fp );
	}

	return 0;
}

int is_old_bus_format()
{
	int iRet = 0;
	char szLine[256] = {0};

	if ( F_OK == access( ACE_VPP_PORT_MAP_FILE, F_OK ) )
	{
		FILE* fVpp = fopen( ACE_VPP_PORT_MAP_FILE, "r" );

		if ( fVpp )
		{
			while ( fgets( szLine, sizeof( szLine ) - 1, fVpp ) != NULL  )
			{
				if ( strstr( szLine, "ifAutoCfgMode" ) )
				{
					continue;
				}

				if ( strstr( szLine, "/" ) && ( 0 == strncmp( szLine, "enp", strlen( "enp" ) ) ) )
				{
					iRet = 1;
				
					break;
				}
			}

			fclose( fVpp ); fVpp = NULL;
		}
	}

	return iRet;
}

void get_ext_i2c_address( char* pFilter, char* pI2c1, char* pI2c2, int iIndex1, int iIndex2 )
{
	int i = 0;
	int iIndex = 0;
	char szRet[128] = {0};
	char szCmd[256] = {0};
	int iI2c1 = -1, iI2c2 = -1;

	if ( pFilter && pI2c1 )
	{
		snprintf( szCmd, sizeof( szCmd ), "ls -l /sys/bus/i2c/devices/ | grep '%s' | awk '{print $11}' | awk -F'/' '{print $8}'", pFilter );

		FILE* fp = popen( szCmd, "r" );

		if ( fp )
		{
			if ( 0 != fgets( szRet, sizeof( szRet ) - 1, fp ) )
			{
				char* pEnd = strchr( szRet, '\r' );

				if ( pEnd )
				{
					*pEnd = 0;
				}

				pEnd = strchr( szRet, '\n' );

				if ( pEnd )
				{
					*pEnd = 0;
				}
			}

			pclose( fp ); fp = NULL;

			if ( strlen( szRet ) )
			{
				for ( i=1; i<=100; i++ )
				{
					snprintf( szCmd, sizeof( szCmd ), "/sys/bus/i2c/devices/%s/i2c-%d", szRet, i );

					if ( F_OK == access( szCmd, F_OK ) )
					{
						if ( -1 == iI2c1 )
						{
							if ( iIndex == iIndex1 )
								iI2c1 = i;
						}

						if ( pI2c2 )
						{
							if ( -1 == iI2c2 )
							{
								if ( iIndex == iIndex2 )
									iI2c2 = i;
							}
						}

						if ( -1 != iI2c1 && !pI2c2 )
						{
							break;
						}
						else if ( pI2c2 )
						{
							if ( ( -1 != iI2c2 ) && ( -1 != iI2c1 ) )
							{
								break;
							}
						}

						iIndex++;
					}
				}
			}
		}
	}

	if ( pI2c1 && ( -1 != iI2c1 ) )
	{
		sprintf( pI2c1, "%d", iI2c1 );
	}

	if ( pI2c2 && ( -1 != iI2c2 ) )
	{
		sprintf( pI2c2, "%d", iI2c2 );
	}

	return;
}

int changshi_ext_get_card_type( char* pI2cAddr )
{
	int iRet = 0;

	if ( pI2cAddr )
	{
		char szCmd[128] = {0};
		char szLine[128] = {0};
		const char* pCmd = "/usr/bin/i2cget";

		if ( F_OK != access( pCmd, F_OK ) )
		{
			pCmd = "/bin/i2cget";
		}

		// i2cget -f -y 0x00 0x57 0xa0 => 0x02
		// i2cget -f -y 0x00 0x57 0xa1 => 0x0X
		snprintf( szCmd, sizeof( szCmd ) - 1, "%s -f -y %s 0x57 0xa1", pCmd, pI2cAddr );

		FILE* fpHpcType = popen( szCmd, "r" );

		if ( fpHpcType )
		{
			memset( szLine, 0x00, sizeof( szLine ) );

			while ( 0 != fgets( szLine, sizeof( szLine ), fpHpcType ) )
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

int get_chuangshi_slot_type( char* pArgument )
{
	int iRet = 0;
	char szCmd[256] = {0};
	char szLine[256] = {0};
	const char* pCmd = "/usr/bin/i2cdump";

	if ( F_OK != access( pCmd, F_OK ) )
	{
		pCmd = "/bin/i2cdump";
	}

	snprintf( szCmd, sizeof( szCmd ) - 1, "%s %s", pCmd, pArgument );

	FILE* fpHpcType = popen( szCmd, "r" );

	if ( fpHpcType )
	{
		memset( szLine, 0x00, sizeof( szLine ) );

		while ( 0 != fgets( szLine, sizeof( szLine ), fpHpcType ) )
		{
			szLine[4] = 0;

			if ( strncmp( szLine, "XXXX", 4 ) )
			{
				char szValue[8] = {0};

				szValue[0] = '0';
				szValue[1] = 'x';

				strcat( szValue, szLine );

				int iHpcType = strtoul( szValue, NULL, 16 );

				if ( iHpcType > 0 )
				{
					iRet = iHpcType;

					break;
				}
			}

			break;
		}

		pclose( fpHpcType );
	}

	return iRet;
}

void chuangshi_get_slot_info( int iDevType, char* pFilter, int* pSlot1, int* pSlot2, int* piCount )
{
	int i = 0;
	int iSlotIndex = 0;
	char szRet[128] = {0};
	char szCmd[256] = {0};
	int iSlots[2];

	memset( iSlots, 0, sizeof( iSlots ) );

	if ( pFilter && pSlot1 && pSlot2 && piCount )
	{
		snprintf( szCmd, sizeof( szCmd ), "ls -l /sys/bus/i2c/devices/ | grep '%s' | awk '{print $11}' | awk -F'/' '{print $8}'", pFilter );

		FILE* fp = popen( szCmd, "r" );

		if ( fp )
		{
			if ( 0 != fgets( szRet, sizeof( szRet ) - 1, fp ) )
			{
				char* pEnd = strchr( szRet, '\r' );

				if ( pEnd )
				{
					*pEnd = 0;
				}

				pEnd = strchr( szRet, '\n' );

				if ( pEnd )
				{
					*pEnd = 0;
				}
			}

			pclose( fp ); fp = NULL;

			if ( strlen( szRet ) )
			{
				for ( i=1; i<=100; i++ )
				{
					snprintf( szCmd, sizeof( szCmd ), "/sys/bus/i2c/devices/%s/i2c-%d", szRet, i );

					if ( F_OK == access( szCmd, F_OK ) )
					{
						snprintf( szCmd, sizeof( szCmd ), "-f -y %d 0x57 b | grep '^a0' | awk '{print $3$4}'", i );

						int iSlotType = get_chuangshi_slot_type( szCmd );

						if ( iSlotType )
						{
							iSlots[iSlotIndex] = iSlotType; iSlotIndex++;
						}

						if ( iSlotIndex >= 2 )
						{
							break;
						}
					}
				}
			}
		}
	}

	*piCount = iSlotIndex;

	if ( 1 == iSlotIndex )
	{
		*pSlot1 = iSlots[0];

		*pSlot2 = iSlots[0];
	}
	else if ( 2 == iSlotIndex )
	{
		*pSlot1 = iSlots[1];

		*pSlot2 = iSlots[0];
	}

	return;
}

#if defined(CS_NXP_PLATFORM) || defined(CS_M3720_PLATFORM)
int nxp_platform_no_get(void)
{
	FILE* fp          = NULL;
	char  line[256]   = { 0 };
	char  tmpstr[256] = { 0 };

	fp = fopen("/proc/device-tree/model", "r");

	if ( fp )
	{
		fgets( line, sizeof( line ) - 1, fp );

		fclose( fp );
	}

	if (!strncmp(line, "LS1046A NH15P Board", sizeof("LS1046A NH15P Board")-1))
		return NXP_PLATFORM_CS_NH15P;
	else if (!strncmp(line, "LS1043A NH12 Board", sizeof("LS1043A NH12 Board")-1))
		return NXP_PLATFORM_CS_NH12;
	else if ( !strncmp( line, "NH10", sizeof( "NH10" ) - 1 ) )
		return NXP_PLATFORM_CS_NH10;
	else if ( !strncmp( line, "NH31", sizeof( "NH31" ) - 1 ) )
		return NXP_PLATFORM_CS_NH31;
	else if ( !strncmp( line, "NZ02", sizeof( "NZ02" ) - 1 ) || !strncmp( line, "NZ10", sizeof( "NZ10" ) - 1 ) )
		return NXP_PLATFORM_CS_NZ02;
	else if ( strstr( line, "NM59" ) )
		return NXP_PLATFORM_CS_NM59;
	else if ( strstr( line, "NM32" ) )
		return NXP_PLATFORM_CS_NM32;
	else if ( strstr( line, "NM15" ) )
		return NXP_PLATFORM_CS_NM15;

	return NXP_PLATFORM_CS;
}
#endif

#if defined(MT_RK_PLATFORM)
int rk_platform_no_get(void)
{
	FILE* fp          = NULL;
	char  line[256]   = { 0 };
	char  tmpstr[256] = { 0 };

	fp = fopen("/proc/device-tree/model", "r");

	if ( fp )
	{
		fgets( line, sizeof( line ) - 1, fp );

		fclose( fp );
	}

	if ( strstr( line, "RK3568 EVB1" ) )
		return RK_PLATFORM_MT_3568;

	return RK_PLATFORM_MT;
}
#endif

int get_mac_address( const char* interface_name, unsigned char* mac )
{
	int sockfd;
	struct ifreq ifr;

	sockfd = socket( AF_INET, SOCK_DGRAM, 0 );

	if ( sockfd < 0 )
	{
		printf( "%s:%d, Error, call funtion [socket] failed, error:%d,%s, interface:%s.", __FUNCTION__, __LINE__, errno, strerror( errno ), interface_name );

		return -1;
	}

	memset( &ifr, 0, sizeof( ifr ) );
	strncpy( ifr.ifr_name, interface_name, IFNAMSIZ - 1 );

	if ( ioctl( sockfd, SIOCGIFHWADDR, &ifr ) < 0 )
	{
		printf( "%s:%d, Error, call funtion [ioctl] failed, error:%d,%s, interface:%s.", __FUNCTION__, __LINE__, errno, strerror( errno ), interface_name );

		close( sockfd );

		return -1;
	}

	close( sockfd );

	memcpy( mac, ifr.ifr_hwaddr.sa_data, 6 );

	return 0;
}

int set_mac_address( const char* interface_name, const unsigned char* new_mac )
{
    struct ifreq ifr;
    int sockfd;

    sockfd = socket( AF_INET, SOCK_DGRAM, 0 );

    if ( sockfd < 0 )
	{
        printf( "%s:%d, Error, call funtion [socket] failed, error:%d,%s, interface:%s.", __FUNCTION__, __LINE__, errno, strerror( errno ), interface_name );

        return -1;
    }

    memset( &ifr, 0, sizeof( ifr ) );
    strncpy( ifr.ifr_name, interface_name, IFNAMSIZ - 1 );

    memcpy( ifr.ifr_hwaddr.sa_data, new_mac, 6 );

    if ( ioctl( sockfd, SIOCSIFHWADDR, &ifr ) < 0 )
	{
        printf( "%s:%d, Error, call funtion [ioctl] failed, error:%d,%s, interface:%s.", __FUNCTION__, __LINE__, errno, strerror( errno ), interface_name );

        close( sockfd );

        return -1;
    }

    close( sockfd );

    return 0;
}

static int changshi_get_card_type_by( char* pI2cAddr, char* pReg )
{
	int iRet = 0;

	if ( pI2cAddr && pReg )
	{
		char szCmd[128] = {0};
		char szLine[128] = {0};
		const char* pCmd = "/usr/bin/i2cget";

		if ( F_OK != access( pCmd, F_OK ) )
		{
			pCmd = "/bin/i2cget";
		}

		// i2cget -f -y 0x00 0x57 0xa0 => 0x02
		// i2cget -f -y 0x00 0x57 0xa1 => 0x0X
		snprintf( szCmd, sizeof( szCmd ) - 1, "%s -f -y %s %s 0xa1", pCmd, pI2cAddr, pReg );

		FILE* fpHpcType = popen( szCmd, "r" );

		if ( fpHpcType )
		{
			memset( szLine, 0x00, sizeof( szLine ) );

			while ( 0 != fgets( szLine, sizeof( szLine ), fpHpcType ) )
			{
				if ( strstr( szLine, "Error" ) )
				{
					continue;
				}
			
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

int changshi_get_card_type_by_i2c( int iDevType, char* pI2cFeature, int* iRetSlot )
{
	char pi2c1[16] = {0};
	char pi2c2[16] = {0};

	// NXP_PLATFORM_CS_NH10 || NXP_PLATFORM_CS_NZ02
	if ( ( 5 == iDevType ) || ( 7 == iDevType ) )
	{
		get_ext_i2c_address( pI2cFeature, pi2c1, pi2c2, 4, 5 );

		// slot1
		if ( pi2c1[0] )
		{
			iRetSlot[0] = changshi_get_card_type_by( pi2c1, "0x57" );
		}

		// slot2
		if ( pi2c2[0] )
		{
			iRetSlot[1] = changshi_get_card_type_by( pi2c2, "0x57" );
		}
	}
	// NXP_PLATFORM_CS_NH31
	else if ( 6 == iDevType )
	{
		// slot1
		{
			system( "/usr/bin/i2cset -f -y 0x2 0x70 1 1" );

			iRetSlot[0] = changshi_get_card_type_by( "0x02", "0x57" );
		}

		// slot2
		{
			system( "/usr/bin/i2cset -f -y 0x2 0x70 2 2" );

			iRetSlot[1] = changshi_get_card_type_by( "0x02", "0x57" );
		}
	}

	return 0;
}

