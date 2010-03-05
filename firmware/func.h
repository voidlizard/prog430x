#ifndef __func_h
#define __func_h

#include "platform.h"

#define DATA_BUF_SIZE_WORDS 256

typedef struct fet_world_t_
{
    char  echo;
    word  jtag_id;
    dword device_id_ptr;
    word  jtag_id_support;
    word  data[DATA_BUF_SIZE_WORDS];
} fet_world_t;

typedef enum  func_error_ {
     NO_ERR         = 0
    ,PARSE_ERR      = 1
    ,TGT_AQUIRE_ERR = 2
    ,UNKNOWN_ERR    = 3 
} func_error_t;

void log(char *msg);
void log_error(func_error_t err, char *msg);

void set_echo(fet_world_t *world, int echo);

void set_led(fet_world_t *world, byte led); 

void world_init(fet_world_t *world);

void target_aquire(fet_world_t *);
void target_release(fet_world_t *);

void target_jtag_id_support(fet_world_t *world, word id);
word target_jtag_id(fet_world_t *);

word target_read_word(fet_world_t *world, dword addr);
void target_read_mem(fet_world_t *world, dword addr, word len);

void target_power(fet_world_t *world, word status);

void target_erase_flash(fet_world_t *world, dword addr);
void target_erase_flash_mass(fet_world_t *world, dword addr);
void target_write_flash(fet_world_t *world, dword addr, word len);

void data_buf_fill(fet_world_t *world, byte val);
void data_buf_dump_txt(fet_world_t *world, dword addr, word len);

void uart_reset();

void readbytes(fet_world_t *world, word offset, word len); 


dword data_buf(fet_world_t *world);

#endif

