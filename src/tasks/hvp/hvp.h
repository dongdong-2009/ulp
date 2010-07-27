/*
 * 	miaofng@2010 initial version
 */
#ifndef __HVP_H_
#define __HVP_H_

enum {
	HVP_CMD_PROGRAM,
};

typedef struct
{
	char cmd;
	void *para1;
	void *para2;
} hvp_msg_t;

/*specific model program algo*/
typedef struct {
	const char *name; /*such as: mt2x*/
	int (*init)(void); /*read config file, match return 0*/
	int (*update)(void); /*main flash program state machine*/
} pa_t;

int hvp_prog(char *model, char *sub);

#endif
