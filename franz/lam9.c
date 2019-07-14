/*-					-[Sat Oct  1 19:44:47 1983 by jkf]-
 * 	lam9.c
 * lambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lam9.c,v 1.7 85/03/13 17:19:15 sklower Exp";
#else
__RCSID("$Id: lam9.c,v 1.12 2014/12/25 23:57:55 christos Exp $");
#endif
#endif

/*
 * These routines writen in C will allow use of the termcap file
 * by any lisp program. They are very basic routines which initialize
 * termcap and allow the lisp to execute any of the termcap functions.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>		/* add definations for I/O and bandrate */
#ifdef BSD4_4
#include <sgtty.h>
#endif
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <termcap.h>
#include <string.h>
#include <pwd.h>

#include "global.h"

#undef putchar

char            bpbuf[1024];
char            tstrbuf[100];
static int      show(char *, int *, int *);

/*
 *	This routine will initialize the termcap for the lisp programs.
 *	If the termcap file is not found, or terminal type is undefined,
 *	it will print out an error mesg.
 */

lispval
Ltci(void)
{
	char           *cp = getenv("TERM");
	char           *pc;
	int             found;
#ifdef BSD4_4
	struct sgttyb   tty;
#endif

	found = tgetent(bpbuf, cp);	/* open ther termcap file */
	switch (found) {
	case -1:
		printf("\nError Termcap File not found \n");
		break;
	case 0:
		printf("\nError No Termcap Entry for this terminal \n");
		break;
	case 1:{		/* everything was ok	 */
#ifdef BSD4_4
#ifdef TIOCGETP
			ioctl(1, TIOCGETP, &tty);
#else
			gtty(1, &tty);
#endif
			ospeed = tty.sg_ospeed;
#endif
		}
		break;
	}
	cp = tstrbuf;
	BC = tgetstr("bc", &cp);
	UP = tgetstr("up", &cp);
	pc = tgetstr("pc", &cp);
	if (pc)
		PC = *pc;
	return (nil);
}
/*
 * This routine will execute any of the termcap functions used by the lisp
 * program. If the feature is not include in the terminal defined it will
 * ignore the call.
 *		option	: feature to execute
 *		line	: line if is nessery
 *		colum	: colum if is nessaery
 *
 */
lispval
Ltcx(void)
{
	struct argent  *mylbot = lbot;
	int             line = 0, column = 0;

	switch (np - lbot) {
	case 1:
		break;
	case 2:
		error("Wrong number of Arguments to Termcapexecute", FALSE);
		break;
	case 3:
		line = mylbot[1].val->i;
		column = mylbot[2].val->i;
	}
	return (inewint(show((char *) mylbot->val, &line, &column)));
}


static int
show(char *option, int *line, int *colum)
{
	int             found;
	char            clbuf[20];
	char           *clbp = clbuf;
	char           *clear;

	/* the tegetflag doesnot work ? */
	clear = tgetstr(option, &clbp);
#ifdef DEBUG
	printf("option = %s, %s \n", clear, option);
#endif
	if (!clear) {
		found = tgetnum(option);
		if (found)
			return (found);
		return (-1);
	}
	PC = ' ';
	if (strcmp(option, "cm") == 0) {	/* if cursor motion, do it */
		clear = tgoto(clear, *colum, *line);
		if (*clear == 'O')
			clear = 0;
	}
	if (clear)		/* execute the feature */
		tputs(clear, 0, putchar);
	return (0);
}



/*
 * LIfranzcall :: lisp function int:franz-call
 *   this function serves many purposes.  It provides access to
 *   those things that are best done in C or which required a
 *   C access to unix system calls.
 *
 *   Calls to this routine are not error checked, for the most part
 *   because this is only called from trusted lisp code.
 *
 *   The functions in this file may or may not be documented in the manual.
 *   See the lisp interface to this function for more details. (common2.l)
 *
 *  the first argument is always a fixnum index, the other arguments
 *   depend on the function.
 */

