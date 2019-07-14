#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: ediv.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include <string.h>
#include "global.h"
int
ediv(int p[2], int q, char *err)
{
	struct {
		long long l;
		int i[2];
	} x;
	memset(&x, 0, sizeof(x));
	x.i[0] = p[0];
	x.i[1] = p[1];
	p[0] = x.l % q;
	p[1] = x.l / q;
	return p[1];
}
