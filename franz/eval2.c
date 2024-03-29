/*-					-[Sat May  7 23:38:37 1983 by jkf]-
 * 	eval2.c
 * more of the evaluator
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: eval2.c,v 1.8 85/03/24 11:03:02 sklower Exp";
#else
__RCSID("$Id: eval2.c,v 1.7 2014/12/05 17:28:04 christos Exp $");
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "frame.h"

/*
 * Iarray - handle array call. fun - array object args - arguments to the
 * array call , most likely subscripts. evalp - flag, if TRUE then the
 * arguments should be evaluated when they are stacked.
 */
lispval
Iarray(lispval fun, lispval args, int evalp)
{
	Savestack(2);

	lbot = np;
	protect(fun->ar.accfun);
	for (; args != nil; args = args->d.cdr)	/* stack subscripts */
		if (evalp)
			protect(eval(args->d.car));
		else
			protect(args->d.car);
	protect(fun);
	vtemp = Lfuncal();
	Restorestack();
	return (vtemp);
}

int 
dumpmydata(int thing)
{
	int            *ip = &thing;
	int            *lim = ip + nargs(0);

	printf("Dumpdata got %d args:\n", nargs(0));
	while (ip < lim)
		printf("%x\n", *ip++);
	return (0);
}
/*
 * Ifcall :: call foreign function/subroutine Ifcall is handed a binary
 * object which is the function to call. This function has already been
 * determined to be a foreign function by noticing that its discipline field
 * is a string.  The arguments to pass have already been evaluated and
 * stacked.  We create on the stack a 'callg' type argument list to give to
 * the function.  What is passed to the foreign function depends on the type
 * of argument.  Certain args are passes directly, others must be copied
 * since the foreign function my want to change them. When the foreign
 * function returns, we may have to box the result, depending on the type of
 * foreign function.
 */
lispval
Ifcall(lispval a)
{
	intptr_t       *arglist = NULL, *ap;
	int             index;
	struct argent  *mynp;
	lispval         ltemp;
	pbuf            pb;
	int             nargs = np - lbot, kind, mysize;
	Keepxs();

	/*
	 * put a frame on the stack which will save np and lbot in a easy to
	 * find place in a standard way
	 */
	errp = Pushframe(F_TO_FORT, nil, nil);
	mynp = lbot;
	kind = (((char *) a->bcd.discipline)[0]);

	/* dispatch according to whether call by reference or value semantics */
	switch (kind) {
	case 'f':
	case 'i':
	case 's':
	case 'r':
		arglist = alloca((nargs + 1) * sizeof(long));
		*arglist = nargs;
		for (index = 1; index <= nargs; index++) {
			switch (TYPE(ltemp = mynp->val)) {
				/* fixnums and flonums must be reboxed */
			case INT:
				stack(0);
				arglist[index] = (long) sp();
				*(intptr_t *) arglist[index] = ltemp->i;
				break;
			case DOUB:
				stack(0);
				stack(0);
				arglist[index] = (long) sp();
				*(double *) arglist[index] = ltemp->r;
				break;

				/*
				 * these cause only part of the structure to
				 * be sent
				 */

			case ARRAY:
				arglist[index] = (long) ltemp->ar.data;
				break;


			case BCD:
				arglist[index] = (long) ltemp->bcd.start;
				break;

				/* anything else should be sent directly */

			default:
				arglist[index] = (long) ltemp;
				break;
			}
			mynp++;
		}
		break;
	case 'v':
		while (TYPE(mynp->val) != VECTORI)
			mynp->val = error(
					  "First arg to c-function-returning-vector must be of type vector-immediate",
					  TRUE);
		nargs--;
		mynp++;
		lbot++;
	case 'c':
	case 'd':
		/*
		 * make one pass over args calculating size of arglist
		 */
		while (mynp < np)
			switch (TYPE(ltemp = mynp++->val)) {
			case DOUB:
				nargs += ((sizeof(double) / sizeof(int)) - 1);
				break;
			case VECTORI:
				if (ltemp->v.vector[-1] == Vpbv) {
					nargs += -1 + VecTotSize(ltemp->vl.vectorl[-2]);
				}
			}
		arglist = alloca((nargs + 1) * sizeof(long));
		*arglist = nargs;
		ap = arglist + 1;
		/*
		 * make another pass over the args actually copying the
		 * arguments
		 */
		for (mynp = lbot; mynp < np; mynp++)
			switch (TYPE(ltemp = mynp->val)) {
			case INT:
				*ap++ = ltemp->i;
				break;
			case DOUB:
				*(double *) ap = ltemp->r;
				ap += (sizeof(double)) / (sizeof(long));
				break;
			case VECTORI:
				if (ltemp->v.vector[-1] == Vpbv) {
					mysize = ltemp->vl.vectorl[-2];
					mysize = sizeof(long) * VecTotSize(mysize);
					memmove(ltemp, ap, mysize);
					ap = (intptr_t *) (mysize + (intptr_t) ap);
					break;
				}
			default:
				*ap++ = (long) ltemp;
			}
	}
	switch (kind) {
	case 'i':		/* integer-function */
	case 'c':		/* C-function */
		ltemp = inewint(callg(a->bcd.start, arglist)->i);
		break;

	case 'r':		/* real-function */
	case 'd':		/* C function declared returning double */
		{
			double          result =
			callg(a->bcd.start, arglist)->r;
			ltemp = newdoub();
			ltemp->r = result;
		}
		break;

	case 'f':		/* function */
		ltemp = callg(a->bcd.start, arglist);
		break;

	case 'v':		/* C function returning a structure */
		ap = callg(a->bcd.start, arglist)->j;
		ltemp = (--lbot)->val;
		mysize = ltemp->vl.vectorl[-2];
		mysize = sizeof(long) * VecTotSize(mysize);
		memmove(ap, ltemp, mysize);
		break;

	default:
	case 's':		/* subroutine */
		callg(a->bcd.start, arglist);
		ltemp = tatom;
	}
	errp = Popframe();
	Freexs();
	return (ltemp);
}

