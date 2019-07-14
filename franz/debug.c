#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: debug.c,v 1.5 2014/12/05 17:27:37 christos Exp $");
#endif
#define EXPOSE_TYPES
#include "global.h"

const char *
typename(int type)
{
	static char es[128];
	type++;
	if (type < 0 || type >= __arraycount(tstr)) {
		snprintf(es, sizeof(es), "unknown %d\n", type - 1);
		return es;
	}
	return tstr[type];
}

void
dump(const char *prompt, lispval v, int prefix)
{
	printf("%s%*.*s[%p]", prompt, prefix, prefix,
	       "                                  ", v);
	if (v < (lispval) 0x1000) {
		printf("BAD\n");
		return;
	}
	switch (TYPE(v)) {
	case STRNG:
		if (v->str < (char *) 0x1000)
			printf("STRNG: %p, *BAD*\n", v->str);
		else
			printf("STRNG: %p, %s\n", v->str, v->str);
		return;
	case ATOM:
		printf("ATOM: %s\n", v->a.pname);
		return;
	case INT:
		printf("INT: %jd\n", (intmax_t)v->i);
		return;
	case DTPR:
		printf("DTPR CAR %p CDR %p\n", v->d.car, v->d.cdr);
		dump(prompt, v->d.car, prefix + 1);
		dump(prompt, v->d.cdr, prefix + 1);
		return;
	case DOUB:
		printf("DOUB: %lf\n", v->r);
		return;
	case BCD:
		printf("BCD %p\n", v->bcd.start);
		return;
	case PORT:
		printf("PORT %p\n", v->p);
		return;
		/*
			case ARRAY:
			case OTHER:
		*/
	case SDOT:
		printf("SDOT I %jd CDR %p\n", (intmax_t)v->s.I, v->s.CDR);
		dump(prompt, v->s.CDR, prefix + 1);
		return;
	default:
		printf("unhandled %d\n", TYPE(v));
		return;
	}
}
#if 0
#define	ARRAY	7
#define OTHER   8
#define SDOT	9
#define VALUE	10

#define HUNK2	11		/* The hunks */
#define HUNK4	12
#define HUNK8	13
#define HUNK16	14
#define HUNK32	15
#define HUNK64	16
#define HUNK128	17

#define VECTOR  18
#define VECTORI 19
#endif
