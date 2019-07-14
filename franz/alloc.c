/*-
 * 	alloc.c
 * storage allocator and garbage collector
 *
 * (c) copyright 1982, Regents of the University of California
 */
#define DEBUG
#define MARKDEBUG
#include <sys/cdefs.h>
#ifndef lint
#ifdef notdef
static char    *rcsid 
__attribute__((__unused__)) =
"Header: alloc.c,v 1.13 87/12/11 17:27:45 sklower Exp";
#else
__RCSID("$Id: alloc.c,v 1.17 2014/12/11 17:12:17 christos Exp $");
#endif
#endif

#include <sys/types.h>
#include <sys/times.h>
#ifdef METER
#include <sys/vtimes.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include "global.h"
#include "structs.h"

#define BITLONGS TTSIZE * LWORDS	/* length of bit map in long words  */

#define ftstbit(p) 					\
	do {						\
		intptr_t r, s;				\
		if (readbit(p, &r, &s))			\
			return;				\
		bitmapi[r] |= s;			\
	} while (/*CONSTCOND*/0)

#define readbit(p, r, s)	rb((p), (r), (s))
#define setbit(p)		sb((p))

#define roundup(x,l)	(((x - 1) | (l - 1)) + 1)

#define MARKDP(a)	markdp(a)

#define MARKVAL(v)	if(((intptr_t)v) >= (intptr_t)beginsweep) MARKDP(v)
#define ATOLX(p)	(((intptr_t)p) >> (LSHIFT - LALIGN))
#define MSKLX(p)	((((intptr_t)p) >> LALIGN) & LMASK)

/* METER denotes something added to help meter storage allocation. */

extern intptr_t *beginsweep;	/* first sweepable data		 */
extern char     purepage[];
extern int      gcstrings;
#define DEBUG_BITS	0x01
#define DEBUG_GC	0x02
#define DEBUG_VEC	0x04
#define DEBUG_ARR	0x08
#define DEBUG_STR	0x10
int             debugin = 0;	/* temp debug flag */

intptr_t	bitmapi[BITLONGS];	/* the bit map--one bit per long  */
double          zeroq;		/* a quad word of zeros  */
char           *bbitmap = (char *)(void *)bitmapi;/* byte version of bit map
						   * array */
double         *qbitmap = (double *)(void *)bitmapi;/* integer version of bit
						     * map array */
#ifdef METER
extern int      gcstat;
extern struct vtimes
                premark, presweep, alldone;	/* actually struct tbuffer's */

extern int      mrkdpcnt;
extern int      conssame, consdiff, consnil;	/* count of cells whose cdr
						 * point to the same page and
						 * different pages
						 * respectively */
#endif
intptr_t bitmsk[] = {	/* used by bit-marking macros */
0x1L,                0x2L,	          0x4L,                0x8L,
0x10L,               0x20L,               0x40L,               0x80L,
0x100L,              0x200L,              0x400L,              0x800L,
0x1000L,             0x2000L,             0x4000L,             0x8000L,
0x10000L,            0x20000L,            0x40000L,            0x80000L,
0x100000L,           0x200000L,           0x400000L,           0x800000L,
0x1000000L,          0x2000000L,          0x4000000L,          0x8000000L,
0x10000000L,         0x20000000L,         0x40000000L,         0x80000000L,
#ifdef _LP64
0x100000000L,        0x200000000L,        0x400000000L,        0x800000000L,
0x1000000000L,       0x2000000000L,       0x4000000000L,       0x8000000000L,
0x10000000000L,      0x20000000000L,      0x40000000000L,      0x80000000000L,
0x100000000000L,     0x200000000000L,     0x400000000000L,     0x800000000000L,
0x1000000000000L,    0x2000000000000L,    0x4000000000000L,    0x8000000000000L,
0x10000000000000L,   0x20000000000000L,   0x40000000000000L,   0x80000000000000L,
0x100000000000000L,  0x200000000000000L,  0x400000000000000L,  0x800000000000000L,
0x1000000000000000L, 0x2000000000000000L, 0x4000000000000000L, 0x8000000000000000L,
#endif
};
extern intptr_t *bind_lists;	/* lisp data for compiled code */

extern struct types atom_str, strng_str, int_str, dtpr_str, doub_str, array_str,
                sdot_str, val_str, funct_str, hunk_str[], vect_str, vecti_str,
                other_str;

extern struct str_x str_current[];

lispval         hunk_items[7], hunk_pages[7], hunk_name[7];

extern int      initflag;	/* starts off TRUE: initially gc not allowed */


/*
 * this is a table of pointers to all struct types objects the index is the
 * type number.
 */
static struct types *spaces[NUMSPACES] =
{&strng_str, &atom_str, &int_str,
	&dtpr_str, &doub_str, &funct_str,
	(struct types *) 0,	/* port objects not allocated in this way  */
	&array_str,
	&other_str,		/* other objects not allocated in this way  */
	&sdot_str, &val_str,
	&hunk_str[0], &hunk_str[1], &hunk_str[2],
	&hunk_str[3], &hunk_str[4], &hunk_str[5],
	&hunk_str[6],
&vect_str, &vecti_str};


static inline intptr_t
rb(void *p, intptr_t *r, intptr_t *s)
{
	*r = ATOLX(p);
	*s = bitmsk[MSKLX(p)];
	if (debugin & DEBUG_BITS)
		printf("rb %p[%jx] = %jx s=%jx\n", p, (intmax_t)*r,
		    (intmax_t)bitmapi[*r], (intmax_t)*s);
	return bitmapi[*r] & *s;
}

static inline void
sb(void *p)
{
	bitmapi[ATOLX(p)] |= bitmsk[MSKLX(p)];
}

/*
 * this is a table of pointers to collectable struct types objects the index
 * is the type number.
 */
struct types   *gcableptr[] = {
#ifndef GCSTRINGS
	(struct types *) 0,	/* strings not collectable */
#else
	&strng_str,
#endif
	&atom_str,
	&int_str, &dtpr_str, &doub_str,
	(struct types *) 0,	/* binary objects not collectable */
	(struct types *) 0,	/* port objects not collectable */
	&array_str,
	(struct types *) 0,	/* gap in the type number sequence */
	&sdot_str, &val_str,
	&hunk_str[0], &hunk_str[1], &hunk_str[2],
	&hunk_str[3], &hunk_str[4], &hunk_str[5],
	&hunk_str[6],
&vect_str, &vecti_str};


