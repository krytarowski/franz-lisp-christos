/* Copyright (c) 1982, Regents, University of California */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: inewint.c,v 1.1 2014/12/11 17:12:17 christos Exp $");
#endif

#include "global.h"

lispval 
inewint(int n)
{
	lispval         ip;
	if (n < 1024 && n >= -1024)
		return SMALL(n);
	ip = newint();
	ip->i = n;
	return (ip);
}
