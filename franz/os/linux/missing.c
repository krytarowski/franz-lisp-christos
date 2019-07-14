#include <stdio.h>
#include <string.h>

FILE *funopen(void) {
    return NULL;
}

int _fwalk(void) {
	return -1;
}

void fpurge(FILE *fp) {
    fflush(fp);
}

int nlist(void) {
	return -1;
}

size_t strlcpy(char *dst, const char *src, size_t len)
{
	strncpy(dst, src, len);
	dst[len - 1] = '\0';
	return strlen(dst);
}