/*
 *   get_more_space(type_struct,purep)
 *
 *  Allocates and structures a new page, returning 0.
 *  If no space is available, returns positive number.
 *  If purep is TRUE, then pure space is allocated.
 */
int 
get_more_space (struct types *type_struct, int purep)
{
	int             cntr;
	char           *start;
	intptr_t       *loop, *temp;
	lispval         p;
	ptrdiff_t	len;

	if ((intptr_t) datalim >= TTSIZE * LBPG)
		return (2);

	start = xsbrk(1);	/* get new page */


	SETTYPE(start, type_struct->type, 20);	/* set type of page  */

	purepage[ATOX(start)] = (char) purep;	/* remember if page was pure */

	/* bump the page counter for this space if not pure */

	if (!purep)
		++((*(type_struct->pages))->i);

	type_struct->space_left = type_struct->space;
	temp = loop = (intptr_t *) start;
	for (cntr = 1; cntr < type_struct->space; cntr++) {
		loop = (intptr_t *) (*loop = (intptr_t) (loop + type_struct->type_len));
	}

	len = (char *)loop - (char *)start;
	if (len >= LBPG)
		abort();

	/*
	 * attach new cells to either the pure space free list  or the
	 * standard free list
	 */
	if (purep) {
		*loop = (intptr_t) (type_struct->next_pure_free);
		type_struct->next_pure_free = (char *) temp;
	} else {
		*loop = (intptr_t) (type_struct->next_free);
		type_struct->next_free = (char *) temp;
	}

	/* if type atom, set pnames to CNIL  */

	if (type_struct == &atom_str)
		for (cntr = 0, p = (lispval) temp; cntr < atom_str.space; ++cntr) {
			p->a.pname = (char *) CNIL;
			p = (lispval) ((intptr_t *) p + atom_str.type_len);
		}
	return (0);		/* space was available  */
}


/*
 * next_one(type_struct)
 *
 *  Allocates one new item of each kind of space, except STRNG.
 *  If there is no space, calls gc, the garbage collector.
 *  If there is still no space, allocates a new page using
 *  get_more_space
 */

static void
check(const char *file, int line, const char *fr) {
#if 0
struct types *type_struct;
int i;
char *t;
for (i = 0; i < NUMSPACES; i++) {
	type_struct = spaces[i];
	if (type_struct == NULL)
		continue;
int j = 0;
for (t = type_struct->next_free; t != (char *)CNIL; t = *((char **)t)) { printf("%p,\n", t); fflush(stdout); if (j++ == 1000) abort();}
printf("\n");
}
#endif
}

lispval
next_one(struct types *type_struct)
{
	char           *temp;

	while (type_struct->next_free == (char *) CNIL) {
		int             g;

		if (
		    (initflag == FALSE) &&	/* dont gc during init */
#ifndef GCSTRINGS
		    (type_struct->type != STRNG) &&	/* can't collect strings */
#else
		    gcstrings &&/* user (sstatus gcstrings) */
#endif
		    (type_struct->type != BCD) &&	/* nor function headers  */
		    gcdis->a.clb == nil) {	/* gc not disabled */
			/* not to collect during load */
			gc(type_struct);	/* collect  */
		}
		if (type_struct->next_free != (char *) CNIL)
			break;

		if (!(g = get_more_space(type_struct, FALSE)))
			break;

		space_warn(g);
	}
		
	temp = type_struct->next_free;
	type_struct->next_free = *(char **) (type_struct->next_free);
	(*(type_struct->items))->i++;
	return ((lispval) temp);
}
/*
 * Warn about exhaustion of space,
 * shared with next_pure_free().
 */
void 
space_warn (int g)
{
	if (g == 1) {
		plimit->i += NUMSPACES;	/* allow a few more pages  */
		copval(plima, plimit);	/* restore to reserved reg  */

		error("PAGE LIMIT EXCEEDED--EMERGENCY PAGES ALLOCATED", TRUE);
	} else
		error("SORRY, ABSOLUTE PAGE LIMIT HAS BEEN REACHED", TRUE);
}


/*
 * allocate an element of a pure structure.  Pure structures will be ignored
 * by the garbage collector.
 */
lispval
next_pure_one(struct types *type_struct)
{

	char           *temp;

	while (type_struct->next_pure_free == (char *) CNIL) {
		int             g;
		if (!(g = get_more_space(type_struct, TRUE)))
			break;
		space_warn(g);
	}

	temp = type_struct->next_pure_free;
	type_struct->next_pure_free = *(char **) (type_struct->next_pure_free);
	return ((lispval) temp);
}

lispval
newint(void)
{
	return (next_one(&int_str));
}

lispval
pnewint(void)
{
	return (next_pure_one(&int_str));
}

lispval
newdot(void)
{
	lispval         temp;

	temp = next_one(&dtpr_str);
	temp->d.car = temp->d.cdr = nil;
	return (temp);
}

lispval
pnewdot(void)
{
	lispval         temp;

	temp = next_pure_one(&dtpr_str);
	temp->d.car = temp->d.cdr = nil;
	return (temp);
}

lispval
newdoub(void)
{
	return (next_one(&doub_str));
}

lispval
pnewdb(void)
{
	return (next_pure_one(&doub_str));
}

lispval
newsdot(void)
{
	lispval         temp;
	temp = next_one(&sdot_str);
	temp->d.car = temp->d.cdr = 0;
	return (temp);
}

lispval
pnewsdot(void)
{
	lispval         temp;
	temp = next_pure_one(&sdot_str);
	temp->d.car = temp->d.cdr = 0;
	return (temp);
}

struct atom *
newatom(int pure)
{
	struct atom    *save;
	char           *mypname;

	mypname = newstr(pure);
	pnameprot = ((lispval) mypname);
	save = (struct atom *) next_one(&atom_str);
	save->plist = save->fnbnd = nil;
	save->hshlnk = (struct atom *) CNIL;
	save->clb = CNIL;
	save->pname = mypname;
	return (save);
}

