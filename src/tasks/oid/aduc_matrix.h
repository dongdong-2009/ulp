/*
 * 	miaofng@2013-7-6 initial version
 */

#ifndef __ADUC_MATRIX_H_
#define __ADUC_MATRIX_H_

#define DMM_MODE1 (1 << 15) //MBI5024_OUT15, VOLTAGE MEASUREMENT, ADC0/1 or ADC4/5(OPA)
#define DMM_MODE0 (1 << 14) //MBI5024_OUT14, CURRENT SOURCE 0FF/ON

#define RELAY_V_MODE() do { \
	matrix_relay(DMM_MODE0 & 0xFFFF, DMM_MODE0); \
	matrix_relay(DMM_MODE1 & 0x0000, DMM_MODE1); \
} while(0)

#define RELAY_R_MODE() do { \
	matrix_relay(DMM_MODE0 & 0x0000, DMM_MODE0); \
	matrix_relay(DMM_MODE1 & 0xFFFF, DMM_MODE1); \
} while(0)

void matrix_init(void);
void matrix_relay(int image, int mask);
void matrix_pick(int pin0, int pin1);

#endif
