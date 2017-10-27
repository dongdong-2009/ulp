/*
 * 	miaofng@2017-6-11 initial version
 */
#ifndef __AK09915_H_
#define __AK09915_H_

#include "spi.h"

/*Time for measurement
- single mode: 4.5mS@SDR=0
- : 8.5mS@SDR=1
*/
#define AK09915_TSM 10 //unit: mS
#define AK09915_MHZ 1 //max: 4MHz

#define AK09915_CID 0x48
#define AK09915_DID 0x10

enum {
	/*WIA = Who I Am*/
	AK_WIA1 = 0x00, //Company ID = 0x48
	AK_WIA2, //Device ID = 0x10
	AK_WSV,
	AK_INFO, //= 0x02

	AK_ST1 = 0x10, //Status 1
	AK_HXL, //Measurement Magnetic X Data
	AK_HXH, //Measurement Magnetic X Data
	AK_HYL, //Measurement Magnetic Y Data
	AK_HYH, //Measurement Magnetic Y Data
	AK_HZL, //Measurement Magnetic Z Data
	AK_HZH, //Measurement Magnetic Z Data
	AK_TMPS,
	AK_ST2, //Status 2

	AK_CNTL1 = 0x30, //Control 1
	AK_CNTL2,
	AK_CNTL3,
	AK_TS1, //Test
	AK_TS2,
	AK_TS3,
	AK_I2CDIS, //I2C Disable
	AK_TS4,
};

typedef struct {
	const spi_bus_t *bus;
	int cs_pin;
} ak09915_t;

#endif /*__AK09915_H_*/