char *
newstr(int purep)
{
	char           *save;
	int             atomlen;
	struct str_x   *p = str_current + purep;

	atomlen = strlen(strbuf) + 1;
	if (atomlen > p->space_left) {
		if (atomlen >= STRBLEN) {
			save = (char *) csegment(OTHER, atomlen, purep);
			SETTYPE(save, STRNG, 40);
			purepage[ATOX(save)] = (char) purep;
			strcpy(save, strbuf);
			return (save);
		}
		p->next_free = (char *) (purep ?
			  next_pure_one(&strng_str) : next_one(&strng_str));
		p->space_left = LBPG;
	}
	strcpy((save = p->next_free), strbuf);
	if (debugin & DEBUG_STR)
		printf("%p %s %d\n", save, save, atomlen);
#if 0
	while (atomlen & 3)
		++atomlen;	/* even up length of string  */
#endif
	p->next_free += atomlen;
	p->space_left -= atomlen;
	return (save);
}

static char    *
Iinewstr(char *s, int purep)
{
	int             len = strlen(s);
	while (len > (estrbuf - strbuf - 1))
		atomtoolong(strbuf);
	strcpy(strbuf, s);
	return (newstr(purep));
}


char *
inewstr(char *s)
{
	return Iinewstr(s, 0);
}

char *
pinewstr(char *s)
{
	return Iinewstr(s, 1);
}

lispval
newarray(void)
{
	lispval         temp;

	temp = next_one(&array_str);
	temp->ar.data = (char *) nil;
	temp->ar.accfun = nil;
	temp->ar.aux = nil;
	temp->ar.length = SMALL(0);
	temp->ar.delta = SMALL(0);
	return (temp);
}

lispval
newfunct(void)
{
	lispval         temp;
	temp = next_one(&funct_str);
	temp->bcd.start = Badcall;
	temp->bcd.discipline = nil;
	return (temp);
}

lispval
newval(void)
{
	lispval         temp;
	temp = next_one(&val_str);
	temp->l = nil;
	return (temp);
}

lispval
pnewval(void)
{
	lispval         temp;
	temp = next_pure_one(&val_str);
	temp->l = nil;
	return (temp);
}

lispval
newhunk(int hunknum)
{
	lispval         temp;

	temp = next_one(&hunk_str[hunknum]);	/* Get a hunk */
	return (temp);
}

lispval
pnewhunk(int hunknum)
{
	lispval         temp;

	temp = next_pure_one(&hunk_str[hunknum]);	/* Get a hunk */
	return (temp);
}

lispval
inewval(lispval arg)
{
	lispval         temp;
	temp = next_one(&val_str);
	temp->l = arg;
	return (temp);
}

/*
 * Vector allocators.
 * a vector looks like:
 *  longword: N = size in bytes
 *  longword: pointer to lisp object, this is the vector property field
 *  N consecutive bytes
 *
 */

lispval
newvec(int size)
{
	return (getvec(size, &vect_str, FALSE));
}

lispval
pnewvec(int size)
{
	return (getvec(size, &vect_str, TRUE));
}

lispval
nveci(int size)
{
	return (getvec(size, &vecti_str, FALSE));
}

lispval
pnveci(int size)
{
	return (getvec(size, &vecti_str, TRUE));
}

/*
 * getvec
 *  get a vector of size byte, from type structure typestr and
 * get it from pure space if purep is TRUE.
 *  vectors are stored linked through their property field.  Thus
 * when the code here refers to v.vector[0], it is the prop field
 * and vl.vectorl[-1] is the size field.   In other code,
 * v.vector[-1] is the prop field, and vl.vectorl[-2] is the size.
 */
lispval
getvec(int size, struct types *typestr, int purep)
{
	lispval         back, current;
	int             sizewant, bytes, thissize, pages, pindex, triedgc = FALSE;

	/*
	 * we have to round up to a multiple of LWORDS bytes to determine
	 * the size of vector we want.  The rounding up assures that the
	 * property pointers are properly aligned
	 */
	sizewant = VecTotSize(size);
#ifdef DEBUG
	if (debugin & DEBUG_VEC)
		printf("want vect type=%d type_len=%jd size=%d sizewant=%d\n",
		    typestr->type, (intmax_t)typestr->type_len, size, sizewant);
#endif
again:
	if (purep)
		back = (lispval) & (typestr->next_pure_free);
	else
		back = (lispval) & (typestr->next_free);
	current = back->v.vector[0];
	while (current != CNIL) {
		thissize = VecTotSize(current->vl.vectorl[-1]);
#ifdef DEBUG
		if (debugin & DEBUG_VEC)
			printf("next free size %ld thissize=%d; sizewant=%d",
			    current->vl.vectorl[-1], thissize, sizewant);
#endif
		if (thissize == sizewant) {
#ifdef DEBUG
			if (debugin & DEBUG_VEC)
				printf("exact match of size %d at %p\n",
				    4 * thissize, &current->v.vector[1]);
#endif
			back->v.vector[0]
			    = current->v.vector[0]; /* change free pointer */
			current->v.vector[0] = nil; /* put nil in property */
			/* to the user, vector begins one after property */
			return (lispval)&current->v.vector[1];
		} else if (thissize >= sizewant + 3) {
			/*
			 * the reason that there is a `+ 3' instead of `+ 2'
			 * is that we don't want to leave a zero sized vector
			 * which isn't guaranteed to be followed by another
			 * vector
			 */
#ifdef DEBUG
			if (debugin & DEBUG_VEC) {
				printf("thissize %d sizewant %d\n",
				    thissize, sizewant);
				printf("breaking a %ld vector into a ",
				    current->vl.vectorl[-1]);
			}
#endif

			current->v.vector[1 + sizewant + 1]
				= current->v.vector[0];	/* free list pointer */
			current->vl.vectorl[1 + sizewant]
				= VecTotToByte(thissize - sizewant - 2);	/* size info */
			back->v.vector[0] = (lispval) & (current->v.vector[1 + sizewant + 1]);
			current->vl.vectorl[-1] = size;

#ifdef DEBUG
			if (debugin & DEBUG_VEC)
				printf(" %ld one and a %ld one\n",
				    current->vl.vectorl[-1],
				    current->vl.vectorl[1 + sizewant]);
#endif
			current->v.vector[0] = nil;	/* put nil in property */
			/* vector begins one after the property */
#ifdef DEBUG
			if (debugin & DEBUG_VEC)
				printf(" and returning vector at %p\n",
				    &current->v.vector[1]);
#endif
			return ((lispval) (&current->v.vector[1]));
		}
		back = current;
		current = current->v.vector[0];
	}
	if (!triedgc
	    && !purep
	    && (gcdis->a.clb == nil)
	    && (initflag == FALSE)) {
		gc(typestr);
		triedgc = TRUE;
		goto again;
	}
	/* set bytes to size needed for this vector */
	bytes = size + 2 * sizeof(intptr_t);

	/*
	 * must make sure that if the vector we are allocating doesnt
	 * completely fill a page, there is room for another vector to record
	 * the size left over
	 */
	if ((bytes & (LBPG - 1)) > (LBPG - 2 * sizeof(intptr_t)))
		bytes += LBPG;
	bytes = roundup(bytes, LBPG);

	current = csegment(typestr->type, bytes / sizeof(intptr_t), purep);
	current->vl.vectorl[0] = bytes - 2 * sizeof(intptr_t);

	if (purep) {
		current->v.vector[1] = (lispval) (typestr->next_pure_free);
		typestr->next_pure_free = (char *) &(current->v.vector[1]);
		/* make them pure */
		pages = bytes / LBPG;
		for (pindex = ATOX(current); pages; pages--) {
			purepage[pindex++] = TRUE;
		}
	} else {
		current->v.vector[1] = (lispval) (typestr->next_free);
		typestr->next_free = (char *) &(current->v.vector[1]);
#ifdef DEBUG
		if (debugin & DEBUG_VEC)
			printf("grabbed %zu vec pages\n", bytes / LBPG);
#endif
	}
#ifdef DEBUG
	if (debugin & DEBUG_VEC)
		printf("creating a new vec, size %p\n", current->v.vector[0]);
#endif
	goto again;
}

