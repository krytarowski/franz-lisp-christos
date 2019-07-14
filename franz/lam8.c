/*-					-[Thu Sep 29 22:24:10 1983 by jkf]-
 * 	lam8.c
 * lambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lam8.c,v 1.17 87/12/14 18:48:09 sklower Exp";
#else
__RCSID("$Id: lam8.c,v 1.14 2014/12/11 17:12:17 christos Exp $");
#endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include "global.h"
#include "frame.h"

/* various functions from the c math library */
extern int      current;

lispval 
Imath(double (*func)(double))
{
	lispval         handy;
	double          res;
	chkarg(1, "Math functions");

	switch (TYPE(handy = lbot->val)) {
	case INT:
		res = func((double) handy->i);
		break;

	case DOUB:
		res = func(handy->r);
		break;

	default:
		return error("Non fixnum or flonum to math function", FALSE);
	}
	handy = newdoub();
	handy->r = res;
	return (handy);
}
lispval 
Lsin(void)
{
	return (Imath(sin));
}

lispval 
Lcos(void)
{
	return (Imath(cos));
}

lispval 
Lasin(void)
{
	return (Imath(asin));
}

lispval 
Lacos(void)
{
	return (Imath(acos));
}

lispval 
Lsqrt(void)
{
	return (Imath(sqrt));
}
lispval 
Lexp(void)
{
	return (Imath(exp));
}

lispval 
Llog(void)
{
	return (Imath(log));
}

/*
 * although we call this atan, it is really atan2 to the c-world, that is, it
 * takes two args
 */
lispval 
Latan(void)
{
	lispval         arg;
	double          arg1v;
	double          res;
	chkarg(2, "arctan");

	switch (TYPE(arg = lbot->val)) {

	case INT:
		arg1v = (double) arg->i;
		break;

	case DOUB:
		arg1v = arg->r;
		break;

	default:
		return error("Non fixnum or flonum arg to atan2", FALSE);
	}

	switch (TYPE(arg = (lbot + 1)->val)) {

	case INT:
		res = atan2(arg1v, (double) arg->i);
		break;

	case DOUB:
		res = atan2(arg1v, arg->r);
		break;

	default:
		return error("Non fixnum or flonum to atan2", FALSE);
	}
	arg = newdoub();
	arg->r = res;
	return (arg);
}

/*
 * (random) returns a fixnum in the range -2**30 to 2**30 -1 (random fixnum)
 * returns a fixnum in the range 0 to fixnum-1
 */
lispval
Lrandom(void)
{
	int             curval;
	curval = rand();	/* get numb from 0 to 2**31-1 */

	if (np == lbot)
		return (inewint(curval - (int) pow((double) 2, (double) 30)));

	if ((TYPE(lbot->val) != INT)
	    || (lbot->val->i <= 0))
		errorh1(Vermisc, "random: non fixnum arg:",
			nil, FALSE, 0, lbot->val);

	return (inewint(curval % lbot->val->i));

}
lispval
Lmakunb(void)
{
	lispval         work;

	chkarg(1, "makunbound");
	work = lbot->val;
	if (work == nil || (TYPE(work) != ATOM))
		return (work);
	work->a.clb = nil;
	return (work);
}

lispval
Lfseek(void)
{

	FILE           *f;
	long            offset, whence;
	lispval         retp;

	chkarg(3, "fseek");	/* Make sure there are three arguments */

	f = lbot->val->p;	/* Get first argument into f */
	if (TYPE(lbot->val) != PORT)	/* Check type of first */
		error("fseek: First argument must be a port.", FALSE);

	offset = lbot[1].val->i;/* Get second argument */
	if (TYPE(lbot[1].val) != INT)
		error("fseek: Second argument must be an integer.", FALSE);

	whence = lbot[2].val->i;/* Get last arg	 */
	if (TYPE(lbot[2].val) != INT)
		error("fseek: Third argument must be an integer.", FALSE);

	if (fseek(f, offset, (int) whence) == -1)
		error("fseek: Illegal parameters.", FALSE);

	retp = inewint(ftell(f));

	return ((lispval) retp);
}

