#ifndef __fet_hw_h
#define __fet_hw_h

#include "platform.h"

#define CPU_FREQ      7372800
#define FREQUENCY     CPU_FREQ

#define TIMERA0_FREQ  1000

//#define DEFAULT_UART_BITRATE 115200
#define DEFAULT_UART_BITRATE 230400 
//#define DEFAULT_UART_BITRATE 38400 
//#define DEFAULT_UART_BITRATE 460800 


// GOODFET --- GOODFET-M
//  Pin36  --- Pin48          TST
//  Pin26  --- Pin49          RST
//  Pin44  --- Pin44          TMS
//  Pin45  --- Pin45          TDI 
//  Pin46  --- Pin46          TDO
//  Pin47  --- Pin47          TCK

//         --- Pin50          P5.6

#ifdef GOODFET

#define JTAGIN       P5IN   // Control ports are on P5.x
#define JTAGOUT      P5OUT
#define JTAGDIR      P5DIR
#define JTAGSEL      P5SEL
#define TMS          0x01   // P5.0 JTAG TMS input pin
#define TDI          0x02   // P5.1 JTAG TDI input pin  (SIMO1 if SPI mode)
#define TDO          0x04   // P5.2 JTAG TDO output pin (SOMI1 if SPI mode)
#define TCK          0x08   // P5.3 JTAG TCK input pin  (UCLK1 if SPI mode)

#define RSTOUT       P2OUT
#define RSTDIR       P2DIR

#define TSTDIR       P4DIR 
#define TSTOUT       P4OUT

#define RST_BIT      (1<<6)
#define TST_BIT      1

#else

#define JTAGIN       P5IN   // Control ports are on P5.x
#define JTAGOUT      P5OUT
#define JTAGDIR      P5DIR
#define JTAGSEL      P5SEL
#define VJTAGDIR     P5DIR
#define VJTAGOUT     P5OUT

#define RSTOUT       P5OUT
#define RSTDIR       P5DIR

#define TSTDIR       P5DIR 
#define TSTOUT       P5OUT

#define TMS          0x01   // P5.0 JTAG TMS input pin
#define TDI          0x02   // P5.1 JTAG TDI input pin  (SIMO1 if SPI mode)
#define TDO          0x04   // P5.2 JTAG TDO output pin (SOMI1 if SPI mode)
#define TCK          0x08   // P5.3 JTAG TCK input pin  (UCLK1 if SPI mode)

#define RST_BIT      BIT5
#define TST_BIT      BIT4

#define VJTAG        BIT6

#endif

#define RST_SET()   RSTOUT |=  RST_BIT;
#define RST_CLR()   RSTOUT &= ~RST_BIT;

#define TST_SET()  TSTOUT |=  TST_BIT;
#define TST_CLR()  TSTOUT &= ~TST_BIT;

#define VJTAG_SET() VJTAGOUT |=  VJTAG;
#define VJTAG_CLR() VJTAGOUT &= ~VJTAG;

#define BITRATE_HI(X) ((uint16_t)((CPU_FREQ/(X))) >> 8)
#define BITRATE_LO(X) ((uint16_t)((CPU_FREQ/(X))) &0xFF)

#define LED0_ON()      {P1OUT |=  0x01;}
#define LED0_OFF()     {P1OUT &= ~0x01;}
#define LED0_TOGGLE()  {P1OUT ^=  0x01;}

#define FTDI_OFF()   P1OUT &= ~0x10;
#define FTDI_ON()    P1OUT |= 0x10;

void init_hardware();

void enable_nmi();

void delay_us(word);
void delay_ms(uint32_t);

void ioreset();

#endif

