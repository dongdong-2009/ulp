// #
// # this app is for burn panel power on/off test
// #
// # Written by david@2012
// #
// # Input/output configration:
// # Relay Control Pin:PC4
// # AWL Input 1 Detect Pin:PC5
// # AWL Input 2 Detect Pin:PE10

#include "config.h"
#include <stdio.h>
#include "ulp_time.h"
#include "nvm.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "stm32f10x_gpio.h"

/* init time define*/
#define BP_SELFCHECK_TIME		8000             // for product self checking time
#define BP_PowerOn_TIME			(1000 * 60 * 10) //for power on time 10 min
#define BP_PowerOff_TIME		(1000 * 10)      //for power off time 10 s

#define AWL_OFF			Bit_SET
#define AWL_ON			Bit_RESET

#define POWER_OFF		Bit_RESET
#define POWER_ON		Bit_SET

#define RYCTRL_PIN		GPIO_Pin_4
#define AWLFB1_PIN		GPIO_Pin_5
#define AWLFB2_PIN		GPIO_Pin_10

#define RY_CTRL(ba)		GPIO_WriteBit(GPIOC,RYCTRL_PIN,ba)
#define GET_AWL1()		GPIO_ReadInputDataBit(GPIOC,AWLFB1_PIN)
#define GET_AWL2()		GPIO_ReadInputDataBit(GPIOE,AWLFB2_PIN)

//for airbag warning lamp flash times
static int fault_flag1 = 0;
static int fault_flag2 = 0;
static int awl1_times = 0;
static int awl2_times = 0;
static int test_times = 0;
static time_t poweron_timer;
static int test_flag = 0;

//for test time para
static time_t bp_on_time;
static time_t bp_off_time;
static time_t bp_sfcheck_time;

static int bptest_mdelay(int ms);

void bptest_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//for gpio init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = RYCTRL_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
  
	GPIO_InitStructure.GPIO_Pin = AWLFB1_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = AWLFB2_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//for init state
	bp_on_time = BP_PowerOn_TIME;
	bp_off_time = BP_PowerOff_TIME;
	bp_sfcheck_time = BP_SELFCHECK_TIME;
	RY_CTRL(POWER_OFF);
	bptest_mdelay(1000);
}

void bptest_Update(void)
{
	if (test_flag) {
		test_times ++;
		printf("\nBegin the <%d round> burn panel power on/off test...\n", test_times);
		printf("Power on ...\n");
		poweron_timer = time_get(bp_on_time);
		RY_CTRL(POWER_ON);
		bptest_mdelay(bp_sfcheck_time);
		printf("Self checking is over ...\n");
		while(time_left(poweron_timer) > 0) {  //for airbag warning lamp detect
			task_Update();
			if (fault_flag1 == 0) {
				if (GET_AWL1() == AWL_ON) {
					bptest_mdelay(2);
					if (GET_AWL1() == AWL_ON) {
						awl1_times ++;
						fault_flag1 = 1;
						printf("awl1 fault captured,test time left %d s\n", time_left(poweron_timer)/1000);
					}
				}
			}
			if (fault_flag2 == 0) {
				if (GET_AWL2() == AWL_ON) {
					bptest_mdelay(2);
					if (GET_AWL2() == AWL_ON) {
						awl2_times ++;
						fault_flag2 = 1;
						printf("awl2 fault captured,test time left %d s\n", time_left(poweron_timer)/1000);
					}
				}
			}
			if (test_flag == 0) {
				break;
			}
		}
		printf("Power off ...\n");
		RY_CTRL(POWER_OFF);
		printf("test times is %d, awl1 fault times is %d\n", test_times, awl1_times);
		printf("test times is %d, awl2 fault times is %d\n", test_times, awl2_times);
		printf("##########This round test over!!!##########################\n");

		//related varibles back to initial value
		fault_flag1 = 0;
		fault_flag2 = 0;
		bptest_mdelay(bp_off_time);
	}
}

static int bptest_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 5) { //system update period is expected less than 10ms
			task_Update();
		}
	} while(left > 0);

	return 0;
}

void main(void)
{
	task_Init();
	bptest_Init();
	while(1) {
		task_Update();
		bptest_Update();
	}
}

#if 1
#include <stdlib.h>
#include <stdio.h>
#include "ulp_time.h"
#include "shell/cmd.h"
#include <string.h>

int cmd_bp_func(int argc, char *argv[])
{
	int temp;

	const char * usage = { \
		" usage:\n" \
		" bp start,   start the burn panel power on/off test\n" \
		" bp status,  return the result of test\n"
		" bp stop,    stop burn panel test\n"
		" bp on,      set power on for the panel\n"
		" bp off,     set power off for the panel\n"
		" bp set on off sfcheck, set power on and power off time(s) and sfcheck time(s)\n"
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (strcmp(argv[1],"start") == 0) {
		test_flag = 1;
	}

	if (strcmp(argv[1], "status") == 0) {
		printf("test times is %d, awl1 fault times is %d\n", test_times, awl1_times);
		printf("test times is %d, awl2 fault times is %d\n", test_times, awl2_times);
		printf("power on time is %d s\n", bp_on_time/1000);
		printf("power off time is %d s\n", bp_off_time/1000);
		printf("burn panel selfcheck time is %d s\n", bp_sfcheck_time/1000);
	}

	if (strcmp(argv[1], "stop") == 0) {
		test_flag = 0;
	}

	if (strcmp(argv[1], "on") == 0) {
		RY_CTRL(POWER_ON);
		printf("Burn panel power on\n");
	}

	if (strcmp(argv[1], "off") == 0) {
		RY_CTRL(POWER_OFF);
		printf("Burn panel power off\n");
	}

	if (strcmp(argv[1], "set") == 0) {
		sscanf(argv[2], "%d", &temp);
		bp_on_time = (time_t)(temp * 1000);
		printf("power on time is %d s\n", temp);
		sscanf(argv[3], "%d", &temp);
		bp_off_time = (time_t)(temp * 1000);
		printf("power off time is %d s\n", temp);
		sscanf(argv[4], "%d", &temp);
		bp_sfcheck_time = (time_t)(temp * 1000);
		printf("burn panel selfcheck time is %d s\n", temp);
	}

	return 0;
}

const cmd_t cmd_bp = {"bp", cmd_bp_func, "bp cmds"};
DECLARE_SHELL_CMD(cmd_bp)
#endif
