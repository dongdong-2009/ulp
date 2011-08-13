
#include "eth_demo/tcpserver.h"
#include "lwip/tcp.h"
#include "spi.h"
#include "time.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "matrix.h"

#include <string.h>

#define GREETING "welcome to board tcp server! \r\n"
#define HELLO "hello,beautiful girl! \r\n"

static err_t tcpserver_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcpserver_accept(void *arg, struct tcp_pcb *pcb, err_t err);
static void tcpserver_conn_err(void *arg, err_t err);

void lwip_app_Init(void)
{
	struct tcp_pcb *pcb;

	pcb = tcp_new();

	/* Assign to the new pcb a local IP address and a port number */
	/* Using IP_ADDR_ANY allow the pcb to be used by any local interface */
	tcp_bind(pcb, IP_ADDR_ANY, TCP_SERVER_PORT);

	/* Set the connection to the LISTEN state */
	pcb = tcp_listen(pcb);

	/* Specify the function to be called when a connection is established */
	tcp_accept(pcb, tcpserver_accept);

	matrix_init();
}

static err_t tcpserver_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	/* Tell LwIP to associate this structure with this connection. */
	tcp_arg(pcb, mem_malloc(100));

	/* Configure LwIP to use our call back functions. */
	tcp_err(pcb, tcpserver_conn_err);
	tcp_recv(pcb, tcpserver_recv);

	/* Send out the first message */
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
		//tcp_write(pcb, HELLO, strlen(HELLO), 1);

		//tcp_write(pcb, recv, p->tot_len, TCP_WRITE_FLAG_COPY);

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


