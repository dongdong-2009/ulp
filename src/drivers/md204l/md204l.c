/* console.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "md204l.h"
#include "usart.h"

static md204l_req_t md204l_req;
static md204l_ack_t md204l_ack;

void md204l_Init(void)
{
	md204l_req.station = 1;
	usart_Init();
}

int md204l_Write(int addr, short *pbuf, int count)
{
	int i;
	int temp = 0;

	if(addr < 0 || addr >127 || count > 128)
		return 1;
		
	md204l_req.station = 1;
	md204l_req.cmd = MD204L_WRITE;
	md204l_req.addr = (unsigned char)addr;
	md204l_req.len = (unsigned char)count;
	md204l_req.data = pbuf;

	/*calculate check sum*/
	for (i = 0; i < count; i++) {
		temp += pbuf[i] & 0x00ff;
		temp += pbuf[i] >> 8;
	}

	temp += md204l_req.station;
	temp += md204l_req.cmd;
	temp += md204l_req.addr;
	temp += md204l_req.len;

	md204l_req.cksum = (unsigned char)(temp%0x100);

	/*send data by usart*/
	usart_putchar(md204l_req.station);
	usart_putchar(md204l_req.cmd);
	usart_putchar(md204l_req.addr);
	usart_putchar(md204l_req.len);
	for (i = 0; i < count; i++) {
		usart_putchar(pbuf[i] >> 8);
		usart_putchar(pbuf[i] & 0x00ff);
	}
	usart_putchar(md204l_req.cksum);

	/*receive ack data*/
	md204l_ack.station = (unsigned char)usart_getch();
	md204l_ack.status = (unsigned char)usart_getch();
	if(md204l_ack.status != MD204L_OK)
		return 1;

	return 0;
}

int md204l_Read(int addr, short *pbuf, int count)
{
	return 0;
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static int cmd_md204l_func(int argc, char *argv[])
{
	short temp;
	int addr;

	const char usage[] = { \
		" usage:\n" \
		" md204l init,md204l debug " \
		" md204l write regaddr value,regaddr:0-127,value:16bits " \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (strcmp("init",argv[1]) == 0)
		md204l_Init();

	if (strcmp("write",argv[1]) == 0) {
		addr = atoi(argv[2]);
		temp = atoi(argv[3]);
		//sscanf(argv[2], "%x", &addr);
		//sscanf(argv[3], "%x", &temp);
		md204l_Write(addr, &temp, 1);
	}

	return 0;
}
const cmd_t cmd_md204l = {"md204l", cmd_md204l_func, "md204l cmd"};
DECLARE_SHELL_CMD(cmd_md204l)
#endif
