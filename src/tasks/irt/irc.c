/*
*
*  miaofng@2014-5-10   initial version
*
*/

#include "ulp/sys.h"
#include "irc.h"
#include "vm.h"
#include "nvm.h"
#include "led.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"
#include "can.h"
#include "err.h"

static const can_bus_t *irc_bus = &can1;
static volatile int irc_vmcomp_pulsed;
static int irc_ecode = 0;
static char irc_busy = 0;

#if 1
#include "stm32f10x.h"

void EXTI3_IRQHandler(void)
{
	irc_vmcomp_pulsed = 1;
	EXTI->PR = EXTI_Line3;
}

static inline void _trig_set(int high) {
	irc_vmcomp_pulsed = 0;
	BitAction ba = (high) ? Bit_SET : Bit_RESET;
	GPIO_WriteBit(GPIOC, GPIO_Pin_2, ba);
}

static inline int _trig_get(void) {
	return irc_vmcomp_pulsed;
}

static inline void _le_set(int high) {
	BitAction ba = (high) ? Bit_SET : Bit_RESET;
	GPIO_WriteBit(GPIOC, GPIO_Pin_6, ba);
}

static inline int _le_get(void) {
	int level = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7);
	return level;
}

/*
TRIG	PC2	OUT
VMCOMP	PC3	IN
LE_D	PC6	OUT
LE_R	PC7	IN
*/
static inline void _irc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//vmcomp exti input
	EXTI_InitTypeDef EXTI_InitStruct;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3);
	EXTI_InitStruct.EXTI_Line = EXTI_Line3;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
#endif

void irc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	irc_bus->init(&cfg);
	_irc_init();
}

int relay_latch(void)
{
	time_t timer = time_get(1);

	_le_set(1);
	for(int i = 0; i < IRC_RLY_MS;) {
		//ok? break
		if(_le_get() > 0) {
			_le_set(0);
			return 0;
		}

		//update command response in case wait too long
		if(i > 2) {
			sys_update();
		}

		//ms timer
		if(time_left(timer) < 0) {
			timer = time_get(1);
			i ++;
		}
	}

	return -1;
}

int dmm_trig(void)
{
	time_t timer = time_get(1);

	_trig_set(1);
	for(int i = 0; i < IRC_DMM_MS;) {
		//ok? break
		if(_trig_get() > 0) {
			_trig_set(0);
			return 0;
		}

		//update command response in case wait too long
		if(i > 2) {
			sys_update();
		}

		//ms timer
		if(time_left(timer) < 0) {
			timer = time_get(1);
			i ++;
		}
	}

	return -1;
}

void irc_update(void)
{
	int ecode = 0;
	int cnt = 0, bytes, over;
	can_msg_t msg;

	if(irc_ecode) {
		return;
	}

	do {
		bytes = 8;
		ecode = vm_fetch(msg.data, &bytes);
		over = vm_opgrp_is_over();
		if(bytes > 0) {
			//send can message
			msg.dlc = (char) bytes;
			msg.id =  over ? CAN_ID_CMD : CAN_ID_DAT;
			msg.id += cnt & 0x0F;
			ecode = irc_bus->send(&msg);
			if(ecode) {
				irc_ecode = -IRT_E_CAN;
				return;
			}

			cnt ++;

			//LE operation is needed?
			if(over) {
				ecode = relay_latch();
				if(ecode) { //find the bad guys hide inside the good slots
					irc_ecode = -IRT_E_SLOT;
					return;
				}

				if(vm_opgrp_is_scan()) {
					ecode = dmm_trig();
					if(ecode) {
						irc_ecode = -IRT_E_DMM;
						return;
					}
				}
			}
		}
	} while(!over);
}

int irc_reset(void)
{
	return 0;
}

int irc_abort(void)
{
	return 0;
}

int main(void)
{
	sys_init();
	vm_init();
	printf("irc v1.0, SW: %s %s\n\r", __DATE__, __TIME__);
	while(1) {
		sys_update();
		irc_busy = 1;
		irc_update();
		irc_busy = 0;
	}
}

#include "shell/cmd.h"

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*OPC?		operation is completed?\n"
		"*ERR?		error code & info\n"
		"*RST		instrument reset\n"
		"ABORT		abort operation queue left\n"
	};

	int ecode = 0;
	if(!strcmp(argv[0], "*IDN?")) {
		printf("<Linktron Technology,IRT16X3254,MY%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*OPC?")) {
		int opc = vm_is_opc();
		opc = (irc_busy) ? 0 : opc;
		printf("<%+d\n\r", opc);
		return 0;
	}
	else if(!strcmp(argv[0], "*ERR?")) {
		printf("<%+d\n\r", irc_ecode);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		ecode = irc_reset();
	}
	else if(!strcmp(argv[0], "ABORT")) {
		ecode = irc_abort();
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		ecode = -IRT_E_CMD_FORMAT;
	}

	err_print(ecode);
	return 0;
}

static int cmd_mode_func(int argc, char *argv[])
{
	return 0;
}
const cmd_t cmd_mode = {"MODE", cmd_mode_func, "change irc work mode"};
DECLARE_SHELL_CMD(cmd_mode)
