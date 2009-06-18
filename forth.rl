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

        action reset   { literal = 0;}
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

        action tgt_read_w { spop(astack, tmp); spush(astack, target_read_word(world, tmp)); }

        ping    = 'ping'    %{printf("\r\npong\r\n");};
        drop    = 'drop'    %drop;
        swap    = 'swap'    %swap;
        dup     = 'dup'     %dup;
        dot_x   = '.x'      %dot_x;
        dot_c   = '.c'      %dot_c;
        echo    = 'echo'    %echo;
        led     = 'led'     %led;
        aquire  = 'aquire'  %aquire;
        release = 'release' %release;
        tgt_read_w = '@xw'  %tgt_read_w;

        literal = (('$' xdigit+ @{add_hex(&literal, fc);}) | digit+ @{add_dec(&literal, fc);} ) %lit;
        word    = ping | dot_x | dot_c | drop | swap | echo | led
                  | aquire | release | tgt_read_w;

        main := ((literal | word ) space+ %reset )* ;
        
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

