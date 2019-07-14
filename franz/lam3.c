/*-					-[Fri Aug  5 12:47:19 1983 by jkf]-
 * 	lam3.c
 * lambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lam3.c,v 1.4 84/04/06 23:08:13 layer Exp";
#else
__RCSID("$Id: lam3.c,v 1.6 2014/12/07 21:03:33 christos Exp $");
#endif
#endif

#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "chars.h"
#include "chkrtab.h"

lispval
Lalfalp(void)
{
	char           *first, *second;

	chkarg(2, "alphalessp");
	first = verify(lbot->val, "alphalessp: non symbol or string arg");
	second = verify((lbot + 1)->val, "alphalessp: non symbol or string arg");
	if (strcmp(first, second) < 0)
		return (tatom);
	else
		return (nil);
}

lispval
Lncons(void)
{
	lispval         handy;

	chkarg(1, "ncons");
	handy = newdot();
	handy->d.cdr = nil;
	handy->d.car = lbot->val;
	return (handy);
}
lispval
Lzerop(void)
{
	lispval         handy;

	chkarg(1, "zerop");
	handy = lbot->val;
	switch (TYPE(handy)) {
	case INT:
		return (handy->i == 0 ? tatom : nil);
	case DOUB:
		return (handy->r == 0.0 ? tatom : nil);
	}
	return (nil);
}
lispval
Lonep(void)
{
	lispval         handy;

	handy = lbot->val;
	switch (TYPE(handy)) {
	case INT:
		return (handy->i == 1 ? tatom : nil);
	case DOUB:
		return (handy->r == 1.0 ? tatom : nil);
	case SDOT:
		protect(inewint(0));
		handy = Ladd();
		if (TYPE(handy) != INT || handy->i != 1)
			return (nil);
		else
			return (tatom);
	}
	return (nil);
}

lispval
cmpx(int lssp)
{
	struct argent  *argp;
	struct argent  *outarg;
	struct argent  *onp = np;
	Savestack(3);


	argp = lbot + 1;
	outarg = np;
	while (argp < onp) {

		np = outarg + 2;
		lbot = outarg;
		if (lssp)
			*outarg = argp[-1], outarg[1] = *argp++;
		else
			outarg[1] = argp[-1], *outarg = *argp++;
		lbot->val = Lsub();
		np = lbot + 1;
		if (Lnegp() == nil) {
			Restorestack();
			return (nil);
		}
	}
	Restorestack();
	return (tatom);
}

lispval
Lgreaterp(void)
{
	int             typ;
	/* do the easy cases first */
	if (np - lbot == 2) {
		if ((typ = TYPE(lbot->val)) == INT) {
			if ((typ = TYPE(lbot[1].val)) == INT)
				return ((lbot[0].val->i - lbot[1].val->i) > 0 ? tatom : nil);
			else if (typ == DOUB)
				return ((lbot[0].val->i - lbot[1].val->r) > 0.0 ? tatom : nil);
		} else if (typ == DOUB) {
			if ((typ = TYPE(lbot[1].val)) == INT)
				return ((lbot[0].val->r - lbot[1].val->i) > 0.0 ? tatom : nil);
			else if (typ == DOUB)
				return ((lbot[0].val->r - lbot[1].val->r) > 0.0 ? tatom : nil);
		}
	}
	return (cmpx(FALSE));
}

lispval
Llessp(void)
{
	int             typ;
	/* do the easy cases first */
	if (np - lbot == 2) {
		if ((typ = TYPE(lbot->val)) == INT) {
			if ((typ = TYPE(lbot[1].val)) == INT)
				return ((lbot[0].val->i - lbot[1].val->i) < 0 ? tatom : nil);
			else if (typ == DOUB)
				return ((lbot[0].val->i - lbot[1].val->r) < 0.0 ? tatom : nil);
		} else if (typ == DOUB) {
			if ((typ = TYPE(lbot[1].val)) == INT)
				return ((lbot[0].val->r - lbot[1].val->i) < 0.0 ? tatom : nil);
			else if (typ == DOUB)
				return ((lbot[0].val->r - lbot[1].val->r) < 0.0 ? tatom : nil);
		}
	}
	return (cmpx(TRUE));
}

lispval
Ldiff(void)
{
	lispval         arg1, arg2;
	int             handy = 0;


	chkarg(2, "Ldiff");
	arg1 = lbot->val;
	arg2 = (lbot + 1)->val;
	if (TYPE(arg1) == INT && TYPE(arg2) == INT) {
		handy = arg1->i - arg2->i;
	} else
		error("non-numeric argument", FALSE);
	return (inewint(handy));
}

