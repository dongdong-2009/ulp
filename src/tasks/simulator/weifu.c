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
#include <stdint.h>
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
#include "weifu_lcm.h"

#define WEIFU_DEBUG 1
#if WEIFU_DEBUG
#define DEBUG_TRACE(args)    (printf args)
#else
#define DEBUG_TRACE(args)
#endif

//define sample points for axle and op
#define SAMPLE_DENSITY_HIGH  1
#if SAMPLE_DENSITY_HIGH
#define AXLE_SAMPLE_POINTS  32
#define OP_SAMPLE_POINTS    104
#else
#define AXLE_SAMPLE_POINTS  16
#define OP_SAMPLE_POINTS    52
#endif

//define two wave output support
#define WEIFU_SUPPORT_TWO_WAVE  1
enum {
	OP_WAVE_MODE_37X,
	OP_WAVE_MODE_120X,
};
static int op_mode = OP_WAVE_MODE_37X;
//op and eng rpm wave cofficient define
// (0 - 500)=0.3, (500 - 1000) = 0.5
// (1000 - 1500) = 0.7, (1500 - 5000) = 1
#define WAVE_FACTOR_3 3
#define WAVE_FACTOR_5 5
#define WAVE_FACTOR_7 7
#define WAVE_FACTOR_10 10

// define the time clock reference
static int axle_clock_cnt = 0;  //for axle gear
static int op_clock_cnt = 0;    //for oil pump gear
static int op_tri_flag = 0;

//for engine axle gear singal
static int axle_cnt, op_cnt;
static int prev_axle_cnt, prev_op_cnt;
static volatile int eng_factor, op_factor;
static volatile int eng_rpm, op_rpm, phase_diff, vss, hfmsig, hfmref;
static int tim_dc, tim_frq;

//timer define for eng_speed and wtout
static time_t eng_speed_timer;
static time_t wtout_timer;

//for mcamos defines, com between lcm and core board
static struct mcamos_s lcm = {
	.can = &can1,
	.baud = 500000,
	.id_cmd = MCAMOS_MSG_CMD_ID,
	.id_dat = MCAMOS_MSG_DAT_ID,
	.timeout = 50,
};
static lcm_dat_t cfg_data;

//private functions
static int weifu_ConfigData(void);

void weifu_Init(void)
{
	clock_Init();
	axle_Init(0);
	op_Init(0);
	vss_Init();
	wss_Init();
	pwmin1_Init();
	counter2_Init();
	eng_speed_timer = time_get(1000);
	wtout_timer = time_get(1000);

	mcamos_init_ex(&lcm);
	cfg_data.eng_speed = 0;
	cfg_data.wtout = 0;
	phase_diff = 2;
	eng_factor = 10;
	op_factor = 10;
	vss = 0;
	tim_dc = 0;
	tim_frq = 0;
	hfmsig = 0;
	hfmref = 0;

	simulator_Start();
}

void weifu_Update(void)
{
	//communication update
	weifu_ConfigData();
	if (eng_rpm != cfg_data.eng_rpm) {
		#if SAMPLE_DENSITY_HIGH
		clock_SetFreq(cfg_data.eng_rpm << 5);
		#else
		clock_SetFreq(cfg_data.eng_rpm << 4);
		#endif
		eng_rpm = cfg_data.eng_rpm;
		op_rpm = eng_rpm >> 1;

		if (IS_IN_RANGE(eng_rpm, 0, 500)) {
			eng_factor = WAVE_FACTOR_3;
		} else if (IS_IN_RANGE(eng_rpm, 500, 1000)) {
			eng_factor = WAVE_FACTOR_5;
		} else if (IS_IN_RANGE(eng_rpm, 1000, 1500)) {
			eng_factor = WAVE_FACTOR_7;
		} else {
			eng_factor = WAVE_FACTOR_10;
		}

		if (IS_IN_RANGE(op_rpm, 0, 500)) {
			op_factor = WAVE_FACTOR_3;
		} else if (IS_IN_RANGE(op_rpm, 500, 1000)) {
			op_factor = WAVE_FACTOR_5;
		} else if (IS_IN_RANGE(op_rpm, 1000, 1500)) {
			op_factor = WAVE_FACTOR_7;
		} else {
			op_factor = WAVE_FACTOR_10;
		}
	}
	if (phase_diff != cfg_data.phase_diff) {
		phase_diff = cfg_data.phase_diff;
	}
	if (vss != cfg_data.vss) {        //need vss
		vss = cfg_data.vss;
		vss_SetFreq(vss);
	}
	if (tim_dc != cfg_data.tim_dc) {  //need pwm1
		tim_dc = cfg_data.tim_dc;
		tim_frq = cfg_data.tim_frq;
		pwm1_Init(tim_frq + 3, 100 - tim_dc);
	}
	if (tim_frq != cfg_data.tim_frq) {  //need pwm1
		tim_dc = cfg_data.tim_dc;
		tim_frq = cfg_data.tim_frq;
		pwm1_Init(tim_frq + 3, 100 - tim_dc);
	}
	if (hfmsig != cfg_data.hfmsig) {  //need wss
		hfmsig = cfg_data.hfmsig;
		wss_SetFreq(hfmsig);
	}
	if (hfmref != cfg_data.hfmref) {  //need pwm2
		hfmref = cfg_data.hfmref;
		pwm2_Init(hfmref, 50);
	}
	#if WEIFU_SUPPORT_TWO_WAVE
	if (op_mode != cfg_data.op_mode) {  //need pwm2
		op_mode = cfg_data.op_mode;
		axle_clock_cnt = 0;
		op_clock_cnt  = 0;
		op_tri_flag = 0;
	}
	#endif

	//for eng_speed and wtout output 
	if (time_left(wtout_timer) < 0) {
		cfg_data.wtout = 100 - pwmin1_GetDC();
		wtout_timer = time_get(250);
	}
	if (time_left(eng_speed_timer) < 0) {
		cfg_data.eng_speed = counter2_GetValue();
		counter2_SetValue(0);
		//2hz = 1 eng round
		eng_speed_timer = time_get(1000);
	}
}

