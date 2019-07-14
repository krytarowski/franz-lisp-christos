/*-					-[Fri Feb 17 16:44:24 1984 by layer]-
 * 	lam1.c
 * lambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lam1.c,v 1.8 87/12/14 18:39:12 sklower Exp";
#else
__RCSID("$Id: lam1.c,v 1.12 2014/12/25 23:57:55 christos Exp $");
#endif
#endif

#include <sys/param.h>
#include <string.h>
#include "global.h"
#include "chkrtab.h"
#include "frame.h"

extern int      plevel, plength;

lispval
Leval(void)
{
	lispval         temp;

	enter(1, "eval");
	temp = lbot->val;
	leave(eval(temp));
}

lispval
Lxcar(void)
{
	int             typ;
	lispval         temp, result;

	enter(1, "xcar");
	temp = lbot->val;
	if (((typ = TYPE(temp)) == DTPR) || (typ == ATOM) || HUNKP(temp))
		leave(temp->d.car);
	else if (typ == SDOT) {
		result = inewint(temp->i);
		leave(result);
	} else if (Schainp != nil && typ == ATOM)
		leave(nil);
	else
		leave(error("Bad arg to car", FALSE));
}

lispval
Lxcdr(void)
{
	int             typ;
	lispval         temp;

	enter(1, "xcdr");
	temp = lbot->val;
	if (temp == nil)
		leave(nil);

	if (((typ = TYPE(temp)) == DTPR) || HUNKP(temp))
		leave(temp->d.cdr);
	else if (typ == SDOT) {
		if (temp->s.CDR == 0)
			leave(nil);
		temp = temp->s.CDR;
		if (TYPE(temp) == DTPR)
			errorh1(Vermisc, "Fell off the end of a bignum", nil, FALSE, 5, lbot->val);
		leave(temp);
	} else if (Schainp != nil && typ == ATOM)
		leave(nil);
	else
		leave(error("Bad arg to cdr", FALSE));
}

lispval
cxxr(int as, int ds)
{

	lispval         temp, temp2;
	int             i, typ;

	enter(1, "c{ad}+r");
	temp = lbot->val;

	for (i = 0; i < ds; i++) {
		if (temp != nil) {
			typ = TYPE(temp);
			if ((typ == DTPR) || HUNKP(temp))
				temp = temp->d.cdr;
			else if (typ == SDOT) {
				if (temp->s.CDR == 0)
					temp = nil;
				else
					temp = temp->s.CDR;
				if (TYPE(temp) == DTPR)
					errorh1(Vermisc, "Fell off the end of a bignum", nil, FALSE, 5, lbot->val);
			} else if (Schainp != nil && typ == ATOM)
				leave(nil);
			else
				leave(errorh1(Vermisc, "Bad arg to cdr", nil, FALSE, 5, temp));
		}
	}

	for (i = 0; i < as; i++) {
		if (temp != nil) {
			typ = TYPE(temp);
			if ((typ == DTPR) || HUNKP(temp))
				temp = temp->d.car;
			else if (typ == SDOT)
				temp2 = inewint(temp->i), temp = temp2;
			else if (Schainp != nil && typ == ATOM)
				leave(nil);
			else
				leave(errorh1(Vermisc, "Bad arg to car", nil, FALSE, 5, temp));
		}
	}

	leave(temp);
}

lispval
Lcar(void)
{
	return (cxxr(1, 0));
}

lispval
Lcdr(void)
{
	return (cxxr(0, 1));
}

lispval
Lcadr(void)
{
	return (cxxr(1, 1));
}

lispval
Lcaar(void)
{
	return (cxxr(2, 0));
}

lispval
Lc02r(void)
{
	return (cxxr(0, 2));
}				/* cddr */

lispval
Lc12r(void)
{
	return (cxxr(1, 2));
}				/* caddr */

lispval
Lc03r(void)
{
	return (cxxr(0, 3));
}				/* cdddr */

lispval
Lc13r(void)
{
	return (cxxr(1, 3));
}				/* cadddr */

