/*-					-[Fri Aug  5 12:51:31 1983 by jkf]-
 * 	lam7.c
 * lambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lam7.c,v 1.9 87/12/14 18:48:02 sklower Exp";
#else
__RCSID("$Id: lam7.c,v 1.10 2014/12/11 17:12:17 christos Exp $");
#endif
#endif

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#include "global.h"



lispval
Lfork(void)
{
	int             pid;

	chkarg(0, "fork");
	if ((pid = fork())) {
		return (inewint(pid));
	} else
		return (nil);
}

lispval
Lwait(void)
{
	lispval         ret, temp;
	int             status = -1, pid;
	Savestack(2);


	chkarg(0, "wait");
	pid = wait(&status);
	ret = newdot();
	protect(ret);
	temp = inewint(pid);
	ret->d.car = temp;
	temp = inewint(status);
	ret->d.cdr = temp;
	Restorestack();
	return (ret);
}

lispval
Lpipe(void)
{
	lispval         ret, temp;
	int             pipes[2];
	Savestack(2);

	chkarg(0, "pipe");
	pipes[0] = -1;
	pipes[1] = -1;
	pipe(pipes);
	ret = newdot();
	protect(ret);
	temp = inewint(pipes[0]);
	ret->d.car = temp;
	temp = inewint(pipes[1]);
	ret->d.cdr = temp;
	Restorestack();
	return (ret);
}

lispval
Lfdopen(void)
{
	lispval         fd, type;
	FILE           *ptr;

	chkarg(2, "fdopen");
	type = (np - 1)->val;
	fd = lbot->val;
	if (TYPE(fd) != INT)
		return (nil);
	if ((ptr = fdopen((int) fd->i, (char *) type->a.pname)) == NULL)
		return (nil);
	return (P(ptr));
}

lispval
Lexece(void)
{
	lispval         fname, arglist, envlist, temp;
	char           *args[100], *envs[100], estrs[1024];
	char           *p, *cp, **argsp;

	fname = nil;
	arglist = nil;
	envlist = nil;

	switch (np - lbot) {
	case 3:
		envlist = lbot[2].val;
	case 2:
		arglist = lbot[1].val;
	case 1:
		fname = lbot[0].val;
	case 0:
		break;
	default:
		return argerr("exece");
	}

	while (TYPE(fname) != ATOM)
		fname = error("exece: non atom function name", TRUE);
	while (TYPE(arglist) != DTPR && arglist != nil)
		arglist = error("exece: non list arglist", TRUE);
	for (argsp = args; arglist != nil; arglist = arglist->d.cdr) {
		temp = arglist->d.car;
		if (TYPE(temp) != ATOM)
			return error("exece: non atom argument seen", FALSE);
		*argsp++ = temp->a.pname;
	}
	*argsp = 0;
	if (TYPE(envlist) != DTPR && envlist != nil)
		return (nil);
	for (argsp = envs, cp = estrs; envlist != nil; envlist = envlist->d.cdr) {
		temp = envlist->d.car;
		if (TYPE(temp) != DTPR || TYPE(temp->d.car) != ATOM
		    || TYPE(temp->d.cdr) != ATOM)
			return error("exece: Bad enviroment list", FALSE);
		*argsp++ = cp;
		for (p = temp->d.car->a.pname; (*cp++ = *p++););
		*(cp - 1) = '=';
		for (p = temp->d.cdr->a.pname; (*cp++ = *p++););
	}
	*argsp = 0;

	return (inewint(execve(fname->a.pname, args, envs)));
}

/*
 * Lprocess - C code to implement the *process function call: (*process
 * 'st_command ['s_readp ['s_writep]]) where st_command is the command to
 * execute s_readp is non nil if you want a port to read from returned
 * s_writep is non nil if you want a port to write to returned both flags
 * default to nil *process returns the exit status of the process if s_readp
 * and s_writep not given (in this case the parent waits for the child to
 * finish) a list of (readport writeport childpid) if one of s_readp or
 * s_writep is given.  If only s_readp is non nil, then writeport will be
 * nil, If only s_writep is non nil, then readport will be nil
 */

