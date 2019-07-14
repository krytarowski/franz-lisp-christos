/* Copyright (c) 1982, Regents, University of California */

/*-
 * exarith(mul1,mul2,add,hi,lo)
 *
 * (hi,lo) gets 64 bit product + sum of mul1*mul2+add;
 * routine returns non-zero if product is bigger than 30 bits
 */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: exarith.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

intptr_t 
exarith(int mul1, int mul2, intptr_t add, intptr_t *hi, intptr_t *lo)
{
	int             work[2];
	intptr_t        rlo;

	emul(mul1, mul2, add, work);
	add = work[HI];
	add <<= 2;
	if ((rlo = work[LO]) < 0)
		add += 2;
	if (rlo & 0x40000000)
		add += 1;
	*lo = rlo & 0x3fffffff;
	(*hi = add);
	if ((add == 0) || (add != -1))
		return (add);
	*lo = rlo;
	return (0);
}
