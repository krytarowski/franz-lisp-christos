#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: qfuncl.c,v 1.4 2014/12/31 01:08:01 christos Exp $");
#endif

#include <sys/param.h>
#include <stdio.h>
#include "global.h"
#include "frame.h"

char            _erthrow[] = "Uncaught throw from compiled code";

lispval        *tynames[] = {
	(lispval *) nil,
	&str_name,
	&atom_name,
	&int_name,
	&dtpr_name,
	&doub_name,
	&funct_name,
	&port_name,
	&array_name,
	&other_name,
	&sdot_name,
	&val_name,
	&hunk_name[0],
	&hunk_name[1],
	&hunk_name[2],
	&hunk_name[3],
	&hunk_name[4],
	&hunk_name[5],
	&hunk_name[6],
	&vect_name,
	&vecti_name
};

void           *
gstart(void)
{
#ifndef __APPLE__
#ifdef BSD4_4
	extern void     start(void) __asm("___start");
	return (void *) &start;
#endif
#endif
}

void 
vlsub(int *a, int *b)
{
	a[HI] -= b[HI];
	if (b[LO] > a[LO])
		a[HI]--;
	a[LO] -= b[LO];
}

struct frame   *retframe;	/* gives value of Pushframe */
int             jmpval;		/* for use when debugging Pushframe */


/*
 * Ipushf -- set values into pushframe
 * 
 * This function is called only from within the Pushframe macro. Essentially,
 * what Pushframe does is to (1) call Ipushf to fill in most of the local
 * frame, assigning the result to retframe; (2) call setjmp to set the
 * frame's retenv; and (3) return the value of retframe.
 */

struct frame   *
Ipushf(int fclass, lispval arg1, lispval arg2, struct frame *loc_frame)
	/* frame local (auto storage) to caller */
{
	loc_frame->olderrp = errp;
	loc_frame->svlbot = lbot;
	loc_frame->svnp = np;
	loc_frame->svbnp = bnp;
	loc_frame->svxsp = xsp;
	loc_frame->class = fclass;
	loc_frame->larg1 = arg1;
	loc_frame->larg2 = arg2;

	retval = C_INITIAL;	/* indicate set rather than longjmp */

	return (loc_frame);	/* will be assigned to retframe */
}

/*
 * qretfromfr -- restore values from frame and do longjmp
 * 
 * The longjmp is supposed to look like another return from Pushframe.
 * Therefore, we must set retframe, so that it, in turn, can be used as the
 * Pushframe result.  This Pushframe return is distinguished from the one
 * that initialized the frame by the value of retval, set by our caller. The
 * value passed via longjmp is used only for debugging.
 */

int
qretfromfr(struct frame *loc_frame)
{
	lbot = loc_frame->svlbot;
	np = loc_frame->svnp;
	bnp = loc_frame->svbnp;
	xsp = loc_frame->svxsp;

	retframe = loc_frame;
	longjmp(loc_frame->retenv, 1);

	/* NOT REACHED */
}
