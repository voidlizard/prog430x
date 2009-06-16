#include "jtag.h"
#include "fet_hw.h"

void init_target()
{
    JTAGSEL  = 0x00;  // Pins all I/Os except during SPI access
    JTAGOUT  = 0x00;
    JTAGDIR  = 0;

    TSTOUT  |= TST_BIT;
    RSTOUT  |= RST_BIT;
    TSTDIR  |= TST_BIT;
    RSTDIR  |= RST_BIT;

    TSTOUT   = 0;

    delay_ms(10);

    TSTOUT |=  TST_BIT;

    delay_us(100);

    RSTOUT  =  0;

    delay_us(100);

    TSTOUT = 0;
    TSTOUT |= TST_BIT;

    delay_ms(3);

    RSTOUT |= RST_BIT;

/*    delay_ms(17);*/
}

void release_target()
{
	JTAGDIR  =  0x00;	    // VCC is off, all I/Os are HI-Z
    delay_ms(50);
}




