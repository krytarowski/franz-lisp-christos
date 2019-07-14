/*-					-[Sat Jan 29 13:24:33 1983 by jkf]-
 * 	lisp.c
 * main program
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lisp.c,v 1.3 83/11/26 12:00:58 sklower Exp";
#else
__RCSID("$Id: lisp.c,v 1.6 2014/12/11 20:34:43 christos Exp $");
#endif
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "global.h"
#include "frame.h"

/* main **************************************************************** */
/* Execution of the lisp system begins here.  This is the top level	 */
/* executor which is an infinite loop.  The structure is similar to	 */
/* error.								 */

extern lispval  reborn;
extern int      rlevel;
static int      virgin = 0;
int             Xargc;
char          **Xargv;
char	       *progname;

int 
main (int argc, char **argv, char **arge)
{
	pbuf            pb;

#ifndef DEBUG
	setvbuf(stdout, NULL, _IOLBF, 0);
#else
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		progname++;
	Xargc = argc;
	Xargv = argv;
	virgin = 0;
	errp = (struct frame *) 0;
	initial();

	errp = Pushframe(F_RESET, nil, nil);
	switch (retval) {
	case C_RESET:
		break;		/* what to do? */
	case C_INITIAL:
		break;		/* first time  */
	}

	for (EVER) {
		lbot = np = orgnp;
		rlevel = 0;
		depth = 0;
		clearerr(piport = stdin);
		clearerr(poport = stdout);
		np++->val = matom("top-level");
		np++->val = nil;
		Lapply();
	}
}

static lispval
Ninit(void)
{
	FILE *fp;
	char path[BUFSIZ];
	snprintf(path, sizeof(path), "%s/startup/%s.l", LISPDIR, progname);

	if (((fp = fopen(path, "r"))) == NULL)
		return NULL;

	lbot = np;
	np++->val = P(fp);
	np++->val = eofa;
	while (TRUE) {
		dmpport(stdout);
		vtemp = Lread();
		if (vtemp == eofa) {
			fclose(fp);
			return NULL;
		}
		eval(vtemp);
	}
}

lispval
Ntpl(void)
{
	if (virgin == 0) {
		Ninit();
		fputs((char *) Istsrch(matom("version"))->d.cdr->d.cdr->d.cdr, poport);
		virgin = 1;
	}
	lbot = np;
	np++->val = P(stdin);
	np++->val = eofa;
	while (TRUE) {
		fputs("\n-> ", stdout);
		dmpport(stdout);
		vtemp = Lread();
		if (vtemp == eofa)
			exit(0);
		printr(eval(vtemp), stdout);
	}
}

/*
 * franzexit :: give up the ghost this function is called whenever one
 * decides to kill this process. We clean up a bit then call then standard
 * exit routine.  C code in franz should never call exit() directly.
 */
void 
franzexit(int code)
{
	extern int      fvirgin;
	extern char    *stabf;
	if (!fvirgin)
		unlink(stabf);	/* give up any /tmp symbol tables */
#ifdef notdef
	/*
	 * ??: is this something special?? christos: yes, the profiler
	 */
	_cleanup();
	proflush();
	_exit(code);
#else
	exit(code);
#endif
}