/*
 * Ipurep :: routine to check for pureness of a data item
 *
 */
lispval
Ipurep(lispval element)
{
	if (purepage[ATOX(element)])
		return (tatom);
	else
		return (nil);
}

/*
 * routines to return space to the free list.  These are used by the
 * arithmetic routines which tend to create large intermediate results which
 * are know to be garbage after the calculation is over.
 * 
 * There are jsb callable versions of these routines in qfuncl.s
 */

/*
 * pruneb   - prune bignum. A bignum is an sdot followed by a list of dtprs.
 * The dtpr list is linked by car instead of cdr so when we put it in the
 * free list, we have to change the links.
 */
void
pruneb(lispval bignum)
{
	lispval         temp = bignum;

	if (TYPE(temp) != SDOT)
		errorh(Vermisc, "value to pruneb not a sdot", nil, FALSE, 0);

	--(sdot_items->i);
	temp->s.I = (intptr_t) sdot_str.next_free;
	sdot_str.next_free = (char *) temp;

	/*
	 * bignums are not terminated by nil on the dual, they are terminated
	 * by (lispval) 0
	 */

	while ((temp = temp->s.CDR) != NULL) {
		if (TYPE(temp) != DTPR)
			errorh(Vermisc, "value to pruneb not a list",
			       nil, FALSE, 0);
		--(dtpr_items->i);
		temp->s.I = (intptr_t) dtpr_str.next_free;
		dtpr_str.next_free = (char *) temp;
	}
}
lispval
Badcall(void)
{
	return error("BAD FUNCTION DESCRIPTOR USED IN CALL", FALSE);
}


/*
 * Ngc
 *  this is the lisp function gc
 *
 */

lispval
Ngc(void)
{
	return (gc((struct types *) CNIL));
}

/*
 * gc(type_struct)
 *
 *  garbage collector:  Collects garbage by mark and sweep algorithm.
 *  After this is done, calls the Nlambda, gcafter.
 *  gc may also be called from LISP, as an  nlambda of no arguments.
 * type_struct is the type of lisp data that ran out causing this
 * garbage collection
 */
int             printall = 0;
lispval
gc(struct types *type_struct)
{
	lispval         save;
	struct tms      begin, finish;
	extern int      gctime;

	/*
	 * if this was called automatically when space ran out print out a
	 * message
	 */
	if (Vgcprint && (Vgcprint->a.clb != nil)
	    && (type_struct != (struct types *) CNIL)) {
		FILE           *port = okport(Vpoport->a.clb, poport);
		fprintf(port, "gc(%s):", (*(type_struct->type_name))->a.pname);
		fflush(port);
	}
	if (gctime)
		times(&begin);

if (1) check(__FILE__, __LINE__, "after gc");
	gc1();			/* mark&sweep */
if (1) check(__FILE__, __LINE__, "after gc");

	/* Now we call gcafter--special case if gc called from LISP */

	if (gccall1) {
		if (type_struct == (struct types *) CNIL)
			gccall1->d.cdr = nil;	/* make the call "(gcafter)" */
		else {
			gccall1->d.cdr = gccall2;
			if (type_struct)
			    gccall2->d.car = *(type_struct->type_name);
		}
		PUSHDOWN(gcdis, gcdis);	/* flag to indicate in gc */
		save = eval(gccall1);	/* call gcafter  */
		POP;			/* turn off flag  */
	} else
		save = NULL;

	if (gctime) {
		times(&finish);
		gctime += (finish.tms_utime - begin.tms_utime);
	}
	return (save);		/* return result of gcafter  */
}



/* gc1()  ************************************************************* */
/* */
/* Mark-and-sweep phase						 */

