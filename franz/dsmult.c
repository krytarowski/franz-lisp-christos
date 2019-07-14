/* Copyright (c) 1982, Regents, University of California */
/*
 * dsmult(top,bot,mul) --
 * multiply an array of intptr_ts on the stack, by mul.
 * the element top through bot (inclusive) get changed.
 * if you expect a carry out of the most significant,
 * it is up to you to provide a space for it to overflow.
 */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: dsmult.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

void
dsmult(intptr_t *top, intptr_t *bot, intptr_t mul)
{
	intptr_t       *p;
	int             work[2] = { 0, 0 };
	intptr_t        add = 0;

	for (p = top; p >= bot; p--) {
		emul(*p, mul, add, work);	/* *p has 30 bits of info,
						 * mul has 32 yielding a 62
						 * bit product. */
		*p = work[LO] & 0x3fffffff;	/* the stack gets the low 30
						 * bits */
		add = work[HI];	/* we want add to get the next 32 bits. on a
				 * 68k you might better be able to do this by
				 * shifts and tests on the carry but I don't
				 * know how to do this from C, and the code
				 * generated here will not be much worse.
				 * Far less bad than shifting work.low to the
				 * right 30 bits just to get the top 2. */
		add <<= 2;
		if (work[LO] < 0)
			add += 2;
		if (work[LO] & 0x40000000)
			add += 1;
	}
	p[1] = work[LO];	/* on the final store want all 32 bits. */
}
