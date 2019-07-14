/* Copyright (c) 1982, Regents, University of California */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: dodiv.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

intptr_t 
dodiv(intptr_t *top, intptr_t *bottom)	/* top least significant; bottom most */
{
	int             work[2];
	char            error;
	intptr_t        rem = 0;
	intptr_t       *p = bottom;

	for (; p <= top; p++) {
		emul(0x40000000, rem, *p, work);
		*p = ediv(work, 1000000000, &error);
		rem = work[HI];
	}
	return (rem);
}

void
dsneg(intptr_t *top, intptr_t *bottom)
{
	intptr_t       *p = top;
	int             carry = 0;
	int             digit;

	while (p >= bottom) {
		digit = carry - *p;
		/* carry = digit >> 30; is slow on 68K */
		if (digit < 0)
			carry = -2;
		if (digit & 0x40000000)
			carry += 1;
		*p-- = digit & 0x3fffffff;
	}
}
