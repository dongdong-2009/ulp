#ifndef _LINUX_SORT_H
#define _LINUX_SORT_H

#include <stddef.h>

void sort(void *base, size_t num, size_t size,
	  int (*cmp)(const void *, const void *),
	  void (*swap)(void *, void *, int));

#endif
