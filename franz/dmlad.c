#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: dmlad.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

#define SNIL 0

int             big_debug = FALSE;

/* changed args to emul (see emul.c) */

void
dmlad(lispval ptr, int mul, int add)
{
	lispval         psave, penult;
	long            c;
	int             res[2];

	psave = ptr;
	c = add;
	do {
		emul((ptr->s.I), mul, c, res);

		/* convert from 32/32 to 32/30 */
		res[0] <<= 2;
		if (res[1] < 0)
			res[0] += 2;
		if (res[1] & 0x40000000)
			res[0] += 1;
		res[1] &= 0x3fffffff;

		c = res[0];
		ptr->s.I = res[1];
		penult = ptr;
		ptr = ptr->s.CDR;
	} while (ptr != SNIL);

	if (c != 0) {
		if (c == -1)
			penult->s.I |= 0xc0000000;
		else {
			ptr = penult->s.CDR = newdot();
			ptr->s.I = c;
		}
	}
	ptr = psave;
}
