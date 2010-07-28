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
#include <stdarg.h>

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

int print(const char *fmt, ...)
{
	va_list ap;
	char *pstr, *p;
	int n;
	
	n = 0;
	pstr = pvPortMalloc(PRINT_LINE_SIZE);
	if(pstr != NULL) {
		va_start(ap, fmt);
		n += vsnprintf(pstr + n, PRINT_LINE_SIZE - n, fmt, ap);
		va_end(ap);

		n = strlen(pstr);
		p = pvPortMalloc(n + 1);
		strcpy(p, pstr);
		vPortFree(pstr);
		xQueueSend(xPrintQueue, &p, portMAX_DELAY);
		return 0;
	}
	
	return -1;
}