lispval
ftolsp_(lispval arg1)
{
	int             count;
	lispval        *ap = &arg1;
	lispval         save;
	pbuf            pb;
	Savestack(1);

	if ((count = nargs(0)) == 0)
		return nil;

	if (errp->class == F_TO_FORT)
		np = errp->svnp;
	errp = Pushframe(F_TO_LISP, nil, nil);
	lbot = np;
	for (; count > 0; count--)
		np++->val = *ap++;
	save = Lfuncal();
	errp = Popframe();
	Restorestack();
	return (save);
}

lispval
ftlspn_(lispval func, intptr_t *arglist)
{
	int             count;
	lispval         save;
	pbuf            pb;
	Savestack(1);

	if (errp->class == F_TO_FORT)
		np = errp->svnp;
	errp = Pushframe(F_TO_LISP, nil, nil);
	lbot = np;
	np++->val = func;
	count = *arglist++;
	for (; count > 0; count--)
		np++->val = (lispval) (*arglist++);
	save = Lfuncal();
	errp = Popframe();
	Restorestack();
	return (save);
}



/*
 * Ifclosure :: evaluate a fclosure  (new version) the argument clos is a
 * vector whose property is the atom fclosure the form of the vector is 0:
 * function to run then for each symbol there is on vector entry containing a
 * pointer to a sequence of two list cells of this form: (name value . count)
 * name is the symbol name to close over value is the saved value of the
 * closure (if the closure is 'active', the current value will be in the
 * symbol itself) count is a fixnum box (which can be destructively modified
 * safely) it is normally 0.  Each time the variable is put on the stack, it
 * is incremented.  It is decremented each time the the closure is left. If
 * the closure is invoked recusively without a rebinding of the closure
 * variable X, then the count will not be incremented.
 * 
 * when entering a fclosure, for each variable there are three possibities: (a)
 * this is the first instance of this closed variable (b) this is the second
 * or greater recursive instance of this closure variable, however it hasn't
 * been normally lambda bound since the last closure invocation (c) like (b)
 * but it has been lambda bound before the most recent closure.
 * 
 * case (a) can be determined by seeing if the count is 0. if the count is >0
 * then we must scan from the top of the stack down until we find either the
 * closure or a lambda binding of the variable this determines whether it is
 * case (b) or (c).
 * 
 * There are three actions to perform in this routine: 1.  determine the closure
 * type (a,b or c) and do any binding necessary 2.  call the closure function
 * 3.  unbind any necessary closure variables.
 * 
 * Now, the details of those actions: 1. for case (b), do nothing as we are
 * still working with the correct value for case (a), pushdown the symbol and
 * give it the value from the closure, inc the closure count push a closure
 * marker on the bindstack too. for case (c), must locate the correct value
 * to set by searching for the last lambda binding before the previous
 * closure. pushdown the symbol and that value, inc the closure count push a
 * closure marker on the bindstack too. a closure marker has atom ==
 * int:closure-marker and value pointing to the closure list.  This will be
 * noticed when unbinding.
 * 
 * 3. unbinding is just like popnames except if a closure marker is seen, then
 * this must be done: if the count is 1, just store the symbol's value in the
 * closure and decrement the count. if the count is >1, then search up the
 * stack for the last lambda before the next occurance of this closure
 * variable and set its value to the current value of the closure. decrement
 * the closure count.
 * 
 * clos is the fclosure, funcallp is TRUE if this is called from funcall,
 * otherwise it is called from apply
 */

