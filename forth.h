#ifndef __forth_h
#define __forth_h

typedef int (*reader_t)(char *buf, int size, int echo);

void serial_interp(reader_t readf, world_t *world);

#endif

