/*-					-[Sat Jan 29 12:44:16 1983 by jkf]-
 * 	fpipe.c
 * pipe creation
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: fpipe.c,v 1.3 85/05/22 07:53:41 sklower Exp";
#else
__RCSID("$Id: fpipe.c,v 1.4 2012/02/12 20:08:39 christos Exp $");
#endif
#endif


#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "global.h"

static int      NiorUtil(FILE *);

FILE *
fpipe(FILE *info[2])
{
	FILE           *p;
	int             fd[2];

	if (0 > pipe(fd))
		return ((FILE *) - 1);

	if (NULL == (p = fdopen(fd[0], "r"))) {
		close(fd[0]);
		close(fd[1]);
		return ((FILE *) - 1);
	}
	info[0] = p;
	if (NULL == (p = fdopen(fd[1], "w"))) {
		close(fd[0]);
		close(fd[1]);
		return ((FILE *) - 1);
	}
	info[1] = p;

	return ((FILE *) 2);	/* indicate sucess */
}
/* Nioreset ************************************************************ */

lispval
Nioreset(void)
{
	extern int      _fwalk(int (*) (FILE *));

	_fwalk(NiorUtil);
	return (nil);
}

FILE            FILEdummy;

static int
NiorUtil(FILE *p)
{
	lispval         handy;
	if (p == stdin || p == stdout || p == stderr || p == &FILEdummy)
		return (0);
	fclose(p);
	handy = P(p);
	if (TYPE(handy) == PORT) {
		handy->p = &FILEdummy;
	}
	return 0;
}
FILE          **xports;

#define LOTS (LBPG/(sizeof (FILE *)))
lispval 
P(FILE *p)
{
	FILE          **q;

	if (xports == ((FILE **) 0)) {
		/* this is gross.  I don't want to change csegment -- kls */
		xports = (FILE **) csegment(OTHER, LOTS, 0);
		SETTYPE(xports, PORT, 31);
		for (q = xports; q < xports + LOTS; q++) {
			*q = &FILEdummy;
		}
	}
	for (q = xports; q < xports + LOTS; q++) {
		if (*q == p)
			return ((lispval) q);
		if (*q == &FILEdummy) {
			*q = p;
			return ((lispval) q);
		}
	}
	/* Heavens above knows this could be disasterous in makevals() */
	return error("Ran out of Ports", FALSE);
}

struct buf {
	char           *cur;
	char           *end;
};

static int
bufwrite(void *p, const char *s, int l)
{
	struct buf     *b = p;
	int             r = l;
	while (l > 0 && b->cur < b->end) {
		*(b->cur)++ = *s++;
		l--;
	}
	return r - l;
}

static int
bufread(void *p, char *s, int l)
{
	struct buf     *b = p;
	int             r = l;
	while (l > 0 && b->cur < b->end) {
		*s++ = *(b->cur)++;
		l--;
	}
	return r - l;
}

static int
bufclose(void *p)
{
	free(p);
	return 0;
}

FILE           *
fstopen(char *base, int count, char *flag)
{
	struct buf     *b = malloc(sizeof(*b));
	b->cur = base;
	b->end = base + count;
	return funopen(b, bufread, bufwrite, NULL, bufclose);
}