lispval
Lprocess(void)
{
	int             wflag, childsi, childso, child;
	lispval         handy;
	char           *command, *p;
	int             writep, readp;
	int             itmp;
	sig_t           hdlr;
	FILE           *bufs[2], *obufs[2];
	Savestack(0);

	writep = readp = FALSE;
	wflag = TRUE;

	switch (np - lbot) {
	case 3:
		if (lbot[2].val != nil)
			writep = TRUE;
	case 2:
		if (lbot[1].val != nil)
			readp = TRUE;
		wflag = 0;
	case 1:
		command = verify(lbot[0].val, "*process: non atom first arg");
		break;
	default:
		return argerr("*process");
	}

	childsi = 0;
	childso = 1;

	/*
	 * if there will be communication between the processes, it will be
	 * through these pipes: parent ->  bufs[1] ->  bufs[0] -> child    if
	 * writep parent <- obufs[0] <- obufs[1] <- parent   if readp
	 */
	if (writep) {
		fpipe(bufs);
		childsi = fileno(bufs[0]);
	}
	if (readp) {
		fpipe(obufs);
		childso = fileno(obufs[1]);
	}
	hdlr = signal(SIGINT, SIG_IGN);
	if ((child = vfork()) == 0) {
		/*
		 * if we will wait for the child to finish and if the process
		 * had ignored interrupts before we were called, then leave
		 * them ignored, else set it back the the default (death)
		 */
		if (wflag && hdlr != SIG_IGN)
			signal(SIGINT, SIG_DFL);

		if (writep) {
			close(0);
			dup(childsi);
		}
		if (readp) {
			close(1);
			dup(childso);
		}
		if ((p = getenv("SHELL")) != NULL) {
			execlp(p, p, "-c", command, NULL);
			_exit(-1);	/* if exec fails, signal problems */
		} else {
			execlp("csh", "csh", "-c", command, NULL);
			execlp("sh", "sh", "-c", command, NULL);
			_exit(-1);	/* if exec fails, signal problems */
		}
	}
	/*
	 * close the duplicated file descriptors e.g. if writep is true then
	 * we've created two desriptors, bufs[0] and bufs[1],  we will write
	 * to bufs[1] and the child (who has a copy of our bufs[0]) will read
	 * from bufs[0] We (the parent) close bufs[0] since we will not be
	 * reading from it.
	 */
	if (writep)
		fclose(bufs[0]);
	if (readp)
		fclose(obufs[1]);

	if (wflag && child != -1) {
		int             status = 0;
		/* we await the death of the child */
		while (wait(&status) != child) {
		}
		/* the child has died */
		signal(2, hdlr);	/* restore the interrupt handler */
		itmp = status >> 8;
		Restorestack();
		return (inewint(itmp));	/* return its status */
	}
	/*
	 * we are not waiting for the childs death build a list containing
	 * the write and read ports
	 */
	protect(handy = newdot());
	handy->d.cdr = newdot();
	handy->d.cdr->d.cdr = newdot();
	if (readp) {
		handy->d.car = P(obufs[0]);
		ioname[PN(obufs[0])] = (lispval) inewstr((char *) "from-process");
	}
	if (writep) {
		handy->d.cdr->d.car = P(bufs[1]);
		ioname[PN(bufs[1])] = (lispval) inewstr((char *) "to-process");
	}
	handy->d.cdr->d.cdr->d.car = (lispval) inewint(child);
	signal(SIGINT, hdlr);
	Restorestack();
	return (handy);
}

extern int      gensymcounter;

lispval
Lgensym(void)
{
	lispval         arg;
	char            leader;

	switch (np - lbot) {
	case 0:
		arg = nil;
		break;
	case 1:
		arg = lbot->val;
		break;
	default:
		return argerr("gensym");
	}
	leader = 'g';
	if (arg != nil && TYPE(arg) == ATOM)
		leader = arg->a.pname[0];
	snprintf(strbuf, estrbuf - strbuf, "%c%05d", leader, gensymcounter++);
	atmlen = 7;
	return ((lispval) newatom(0));
}

extern struct types {
	char           *next_free;
	int             space_left, space, type, type_len;	/* note type_len is in
								 * units of int */
	lispval        *items, *pages, *type_name;
	struct heads
	               *first;
}               atom_str;

