#include "platform.h"
#include "fet_hw.h" 
#include "jtag.h"

#include "func.h"

typedef fet_world_t world_t;

#include "forth.h"

static void wait_bsl(void);
static void wait_terminal(void);
static int serial_reader(char *buf, int size, int echo);

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

int serial_reader(char *buf, int size, int echo)
{
    int read  = 0;
    char c;
    while(size--) {
        c = getchar();
        if( echo ) {
            if( c == '\n' || c == '\r' ) printf("\r\n");
            else putchar(c);
        }
        *buf++ = c;
        read++;
    }
    return read;
}

int main(void)
{
    world_t world;

    init_hardware();
 
    wait_bsl();

    LED0_OFF();

    enable_nmi();

    world_init(&world);

    for(;;) {
        serial_interp(serial_reader, &world);
        log_error(PARSE_ERR, "oops");
    }

    return 0;
}

