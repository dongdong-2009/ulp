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
	short knf_min; //knock frequency
	short knf_max;
	short knp_min; //knock position
	short knp_max;
	short knw_min; //knock width
	short knw_max;
} lcm_cfg_t;

typedef struct {
	short rpm;
	short cam; //fire advance angle
	short wss; //wheel speed
	short vss; //vehicle speed
	short mfr; //misfire strength
	short knk; //knock strength
	short knf; //knock freq
	short knp; //knock postion
	short knw; //knock width

	short dio; //digtal switch input
	short crc; //cksum
} lcm_dat_t;

enum {
	LCM_CMD_NONE,
	LCM_CMD_CONFIG, //config limit, inbox detail ref lcm_cfg_t
	LCM_CMD_READ, //para ref lcm_dat_t
	LCM_CMD_WRITE, //para ref lcm_dat_t
	LCM_CMD_SAVE,
};

#endif /*__VVT_LCM_H_*/

