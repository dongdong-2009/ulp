/*
 * 	david@2011 initial version
 *
 */
#ifndef __USDT_H_
#define __USDT_H_

#include "config.h"
#include "can.h"
#include <stddef.h>

int usdt_Init(can_bus_t const *pcan);

/*pResFilter: in apt and pdi project,this value can be set to NULL*/
int usdt_GetDiagFirstFrame(can_msg_t const *pReq, int req_len, can_filter_t const *pResFilter, can_msg_t *pRes, int *p_msg_len);

/*msg_len: get from usdt_GetDiagFirstFrame function*/
int usdt_GetDiagLeftFrame(can_msg_t *pRes, int msg_len);

#endif /*__USDT_H_*/
