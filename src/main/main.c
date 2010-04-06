/******************** (C) COPYRIGHT 2007 STMicroelectronics ********************
* File Name          : main.c
* Author             : MCD Application Team
* Version            : V1.0
* Date               : 10/08/2007
* Description        : Main program body
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "debug.h"
#include "sys/system.h"
#include "motor/motor.h"
#include "vvt/vvt.h"
#include "shell/shell.h"

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main(void)
{
#if CONFIG_SYS_DEBUG == 1
	sys_Debug();
#endif

#if CONFIG_MOTOR_DEBUG == 1
	motor_Debug();
#endif
	
	//init funcs
	sys_Init();
#if CONFIG_TASK_MOTOR == 1
	motor_Init();
#endif
#if CONFIG_TASK_VVT == 1
	vvt_Init();
#endif
#if CONFIG_TASK_SHELL == 1
	shell_Init();
#endif
	
	while(1) {
		//update funcs
		sys_Update();
#if CONFIG_TASK_MOTOR == 1
		motor_Update();
#endif
#if CONFIG_TASK_VVT == 1
		vvt_Update();
#endif
#if CONFIG_TASK_SHELL == 1
		shell_Update();
#endif
	}
}
