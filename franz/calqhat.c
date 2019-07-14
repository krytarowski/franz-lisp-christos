/* Copyright (c) 1982, Regents, University of California */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: calqhat.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

intptr_t
calqhat(intptr_t *uj, intptr_t *v1)
{
	int             work1[2], work2[2];
	int             handy, handy2;
	int             qhat, rhat;
	char            err;
	if (*v1 == *uj) {
		/*
		 * set qhat to b-1 rhat is easily calculated since if we
		 * substite b-1 for qhat in the formula below, one gets
		 * (u[j+1] + v[1])
		 */
		qhat = 0x3fffffff;
		rhat = uj[1] + *v1;
	} else {
		/* work1 = u[j]b + u[j+1]; */
		handy2 = uj[1];
		handy = *uj;
		if (handy & 1)
			handy2 |= 0x40000000;
		if (handy & 2)
			handy2 |= 0x80000000;
		handy >>= 2;
		work1[LO] = handy2;
		work1[HI] = handy;
		qhat = ediv(work1, *v1, &err);
		/* rhat = work1 - qhat*v[1]; */
		rhat = work1[HI];
	}
again:
	/* check if v[2]*qhat > rhat*b+u[j+2] */
	emul(qhat, v1[1], 0, work1);
	/* work2 = rhat*b+u[j+2]; */
	{
		handy2 = uj[2];
		handy = rhat;
		if (handy & 1)
			handy2 |= 0x40000000;
		if (handy & 2)
			handy2 |= 0x80000000;
		handy >>= 2;
		work2[LO] = handy2;
		work2[HI] = handy;
	}
	vlsub(work1, work2);
	if (work1[HI] <= 0)
		return (qhat);
	qhat--;
	rhat += *v1;
	goto again;
}
