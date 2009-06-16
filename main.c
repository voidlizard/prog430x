#include "fet_hw.h" 
#include "jtag.h"

static void wait_bsl(void);
static void wait_terminal(void);

static void wait_bsl(void)
{
    word i = 0;
    for(i=0;i<50;i++)
    {
        LED0_TOGGLE();
        delay_ms(100);
    }
}

static void wait_terminal(void)
{
    word i;
    for(i=0;i<10;i++)
    {
        LED0_TOGGLE();
        delay_ms(1000);
        printf(".");
    } 
}

#define MAX_ENTRY_TRY 7

typedef struct msp430x_mcu_info_
{
    word  jtad_id;
    word  core_id;
    word  device_id_addr;
    word  device_id;
} msp430x_mcu_info_t; 

void mcu_info_init(msp430x_mcu_info_t *mcu)
{
    mcu->jtad_id        = 0;
    mcu->core_id        = 0;
    mcu->device_id_addr = 0;
    mcu->device_id      = 0;
}

static word test_core_id(msp430x_mcu_info_t *mcu_info)
{
    word i;
    word JtagId          = 0x00;
    word CoreId          = 0x00;
    word DeviceIdPointer = 0x00;

    for (i = 0; i < MAX_ENTRY_TRY; i++)
    {
        StopJtag();    // release JTAG/TEST signals to savely reset the test logic
        JtagId = StartJtag();   // establish the physical connection to the JTAG interface
        if(JtagId == JTAG_ID91) // break if a valid JTAG ID is being returned
          break;
    }

    mcu_info->jtad_id = JtagId;

    if(i >= MAX_ENTRY_TRY) return(STATUS_ERROR);

  if(JtagId == JTAG_ID91)
  {
    // Get Core identification info
    IR_Shift(IR_COREIP_ID);
    CoreId = DR_Shift16(0);
    if(CoreId == 0)
    {
      return(STATUS_ERROR);
    }
    IR_Shift(IR_DEVICE_ID);
    DeviceIdPointer = DR_Shift20(0);
    // The ID pointer is an un-scrambled 20bit value
    DeviceIdPointer = ((DeviceIdPointer & 0xFFFF) << 4 )  + (DeviceIdPointer >> 16 );
   
    printf("JTAG ID: %04X\r\n",JtagId);
    printf("CORE ID: %04X\r\n",CoreId);
    printf("DEVICE ID PTR: %04X %04X\r\n", (word)(DeviceIdPointer>>16), (word)(DeviceIdPointer & 0xFFFF));

    return(STATUS_OK);
  }
  else
  {
    return(STATUS_ERROR);
  }
}

word IsLockKeyProgrammed(void)
{
    word i;

    for (i = 3; i > 0; i--)     //  First trial could be wrong
    {
        IR_Shift(IR_CNTRL_SIG_CAPTURE);
        if (DR_Shift16(0xAAAA) == 0x5555)
        {
            return(STATUS_OK);  // Fuse is blown
        }
    }
    return(STATUS_ERROR);       // fuse is not blown
}

word data[] = {0xDEAD, 0xBEEF};

int main(void)
{

    word jtagId = 0x00, i;
    word deviceId = 0x00;

    unsigned long addr = 0;

    msp430x_mcu_info_t mcu;

    mcu_info_init(&mcu);

    init_hardware();
 
    wait_bsl();

    LED0_ON();

    enable_nmi();

    wait_terminal();

    init_target();

    if( STATUS_ERROR == test_core_id(&mcu) ) printf("\r\nFUCKUP\r\n");

    if( STATUS_OK == IsLockKeyProgrammed() ) printf("\r\nLOCKED\r\n");

    if( SyncJtag_AssertPor() != STATUS_OK) printf("\r\nPOR FUCKUP\r\n");
    else printf("\r\nPOR OK\r\n");

    // RAM ACCESS - OK
    WriteMem_430Xv2(F_WORD, 0x1C00, 0xDEAD);
    printf("0x1C00: %04X\r\n", ReadMem_430Xv2(F_WORD, 0x1C00));

    printf("0xF00F: %04X\r\n", ReadMem_430Xv2(F_WORD, 0xF00F));
    printf("0x0FF0: %04X\r\n", ReadMem_430Xv2(F_WORD, 0x0FF0));

    IR_Shift(IR_DEVICE_ID);
    addr = DR_Shift20(0);

    addr = ((addr & 0xFFFF) << 4 )  + (addr >> 16 );
    printf("DeviceId ADDR: %04X %04X\r\n", (word)(addr >> 16), (word)(addr & 0xFFFF) );

/*    ResetTAP();*/

/*    for(i = 0; i < 8; i++, addr += 2 ) {*/
/*        ReadMemQuick_430Xv2(addr, 1, &deviceId);*/
/*        printf("ADDR[i]: %d,  VALUE: %04X\r\n", i, deviceId);*/
/*    }*/

    IR_Shift(IR_DEVICE_ID);
    addr = DR_Shift16(0);

    printf("DeviceId ADDR 16: %04X \r\n", (word)(addr) );
    printf("VALUE: %04X\r\n", ReadMem_430Xv2(F_WORD, addr));

    printf("\r\n--\r\n");

/*    for( i = 0, addr = 0x1C00; i < 16384/2; i++, addr += 2 ) {*/
/*        WriteMem_430Xv2(F_WORD, addr, 0xDDDD);*/
/*    }*/

/*    for( i = 0, addr = 0x0000; i < 0xFFFF / 2; i++, addr += 2 ) {*/
/*        if( !(i%8) ) printf("%04X: ", addr);*/
/*        deviceId = ReadMem_430Xv2(F_WORD, addr);*/
/*        printf("%02x %02x ", deviceId & 0xFF, deviceId >> 8);*/
/*        if( !((i+1) % 8) ) printf("\r\n");*/
/*    }*/

/*    WriteFLASH_430Xv2(0, 0, 0);*/

    EraseFLASH_430Xv2_wo_release(ERASE_SGMT, 0x5C00);

    delay_ms(500);

    WriteFLASH_430Xv2_wo_release(0x5C00, 2, data);

    delay_ms(1000);

    for( i = 0, addr = 0x5C00; i < 256; i++, addr += 2 ) {
        if( !(i%8) ) printf("%04X: ", addr);
        deviceId = ReadMem_430Xv2(F_WORD, addr);
        printf("%02x %02x ", deviceId & 0xFF, deviceId >> 8);
        if( !((i+1) % 8) ) printf("\r\n");
    }


/*    // The ID pointer is an un-scrambled 20bit value*/
/*    DeviceIdPointer = ((DeviceIdPointer & 0xFFFF) << 4 )  + (DeviceIdPointer >> 16 );*/
/*    printf("0x000A: %04X\r\n", ReadMem_430Xv2(F_WORD, 0x000A));*/

/*    for(i = 0, addr = 0x0000; i< 0xFFFF/2; i++, addr += 2 ) {*/
/*        deviceId = ReadMem_430Xv2(F_WORD, addr);*/
/*        printf("A: %04X, V: %04X\r\n", addr, deviceId);*/
/*    }*/

/*    ReadMemQuick_430Xv2(mcu.device_id_addr + 4, 1, (word*)&(mcu.device_id));*/

/*    printf("CPU ID: %04X\r\n", mcu.device_id);*/

    for(;;);
    return 0;
}

