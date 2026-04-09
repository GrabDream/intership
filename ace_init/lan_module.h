#include <unistd.h>
#include <stdio.h>
#ifndef ARCH_ARM64
#include <sys/io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "NSA7135.h"

unsigned int SMBus_Port;
unsigned char Slot = 0;
unsigned int slave=0;
char p1,p2,p3,l1,l2,l3;		//p1,p2,p3 are the PCA9548's address. l1,l2,l3 are the parametets to decide the path.



int InitAPI(void)
{
	if ( iopl(3) < 0 )
	{
		return(D_NO);
	}

	return(D_OK);
}

unsigned int Get_SMBus_Port()
{
	unsigned int B_SMBUS;
	outl(SMBUS_BASE,PCI_INDEX);
	B_SMBUS = inl(PCI_DATA);
	return B_SMBUS; 		//return the base address
}


/****************************/
/*******SMBus Control********/
/****************************/

void Delay5ms()
{
	usleep(5000);
}

void Check_Data()
{
    int i;
    unsigned int c;

    for (i=0;i<6;i++){
        c = inb(SMBus_Port);
        if (c != 0) break;
    }

}

char CT_Chk_SMBus_Ready()
{
    unsigned int c;
    int i;

    for (i=0;i<0x800;i++){
        c = inb(SMBus_Port);		//get status reg0
        Check_Data();
        outb(c,SMBus_Port);			//clear status bit

        if (c & 2) goto Clear_final;
        if ((c & ~0x40)==0) goto Clear_final;
        if (c & 0x4) goto SMBus_Err;
    }

SMBus_Err:
        return 0;
Clear_final:
        return 1;
}

int PCA9548_Setting(unsigned int Slot)
{
	switch(Slot){
		case 1:
			p1=0xea;
			p2=0xe8;
			p3=0xe0;
			l1=0x1;
			l2=0x1;
			l3=0x1;
			return 0x0;
		case 2:
			p1=0xea;
			p2=0xe8;
			p3=0xe2;
			l1=0x1;
			l2=0x2;
			l3=0x1;
			return 0x0;
		case 5:
			p1=0xea;
			p2=0xe8;
			p3=0xe4;
			l1=0x1;
			l2=0x4;
			l3=0x1;
			return 0x0;
		case 6:
			p1=0xea;
			p2=0xe8;
			p3=0xe6;
			l1=0x1;
			l2=0x8;
			l3=0x1;
			return 0x0;
		//lan modules on the top.
		case 3:
			p1=0xea;
			p2=0xee;
			p3=0xe4;
			l1=0x2;
			l2=0x4;
			l3=0x1;
			return 0x0;
		case 4:
			p1=0xea;
			p2=0xee;
			p3=0xe6;
			l1=0x2;
			l2=0x8;
			l3=0x1;
			return 0x0;
		case 7:
			p1=0xea;
			p2=0xee;
			p3=0xe2;
			l1=0x2;
			l2=0x2;
			l3=0x1;
			return 0x0;
		case 8:
			p1=0xea;
			p2=0xee;
			p3=0xe0;
			l1=0x2;
			l2=0x1;
			l3=0x1;
			return 0x0;

		default:
//			printf("Key-in the wrong number.\n");
			return 0x1;
	}
}

