/*
 *  David@2012 initial version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "stm32f10x.h"
#include "ulp_time.h"
#include "stm32f10x_it.h"
#include "lin.h"

#if CONFIG_LIN1_RF_SZ > 0
#define RX_FIFO_SZ CONFIG_LIN1_RF_SZ
static lin_msg_t lin_fifo_rx[RX_FIFO_SZ];
static int circle_header;
static int circle_tailer;
static int circle_number;
#endif

#define uart USART1
#define Enable_BreakInt()    USART_ITConfig(uart, USART_IT_LBD, ENABLE)
#define Disable_BreakInt()   USART_ITConfig(uart, USART_IT_LBD, DISABLE)
#define Enable_RecvInt()     USART_ITConfig(uart, USART_IT_RXNE, ENABLE)
#define Disable_RecvInt()    USART_ITConfig(uart, USART_IT_RXNE, DISABLE)

//val ? [min, max]
#define IS_IN_RANGE(val, min, max) \
		(((val) >= (min)) && ((val) <= (max)))

// A is the variable while B is the bit number
#define BIT(A,B) \
		((A>>B)&0x01)

//local varibles
static lin_frame_t st_frame;
static int node_type, rsp_count, RECV_DATA_LENGTH;
static unsigned char master_pid;
static lin_msg_t rsp_msg; //bus for slave task sending data to lin bus
static int (*pid_handler)(unsigned char , lin_msg_t *);

//local functions
static unsigned char LINCalcParity(unsigned char id);
static unsigned char LINCalcChecksum(unsigned char *data, int num);
static void slave_node_process(void);
static void master_node_process(void);
static int lin_init(lin_cfg_t * cfg);
static int lin_sendpid(unsigned char pid);
static int lin_recvmsg(lin_msg_t *pmsg);
static int lin_ckstate(void);
static void lin_error_reset(void);

static int lin_init(lin_cfg_t * cfg)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
	/*configure PA9<uart1.tx>, PA10<uart1.rx>*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*init serial port*/
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = cfg->baud;
	USART_Init(uart, &uartinfo);

	/* Configure the NVIC Preemption Priority Bits */  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_LINBreakDetectLengthConfig(uart, USART_LINBreakDetectLength_10b);
	USART_LINCmd(uart, ENABLE);
	Enable_BreakInt();
	Disable_RecvInt();
	USART_Cmd(uart, ENABLE);
	pid_handler = cfg -> pid_callback;

	//data init
	st_frame.state = LIN_IDLE;
	st_frame.status = ERROR_NONE;
	st_frame.msg.dlc = 0;
	rsp_count = 0;
	node_type = cfg->node;
#if CONFIG_LIN1_RF_SZ > 0
	circle_header = 0;
	circle_tailer = 0;
	circle_number = 0;
#endif
	return 0;
}

