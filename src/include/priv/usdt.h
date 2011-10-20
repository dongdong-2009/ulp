/*
 * 	david@2011 initial version
 *
 */
#ifndef __USDT_H_
#define __USDT_H_

#include "config.h"
#include "can.h"
#include <stddef.h>

int usdt_Init(can_cfg_t * cfg);
int usdc_GetDiagFirstFrame(can_msg_t *pReq, can_filter_t *pResFilter, can_msg_t *pRes);
int usdc_GetDiagLeftFrame(can_msg_t *pRes, int msg_len);

#endif /*__USDT_H_*/
