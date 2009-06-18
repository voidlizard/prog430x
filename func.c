#include "platform.h"
#include "func.h"
#include "jtag.h"
#include "fet_hw.h"

void world_init(fet_world_t *world)
{
    world->echo = 0;
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
    printf("\r\naquire target\r\n"); 
}

void target_release(fet_world_t *world)
{
    printf("\r\nrelease target\r\n"); 
}

word target_read_word(fet_world_t *world, dword addr)
{
    printf("\r\nread word\r\n");
    return (word)0xDEAD;
}

void target_read_mem(fet_world_t *world, dword addr, word len)
{
    printf("\r\nread mem\r\n"); 
}


