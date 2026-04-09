#define D_OK 0
#define D_NO 1

#define SMBUS_BASE	0x8000FB20
#define PCI_INDEX 	0xCF8
#define PCI_DATA	0xCFC

#define LAN_MODULE_INDEX 0xAE

//=== VOUT_MODE ===//
#define SMBUS_OFFSET 0x00

unsigned int Get_SMBus_Port();
void Dealy5ms();
void Check_Data();
char CT_Chk_SMBus_Ready();
int Ct_I2CReadByte(unsigned int ID,unsigned int index,unsigned int *data);
int PCA9548_Setting(unsigned int Slot);
//uint8_t bypass_eeprom(uint8_t);

int InitAPI(void);

#if 1
int PCA9548_Setting_B(unsigned int Slot);
#endif

