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
static int irc_mode = IRC_MODE_L2T;
static int irc_mode_new;

static struct {
	unsigned opc : 1;
	unsigned rst : 1;
	unsigned abt : 1;
	unsigned nmb : 1; /*new mode*/
} irc_status;

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

static inline void _rly_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/*PE0~PE7*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | \
		GPIO_Pin_3 |GPIO_Pin_4 |GPIO_Pin_5 |GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

static inline void _rly_set(int x)
{
	int y = vGPIOE->ODR ;
	y &= 0xf0;
	y |= x;
	vGPIOE->ODR = y;
}
#endif

void irc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	irc_bus->init(&cfg);
	_irc_init();
	_rly_init();
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

static int irc_is_opc(void)
{
	vm_is_opc();
	opc = (irc_status.opc) ? opc : 0;
	return opc;
}

static int irc_reset(void)
{
	return 0;
}

static int irc_abort(void)
{
	return 0;
}

static int irc_mode(int mode)
{
	int ecode = 0;
	int cnt = 0, bytes, over;
	can_msg_t msg;
	irc_cfg_msg_t *cfg = (irc_cfg_msg_t *) msg.data;

	//step 1, turn off all relays
	cfg->cmd = IRC_CFG_ALLOFF_IBUS_ELNE;
	msg.id = CAN_ID_CFG;
	ecode = irc_bus->send(&msg);
	if(ecode) {
		irc_ecode = -IRT_E_CAN;
		return ecode;
	}

	ecode = relay_latch();
	if(ecode) { //find the bad guys hide inside the good slots
		irc_ecode = -IRT_E_SLOT;
		return ecode;
	}

	//step 2, set irc board itself relays
	_rly_set(mode & 0xff);

	return ecode;
}

/*implement a group of switch operation*/
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

		//reset or abort?
		if(irc_status.rst) {
			irc_reset();
			return;
		}

		if(irc_status.abt) {
			irc_abort();
			return;
		}
	} while(!over);
}

int main(void)
{
	sys_init();
	vm_init();
	printf("irc v1.0, SW: %s %s\n\r", __DATE__, __TIME__);
	while(1) {
		sys_update();
		irc_status.opc = 0;
		irc_update();
		irc_status.opc = 1;
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
		int opc = irc_is_opc();
		printf("<%+d\n\r", opc);
		return 0;
	}
	else if(!strcmp(argv[0], "*ERR?")) {
		printf("<%+d\n\r", irc_ecode);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		irc_status.rst = 1;
	}
	else if(!strcmp(argv[0], "ABORT")) {
		irc_status.abt = 1;
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
	const char *usage = {
		"usage:\n"
		"MODE <mode> <lv> <is>\n"
		"	<mode> = [HVR|L4R|W4R|L2R|L2T|RMX|VHV|VLV|IIS]\n"
	};

	int ecode = 0;
	if(!strcmp(argv[1], "MODE")) {
		int lv = (argc > 2) ? atoi(argv[2]) : 0;
		int is = (argc > 3) ? atoi(argv[3]) : 0;
		const char *name[] = {
			"HVR", "L4R", "W4R", "L2R", "L2T", "RMX", "VHV", "VLV", "IIS"
		};
		const int mode_list[] = {
			IRC_MODE_HVR, IRC_MODE_L4R,
			IRC_MODE_W4R, IRC_MODE_L2R,
			IRC_MODE_L2T, IRC_MODE_RMX,
			IRC_MODE_VHV, IRC_MODE_VLV,
			IRC_MODE_IIS
		};
		ecode = -IRT_E_CMD_FORMAT;
		for(int i = 0; i < sizeof(mode_list) / sizeof(int); i ++) {
			if(!strcmp(argv[1], name[i])) {
				int mode = mode_list[i];
				mode |= (mode & IRC_MASK_ELV) ? lv : 0;
				mode |= (mode & IRC_MASK_EIS) ? is : 0;
				ecode = -IRT_E_OP_REFUSED;
				if(irc_is_opc()) {
					irc_mode_new = mode;
					irc_status.nmd = 1;
					ecode = 0;
				}
			}
		}
	}
	else if(!strcmp(argv[1], "HELP")) {
		printf("%s", usage);
		return 0;
	}
	else {
		ecode = -IRT_E_CMD_FORMAT;
	}
	err_print(ecode);
	return 0;
}
const cmd_t cmd_mode = {"MODE", cmd_mode_func, "change irc work mode"};
DECLARE_SHELL_CMD(cmd_mode)
