/*-					-[Sat Apr  9 17:03:02 1983 by layer]-
 * 	fex3.c
 * nlambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: fex3.c,v 1.15 85/03/13 17:18:29 sklower Exp";
#else
__RCSID("$Id: fex3.c,v 1.16 2014/12/31 01:08:01 christos Exp $");
#endif
#endif

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "aout.h"
static int      pagsiz, pagrnd;


/*
 *Ndumplisp -- create executable version of current state of this lisp.
 */
#include <sys/types.h>

extern int      dmpmode;
extern lispval  reborn;
extern char     etext[];

lispval
Ndumplisp(void)
{
	struct exec    *workp;
	lispval         argsptr, temp;
	char           *fname;
	struct exec     work, old;
	int             descrip, des2, ax, mode;
	char            tbuf[BUFSIZ];
	off_t           count;
	extern void	_init(void);


	pageseql();
	pagsiz = Igtpgsz();
	pagrnd = pagsiz - 1;

#ifdef __NetBSD__
	{
		/* Round the break up to a multiple of a page size */
		int             excess;
		excess = (long) sbrk(0) & pagrnd;
		if (excess > 0)
			sbrk(pagsiz - excess);
	}
#endif

	/*
	 * dump mode is kept in decimal (which looks like octal in dmpmode)
	 * and is changeable via (sstatus dumpmode n) where n is 413 or 410
	 * base 10
	 */
	if (dmpmode == 413)
		mode = 0413;
	else if (dmpmode == 407)
		mode = 0407;
	else
		mode = 0410;

	workp = &work;
	workp->a_magic = mode;
#ifdef os_masscomp
	workp->a_stamp = 1;
#endif

#ifndef __APPLE__
	if (mode == 0407)
		workp->a_text = (unsigned long) etext - (unsigned long)_init;
	else
		workp->a_text = 1 + ((((unsigned long) etext) - 1
		    - ((unsigned long)_init)) | pagrnd);
#endif
	workp->a_data = (intptr_t)sbrk(0);
	workp->a_bss = 0;
	workp->a_syms = 0;
	workp->a_entry = (uintptr_t) gstart();
	workp->a_trsize = 0;
	workp->a_drsize = 0;

	fname = "savedlisp";	/* set defaults */
	reborn = CNIL;
	argsptr = lbot->val;
	if (argsptr != nil) {
		temp = argsptr->d.car;
		if ((TYPE(temp)) == ATOM)
			fname = temp->a.pname;
	}
	des2 = open(gstab(), 0);
	if (des2 >= 0) {
		if (read(des2, (char *) &old, sizeof(old)) >= 0)
			work.a_syms = old.a_syms;
	}
	descrip = creat(fname, 0777);	/* doit! */
	if (-1 == write(descrip, (char *) workp, sizeof(work))) {
		close(descrip);
		error("Dumplisp header failed", FALSE);
	}
	if (mode == 0413)
		lseek(descrip, (long) pagsiz, 0);
#ifndef __APPLE__
	if (-1 == write(descrip, (char *) _init, (int) workp->a_text)) {
		close(descrip);
		error("Dumplisp text failed", FALSE);
	}
#endif
	if (des2 > 0 && work.a_syms) {
		count = old.a_text + old.a_data
			+ (old.a_magic == 0413
			   ? pagsiz
			   : sizeof(old));
		if (-1 == lseek(des2, count, 0))
			error("Could not seek to stab", FALSE);
		for (count = old.a_syms; count > 0; count -= BUFSIZ) {
			ax = read(des2, tbuf, (int) (count < BUFSIZ ? count : BUFSIZ));
			if (ax == 0) {
				printf("Unexpected end of syms %lld", (long long)count);
				fflush(stdout);
				break;
			} else if (ax > 0)
				write(descrip, tbuf, ax);
			else
				error("Failure to write dumplisp stab", FALSE);
		}
#if ! (os_unix_ts | os_unisoft)
		if (-1 == lseek(des2, (long)
				((old.a_magic == 0413 ? pagsiz : sizeof(old))
				 + old.a_text + old.a_data
				 + old.a_trsize + old.a_drsize + old.a_syms),
				0))
			error(" Could not seek to string table ", FALSE);
		for (ax = 1; ax > 0;) {
			ax = read(des2, tbuf, BUFSIZ);
			if (ax > 0)
				write(descrip, tbuf, ax);
			else if (ax < 0)
				error("Error in string table read ", FALSE);
		}
#endif
	}
	close(descrip);
	if (des2 > 0)
		close(des2);
	reborn = 0;

	pagenorm();

	return (nil);
}

#ifndef __linux__
#include <sys/vadvise.h>
#endif

#ifdef __APPLE__
#define vadvise(a)
#endif

void            pagerand(void)
{
#ifdef VA_ANOM
	vadvise(VA_ANOM);
#endif
}
void            pageseql(void)
{
#ifdef VA_SEQL
	vadvise(VA_SEQL);
#endif
}
void            pagenorm(void)
{
#ifdef VA_NORM
	vadvise(VA_NORM);
#endif
}

/*
 * getaddress --
 * 
 * (getaddress '|_entry1| 'fncname1 '|_entry2| 'fncname2 ...)
 * 
 * binds value of symbol |_entry1| to function defition of atom fncname1, etc.
 * 
 * returns fnc-binding of fncname1.
 * 
 */

lispval
Lgetaddress(void)
{
	struct argent  *mlbot = lbot;
	lispval         work;
	int             numberofargs, i;
	char            ostabf[128];
	struct nlist    NTABLE[100];

	Savestack(4);

	if (np - lbot == 2)
		protect(nil);	/* allow 2 args */
	numberofargs = (np - lbot) / 3;
	if (numberofargs * 3 != np - lbot)
		error("getaddress: arguments must come in triples ", FALSE);

	for (i = 0; i < numberofargs; i++, mlbot += 3) {
		NTABLE[i].n_value = 0;
		mlbot[0].val = verifyl(mlbot[0].val, "Incorrect entry specification for binding");
		STASSGN(i, (char *) mlbot[0].val);
		while (TYPE(mlbot[1].val) != ATOM)
			mlbot[1].val = errorh1(Vermisc,
				     "Bad associated atom name for binding",
					       nil, TRUE, 0, mlbot[1].val);
		mlbot[2].val = dispget(mlbot[2].val, "getaddress: Incorrect discipline specification ", (lispval) Vsubrou->a.pname);
	}
	STASSGN(numberofargs, "");
	strncpy(ostabf, gstab(), 128);
#ifndef __APPLE__
	if (nlist(ostabf, NTABLE) == -1) {
		errorh1(Vermisc, "Getaddress: Bad file", nil, FALSE, 0, (lispval) inewstr(ostabf));
	} else
		for (i = 0, mlbot = lbot + 1; i < numberofargs; i++, mlbot += 3) {
			if (NTABLE[i].n_value == 0)
				printf("Undefined symbol: %s\n",
					NTABLE[i].N_name);
			else {
				work = newfunct();
				work->bcd.start = (lispval(*) (void)) NTABLE[i].n_value;
				work->bcd.discipline = mlbot[1].val;
				mlbot->val->a.fnbnd = work;
			}
		}
#endif
	Restorestack();
	return (lbot[1].val->a.fnbnd);
};

int
Igtpgsz(void)
{
	return (getpagesize());
}
