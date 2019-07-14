/*-					-[Sat Jan 29 12:22:36 1983 by jkf]-
 * 	divbig.c
 * bignum division
 *
 * (c) copyright 1982, Regents of the University of California
 */

#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: divbig.c,v 1.4 83/11/26 12:10:16 sklower Exp";
#else
__RCSID("$Id: divbig.c,v 1.8 2012/04/14 21:49:19 christos Exp $");
#endif
#endif

#include <stdlib.h>
#include "global.h"

#define b 0x40000000
#define toint(p) ((intptr_t) (p))

void
divbig(lispval dividend, lispval divisor, lispval *quotient, lispval *remainder)
{
	intptr_t       *ujp;
	int             d, negflag = 0, m, n, rem, qhat, j;
	int             borrow, negrem = 0;
	intptr_t       *utop = sp(), *ubot, *vbot, *qbot;
	lispval         work;
	Keepxs();

	/* copy dividend */
	for (work = dividend; work; work = work->s.CDR)
		stack(work->s.I);
	ubot = sp();
	if (*ubot < 0) {	/* knuth's division alg works only for pos
				 * bignums				 */
		negflag ^= 1;
		negrem = 1;
		dsmult(utop - 1, ubot, -1);
	}
	stack(0);
	ubot = sp();


	/* copy divisor */
	for (work = divisor; work; work = work->s.CDR)
		stack(work->s.I);

	vbot = sp();
	stack(0);
	if (*vbot < 0) {
		negflag ^= 1;
		dsmult(ubot - 1, vbot, -1);
	}
	/* check validity of data */
	n = ubot - vbot;
	m = utop - ubot - n - 1;
	if (n == 1) {
		/* do destructive division by  a single. */
		rem = dsdiv(utop - 1, ubot, *vbot);
		if (negrem)
			rem = -rem;
		if (negflag)
			dsmult(utop - 1, ubot, -1);
		if (remainder)
			*remainder = inewint(rem);
		if (quotient)
			*quotient = export(utop, ubot);
		Freexs();
		return;
	}
	if (m < 0) {
		if (remainder)
			*remainder = dividend;
		if (quotient)
			*quotient = inewint(0);
		Freexs();
		return;
	}
	qbot = alloca(toint(utop) + toint(vbot) - 2 * toint(ubot));
	d = b / (*vbot + 1);
	dsmult(utop - 1, ubot, d);
	dsmult(ubot - 1, vbot, d);

	for (j = 0, ujp = ubot; j <= m; j++, ujp++) {

		qhat = calqhat(ujp, vbot);
		if ((borrow = mlsb(ujp + n, ujp, ubot, -qhat)) < 0) {
			adback(ujp + n, ujp, ubot);
			qhat--;
		}
		qbot[j] = qhat;
	}
	if (remainder) {
		dsdiv(utop - 1, utop - n, d);
		if (negrem)
			dsmult(utop - 1, utop - n, -1);
		*remainder = export(utop, utop - n);
	}
	if (quotient) {
		if (negflag)
			dsmult(qbot + m, qbot, -1);
		*quotient = export(qbot + m + 1, qbot);
	}
	Freexs();
}

lispval
export(intptr_t *top, intptr_t *bot)
{
	lispval         p;
	lispval         result;

	top--;			/* screwey convention matches original vax
				 * assembler convenience */
	while (bot < top) {
		if (*bot == 0)
			bot++;
		else if (*bot == -1)
			*++bot |= 0xc0000000;
		else
			break;
	}
	if (bot == top)
		return (inewint(*bot));
	result = p = newsdot();
	protect(p);
	p->s.I = *top--;
	while (top >= bot) {
		p = p->s.CDR = newdot();
		p->s.I = *top--;
	}
	p->s.CDR = 0;
	np--;
	return (result);
}

#define MAXINT 0x80000000U

int
Ihau(int fix)
{
	int             count;
	if ((unsigned int)fix == MAXINT)
		return (32);
	if (fix < 0)
		fix = -fix;
	for (count = 0; fix; count++)
		fix /= 2;
	return (count);
}
lispval
Lhau(void)
{
	int             count;
	lispval         handy;

	handy = lbot->val;
top:
	switch (TYPE(handy)) {
	case INT:
		count = Ihau(handy->i);
		break;
	case SDOT:
		handy = Labsval();
		for (count = 0; handy->s.CDR != ((lispval) 0); handy = handy->s.CDR)
			count += 30;
		count += Ihau(handy->s.I);
		break;
	default:
		handy = errorh1(Vermisc, "Haulong: bad argument", nil,
				TRUE, 997, handy);
		goto top;
	}
	return (inewint(count));
}

