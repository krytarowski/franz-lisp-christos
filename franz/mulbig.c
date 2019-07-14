/*-
 * Header: mulbig.c,v 1.2 83/11/26 12:13:29 sklower Exp
 *
 * Copyright (c) 1982, Regents, University of California
 *
 */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: mulbig.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include <stdlib.h>
#include "global.h"

lispval 
mulbig(lispval a, lispval b)
{
	int             la = 1, lb = 1;
	intptr_t       *sa, *sb, *sc, *base;
	lispval         p;
	intptr_t       *q, *r, *s;
	intptr_t        carry = 0, test;
	int             work[2];
	Keepxs();

	/* compute lengths */

	for (p = a; p->s.CDR; p = p->s.CDR)
		la++;
	for (p = b; p->s.CDR; p = p->s.CDR)
		lb++;

	/* allocate storage areas on the stack */

	base = alloca((la + la + lb + lb + 1) * sizeof(long));
	sc = base + la + lb + 1;
	sb = sc + lb;
	sa = sb + la;
	q = sa;

	/* copy s_dots onto stack */
	p = a;
	do {
		*--q = p->s.I;
		p = p->s.CDR;
	} while (p);
	p = b;
	do {
		*--q = p->s.I;
		p = p->s.CDR;
	} while (p);
	while (q > base)
		*--q = 0;	/* initialize target */

	/* perform the multiplication */
	for (q = sb; q > sc; *--s = carry)
		for ((r = sa, s = (q--) - lb, carry = 0); r > sb;) {
			carry += *--s;
			emul(*q, *--r, carry, work);
			test = work[LO];
			carry = work[HI] << 2;
			if (test < 0)
				carry += 2;
			if (test & 0x40000000)
				carry += 1;
			*s = test & 0x3fffffff;
		}

	p = export(sc, base);
	Freexs();
	return (p);
}
