/*
 * File Name : ls1203.h
 * Author    : peng.guo david
 */

#ifndef __LS1203_H
#define __LS1203_H

#include "stm32f10x.h"
#include "time.h"
#include "uart.h"

typedef struct {
	uart_bus_t *bus;
	int data_len;
	time_t dead_time;
} ls1203_t;

void ls1203_Init(ls1203_t *chip);
int ls1203_Read(ls1203_t *chip, unsigned char *pdata);

#endif /* __LS1203_H */