/* function hashtabstat  : return list of number of members in  each bucket */
lispval 
Lhashst(void)
{
	lispval         handy, cur;
	struct atom    *pnt;
	int             i, cnt;
	extern int      hashtop;
	Savestack(3);

	handy = newdot();
	protect(handy);
	cur = handy;
	for (i = 0; i < hashtop; i++) {
		pnt = hasht[i];
		for (cnt = 0; pnt != (struct atom *) CNIL; pnt = pnt->hshlnk, cnt++);
		cur->d.cdr = newdot();
		cur = cur->d.cdr;
		cur->d.car = inewint(cnt);
	}
	cur->d.cdr = nil;
	Restorestack();
	return (handy->d.cdr);
}


/*
 * Lctcherr this routine should only be called by the unwind protect
 * simulation lisp code It is called after an unwind-protect frame has been
 * entered and evalated and we want to get on with the error or throw We only
 * handle the case where there are 0 to 2 extra arguments to the error call.
 */
lispval
Lctcherr(void)
{
	lispval         handy;
	lispval         type, messg, valret, contuab, uniqid, datum1, datum2;

	chkarg(1, "I-throw-err");

	handy = lbot->val;

	if (TYPE(handy->d.car) == INT) {	/* continuing a non error
						 * (throw,reset, etc) */
		Inonlocalgo((int) handy->d.car->i,
			    handy->d.cdr->d.car,
			    handy->d.cdr->d.cdr->d.car);
		/* NOT REACHED */
	}
	if (handy->d.car != nil) {
		errorh1(Vermisc, "I-do-throw: first element not fixnum or nil",
			nil, FALSE, 0, handy);
	}
	/* decode the arg list */
	handy = handy->d.cdr;
	type = handy->d.car;
	handy = handy->d.cdr;
	messg = handy->d.car;
	handy = handy->d.cdr;
	valret = handy->d.car;
	handy = handy->d.cdr;
	contuab = handy->d.car;
	handy = handy->d.cdr;
	uniqid = handy->d.car;
	handy = handy->d.cdr;

	/* if not extra args */
	if (handy == nil) {
		errorh(type, messg->a.pname, valret, (int) contuab->i, (int) uniqid->i);
	}
	datum1 = handy->d.car;
	handy = handy->d.cdr;

	/* if one extra arg */
	if (handy == nil) {
		errorh1(type, messg->a.pname, valret, (int) contuab->i, (int) uniqid->i, datum1);
	}
	/* if two or more extra args, just use first 2 */
	datum2 = handy->d.car;
	return errorh2(type, messg->a.pname, valret, (int) contuab->i, (int) uniqid->i, datum1, datum2);
}

/*
 *	(*makhunk '<fixnum>)
 *			  <fixnum>
 * Create a hunk of size 2       . <fixnum> must be between 0 and 6.
 *
 */

lispval
LMakhunk(void)
{
	int             hsize, hcntr;
	lispval         result;

	chkarg(1, "Makehunk");
	if (TYPE(lbot->val) == INT) {
		hsize = lbot->val->i;	/* size of hunk (0-6) */
		if ((hsize >= 0) && (hsize <= 6)) {
			result = newhunk(hsize);
			hsize = 2 << hsize;	/* size of hunk (2-128) */
			for (hcntr = 0; hcntr < hsize; hcntr++)
				result->h.hunk[hcntr] = hunkfree;
		} else
			return error("*makhunk: Illegal hunk size", FALSE);
		return (result);
	} else
		return error("*makhunk: First arg must be an fixnum", FALSE);
}

/*
 *	(cxr '<fixnum> '<hunk>)
 * Returns the <fixnum>'th element of <hunk>
 *
 */
lispval
Lcxr(void)
{
	lispval         temp;

	chkarg(2, "cxr");
	if (TYPE(lbot->val) != INT)
		return error("cxr: First arg must be a fixnum", FALSE);
	else {
		if (!HUNKP(lbot[1].val))
			return error("cxr: Second arg must be a hunk", FALSE);
		else if ((lbot->val->i >= 0) &&
			 (lbot->val->i < (2 << HUNKSIZE(lbot[1].val)))) {
			temp = lbot[1].val->h.hunk[lbot->val->i];
			if (temp != hunkfree)
				return (temp);
			else
				return error("cxr: Arg outside of hunk range",
					     FALSE);
		} else
			return error("cxr: Arg outside of hunk range", FALSE);
	}
}