void 
gc1 (void)
{
	int             k;
	intptr_t       *start, bvalue, type_len;
	struct types   *s;
	intptr_t       *point, i, freecnt, itemstogo, bits, bindex, type,
	                bytestoclear;
	int             usedcnt;
	char           *pindex;
	struct argent  *loop2;
	struct nament  *loop3;
	struct atom    *symb;
	extern int      hashtop;

	pagerand();
	/* decide whether to check LISP structure or not  */


#ifdef METER
	vtimes(&premark, 0);
	mrkdpcnt = 0;
	conssame = consdiff = consnil = 0;
#endif

	/* first set all bit maps to zero  */


	bytestoclear = ((((intptr_t) datalim) - ((intptr_t) beginsweep)) >> (LSHIFT - LALIGN)) * LWORDS;
	memset(start = bitmapi + ATOLX(beginsweep), 0, bytestoclear);

	/* mark all atoms in the oblist */
	for (bvalue = 0; bvalue <= hashtop - 1; bvalue++) {	/* though oblist */
		for (symb = hasht[bvalue]; symb != (struct atom *) CNIL;
		     symb = symb->hshlnk) {
			MARKDP((lispval) symb);
		}
	}


	/*
	 * Mark all the atoms and ints associated with the hunk data types
	 */

	for (i = 0; i < 7; i++) {
		MARKDP(hunk_items[i]);
		MARKDP(hunk_name[i]);
		MARKDP(hunk_pages[i]);
	}
	/* next run up the name stack */
	if (np != NULL)
		for (loop2 = np - 1; loop2 >= orgnp; --loop2)
		    MARKVAL(loop2->val);

	/* now the bindstack (vals only, atoms are marked elsewhere ) */
	if (bnp != NULL)
		for (loop3 = bnp - 1; loop3 >= orgbnp; --loop3)
			MARKVAL(loop3->val);


	/* next mark all compiler linked data */
	/*
	 * if the Vpurcopylits switch is non nil (lisp variable $purcopylits)
	 * then when compiled code is read in, it tables will not be linked
	 * into this table and thus will not be marked here.  That is ok
	 * though, since that data is assumed to be pure.
	 */
	point = bind_lists;
	while ((start = point) != (intptr_t *) CNIL) {
		while (*start != -1) {
			MARKDP((lispval) *start);
			start++;
		}
		point = (intptr_t *) *(point - 1);
	}

	/* next mark all system-significant lisp data */


	for (i = 0; i < SIGNIF; ++i)
		MARKDP((lispsys[i]));

#ifdef METER
	vtimes(&presweep, 0);
#endif
	/* all accessible data has now been marked. */
	/* all collectable spaces must be swept,    */
	/* and freelists constructed.		    */

	/*
	 * first clear the structure elements for types we will sweep
	 */

	for (k = 0; k <= VECTORI; k++) {
		if ((s = gcableptr[k]) != 0) {
			if (k == STRNG && !gcstrings) {	/* don't do anything */
			} else if (*(s->items)) {
				(*(s->items))->i = 0;
				s->space_left = 0;
				s->next_free = (char *) CNIL;
			}
		}
	}

	if (debugin & DEBUG_GC)
		printf("pagewords=%zu, pagesize=%zu, align=%u\n",
		    PAGEWORDS, LBPG, LALIGN);
	/* sweep up in memory looking at gcable pages */
	for (start = beginsweep, bindex = ATOLX(start),
	     pindex = &purepage[ATOX(start)];
	     start < (intptr_t *) datalim;
	     start += PAGEWORDS, pindex++) {
		if (debugin & (DEBUG_GC|DEBUG_BITS))
			printf("start=%p PAGE=%jx bindex=%jx type=%jd\n",
			    start, (intmax_t)ATOLX(start), (intmax_t)bindex,
			    (intmax_t)TYPE(start));
		if (!(s = gcableptr[type = TYPE(start)]) || *pindex
#ifdef GCSTRINGS
		    || (type == STRNG && !gcstrings)
#endif
			) {
			/* ignore this page but advance pointer 	 */
			bindex += PAGEENTRIES;
			if (debugin & (DEBUG_GC|DEBUG_BITS))
				printf("PLUS4 %jx\n", (intmax_t)bindex);
			continue;
		}
		freecnt = 0;	/* number of free items found */
		usedcnt = 0;	/* number of used items found */

		point = start;
		if (ATOLX(point) != bindex) {
			printf("PAGE INCONSISTENCY TYPE %jd %jx != %jx\n",
			    (intmax_t)type,
			    (intmax_t)ATOLX(point), (intmax_t)bindex);
			abort();
		}
		/*
		 * sweep dtprs as a special case, since 1) there will
		 * (usually) be more dtpr pages than any other type 2) most
		 * dtpr pages will be empty so we can really win by special
		 * caseing the sweeping of massive numbers of free cells
		 */
		/*
		 * since sdot's have the same structure as dtprs, this code
		 * will work for them too
		 */
		if ((type == DTPR) || (type == SDOT)) {
			int j;
			intptr_t       *head, *lim;
			head = (intptr_t *) s->next_free;/* first value on free
							 * list */
			if (debugin & DEBUG_GC)
				printf("TYPE %jd init\n", (intmax_t)type);
			for (i = 0; i < PAGEENTRIES; i++) {
				/* 32 bits -> 16 dptrs */
				/* 64 bits -> 32 dptrs */
#define DPTRS	(LBITS / 2)
				bvalue = bitmapi[bindex++];
				if (bvalue == 0) {	/* if all are free	 */
					*point = (intptr_t) head;
					lim = point + LBITS;	
					for (point += 2; point < lim; point += 2) {
						*point = (intptr_t) (point - 2);
					}
					head = point - 2;
					freecnt += DPTRS;
				} else
					for (j = 0; j < DPTRS; j++) {
						if (!(bvalue & 1)) {
							freecnt++;
							*point = (intptr_t) head;
							head = point;
						}
#ifdef METER
						/*
						 * check if the page address
						 * of this cell is the same
						 * as the address of its cdr
						 */
						else if (FALSE && gcstat && (type == DTPR)) {
							if (((intptr_t) point & ~511)
							    == ((intptr_t) (*point) & ~511))
								conssame++;
							else
								consdiff++;
							usedcnt++;
						}
#endif
						else
							usedcnt++;	/* keep track of used */

						point += 2;
						bvalue = bvalue >> 2;
					}
			}
			s->next_free = (char *) head;
		} else if ((type == VECTOR) || (type == VECTORI)) {
			int             canjoin;
			intptr_t       *tempp;

			/*
			 * check if first item on freelist ends exactly at
			 * this page
			 */
			if (((tempp = (intptr_t *) s->next_free) != (intptr_t *) CNIL)
			  && ((VecTotSize(((lispval) tempp)->vl.vectorl[-1])
			       + 1 + tempp) == point))
				canjoin = TRUE;
			else
				canjoin = FALSE;

			if (debugin & (DEBUG_VEC|DEBUG_GC))
				printf("VECTOR* %jd init canJoin=%d\n",
				(intmax_t)type, canjoin);
			/* arbitrary sized vector sweeper */
			/*
			 * jump past first word since that is a size fixnum
			 * and second word since that is property word
			 */
#ifdef DEBUG
			if (debugin & (DEBUG_VEC|DEBUG_GC))
				printf("vector sweeping, start at %p\n",
					point);
#endif
			bits = LBITS - 2;
			bvalue = bitmapi[bindex++] >> 2;
			point += 2;
			while (TRUE) {
				type_len = point[VSizeOff];
#ifdef DEBUG
				if (debugin & (DEBUG_VEC|DEBUG_GC)) {
					printf("point: %p, type_len %jd\n",
					    point, (intmax_t)type_len);
					printf("bvalue: 0x%jx, bits: %jd,"
					    " bindex: 0x%jx\n",
					    (intmax_t)bvalue, (intmax_t)bits,
					    (intmax_t)bindex);
				}
#endif
				/* get size of vector */
				if (!(bvalue & 1)) {	/* if free */
#ifdef DEBUG
					if (debugin & (DEBUG_VEC|DEBUG_GC))
						printf("free\n");
#endif
					freecnt += type_len + 2 * LWORDS;
					if (canjoin) {
						/*
						 * join by adjusting size of
						 * first vector
						 */
						((lispval) (s->next_free))->vl.vectorl[-1]
							+= type_len + 2 * LWORDS;
#ifdef DEBUG
						if (debugin & (DEBUG_VEC|DEBUG_GC))
							printf("joined size: %jd\n",
								(intmax_t)((lispval) (s->next_free))->vl.vectorl[-1]);
#endif
					} else {
						/*
						 * vectors are linked at the
						 * property word
						 */
						*(point - 1) = (intptr_t) (s->next_free);
						s->next_free = (char *) (point - 1);
					}
					canjoin = TRUE;
				} else {
					canjoin = FALSE;
					usedcnt += type_len + 2 * LWORDS;
				}

				point += VecTotSize(type_len);
				/*
				 * we stop sweeping only when we reach a page
				 * boundary since vectors can span pages
				 */
				if (((intptr_t) point & (LBPG - 1)) == 0) {
					/*
					 * reset the counters, we cannot
					 * predict how many pages we have
					 * crossed over
					 */
					bindex = ATOLX(point);
					/*
					 * these will be inced, so we must
					 * dec
					 */
					pindex = &purepage[ATOX(point)] - 1;
					start = point - PAGEWORDS;
#ifdef DEBUG
					if (debugin & (DEBUG_VEC|DEBUG_GC))
						printf(
							"out of vector sweep when point = %p\n",
							point);
#endif
					break;
				}
				/*
				 * must advance to next point and next value
				 * in bitmap. we add VecTotSize(type_len) + 2
				 * to get us to the 0th entry in the next
				 * vector (beyond the size fixnum)
				 */
				point += 2;	/* point to next 0th entry */
				if ((bits -= (VecTotSize(type_len) + 2)) > 0)
					bvalue = bvalue >> (VecTotSize(type_len) + 2);
				else {
					bits = -bits;	/* must advance to next
							 * word in map */
					bindex += bits / (LMASK + 1);	/* this is tricky
								 * stuff... */
					bits = bits % (LMASK + 1);
					bvalue = bitmapi[bindex++] >> bits;
					bits = (LMASK + 1) - bits;
				}
			}
		} else {

			/* general sweeper, will work for all types */
			/* XXX: except vector types! */
			itemstogo = s->space;	/* number of items per page  */
			bits = LBITS;		/* number of bits per word */
			type_len = s->type_len;

#ifdef DEBUG
			if (debugin & DEBUG_GC)
				printf(" s %d, itemstogo %jd, len %jd\n",
				    s->type, (intmax_t)itemstogo,
				    (intmax_t)type_len);
#endif
			bvalue = bitmapi[bindex++];

			while (TRUE) {
				if (!(bvalue & 1)) {	/* if data element is
							 * not marked */
					freecnt++;
#ifdef DEBUG
					if (debugin & DEBUG_BITS)
						printf(" s %d, %jx free %p next = %p\n",
						    s->type, (intmax_t)bvalue, point, s->next_free);
#endif
					*point = (intptr_t) (s->next_free);
					s->next_free = (char *) point;
#ifdef DEBUG
					if (debugin & DEBUG_BITS)
						printf(" x %d, free %p next = %p\n",
						    s->type, point, s->next_free);
#endif
				} else
					usedcnt++;

				if (--itemstogo <= 0) {
					if (type_len < LBITS * 2)
						break;
					bindex++;
					if (debugin & DEBUG_GC)
						printf("increment1 %jd\n",
						    (intmax_t)type_len);
					if (type_len < LBITS * 4)
						break;
					bindex += 2;
					if (debugin & DEBUG_GC)
						printf("increment2 %jd\n",
						    (intmax_t)type_len);
					break;
				}
				point += type_len;
				/*
				 * shift over mask by number of words in data
				 * type
				 */

				if ((bits -= type_len) > 0) {
					bvalue = bvalue >> type_len;
				} else if (bits == 0) {
					bvalue = bitmapi[bindex++];
					bits = LBITS;
				} else {
					bits = -bits;
					while (bits >= LBITS) {
						bindex++;
						bits -= LBITS;
					}
					bvalue = bitmapi[bindex++];
					bvalue = bvalue >> bits;
					bits = LBITS - bits;
				}
			}
		}

		s->space_left += freecnt;
		(*(s->items))->i += usedcnt;
	}

#ifdef METER
	vtimes(&alldone, 0);
	if (gcstat)
		gcdump();
#endif
	pagenorm();
}

