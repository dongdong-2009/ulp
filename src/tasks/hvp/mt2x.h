/*
 * 	miaofng@2010 initial version
 */
#ifndef __MT2X_H_
#define __MT2X_H_

typedef struct {
	int addr; /*utility dnload addr*/
	int size; /*utility dnload size*/
} mt2x_model_t;

int mt2x_Init(void);
int mt2x_Prog(void);

#endif

