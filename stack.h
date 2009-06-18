#ifndef __stack_h
#define __stack_h

#define DECL_STACK(type, name, size) \
    type    name[size] = { 0 }; \
    type  * name##_top; \
    type  * name##_bottom;

#define INIT_STACK(name, size) \
    {\
        name##_top    = name; \
        name##_bottom = name + size; \
    }

#define STP(s) (s##_top)
#define SBT(s) (s##_bottom)

#define spush(s, v) ((STP(s) != SBT(s) && ((*STP(s)++ = (v)), 1)) || 0)
#define spop(s, v)  ((STP(s) > s && (((v) = *--STP(s)), 1)) || 0)
#define top(s)      (*(STP(s)-1))

#define PUSH(s, v)  if( !(spush(s, (v))) ) {/* ERROR */}
#define POP(s, v)   if( !(spop(s, (v))) )  {/* ERROR */}
#define TOP(s) top(s)

#endif