/*
 * alloc
 *
 *  This routine tries to allocate one or more pages of the space named
 *  by the first argument.   Returns the number of pages actually allocated.
 *
 */

lispval
alloc(lispval tname, intptr_t npages)
{
	intptr_t        ii, jj;
	struct types   *typeptr;

	ii = typenum(tname);
	typeptr = spaces[ii];
	if (npages <= 0)
		return (inewint(npages));

	if ((ATOX(datalim)) + npages > TTSIZE)
		error("Space request would exceed maximum memory allocation", FALSE);
	if ((ii == VECTOR) || (ii == VECTORI)) {
		/* allocate in one big chunk */
		tname = csegment((intptr_t) ii, (intptr_t) npages * PAGEWORDS, 0);
		tname->vl.vectorl[0] = (npages * LBPG - 2 * sizeof(intptr_t));
		tname->v.vector[1] = (lispval) typeptr->next_free;
		typeptr->next_free = (char *) &(tname->v.vector[1]);
#ifdef DEBUG
		if (debugin & DEBUG_VEC)
			printf("alloced %jd vec pages\n", (intmax_t)npages);
#endif
		return (inewint(npages));
	}
	for (jj = 0; jj < npages; ++jj)
		if (get_more_space(spaces[ii], FALSE))
			break;
	return (inewint(jj));
}