#define Case_A 0
#define Case_B 1
#define Case_C 2

lispval
Ifclosure(lispval clos, int funcallp)
{
	struct nament  *oldbnp = bnp, *lbnp;
	int             i;
	int             vlength, tcase, foundc;
	lispval         handy, atm_dtpr, value_dtpr;
	Savestack(3);

	/* bind variables to their values given in the fclosure */
	vlength = VecTotSize(clos->vl.vectorl[VSizeOff]);
	/*
	 * vector length must be positive (it has to have a function at
	 * least)
	 */
	if (vlength < 1)
		errorh1(Vermisc, "funcall: fclosure has wrong size ", nil, FALSE, 0, clos);

	// numvars = (vlength - 1);/* number of varibles */

	for (i = 1; i < vlength; i += 1) {
		atm_dtpr = clos->v.vector[i];	/* car is symbol name */
		value_dtpr = atm_dtpr->d.cdr;	/* car: value, cdr:  fixnum
						 * count */

		if (value_dtpr->d.cdr->i == 0)
			tcase = Case_A;	/* first call */
		else {
			lbnp = locatevar(atm_dtpr, &foundc, bnp - 1);
			if (!foundc) {
				/*
				 * didn't find the expected closure, count
				 * must be wrong, correct it and assume case
				 * (a)
				 */
				tcase = Case_A;
				value_dtpr->d.cdr->i = 0;
			} else if (lbnp)
				tcase = Case_C;	/* found intermediate lambda
						 * bnd */
			else
				tcase = Case_B;	/* no intermediate lambda
						 * bind */
		}

		/* now bind the value if necessary */
		switch (tcase) {
		case Case_A:
			PUSHDOWN(atm_dtpr->d.car, value_dtpr->d.car);
			PUSHVAL(clos_marker, atm_dtpr);
			value_dtpr->d.cdr->i += 1;
			break;

		case Case_B:
			break;	/* nothing to do */

		case Case_C:	/* push first bound value after last close */
			PUSHDOWN(atm_dtpr->d.car, lbnp->val);
			PUSHVAL(clos_marker, atm_dtpr);
			value_dtpr->d.cdr->i += 1;
			break;
		}
	}

	if (funcallp)
		handy = Ifuncal(clos->v.vector[0]);
	else {
		handy = lbot[-2].val;	/* get args to apply.  This is hacky
					 * and may fail if apply is changed */
		lbot = np;
		protect(clos->v.vector[0]);
		protect(handy);
		handy = Lapply();
	}

	xpopnames(oldbnp);	/* pop names with consideration for closure
				 * markers */

	if (!funcallp)
		Restorestack();
	return (handy);
}