static void master_node_process(void)
{
	char ch;

	switch(st_frame.state) {
	case LIN_BREAK:
		//clear error flags, must be added
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE | USART_FLAG_ORE | 
							   USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)
							   != RESET) 
		{
			ch = USART_ReceiveData(uart);
		}
		if(USART_GetFlagStatus(uart, USART_FLAG_LBD) != RESET) {
			printf("break detected \n");
			USART_ClearFlag(uart, USART_FLAG_LBD);
		} else {
			lin_error_reset();
			st_frame.status = ERROR_BREAK;
			break;
		}
		//master node send sync char
		USART_SendData(uart, 0x55);
		st_frame.state = LIN_SYNC;
		Disable_BreakInt();
		Enable_RecvInt();
		break;
	case LIN_SYNC:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recv sync 0x% x\n", ch);
		}
		if (ch == 0x55) {
			//master node sends pid
			USART_SendData(uart, LINCalcParity(master_pid));
			st_frame.state = LIN_PID;
		} else {
			lin_error_reset();
			st_frame.status = ERROR_SYNC;
			printf("sync error \n");
			break;
		}
		break;
	case LIN_PID:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recv pid, 0x%x\n", ch&0x3f);
		}
		ch &= 0x3f;
		st_frame.msg.pid = ch;
		if (!IS_IN_RANGE(ch, 0x00, 0x3f)) {
			lin_error_reset();
			st_frame.status = ERROR_PID;
			break;
		}
		if (pid_handler(st_frame.msg.pid, &rsp_msg) == LIN_SLAVE_TASK_RESPONSE) {
			USART_SendData(uart, rsp_msg.data[rsp_count]);
			rsp_count ++;
			st_frame.state = LIN_TRANSMITING;
		} else { 
			//LIN_SLAVE_RX_FRAME
			//calc the length of the data
			if(IS_IN_RANGE(ch, 0x00, 0x1f))
				RECV_DATA_LENGTH = 0x02;
			else if(IS_IN_RANGE(ch, 0x20, 0x2f))
				RECV_DATA_LENGTH = 0x04;
			else if(IS_IN_RANGE(ch, 0x30, 0x3f))
				RECV_DATA_LENGTH = 0x08;
			st_frame.state = LIN_RECEIVING;
		}
		break;
	case LIN_TRANSMITING:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recving data, 0x%2x\n", ch);
		}
		if (rsp_count < rsp_msg.dlc) {
			USART_SendData(uart, rsp_msg.data[rsp_count]);
			rsp_count++;
		} else {
			st_frame.state = LIN_CHECKSUM;
			USART_SendData(uart, LINCalcChecksum(rsp_msg.data, rsp_msg.dlc));
		}
		st_frame.msg.data[st_frame.msg.dlc] = ch;
		st_frame.msg.dlc ++;
		break;
	case LIN_RECEIVING:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recving data, 0x%2x\n", ch);
		}
		st_frame.msg.data[st_frame.msg.dlc] = ch;
		st_frame.msg.dlc ++;
		if (st_frame.msg.dlc == RECV_DATA_LENGTH) {
			st_frame.state = LIN_CHECKSUM;
		}
	case LIN_CHECKSUM:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recv cs 0x%x\n", ch);
		}
		st_frame.cs = ch;
		if (ch != LINCalcChecksum(st_frame.msg.data, st_frame.msg.dlc)) {
			lin_error_reset();
			st_frame.status = ERROR_CS;
		}
		// copy rx data to lin buffer
		if (circle_number < RX_FIFO_SZ) {
			circle_number ++;
			lin_fifo_rx[circle_header] = st_frame.msg;
			circle_header ++;
			if (circle_header == RX_FIFO_SZ)
				circle_header = 0;
		}
		//init all slave task variables
		lin_error_reset();
		st_frame.status = ERROR_NONE;
		break;
	case LIN_IDLE:
	case LIN_ERROR:
	default:
		break;
	}
}

static void slave_node_process(void)
{
	char ch;

	switch(st_frame.state) {
	case LIN_IDLE:
		//clear error flags, must be added
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE | USART_FLAG_ORE | 
							   USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)
							   != RESET) 
		{
			ch = USART_ReceiveData(uart);
		}
		if(USART_GetFlagStatus(uart, USART_FLAG_LBD) != RESET) {
			printf("break detected \n");
			USART_ClearFlag(uart, USART_FLAG_LBD);
		} else {
			lin_error_reset();
			st_frame.status = ERROR_BREAK;
			break;
		}
		st_frame.state = LIN_SYNC; //jump break state for it taken as tri-input
		Disable_BreakInt();
		Enable_RecvInt();
		break;
	case LIN_SYNC:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recv sync 0x%x\n", ch);
		}
		if (ch == 0x55) {
			st_frame.state = LIN_PID;
		} else {
			lin_error_reset();
			st_frame.status = ERROR_SYNC;
			printf("sync error \n");
			break;
		}
		break;
	case LIN_PID:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recv pid, 0x%x\n", ch&0x3f);
		}
		ch &= 0x3f;
		st_frame.msg.pid = ch;
		if (!IS_IN_RANGE(ch, 0x00, 0x3f)) {
			lin_error_reset();
			st_frame.status = ERROR_PID;
			break;
		}
		if (pid_handler(st_frame.msg.pid, &rsp_msg) == LIN_SLAVE_TASK_RESPONSE) {
			USART_SendData(uart, rsp_msg.data[rsp_count]);
			rsp_count ++;
			st_frame.state = LIN_TRANSMITING;
		} else { 
			//LIN_SLAVE_RX_FRAME
			//calc the length of the data
			if(IS_IN_RANGE(ch, 0x00, 0x1f))
				RECV_DATA_LENGTH = 0x02;
			else if(IS_IN_RANGE(ch, 0x20, 0x2f))
				RECV_DATA_LENGTH = 0x04;
			else if(IS_IN_RANGE(ch, 0x30, 0x3f))
				RECV_DATA_LENGTH = 0x08;
			st_frame.state = LIN_RECEIVING;
		}
		break;
	case LIN_TRANSMITING:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recving data, 0x%2x\n", ch);
		}
		if (rsp_count < rsp_msg.dlc) {
			USART_SendData(uart, rsp_msg.data[rsp_count]);
			rsp_count++;
		} else {
			st_frame.state = LIN_CHECKSUM;
			USART_SendData(uart, LINCalcChecksum(rsp_msg.data, rsp_msg.dlc));
		}
		st_frame.msg.data[st_frame.msg.dlc] = ch;
		st_frame.msg.dlc ++;
		break;
	case LIN_RECEIVING:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recving data, 0x%2x\n", ch);
		}
		st_frame.msg.data[st_frame.msg.dlc] = ch;
		st_frame.msg.dlc ++;
		if (st_frame.msg.dlc == RECV_DATA_LENGTH) {
			st_frame.state = LIN_CHECKSUM;
		}
	case LIN_CHECKSUM:
		if(USART_GetFlagStatus(uart, USART_FLAG_RXNE) != RESET) {
			ch = USART_ReceiveData(uart);
			printf("recv cs 0x%x\n", ch);
		}
		st_frame.cs = ch;
		if (ch != LINCalcChecksum(st_frame.msg.data, st_frame.msg.dlc)) {
			lin_error_reset();
			st_frame.status = ERROR_CS;
		}
		// copy rx data to lin buffer
		if (circle_number < RX_FIFO_SZ) {
			circle_number ++;
			lin_fifo_rx[circle_header] = st_frame.msg;
			circle_header ++;
			if (circle_header == RX_FIFO_SZ)
				circle_header = 0;
		}
		//init all slave task variables
		lin_error_reset();
		st_frame.status = ERROR_NONE;
		break;
	case LIN_ERROR:
	default:
		break;
	}

}