/*
 * csegment(typecode,nitems,useholeflag)
 *  allocate nitems of type typecode.  If useholeflag is true, then
 * allocate in the hole if there is room.  This routine doesn't look
 * in the free lists, it always allocates space.
 */
lispval
csegment(int typecode, int nitems, int useholeflag)
{
	int            ii, jj;
	char           *charadd;

	ii = typecode;

	if (ii != OTHER)
		nitems *= LWORDS * spaces[ii]->type_len;
	nitems = roundup(nitems, LBPG);	/* round up to right length  */
	if (debugin & DEBUG_VEC)
		printf("nitems = %d typecode=%d\n", nitems, typecode);
	charadd = sbrk(nitems);
	if (charadd == (void *) -1)
		error("NOT ENOUGH SPACE FOR ARRAY", FALSE);
	datalim = (lispval) (charadd + nitems);
	 /* if(ii!=OTHER) */ (*spaces[ii]->pages)->i += nitems / LBPG;
	if (ATOX(datalim) > fakettsize) {
		datalim = (lispval) (intptr_t)(fakettsize << LSHIFT);
		if (fakettsize >= TTSIZE) {
			printf("There isn't room enough to continue, goodbye\n");
			franzexit(1);
		}
		fakettsize++;
		badmem(53);
	}
	if (debugin & DEBUG_VEC)
	    printf("allocating %d bytes (%d pages) at %p for type %d\n",
		nitems, nitems >> LSHIFT, charadd, ii);
	for (jj = 0; jj < nitems; jj += LBPG) {
		SETTYPE(charadd + jj, ii, 30);
	}
	memset(charadd, 0, nitems);
	return ((lispval) charadd);
}

int 
csizeof(lispval         tname)
{
	return (spaces[typenum(tname)]->type_len * 4);
}

int 
typenum(lispval         tname)
{
	int             ii;

chek:	for (ii = 0; ii < NUMSPACES; ++ii)
		if (spaces[ii] && tname == *(spaces[ii]->type_name))
			break;
	if (ii == NUMSPACES) {
		tname = error("BAD TYPE NAME", TRUE);
		goto chek;
	}
	return (ii);

}

char *
gethspace(int segsiz, int type)
{
	char           *value;

	value = (ysbrk(segsiz / LBPG, type));
	datalim = (lispval) (value + segsiz);
	return (value);
}

void 
gcrebear(void)
{
}

/** markit(p) ***********************************************************/
/* just calls markdp							 */

void
markit(lispval *p)
{
	MARKDP(*p);
}

/*
 * markdp(p)
 *
 *  markdp is the routine which marks each data item.  If it is a
 *  dotted pair, the car and cdr are marked also.
 *  An iterative method is used to mark list structure, to avoid
 *  excessive recursion.
 */
void
markdp(lispval p)
{
#ifdef METER
	mrkdpcnt++;
#endif
ptr_loop:
	if (((intptr_t) p) <= ((intptr_t) nil))
		return;		/* do not mark special data types or nil=0  */


	switch (TYPE(p)) {
	case ATOM:
		ftstbit(p);
		MARKVAL(p->a.clb);
		MARKVAL(p->a.plist);
		MARKVAL(p->a.fnbnd);
#ifdef GCSTRINGS
		if (gcstrings)
			MARKVAL(((lispval) p->a.pname));
		return;

	case STRNG:
		p = (lispval) (((intptr_t) p) & ~(LBPG - 1));
		ftstbit(p);
#endif
		return;

	case INT:
	case DOUB:
		ftstbit(p);
		return;
	case VALUE:
		ftstbit(p);
		p = p->l;
		goto ptr_loop;
	case DTPR:
		ftstbit(p);
		MARKVAL(p->d.car);
#ifdef METER
		/*
		 * if we are metering , then check if the cdr is nil, or if
		 * the cdr is on the same page, and if it isn't one of those,
		 * then it is on a different page
		 */
		if (gcstat) {
			if (p->d.cdr == nil)
				consnil++;
			else if (((intptr_t) p & ~511)
				 == (((intptr_t) (p->d.cdr)) & ~511))
				conssame++;
			else
				consdiff++;
		}
#endif
		p = p->d.cdr;
		goto ptr_loop;

	case ARRAY:
		ftstbit(p);	/* mark array itself */

		MARKVAL(p->ar.accfun);	/* mark access function */
		MARKVAL(p->ar.aux);	/* mark aux data */
		MARKVAL(p->ar.length);	/* mark length */
		MARKVAL(p->ar.delta);	/* mark delta */
		if (TYPE(p->ar.aux) == DTPR && p->ar.aux->d.car == Vnogbar) {
			/*
			 * a non garbage collected array must have its array
			 * space marked but the value of the array space is
			 * not marked
			 */
			int             l;
			int             cnt, d;
#ifdef DEBUG
			if (debugin & DEBUG_ARR) {
				printf("mark array holders len %jd, del %jd, start %p\n",
				(intmax_t)p->ar.length->i, (intmax_t)p->ar.delta->i, p->ar.data);
			}
#endif
			l = p->ar.length->i;	/* number of elements */
			d = p->ar.delta->i;	/* bytes per element  */
			p = (lispval) p->ar.data;	/* address of first one */
			if (purepage[ATOX(p)])
				return;

			for ((cnt = 0); cnt < l;
			     p = (lispval) (((char *) p) + d), cnt++) {
				setbit(p);
			}
		} else {
			int             i, l, d;
			char           *dataptr = p->ar.data;

			for (i = 0, l = p->ar.length->i, d = p->ar.delta->i; i < l; ++i) {
				MARKDP((lispval) dataptr);
				dataptr += d;
			}
		}
		return;
	case SDOT:
		do {
			ftstbit(p);
			p = p->s.CDR;
		} while (p != 0);
		return;

	case BCD:
		ftstbit(p);
		MARKDP(p->bcd.discipline);
		return;

	case HUNK2:
	case HUNK4:
	case HUNK8:
	case HUNK16:
	case HUNK32:
	case HUNK64:
	case HUNK128:
		{
			int hsize, hcntr;
			hsize = 2 << HUNKSIZE(p);
			ftstbit(p);
			for (hcntr = 0; hcntr < hsize; hcntr++)
				MARKVAL(p->h.hunk[hcntr]);
			return;
		}

	case VECTORI:
		ftstbit(p);
		MARKVAL(p->v.vector[-1]);	/* mark property */
		return;

	case VECTOR:
		{
			int             vsize;
			ftstbit(p);
			vsize = VecSize(p->vl.vectorl[VSizeOff]);
#ifdef DEBUG
			if (debugin & DEBUG_VEC)
				printf("mark vect at %p size %d\n",
					p, vsize);
#endif
			while (--vsize >= -1) {
				MARKVAL(p->v.vector[vsize]);
			};
			return;
		}
	case PORT:
#ifndef GCSTRINGS
	case STRNG:
#endif
		return;
	default:
		printf("BAD %p, type=%jd\n", p, (intmax_t)TYPE(p));
		abort();
	}
	return;
}


