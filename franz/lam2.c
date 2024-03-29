/*-					-[Fri Aug  5 12:46:16 1983 by jkf]-
 * 	lam2.c
 * lambda functions
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: lam2.c,v 1.6 87/12/14 18:48:13 sklower Exp";
#else
__RCSID("$Id: lam2.c,v 1.9 2015/02/05 02:10:09 christos Exp $");
#endif
#endif

#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "structs.h"
#include "chars.h"
#include "chkrtab.h"

extern int      rlevel;

/*
 * (flatc 'thing ['max]) returns the smaller of max and the number of chars
 * required to print thing linearly.
 * if max argument is not given, we assume the second arg is infinity
 */
static int      flen;		/* Internal to this module, used as a running
				 * counter of flatsize */
static int      fmaxlen;	/* used for maximum for quick reference */

lispval
Lflatsi(void)
{
	lispval         current;
	Savestack(1);		/* fixup entry mask */

	fmaxlen = 0x7fffffff;	/* biggest integer by default */
	switch (np - lbot) {
	case 2:
		current = lbot[1].val;
		while (TYPE(current) != INT)
			current = errorh1(Vermisc,
					  "flatsize: second arg not integer",
					  nil, TRUE, 0, current);
		fmaxlen = current->i;
	case 1:
		break;
	default:
		return argerr("flatsize");
	}

	flen = 0;
	current = lbot->val;
	protect(nil);		/* create space for argument to pntlen */
	Iflatsi(current);
	Restorestack();
	return (inewint(flen));
}
/*
 * Iflatsi does the real work of the calculation for flatc
 */
void
Iflatsi(lispval current)
{

	if (flen > fmaxlen)
		return;
	switch (TYPE(current)) {

patom:
	case INT:
	case ATOM:
	case DOUB:
	case STRNG:
		np[-1].val = current;
		flen += Ipntlen();
		return;

pthing:
	case DTPR:
		flen++;
		Iflatsi(current->d.car);
		current = current->d.cdr;
		if (current == nil) {
			flen++;
			return;
		}
		if (flen > fmaxlen)
			return;
		switch (TYPE(current)) {
		case INT:
		case ATOM:
		case DOUB:
			flen += 4;
			goto patom;
		case DTPR:
			goto pthing;
		}
	}
}


#define EADC -1
#define EAD  -2
lispval
Lread(void)
{
	return (Lr(EAD));
}

lispval
Lratom(void)
{
	return (Lr(ATOM));
}

lispval
Lreadc(void)
{
	return (Lr(EADC));
}


/* r ******************************************************************** */
/* this function maps the desired read 	function into the system-defined */
/* reading functions after testing for a legal port.			 */
lispval
Lr(int op)
{
	unsigned char   c;
	lispval         result;
	int             cc;
	int             orlevel;
	FILE           *ttmp;
	struct nament  *oldbnp = bnp;
	Savestack(2);

	switch (np - lbot) {
	case 0:
		protect(nil);
	case 1:
		protect(nil);
	case 2:
		break;
	default:
		argerr("read or ratom or readc");
	}
	result = Vreadtable->a.clb;
	chkrtab(result);
	orlevel = rlevel;
	rlevel = 0;
	ttmp = okport(Vpiport->a.clb, stdin);
	ttmp = okport(lbot->val, ttmp);
	if (ttmp == stdin)
		fflush(stdout);	/* flush any pending characters if reading
				 * stdin there should be tests to see if this
				 * is a tty or pipe */

	switch (op) {
	case EADC:
		rlevel = orlevel;
		cc = getc(ttmp);
		c = cc;
		if (cc == EOF) {
			Restorestack();
			return (lbot[1].val);
		} else {
			strbuf[0] = hash = (c & 0177);
			strbuf[1] = 0;
			atmlen = 2;
			Restorestack();
			return ((lispval) getatom(TRUE));
		}

	case ATOM:
		rlevel = orlevel;
		result = (ratomr(ttmp));
		goto out;

	case EAD:
		PUSHDOWN(Vpiport, P(ttmp));	/* rebind Vpiport */
		result = readr(ttmp);
out:		if (result == eofa) {
			if (sigintcnt > 0)
				sigcall(SIGINT);
			result = lbot[1].val;
		}
		rlevel = orlevel;
		popnames(oldbnp);	/* unwind bindings */
		Restorestack();
		return (result);
	default:
		abort();
		return nil;
	}
}