lispval
Lc04r(void)
{
	return (cxxr(0, 4));
}				/* cddddr */

lispval
Lc14r(void)
{
	return (cxxr(1, 4));
}				/* caddddr */

/*
 *
 *	(nthelem num list)
 *
 * Returns the num'th element of the list, by doing a caddddd...ddr
 * where there are num-1 d's. If num<=0 or greater than the length of
 * the list, we return nil.
 *
 */

lispval
Lnthelem(void)
{
	lispval         temp;
	int             i;

	enter(2, "nthelem");

	if (TYPE(temp = lbot->val) != INT)
		leave(error("First arg to nthelem must be a fixnum", FALSE));

	i = temp->i;		/* pick up the first arg */

	if (i <= 0)
		leave(nil);

	++lbot;			/* fix lbot for call to cxxr() 'cadddd..r' */
	temp = cxxr(1, i - 1);
	--lbot;

	leave(temp);
}

lispval
Lscons(void)
{
	struct argent  *argp = lbot;
	lispval         retp, handy;

	enter(2, "scons");
	retp = newsdot();
	handy = (argp)->val;
	if (TYPE(handy) != INT)
		error("First arg to scons must be an int.", FALSE);
	retp->s.I = handy->i;
	handy = (argp + 1)->val;
	if (handy == nil)
		retp->s.CDR = (lispval) 0;
	else {
		if (TYPE(handy) != SDOT)
			error("Currently you may only link sdots to sdots.", FALSE);
		retp->s.CDR = handy;
	}
	leave(retp);
}

lispval
Lbigtol(void)
{
	lispval         handy, newp;

	enter(1, "Bignum-to-list");
	handy = lbot->val;
	while (TYPE(handy) != SDOT)
		handy = errorh1(Vermisc,
				"Non bignum argument to Bignum-to-list",
				nil, TRUE, 5755, handy);
	protect(newp = newdot());
	while (handy) {
		newp->d.car = inewint((long) handy->s.I);
		if (handy->s.CDR == (lispval) 0)
			break;
		newp->d.cdr = newdot();
		newp = newp->d.cdr;
		handy = handy->s.CDR;
	}
	handy = (--np)->val;
	leave(handy);
}

lispval
Lcons(void)
{
	lispval         retp;
	struct argent  *argp;

	enter(2, "cons");
	retp = newdot();
	retp->d.car = ((argp = lbot)->val);
	retp->d.cdr = argp[1].val;
	leave(retp);
}
#define CA 0
#define CD 1

lispval
rpla(int what)
{
	struct argent  *argp;
	int             typ;
	lispval         first, second;

	enter(2, "rplac[ad]");
	argp = np - 1;
	first = (argp - 1)->val;
	while (first == nil)
		first = error("Attempt to rplac[ad] nil.", TRUE);
	second = argp->val;
	if (((typ = TYPE(first)) == DTPR) || (typ == ATOM) || HUNKP(first)) {
		if (what == CA)
			first->d.car = second;
		else
			first->d.cdr = second;
		leave(first);
	}
	if (typ == SDOT) {
		if (what == CA) {
			typ = TYPE(second);
			if (typ != INT)
				error("Rplacca of a bignum will only replace INTS", FALSE);
			first->s.I = second->i;
		} else {
			if (second == nil)
				first->s.CDR = (lispval) 0;
			else
				first->s.CDR = second;
		}
		leave(first);
	}
	leave(error("Bad arg to rpla", FALSE));
}
lispval
Lrplca(void)
{
	return (rpla(CA));
}

lispval
Lrplcd(void)
{
	return (rpla(CD));
}


lispval
Leq(void)
{
	struct argent  *mynp = lbot + AD;

	enter(2, "eq");
	if (mynp->val == (mynp + 1)->val)
		leave(tatom);
	leave(nil);
}



lispval
Lnull(void)
{
	enter(1, "null");
	leave((lbot->val == nil) ? tatom : nil);
}



/* Lreturn ************************************************************* */
/* Returns the first argument - which is nill if not specified.		 */