lispval
Lmod(void)
{
	lispval         arg1, arg2;
	lispval         handy;
	struct sdot     fake1, fake2;
	fake2.CDR = 0;
	fake1.CDR = 0;

	chkarg(2, "mod");
	handy = arg1 = lbot->val;
	arg2 = (lbot + 1)->val;
	switch (TYPE(arg1)) {
	case SDOT:
		switch (TYPE(arg2)) {
		case SDOT:	/* both are already bignums */
			break;
		case INT:	/* convert arg2 to bignum   */
			fake2.I = arg2->i;
			arg2 = (lispval) & fake2;
			break;
		default:
			error("non-numeric argument", FALSE);
		}
		break;
	case INT:
		switch (TYPE(arg2)) {
		case SDOT:	/* convert arg1 to bignum */
			fake1.I = arg1->i;
			arg1 = (lispval) & fake1;
			break;
		case INT:	/* both are fixnums 	  */
			return (inewint((arg1->i) % (arg2->i)));
		default:
			error("non-numeric argument", FALSE);
		}
		break;
	default:
		error("non-numeric argument", FALSE);
	}
	if (TYPE((lbot + 1)->val) == INT && lbot[1].val->i == 0)
		return (handy);
	divbig(arg1, arg2, (lispval *) 0, &handy);
	if (handy == ((lispval) & fake1))
		handy = inewint(fake1.I);
	if (handy == ((lispval) & fake2))
		handy = inewint(fake2.I);
	return (handy);
}
lispval
Ladd1(void)
{
	lispval         handy;
	Savestack(1);		/* fixup entry mask */
	chkarg(1, "add1");

	/* simple test first */
	if ((TYPE(lbot->val) == INT) && (lbot->val->i < MaxINT)) {
		Restorestack();
		return (inewint(lbot->val->i + 1));
	}
	handy = rdrint;
	handy->i = 1;
	protect(handy);
	handy = Ladd();
	Restorestack();
	return (handy);

}



lispval
Lsub1(void)
{
	lispval         handy;
	Savestack(1);		/* fixup entry mask */
	chkarg(1, "sub1");

	if ((TYPE(lbot->val) == INT) && (lbot->val->i > MinINT)) {
		Restorestack();
		return (inewint(lbot->val->i - 1));
	}
	handy = rdrint;
	handy->i = -1;
	protect(handy);
	handy = Ladd();
	Restorestack();
	return (handy);
}

lispval
Lminus(void)
{
	lispval         arg1, handy;

	chkarg(1, "minus");
	arg1 = lbot->val;
	handy = nil;
	switch (TYPE(arg1)) {
	case INT:
		handy = inewint(0 - arg1->i);
		break;
	case DOUB:
		handy = newdoub();
		handy->r = -arg1->r;
		break;
	case SDOT:{
			struct sdot     dummyb;
			handy = (lispval) & dummyb;
			handy->s.I = 0;
			handy->s.CDR = (lispval) 0;
			handy = subbig(handy, arg1);
			break;
		}

	default:
		error("non-numeric argument", FALSE);
	}
	return (handy);
}

lispval
Lnegp(void)
{
	lispval         handy = np[-1].val, work;
	int             flag = 0;

loop:
	switch (TYPE(handy)) {
	case INT:
		if (handy->i < 0)
			flag = TRUE;
		break;
	case DOUB:
		if (handy->r < 0)
			flag = TRUE;
		break;
	case SDOT:
		for (work = handy;
		     work->s.CDR != (lispval) 0;
		     work = work->s.CDR) {;
		}
		if (work->s.I < 0)
			flag = TRUE;
		break;
	default:
		handy = errorh1(Vermisc,
				"minusp: Non-(int,real,bignum) arg: ",
				nil,
				TRUE,
				0,
				handy);
		goto loop;
	}
	if (flag)
		return (tatom);
	return (nil);
}

lispval
Labsval(void)
{
	lispval         arg1;

	chkarg(1, "absval");
	arg1 = lbot->val;
	if (Lnegp() != nil)
		return (Lminus());

	return (arg1);
}

/*
 *
 * (oblist)
 *
 * oblist returns a list of all symbols in the oblist
 *
 * written by jkf.
 */
lispval
Loblist(void)
{
	int             indx;
	lispval         headp, tailp;
	struct atom    *symb;
	extern int      hashtop;
	Savestack(0);

	headp = tailp = newdot();	/* allocate first DTPR */
	protect(headp);		/* protect the list from garbage collection */
	/* line added by kls			  */

	for (indx = 0; indx <= hashtop - 1; indx++) {	/* though oblist */
		for (symb = hasht[indx];
		     symb != (struct atom *) CNIL;
		     symb = symb->hshlnk) {
			if (TYPE(symb) != ATOM) {
				printf(" non symbol in hasht[%d] = %p: ", indx, symb);
				printr((lispval) symb, stdout);
				printf(" \n");
				fflush(stdout);
			}
			tailp->d.car = (lispval) symb;	/* remember this atom */
			tailp = tailp->d.cdr = newdot();	/* link to next DTPR */
		}
	}

	tailp->d.cdr = nil;	/* close the list unfortunately throwing away
				 * the last DTPR */
	Restorestack();
	return (headp);
}

/*
 * Maclisp setsyntax function:
 *    (setsyntax c s x)
 * c represents character either by fixnum or atom
 * s is the atom "macro" or the atom "splicing" (in which case x is the
 * macro to be invoked); or nil (meaning don't change syntax of c); or
 * (well thats enough for now) if s is a fixnum then we modify the bits
 * for c in the readtable.
 */

