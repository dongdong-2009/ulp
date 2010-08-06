/*
 * 	miaofng@2010 initial version
 *		this is the main program of Handheld Vehicle Programmer
 *
 */

#include "config.h"
#include "hvp.h"
#include "mt2x.h"
#include "sys/task.h"
#include "shell/cmd.h"
#include "common/print.h"
#include "led.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "ff.h"

#define __DEBUG

/*private var declaration*/
static FATFS hvp_fs;
static xQueueHandle hvp_msg_queue;

/*private func declaration*/
static void hvp_thread(void *pvParameters);

void hvp_Init(void)
{
	//mount sd card
	if(f_mount(0, &hvp_fs))
		printf("mount sd card err!!!\n");
	hvp_msg_queue = xQueueCreate(1, sizeof(hvp_msg_t));
	xTaskCreate(hvp_thread, (signed portCHAR *) "Hvp", 256, NULL, tskIDLE_PRIORITY + 1, NULL);	
}

void hvp_Update(void)
{
}

DECLARE_TASK(hvp_Init, hvp_Update)

static void hvp_thread(void *pvParameters)
{
	int len;
	char *folder;
	hvp_msg_t msg;
	
	//indicate idle status
	led_on(LED_GREEN);
	led_off(LED_RED);
	
	//wait for cmd sent by Update thread ...
	while(xQueueReceive(hvp_msg_queue, &msg, portMAX_DELAY) == pdPASS) {
		//indicate busy
		led_on(LED_GREEN);
		led_on(LED_RED);
		
		// ...excute the command
		len = strlen(msg.para1);
		len += strlen(msg.para2);
		len += 2;
		folder = pvPortMalloc(len + 1);
		strcpy(folder, msg.para1);
		strcat(folder, "/");
		strcat(folder, msg.para2);
		strcat(folder, "/");
		vPortFree(msg.para1);
		vPortFree(msg.para2);
#ifdef __DEBUG
		f_chdir("/MT20U/28025614");
#else
		f_chdir(folder);
#endif
		vPortFree(folder);
		if(!mt2x_Init()) {
			print("mt2x programmer is selected\n");
			if(!mt2x_Prog())
				print("O(กษ_กษ)O\n");
			else
				print("~~~~(>_<)~~~~ \n");
		}
		
		//indicate idle
		led_on(LED_GREEN);
		led_off(LED_RED);
	}
}

/*inform the hvp thread: "hey boy, it's time to programming now ...:)"
	model: something like MT22U
	sub: something like 28172362
*/
int hvp_prog(char *model, char *sub)
{
	int len;
	hvp_msg_t msg;
	portBASE_TYPE ret;
	
	msg.cmd = HVP_CMD_PROGRAM;
	len = strlen(model);
	msg.para1 = pvPortMalloc(len + 1);
	strcpy(msg.para1, model);
	len = strlen(sub);
	msg.para2 = pvPortMalloc(len + 1);
	strcpy(msg.para2, sub);
	ret = xQueueSend(hvp_msg_queue, &msg, 0);
	if(ret != pdTRUE) {
		vPortFree(msg.para1);
		vPortFree(msg.para2);
		return -1;
	}
	
	return 0;
}

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

	hvp_prog(argv[1], argv[2]);
	return 0;
}

const cmd_t cmd_hvp = {"hvp", cmd_hvp_func, "hvp programmer"};
DECLARE_SHELL_CMD(cmd_hvp)