lispval
Lremprop(void)
{
	struct argent  *argp;
	lispval         pptr, ind, opptr;
	lispval         atm;
	int             disemp = FALSE;

	chkarg(2, "remprop");
	argp = lbot;
	ind = argp[1].val;
	atm = argp->val;
	switch (TYPE(atm)) {
	case DTPR:
		pptr = atm->d.cdr;
		disemp = TRUE;
		break;
	case ATOM:
		if ((lispval) atm == nil)
			pptr = nilplist;
		else
			pptr = atm->a.plist;
		break;
	default:
		return errorh1(Vermisc, "remprop: Illegal first argument :",
			       nil, FALSE, 0, atm);
	}
	opptr = nil;
	if (pptr == nil)
		return (nil);
	while (TRUE) {
		if (TYPE(pptr->d.cdr) != DTPR)
			return errorh1(Vermisc, "remprop: Bad property list",
				       nil, FALSE, 0, atm);
		if (pptr->d.car == ind) {
			if (opptr != nil)
				opptr->d.cdr = pptr->d.cdr->d.cdr;
			else if (disemp)
				atm->d.cdr = pptr->d.cdr->d.cdr;
			else if (atm == nil)
				nilplist = pptr->d.cdr->d.cdr;
			else
				atm->a.plist = pptr->d.cdr->d.cdr;
			return (pptr->d.cdr);
		}
		if ((pptr->d.cdr)->d.cdr == nil)
			return (nil);
		opptr = pptr->d.cdr;
		pptr = (pptr->d.cdr)->d.cdr;
	}
}

lispval
Lbcdad(void)
{
	lispval         ret, temp;

	chkarg(1, "bcdad");
	temp = lbot->val;
	if (TYPE(temp) != ATOM)
		return error("ONLY ATOMS HAVE FUNCTION BINDINGS", FALSE);
	temp = temp->a.fnbnd;
	if (TYPE(temp) != BCD)
		return (nil);
	ret = newint();
	ret->i = (intptr_t) temp;
	return (ret);
}

lispval
Lstringp(void)
{
	chkarg(1, "stringp");
	if (TYPE(lbot->val) == STRNG)
		return (tatom);
	return (nil);
}

lispval
Lsymbolp(void)
{
	chkarg(1, "symbolp");
	if (TYPE(lbot->val) == ATOM)
		return (tatom);
	return (nil);
}

lispval
Lrematom(void)
{
	lispval         temp;

	chkarg(1, "rematom");
	temp = lbot->val;
	if (TYPE(temp) != ATOM)
		return (nil);
	temp->a.fnbnd = nil;
	temp->a.pname = (char *) CNIL;
	temp->a.plist = nil;
	(atom_items->i)--;
	(atom_str.space_left)++;
	temp->a.clb = (lispval) atom_str.next_free;
	atom_str.next_free = (char *) temp;
	return (tatom);
}

#define QUTMASK 0200
#define VNUM 0000

lispval
Lprname(void)
{
	lispval         a, ret;
	lispval         work, prev;
	char           *front, *temp;
	int             clean;
	char            ctemp[100];
	Savestack(2);

	chkarg(1, "prname");
	a = lbot->val;
	switch (TYPE(a)) {
	case INT:
		snprintf(ctemp, sizeof(ctemp), "%jd", (intmax_t)a->i);
		break;

	case DOUB:
		snprintf(ctemp, sizeof(ctemp), "%f", a->r);
		break;

	case ATOM:
		temp = front = a->a.pname;
		clean = *temp;
		if (*temp == '-')
			temp++;
		clean = clean && (ctable[(unsigned char) *temp] != VNUM);
		while (clean && *temp)
			clean = (!(ctable[(unsigned char) *temp++] & QUTMASK));
		if (clean)
			strlcpy(ctemp, front, sizeof(ctemp));
		else
			snprintf(ctemp, sizeof(ctemp), "\"%s\"", front);
		break;

	default:
		return error("prname does not support this type", FALSE);
	}
	temp = ctemp;
	protect(ret = prev = newdot());
	while (*temp) {
		prev->d.cdr = work = newdot();
		strbuf[0] = *temp++;
		strbuf[1] = 0;
		work->d.car = getatom(FALSE);
		work->d.cdr = nil;
		prev = work;
	}
	Restorestack();
	return (ret->d.cdr);
}

