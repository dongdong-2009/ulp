/*
 *
 */

#ifndef __YBS_DMM_H__
#define __YBS_DMM_H__

enum {
	DMM_CH_ASIG, //p0.0
	DMM_CH_DET, //p0.1
	DMM_CH_BURN, //p0.2
};

struct dmm_result_s {
	float value;
	unsigned ready : 1; //test result is ready or test busy
	unsigned ovld : 1; //overload
};

#endif
