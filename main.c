#include "fet_hw.h" 
#include "jtag.h"

static void wait_bsl(void);
static void wait_terminal(void);

static void wait_bsl(void)
{
    word i = 0;
    for(i=0;i<50;i++)
    {
        LED0_TOGGLE();
        delay_ms(100);
    }
}

static void wait_terminal(void)
{
    word i;
    for(i=0;i<10;i++)
    {
        LED0_TOGGLE();
        delay_ms(1000);
        printf(".");
    } 
}

int main(void)
{
    init_hardware();
 
    wait_bsl();

    LED0_ON();

    enable_nmi();

    wait_terminal();

    for(;;);
    return 0;
}

