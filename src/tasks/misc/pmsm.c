/*
 * 	king@2013 initial version
 */

#include "config.h"
#include "sys/task.h"
#include "pmsm/foc.h"
#include "pmsm/pmsm.h"
#include "ulp/pmsmd.h"
#include "stm32f10x.h"
#include "common/pid.h"
#include "shell/cmd.h"
#include <string.h>

void dac_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	DAC_InitTypeDef DAC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	DAC_StructInit(&DAC_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_1, ENABLE);
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_2, ENABLE);
}






const pmsm_t pmsm1 = {
	.pp = 2,
	.crp = 400,
};

struct pid_q15_s speed_pid = {
	.kp = 3,
	.ki = 10,
	.kd = 0,
	.x[0] = 0,
	.x[1] = 0,
	.y = 0,
	.max = 0x00004900,
	.min = -0x00004900
};

struct pid_q15_s id_pid = {
	.kp = 0,
	.ki = 0,
	.kd = 0,
	.x[0] = 0,
	.x[1] = 0,
	.y = 0,
	.max = 0x00003900,
	.min = -0x00003900
};

struct pid_q15_s iq_pid = {
	.kp = 1,
	.ki = 0,
	.kd = 0,
	.x[0] = 0,
	.x[1] = 0,
	.y = 0,
	.max = 0x00003900,
	.min = -0x00003900
};


int foc1;
int start = 0;

 int main(void)
{
	task_Init();

	dac_init();

	foc1 = foc_new();
	foc_cfg(foc1, "frq", 20000, "pmsm", &pmsm1, "board", &pmsmd1_stm32, "speed_pid", &speed_pid, "id_pid", &id_pid, "iq_pid", &iq_pid, "end");

	while(!start) {
		task_Update();
	}
	foc_init(foc1);
	while(1) {
		task_Update();
		foc_update(foc1);
	}
}

void TIM1_CC_IRQHandler(void)
{
	if((TIM1->SR & 0x0010) != 0 && (TIM1->DIER & 0x0010) != 0) {
		foc_isr(foc1);
		TIM1->SR &= 0xffef;
	}
}

static int cmd_foc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"  foc start\n"
		"  foc test\n"
		"  foc set speed xxx\n"
	};
	int a;
	if(argc == 2 && !strcmp(argv[1], "start")) {
		start = 1;
		printf("foc feedback start\n");
		return 0;
	}
	else if(argc == 2 && !strcmp(argv[1], "test")) {
		printf("foc feedback test\n");
		return 0;
	}
	else if(argc == 4 && !strcmp(argv[1], "set")) {
		if(!strcmp(argv[2], "speed")) {
			sscanf(argv[3], "%d", &a);
			foc_speed_set(foc1, a);
			return 0;
		}
		else {
			printf("error: command is wrong!!\n");
			printf("%s", usage);
			return -1;
		}
	}
	else {
		printf("error: command is wrong!!\n");
		printf("%s", usage);
		return -1;
	}
}

const cmd_t cmd_foc = {"foc", cmd_foc_func, "control foc"};
DECLARE_SHELL_CMD(cmd_foc)