/*
 * xpopnames :: pop values from bindstack, but look out for closure markers.
 * This is  used (instead of the faster popnames) when we know there will be
 * closure markers or when we can't be sure that there won't be closure
 * markers (eg. in non-local go's)
 */
void 
xpopnames(struct nament *llimit)
{
	struct nament  *rnp, *lbnp;
	lispval         atm_dtpr, value_dtpr;
	int             foundc;

	for (rnp = bnp; --rnp >= llimit;) {
		if (rnp->atm == clos_marker) {
			atm_dtpr = rnp->val;
			value_dtpr = atm_dtpr->d.cdr;
			if (value_dtpr->d.cdr->i <= 1) {
				/*
				 * this is the only occurance of this closure
				 * variable just restore current value to
				 * this closure.
				 */
				value_dtpr->d.car = atm_dtpr->d.car->a.clb;
			} else {
				/*
				 * locate the last lambda before the next
				 * occurance of this closure and store the
				 * current symbol's value there
				 */
				lbnp = locatevar(atm_dtpr, &foundc, rnp - 2);
				if (!foundc) {
					/*
					 * strange, there wasn't a closure to
					 * be found. well, we will fix things
					 * up so the count is right.
					 */
					value_dtpr->d.car = atm_dtpr->d.car->a.clb;
					value_dtpr->d.cdr->i = 1;
				} else if (lbnp) {
					/*
					 * note how the closures value isn't
					 * necessarily stored in the closure,
					 * it may be stored on the bindstack
					 */
					lbnp->val = atm_dtpr->d.car->a.clb;
				}
				/*
				 * the case where lbnp is 0 should never
				 * happen, but if it does, we can just do
				 * nothing safely
				 */
			}
			value_dtpr->d.cdr->i -= 1;
		} else
			rnp->atm->a.clb = rnp->val;	/* the normal case */
	}
	bnp = llimit;
}


struct nament  *
locatevar(lispval clos, int *foundc, struct nament *rnp)
{
	struct nament  *retbnp;
	lispval         symb;

	retbnp = (struct nament *) 0;
	*foundc = 0;

	symb = clos->d.car;

	for (; rnp >= orgbnp; rnp--) {
		if ((rnp->atm == clos_marker) && (rnp->val == clos)) {
			*foundc = 1;	/* found the closure */
			return (retbnp);
		}
		if (rnp->atm == symb)
			retbnp = rnp;
	}
	return (retbnp);
}

lispval
LIfss(void)
{
	lispval         atm_dtpr = nil, value_dtpr = nil;
	struct nament  *lbnp = NULL;
	int             tcase, foundc = 0;
	lispval         newval = nil;
	int             argc = 1;
	Savestack(2);

	switch (np - lbot) {
	case 2:
		newval = np[-1].val;
		argc++;
	case 1:
		atm_dtpr = lbot->val;
		value_dtpr = atm_dtpr->d.cdr;
		break;
	default:
		return argerr("int:fclosure-symbol-stuff");
	}
	/* this code is copied from Ifclosure */

	if (value_dtpr->d.cdr->i == 0)
		tcase = Case_A;	/* closure is not active */
	else {
		lbnp = locatevar(atm_dtpr, &foundc, bnp - 1);
		if (!foundc) {
			/*
			 * didn't find closure, count must be wrong, correct
			 * it and assume case (a).
			 */
			tcase = Case_A;
			value_dtpr->d.cdr->i = 0;
		} else if (lbnp)
			tcase = Case_C;	/* found intermediate lambda */
		else
			tcase = Case_B;
	}

	Restorestack();
	switch (tcase) {
	case Case_B:
		if (argc == 2)
			return (atm_dtpr->d.car->a.clb = newval);
		return (atm_dtpr->d.car->a.clb);

	case Case_A:
		if (argc == 2)
			return (value_dtpr->d.car = newval);
		return (value_dtpr->d.car);

	case Case_C:
		if (argc == 2)
			return (lbnp->val = newval);
		return (lbnp->val);
	default:
		abort();
		return nil;
	}
	/* NOTREACHED */
}
