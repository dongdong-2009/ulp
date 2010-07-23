/*
 * 	miaofng@2010 initial version
 */
#ifndef __HVP_H_
#define __HVP_H_

enum {
	HVP_STM_IDLE,
	HVP_STM_START,
	HVP_STM_ERROR,
};

/*specific model program algo*/
typedef struct {
	const char *name; /*such as: mt2x*/
	int (*init)(void); /*read config file, match return 0*/
	int (*update)(void); /*main flash program state machine*/
} pa_t;

#endif