/* Lload **************************************************************** */
/* Reads in and executes forms from the specified file. This should      */
/* really be an nlambda taking multiple arguments, but the error 	 */
/* handling gets funny in that case (one file out of several not 	 */
/* openable, for instance).						 */
lispval
Lload(void)
{
	FILE           *port;
	char           *p, *ttmp;
	lispval         vtmp;
	struct nament  *oldbnp = bnp;
	int             orlevel, typ;
	char            longname[100];
	char           *shortname, *end2;
	/* Savestack(4); not necessary because np not altered */

	chkarg(1, "load");
	if ((typ = TYPE(lbot->val)) == ATOM)
		ttmp = lbot->val->a.pname;	/* ttmp will point to name */
	else if (typ == STRNG)
		ttmp = (char *) lbot->val;
	else
		return (error("FILENAME MUST BE ATOMIC", FALSE));
	strcpy(longname, Ilibdir());
	for (p = longname; *p; p++);
	*p++ = '/';
	*p = 0;
	shortname = p;
	strcpy(p, ttmp);
	for (; *p; p++);
	end2 = p;
	strcpy(p, ".l");
	if ((port = fopen(shortname, "r")) == NULL &&
	    (port = fopen(longname, "r")) == NULL) {
		*end2 = 0;
		if ((port = fopen(shortname, "r")) == NULL &&
		    (port = fopen(longname, "r")) == NULL)
			errorh1(Vermisc, "Can't open file: ",
				nil, FALSE, 0, lbot->val);
	}
	orlevel = rlevel;
	rlevel = 0;

	if (ISNIL(copval(gcload, CNIL)) &&
	    loading->a.clb != tatom &&
	    ISNIL(copval(gcdis, CNIL)))
		gc((struct types *) CNIL);	/* do a gc if gc will be off  */

	/* shallow bind the value of lisp atom piport 	 */
	/* so readmacros will work			 */
	PUSHDOWN(Vpiport, P(port));
	PUSHDOWN(loading, tatom);	/* set indication of loading status */

	while ((vtmp = readr(port)) != eofa) {
		eval(vtmp);
	}
	popnames(oldbnp);	/* unbind piport, loading */

	rlevel = orlevel;
	fclose(port);
	return (nil);
}


/*
 * concat ************************************************** - -  use:
 * (concat arg1 arg2 ... ) - -  concatenates the print names of all of its
 * arguments. - the arguments may be atoms, integers or real numbers. - - ********************************************************
 */
lispval
Iconcat(int unintern)
{
	struct argent  *temnp;
	char           *cp = strbuf;
	lispval         cur;
	int             n;

	*cp = NULL_CHAR;

	/* loop for each argument */
	for (temnp = lbot + AD; temnp < np; temnp++) {
		cur = temnp->val;
		switch (TYPE(cur)) {
		case ATOM:
			n = strlen(cur->a.pname);
			while (n + cp >= estrbuf)
				cp = atomtoolong(cp);
			strlcpy(cp, cur->a.pname, estrbuf - cp);
			cp += n;
			break;

		case STRNG:
			n = strlen((char *) cur);
			while (n + cp >= estrbuf)
				cp = atomtoolong(cp);
			strlcpy(cp, (char *) cur, estrbuf - cp);
			cp += n;
			break;

		case INT:
			if (15 + cp >= estrbuf)
				cp = atomtoolong(cp);
			snprintf(cp, estrbuf - cp, "%jd", (intmax_t)cur->i);
			while (*cp)
				cp++;
			break;

		case DOUB:
			if (15 + cp >= estrbuf)
				cp = atomtoolong(cp);
			snprintf(cp, estrbuf - cp, "%f", cur->r);
			while (*cp)
				cp++;
			break;

		case SDOT:{
				lispval         handy = cur;
				FILE           *f;

				for (n = 12; handy->s.CDR != (lispval) 0; handy = handy->s.CDR)
					n += 12;

				while (n + cp >= estrbuf)
					cp = atomtoolong(cp);

				f = fstopen(cp, estrbuf - cp - 1, "w");
				pbignum(cur, f);
				fclose(f);
				while (*cp)
					cp++;
				*cp = '\0';
				break;
			}

		default:
			cur = error("Non atom or number to concat", TRUE);
			continue;	/* if returns value, try it */
		}

	}

	if (unintern)
		return ((lispval) newatom(FALSE));	/* uninterned atoms may
							 * have printname gc'd */
	else
		return ((lispval) getatom(FALSE));
}
lispval
Lconcat(void)
{
	return (Iconcat(FALSE));
}
lispval
Luconcat(void)
{
	return (Iconcat(TRUE));
}

