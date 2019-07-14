#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: emul.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include <string.h>
#include "global.h"
int
emul(int p, int q, int r, int s[2])
{
	union {
		long long       l;
		int             s[2];
	}               x;
	x.l = p * q + r;
	s[1] = x.s[0];
	s[0] = x.s[1];
	return 0;
}