lispval
Lreturn(void)
{
	if (lbot == np)
		protect(nil);
	Inonlocalgo(C_RET, lbot->val, nil);
	return nil;
}


lispval
Linfile(void)
{
	FILE           *port;
	lispval         name;

	enter(1, "infile");
	name = lbot->val;
loop:
	name = verifyl(name, "infile: file name must be atom or string");
	/*
	 * return nil if file couldnt be opened if ((port = fopen((char
	 * *)name,"r")) == NULL) return(nil);
	 */

	if ((port = fopen((char *) name, "r")) == NULL) {
		name = errorh1(Vermisc, "Unable to open file for reading.", nil, TRUE, 31, name);
		goto loop;
	}
	ioname[PN(port)] = (lispval) inewstr((char *) name);	/* remember name */
	leave(P(port));
}

/*
 * outfile - open a file for writing.  27feb81 [jkf] - modifed to accept two
 * arguments, the second one being a string or atom, which if it begins with
 * an `a' tells outfile to open the file in append mode
 */
lispval
Loutfile(void)
{
	FILE           *port;
	lispval         name;
	char           *mode = "w";	/* mode is w for create new file, a
					 * for append */
	char           *given;

	if (lbot + 1 == np)
		protect(nil);
	enter(2, "outfile");
	name = lbot->val;
	given = verify((lbot + 1)->val, "Illegal file open mode.");
	if (*given == 'a')
		mode = "a";
loop:
	name = verifyl(name, "Please supply atom or string name for port.");
#ifdef	os_vms
	/*
	 *	If "w" mode, open it as a "txt" file for convenience in VMS
	 */
	if (strcmp(mode, "w") == 0) {
		int             fd;

		if ((fd = creat(name, 0777, "txt")) < 0) {
			name = errorh1(Vermisc, "Unable to open file for writing.", nil, TRUE, 31, name);
			goto loop;
		}
		port = fdopen(fd, mode);
	} else
#endif
	if ((port = fopen((char *) name, mode)) == NULL) {
		name = errorh1(Vermisc, "Unable to open file for writing.", nil, TRUE, 31, name);
		goto loop;
	}
	ioname[PN(port)] = (lispval) inewstr((char *) name);
	leave(P(port));
}

lispval
Lterpr(void)
{
	lispval         handy;
	FILE           *port;

	if (lbot == np) {
		handy = nil;
		enter(0, "terpr");
	} else {
		enter(1, "terpr");
		handy = lbot->val;
	}

	port = okport(handy, okport(Vpoport->a.clb, stdout));
	putc('\n', port);
	fflush(port);
	leave(nil);
}

lispval
Lclose(void)
{
	lispval         port;

	enter(1, "close");
	port = lbot->val;
	if ((TYPE(port)) == PORT) {
		fclose(port->p);
		ioname[PN(port->p)] = nil;
		leave(tatom);
	}
	leave(errorh1(Vermisc, "close:Non-port", nil, FALSE, 987, port));
}

lispval
Ltruename(void)
{
	enter(1, "truename");
	if (TYPE(lbot->val) != PORT)
		errorh1(Vermisc, "truename: non port argument", nil, FALSE, 0, lbot->val);

	leave(ioname[PN(lbot->val->p)]);
}

lispval
Lnwritn(void)
{
	FILE           *port;
	int             value;
	lispval         handy;

#ifdef BSD4_4
	if (lbot == np) {
		enter(0, "nwritn");
		handy = nil;
	} else {
		enter(1, "nwritn");
		handy = lbot->val;
	}

	port = okport(handy, okport(Vpoport->a.clb, stdout));
	value = port->_p - port->_bf._base;
#else
	value = 0;
#endif
	leave(inewint(value));
}

lispval
Ldrain(void)
{
	FILE           *port;
	lispval         handy;

	if (lbot == np) {
		enter(0, "drain");
		handy = nil;
	} else {
		enter(1, "drain");
		handy = lbot->val;
	}
	port = okport(handy, okport(Vpoport->a.clb, stdout));
#ifdef BSD4_4
	if (port->_w) {
		fflush(port);
		leave(nil);
	}
	if (port->_r) {
		fpurge(port);
		leave(P(port));
	}
	leave(nil);
#else
	fflush(port);
	leave(P(port));
#endif
}