lispval
Lputprop(void)
{
	chkarg(3, "putprop");
	return (Iputprop(lbot->val, lbot[1].val, lbot[2].val));
}

/*
 * Iputprop :internal version of putprop used by some C functions
 *  note: prop and ind are lisp values but are not protected (by this
 * function) from gc.  The caller should protect them!!
 */
lispval
Iputprop(lispval atm, lispval prop, lispval ind)
{
	lispval         pptr;
	lispval        *tack;	/* place to begin property list */
	lispval         pptr2;
	Savestack(4);

top:
	switch (TYPE(atm)) {
	case ATOM:
		if (atm == nil)
			tack = &nilplist;
		else
			tack = &(atm->a.plist);
		break;
	case DTPR:
		for (pptr = atm->d.cdr; pptr != nil; pptr = pptr->d.cdr->d.cdr)
			if (TYPE(pptr) != DTPR || TYPE(pptr->d.cdr) != DTPR)
				break;
		if (pptr != nil) {
			atm = errorh1(Vermisc,
				   "putprop: bad disembodied property list",
				      nil, TRUE, 0, atm);
			goto top;
		}
		tack = (lispval *) & (atm->d.cdr);
		break;
	default:
		return errorh1(Vermisc, "putprop: Bad first argument: ", nil, FALSE, 0, atm);
	}
	pptr = *tack;		/* start of property list */
	/* findit: */
	for (pptr = *tack; pptr != nil; pptr = pptr->d.cdr->d.cdr)
		if (pptr->d.car == ind) {
			(pptr->d.cdr)->d.car = prop;
			Restorestack();
			return (prop);
		}
	/*
	 * not found, add to front be careful, a gc could occur before the
	 * second newdot()
	 */

	pptr = newdot();
	pptr->d.car = prop;
	pptr->d.cdr = *tack;
	protect(pptr);
	pptr2 = newdot();
	pptr2->d.car = ind;
	pptr2->d.cdr = pptr;
	*tack = pptr2;
	Restorestack();
	return (prop);
}

/*
 * get from property list there are three routines to accomplish this Lget -
 * lisp callable, the first arg can be a symbol or a disembodied property
 * list.  In the latter case we check to make sure it is a real one (as best
 * we can). Iget - internal routine, the first arg must be a symbol, no
 * disembodied plists allowed Igetplist - internal routine, the first arg is
 * the plist to search.
 */
lispval
Lget(void)
{
	lispval         ind, atm;
	lispval         dum1;

	chkarg(2, "get");
	ind = lbot[1].val;
	atm = lbot[0].val;
top:
	switch (TYPE(atm)) {
	case ATOM:
		if (atm == nil)
			atm = nilplist;
		else
			atm = atm->a.plist;
		break;

	case DTPR:
		for (dum1 = atm->d.cdr; dum1 != nil; dum1 = dum1->d.cdr->d.cdr)
			if ((TYPE(dum1) != DTPR) ||
			    (TYPE(dum1->d.cdr) != DTPR))
				break;	/* bad prop list */
		if (dum1 != nil) {
			atm = errorh1(Vermisc,
				      "get: bad disembodied property list",
				      nil, TRUE, 0, atm);
			goto top;
		}
		atm = atm->d.cdr;
		break;
	default:
		/*
		 * remove since maclisp doesnt treat this as an error, ugh
		 * return(errorh1(Vermisc,"get: bad first argument: ",
		 * nil,FALSE,0,atm));
		 */
		return (nil);
	}

	while (atm != nil) {
		if (atm->d.car == ind)
			return ((atm->d.cdr)->d.car);
		atm = (atm->d.cdr)->d.cdr;
	}
	return (nil);
}
/*
 * Iget - the first arg must be a symbol.
 */

lispval
Iget(lispval atm, lispval ind)
{

	if (atm == nil)
		atm = nilplist;
	else
		atm = atm->a.plist;
	return (Igetplist(atm, ind));
}

/*
 *  Igetplist
 * pptr is a plist
 * ind is the indicator
 */

lispval
Igetplist(lispval pptr, lispval ind)
{
	while (pptr != nil) {
		if (pptr->d.car == ind)
			return ((pptr->d.cdr)->d.car);
		pptr = (pptr->d.cdr)->d.cdr;
	}
	return (nil);
}
lispval
Lgetd(void)
{
	lispval         typ;

	chkarg(1, "getd");
	typ = lbot->val;
	if (TYPE(typ) != ATOM)
		errorh1(Vermisc,
			"getd: Only symbols have function definitions",
			nil,
			FALSE,
			0,
			typ);
	return (typ->a.fnbnd);
}
lispval
Lputd(void)
{
	lispval         atom, list;

	chkarg(2, "putd");
	list = lbot[1].val;
	atom = lbot->val;
	if (TYPE(atom) != ATOM)
		error("only symbols have function definitions",
		      FALSE);
	atom->a.fnbnd = list;
	return (list);
}

