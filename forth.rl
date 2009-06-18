#include "platform.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "func.h"

typedef fet_world_t world_t;

#include "forth.h"
#include "stack.h"

%%{
	machine forth;
    write data;
}%%

#define BUFSIZE 1
#define ASTACK_SIZE 16

void add_dec(dword *val, int c)
{
    *val *= 10;
    *val += (c - '0');
}

void add_hex(dword *val, int c)
{
    int i = 0;
    switch(c) {
        case '0': i = 0; break;
        case '1': i = 1; break;
        case '2': i = 2; break;
        case '3': i = 3; break;
        case '4': i = 4; break;
        case '5': i = 5; break;
        case '6': i = 6; break;
        case '7': i = 7; break;
        case '8': i = 8; break;
        case '9': i = 9; break;
        case 'a':
        case 'A': i = 0x0A; break;
        case 'b':
        case 'B': i = 0x0B; break;
        case 'c':
        case 'C': i = 0x0C; break;
        case 'd':
        case 'D': i = 0x0D; break;
        case 'e':
        case 'E': i = 0x0E; break;
        case 'f':
        case 'F': i = 0x0F; break;
    }
    *val *= 16;
    *val += i;
}

void serial_interp(reader_t readf, world_t *world) {
	char *p  = 0, *pe = 0, *eof = 0, *ts = 0, *te = 0;
	int cs, have = 0, space = 0, len = 0;
	int done = 0;
	char buf[BUFSIZE];

    dword literal = 0, tmp = 0, ttmp = 0;
    DECL_STACK(dword, astack, 16);

    %%{
        machine forth;

        action reset_lit { literal = 0;}

        action reset   { astack_top = astack; literal = 0; }

        action lit     { spush(astack, literal); }
        action drop    { spop(astack, tmp); }
        action dup     { spush(astack, top(astack)); }
        action swap    { spop(astack, tmp); spop(astack, ttmp); spush(astack, tmp); spush(astack, ttmp); } 
        action dot_x   { spop(astack, tmp); printf("%04X", (word)tmp); }
        action dot_c   { spop(astack, tmp); printf("%c",   (word)tmp); }

        action led     { spop(astack, tmp); set_led(world,  tmp); }
        action echo    { spop(astack, tmp); set_echo(world, tmp); }

        action aquire  { target_aquire(world); }
        action release { target_release(world); }

        action jtag_id { spush(astack, target_jtag_id(world)); }
        action jtag_id_sup { spop(astack, tmp); target_jtag_id_support(world, tmp); }

        action tgt_read_w { spop(astack, tmp); spush(astack, target_read_word(world, tmp)); }

        action tgt_read_m { spop(astack, tmp); spop(astack, ttmp); target_read_mem(world, ttmp, tmp); }

        action dump_buf_txt { data_buf_dump_txt(world, (word)(world->data), DATA_BUF_SIZE_WORDS*2 ); }
        
        action  xdump       { spop(astack, tmp); spop(astack, ttmp); data_buf_dump_txt(world, ttmp, tmp ); }
        
        action bfill        { spop(astack, tmp); data_buf_fill(world, (byte)tmp); }

        ping         = 'ping'    %{log("pong");};
        drop         = 'drop'    %drop;
        swap         = 'swap'    %swap;
        dup          = 'dup'     %dup;
        dot_x        = '.x'      %dot_x;
        dot_c        = '.c'      %dot_c;
        echo         = 'echo'    %echo;
        led          = 'led'     %led;
        
        reset        = 'reset'   %reset;
        
        aquire       = 'aquire'  %aquire;
        release      = 'release' %release;

        jtag_id      = 'jtag-id' %jtag_id;
        jtag_id_sup  = 'jtag-id-sup' %jtag_id_sup;

        tgt_read_w   = '@xw'  %tgt_read_w;
        tgt_read_m   = '@xm'  %tgt_read_m;

        xdump        = '@xdump' %xdump;
        
        dump_buf_txt = '@dump-buf-txt' %dump_buf_txt;

        bfill        = 'bfill' %bfill;

        literal = (('$' xdigit+ @{add_hex(&literal, fc);}) | digit+ @{add_dec(&literal, fc);} ) %lit;
        word    = ping | dot_x | dot_c | drop | swap | echo | led
                  | aquire | release | jtag_id | jtag_id_sup | tgt_read_w | tgt_read_m
                  | dump_buf_txt | xdump | bfill | reset;

        main := ((literal | word ) space+ %reset_lit )* ;
        
        write init;
    }%%

    INIT_STACK(astack, 16);    

	while( !done )
	{
        eof = 0;
		p = buf + have;
		space = BUFSIZE - have;

		if( !space ) 
		{
			break;
		}

		len = readf(p, space, world->echo);

		pe = p + len;

		if( len < space )
		{
			done = 1;
			eof = pe;
		}

		%% write exec;

		if( cs == forth_error ) {
			break;
		}

		if( ts == 0 )
			have = 0;
		else
		{
			have = pe - ts;
			memmove( buf, ts, have );
			te = buf + (te-ts);
			ts = buf;
		}
	}
}

