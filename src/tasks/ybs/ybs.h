/*
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */

#ifndef __YBS_H_
#define __YBS_H_

#define YBS_US 50 //unit: us
#define YBS_VO_MIN 2 //unit: V
#define YBS_VO_MAX 20 //unit: V

#define d2uv(d) (d)
#define uv2d(uv) ((int)(uv))

#define d2mv(d) (d2uv(d)/1000)
#define mv2d(mv) (uv2d(mv*1000))

#define d2v(d) (d2mv(d)/1000)
#define v2d(v) (mv2d(v*1000))

void ybs_mdelay(int ms);
void ybs_update(void);

//digital filter
struct filter_s {
	int b0, b1, bn; //num
	int a0, a1, an; //den
	int xn_1, yn_1;
};

int filt(struct filter_s *f, int xn);
#endif
