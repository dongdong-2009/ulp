/*
 * 	v2.0 miaofng@2013 initial version
 *	risk note:
 *	- bank switch need an settling time dueto relay factor & auto range search
 *
 *	v2.1@2013.7
 *	- remove useless register definition
 *	- auto-range function is removed(it's over-design)
 */

#ifndef __OID_DMM_H__
#define __OID_DMM_H__

/*MOHM does exists, but it's too big to measure its accurate value by dmm*/
#define DMM_DATA_UNKNOWN ((1 << 23) - 2)	//result invalid dueto dmm's hardware limitation
#define DMM_DATA_INVALID ((1 << 23) - 1)	//result invalid dueto over-range

/*value outside this range indicates system failure*/
#define DMM_MOHM_MIN 10
#define DMM_MOHM_MAX 200000
#define DMM_MOHM_OPEN DMM_DATA_INVALID

typedef union {
	struct {
		int result : 24; //test result

		unsigned mohm : 1; //result's unit = mohm
		unsigned ready : 1; //dmm new measurement data is ready
		unsigned pinA : 3;
		unsigned pinK : 3;
	};
	int value;
} dmm_data_t;

#endif