lispval
Lsetsyn(void)
{
	lispval         s, c;
	struct argent  *mynp;
	int             index;
	lispval         x /* ,debugmode  */ ;

	switch (np - lbot) {
	case 2:
		x = nil;	/* only 2 args given */
	case 3:
		x = lbot[2].val;/* all three args given */
		break;
	default:
		return argerr("setsyntax");
	}
	s = Vreadtable->a.clb;
	chkrtab(s);
#ifdef DEBUG
	if (Istsrch(matom("debugging"))->d.cdr->d.cdr->d.cdr) {
		printf("Readtable addr: %p\n",ctable);
	}
#endif
	mynp = lbot;
	c = (mynp++)->val;
	s = (mynp++)->val;

	switch (TYPE(c)) {
	default:
		error("neither fixnum, atom or string as char to setsyntax", FALSE);

	case ATOM:
		index = *(c->a.pname);
		if ((c->a.pname)[1])
			errorh1(Vermisc, "Only 1 char atoms to setsyntax",
				nil, FALSE, 0, c);
		break;

	case INT:
		index = c->i;
		break;

	case STRNG:
		index = (int) *((char *) c);
	}
	switch (TYPE(s)) {
	case ATOM:
		if (s == splice || s == macro) {
			if (s == splice)
				ctable[index] = VSPL;
			else if (s == macro)
				ctable[index] = VMAC;
			if (TYPE(c) != ATOM) {
				strbuf[0] = index;
				strbuf[1] = 0;
				c = (getatom(TRUE));
			}
			Iputprop(c, x, lastrtab);
			return (tatom);
		}
		/* ... fall into */
	default:
		errorh1(Vermisc, "int:setsyntax : illegal second argument ",
			nil, FALSE, 0, s);
		/* not reached */

	case INT:
		switch (synclass(s->i)) {
		case CESC:
			Xesc = (char) index;
			break;
		case CDQ:
			Xdqc = (char) index;
			break;
		case CSD:
			Xsdc = (char) index;	/* string */
		}

		if (synclass(ctable[index]) == CESC	/* if we changed the
							 * current esc */
		    && (synclass(s->i) != CESC)	/* to something else, pick
						 * current */
		    &&Xesc == (char) index) {
			ctable[index] = s->i;
			rpltab(CESC, &Xesc);
		} else if (synclass(ctable[index]) == CDQ	/* likewise for double
								 * quote */
			   && synclass(s->i) != CDQ
			   && Xdqc == (char) index) {
			ctable[index] = s->i;
			rpltab(CDQ, &Xdqc);
		} else if (synclass(ctable[index]) == CSD	/* and for string
								 * delimiter */
			   && synclass(s->i) != CSD
			   && Xsdc == (char) index) {
			ctable[index] = s->i;
			rpltab(CSD, &Xsdc);
		} else
			ctable[index] = s->i;

		break;

	}
	return (tatom);
}

/*
 * this aux function is used by setsyntax to determine the new current
 * escape or double quote character.  It scans the character table for
 * the first character with the given class (either VESC or VDQ) and
 * puts that character in Xesc or Xdqc (whichever is pointed to by
 * addr).
 */
void
rpltab(char cclass, unsigned char  *addr)
{
	int             i;
	for (i = 0; i <= 127 && synclass(ctable[i]) != cclass; i++);
	if (i <= 127)
		*addr = (unsigned char) i;
	else
		*addr = '\0';
}


/*
 * int:getsyntax from lisp.
 * returns the fixnum syntax code from the readtable for the given character.
 * to be used by the lisp-code function getsyntax, not to be used by
 * joe user.
 */
lispval
Lgetsyntax(void)
{
	char           *name;
	int             number, typ;
	lispval         handy;

	chkarg(1, "int:getsyntax");
	handy = lbot[0].val;
	while (1) {
		if ((typ = TYPE(handy)) == ATOM) {
			name = handy->a.pname;
		} else if (typ == STRNG) {
			name = (char *) handy;
		} else if (typ == INT) {
			number = handy->i;
			break;
		} else {
			handy =
				errorh1(Vermisc, "int:getsyntax : bad character ",
					nil, TRUE, 0, handy);
			continue;	/* start at the top */
		}
		/* figure out the number of the first byte */
		number = (int) name[0];
		if (name[1] != '\0') {
			handy = errorh1(Vermisc,
			   "int:getsyntax : only single character allowed ",
					nil, TRUE, 0, handy);
		} else
			break;
	}
	/* see if number is within range */
	if (number < 0 || number > 255)
		errorh1(Vermisc, "int:getsyntax : character number out of range ", nil,
			FALSE, 0, inewint(number));
	chkrtab(Vreadtable->a.clb);	/* make sure readtable is correct */
	return (inewint(ctable[number]));
}




lispval
Lzapline(void)
{
	FILE           *port;

	port = rdrport;
	while (!feof(port) && (getc(port) != '\n'));
	return (nil);
}
