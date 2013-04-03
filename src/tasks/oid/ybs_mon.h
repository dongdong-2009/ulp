/*
 * 	miaofng@2013-3-27 initial version
 */

#ifndef __YBS_MON_H_
#define __YBS_MON_H_

#include "uart.h"
#include "stm32f10x.h"

#define MCD_CH_DET	11
#define MCD_CH_ASIG	10

#define ybs_uart uart2
#define ybs_uart_sel() do { \
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_RESET); \
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_RESET); \
} while(0)

#define IN_RANGE(x, min, max) (((x) >= (min)) & ((x) < (max)))
#define OV_RANGE(x, min, max) (!IN_RANGE(x, min, max))

enum {
	OK = 0,
	BUSY,
	E_PARA,
	E_MCD_READ,
	E_SGM_READ,
	E_MGF_OVER,
	E_POS_OVER,
	E_LVL_OVER,

	E_YBS_COMM, /*ybs communication general fail*/
	E_YBS_FRAME, /*ybs communication frame error*/
	E_YBS_TIMEOUT, /*ybs communication error*/
	E_YBS_RESPONSE, /*ybs sensor error*/

	E_CAL_PRECHECK_DET,
	E_CAL_PRECHECK_ASIG,
};

int monitor_init(void);

int sgm_init(void);
int sgm_reset_to_zero(void);
int sgm_read(int *mgf);

void mov_i(int distance); /*for initially positioning purpose*/
int mov_f(int target_mgf, int ms_per_step);
int mov_n(int steps, int mgf_max);
int mov_p(int pos); //mov to absolute position

#endif
