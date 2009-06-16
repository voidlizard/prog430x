#ifndef __fet_hw_h
#define __fet_hw_h

#include "platform.h"

#define CPU_FREQ      7372800
#define FREQUENCY     CPU_FREQ

#define TIMERA0_FREQ  1000

#define DEFAULT_UART_BITRATE 115200

void init_hardware();
void init_target();

void delay_us(word);
void delay_ms(uint32_t);

#endif

