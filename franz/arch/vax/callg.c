#include "global.h"
#include "dfuncs.h"
#include "err.h"

lispval
callg(lispval (*fn)(void), intptr_t[] args) {
	asm("	callg	*8(ap), *4(ap)");
}