void weifu_isr(void)
{
	axle_clock_cnt ++;
	op_clock_cnt ++;

	int temp, result = 0;

	/**********  for axle and oil pump signal output  *******/
	//calculate the axle gear output
	temp = axle_clock_cnt % AXLE_SAMPLE_POINTS;
#if SAMPLE_DENSITY_HIGH
	axle_cnt = (axle_clock_cnt >> 5) % 120;
#else
	axle_cnt = (axle_clock_cnt >> 4) % 120;
#endif

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

	result |= IS_IN_RANGE(axle_cnt, 58, 59);
	result |= IS_IN_RANGE(axle_cnt, 118, 119);
	#if SAMPLE_DENSITY_HIGH
		if (!result) {
			if (prev_axle_cnt != temp) {
				axle_SetAmp((gear32[temp] * eng_factor) / 10);
				// axle_SetAmp(gear32[temp]);
				prev_axle_cnt = temp;
			}
		} else {
			if (prev_axle_cnt != temp) {
				axle_SetAmp((gear32[0] * eng_factor) / 10);
				// axle_SetAmp(gear32[temp]);
				prev_axle_cnt = temp;
			}
		}
	#else
		if (!result) {
			if (prev_axle_cnt != temp) {
				axle_SetAmp((gear16[temp] * eng_factor) / 10);
				prev_axle_cnt = temp;
			}
		} else {
			if (prev_axle_cnt != temp) {
				axle_SetAmp((gear16[0] * eng_factor) / 10);
				prev_axle_cnt = temp;
			}
		}
	#endif

	if (op_mode == OP_WAVE_MODE_120X) {
		//calculate the oil pump gear output
		temp = op_clock_cnt % AXLE_SAMPLE_POINTS;
		#if SAMPLE_DENSITY_HIGH
			op_cnt = (op_clock_cnt >> 5) % 120;
		#else
			op_cnt = (op_clock_cnt >> 4) % 120;
		#endif

		result = IS_IN_RANGE(op_cnt, 28, 29);
		result |= IS_IN_RANGE(op_cnt, 58, 59);
		result |= IS_IN_RANGE(op_cnt, 88, 89);
		result |= IS_IN_RANGE(op_cnt, 118, 119);
		#if SAMPLE_DENSITY_HIGH
			if (!result) {
				if (prev_op_cnt != temp) {
					op_SetAmp(gear32[temp] >> 2);
					prev_op_cnt = temp;
				}
			} else {
				if (prev_op_cnt != temp) {
					op_SetAmp(gear32[0] >> 2);
					prev_op_cnt = temp;
				}
			}
		#else
			if (!result) {
				if (prev_op_cnt != temp) {
					op_SetAmp(gear16[temp] >> 2);
					prev_op_cnt = temp;
				}
			} else {
				if (prev_op_cnt != temp) {
					op_SetAmp(gear16[0] >> 2);
					prev_op_cnt = temp;
				}
			}
		#endif
		if (IS_IN_RANGE(op_cnt, 118, 119)) {
			op_tri_flag = 0;
		}
	}

	if (op_mode == OP_WAVE_MODE_37X) {
		//calculate the oil pump gear output
		temp = op_clock_cnt % OP_SAMPLE_POINTS;
		op_cnt = (op_clock_cnt/OP_SAMPLE_POINTS) % 37;
		result = IS_IN_RANGE(op_cnt, 36, 36);
		#if SAMPLE_DENSITY_HIGH
			if (result) {  //if hypodontia, output zero
				if (prev_op_cnt != temp) {
					op_SetAmp((gear104[temp] * op_factor) / 10);
					// op_SetAmp(gear104[temp]);
					prev_op_cnt = temp;
					op_tri_flag = 0;
				}
			} else {
				if (prev_op_cnt != temp) {
					op_SetAmp((gear104[temp] * op_factor) / 10);
					// op_SetAmp(gear104[temp]);
					prev_op_cnt = temp;
				}
			}
		#else
			if (result) {  //if hypodontia, output zero
				if (prev_op_cnt != temp) {
					op_SetAmp((gear52[temp] * op_factor) / 10);
					prev_op_cnt = temp;
					op_tri_flag = 0;
				}
			} else {
				if (prev_op_cnt != temp) {
					op_SetAmp((gear52[temp] * op_factor) / 10);
					prev_op_cnt = temp;
				}
			}
		#endif
	}
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

//how to handle the data, please see lcm mcamos server module.
static int weifu_ConfigData(void)
{
	int ret = 0;
	char lcm_cmd = LCM_CMD_READ;
	//communication with the lcm board
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, &lcm_cmd, 1);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR + 2, &cfg_data, 16);
	if (ret) {
		DEBUG_TRACE(("##MCAMOS ERROR!##\n"));
		return -1;
	}

	lcm_cmd = LCM_CMD_WRITE;
	//communication with the lcm board
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, &lcm_cmd, 1);
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR + 1, &(cfg_data.eng_speed), 4);
	if (ret) {
		DEBUG_TRACE(("##MCAMOS ERROR!##\n"));
		return -1;
	}
	return 0;
}

#if 1
static int cmd_weifu_func(int argc, char *argv[])
{
	int result = -1;

	if(!strcmp(argv[1], "axle") && (argc == 3)) {
		axle_SetFreq(atoi(argv[2]));
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
