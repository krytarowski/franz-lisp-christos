#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: prunei.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"
#include "structs.h"

void
prunei(lispval what)
{
	extern struct types int_str;

	if (((long) what) > ((long) gstart)) {
		--(int_items->i);
		what->i = (long) int_str.next_free;
		int_str.next_free = (char *) what;
	}
}
