#include <stdlib.h>
#include <stdio.h>
#include "util.h"

void oom(void)
{
	fprintf(stderr, "Out of virtual memory\n");
	exit(1);
}

void *xcalloc(size_t a, size_t b)
{
	void *p = calloc(a, b);
	if (!p)
		oom();
	return p;
}

void *xalloc(size_t sz)
{
	void *p = calloc(sz, 1);
	if (!p)
		oom();
	return p;
}

void err(const char *msg)
{
	perror(msg);
	exit(1);
}
