/*
 * miaofng@2012 initial version
 *
 */

#ifndef __MEMPOOL_H_
#define __MEMPOOL_H_

typedef struct mempool_s {
	int sz; /*unit size, after alignment*/
	int nr; /*nr of units*/
	circbuf_t fu; /*free units*/
	void *bits; /*total nr bits, 1 -> busy*/
	void *pmem;
} mempool_t;

mempool_t *mempool_create(int sz, int nr);
void mempool_destroy(mempool_t *pool);
void *mempool_alloc(mempool_t *pool);
void mempool_free(mempool_t *pool, void *unit);

#endif
