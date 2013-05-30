/*
 * cong.chen@2011 initial version
 * David@2011 improved
 */
#include "lwip/tcp.h"
#include "spi.h"
#include "ulp_time.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "matrix.h"

#include <string.h>

#include "config.h"
#include "ulp/sys.h"
#include "stm32f10x.h"

extern void lwip_lib_isr(void);
void EXTI4_IRQHandler(void)
{
	lwip_lib_isr();
	EXTI_ClearFlag(EXTI_Line4);
}

int main(void)
{
	sys_init();
	matrix_init();

	/*enable ethernet irq*/
	GPIO_InitTypeDef  GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode= GPIO_Mode_IN_FLOATING ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	while(1) {
		sys_update();
	}
}

#define GREETING "welcome to Matrix Board tcp server! \r\n"

static err_t tcpserver_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcpserver_accept(void *arg, struct tcp_pcb *pcb, err_t err);
static void tcpserver_conn_err(void *arg, err_t err);

void lwip_app_Init(void)
{
	struct tcp_pcb *pcb;

	pcb = tcp_new();

	/* Assign to the new pcb a local IP address and a port number */
	/* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
	tcp_bind(pcb, IP_ADDR_ANY, 3838);

	/* Set the connection to the LISTEN state */
	pcb = tcp_listen(pcb);

	/* Specify the function to be called when a connection is established */
	tcp_accept(pcb, tcpserver_accept);
}

static err_t tcpserver_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	/* Tell LwIP to associate this structure with this connection. */
	tcp_arg(pcb, mem_malloc(100));

	/* Configure LwIP to use our call back functions. */
	tcp_err(pcb, tcpserver_conn_err);
	tcp_recv(pcb, tcpserver_recv);

	//Send out the first message, greeting message
	tcp_write(pcb, GREETING, strlen(GREETING), 1);

	return ERR_OK;
}

static void tcpserver_conn_err(void *arg, err_t err)
{
	char *data;
	data = (char *)arg;

	mem_free(data);
}

static err_t tcpserver_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct pbuf *q;
	char *c, *recv = (char *)arg;
	int i, j = 0;
	unsigned char cmd ;
	/* We perform here any necessary processing on the pbuf */
	if (p != NULL) {
		/* We call this function to tell the LwIp that we have processed the data */
		/* This lets the stack advertise a larger window, so more data can be received*/
		tcp_recved(pcb, p->tot_len);

		/* Check the name if NULL, no data passed, return withh illegal argument error */
		if(!recv) {
			pbuf_free(p);
			return ERR_ARG;
		}

		for(q=p; q != NULL; q = q->next) {
			c = q->payload;
			for(i=0; i<q->len; i++) {
				recv[j++] = c[i];
			}
		}
		cmd = recv[0];
		matrix_handler(cmd,recv);

		/* End of processing, we free the pbuf */
		pbuf_free(p);
	} else if (err == ERR_OK) {
		/* When the pbuf is NULL and the err is ERR_OK, the remote end is closing the connection. */
		/* We free the allocated memory and we close the connection */
		mem_free(recv);
		return tcp_close(pcb);
	}

	return ERR_OK;
}