/*
 *	(rplacx '<fixnum> '<hunk> '<expr>)
 * Replaces the <fixnum>'th element of <hunk> with <expr>.
 *
 */
lispval
Lrplcx(void)
{
	lispval        *handy;
	chkarg(3, "rplacx");
	if (TYPE(lbot->val) != INT)
		error("rplacx: First arg must be a fixnum", FALSE);
	else {
		if (!HUNKP(lbot[1].val))
			error("rplacx: Second arg must be a hunk", FALSE);
		else {
			if ((lbot->val->i >= 0) &&
			    (lbot->val->i < (2 << HUNKSIZE(lbot[1].val)))) {
				if (*(handy = &(lbot[1].val->h.hunk[lbot->val->i]))
				    != hunkfree)
					*handy = lbot[2].val;
				else
					error("rplacx: Arg outside hunk range", FALSE);
			} else
				error("rplacx: Arg outside hunk range", FALSE);
		}
	}
	return (lbot[1].val);
}

/*
 *	(*rplacx '<fixnum> '<hunk> '<expr>)
 * Replaces the <fixnum>'th element of <hunk> with <expr>. This is the
 * same as (rplacx ...) except with this function you can replace EMPTY's.
 *
 */
lispval
Lstarrpx(void)
{
	chkarg(3, "*rplacx");
	if (TYPE(lbot->val) != INT)
		error("*rplacx: First arg must be a fixnum", FALSE);
	else {
		if (!HUNKP(lbot[1].val))
			error("*rplacx: Second arg must be a hunk", FALSE);
		else {
			if ((lbot->val->i >= 0) &&
			    (lbot->val->i < (2 << HUNKSIZE(lbot[1].val))))
				lbot[1].val->h.hunk[lbot->val->i] = lbot[2].val;
			else
				error("*rplacx: Arg outside hunk range", FALSE);
		}
	}
	return (lbot[1].val);
}

/*
 *	(hunksize '<hunk>)
 * Returns the size of <hunk>
 *
 */
lispval
Lhunksize(void)
{
	int             size, i;

	chkarg(1, "hunksize");
	if (HUNKP(lbot->val)) {
		size = 2 << HUNKSIZE(lbot->val);
		for (i = size - 1; i >= 0; i--) {
			if (lbot->val->h.hunk[i] != hunkfree) {
				size = i + 1;
				break;
			}
		}
		return (inewint(size));
	} else
		return error("hunksize: First argument must me a hunk", FALSE);
}

/*
 * (hunk-to-list 'hunk)	returns a list of the hunk elements
 */
lispval
Lhtol(void)
{
	lispval         handy, rval, last = nil;
	int             i;
	int             size;
	Savestack(4);

	chkarg(1, "hunk-to-list");
	handy = lbot->val;
	if (!(HUNKP(handy)))
		return errorh1(Vermisc, "hunk-to-list: non hunk argument: ", nil, 0, FALSE,
			       handy);
	size = 2 << HUNKSIZE(handy);
	rval = nil;
	for (i = 0; i < size; i++) {
		if (handy->h.hunk[i] != hunkfree) {
			if (rval == nil) {
				protect(rval = newdot());
				last = rval;
			} else {
				last = (last->d.cdr = newdot());
			}
			last->d.car = handy->h.hunk[i];
		} else
			break;
	}
	Restorestack();
	return (rval);
}

/*
 *	(fileopen  filename mode)
 * open a file for read, write, or append the arguments can be either
 * strings or atoms.
 */
lispval
Lfileopen(void)
{
	FILE           *port;
	lispval         name;
	lispval         mode;
	char           *namech;
	char           *modech;

	chkarg(2, "fileopen");
	name = lbot->val;
	mode = lbot[1].val;

	namech = verify(name, "fileopen:args must be atoms or strings");
	modech = verify(mode, "fileopen:args must be atoms or strings");

	while (modech[0] != 'r' && modech[0] != 'w' && modech[0] != 'a') {
		mode = errorh(Vermisc, "Modes are only r, w, a.", nil, TRUE, 31);
		modech = verify(mode, "fileopen:args must be atoms or strings");
	}

	while ((port = fopen(namech, modech)) == NULL) {
		name = errorh1(Vermisc, "Unable to open file.", nil, TRUE, 31, name);
		namech = verify(name, "fileopen:args must be atoms or strings");
	}
	/* xports is a FILE *, cc complains about adding pointers */

	ioname[PN(port)] = (lispval) inewstr(namech);	/* remember name */
	return (P(port));
}

