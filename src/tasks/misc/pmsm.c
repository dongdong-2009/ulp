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
#include "sys/task.h"
#include "pmsm/foc.h"
#include "pmsm/pmsm.h"
#include "ulp/pmsmd.h"
#include "stm32f10x.h"

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

const pmsm_t pmsm1 = {
	.pp = 2,
	.crp = 400,
};

int foc1;

int main(void)
{
	task_Init();
	foc1 = foc_new();
	foc_cfg(foc1, "frq", 20000, "pmsm", &pmsm1, "board", &pmsmd1_stm32, "end");
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