int Ct_I2CReadByte(unsigned int ID,unsigned int index,unsigned int *data) //ID= slave addr, index = offset
{
    int i=0;
    SMBus_Port &= 0xfffe;
    CT_Chk_SMBus_Ready();
    outb(l1,SMBus_Port+5);			//data0
    Delay5ms();
    Delay5ms();
    outb(p1,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 1 = set read bit
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);

    CT_Chk_SMBus_Ready();
    outb(l2,SMBus_Port+5);			//data0
    Delay5ms();
    Delay5ms();
    outb(p2,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 1 = set read bit
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);


    CT_Chk_SMBus_Ready();
    outb(l3,SMBus_Port+5);			//data0
    Delay5ms();
    Delay5ms();
    outb(p3,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 1 = set read bit
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);
//    printf("The lan module is\t");
	int j;
  	  for (i=22; i <= 23; i++)
	{
		j = 0;
		CT_Chk_SMBus_Ready();
		outb(ID+1,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 1 = set read bit
		Delay5ms();
		Delay5ms();
		outb(i,SMBus_Port+3);			//set the start address to read.
		Delay5ms();
		Delay5ms();
		outb(0x48,SMBus_Port+2);
		j = inb(SMBus_Port+5);
		if(j != 0xffffffff);
		j = j & 0x0f;
	}
    Delay5ms();
    Delay5ms();
    Check_Data();
    //printf("%x",j);
    return j;
}

#if 1
int PCA9548_Setting_B(unsigned int Slot)
{
	switch(Slot){
		case 1:
			p1=0xea;
			p2=0xe8;
			p3=0xe0;
			l1=0x1;
			l2=0x1;
			l3=0x1;
			return 0x0;
		case 2:
			p1=0xea;
			p2=0xe8;
			p3=0xe2;
			l1=0x1;
			l2=0x2;
			l3=0x1;
			return 0x0;
		case 3:
			p1=0xea;
			p2=0xe8;
			p3=0xe4;
			l1=0x1;
			l2=0x4;
			l3=0x1;
			return 0x0;
		case 4:
			p1=0xea;
			p2=0xe8;
			p3=0xe6;
			l1=0x1;
			l2=0x8;
			l3=0x1;
			return 0x0;
		//lan modules on the top.
		case 5:
			p1=0xea;
			p2=0xee;
			p3=0xe4;
			l1=0x2;
			l2=0x4;
			l3=0x1;
			return 0x0;
		case 6:
			p1=0xea;
			p2=0xee;
			p3=0xe6;
			l1=0x2;
			l2=0x8;
			l3=0x1;
			return 0x0;
		case 7:
			p1=0xea;
			p2=0xee;
			p3=0xe2;
			l1=0x2;
			l2=0x2;
			l3=0x1;
			return 0x0;
		case 8:
			p1=0xea;
			p2=0xee;
			p3=0xe0;
			l1=0x2;
			l2=0x1;
			l3=0x1;
			return 0x0;

		default:
//			printf("Key-in the wrong number.\n");
			return 0x1;
	}
}

int Ct_I2CReadByte3(unsigned int ID,unsigned int index,unsigned int *data) //ID= slave addr, index = offset
{
    int i=0;
    outb(p1,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 0 = set write bit
    Delay5ms();
    Delay5ms();
    CT_Chk_SMBus_Ready();
    outb(0x00,SMBus_Port+3);
    Delay5ms();
    Delay5ms();
    outb(l1,SMBus_Port+5);
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);
    Delay5ms();
    Delay5ms();
    *data = inb(SMBus_Port+5);
//    printf("\nBypass setting: address 0x%x is 0x%x\n",index,*data);
    Delay5ms();
    Delay5ms();
    Check_Data();
    
    outb(p2,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 0 = set write bit
    Delay5ms();
    Delay5ms();
    CT_Chk_SMBus_Ready();
    outb(0x00,SMBus_Port+3);
    Delay5ms();
    Delay5ms();
    outb(l2,SMBus_Port+5);
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);
    Delay5ms();
    Delay5ms();
    *data = inb(SMBus_Port+5);
//    printf("\nBypass setting: address 0x%x is 0x%x\n",index,*data);
    Delay5ms();
    Delay5ms();
    Check_Data();
	
    outb(p3,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 0 = set write bit
    Delay5ms();
    Delay5ms();
    CT_Chk_SMBus_Ready();
    outb(0x00,SMBus_Port+3);
    Delay5ms();
    Delay5ms();
    outb(l3,SMBus_Port+5);
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);
    Delay5ms();
    Delay5ms();
    *data = inb(SMBus_Port+5);
//    printf("\nBypass setting: address 0x%x is 0x%x\n",index,*data);
    Delay5ms();
    Delay5ms();
    Check_Data();
    
    outb(ID+1,SMBus_Port+4);			//slave addr reg = base addr + 4 , add 0 = set write bit
    Delay5ms();
    Delay5ms();
    CT_Chk_SMBus_Ready();
    outb(index,SMBus_Port+3);
    Delay5ms();
    Delay5ms();
    outb(0x48,SMBus_Port+2);
    Delay5ms();
    Delay5ms();
    *data = inb(SMBus_Port+5);
//    printf("\nBypass setting: address 0x%x is 0x%x\n",index,*data);
    Delay5ms();
    Delay5ms();
    Check_Data();
    return *data;
}
#endif
