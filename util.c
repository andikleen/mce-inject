#include <stdlib.h>
#include <stdio.h>
#include "util.h"

void *xalloc(size_t sz)
{
	void *p = calloc(sz, 1);
	if (!p) {
		fprintf(stderr, "Out of virtual memory\n");
		exit(1);
	}
	return p;
}

void err(const char *msg)
{
	perror(msg);
	exit(1);
}
