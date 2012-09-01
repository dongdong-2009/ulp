/*
 * 	miaofng@2012 initial version
 *	- based on malloc/free
 *	- fast distribution & recyling
 *	- avoid memory fragmentation
 *
 */

#include "ulp/sys.h"
#include "common/circbuf.h"
#include "common/bitops.h"
#include "common/mempool.h"
#include <string.h>

mempool_t *mempool_create(int sz, int nr)
{
	int hsz, bsz, usz, ofs;
	mempool_t *pool;
	unsigned char index;

	assert((nr >= 0) && (nr <= 255) && (sz >= 4)); /*nr > 255 is not supported*/
	hsz = sizeof(mempool_t);
	bsz = sys_align(nr, 8) >> 3;
	ofs = sys_align(hsz + bsz + nr, 4);
	usz = sys_align(sz, 4);

	pool = sys_malloc(ofs + nr * usz);
	assert(pool != NULL);
	pool->sz = usz;
	pool->nr = nr;
	pool->bits = (char *) pool + hsz;
	pool->pmem = (char *) pool + ofs;
	pool->fu.data = (char *) pool->bits + bsz;
	pool->fu.totalsize = nr;
	buf_init(&pool->fu, 0);

	memset(pool->bits, 0, bsz);
	for(index = 0; index < nr; index ++) {
		buf_push(&pool->fu, &index, 1);
	}
	return pool;
}

void mempool_destroy(mempool_t *pool)
{
	free(pool);
}

void *mempool_alloc(mempool_t *pool)
{
	unsigned char index;
	unsigned addr = (unsigned) pool->pmem;
	if(buf_size(&pool->fu) == 0) /*there is no free unit*/
		return NULL;

	buf_pop(&pool->fu, &index, 1);
	assert((index < pool->nr) && (bit_get(index, pool->bits) == 0));
	bit_set(index, pool->bits);
	addr += pool->sz * index;
	return (void *) addr;
}

void mempool_free(mempool_t *pool, void *unit)
{
	unsigned char index;
	unsigned offset, addr = (unsigned) pool->pmem;
	offset = (unsigned) unit - addr;
	assert(offset % pool->sz == 0);
	offset /= pool->sz;
	assert((offset < pool->nr) && (bit_get(offset, pool->bits) == 1));
	bit_clr(offset, pool->bits);
	index = (unsigned char) offset;
	buf_push(&pool->fu, &index, 1);
}
