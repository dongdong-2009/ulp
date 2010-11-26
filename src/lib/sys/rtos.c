/*
 * 	miaofng@2010 initial version
		this file is used when rtos is enabled
 */

#include "config.h"
#include "sys/task.h"
#include "sys/system.h"
#include "time.h"
#include "driver.h"

/* rtos includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern void xPortSysTickHandler( void );

static void vUpdateTask(void *pvParameters)
{
	drv_Init();
	lib_init();
	task_init();
	while(1) {
		lib_update();
		task_update();
	}
}

void task_Init(void)
{
	sys_Init();
	xTaskCreate( vUpdateTask, (signed portCHAR *) "Update", 512, NULL, tskIDLE_PRIORITY + 1, NULL );	
	
	/* Start the scheduler. */
	vTaskStartScheduler();
	
	/* Will only get here if there was not enough heap space to create the
	idle task. */
}

void task_Update(void)
{
	//never reach here
}

void task_Isr(void)
{
	time_isr();
	xPortSysTickHandler();
}

void task_SetForeground(void (*task)(void))
{
}
