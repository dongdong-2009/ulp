/*
 * 	miaofng@2010 initial version
 *
 */
#ifndef __GLIB_H_
#define __GLIB_H_

#include "config.h"

#define GLIB_DATA_TYPE short

typedef struct {
	GLIB_DATA_TYPE x;
	GLIB_DATA_TYPE y;
} dot_t;

typedef struct { //note: start point must be located on left top
	GLIB_DATA_TYPE x1;
	GLIB_DATA_TYPE y1;
	GLIB_DATA_TYPE x2;
	GLIB_DATA_TYPE y2;
} rect_t;

//rect ops
static inline rect_t *rect_set(rect_t *dest, int x, int y, int w, int h)
{
	dest -> x1 = (GLIB_DATA_TYPE) x;
	dest -> y1 = (GLIB_DATA_TYPE) y;
	dest -> x2 = (GLIB_DATA_TYPE)(x + w);
	dest -> y2 = (GLIB_DATA_TYPE)(y + h);
	return dest;
}

static inline rect_t *rect_copy(rect_t *dest, const rect_t *src)
{
	dest -> x1 = src -> x1;
	dest -> y1 = src -> y1;
	dest -> x2 = src -> x2;
	dest -> y2 = src -> y2;
	return dest;
}

static inline rect_t *rect_zero(rect_t *dest)
{
	return rect_set(dest, 0, 0, 0, 0);
}

static inline void rect_get(const rect_t *r, int *x, int *y, int *w, int *h)
{
	*x = r -> x1;
	*y = r -> y1;
	*w = r -> x2 - r -> x1;
	*h = r -> y2 - r -> y1;
}

static inline int rect_zoom(rect_t *r, int xfactor, int yfactor)
{
	r -> x1 *= xfactor;
	r -> x2 *= xfactor;
	r -> y1 *= yfactor;
	r -> y2 *= yfactor;
	return 0;
}

static inline int rect_have(const rect_t *r, const dot_t *p)
{
	return ( \
		(p -> x >= r -> x1) && \
		(p -> x < r -> x2) && \
		(p -> y >= r -> y1) && \
		(p -> y < r -> y2) \
	);
}

static inline int rect_have_rect(const rect_t *dest, const rect_t *src)
{
	dot_t p1 = {src -> x1, src -> y1};
	dot_t p2 = {src -> x2 - 1, src -> y2 - 1};
	
	return rect_have(dest, &p1) && \
		rect_have(dest, &p2);
}

static inline rect_t *rect_move(rect_t *r, int xofs, int yofs)
{
	r -> x1 += xofs;
	r -> x2 += xofs;
	r -> y1 += yofs;
	r -> y2 += yofs;
	return r;
}

static inline rect_t *rect_join(rect_t *dest, const rect_t *src)
{
	dest -> x1 = (dest -> x1 > src -> x1) ? dest -> x1 : src -> x1;
	dest -> y1 = (dest -> y1 > src -> y1) ? dest -> y1 : src -> y1;
	dest -> x2 = (dest -> x2 < src -> x1) ? dest -> x2 : src -> x2;
	dest -> y2 = (dest -> y2 < src -> y1) ? dest -> y2 : src -> y2;
	return dest;
}

static inline rect_t *rect_merge(rect_t *dest, const rect_t *src)
{
	int w, h;
	w = dest -> x2 - dest -> x1;
	h = dest -> y2 - dest -> y1;
	
	if(!w || !h) { //copy
		dest -> x1 = src -> x1;
		dest -> y1 = src -> y1;
		dest -> x2 = src -> x2;
		dest -> y2 = src -> y2;
	}
	else {
		dest -> x1 = (dest -> x1 < src -> x1) ? dest -> x1 : src -> x1;
		dest -> y1 = (dest -> y1 < src -> y1) ? dest -> y1 : src -> y1;
		dest -> x2 = (dest -> x2 > src -> x1) ? dest -> x2 : src -> x2;
		dest -> y2 = (dest -> y2 > src -> y1) ? dest -> y2 : src -> y2;
	}
	
	return dest;
}

#endif /*__GLIB_H_*/
