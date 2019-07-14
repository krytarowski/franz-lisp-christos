/* Copyright (c) 1982, Regents, University of California */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: mlsb.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

intptr_t
mlsb(intptr_t *utop, intptr_t *ubot, intptr_t *vtop, int nqhat)
{
	int             handy, carry;
	int             work[2];

	for (carry = 0; utop >= ubot; utop--) {
		emul(nqhat, *--vtop, carry + *utop, work);
		carry = work[HI];
		handy = work[LO];
		*utop = handy & 0x3fffffff;
		carry <<= 2;
		if (handy & 0x80000000)
			carry += 2;
		if (handy & 0x40000000)
			carry += 1;
	}
	return (carry);
}
intptr_t
adback(intptr_t *utop, intptr_t *ubot, intptr_t *vtop)
{
	int             handy, carry;
	carry = 0;
	for (; utop >= ubot; utop--) {
		carry += *--vtop;
		carry += *utop;
		*utop = carry & 0x3fffffff;
		/* next junk is faster version of  carry >>= 30; */
		handy = carry;
		carry = 0;
		if (handy & 0x80000000)
			carry -= 2;
		if (handy & 0x40000000)
			carry += 1;
	}
	return (carry);
}

intptr_t 
dsdiv(intptr_t *top, intptr_t *bot, intptr_t div)
{
	int             work[2];
	char            err;
	intptr_t            handy, carry = 0;
	for (carry = 0; bot <= top; bot++) {
		handy = *bot;
		if (carry & 1)
			handy |= 0x40000000;
		if (carry & 2)
			handy |= 0x80000000;
		carry >>= 2;
		work[LO] = handy;
		work[HI] = carry;
		*bot = ediv(work, div, &err);
		carry = work[HI];
	}
	return (carry);
}

void
dsadd1(intptr_t *top, intptr_t *bot)
{
	intptr_t       *p, work = 0, carry = 0;

	/*
	 * this assumes canonical inputs
	 */
	for (p = top; p >= bot; p--) {
		work = *p + carry;
		*p = work & 0x3fffffff;
		carry = 0;
		if (work & 0x40000000)
			carry += 1;
		if (work & 0x80000000)
			carry -= 2;
	}
	p[1] = work;
}
intptr_t
dsrsh(intptr_t *top, intptr_t *bot, intptr_t ncnt, intptr_t mask1)
{
	intptr_t       *p = bot;
	intptr_t        r = -ncnt, l = 30 + ncnt, carry = 0, work, save;
	intptr_t        mask = -1 ^ mask1;

	while (p <= top) {
		save = work = *p;
		save &= mask;
		work >>= r;
		carry <<= l;
		work |= carry;
		*p++ = work;
		carry = save;
	}
	return (carry);
}
