/* Copyright (c) 1982, Regents, University of California */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: nargs.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include <stdlib.h>
#include "global.h"

int
nargs(intptr_t arg)	/* this is only here for address calculation */
{
	/* We don't support these hacks */
	abort();
	return (int)arg;
}
