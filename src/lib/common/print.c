/*
 * 	miaofng@2010 initial version
 *		thread safe version of printf
 *		print_init/update will run in Update thread which hold the console
 *		print may be called by a thread
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "common/print.h"
#include "sys/task.h"

static xQueueHandle xPrintQueue;

void print_init(void)
{
	xPrintQueue = xQueueCreate(PRINT_QUEUE_SIZE, sizeof(char *));
}

void print_update(void)
{
	char *pstr;
	while(xQueueReceive(xPrintQueue, (void *) &pstr, 0) == pdPASS) {
		printf(pstr);
		vPortFree(pstr);
	}
}

DECLARE_TASK(print_init, print_update)

int print(const char *msg)
{
	int len;
	char *pstr;

	len = strlen(msg);
	pstr = pvPortMalloc(len + 1);
	if(pstr != NULL) {
		strcpy(pstr, msg); //copy, avoid msg is constructed in stack
		xQueueSend(xPrintQueue, (void *) &pstr, portMAX_DELAY);
		return 0;
	}
	
	return -1;
}


