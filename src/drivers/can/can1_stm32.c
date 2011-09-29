/*
 *	miaofng@2010 initial version
 *	David, add interrupt support
 */

#include "config.h"
#include "can.h"
#include "stm32f10x.h"
#include <string.h>
#include "time.h"
#include "stm32f10x_it.h"

#if CONFIG_CAN1_RF_SZ > 0
#define ENABLE_CAN_INT 1
#define RX_FIFO_SZ CONFIG_CAN1_RF_SZ
static can_msg_t can_fifo_rx[RX_FIFO_SZ];
static int circle_header;
static int circle_tailer;
static int circle_number;
#endif

static int can_init(const can_cfg_t *cfg)
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	RCC_ClocksTypeDef  RCC_Clocks;

#ifdef CONFIG_CAN1_REMAP
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/*RX = PB8, TX = PB9 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinRemapConfig(GPIO_Remap1_CAN1 , ENABLE);
#else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/*RX = PA11 TX = PA12 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

	/* CAN register init */
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);

	//get apb clock
	RCC_GetClocksFreq(&RCC_Clocks);

	/* CAN cell init */
	CAN_InitStructure.CAN_TTCM = DISABLE;
	CAN_InitStructure.CAN_ABOM = DISABLE;
	CAN_InitStructure.CAN_AWUM = DISABLE;
	CAN_InitStructure.CAN_NART = DISABLE;
	CAN_InitStructure.CAN_RFLM = DISABLE;
	CAN_InitStructure.CAN_TXFP = ENABLE;
	CAN_InitStructure.CAN_Mode = (cfg -> silent) ? CAN_Mode_Silent : CAN_Mode_Normal;
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq;
	CAN_InitStructure.CAN_BS2 = CAN_BS2_5tq;
	CAN_InitStructure.CAN_Prescaler = RCC_Clocks.PCLK1_Frequency/cfg->baud/(1+3+5);
	CAN_Init(CAN1, &CAN_InitStructure);

	/* CAN filter init */
	CAN_FilterInitStructure.CAN_FilterNumber=0;
	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=0 ;
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

#if ENABLE_CAN_INT
	NVIC_InitTypeDef  NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
#ifndef STM32F10X_CL
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
#else
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
#endif
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
	circle_header = 0;
	circle_tailer = 0;
	circle_number = 0;
#endif

	return 0;
}

/*
	nonblocked send, success return 0, else return -1
*/
int can_send(const can_msg_t *msg)
{
	int ret = 0;
	CanTxMsg msg_st;

	msg_st.StdId = msg->id;
	msg_st.ExtId = msg->id;
	msg_st.IDE = (msg->flag & CAN_FLAG_EXT) ? CAN_ID_EXT : CAN_ID_STD;
	msg_st.RTR = (msg->flag & CAN_FLAG_RTR) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
	msg_st.DLC = msg->dlc;
	memcpy(msg_st.Data, msg->data, msg->dlc);
	ret = CAN_Transmit(CAN1, &msg_st);
	ret = (ret == CAN_NO_MB) ? -1 : 0;
	return ret;
}

int can_recv(can_msg_t *msg)
{
#if ENABLE_CAN_INT
	if (circle_number == 0) {
		return -1;
	} else {
		(*msg) = can_fifo_rx[circle_tailer];
		circle_number --;
		circle_tailer ++;
		if (circle_tailer == RX_FIFO_SZ)
			circle_tailer = 0;
	}
	return 0;
#else
	CanRxMsg msg_st;

	if(CAN_MessagePending(CAN1, CAN_FIFO0) > 0)
		CAN_Receive(CAN1, CAN_FIFO0, &msg_st);
	else if(CAN_MessagePending(CAN1, CAN_FIFO1) > 0)
		CAN_Receive(CAN1, CAN_FIFO1, &msg_st);
	else
		return -1;

	msg->id = (msg_st.IDE == CAN_ID_EXT) ? msg_st.ExtId : msg_st.StdId;
	msg->flag = (msg_st.IDE == CAN_ID_EXT) ? CAN_FLAG_EXT : 0;
	msg->dlc = msg_st.DLC;
	memcpy(msg->data, msg_st.Data, msg->dlc);
	return 0;
#endif
}

int can_filt(can_filter_t *filter, int n)
{
	int i, j, ret = 0;
	short id0, id1, msk0, msk1;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	for(i = 0, j = 0; i < n; j ++) {
		id0 = filter[i].id << 5;
		msk0 = filter[i].mask << 5;
		i ++;

		id1 = (i < n) ? filter[i].id << 5 : 0x0000;
		msk1 = (i < n) ? filter[i].mask << 5 : 0xffff;
		i ++;

		/* CAN filter init */
		CAN_FilterInitStructure.CAN_FilterNumber = j;
		CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask; //default id mask mode
		CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_16bit; //default id 16bit
		CAN_FilterInitStructure.CAN_FilterIdHigh = id0;
		CAN_FilterInitStructure.CAN_FilterIdLow = id1;
		CAN_FilterInitStructure.CAN_FilterMaskIdHigh = msk0;
		CAN_FilterInitStructure.CAN_FilterMaskIdLow = msk1;
		CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0 ; //default fifo use 0
		CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
		CAN_FilterInit(&CAN_FilterInitStructure);
	}
	return ret;
}

void can_flush(void)
{
	int busy;
	time_t deadline = time_get(10); //max wait 10 ms
	do {
		busy = 0;
		busy += (CAN_TransmitStatus(CAN1, 0) == CANTXPENDING);
		busy += (CAN_TransmitStatus(CAN1, 1) == CANTXPENDING);
		busy += (CAN_TransmitStatus(CAN1, 2) == CANTXPENDING);
		if(!busy)
			break;
	} while(time_left(deadline) > 0);

	CAN_CancelTransmit(CAN1, 0);
	CAN_CancelTransmit(CAN1, 1);
	CAN_CancelTransmit(CAN1, 2);
	CAN_FIFORelease(CAN1, CAN_FIFO0);
	CAN_FIFORelease(CAN1, CAN_FIFO1);
}

#if ENABLE_CAN_INT
//for can1 receive FIFO 0 interrupt
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CanRxMsg msg_st;
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	while (CAN_MessagePending(CAN1, CAN_FIFO0)) {
		CAN_Receive(CAN1, CAN_FIFO0, &msg_st);
		can_fifo_rx[circle_header].id = (msg_st.IDE == CAN_ID_EXT) ? msg_st.ExtId : msg_st.StdId;
		can_fifo_rx[circle_header].flag = (msg_st.IDE == CAN_ID_EXT) ? CAN_FLAG_EXT : 0;
		can_fifo_rx[circle_header].dlc = msg_st.DLC;
		memcpy(can_fifo_rx[circle_header].data, msg_st.Data, msg_st.DLC);
		circle_number ++;
		if (circle_number == RX_FIFO_SZ) {
			circle_number --;
		} else {
			circle_header ++;
			if (circle_header == RX_FIFO_SZ)
				circle_header = 0;
		}
	}
}
#endif

const can_bus_t can1 = {
	.init = can_init,
	.send = can_send,
	.recv = can_recv,
	.filt = can_filt,
	.flush = can_flush,
};