void USART1_IRQHandler(void)
{
	USART_Cmd(uart, DISABLE);

	if (node_type == LIN_MASTER_NODE)
		master_node_process();
	else
		slave_node_process();

	USART_Cmd(uart, ENABLE);
}

static int lin_sendpid(unsigned char pid)
{
	if (node_type != LIN_MASTER_NODE)
		return 1;
	if (st_frame.state)
		return 1;
	master_pid = pid;
	Disable_RecvInt();
	Enable_BreakInt();
	USART_SendBreak(uart);
	st_frame.state = LIN_BREAK;
	return 0;
}

static int lin_recvmsg(lin_msg_t *pmsg)
{
	if (circle_number == 0) {
		return -1;
	} else {
		(*pmsg) = lin_fifo_rx[circle_tailer];
		circle_number --;
		circle_tailer ++;
		if (circle_tailer == RX_FIFO_SZ)
			circle_tailer = 0;
	}
	return 0;
}

static int lin_ckstate(void)
{
	return st_frame.state;
}

static int lin_ckstatus(void)
{
	int status = st_frame.status;
	st_frame.status = ERROR_NONE;
	return status;
}

static void lin_error_reset(void)
{
	rsp_count = 0;
	Disable_RecvInt();
	Enable_BreakInt();
	st_frame.msg.dlc = 0;
	st_frame.state = LIN_IDLE;
}

static unsigned char LINCalcParity(unsigned char id)
{
	unsigned char parity, P_E, P_O;
	parity=id; 
	P_E = (BIT(parity,0)^BIT(parity,1)^BIT(parity,2)^BIT(parity,4)) << 6;
	P_O = (~(BIT(parity,1)^BIT(parity,3)^BIT(parity,4)^BIT(parity,5))) << 7;
	parity |= (P_E|P_O);
	return parity;
}

static unsigned char LINCalcChecksum(unsigned char *data, int num)
{
	unsigned int sum = 0;
	unsigned char i;
	for (i = 0; i < num; i++) {
		sum += data[i];
		if(sum&0xFF00)  // Adds carry
			sum = (sum&0x00FF) + 1;
	}
	sum ^= 0x00FF;
	return (unsigned char)sum;
}

lin_bus_t lin1 = {
	.init = lin_init,
	.recv = lin_recvmsg,
	.send = lin_sendpid,
	.ckstate = lin_ckstate,
	.ckstatus = lin_ckstatus,
};