/*
 *	(*invmod '<number> '<modulus>)
 * This function returns the inverse of  <number>
 * mod <modulus> in balanced representation
 * It is used in vaxima as a speed enhancement.
 */

static          lispval
Ibalmod(int invmodp)
{
	long            mod_div_2, number, modulus;

	chkarg(2, "*mod");
	if ((TYPE(lbot->val) == INT) && (TYPE(lbot[1].val) == INT)) {
		modulus = lbot[1].val->i;
		if (invmodp)
			number = invmod(lbot->val->i, modulus);
		else
			number = lbot->val->i % modulus;
		mod_div_2 = modulus / 2;
		if (number < 0) {
			if (number < (-mod_div_2))
				number += modulus;
		} else {
			if (number > mod_div_2)
				number -= modulus;
		}
		return (inewint(number));
	} else
		return error("*mod: Arguments must be fixnums", FALSE);
}

intptr_t
invmod(intptr_t n, intptr_t modulus)

{
	long            a1, a2, a3, y1, y2, y3, q;

	a1 = modulus;
	a2 = n;
	y1 = 0;
	y2 = 1;
	goto step3;
step2:
	q = a1 / a2;		/* truncated quotient */
	a3 = mmuladd(modulus - a2, q, a1, modulus);
	y3 = mmuladd(modulus - y2, q, y1, modulus);
	a1 = a2;
	a2 = a3;
	y1 = y2;
	y2 = y3;
step3:
	if (a2 == 0) {
		error("invmod: inverse of zero divisor", TRUE);
		return 0;
	} else if (a2 != 1)
		goto step2;
	else
		return (y2);
	/* NOTREACHED */
}

lispval
Lstarinvmod(void)
{
	return (Ibalmod(TRUE));
}

/*
 *	(*mod '<number> '<modulus>)
 * This function returns <number> mod <modulus> (for balanced modulus).
 * It is used in vaxima as a speed enhancement.
 */
lispval
LstarMod(void)
{
	return (Ibalmod(FALSE));
}

lispval
Llsh(void)
{
	struct argent  *mylbot = lbot;
	int             val, shift;

	chkarg(2, "lsh");
	if ((TYPE(mylbot->val) != INT) || (TYPE(mylbot[1].val) != INT))
		errorh2(Vermisc,
			"Non ints to lsh",
			nil, FALSE, 0, mylbot->val, mylbot[1].val);
	val = mylbot[0].val->i;
	shift = mylbot[1].val->i;
	if (shift < -32 || shift > 32)
		return (inewint(0));
	if (shift < 0)
		val = val >> -shift;
	else
		val = val << shift;
	if ((val < 0) && (shift < 0)) {	/* special case: the vax doesn't have
					 * a logical shift instruction, so we
					 * must zero out the ones which will
					 * propogate from the sign position */
		return (inewint(val & ~(0x80000000 >> -(shift + 1))));
	} else
		return (inewint(val));
}

/* very temporary function to test the validity of the bind stack */

void
bndchk(void)
{
	struct nament  *npt;
	lispval         in2;

	in2 = inewint(200);
	for (npt = orgbnp; npt < bnp; npt++) {
		if ((intptr_t) npt->atm < (intptr_t) in2)
			abort();
	}
}

/*
 *	formatted printer for lisp data
 *    use: (cprintf formatstring datum [port])
 */
lispval
Lcprintf(void)
{
	FILE           *p;
	char           *fstrng;
	lispval         v;
	if (np - lbot == 2)
		protect(nil);	/* write to standard output port */
	chkarg(3, "cprintf");

	fstrng = verify(lbot->val, "cprintf: first arg not string or symbol");

	p = okport(lbot[2].val, okport(Vpoport->a.clb, poport));

	switch (TYPE(v = lbot[1].val)) {

	case INT:
		fprintf(p, fstrng, v->i);
		break;

	case DOUB:
		fprintf(p, fstrng, v->r);
		break;

	case ATOM:
		fprintf(p, fstrng, v->a.pname);
		break;

	case STRNG:
		fprintf(p, fstrng, v);
		break;

	default:
		error("cprintf: Illegal second argument", FALSE);
	};

	return (lbot[1].val);
}


