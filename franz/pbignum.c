/*-					-[Sat Jan 29 13:30:47 1983 by jkf]-
 * 	pbignum.c
 * print a bignum
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: pbignum.c,v 1.3 83/09/12 14:17:59 sklower Exp";
#else
__RCSID("$Id: pbignum.c,v 1.6 2012/04/14 21:49:20 christos Exp $");
#endif
#endif

#include <stdlib.h>

#include "global.h"

void
pbignum(lispval current, FILE *useport)
{
	intptr_t      *digitp, *top, *bot, *work, negflag = 0;
	Keepxs();

	/* copy bignum onto stack */
	top = (sp()) - 1;
	do {
		stack(current->s.I);
	} while ((current = current->s.CDR) != NULL);

	bot = sp();
	if (top == bot) {
		fprintf(useport, "%jd", (intmax_t)*bot);
		Freexs();
		return;
	}
	/* save space for printed digits */
	work = alloca((top - bot) * 2 * sizeof(*work));
	if (*bot < 0) {
		negflag = 1;
		dsneg(top, bot);
	}
	/* figure out nine digits at a time by destructive division */
	for (digitp = work; bot <= top; digitp++) {
		*digitp = dodiv(top, bot);
		if (*bot == 0)
			bot += 1;
	}

	/* print them out */

	if (negflag)
		putc('-', useport);
	fprintf(useport, "%jd", (intmax_t)*--digitp);
	while (digitp > work)
		fprintf(useport, "%.09jd", (intmax_t)*--digitp);
	Freexs();
}
