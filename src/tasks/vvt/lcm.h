/*
 * 	miaofng@2011 initial version
 */
#ifndef __VVT_LCM_H_
#define __VVT_LCM_H_

typedef struct {
	short rpm_min; //round per minute
	short rpm_max;
	short cam_min; //cam advance angle
	short cam_max;
	short wss_min; //wheel speed
	short wss_max;
	short vss_min; //vehicle speed
	short vss_max;
	short mfr_min; //misfire strength
	short mfr_max;
	short knk_min; //knock strength
	short knk_max;
} lcm_cfg_t;

typedef struct {
	short rpm;
	short cam;
	short wss;
	short vss;
	short mfr;
	short knk;
	short dio; //digtal switch input
} lcm_dat_t;

#endif /*__VVT_LCM_H_*/

