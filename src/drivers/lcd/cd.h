/*
 * File Name : cd.h
 * Author    : dusk
 */

#ifndef __CD_H
#define __CD_H

#include "stm32f10x.h"
#include "time.h"
#include "uart.h"

enum {
	MODE_IL0 = 0x30,
	MODE_IL1,
	MODE_IL2,
	MODE_IL3,
	MODE_IL4
};

typedef struct {
	uart_bus_t *bus;
	int reserve;
} cd_t;

void cd_Init(cd_t *dev);
int cd_Clr(cd_t *dev);
int cd_Send(cd_t *dev, unsigned char *data, int length);
int cd_SetIndicationLight(cd_t *dev, int mode);
int cd_SetBaud(cd_t *dev, int baud);

#endif /* __CD_H */
