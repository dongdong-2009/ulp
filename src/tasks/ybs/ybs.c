/*
 * junjun@2011 initial version
 */

#include <stdio.h>
#include "config.h"
#include "lib/ybs.h"
#include "sys/task.h"

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void main(void)
{
	task_Init();
	ybs_Init();
	printf("Power Conditioning - YBS\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		task_Update();
		ybs_Update();
		//gpcon_signal(SENSOR,SENPR_ON);
		//ybs_mdelay(5000);
		//gpcon_signal(SENSOR,SENPR_OFF);
		//ybs_mdelay(5000);
	}
} // main
