/*
 *	miaofng@2010 initial version
 */

#include "config.h"
#include "can.h"
#include "lm3s.h"
#include <string.h>
#include <assert.h>

#define LM3S_TxMsgObjNr 32	//send msg object
#define LM3S_RxMsgObjNr 1	//recv msg object

static int can_init(const can_cfg_t *cfg)
{
	tCANMsgObject MsgObjectRx;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
	GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	CANInit(CAN0_BASE);

	//PLLclock is 400M, and can module clock is 8M(400/50)
	if (CANBitRateSet(CAN0_BASE, 8000000, cfg->baud) == 0)
		return 1;

	//can receive setting,set MsgIDMask value to zero, receive all!
	MsgObjectRx.ulFlags = MSG_OBJ_NO_FLAGS | MSG_OBJ_USE_ID_FILTER;
	MsgObjectRx.ulMsgIDMask = 0;
	CANMessageSet(CAN0_BASE, LM3S_RxMsgObjNr, &MsgObjectRx, MSG_OBJ_TYPE_RX);
	CANEnable(CAN0_BASE);

	return 0;
}

/*
	nonblocked send, success return 0, else return -1
*/
int can_send(const can_msg_t *msg)
{
	unsigned long status;
	tCANMsgObject MsgObjectTx;

	status = CANStatusGet(CAN0_BASE, CAN_STS_TXREQUEST);
	if(status & (unsigned long)(1 << (LM3S_TxMsgObjNr - 1)))
		return 1;

	MsgObjectTx.ulMsgID = msg->id;
	MsgObjectTx.ulMsgLen = msg->dlc;
	MsgObjectTx.pucMsgData = (unsigned char *)msg->data;
	if (msg->flag) {
		MsgObjectTx.ulFlags = MSG_OBJ_EXTENDED_ID;
	} else {
		MsgObjectTx.ulFlags = MSG_OBJ_NO_FLAGS;
	}

	CANRetrySet(CAN0_BASE, LM3S_TxMsgObjNr);	//set retry send
	CANMessageSet(CAN0_BASE, LM3S_TxMsgObjNr, &MsgObjectTx, MSG_OBJ_TYPE_TX);

	return 0;
}

int can_recv(can_msg_t *msg)
{
	tCANMsgObject MsgObjectRx;
	int i;
	unsigned long MsgObjNr,temp;

	MsgObjNr = CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);
	if(MsgObjNr == 0)
		return 1;

	//find which msg object receives the can frame
	for (i = 0; i < 31; i++) {
		temp = 1 << i;
		if (MsgObjNr & temp)
			break;
	}

	MsgObjectRx.pucMsgData = (unsigned char *)msg->data;
	CANMessageGet(CAN0_BASE, i + 1, &MsgObjectRx, 1);
	msg->dlc = MsgObjectRx.ulMsgLen;
	if (MsgObjectRx.ulFlags & MSG_OBJ_EXTENDED_ID) {
		msg->flag = 1;
		msg->id = (MsgObjectRx.ulMsgID & 0x1FFFFFFF);
	} else {
		msg->flag = 0;
		msg->id = (MsgObjectRx.ulMsgID & 0x000007FF);
	}

	return 0;
}

int can_filt(can_filter_t *filter, int n)
{
	assert(1 == 0); //not supported yet!!!
	return 0;
}

const can_bus_t can0 = {
	.init = can_init,
	.send = can_send,
	.recv = can_recv,
	.filt = can_filt,
};
