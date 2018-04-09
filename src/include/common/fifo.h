/*
 * (C) Copyright 2017
 * miaofng, feng.ni@ulicar.com, ulicar
 * irq safe version of circbuf
 *
 */

#ifndef __FIFO_H__
#define __FIFO_H__

typedef struct {
	int *buf;
	int size;
	int wpos;
	int rpos;
} fifo_t;

int fifo_init(fifo_t *fifo, int nitems);
void fifo_dump(fifo_t *fifo);
int fifo_push(fifo_t *fifo, int data);
int fifo_push_force(fifo_t *fifo, int data);
int fifo_pop(fifo_t *fifo, int *data);
void fifo_free(fifo_t *fifo);

//return nr of data inside fifo now
static inline int fifo_size(fifo_t *fifo) {
	int wpos = fifo->wpos;
	int rpos = fifo->rpos;
	return (wpos >= rpos) ? (wpos - rpos) : (fifo->size + wpos - rpos);
}

#endif
