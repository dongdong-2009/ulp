/*  encoder.h
 * 	dusk@2011 initial version
 */
#ifndef __ENCODER_H_
#define __ENCODER_H_

typedef struct {
	int max_value;	//do not exceed 65535
} encoder_cfg_t;

typedef struct {
	void (* init)(const encoder_cfg_t *cfg);
	void (* setCounter)(int counter);
	int (* getCounter)(void);
	void (* setMaxValue)(int value);
} encoder_t;

extern const encoder_t encoder1;
extern const encoder_t encoder2;
extern const encoder_t encoder3;
extern const encoder_t encoder4;

#endif /*__ENCODER_H_*/
