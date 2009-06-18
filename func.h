#ifndef __func_h
#define __func_h

#include "platform.h"

typedef struct fet_world_t_
{
    char  echo;
} fet_world_t;

void set_echo(fet_world_t *world, int echo);

void set_led(fet_world_t *world, byte led); 

void world_init(fet_world_t *world);

void target_aquire(fet_world_t *);
void target_release(fet_world_t *);

word target_read_word(fet_world_t *world, dword addr);
void target_read_mem(fet_world_t *world, dword addr, word len);

#endif

