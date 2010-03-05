#include "fet_hw.h"

#include <mspgcc/util.h>
#include <mspgcc/ringbuffer.h>

static void init_timer();
static void init_uarts();

RINGBUFFER_NEW(ringbuffer, 512);

#define USART0_TX_BUFFER_SIZE 64 

static volatile uint16_t uart_tx_busy = 0;
static volatile int  txwrite = 0;
static volatile int  txread  = 0;
static volatile char txbuf[USART0_TX_BUFFER_SIZE];

volatile uint32_t __ticks_ms = 0;

static void __init_UART0(void);

int putchar(int);

interrupt (NMI_VECTOR) wakeup nmi_isr (void);
interrupt (TIMERA0_VECTOR) isr_timer_a0( void );
interrupt (UART0RX_VECTOR) wakeup uart0_rx_isr( void );
interrupt (UART0TX_VECTOR) wakeup uart0_tx_isr( void );

int getchar()
{
    while(!ringbuffer_len(&ringbuffer)) _NOP();
    return ringbuffer_get(&ringbuffer);
}

void ioreset()
{
    ringbuffer_clear(&ringbuffer);
}

static void init_timer()
{
    volatile unsigned int i;
    BCSCTL1 |= XTS;                           // ACLK= LFXT1= HF XTAL
    do {
        IFG1 &= ~OFIFG;                       // Clear OSCFault flag
        for (i = 0xFF; i > 0; i--);           // Time for flag to set
    } while ((IFG1 & OFIFG));                 // OSCFault flag still set?
    BCSCTL2 |= SELM_3;                        // MCLK= LFXT1 (safe)
    CCR0   = (CPU_FREQ/TIMERA0_FREQ);         // 1000 Hz
    TACTL  = TASSEL_1 + MC_1;
    CCTL0  = CCIE;
}

static void init_uarts()
{
    __init_UART0();
}

void init_hardware() 
{
    _DINT();

    WDTCTL = WDTPW + WDTHOLD; // + WDTNMI + WDTNMIES;  // WDT off NMI hi/lo

    //IE1  |= NMIIE;                                 // Enable NMI
    //IFG1 |= NMIIFG;

    init_timer();
    init_uarts();

    P1OUT = 0x10;
    P1DIR = 0x01|0x10;

#ifndef GOODFET

    VJTAGDIR |= VJTAG;
    VJTAGOUT |= VJTAG;

#endif

/*    P1OUT |= 10;*/
/*    P1DIR |= 0x10;*/
/*    P1OUT |= 10;*/

    _EINT();
}

static void __init_UART0(void) {
    P3SEL |= 0x30;                            // P3.6,7 = USART1 TXD/RXD
    ME1 |= UTXE0 + URXE0;                     // Enable USART0 TXD/RXD
    UCTL0 |= CHAR;                            // 8-bit character
    UTCTL0 |= SSEL0;                          // UCLK = ACLK
    UBR00 = BITRATE_LO(DEFAULT_UART_BITRATE);
    UBR10 = BITRATE_HI(DEFAULT_UART_BITRATE);
    UMCTL0 = 0x00;                            // Modulation
    UCTL0 &= ~SWRST;                          // Initialize USART state machine
    IE1 |= URXIE0|UTXIE0;                     // Enable USART0 RX/TX interrupts
    //IE1 |= UTXIE0;                     // Enable USART0 RX/TX interrupts
}

interrupt (TIMERA0_VECTOR) isr_timer_a0( void )
{
    __ticks_ms += 1;
}


interrupt (UART0RX_VECTOR) wakeup uart0_rx_isr( void )
{
    char character = U0RXBUF;
    ringbuffer_put(&ringbuffer, character);
}


interrupt (UART0TX_VECTOR) wakeup uart0_tx_isr( void )
{
    if (txwrite != txread )
    {
        TXBUF0 = txbuf[txread++];
        txread %= USART0_TX_BUFFER_SIZE;
    }
    else
    {
        uart_tx_busy = 0;
    }

/*    char character = 0;*/
/*    if (ringbuffer_len(&ringbuffer_tx)) {*/
/*        TXBUF0 = character;*/
/*        LED0_ON();*/
/*    } else {*/
/*        uart_tx_busy = 0;*/
/*        LED0_OFF();*/
/*    }*/
}

interrupt (NMI_VECTOR) wakeup nmi_isr (void)
{
    IFG1 &= ~NMIIFG;                          // Reclear NMI flag in case bounce
    IE1 |= NMIIE;                             // Enable NMI
    //P1OUT ^= 0x01;
}

void delay_ms(uint32_t ms)
{
    uint32_t t1 = 0;
    t1 = __ticks_ms;
    for(;;)
        if( __ticks_ms - t1 > ms || t1 > __ticks_ms ) break;
}

/*---------------------------------------------------------------------------
   Delay function (resolution is ~1 us at 8MHz clock)
   Arguments: word microeconds (number of ms, max number is 0xFFFF)
*/
void delay_us(word microeconds)
{
    do
    {
        _NOP();
        _NOP();
    }
    while (--microeconds > 0);
}

int putchar(int character) {
  
    uint16_t temp;

    temp = (txwrite + 1) % USART0_TX_BUFFER_SIZE;

    if (temp == txread)
        return -1;                          // no room

    U0IE &= ~UTXIE0 ;                     // disable TX interrupts

    // check if in process of sending data
    if (uart_tx_busy)
    {
        // add to queue
        txbuf[txwrite] = (char)character;
        txwrite = temp;
    }
    else
    {
        // set running flag and write to output register
        uart_tx_busy = 1;
        TXBUF0 = character;
    }

    U0IE |= UTXIE0;                       // enable TX interrupts

/*    IE1 &= ~UTXIE0; */
/*    if( !uart_tx_busy ) {*/
/*        TXBUF0 = character;*/
/*        uart_tx_busy = 1;*/
/*    }*/
/*    else {*/
/*        ringbuffer_put(&ringbuffer_tx, character);*/
/*    }*/
/*    IE1 |= UTXIE0; */

    return 1;
}

void enable_nmi()
{
    _DINT();
    WDTCTL = WDTPW + WDTHOLD + WDTNMI + WDTNMIES;  // WDT off NMI hi/lo
    IE1  |= NMIIE;                                 // Enable NMI
    IFG1 |= NMIIFG;
    _EINT();
}