/*
 * =========================================================== - mapping
 * functions which return a list of the answers - mapcar applies the given
 * function to successive elements - maplist applies the given function to
 * successive sublists -
 * ===========================================================
 */

lispval
Lmapcrx(
	int maptyp,	/* 0 = mapcar,  1 = maplist  */
	int join	/* 0 = the above, 1 = s/car/can/ */
)
{
	struct argent  *nameptr;
	int             index;
	lispval         temp;
	lispval         current;

	struct argent  *first, *last;
	int             count;
	lispval         lists[25], result;
	Savestack(4);

	nameptr = lbot + 1;
	count = np - nameptr;
	if (count <= 0)
		return (nil);
	result = current = (lispval) np;
	protect(nil);		/* set up space for returned list */
	protect(lbot->val);	/* copy funarg for call to funcall */
	lbot = np - 1;
	first = np;
	last = np += count;
	for (index = 0; index < count; index++) {
		temp = (nameptr++)->val;
		if (TYPE(temp) != DTPR && temp != nil)
			error("bad list argument to map", FALSE);
		lists[index] = temp;
	}
	for (;;) {
		for (nameptr = first, index = 0; index < count; index++) {
			temp = lists[index];
			if (temp == nil)
				goto done;

			if (maptyp == 0)
				(nameptr++)->val = temp->d.car;
			else
				(nameptr++)->val = temp;

			lists[index] = temp->d.cdr;
		}
		if (join == 0) {
			current->l = newdot();
			current->l->d.car = Lfuncal();
			current = (lispval) & current->l->d.cdr;
		} else {
			current->l = Lfuncal();
			if (TYPE(current->l) != DTPR && current->l != nil)
				error("bad type returned from funcall inside map", FALSE);
			else
				while (current->l != nil)
					current = (lispval) & (current->l->d.cdr);
		}
		np = last;
	}
done:	if (join == 0)
		current->l = nil;
	Restorestack();
	return (result->l);
}

/*
 * ============================ - - Lmapcar - =============================
 */

lispval
Lmpcar(void)
{
	return (Lmapcrx(0, 0));	/* call general routine */
}


/*
 * ============================ - - -  Lmaplist -
 * ==============================
 */

lispval
Lmaplist(void)
{
	return (Lmapcrx(1, 0));	/* call general routine */
}


/*
 * ================================================ - mapping functions which
 * return the value of the last function application. - mapc and map -
 * ===================================================
 */

lispval
Lmapcx(
	int maptyp	/* 0= mapc   , 1= map  */
)
{
	struct argent  *nameptr;
	int             index;
	lispval         temp;
	lispval         result;

	int             count;
	struct argent  *first;
	lispval         lists[25];
	Savestack(4);

	nameptr = lbot + 1;
	count = np - nameptr;
	if (count <= 0)
		return (nil);
	result = lbot[1].val;	/* This is what macsyma wants so ... */
	/* copy funarg for call to funcall */
	lbot = np;
	protect((nameptr - 1)->val);
	first = np;
	np += count;

	for (index = 0; index < count; index++) {
		temp = (nameptr++)->val;
		while (temp != nil && TYPE(temp) != DTPR)
			temp = errorh1(Vermisc, "Inappropriate list argument to mapc", nil, TRUE, 0, temp);
		lists[index] = temp;
	}
	for (;;) {
		for (nameptr = first, index = 0; index < count; index++) {
			temp = lists[index];
			if (temp == nil)
				goto done;
			if (maptyp == 0)
				(nameptr++)->val = temp->d.car;
			else
				(nameptr++)->val = temp;
			lists[index] = temp->d.cdr;
		}
		Lfuncal();
	}
done:
	Restorestack();
	return (result);
}


/*
 * ================================== - -	mapc   map the car of the
 * lists - - ==================================
 */

lispval
Lmapc(void)
{
	return (Lmapcx(0));
}


/*
 * ================================= - -	map    map the cdr of the
 * lists - - ===================================
 */

lispval
Lmap(void)
{
	return (Lmapcx(1));
}


lispval
Lmapcan(void)
{
	return (Lmapcrx(0, 1));
}

lispval
Lmapcon(void)
{
	return (Lmapcrx(1, 1));
}
