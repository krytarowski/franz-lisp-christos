/*-					-[Sat Jan 29 13:07:37 1983 by jkf]-
 * 	lamgc.c
 * file used to meter gc, not always loaded
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lamgc.c,v 1.5 84/03/31 22:34:28 layer Exp";
#else
__RCSID("$Id: lamgc.c,v 1.6 2012/02/12 22:38:54 christos Exp $");
#endif
#endif

#include <sys/types.h>
#ifdef METER
#include <sys/vtimes.h>
#endif
#include <fcntl.h>
#include <unistd.h>

#include "global.h"
#include "gc.h"

/*
 * this file  is temporary and will contain routines to meter the garbage
 * collector
 */

/*
 * gcstat - temporary routine used to report on gc statistics. if this causes
 * variables to be undefined,then it should be removed
 */

extern int     *beginsweep, gensymcounter;
int             gcstat;
int             mrkdpcnt;
int             gccount;
int             conssame;
int             consdiff;
int             consnil;


/*
 *	gcstat  - initiate and record gc statistics
 * calls:
 *  	(gcstat 0) -- initiate gc statistics by creating gc.out
 *		      and writing header.
 *	(gcstat 1) -- finish off gc statistics file by writing typetable
 *		      and closing file.
 */
lispval
Lgcstat(void)
{
	lispval         handy;
	int             nbytes;
	struct gchead   hhh;

	chkarg(1, "gcstat");

	if (TYPE(handy = lbot->val) != INT) {
		error("gcstat: non integer arg ", FALSE);
	}
	switch (handy->i) {
	case 0:
		if ((gcstat = creat("gc.out", 0644)) < 0)
			error("cant open gc.out", FALSE);
		hhh.version = 5;
		hhh.lowdata = (intptr_t) beginsweep;
		printf("writing %zu bytes \n", sizeof(hhh));
		write(gcstat, (char *) &hhh, sizeof(hhh));
		gccount = 0;
		return (tatom);

	case 1:
		/* first write out the type table */
		nbytes = 0;
		/* 0 means type table follows */
		printf("gc's %d, writing %zu bytes \n", gccount,
		       sizeof(nbytes));
		write(gcstat, (char *) &nbytes, sizeof(nbytes));
		write(gcstat, (char *) &typetable[ATOX(beginsweep) + 1],
		      nbytes = ((intptr_t) datalim - (intptr_t) beginsweep) >> LSHIFT);
		printf("writing %zu bytes \n", nbytes + sizeof(nbytes));
		write(gcstat, (char *) &nbytes, sizeof(nbytes));
		close(gcstat);
		gcstat = 0;
		return (inewint(nbytes));
	default:
		return error("Bad value to gcstat ", TRUE);
	}
}
extern int      bitmapi[];	/* a bit of a lie it is really a double array */
char           *bitmapc = (char *) bitmapi;
/*
 * called in the garbage collector after the bit maps have been made only if
 * gcstat is non zero
 */
void
gcdump(void)
{
#ifdef notyet
	extern struct vtimes premark, presweep, alldone;
	int             nbytes, recsize;
	/* 16 bytes/page in the bitmap */
	nbytes = (((intptr_t) datalim - (intptr_t) beginsweep) >> LSHIFT) * 16;
	recsize = nbytes + 6 * sizeof(intptr_t) + 3 * sizeof(struct vtimes);
	write(gcstat, (char *) &recsize, sizeof(recsize));	/* whole record size */
	write(gcstat, (char *) &premark, sizeof(premark));
	write(gcstat, (char *) &presweep, sizeof(presweep));
	write(gcstat, (char *) &alldone, sizeof(alldone));
	write(gcstat, (char *) &gensymcounter, sizeof(intptr_t));
	write(gcstat, (char *) &conssame, sizeof(intptr_t));
	write(gcstat, (char *) &consdiff, sizeof(intptr_t));
	write(gcstat, (char *) &consnil, sizeof(intptr_t));
	write(gcstat, (char *) &mrkdpcnt, sizeof(intptr_t));
	write(gcstat, (char *) &nbytes, sizeof(nbytes));	/* bit table size */
	write(gcstat, (char *) &bitmapc[ATOX(beginsweep) * 16], nbytes);
	printf("gc: %d, written %d bytes\n", ++gccount, nbytes);
#endif
}