lispval
Lhaipar(void)
{
	lispval         work;
	int             n;
	intptr_t       *top = sp() - 1;
	intptr_t       *bot;
	int             mylen;

	/* chkarg(2); */
	work = lbot->val;
	/* copy data onto stack */
on1:
	switch (TYPE(work)) {
	case INT:
		stack(work->i);
		break;
	case SDOT:
		for (; work != ((lispval) 0); work = work->s.CDR)
			stack(work->s.I);
		break;
	default:
		work = errorh1(Vermisc, "Haipart: bad first argument", nil,
			       TRUE, 996, work);
		goto on1;
	}
	bot = sp();
	if (*bot < 0) {
		stack(0);
		dsmult(top, bot, -1);
		bot--;
	}
	for (; *bot == 0 && bot < top; bot++);
	/* recalculate haulong internally */
	mylen = (top - bot) * 30 + Ihau(*bot);
	/* get second argument		  */
	work = lbot[1].val;
	while (TYPE(work) != INT)
		work = errorh1(Vermisc, "Haipart: 2nd arg not int", nil,
			       TRUE, 995, work);
	n = work->i;
	if (n >= mylen || -n >= mylen)
		goto done;
	if (n == 0)
		return (inewint(0));
	if (n > 0) {
		/*
		 * Here we want n most significant bits so chop off mylen - n
		 * bits
		 */
		stack(0);
		n = mylen - n;
		for (; n >= 30; n -= 30)
			top--;
		if (top < bot)
			error("Internal error in haipart #1", FALSE);
		dsdiv(top, bot, 1 << n);

	} else {
		/* here we want abs(n) low order bits */
		stack(0);
		bot = top + 1;
		for (; n <= 0; n += 30)
			bot--;
		n = 30 - n;
		*bot &= ~(-1 << n);
	}
done:
	return (export(top + 1, bot));
}
#define STICKY 1
#define TOEVEN 2
lispval
Ibiglsh(lispval bignum, lispval count, int mode)
{
	lispval         work;
	int             n;
	intptr_t       *top = sp() - 1;
	intptr_t       *bot;
	int             guard = 0, sticky = 0, round = 0;

	/* get second argument		  */
	work = count;
	while (TYPE(work) != INT)
		work = errorh1(Vermisc, "Bignum-shift: 2nd arg not int", nil,
			       TRUE, 995, work);
	n = work->i;
	if (n == 0)
		return (bignum);
	for (; n >= 30; n -= 30) {	/* Here we want to multiply by 2^n so
					 * start by copying n/30 zeroes onto
					 * stack */
		stack(0);
	}

	work = bignum;		/* copy data onto stack */
on1:
	switch (TYPE(work)) {
	case INT:
		stack(work->i);
		break;
	case SDOT:
		for (; work != ((lispval) 0); work = work->s.CDR)
			stack(work->s.I);
		break;
	default:
		work = errorh1(Vermisc, "Bignum-shift: bad bignum argument", nil,
			       TRUE, 996, work);
		goto on1;
	}
	bot = sp();
	if (n >= 0) {
		stack(0);
		bot--;
		dsmult(top, bot, 1 << n);
	} else {
		/*
		 * Trimming will only work without leading zeroes without my
		 * having to think a lot harder about it, if the inputs are
		 * canonical
		 */
		for (n = -n; n > 30; n -= 30) {
			if (guard)
				sticky |= 1;
			guard = round;
			if (top > bot) {
				round = *top;
				top--;
			} else {
				round = *top;
				*top >>= 30;
			}
		}
		if (n > 0) {
			if (guard)
				sticky |= 1;
			guard = round;
			round = dsrsh(top, bot, -n, -1 << n);
		}
		stack(0);	/* so that dsadd1 will work; */
		if (mode == STICKY) {
			if (((*top & 1) == 0) && (round | guard | sticky))
				dsadd1(top, bot);
		} else if (mode == TOEVEN) {
			int             mask;

			if (n == 0)
				n = 30;
			mask = (1 << (n - 1));
			if (!(round & mask))
				goto chop;
			mask -= 1;
			if (((round & mask) == 0)
			    && guard == 0
			    && sticky == 0
			    && (*top & 1) == 0)
				goto chop;
			dsadd1(top, bot);
		}
chop:		;
	}
	work = export(top + 1, bot);
	return (work);
}

/*
 * From drb  Mon Jul 27 01:25:56 1981 To: sklower
 * 
 * The idea is that the answer/2 is equal to the exact answer/2 rounded towards
 * - infinity.  The final bit of the answer is the "or" of the true final
 * bit, together with all true bits after the binary point.  In other words,
 * the 1's bit of the answer is almost always 1.  THE FINAL BIT OF THE ANSWER
 * IS 0 IFF n*2^i = THE ANSWER RETURNED EXACTLY, WITH A 0 FINAL BIT.
 * 
 * 
 * To try again, more succintly:  the answer is correct to within 1, and the 1's
 * bit of the answer will be 0 only if the answer is exactly correct.
 */

lispval
Lsbiglsh(void)
{
	chkarg(2, "sticky-bignum-leftshift");
	return (Ibiglsh(lbot->val, lbot[1].val, STICKY));
}
lispval
Lbiglsh(void)
{
	chkarg(2, "bignum-leftshift");
	return (Ibiglsh(lbot->val, lbot[1].val, TOEVEN));
}
lispval
HackHex(void)
{				/* this is a one minute function so drb and
				 * kls can debug biglsh */
	/*
	 * (HackHex i) returns a string which is the result of printing i in
	 * hex
	 */
	char            buf[32];
	snprintf(buf, sizeof(buf), "%jx", (intmax_t)lbot->val->i);
	return ((lispval) inewstr(buf));
}