lispval
Llist(void)
{
	/* added for the benefit of mapping functions. */
	struct argent  *ulim, *nameptr;
	lispval         temp, result;
	Savestack(4);

	ulim = np;
	nameptr = lbot + AD;
	temp = result = (lispval) np;
	protect(nil);
	for (; nameptr < ulim;) {
		temp = temp->l = newdot();
		temp->d.car = (nameptr++)->val;
	}
	temp->l = nil;
	Restorestack();
	return (result->l);
}

lispval
Lnumberp(void)
{
	enter(1, "numberp");
	switch (TYPE(lbot->val)) {
	case INT:
	case DOUB:
	case SDOT:
		leave(tatom);
	}
	leave(nil);
}

lispval
Latom(void)
{
	struct argent  *lb = lbot;
	enter(1, "atom");
	if (TYPE(lb->val) == DTPR || (HUNKP(lb->val)))
		leave(nil);
	else
		leave(tatom);
}

lispval
Ltype(void)
{
	enter(1, "type");
	switch (TYPE(lbot->val)) {
	case INT:
		leave(int_name);
	case ATOM:
		leave(atom_name);
	case SDOT:
		leave(sdot_name);
	case DOUB:
		leave(doub_name);
	case DTPR:
		leave(dtpr_name);
	case STRNG:
		leave(str_name);
	case ARRAY:
		leave(array_name);
	case BCD:
		leave(funct_name);
	case OTHER:
		leave(other_name);

	case HUNK2:
		leave(hunk_name[0]);
	case HUNK4:
		leave(hunk_name[1]);
	case HUNK8:
		leave(hunk_name[2]);
	case HUNK16:
		leave(hunk_name[3]);
	case HUNK32:
		leave(hunk_name[4]);
	case HUNK64:
		leave(hunk_name[5]);
	case HUNK128:
		leave(hunk_name[6]);

	case VECTOR:
		leave(vect_name);
	case VECTORI:
		leave(vecti_name);

	case VALUE:
		leave(val_name);
	case PORT:
		leave(port_name);
	}
	leave(nil);
}

lispval
Ldtpr(void)
{
	enter(1, "dtpr");
	leave(typred(DTPR, lbot->val));
}

lispval
Lbcdp(void)
{
	enter(1, "bcdp");
	leave(typred(BCD, lbot->val));
}

lispval
Lportp(void)
{
	enter(1, "portp");
	leave(typred(PORT, lbot->val));
}

lispval
Larrayp(void)
{
	enter(1, "arrayp");
	leave(typred(ARRAY, lbot->val));
}

/*
 *	(hunkp 'g_arg1)
 * Returns t if g_arg1 is a hunk, otherwise returns nil.
 */

lispval
Lhunkp(void)
{
	enter(1, "hunkp");
	if (HUNKP(lbot->val))
		leave(tatom);	/* If a hunk, leavet */
	else
		leave(nil);	/* else nil */
}

lispval
Lset(void)
{
	lispval         varble;

	enter(2, "set");
	varble = lbot->val;
	switch (TYPE(varble)) {
	case ATOM:
		leave(varble->a.clb = lbot[1].val);

	case VALUE:
		leave(varble->l = lbot[1].val);
	}

	leave(error("IMPROPER USE OF SET", FALSE));
}

