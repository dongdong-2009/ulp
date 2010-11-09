/*
 *  tslib/src/ts_getxy.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Waits for the screen to be touched, averages x and y sample
 * coordinates until the end of contact
 *
 * miaofng@2010 revised for bldc platform
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linux/sort.h"
#include "common/bitops.h"
#include "pd.h"
#include "lcd.h"
#include "pd_linear.h"
#include "FreeRTOS.h"
#include "time.h"

extern pdl_t pd_pdl;

static int sort_by_x(const void* a, const void *b)
{
	return (((struct pd_sample *)a)->x - ((struct pd_sample *)b)->x);
}

static int sort_by_y(const void* a, const void *b)
{
	return (((struct pd_sample *)a)->y - ((struct pd_sample *)b)->y);
}

static void getxy(int *x, int *y)
{
#define MAX_SAMPLES 256
	struct pd_sample *samp = MALLOC(MAX_SAMPLES * sizeof(struct pd_sample));
	int index, i, v, middle, ret;

	/* Now collect up to MAX_SAMPLES touches into the samp array. */
	for(index = 0; index < MAX_SAMPLES;) {
		ret = pdd_get(&samp[index]);
		if(ret == 0) {
			if(samp[index].z < 2000)
				index ++;
		}
	}
	printf("Took %d samples...\n",index);

	/*
	 * At this point, we have samples in indices zero to (index-1)
	 * which means that we have (index) number of samples.  We want
	 * to calculate the median of the samples so that wild outliers
	 * don't skew the result.  First off, let's assume that arrays
	 * are one-based instead of zero-based.  If this were the case
	 * and index was odd, we would need sample number ((index+1)/2)
	 * of a sorted array; if index was even, we would need the
	 * average of sample number (index/2) and sample number
	 * ((index/2)+1).  To turn this into something useful for the
	 * real world, we just need to subtract one off of the sample
	 * numbers.  So for when index is odd, we need sample number
	 * (((index+1)/2)-1).  Due to integer division truncation, we
	 * can simplify this to just (index/2).  When index is even, we
	 * need the average of sample number ((index/2)-1) and sample
	 * number (index/2).  Calculate (index/2) now and we'll handle
	 * the even odd stuff after we sort.
	 */
	middle = index/2;
	if (x) {
		sort(samp, index, sizeof(struct pd_sample), sort_by_x, 0);
#if 0
		if (index & 1)
			*x = samp[middle].x;
		else
			*x = (samp[middle-1].x + samp[middle].x) / 2;
#else
		v = samp[middle].x;
		for( i = -8; i < 9; i ++) {
			v += samp[middle + i].x;
			v >>= 1;
		}
		*x = v;
#endif
	}
	if (y) {
		sort(samp, index, sizeof(struct pd_sample), sort_by_y, 0);
#if 0
		if (index & 1)
			*y = samp[middle].y;
		else
			*y = (samp[middle-1].y + samp[middle].y) / 2;
#else
		v = samp[middle].y;
		for( i = -8; i < 9; i ++) {
			v += samp[middle + i].y;
			v >>= 1;
		}
		*y = v;
#endif
	}
	FREE(samp);
}

static void put_cross(int x, int y)
{
	int i;
	char *p = MALLOC(128);

	memset(p, 0, 128);
	for( i = 0; i < 32; i ++ ) {
		bit_set(15 + (i << 5), p);
		bit_set(16 + (i << 5), p);
		bit_set((15 << 5) + i, p);
		bit_set((16 << 5) + i, p);
	}

	lcd_bitblt(p, x - 15, y - 15, 32, 32);
	FREE(p);
}

static void clr_cross(int x, int y)
{
	char *p = MALLOC(128);
	memset(p, 0, 128);
	lcd_bitblt(p, x - 15, y - 15, 32, 32);
	FREE(p);
}

static void get_sample (pdl_cal_t *cal, int index, int x, int y, char *name)
{
	put_cross (x, y);
	getxy (&cal->x [index], &cal->y [index]);
	clr_cross(x, y);
	mdelay(1000);

	cal->xfb [index] = x;
	cal->yfb [index] = y;
	printf ("%s : (%4d,%4d)->(%4d,%4d)\n", name, x, y, cal->x [index], cal->y [index]);
}

static int verify(int xres, int yres)
{
	int x, y, ret, event = PDE_NONE;
	dot_t p;

	//mid point
	xres >>= 1;
	yres >>= 1;
	yres -= 32;
	
	//output red&green cross
	lcd_set_color(RED, COLOR_BG_DEF);
	put_cross(xres-32, yres);
	lcd_set_color(GREEN, COLOR_BG_DEF);
	put_cross(xres+32, yres);
	lcd_set_color(COLOR_FG_DEF, COLOR_BG_DEF);
	
	x = 0;
	y = 0;
	while(1) {
		event = pd_GetEvent(&p);
		if(event == PDE_NONE)
			continue;
		
		if(x + y != 0) {
			clr_cross(x, y);
		}
		
		x = p.x;
		y = p.y;
		
		if(y > yres - 16 && y < yres + 16) {
			//red cross is pressed?
			if(x > xres - 32 - 16 && x < xres - 32 + 16) {
				ret = -1;
				break;
			}
		
			//green cross is pressed?
			if(x > xres + 32 - 16 && x < xres + 32 + 16) {
				ret = 0;
				break;
			}
		}
		
		//display cross at the touch point
		put_cross(x, y);
		printf("pd test: %d %d\n", x, y);
	}
	
	return ret;
}

int pd_Calibration(void)
{
	int xres, yres, ret, i;
	pdl_cal_t cal;
	lcd_prop_t prop;

	lcd_get_prop(&prop);
#ifdef CONFIG_FONT_TNR08X16
	xres = prop.w << 3;
	yres = prop.h << 4;
#elif CONFIG_FONT_TNR16X32
	xres = prop.w << 4;
	yres = prop.h << 5;
#else
	xres = prop.w;
	yres = prop.h;
#endif
	printf("xres = %d, yres = %d\n", xres, yres);
	lcd_clear_all();
	get_sample(&cal, 0, 50, 50, "top left");
	get_sample(&cal, 1, xres - 50, 50, "top right");
	get_sample(&cal, 2, xres - 50, yres - 50, "bot right");
	get_sample(&cal, 3, 50, yres - 50, "bot left");
	get_sample(&cal, 4, xres >> 1,  yres >> 1,  "center");
	pdl_cal(&cal, &pd_pdl);

	//visual verify calibration result
	ret = verify(xres, yres);
	lcd_clear_all();
	return ret;
}
