#define HOST 1
#define TRACE 1
short hosttrace[32];
unsigned char hostram[0x10000];
unsigned char hostrom[0x200000];
#include "ai.c"
struct Mem hostmem;
int host_sizeof_mem(void) { return sizeof(struct Mem); }
