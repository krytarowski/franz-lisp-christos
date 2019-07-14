/*-					-[Sat Jan 29 13:36:05 1983 by jkf]-
 * 	subbig.c
 * bignum subtraction
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: subbig.c,v 1.2 83/09/12 14:17:31 sklower Exp";
#else
__RCSID("$Id: subbig.c,v 1.5 2012/04/14 21:49:20 christos Exp $");
#endif
#endif

#include "global.h"

/*
 * subbig -- subtract one bignum from another.
 *
 * What this does is it negates each coefficient of a copy of the bignum
 * which is just pushed on the stack for convenience.  This may give rise
 * to a bignum which is not in canonical form, but is nonetheless a repre
 * sentation of a bignum.  Addbig then adds it to a bignum, and produces
 * a result in canonical form.
 */
lispval
subbig(lispval pos, lispval neg)
{
	lispval         work;
	intptr_t       *mysp = sp() - 2;
	intptr_t       *ersatz = mysp;
	Keepxs();

	for (work = neg; work != 0; work = work->s.CDR) {
		stack((long) (mysp -= 2));
		stack(-work->i);
	}
	mysp[3] = 0;
	work = (adbig(pos, (lispval) ersatz));
	Freexs();
	return (work);
}
