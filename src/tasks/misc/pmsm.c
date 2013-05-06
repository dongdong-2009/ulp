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

const pmsm_t pmsm1 = {
	.pp = 2,
	.crp = 400,
};

struct pid_q15_s speed_pid = {
	.kp = 4,
	.ki = 1,
	.kd = 0,
};

int foc1;

int main(void)
{
	task_Init();
	foc1 = foc_new();
	foc_cfg(foc1, "frq", 20000, "pmsm", &pmsm1, "board", &pmsmd1_stm32, "speed_pid", &speed_pid, "end");
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
		"  foc set speed xxx\n"
	};
	int a;
	if(argc == 4 && !strcmp(argv[1], "set")) {
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