/*
 * C style sprintf: (sprintf "format" {<arg-list>})
 *
 * This function stacks the arguments onto the C stack in reverse
 * order and then calls sprintf with one argument...This is what the
 * C compiler does, so it works just fine. The return value is the
 * string that is the result of the sprintf.
 */
lispval
Lsprintf(void)
{
	struct argent  *argp;
	int             j;
	char            sbuf[600];	/* better way? */
	Keepxs();
	intptr_t       *top = sp();

	if (np - lbot == 0) {
		return argerr("sprintf");
	}
	if (TYPE(lbot->val) == STRNG || TYPE(lbot->val) == INT) {

		for (argp = np - 1; argp >= lbot; argp--) {
			switch (TYPE(argp->val)) {
			case ATOM:
				stack((intptr_t)argp->val->a.pname);
				break;

			case DOUB:
				{
					union {
#ifndef _LP64
						uint32_t i[2];
#else
						intptr_t i;
#endif
						double d;
					} rr;
					rr.d = argp->val->r;
#ifdef _LP64
					stack(rr.i);
#else
					// XXX: Wrong;
					stack(rr.i[1]);
					stack(rr.i[0]);
#endif
				}
				break;

			case INT:
				stack(argp->val->i);
				break;

			case STRNG:
				stack((long) argp->val);
				break;

			default:
				error("sprintf: Bad data type to sprintf",
				      FALSE);
			}
		}
		// XXX: Wrong
		vsprintf(sbuf, (char *) top[-1], (void *)&top[-2]);
		for (j = 0; j < np - lbot; j++)
			(void)unstack();
	} else
		error("sprintf: First arg must be an atom or string", FALSE);
	Freexs();
	return ((lispval) inewstr(sbuf));
}

lispval
Lprobef(void)
{
	char           *name;
	chkarg(1, "probef");

	name = verify(lbot->val, "probef: not symbol or string arg ");

	if (access(name, 0) == 0)
		return (tatom);
	else
		return (nil);
}

lispval
Lsubstring(void)
{
	char           *name;
	lispval         index, length;
	int             restofstring = FALSE;
	int             len, ind, reallen;

	switch (np - lbot) {
	case 2:
		restofstring = TRUE;
		break;

	case 3:
		break;

	default:
		chkarg(3, "substring");
	}

	name = verify(lbot[0].val, "substring: not symbol or string arg ");

	while (TYPE(index = lbot[1].val) != INT) {
		lbot[1].val = errorh1(Vermisc, "substring: non integer index ", nil,
				      TRUE, 0, index);
	}

	len = strlen(name);
	ind = index->i;

	if (ind < 0)
		ind = len + 1 + ind;

	if (ind < 1 || ind > len)
		return (nil);	/* index out of bounds */
	if (restofstring)
		return ((lispval) inewstr(name + ind - 1));

	while (TYPE(length = lbot[2].val) != INT) {
		lbot[2].val = errorh1(Vermisc, "substring: not integer length ", nil,
				      TRUE, 0, length);
	}

	if ((reallen = length->i) < 0 || (reallen + ind) > len)
		return ((lispval) inewstr(name + ind - 1));

	strncpy(strbuf, name + ind - 1, reallen);
	strbuf[reallen] = '\0';
	return ((lispval) newstr(0));
}

/*
 * This is substringn
 */
