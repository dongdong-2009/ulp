/*
 * david@2012 initial version
 * note: there are two clock reference
 * 1, axle_clock_cnt, its' range is (0, 120 * 16)
 * 2, op_clock_cnt,   its' range is (0, 37 * 52)
 * and both range of them is approximate equal
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f10x.h"
#include "shell/cmd.h"
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "priv/mcamos.h"
#include "ad9833.h"
#include "mcp41x.h"
#include "sim_driver.h"
#include "nvm.h"
#include "wave.h"

// define the time clock reference
static int axle_clock_cnt = 0;  //for axle gear
static int op_clock_cnt = 0;    //for oil pump gear
static int op_tri_flag = 0;

//for engine axle gear singal
static int axle_cnt, op_cnt, phase_diff;
static int prev_axle_cnt, prev_op_cnt;

void weifu_Init(void)
{
	clock_Init();
	axle_Init(0);
	op_Init(0);

	phase_diff = 2;
}

void weifu_Update(void)
{
	int temp, result = 0;

	/**********  for axle and oil pump signal output  *******/
	//calculate the axle gear output
	temp = axle_clock_cnt % 16;
	axle_cnt = (axle_clock_cnt >> 4) % 120;

	//used to tri op clock cnt to zero
	if (!op_tri_flag) {
		if(phase_diff < 0) {  //advanced
			if (axle_cnt == (120 + phase_diff)) {
				op_clock_cnt = 0;
				op_tri_flag = 1;
			}
		} else {         //delayed
			if (axle_cnt == phase_diff) {
				op_clock_cnt = 0;
				op_tri_flag = 1;
			}
		}
	}

	result = IS_IN_RANGE(axle_cnt, 29, 29);
	result |= IS_IN_RANGE(axle_cnt, 59, 59);
	result |= IS_IN_RANGE(axle_cnt, 89, 89);
	result |= IS_IN_RANGE(axle_cnt, 119, 119);
	if (!result) {
		if (prev_axle_cnt != temp) {
			axle_SetAmp(gear16[temp]);
			prev_axle_cnt = temp;
		}
	} else {
		if (prev_axle_cnt != temp) {
			axle_SetAmp(gear16[0]);
			prev_axle_cnt = temp;
		}
	}

	//calculate the oil pump gear output
	temp = op_clock_cnt % 52;
	op_cnt = (op_clock_cnt/52) % 37;

	result = IS_IN_RANGE(op_cnt, 36, 36);
	if (result) {  //if hypodontia, output zero
		if (prev_op_cnt != temp) {
			op_SetAmp(gear52[0]);
			prev_op_cnt = temp;
			op_tri_flag = 0;
		}
	} else {
		if (prev_op_cnt != temp) {
			op_SetAmp(gear52[temp]);
			prev_op_cnt = temp;
		}
	}
}

void weifu_isr(void)
{
	axle_clock_cnt ++;
	op_clock_cnt ++;
}

void EXTI9_5_IRQHandler(void)
{
	weifu_isr();
	EXTI->PR = EXTI_Line6;
}

int main(void)
{
	task_Init();
	weifu_Init();
	while(1) {
		task_Update();
		weifu_Update();
	}
}

/****************************************************************
****************** static local function  ******************
****************************************************************/

static int weifu_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			task_Update();
		}
	} while(left > 0);

	return 0;
}

#if 1
static int cmd_weifu_func(int argc, char *argv[])
{
	int result = -1;

	if(!strcmp(argv[1], "axle") && (argc == 3)) {
		clock_SetFreq((short)(atoi(argv[2])));
		result = 0;
	}

	if(!strcmp(argv[1], "phase") && (argc == 3)) {
		phase_diff = atoi(argv[2]);
		result = 0;
	}

	if(!strcmp(argv[1], "start")) {
		simulator_Start();
		result = 0;
	} else if (!strcmp(argv[1], "stop")) {
		simulator_Stop();
		result = 0;
	}

	if(result == -1) {
		printf("uasge:\n");
		printf(" weifu axle  100	unit: hz/rpm\n");
		printf(" weifu phase 2		minus means advance, positive means delay\n");
		printf(" weifu start		open the 58x output\n");
		printf(" weifu stop 		shut up the 58x output\n");
	}

	return 0;
}

cmd_t cmd_weifu = {"weifu", cmd_weifu_func, "commands for weifu simulator"};
DECLARE_SHELL_CMD(cmd_weifu)

#endif
