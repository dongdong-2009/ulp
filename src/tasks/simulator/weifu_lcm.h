/*
 * david@2012 initial version
 */
#ifndef __WEIFU_LCM_H_
#define __WEIFU_LCM_H_

typedef struct {
	short rpm_min; //round per minute
	short rpm_max;
	short phase_diff_min;  //the phase diff of eng and op
	short phase_diff_max;
	short vss_min; //vehicle speed
	short vss_max;
	short tim_min; //tim sensor duty cycle
	short tim_max;
	short hfmsig_min; //air flow meter(1.5k - 15k)
	short hfmsig_max;
	short hfmref_min; //air flow meter diag(18k)
	short hfmref_max;
} lcm_cfg_t;

typedef struct {
	short eng_rpm;
	short phase_diff;
	short vss;       //vehicle speed
	short tim_dc;    //tim duty cycle
	short hfmsig;    //flow meter signal
	short hfmref;    //flow meter diag
	short eng_speed; //feedback engine speed
	short wtout;     //cooling temper
	short crc;       //cksum
} lcm_dat_t;

enum {
	LCM_CMD_NONE,
	LCM_CMD_CONFIG, //config limit, inbox detail ref lcm_cfg_t
	LCM_CMD_READ, //para ref lcm_dat_t
	LCM_CMD_WRITE, //para ref lcm_dat_t
	LCM_CMD_SAVE,
};

#endif /*__WEIFU_LCM_H_*/

