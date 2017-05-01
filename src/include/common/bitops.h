#ifndef __BITOPS_H_
#define __BITOPS_H_

#include "config.h"
#if CONFIG_TASK_NEST
#define BIT_MASK(n)		(0x80 >> ((n) % 8))
#else
#define BIT_MASK(n)		(0x01 << ((n) % 8))
#endif
#define BIT_WORD(n)		((n) / 8)

static inline void bit_set(int n, void *addr)
{
	unsigned char mask = BIT_MASK(n);
	unsigned char *p = ((unsigned char *)addr) + BIT_WORD(n);

	*p  |= mask;
}

static inline void bit_clr(int n, void *addr)
{
	unsigned char mask = BIT_MASK(n);
	unsigned char *p = ((unsigned char *)addr) + BIT_WORD(n);

	*p &= ~mask;
}

static inline int bit_get(int n, const void *addr)
{
	unsigned char mask = BIT_MASK(n);
	unsigned char *p = ((unsigned char *)addr) + BIT_WORD(n);

	return (*p & mask) ? 1 : 0;
}

int bitcount(int n); //return how many 1 in para n

#endif
