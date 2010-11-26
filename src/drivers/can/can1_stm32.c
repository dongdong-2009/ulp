/*
 *	miaofng@2010 initial version
 */

#include "config.h"
#include "can.h"
#include "stm32f10x.h"
#include <string.h>

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
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
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
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=0;
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);
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
}

const can_bus_t can1 = {
	.init = can_init,
	.send = can_send,
	.recv = can_recv,
};
