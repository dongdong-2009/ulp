// #
// # this app is for burn panel power on/off test
// #
// # Written by david@2012
// #
// # Input/output configration:
// # Relay Control Pin:PC4
// # AWL Input Detect Pin:PC5

#include "config.h"
#include <stdio.h>
#include "ulp_time.h"
#include "nvm.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "stm32f10x_gpio.h"

#define BP_SELFCHECK_TIME		8000 // for product self checking time
#define BP_PowerOn_TIME			(1000 * 60 * 10) //for power on time 10 min
#define BP_PowerOff_TIME		(1000 * 10) //for power off time 10 s

#define AWL_OFF			Bit_RESET
#define AWL_ON			Bit_SET

#define POWER_OFF		Bit_RESET
#define POWER_ON		Bit_SET

#define RYCTRL_PIN		GPIO_Pin_4
#define AWLFB_PIN		GPIO_Pin_5

#define RY_CTRL(ba)		GPIO_WriteBit(GPIOC,RYCTRL_PIN,ba)
#define GET_AWL()		GPIO_ReadInputDataBit(GPIOC,AWLFB_PIN)

static int awl_times = 0;
static int test_times = 0;
static time_t poweron_timer;
static int test_flag = 0;

static int bptest_mdelay(int ms);

void bptest_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//for gpio init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = RYCTRL_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
  
	GPIO_InitStructure.GPIO_Pin = AWLFB_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//for init state
	RY_CTRL(POWER_OFF);
	bptest_mdelay(1000);
}

void bptest_Update(void)
{
	if (test_flag) {
		test_times ++;
		printf("\nPrepare the <%d round> burn panel power on/off test...\n", test_times);
		printf("Power on ...\n");
		poweron_timer = time_get(BP_PowerOn_TIME);
		RY_CTRL(POWER_ON);
		bptest_mdelay(BP_SELFCHECK_TIME);
		printf("Self checking is over ...\n");
		while(time_left(poweron_timer) > 0) {  //for airbag warning lamp detect
			task_Update();
			if (GET_AWL() == AWL_ON) {
				awl_times ++;
				printf("test time left %d min\n", time_left(poweron_timer)/1000/60);
				printf("awl times is %d \n", awl_times);
				break;
			}
		}
		printf("Power off ...\n");
		RY_CTRL(POWER_OFF);
		bptest_mdelay(BP_PowerOff_TIME);
		printf("##########This round test over!!!##########################\n");
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
	const char * usage = { \
		" usage:\n" \
		" bp start,   start the burn panel power on/off test\n" \
		" bp status,  return the result of test\n"
		" bp stop,    stop burn panel test\n"
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (strcmp(argv[1],"start") == 0) {
		test_flag = 1;
	}

	if (strcmp(argv[1], "status") == 0) {
		printf("Test times is %d, Fail times is %d\n", test_times, awl_times);
	}

	if (strcmp(argv[1], "stop") == 0) {
		test_flag = 0;
	}

	return 0;
}

const cmd_t cmd_bp = {"bp", cmd_bp_func, "bp cmds"};
DECLARE_SHELL_CMD(cmd_bp)
#endif
