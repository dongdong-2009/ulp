/*
 * 	miaofng@2010 initial version
 *		this is the main program of Handheld Vehicle Programmer
 *
 */

#include "config.h"
#include "hvp.h"
#include "sys/task.h"
#include "shell/cmd.h"
#include "led.h"
#include <stdio.h>
#include <stdlib.h>

/*private var declaration*/
static char hvp_stm;
static const pa_t *hvp_pa;

/*private func declaration*/
static int hvp_program(char *model, char *sub);

void hvp_Init(void)
{
	hvp_stm = HVP_STM_IDLE;
	hvp_pa = NULL;

	led_on(LED_GREEN);
	led_off(LED_RED);
}

void hvp_Update(void)
{
	int ret;
	if(hvp_stm == HVP_STM_START) {
		ret = hvp_pa->update();
		if(ret == 0) {
			led_on(LED_GREEN);
			led_off(LED_RED);
			hvp_stm = HVP_STM_IDLE;
		}
		else if(ret < 0) {
			led_on(LED_RED);
			led_off(LED_GREEN);
			hvp_stm = HVP_STM_ERROR;
		}
	}

	if(hvp_stm == HVP_STM_ERROR) {
	}
}

DECLARE_TASK(hvp_Init, hvp_Update)

static int cmd_hvp_func(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" hvp MT22U 28172362\n" \
	};

	if(argc != 3) {
		printf(usage);
		return 0;
	}

	hvp_program(argv[1], argv[2]);
	return 0;
}

const cmd_t cmd_hvp = {"hvp", cmd_hvp_func, "hvp programmer"};
DECLARE_SHELL_CMD(cmd_hvp)

extern pa_t mt2x;

/*get the model info from the configuration file*/
static int hvp_program(char *model, char *sub)
{
	int ret = 0;

	/*try to match pa, and read config*/
	hvp_pa = &mt2x;
	hvp_pa->init();

	/*start program state machine*/
	if(hvp_stm == HVP_STM_IDLE) {
		hvp_stm = HVP_STM_START;
		led_flash(LED_GREEN);
		led_off(LED_RED);
	}
	else
		ret = -1;

	return ret;
}
