#include <sys/cdefs.h>
#ifndef lint
__RCSID("$Id: callg-test.c,v 1.1 2014/12/11 17:13:26 christos Exp $");
#endif
#include <stdio.h>
#include <stdlib.h>

#define ASSERT(a, b) \
	if ((a) != (b)) { \
		printf("%ld != %ld\n", (a), (b)); \
		abort(); \
	} else 

long 
f(long n, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15)
{
	switch (n - 1) {
	case 15:
		ASSERT(a15, 15);
	case 14:
		ASSERT(a14, 14);
	case 13:
		ASSERT(a13, 13);
	case 12:
		ASSERT(a12, 12);
	case 11:
		ASSERT(a11, 11);
	case 10:
		ASSERT(a10, 10);
	case 9:
		ASSERT(a9, 9);
	case 8:
		ASSERT(a8, 8);
	case 7:
		ASSERT(a7, 7);
	case 6:
		ASSERT(a6, 6);
	case 5:
		ASSERT(a5, 5);
	case 4:
		ASSERT(a4, 4);
	case 3:
		ASSERT(a3, 3);
	case 2:
		ASSERT(a2, 2);
	case 1:
		ASSERT(a1, 1);
	case 0:
		break;
	}
	return 13337;
}

int
main(void)
{
	long             args[] = {
		0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
	int i;

	for (i = 1; i < 15 ; i++) {
		args[0] = args[1] = i;
		printf(">%d\n", i);
		ASSERT(callg((long (*)(void))f, args), 13337);
		printf("<%d\n", i);
	}
	return 0;
}
