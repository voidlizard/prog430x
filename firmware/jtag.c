#include "jtag.h"
#include "fet_hw.h"

void init_target()
{
    JTAGSEL  = 0x00;  // Pins all I/Os except during SPI access
    JTAGOUT  = 0x00;
    JTAGDIR  = 0;

#ifndef GOODFET
    VJTAGDIR |= VJTAG;
    VJTAGOUT |= VJTAG;
#endif

/*    RSTDIR  |= RST_BIT;*/
/*    RSTOUT   = 0;*/

/*    delay_ms(5);*/

    TSTOUT  |= TST_BIT;
    RSTOUT  |= RST_BIT;
    
    TST_SET();
    RST_SET();
    
    TSTDIR  |= TST_BIT;
    RSTDIR  |= RST_BIT;

    TST_CLR();

    delay_ms(10);

    TST_SET();

    delay_us(100);

    RST_CLR();

    delay_us(100);

    RST_CLR();

    delay_ms(5);

    TST_CLR();
    TST_SET();

    delay_ms(3);

    RST_SET();
}

void release_target()
{
    JTAGDIR  =  0x00;        // VCC is off, all I/Os are HI-Z
    delay_ms(50);
}