lispval
Lsstrn(void)
{
	char           *name;
	int             len, ind, reallen;
	lispval         index, length;
	int             restofstring = FALSE;
	Savestack(4);

	if ((np - lbot) == 2)
		restofstring = TRUE;
	else {
		chkarg(3, "substringn");
	}

	name = verify(lbot[0].val, "substringn: non symbol or string arg ");

	while (TYPE(index = lbot[1].val) != INT) {
		lbot[1].val = errorh1(Vermisc, "substringn: non integer index ", nil,
				      TRUE, 0, index);
	}

	if (!restofstring) {
		while (TYPE(length = lbot[2].val) != INT) {
			lbot[2].val = errorh1(Vermisc, "substringn: not integer length ",
					      nil, TRUE, 0, length);
		}
		reallen = length->i;
	} else
		reallen = -1;

	len = strlen(name);
	ind = index->i;
	if (ind < 0)
		ind = len + 1 + ind;
	if (ind < 1 || ind > len)
		return (nil);

	if (reallen == 0)
		return ((lispval) inewint(*(name + ind - 1)));
	else {
		char           *pnt = name + ind - 1;
		char           *last = name + len - 1;
		lispval         cur, start;

		protect(cur = start = newdot());
		cur->d.car = inewint(*pnt);
		while (++pnt <= last && --reallen != 0) {
			cur->d.cdr = newdot();
			cur = cur->d.cdr;
			cur->d.car = inewint(*pnt);
		}
		Restorestack();
		return (start);
	}

}


/*
 * (character-index 'string 'char)
 * return the index of char in the string.
 * return nil if not present
 * char can be a fixnum (representing a character)
 *  a symbol or string (in which case the first char is used)
 *
 */

#if os_unix_ts
#define index strchr
#endif
lispval
Lcharindex(void)
{
	char           *string;
	char            ch;
	char           *str2;

	chkarg(2, "character-index");


	string = verify(lbot[0].val, "character-index: non symbol or string arg ");
	if (TYPE(lbot[1].val) == INT)
		ch = (char) lbot[1].val->i;
	else {
		str2 = verify(lbot[1].val, "character-index: bad first argument ");
		ch = *str2;	/* grab the first character */
	}

	if ((str2 = strchr(string, ch)) == 0)
		return (nil);	/* not there */
	/* return 1-based index of character */
	return (inewint(str2 - string + 1));
}


lispval
Lpurcopy(void)
{
	chkarg(1, "purcopy");
	return (Ipurcopy(lbot[0].val));
}

lispval
Ipurcopy(lispval handy)
{
	extern intptr_t *beginsweep;
	lispval         retv, curv, lv;
	int             i, size;

	switch (TYPE(handy)) {

	case DTPR:
		retv = curv = pnewdot();
		lv = handy;
		while (TRUE) {
			curv->d.car = Ipurcopy(lv->d.car);
			if (TYPE(lv = lv->d.cdr) == DTPR) {
				curv->d.cdr = pnewdot();
				curv = curv->d.cdr;
			} else {
				curv->d.cdr = Ipurcopy(lv);
				break;
			}
		}
		return (retv);

	case SDOT:
		retv = curv = pnewsdot();
		lv = handy;
		while (TRUE) {
			curv->s.I = lv->s.I;
			if (lv->s.CDR == (lispval) 0)
				break;
			lv = lv->s.CDR;
			curv->s.CDR = pnewdot();
			curv = curv->s.CDR;
		}
		curv->s.CDR = 0;
		return (retv);

	case INT:
		if ((intptr_t *) handy < beginsweep)
			return (handy);
		retv = pnewint();
		retv->i = handy->i;
		return (retv);

	case DOUB:
		retv = pnewdb();
		retv->r = handy->r;
		return (retv);

	case HUNK2:
		i = 0;
		goto hunkit;

	case HUNK4:
		i = 1;
		goto hunkit;

	case HUNK8:
		i = 2;
		goto hunkit;

	case HUNK16:
		i = 3;
		goto hunkit;

	case HUNK32:
		i = 4;
		goto hunkit;

	case HUNK64:
		i = 5;
		goto hunkit;

	case HUNK128:
		i = 6;

hunkit:
		retv = pnewhunk(i);
		size = 2 << i;	/* number of elements to copy over */
		for (i = 0; i < size; i++) {
			retv->h.hunk[i] = Ipurcopy(handy->h.hunk[i]);
		}
		return (retv);



	case STRNG:
#ifdef GCSTRINGS
		{
			extern char     purepage[];

			if (purepage[((long) handy) >> LSHIFT] == 0)
				return ((lispval) pinewstr((char *) handy));
		}

#endif
	case ATOM:
	case BCD:
	case PORT:
		return (handy);	/* We don't want to purcopy these, yet it
				 * won't hurt if we don't mark them since
				 * they either aren't swept or will be marked
				 * in a special way */
	case ARRAY:
		return error("purcopy: can't purcopy array structures", FALSE);

	default:
		return error(" bad type to purcopy ", FALSE);
	}
}

