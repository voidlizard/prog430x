#include "platform.h"

#include <string.h>

#include "func.h"
#include "jtag.h"
#include "fet_hw.h"

static const char *errors[] = {
     "OK"
    ,"PARSE_ERR"
    ,"TGT_AQUIRE_ERR"
    ,"UNKNOWN_ERR"
};

void log(char *msg)
{
    printf("\r\n%s\r\n", msg);
}

void log_error(func_error_t err, char *msg)
{
    printf("\r\n*** Error [%s]: %s\r\n",  errors[err], msg );
}

void world_init(fet_world_t *world)
{
    world->echo = 0;
    world->jtag_id = 0;
    world->device_id_ptr = 0;
    world->jtag_id_support = JTAG_ID91;
    memset(world->data, 0, DATA_BUF_SIZE_WORDS*2);
}

void set_echo(fet_world_t *world, int echo)
{
    world->echo = echo;
}

void set_led(fet_world_t *world, byte led)
{
    if(led)
    {
        LED0_ON(); 
    } 
    else
    {
        LED0_OFF();
    }
}

void target_aquire(fet_world_t *world)
{
    int i = 0;
    word jtag_id = 0;
    word core_id = 0;
    dword device_id_ptr = 0;


    init_target();

    for (i = 0; i < 10; i++)
    {
        StopJtag();              // release JTAG/TEST signals to savely reset the test logic
        jtag_id = StartJtag();   // establish the physical connection to the JTAG interface
        if(jtag_id == world->jtag_id_support)  // break if a valid JTAG ID is being returned
        {
            world->jtag_id = jtag_id;
            break;
        }
    }

    if( world->jtag_id != JTAG_ID91 )
    {
        log_error(TGT_AQUIRE_ERR, "Wrong jtag version"); 
        return;
    }

    IR_Shift(IR_COREIP_ID);
    core_id = DR_Shift16(0);
    
    if( core_id == 0x00 ) {
        log_error(TGT_AQUIRE_ERR, "Wrong core id (0)");
        return;
    }

    IR_Shift(IR_DEVICE_ID);
    device_id_ptr = DR_Shift20(0);
    world->device_id_ptr = ((device_id_ptr & 0xFFFF) << 4 ) + ( device_id_ptr >> 16 );

    if( IsLockKeyProgrammed() ) {
        log_error(TGT_AQUIRE_ERR, "Fuse is blown");
        return;
    }

    if( SyncJtag_AssertPor() != STATUS_OK) {
        log_error(TGT_AQUIRE_ERR, "SyncJTAG failed");
    }

    log("target_aquire ok");
}

void target_release(fet_world_t *world)
{
    RSTOUT = 0;
    JTAGIN = 0;
    
    delay_ms(10);

    RSTOUT |= RST_BIT;

    world->jtag_id = 0;
 
    ReleaseDevice_430Xv2(V_BOR);

    log("target_release ok");
}

word target_read_word(fet_world_t *world, dword addr)
{
    word tmp = 0;
    ReadMemQuick_430Xv2(addr, sizeof(tmp), &tmp);
    return tmp;
}

void target_read_mem(fet_world_t *world, dword addr, word len)
{
    word to_read = 0;
    to_read = len <= DATA_BUF_SIZE_WORDS*2 ? len : DATA_BUF_SIZE_WORDS*2;
    ReadMemQuick_430Xv2(addr, to_read, world->data);
}

void target_jtag_id_support(fet_world_t *world, word id)
{
    world->jtag_id_support = id;
}

word target_jtag_id(fet_world_t *world)
{
    return world->jtag_id;
}

void data_buf_fill(fet_world_t *world, byte val)
{
    memset(world->data, val, DATA_BUF_SIZE_WORDS*2);
}

void data_buf_dump_txt(fet_world_t *world, dword addr, word len)
{
    int i = 0;
    word *data;

    data = world->data;

    printf("\r\n");

    if( addr & 0xFFFF0000UL ) {
        printf("@%04X%04X\r\n", (word)(addr>>16), (word)(addr&0xFFFF));
    }
    else {
        printf("@%04X\r\n", (word)(addr));
    }

    for( i = 0; i < len/2; i++ ) {
        printf("%02X %02X ", data[i] & 0xFF, data[i] >> 8);
        if( !((i+1) % 8) ) printf("\r\n");
    }

    printf("\r\nq\r\n");

}


