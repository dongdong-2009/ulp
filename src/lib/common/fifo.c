/*
 * (C) Copyright 2017
 * miaofng, feng.ni@ulicar.com, ulicar
 * irq safe version of circbuf
 *
 */

#include "config.h"
#include "ulp/sys.h"
#include "common/fifo.h"
#include <string.h>

int fifo_init(fifo_t *fifo, int nitems)
{
	memset(fifo, 0x00, sizeof(fifo_t));
	fifo->buf = sys_malloc(nitems * sizeof(int));
	sys_assert(fifo->buf != NULL);
	fifo->size = nitems;
	fifo->wpos = 0;
	fifo->rpos = 0;
	return 0;
}

void fifo_free(fifo_t *fifo)
{
	sys_assert(fifo->buf != NULL);
	sys_free(fifo->buf);
	fifo->buf = NULL;
}

void fifo_dump(fifo_t *fifo)
{
	fifo->wpos = 0;
	fifo->rpos = 0;
}

int fifo_push(fifo_t *fifo, int data)
{
	int overflow = 0;
	int wpos = fifo->wpos;
	wpos = (wpos + 1) % fifo->size;
	if(wpos == fifo->rpos) {
		overflow = 1;
	}

	if(!overflow) {
		fifo->buf[fifo->wpos] = data;
		fifo->wpos = wpos;
	}

	int npushed = overflow ? 0 : 1;
	return npushed;
}

//return nr of bytes is poped
int fifo_pop(fifo_t *fifo, int *data)
{
	int rpos = fifo->rpos;
	if(rpos != fifo->wpos) {
		*data = fifo->buf[rpos];
		fifo->rpos = (rpos + 1) % fifo->size;
		return 1;
	}

	return 0;
}