/*
 * Lpurep returns t if the given arg is in pure space
 */
lispval
Lpurep(void)
{
	chkarg(1, "purep");
	return (Ipurep(lbot->val));
}



/* vector functions */

/* vector creation and initialization functions */
lispval
Lnvec(void)
{
	return (Inewvector(3));
}

lispval
Lnvecb(void)
{
	return (Inewvector(0));
}

lispval
Lnvecw(void)
{
	return (Inewvector(1));
}

lispval
Lnvecl(void)
{
	return (Inewvector(2));
}

/*
 * (new-vector 'x_size ['g_fill] ['g_prop])
 * class = 0: byte \
 *       = 1: word  > immediate
 *       = 2: long /
 *	 = 3: long
 */
lispval
Inewvector(int class)
{
	int             i;
	lispval         handy;
	lispval        *handy2;
	char           *chandy;
	short          *whandy;
	long           *lhandy;
	lispval         sizearg, fillarg, proparg;
	int             size, vsize;

	fillarg = proparg = nil;

	switch (np - lbot) {
	case 3:
		proparg = lbot[2].val;
	case 2:
		fillarg = lbot[1].val;
	case 1:
		sizearg = lbot[0].val;
		break;
	default:
		return argerr("new-vector");
	}

	while ((TYPE(sizearg) != INT) || sizearg->i < 0)
		sizearg = errorh1(Vermisc, "new-vector: bad size for vector ", nil,
				  TRUE, 0, sizearg);
	size = sizearg->i;
	switch (class) {
	case 0:
		vsize = size * sizeof(char);
		break;
	case 1:
		vsize = size * sizeof(short);
		break;
	default:
		vsize = size * sizeof(long);
		break;
	}

	if (class != 3)
		handy = nveci(vsize);
	else
		handy = newvec(vsize);

	switch (class) {
	case 0:
		chandy = (char *) handy;
		for (i = 0; i < size; i++)
			*chandy++ = (char) (fillarg->i);
		break;

	case 1:
		whandy = (short *) handy;
		for (i = 0; i < size; i++)
			*whandy++ = (short) (fillarg->i);
		break;

	case 2:
		lhandy = (long *) handy;
		for (i = 0; i < size; i++)
			*lhandy++ = (fillarg->i);
		break;

	case 3:
		handy2 = (lispval *) handy;
		for (i = 0; i < size; i++)
			*handy2++ = fillarg;
		break;
	}
	handy->v.vector[-1] = proparg;
	return (handy);
}

lispval
Lvectorp(void)
{
	chkarg(1, "vectorp");
	if (TYPE(lbot->val) == VECTOR)
		return (tatom);
	else
		return (nil);
}

lispval
Lpvp(void)
{
	chkarg(1, "vectorip");
	if (TYPE(lbot->val) == VECTORI)
		return (tatom);
	else
		return (nil);
}

/*
 * int:vref  vector[i] index class
 *  class = 0: byte immed, 1: word immed, 2: long immed, 3: long
 *
 * also do C style dereferencing of pointers.  This is a temporary
 * hack until we decide if we can live without it:
 *  class = 4: char, 5: short, 6: long, 7: float, 8: double
 */
lispval
LIvref(void)
{
	lispval         vect;
	int             index;
	int             class;
	double          value;

	chkarg(3, "int:vref");
	vect = lbot[0].val;
	index = lbot[1].val->i;
	class = lbot[2].val->i;
	switch (class) {
	case 0:
		return (inewint(vect->vb.vectorb[index]));
	case 1:
		return (inewint(vect->vw.vectorw[index]));
	case 2:
		return (inewint(vect->vl.vectorl[index]));
	case 3:
		return (vect->v.vector[index]);
	case 4:
		return (inewint(*(char *) (vect->i + index)));
	case 5:
		return (inewint(*(short *) (vect->i + index)));
	case 6:
		return (inewint(*(long *) (vect->i + index)));
	case 7:
		value = *(float *) (vect->i + index);
		vect = newdoub();
		vect->r = value;
		return (vect);
	case 8:
		value = *(double *) (vect->i + index);
		vect = newdoub();
		vect->r = value;
		return (vect);
	}
	return error("int:vref: impossible class detected", FALSE);
}