#define fc_getpwnam 1
#define fc_access   2
#define fc_chdir    3
#define fc_unlink   4
#define fc_time	    5
#define fc_chmod    6
#define fc_getpid   7
#define fc_stat     8
#define fc_gethostname 9
#define fc_link     10
#define fc_sleep    11
#define fc_nice	    12

lispval
LIfranzcall(void)
{
	lispval         handy;

	if ((np - lbot) <= 0)
		return argerr("int:franz-call");

	switch (lbot[0].val->i) {

	case fc_getpwnam:
		/*
		 * arg 1 = user name return vector of name, uid, gid, dir or
		 * nil if doesn't exist.
		 */
		{
			struct argent  *oldnp;
			struct passwd  *pw;

			pw = getpwnam(verify(lbot[1].val, "int:franz-call: invalid name"));
			if (pw) {
				handy = newvec(4 * sizeof(long));
				oldnp = np;
				protect(handy);
				handy->v.vector[0] = (lispval) inewstr(pw->pw_name);
				handy->v.vector[1] = inewint(pw->pw_uid);
				handy->v.vector[2] = inewint(pw->pw_gid);
				handy->v.vector[3] = (lispval) inewstr(pw->pw_dir);
				np = oldnp;
				return (handy);
			}
			return (nil);
		}
	case fc_access:
		return (inewint
			(access(verify(lbot[1].val, "i:fc,access: non string"),
			lbot[2].val->i)));
	case fc_chdir:
		return (inewint
		(chdir(verify(lbot[1].val, "i:fc,chdir: non string"))));

	case fc_unlink:
		return (inewint
			(unlink(verify(lbot[1].val, "i:fc,unlink: non string"))));

	case fc_time:
		return (inewint(time(0)));

	case fc_chmod:
		return (inewint(chmod(verify(lbot[1].val,
					     "i:fc,chmod: non string"),
				      lbot[2].val->i)));

	case fc_getpid:
		return (inewint(getpid()));

	case fc_stat:
		{
			struct argent  *oldnp;
			struct stat     statbuf;

			if (stat(verify(lbot[1].val, "ifc:stat bad file name "),
				 &statbuf)
			    != 0)
				return (nil);	/* nil on error */
			handy = newvec(12 * sizeof(long));
			oldnp = np;
			protect(handy);
			handy->v.vector[0] = inewint(statbuf.st_mode & 07777);
			handy->v.vector[1] = inewint(
					  (statbuf.st_mode & S_IFMT) >> 12);
			handy->v.vector[2] = inewint(statbuf.st_nlink);
			handy->v.vector[3] = inewint(statbuf.st_uid);
			handy->v.vector[4] = inewint(statbuf.st_gid);
			handy->v.vector[5] = inewint(statbuf.st_size);
			handy->v.vector[6] = inewint(statbuf.st_atime);
			handy->v.vector[7] = inewint(statbuf.st_mtime);
			handy->v.vector[8] = inewint(statbuf.st_ctime);
			handy->v.vector[9] = inewint(statbuf.st_dev);
			handy->v.vector[10] = inewint(statbuf.st_rdev);
			handy->v.vector[11] = inewint(statbuf.st_ino);
			np = oldnp;
			return (handy);
		}
	case fc_gethostname:
		{
			char            hostname[32];
			gethostname(hostname, sizeof(hostname));
			return ((lispval) inewstr(hostname));
		}
	case fc_link:
		return (inewint
		    (link(verify(lbot[1].val, "i:fc,link: non string"),
		       verify(lbot[2].val, "i:fc,link: non string"))));

		/* sleep for the given number of seconds */
	case fc_sleep:
		return (inewint(sleep(lbot[1].val->i)));

	case fc_nice:
		return (inewint(nice(lbot[1].val->i)));

	default:
		return (inewint(-1));
	}			/* end of switch */
}
