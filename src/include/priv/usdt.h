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
int usdt_GetDiagFirstFrame(can_msg_t const *pReq, int req_len, can_filter_t *pResFilter, can_msg_t *pRes);
int usdt_GetDiagLeftFrame(can_msg_t *pRes, int msg_len);

#endif /*__USDT_H_*/