/*
 * int:vset vector[i] index value class
 *  class = 0: byte immed, 1: word immed, 2: long immed, 3: long
 */
lispval
LIvset(void)
{
	lispval         vect, value;
	int             index;
	int             class;

	chkarg(4, "int:vset");
	vect = lbot[0].val;
	index = lbot[1].val->i;
	value = lbot[2].val;
	class = lbot[3].val->i;
	switch (class) {
	case 0:
		vect->vb.vectorb[index] = (char) value->i;
		break;
	case 1:
		vect->vw.vectorw[index] = (short) value->i;
		break;
	case 2:
		vect->vl.vectorl[index] = value->i;
		break;
	case 3:
		vect->v.vector[index] = value;
		break;
	case 4:
		*(char *) (vect->i + index) = value->i;
		break;
	case 5:
		*(short *) (vect->i + index) = value->i;
		break;
	case 6:
		*(long *) (vect->i + index) = value->i;
		break;
	case 7:
		*(float *) (vect->i + index) = value->r;
		break;
	case 8:
		*(double *) (vect->i + index) = value->r;
		break;
	default:
		error("int:vref: impossible class detected", FALSE);
	}
	return (value);
}

/*
 * LIvsize == (int:vsize 'vector 'x_shift)
 *  return the vsize field of the vector shifted right by x_shift
 */
lispval
LIvsize(void)
{
	chkarg(2, "int:vsize");
	return (inewint((lbot[0].val->vl.vectorl[VSizeOff]) >> lbot[1].val->i));
}

lispval
Lvprop(void)
{
	int             typ;
	chkarg(1, "vprop");

	if (((typ = TYPE(lbot->val)) != VECTOR) && (typ != VECTORI))
		errorh1(Vermisc, "vprop: non vector argument: ", nil, FALSE, 0,
			lbot->val);
	return (lbot[0].val->v.vector[VPropOff]);
}


lispval
Lvsp(void)
{
	int             typ;
	lispval         vector, property;
	chkarg(2, "vsetprop");

	vector = lbot->val;
	property = lbot[1].val;
	typ = TYPE(vector);

	if (typ != VECTOR && typ != VECTORI)
		errorh1(Vermisc, "vsetprop: non vector argument: ",
			nil, FALSE, 0, vector);
	vector->v.vector[VPropOff] = property;
	return (property);
}


/*
 * vecequal check if the two vector arguments are 'equal' this is called by
 * equal which has already checked that the arguments are vector
 */
int
vecequal(lispval v, lispval w)
{
	int             i;
	lispval         vv, ww, ret;
	intptr_t        vsize = (intptr_t) v->v.vector[VSizeOff];
	intptr_t        wsize = (intptr_t) w->v.vector[VSizeOff];
	struct argent  *oldlbot = lbot;

	if (vsize != wsize)
		return (FALSE);

	vsize /= sizeof(int);	/* determine number of entries */

	for (i = 0; i < vsize; i++) {
		vv = v->v.vector[i];
		ww = w->v.vector[i];
		/* avoid calling equal if they are eq */
		if (vv != ww) {
			lbot = np;
			protect(vv);
			protect(ww);
			ret = Lequal();
			np = lbot;
			lbot = oldlbot;
			if (ret == nil)
				return (FALSE);
		}
	}
	return (TRUE);
}

/*
 * veciequal check if the two vectori arguments are 'equal' this is called by
 * equal which has already checked that the arguments are vector Note: this
 * would run faster if we did as many 'longword' comparisons as possible and
 * then did byte comparisons. or if we used pointers instead of indexing.
 */
int
veciequal(lispval v, lispval w)
{
	int             i;
	intptr_t        vsize = (intptr_t) v->v.vector[VSizeOff];
	intptr_t        wsize = (intptr_t) w->v.vector[VSizeOff];

	if (vsize != wsize)
		return (FALSE);


	for (i = 0; i < vsize; i++) {
		if (v->vb.vectorb[i] != w->vb.vectorb[i])
			return (FALSE);
	}
	return (TRUE);
}