lispval
Lexit(void)
{
	lispval         handy;
	if (np - lbot == 0)
		franzexit(0);
	handy = lbot->val;
	if (TYPE(handy) == INT)
		franzexit((int) handy->i);
	franzexit(-1);
}
lispval
Iimplode(int unintern)
{
	lispval         handy, work;
	char           *cp = strbuf;

	chkarg(1, "implode");
	for (handy = lbot->val; handy != nil; handy = handy->d.cdr) {
		work = handy->d.car;
		if (cp >= estrbuf)
			cp = atomtoolong(cp);
again:
		switch (TYPE(work)) {
		case ATOM:
			*cp++ = work->a.pname[0];
			break;
		case SDOT:
			*cp++ = work->s.I;
			break;
		case INT:
			*cp++ = work->i;
			break;
		case STRNG:
			*cp++ = *(char *) work;
			break;
		default:
			work = errorh1(Vermisc, "implode/maknam: Illegal type for this arg:", nil, FALSE, 44, work);
			goto again;
		}
	}
	*cp = 0;
	if (unintern)
		return ((lispval) newatom(FALSE));
	else
		return ((lispval) getatom(FALSE));
}

lispval
Lmaknam(void)
{
	return (Iimplode(TRUE));/* unintern result */
}

lispval
Limplode(void)
{
	return (Iimplode(FALSE));	/* intern result */
}

lispval
Lntern(void)
{
	int             hashv;
	lispval         handy, atpr;


	chkarg(1, "intern");
	if (TYPE(handy = lbot->val) != ATOM)
		return errorh1(Vermisc, "non atom to intern ", nil, FALSE, 0, handy);
	/* compute hash of pname of arg */
	hashv = hashfcn(handy->a.pname);

	/* search for atom with same pname on hash list */

	atpr = (lispval) hasht[hashv];
	for (atpr = (lispval) hasht[hashv];
	     atpr != CNIL
	     ; atpr = (lispval) atpr->a.hshlnk) {
		if (strcmp(atpr->a.pname, handy->a.pname) == 0)
			return (atpr);
	}

	/* not there yet, put the given one on */

	handy->a.hshlnk = hasht[hashv];
	hasht[hashv] = (struct atom *) handy;
	return (handy);
}

/*** Ibindvars :: lambda bind values to variables
	called with a list of variables and values.
	does the special binding and returns a fixnum which represents
	the value of bnp before the binding
	Use by compiled progv's.
 ***/
lispval
Ibindvars(void)
{
	lispval         vars, vals, handy;
	struct nament  *oldbnp = bnp;

	chkarg(2, "int:bindvars");

	vars = lbot[0].val;
	vals = lbot[1].val;

	if (vars == nil)
		return (inewint((long) oldbnp));

	if (TYPE(vars) != DTPR)
		return errorh1(Vermisc, "progv (int:bindvars): bad first argument ", nil,
			       FALSE, 0, vars);
	if ((vals != nil) && (TYPE(vals) != DTPR))
		return errorh1(Vermisc, "progv (int:bindvars): bad second argument ", nil,
			       FALSE, 0, vals);

	for (; vars != nil; vars = vars->d.cdr, vals = vals->d.cdr) {
		handy = vars->d.car;
		if (TYPE(handy) != ATOM)
			return errorh1(Vermisc, "progv (int:bindvars): non symbol argument to bind ",
				       nil, FALSE, 0, handy);
		PUSHDOWN(handy, vals->d.car);
	}
	return (inewint((long) oldbnp));
}


/*** Iunbindvars :: unbind the variable stacked by Ibindvars
     called by compiled progv's
 ***/

lispval
Iunbindvars(void)
{
	struct nament  *oldbnp;

	chkarg(1, "int:unbindvars");
	oldbnp = (struct nament *) (lbot[0].val->i);
	if ((oldbnp < orgbnp) || (oldbnp > bnp))
		return errorh1(Vermisc, "int:unbindvars: bad bnp value given ", nil, FALSE, 0,
			       lbot[0].val);
	popnames(oldbnp);
	return (nil);
}

/*
 * (time-string ['x_milliseconds])
 * if given no argument, returns the current time as a string
 * if given an argument which is a fixnum representing the current time
 * as a fixnum, it generates a string from that
 *
 * the format of the string returned is that defined in the Unix manual
 * except the trailing newline is removed.
 *
 */
lispval
Ltymestr(void)
{
	time_t          timevalue;
	char           *rval;

	switch (np - lbot) {
	case 0:
		time(&timevalue);
		break;
	case 1:
		while (TYPE(lbot[0].val) != INT)
			lbot[0].val =
				errorh1(Vermisc, "time-string: non fixnum argument ",
					nil, TRUE, 0, lbot[0].val);
		timevalue = lbot[0].val->i;
		break;
	default:
		return argerr("time-string");
	}

	rval = ctime(&timevalue);
	/* remove newline character */
	rval[strlen(rval) - 1] = '\0';
	return ((lispval) inewstr(rval));
}
