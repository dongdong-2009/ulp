/* console.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "md204l.h"

static int md204l_GetByte(md204l_t *chip);

void md204l_Init(md204l_t *chip)
{
	uart_cfg_t cfg = { //UART_CFG_DEF;
		.baud = 19200,
	};

	chip->bus->init(&cfg);
}

int md204l_Write(md204l_t *chip, int addr, short *pbuf, int count)
{
	int i;
	int temp = 0;
	md204l_req_t md204l_req;
	md204l_ack_t md204l_ack;

	md204l_req.station = chip->station;
	md204l_req.cmd = CMD_MD204L_WRITE;
	md204l_req.addr = (unsigned char)addr;
	md204l_req.len = count;

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
	chip->bus->putchar(md204l_req.station);
	chip->bus->putchar(md204l_req.cmd);
	chip->bus->putchar(md204l_req.addr);
	chip->bus->putchar(md204l_req.len);
	for (i = 0; i < count; i++) {
		chip->bus->putchar(pbuf[i] >> 8);
		chip->bus->putchar(pbuf[i] & 0x00ff);
	}
	chip->bus->putchar(md204l_req.cksum);

	/*receive ack data*/
	md204l_ack.station = (unsigned char)md204l_GetByte(chip);
	md204l_ack.status = (unsigned char)md204l_GetByte(chip);
	md204l_GetByte(chip);
	if (md204l_ack.status != MD204L_OK)
		return 1;

	return 0;
}

int md204l_Read(md204l_t *chip, int addr, short *pbuf, int count)
{
	int i;
	int temp = 0;
	md204l_req_t md204l_req;
	md204l_ack_t md204l_ack;

	md204l_req.station = chip->station;
	md204l_req.cmd = CMD_MD204L_READ;
	md204l_req.addr = (unsigned char)addr;
	md204l_req.len = count;

	temp += md204l_req.station;
	temp += md204l_req.cmd;
	temp += md204l_req.addr;
	temp += md204l_req.len;

	md204l_req.cksum = (unsigned char)(temp % 0x100);

	/*send data by usart*/
	chip->bus->putchar(md204l_req.station);
	chip->bus->putchar(md204l_req.cmd);
	chip->bus->putchar(md204l_req.addr);
	chip->bus->putchar(md204l_req.len);
	chip->bus->putchar(md204l_req.cksum);
	/*receive ack data*/
	md204l_ack.station = (unsigned char)md204l_GetByte(chip);
	md204l_ack.status = (unsigned char)md204l_GetByte(chip);
	md204l_ack.addr  = (unsigned char)md204l_GetByte(chip);
	md204l_ack.len = (unsigned char)md204l_GetByte(chip);
	for (i = 0; i < count; i++) {
		pbuf[i] = (short)(md204l_GetByte(chip) << 8);
		pbuf[i] |= (short)(md204l_GetByte(chip) & 0x00ff);
	}
	md204l_GetByte(chip);

	if ((md204l_ack.status != MD204L_OK) || (md204l_ack.station != chip->station))
		return 1;
	return 0;
}

void md204l_Update(void)
{

}

static int md204l_GetByte(md204l_t *chip)
{
	chip->deadtime = time_get(25);
	while(time_left(chip->deadtime) > 0) {
		if(chip->bus->poll())
			return chip->bus->getchar();
	}
	return -1;
}

#if 0
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static md204l_t dbg_hid_md204l = {
	.bus = &uart2,
	.station = 0x01,
};


static int cmd_md204l_func(int argc, char *argv[])
{
	short temp = 0;
	int addr;

	const char *usage = { \
		" usage:\n" \
		" md204l init,md204l debug \n" \
		" md204l write regaddr value, regaddr:0-127,value:16bits \n" \
		" md204l read regaddr, regaddr:0-127,value:16bits \n" \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (argv[1][0] == 'i') {
		md204l_Init(&dbg_hid_md204l);
		printf("init ok \n");
	}

	if (argv[1][0] == 'w') {
		addr = atoi(argv[2]);
		temp = atoi(argv[3]);
		if (md204l_Write(&dbg_hid_md204l, addr, &temp, 1))
			printf("Write error\n");
		else
			printf("Write sucessfully\n");
	}
	if (argv[1][0] == 'r') {
		temp = 0;
		addr = atoi(argv[2]);
		if (md204l_Read(&dbg_hid_md204l, addr, &temp, 1))
			printf("Read error!\n");
		else
			printf("data is: %d\n\r",temp);
	}

	return 0;
}
const cmd_t cmd_md204l = {"md204l", cmd_md204l_func, "md204l cmd"};
DECLARE_SHELL_CMD(cmd_md204l)
#endif