lispval
Lequal(void)
{
	lispval         first, second;
	int             type1, type2;
	intptr_t       *oldsp;

	Keepxs();
	enter(2, "equal");

	if (lbot->val == lbot[1].val)
		leave(tatom);

	oldsp = sp();
	stack((intptr_t) lbot->val);
	stack((intptr_t) lbot[1].val);
	for (; oldsp > sp();) {

		first = (lispval) unstack();
		second = (lispval) unstack();
again:
		if (first == second)
			continue;

		type1 = TYPE(first);
		type2 = TYPE(second);
		if (type1 != type2) {
			if ((type1 == SDOT && type2 == INT) || (type1 == INT && type2 == SDOT))
				goto dosub;
			{
				Freexs();
				leave(nil);
			}
		}
		switch (type1) {
		case DTPR:
			stack((long) first->d.cdr);
			stack((long) second->d.cdr);
			first = first->d.car;
			second = second->d.car;
			goto again;
		case DOUB:
			if (first->r != second->r) {
				Freexs();
				leave(nil);
			}
			continue;
		case INT:
			if (first->i != second->i) {
				Freexs();
				leave(nil);
			}
			continue;
		case VECTOR:
			if (!vecequal(first, second)) {
				Freexs();
				leave(nil);
			}
			continue;
		case VECTORI:
			if (!veciequal(first, second)) {
				Freexs();
				leave(nil);
			}
			continue;
	dosub:
		case SDOT:{
				lispval         temp;
				struct argent  *OLDlbot = lbot;
				lbot = np;
				np++->val = first;
				np++->val = second;
				temp = Lsub();
				np = lbot;
				lbot = OLDlbot;
				if (TYPE(temp) != INT || temp->i != 0) {
					Freexs();
					leave(nil);
				}
			}
			continue;
		case VALUE:
			if (first->l != second->l) {
				Freexs();
				leave(nil);
			}
			continue;
		case STRNG:
			if (strcmp((char *) first, (char *) second) != 0) {
				Freexs();
				leave(nil);
			}
			continue;

		default:
			{
				Freexs();
				leave(nil);
			}
		}
	}
	{
		Freexs();
		leave(tatom);
	}
}

lispval
oLequal(void)
{
	enter(2, "equal");

	if (lbot[1].val == lbot->val)
		leave(tatom);
	if (Iequal(lbot[1].val, lbot->val))
		leave(tatom);
	else
		leave(nil);
}

int
Iequal(lispval first, lispval second)
{
	int             type1, type2;

	enter(0, "equal");
	if (first == second)
		leave(1);
	type1 = TYPE(first);
	type2 = TYPE(second);
	if (type1 != type2) {
		if ((type1 == SDOT && type2 == INT) || (type1 == INT && type2 == SDOT))
			goto dosub;
		leave(0);
	}
	switch (type1) {
	case DTPR:
		leave(
			Iequal(first->d.car, second->d.car) &&
			Iequal(first->d.cdr, second->d.cdr));
	case DOUB:
		leave(first->r == second->r);
	case INT:
		leave((first->i == second->i));
dosub:
	case SDOT:
		{
			lispval         temp;
			struct argent  *OLDlbot = lbot;
			lbot = np;
			np++->val = first;
			np++->val = second;
			temp = Lsub();
			np = lbot;
			lbot = OLDlbot;
			leave(TYPE(temp) == INT && temp->i == 0);
		}
	case VALUE:
		leave(first->l == second->l);
	case STRNG:
		leave(strcmp((char *) first, (char *) second) == 0);
	}
	leave(0);
}
lispval
Zequal(void)
{
	lispval         first, second;
	int             type1, type2;
	intptr_t       *oldsp;

	Keepxs();
	enter(2, "equal");

	if (lbot->val == lbot[1].val)
		leave(tatom);

	oldsp = sp();
	stack((long) lbot->val);
	stack((long) lbot[1].val);

	for (; oldsp > sp();) {

		first = (lispval) unstack();
		second = (lispval) unstack();
again:
		if (first == second)
			continue;

		type1 = TYPE(first);
		type2 = TYPE(second);
		if (type1 != type2) {
			if ((type1 == SDOT && type2 == INT) || (type1 == INT && type2 == SDOT))
				goto dosub;
			{
				Freexs();
				leave(nil);
			}
		}
		switch (type1) {
		case DTPR:
			stack((long) first->d.cdr);
			stack((long) second->d.cdr);
			first = first->d.car;
			second = second->d.car;
			goto again;
		case DOUB:
			if (first->r != second->r) {
				Freexs();
				leave(nil);
			}
			continue;
		case INT:
			if (first->i != second->i) {
				Freexs();
				leave(nil);
			}
			continue;
	dosub:
		case SDOT:
			{
				lispval         temp;
				struct argent  *OLDlbot = lbot;
				lbot = np;
				np++->val = first;
				np++->val = second;
				temp = Lsub();
				np = lbot;
				lbot = OLDlbot;
				if (TYPE(temp) != INT || temp->i != 0) {
					Freexs();
					leave(nil);
				}
			}
			continue;
		case VALUE:
			if (first->l != second->l) {
				Freexs();
				leave(nil);
			}
			continue;
		case STRNG:
			if (strcmp((char *) first, (char *) second) != 0) {
				Freexs();
				leave(nil);
			}
			continue;
		}
	}
	{
		Freexs();
		leave(tatom);
	}
}

