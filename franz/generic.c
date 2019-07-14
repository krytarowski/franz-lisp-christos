/* Functions corresponding to those in 68k.c */
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: generic.c,v 1.2 2014/12/25 23:57:55 christos Exp $");
#endif

#include "global.h"
#include <signal.h>
#include <stdio.h>
#include <math.h>

static lispval  isho(int);

int
mmuladd(intptr_t a, intptr_t b, intptr_t c, intptr_t m)
{
	int             work[2];
	char            err;
	emul(a, b, c, work);
	ediv(work, m, &err);
	return (work[0]);
}

void
Imuldiv(intptr_t p1, intptr_t p2, intptr_t add, intptr_t dv, intptr_t *quo, intptr_t *rem)
{
	int             work[2];
	char            err;

	emul(p1, p2, add, work);
	*quo = ediv(work, dv, &err);
	*rem = *work;
}

lispval
Lpolyev(void)
{
	/* Interface to VAX polyd instruction -- polynomial eval */
	return error("polyev has not been implemented", FALSE);
}


lispval
Lrot(void)
{				/* From tahoe version */
	int             val;
	uintptr_t       mask2 = -1;
	struct argent  *mylbot = lbot;
	intptr_t        rot;

	chkarg(2, "rot");
	if ((TYPE(mylbot->val) != INT) || (TYPE(mylbot[1].val) != INT))
		errorh2(Vermisc,
			"Non ints to rot",
			nil, FALSE, 0, mylbot->val, mylbot[1].val);
	val = mylbot[0].val->i;
	rot = mylbot[1].val->i;
	rot = rot & 0x3f;	/* bring it down below one byte in size */
	mask2 >>= rot;
	mask2 ^= -1;
	mask2 &= val;
	mask2 >>= (32 - rot);
	val <<= rot;
	val |= mask2;
	return (inewint(val));
}

/*
 * i386 bsd 4.3 version of kernel showstack (there is also a Lisp version)
 * 
 * We look through the C stack for return addresses in the routine eval.  When
 * we find one, the first argument in the previous frame (the return to
 * eval's caller) is the argument to eval.  By printing these arguments, we
 * produce a backtrace of the nested evaluations that are still active,
 * beginning with the most recent.
 * 
 * To start looking at stack frames, though, we first have to find our own.
 * This is done by taking an offset from the first paremeter. (The first
 * local variable could also have been used.)
 * 
 * The main routine is 'isho'.  It can produce two styles of backtrace.
 * 
 * (showstack)
 * 
 * will print the entire form at each level, while
 * 
 * (baktrace)
 * 
 * prints just the function names.
 * 
 */

#include "machframe.h"

lispval
Lshostk(void)
{				/* (showstack) */
	return (isho(1));
}

lispval
Lbaktrace(void)
{				/* (baktrace) */
	return isho(0);
}

static          lispval
isho(int f)
{
	int           **fp __attribute__((__unused__));	/* this must be the
							 * first local */
	struct machframe *myfp;
	lispval         handy;
	int             virgin = 1;
	extern int      plevel, plength;

	if (TYPE(Vprinlevel->a.clb) == INT) {
		plevel = Vprinlevel->a.clb->i;
	} else
		plevel = -1;
	if (TYPE(Vprinlength->a.clb) == INT) {
		plength = Vprinlength->a.clb->i;
	} else
		plength = -1;

	if (f == 1)
		printf("Forms in evaluation:\n");
	else
		printf("Backtrace:\n\n");

	/* point to current machframe */
	myfp = (struct machframe *)
		((char *) &f - sizeof(*myfp) + sizeof(myfp->ap));

	while (TRUE) {
		if (((void *) myfp->pc > (void *) eval &&	/* interpreted code */
		     (void *) myfp->pc < (void *) popnames)
		    ||
		    ((void *) myfp->pc > (void *) Ifuncal &&	/* compiled code */
		     (void *) myfp->pc < (void *) Lfuncal)) {
			{
				handy = (myfp->fp->ap[0]);
				if (f == 1)
					printr(handy, stdout), putchar('\n');
				else {
					if (virgin)
						virgin = 0;
					else
						printf(" -- ");
					printr((TYPE(handy) == DTPR) ? handy->d.car : handy, stdout);
				}
			}

		}
		if (myfp > myfp->fp)
			break;	/* end of frames */
		else
			myfp = myfp->fp;
	}
	putchar('\n');
	return (nil);
}


/*
 * (int:showstack 'stack_pointer)
 *
 * This is used by the in-Lisp versions of showstack and baktrace.
 *
 * return
 *   nil if at the end of the stack or illegal
 *   ( expresssion . next_stack_pointer) otherwise
 *   where expression is something passed to eval
 */
lispval
LIshowstack(void)
{
	struct machframe *fp = __builtin_frame_address(0);
	const void *stack = &stack;
	lispval         handy;
	struct machframe *myfp;
	lispval         rval;
	Savestack(2);

	chkarg(1, "int:showstack");

	if ((TYPE(handy = lbot[0].val) != INT) && (handy != nil))
		error("int:showstack non fixnum arg", FALSE);
#ifdef __MACHINE_STACK_GROWS_UP
#define BELOW >
#else
#define BELOW <
#endif

	if (handy == nil)
		myfp = fp;
	else
		myfp = (struct machframe *) handy->i;


	if ((const void *)myfp BELOW stack)
		error("int:showstack illegal stack value", FALSE);
	while (myfp > 0) {
		if (((void *) myfp->pc > (void *) eval &&	/* interpreted code */
		     (void *) myfp->pc < (void *) popnames)
		    ||
		    ((void *) myfp->pc > (void *) Ifuncal &&	/* compiled code */
		     (void *) myfp->pc < (void *) Lfuncal)) {
			{
				handy = (lispval) (myfp->ap[0]);	/* arg to eval */

				protect(rval = newdot());
				rval->d.car = handy;
				if (myfp > myfp->fp)
					myfp = 0;	/* end of frames */
				else
					myfp = myfp->fp;
				rval->d.cdr = inewint((intptr_t) myfp);
				Restorestack();
				return (rval);
			}
		}
		if ((const void *)myfp BELOW (const void *)myfp->fp)
			myfp = 0;	/* end of frames */
		else
			myfp = myfp->fp;

	}
	Restorestack();
	return (nil);
}



void
myfrexp(double d, int *ex, int *hi, int *lo)
{
	union {
		double          d;
		int             i[2];
	}               x;
	x.d = frexp(d, ex);
	*hi = x.i[0];
	*lo = x.i[1];
}


/* Dummy stubs */
lispval
Lmkcth(void)
{
	fprintf(stderr, "mkcth called\n");
	return nil;
}
