
#include <msp430x54xx.h>
#include <stdlib.h>
#include <io.h>

__attribute__((naked, section(".init3"))) void __low_level_init(void)
{
 WDTCTL = WDTPW + WDTHOLD;
/*// any other low level initialization you want*/
}

int main(void) {

    WDTCTL= WDTPW + WDTHOLD;

    P3DIR = 0xFF;
    P3OUT = 0xFF;

    for(;;) _NOP();

    return 0;
}