/*
 * (print 'expression ['port]) prints the given expression to the given
 * port or poport if no port is given.  The amount of structure
 * printed is a function of global lisp variables plevel and
 * plength.
 */
lispval
Lprint(void)
{
	lispval         handy;


	handy = nil;		/* port is optional, default nil */
	switch (np - lbot) {
	case 2:
		handy = lbot[1].val;
	case 1:
		break;
	default:
		return (argerr("print"));
	}

	chkrtab(Vreadtable->a.clb);
	if (TYPE(Vprinlevel->a.clb) == INT) {
		plevel = Vprinlevel->a.clb->i;
	} else
		plevel = -1;
	if (TYPE(Vprinlength->a.clb) == INT) {
		plength = Vprinlength->a.clb->i;
	} else
		plength = -1;
	printr(lbot->val, okport(handy, okport(Vpoport->a.clb, poport)));
	return (nil);
}

/*
 * patom does not use plevel or plength
 * 
 * form is (patom 'value ['port])
 */
lispval
Lpatom(void)
{
	lispval         temp;
	lispval         handy;
	int             typ;
	FILE           *port;

	handy = nil;		/* port is optional, default nil */
	switch (np - lbot) {
	case 2:
		handy = lbot[1].val;
	case 1:
		break;
	default:
		argerr("patom");
	}

	temp = Vreadtable->a.clb;
	chkrtab(temp);
	port = okport(handy, okport(Vpoport->a.clb, stdout));
	if ((typ = TYPE((temp = (lbot)->val))) == ATOM)
		fputs(temp->a.pname, port);
	else if (typ == STRNG)
		fputs((char *) temp, port);
	else {
		if (TYPE(Vprinlevel->a.clb) == INT) {
			plevel = Vprinlevel->a.clb->i;
		} else
			plevel = -1;
		if (TYPE(Vprinlength->a.clb) == INT) {
			plength = Vprinlength->a.clb->i;
		} else
			plength = -1;

		printr(temp, port);
	}
	return (temp);
}

/*
 * (pntlen thing) returns the length it takes to print out
 * an atom or number.
 */

lispval
Lpntlen(void)
{
	return (inewint((long) Ipntlen()));
}

int
Ipntlen(void)
{
	lispval         temp;
	char           *handy;

	temp = np[-1].val;
loop:	switch (TYPE(temp)) {

	case ATOM:
		handy = temp->a.pname;
		break;

	case STRNG:
		handy = (char *) temp;
		break;

	case INT:
		snprintf(strbuf, estrbuf - strbuf, "%jd", (intmax_t)temp->i);
		handy = strbuf;
		break;

	case DOUB:
		snprintf(strbuf, estrbuf - strbuf, "%g", temp->r);
		handy = strbuf;
		break;

	default:
		temp = error("Non atom or number to pntlen\n", TRUE);
		goto loop;
	}

	return (strlen(handy));
}
#undef okport
FILE *
okport(lispval arg, FILE *proper)
{
	if (TYPE(arg) != PORT)
		return (proper);
	else
		return (arg->p);
}
