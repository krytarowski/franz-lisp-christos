/*-
 * Header: adbig.c,v 1.2 83/11/26 12:12:37 sklower Exp
 *
 * Copyright (c) 1982, Regents, University of California
 *
 */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: adbig.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include <stdlib.h>

#include "global.h"

struct vl {
	intptr_t        high;
	intptr_t        low;
};

lispval 
adbig(lispval a, lispval b)
{
	int             la = 1, lb = 1;
	intptr_t       *sa, *sb, *sc, *base;
	lispval         p;
	intptr_t       *q, *r, *s;
	int             carry = 0;
	Keepxs();

	/* compute lengths */

	for (p = a; p->s.CDR; p = p->s.CDR)
		la++;
	for (p = b; p->s.CDR; p = p->s.CDR)
		lb++;
	if (lb > la)
		la = lb;

	/* allocate storage areas on the stack */

	base = alloca((3 * la + 1) * sizeof(*base));
	sc = base + la + 1;
	sb = sc + la;
	sa = sb + la;
	q = sa;

	/* copy s_dots onto stack */
	p = a;
	do {
		*--q = p->s.I;
		p = p->s.CDR;
	} while (p);
	while (q > sb)
		*--q = 0;
	p = b;
	do {
		*--q = p->s.I;
		p = p->s.CDR;
	} while (p);
	while (q > sc)
		*--q = 0;

	/* perform the addition */
	for (q = sa, r = sb, s = sc; q > sb;) {
		carry += *--q + *--r;
		*--s = carry & 0x3fffffff;
		carry >>= 30;
	}
	*--s = carry;

	p = export(sc, base);
	Freexs();
	return (p);
}