/*
 * xsbrk allocates space in large chunks (currently 16 pages) xsbrk(1)
 * returns a pointer to a page xsbrk(0)  returns a pointer to the next page
 * we will allocate (like sbrk(0))
 */

char *
xsbrk (int n)
{
	static char    *xx;	/* pointer to next available blank page  */
	extern int      xcycle;	/* number of blank pages available  */
	lispval         u;	/* used to compute limits of bit table  */

	if ((xcycle--) <= 0) {
		xcycle = 15;
		xx = sbrk(16 * LBPG);	/* get pages 16 at a time  */
		if (xx == (void *) -1)
			lispend("For sbrk from lisp: no space... Goodbye!");
	} else
		xx += LBPG;

	if (n == 0) {
		xcycle++;	/* don't allocate the page */
		xx -= LBPG;
		return (xx);	/* just return its address */
	}
	if ((u = (lispval) (xx + LBPG)) > datalim)
		datalim = u;
	return (xx);
}

char *
ysbrk (int pages, int type)
{
	char           *xx;	/* will point to block of storage  */
	int             i;

	xx = sbrk(pages * LBPG);
	if (xx == (void *) -1)
		error("OUT OF SPACE FOR ARRAY REQUEST", FALSE);

	datalim = (lispval) (xx + pages * LBPG);	/* compute bit table
							 * limit  */

	/* set type for pages  */

	for (i = 0; i < pages; ++i) {
		SETTYPE((xx + i * LBPG), type, 10);
	}

	return (xx);		/* return pointer to block of storage  */
}

/*
 * getatom
 * returns either an existing atom with the name specified in strbuf, or
 * if the atom does not already exist, regurgitates a new one and
 * returns it.
 */
lispval
getatom(int purep)
{
	lispval         aptr;
	char           *name, *endname;
	int             hashv;
	lispval         b;
	char            c;

	name = strbuf;
	if (*name == (char) 0377)
		return (eofa);
	hashv = hashfcn(name);
	atmlen = strlen(name) + 1;
	aptr = (lispval) hasht[hashv];
	while (aptr != CNIL)
		/* if (strcmp(name,aptr->a.pname)==0) */
		if (*name == *aptr->a.pname && strcmp(name, aptr->a.pname) == 0) {
			return (aptr);
		}
		else
			aptr = (lispval) aptr->a.hshlnk;
	aptr = (lispval) newatom(purep);	/* share pname of atoms on
						 * oblist */
	aptr->a.hshlnk = hasht[hashv];
	hasht[hashv] = (struct atom *) aptr;
	endname = name + atmlen - 2;
	if ((atmlen != 4) && (*name == 'c') && (*endname == 'r')) {
		b = newdot();
		protect(b);
		b->d.car = lambda;
		b->d.cdr = newdot();
		b = b->d.cdr;
		b->d.car = newdot();
		(b->d.car)->d.car = xatom;
		while (TRUE) {
			b->d.cdr = newdot();
			b = b->d.cdr;
			if (++name == endname) {
				b->d.car = (lispval) xatom;
				aptr->a.fnbnd = (--np)->val;
				break;
			}
			b->d.car = newdot();
			b = b->d.car;
			if ((c = *name) == 'a')
				b->d.car = cara;
			else if (c == 'd')
				b->d.car = cdra;
			else {
				--np;
				break;
			}
		}
	}
	return (aptr);
}

/*
 * inewatom is like getatom, except that you provide it a string
 * to be used as the print name.  It doesn't do the automagic
 * creation of things of the form c[ad]*r.
 */
lispval
inewatom(char *name)
{
	struct atom    *aptr;
	int             hashv;

	if (*name == (char) 0377)
		return (eofa);
	hashv = hashfcn(name);
	aptr = hasht[hashv];
	while (aptr != (struct atom *) CNIL)
		if (strcmp(name, aptr->pname) == 0) {
			return ((lispval) aptr);
		}
		else
			aptr = aptr->hshlnk;
	aptr = (struct atom *) next_one(&atom_str);
	aptr->plist = aptr->fnbnd = nil;
	aptr->clb = nil;	// XXX: Used to be CNIL?
	aptr->pname = name;
	aptr->hshlnk = hasht[hashv];
	hasht[hashv] = aptr;
	return ((lispval) aptr);
}


/* our hash function */

int 
hashfcn (char *symb)
{
	int             i;
	/* for (i=0 ; *symb ; i += i + *symb++); return(i & (HASHTOP-1)); */
	for (i = 0; *symb; i += i * 2 + *symb++);
	return (i & 077777) % HASHTOP;
}

lispval
LImemory(void)
{
	long            nextadr, pagesinuse;

	printf("Memory report. max pages = %d (0x%x) = %zu Bytes\n",
	       TTSIZE, TTSIZE, TTSIZE * LBPG);
	nextadr = (long) xsbrk(0);	/* next space to be allocated */
	pagesinuse = nextadr / LBPG;
	printf("Next allocation at addr %ld (0x%lx) = page %ld\n",
	       nextadr, nextadr, pagesinuse);
	printf("Free data pages: %ld\n", TTSIZE - pagesinuse);
	return (nil);
}

void 
myhook (void)
{
}
