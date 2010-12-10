/*
 * 	miaofng@2010 initial version
  *		nrf24l01, 2.4g wireless gfsk transceiver
 *		interface: spi, 8Mhz, 8bit, CPOL=0(sck low idle), CPHA=0(strobe at 1st edge of sck), msb first
 *		-tx fifo: 32bytes x 3, rx fifo 32bytes x 3
 *	- always enable Auto ACK
 *	- always enable Dynamic Payload Length(through command R_RX_PL_WID)
*	- always setup addr length to 4 bytes
*	- always use pipe 0 for tx&rx
 */

#include "config.h"
#include "spi.h"
#include "nrf.h"
#include "debug.h"
#include "time.h"
#include "nvm.h"

static const spi_bus_t *nrf_spi;
static char nrf_idx_cs;
static char nrf_idx_ce;
static char nrf_prx_mode;
static char nrf_ch __nvm = 0;

#define cs_set(level) nrf_spi -> csel(nrf_idx_cs, level)
#define ce_set(level) nrf_spi -> csel(nrf_idx_ce, level)
#define spi_write(val) nrf_spi -> wreg(0, val)
#define spi_read() nrf_spi -> rreg(0)

static int nrf_write_buf(int cmd, const char *buf, int count)
{
	int status;
	cs_set(0);
	status = spi_write(cmd);
	while(count > 0) {
		spi_write(*buf ++);
		count --;
	}
	cs_set(1);
	return status;
}

static int nrf_read_buf(int cmd, char *buf, int count)
{
	int status;
	cs_set(0);
	status = spi_write(cmd);
	while(count > 0) {
		*buf ++ = spi_read();
		count --;
	}
	cs_set(1);


        return status;
}

static void nrf_write_reg(char reg, char value)
{
	nrf_write_buf(W_REGISTER(reg), &value, 1);
}

static char nrf_read_reg(char reg)
{
	char value;
	nrf_read_buf(R_REGISTER(reg), &value, 1);
	return value & 0xff;
}

int nrf_init(void)
{
	spi_cfg_t cfg = SPI_CFG_DEF;
	cfg.cpol = 0,
	cfg.cpha = 0,
	cfg.bits = 8,
	cfg.bseq = 1,
	cfg.freq = 8000000;
	cfg.csel = 1;

#if CONFIG_TASK_NEST == 1
	nrf_spi = &spi1;
	nrf_idx_cs = SPI_1_NSS;
	nrf_idx_ce = SPI_CS_PA12;
#else
	nrf_spi = NULL;
#endif

	assert(nrf_spi != NULL);
	nrf_spi -> init(&cfg);
	cs_set(1);
	ce_set(0);

#if 1
	nrf_write_reg(TX_ADDR, 0x11);
	char val = nrf_read_reg(TX_ADDR);
	assert(val == 0x11);
#endif
	nrf_write_reg(EN_AA, 0x01); //enable auto ack of pipe 0
	nrf_write_reg(EN_RXADDR, 0x00); //disable all rx pipe
	nrf_write_reg(SETUP_AW, 0x02); //addr width = 4bytes
	nrf_write_reg(SETUP_RETR, 0x0f); //auto retx delay = 250usx(n + 1), count = 3
	nrf_write_reg(RF_CH, nrf_ch); //rf channel = 0
	nrf_write_reg(RF_SETUP, 0x0f); //2Mbps + 0dBm + LNA enable

	char para = 0x73;
	nrf_write_buf(ACTIVATE, &para, 1); //activate ext function
	nrf_write_reg(DYNPD, 0x01); //enable dynamic payload len of pipe 0
	nrf_write_reg(FEATURE, 0x07); //enable dpl | Ack with Payload | NACK cmd

	nrf_write_buf(FLUSH_TX, 0, 0);
	nrf_write_buf(FLUSH_RX, 0, 0);

	//default enter into prx mode
	nrf_setup(0, 0);
	mdelay(2);
	return 0;
}

int nrf_setup(int prx_mode, int addr)
{
	ce_set(0);
	nrf_prx_mode = prx_mode;
	nrf_write_buf(W_REGISTER(TX_ADDR), (char *)(&addr), 4);
	nrf_write_buf(W_REGISTER(RX_ADDR_P0), (char *)(&addr), 4);
	nrf_write_reg(EN_RXADDR, 1);
	nrf_write_reg(CONFIG, 0x0e | prx_mode); //enable 2 bytes CRC | PWR_UP | mode
	ce_set(prx_mode == 1); //prx automatic enter into recv mode
	if(prx_mode == 1) {
		ce_set(1);
	}
	else {
		nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
		//nrf_write_buf(FLUSH_RX, 0, 0);
		nrf_write_reg(STATUS, TX_DS | MAX_RT);
	}
	return 0;
}

int nrf_send(const char *buf, int count)
{
	int status;

	if(nrf_prx_mode) {
		//always flush tx fifo, if ptx do not get what he want, he should resend the request
		nrf_write_buf(FLUSH_TX, 0, 0);
		nrf_write_buf(W_ACK_PAYLOAD(0), buf, count);
	}
	else {//ptx: send - wait - return
		nrf_write_buf(W_TX_PAYLOAD, buf, count);
		ce_set(1);
		while(1) {
			status = nrf_read_reg(STATUS);
			if(status & TX_DS) {
				nrf_write_reg(STATUS, TX_DS);
				break;
			}

			if(status & MAX_RT) {
				nrf_write_reg(STATUS, MAX_RT);
				//flush tx fifo
				nrf_write_buf(FLUSH_TX, 0, 0);
				count = 0;
				break;
			}
		}
		ce_set(0);
	}

	return count;
}

int nrf_recv(char *buf, int size)
{
	int status;
	char count;

	status = nrf_read_reg(FIFO_STATUS);
	if(status & RX_EMPTY) { //rx empty
		return 0;
	}
	//nrf_write_reg(STATUS, RX_DR);

	nrf_read_buf(R_RX_PL_WID, &count, 1);
	if(count > 0) {
		nrf_read_buf(R_RX_PAYLOAD, buf, count);
	}
	return count;
}

#include "shell/cmd.h"
#include "console.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int cmd_nrf_chat(void)
{
	char c, buf[40],ibuf[40], j, i_save = 0, i = 0;
	int count;
	char arc;

	nrf_init();
	nrf_setup(1, 0x12345678);
	while(1) {
		//read  a line from console & send
		while(console_IsNotEmpty()) {
			c = console_getch();
			if((i < 31) && isprint(c)) {
				console_putchar(c);
				buf[i] = c;
				i ++;
			}

			if(c == 3) { //exit
				return 0;
			}

			if(c != '\r') {
				continue;
			}

			//send
			nrf_setup(0, 0x12345678);
			if(i != 0) {
				buf[i] = 0;
				i ++;
			}
			else {
				i = i_save;
			}
			count = nrf_send(buf, i);
			arc = nrf_read_reg(OBSERVE_TX);
			printf(" send %d bytes...try %d times...", i, arc & 0x0f);
			if(count != i) {
				printf("fail!!!\n");
			}
			else {
				count = nrf_recv(ibuf, 32);
				ibuf[count] = 0;
				printf("echo %d bytes: %s\n", count, ibuf);
			}
			nrf_setup(1, 0x12345678);

			//recv next line
			i_save = i;
			i = 0;
		}


		//recv a line from nrf
		count = nrf_recv(ibuf, 32);
		if(count != 0) {
			j = count - 1;
			if(j < 30) {
				ibuf[j ++] = ':';
				ibuf[j ++] = ')';
				count += 2;
			}
			ibuf[j] = 0;

			//echo back to nrf
			nrf_send(ibuf, count);
			printf("rx %d bytes: %s\n", count - 2, ibuf);
		}
	}
}

static int cmd_nrf_scan(void)
{
	int ch, v;
	char cd;

	printf("\n");
	nrf_init();
	while(1) {
		if(console_IsNotEmpty()) {
			console_getch();
			break;
		}

		v = 0;
		printf("\r CD[0..127] =");
		for(ch = 0; ch < 128; ch ++) {
			nrf_write_reg(RF_CH, ch);
			ce_set(1);
			mdelay(1);
			cd = nrf_read_reg(CD);
			ce_set(0);

			if(cd & 0x01) {
				v |= (1U << (ch & 0x1f));
			}

			if((ch & 0x1f) == 0x1f) {
				printf(" %08x", v);
				v = 0;
			}
		}
	}

	printf("\n");
	return 0;
}

static int cmd_nrf_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"nrf chat	start nrf chat\n"
		"nrf scan	scanf all RF channel\n"
		"nrf ch 0-127	set RF channel(2400MHz + ch * 1Mhz)\n"
	};

	if(argc > 1) {
		if(!strcmp(argv[1], "ch")) {
			int ch = (argc > 2) ? atoi(argv[2]) : 0;
			nrf_ch = ((ch >= 0) && (ch <= 127)) ? ch : 0;
			printf("nrf: setting RF ch = %d(%dMHz)\n", nrf_ch, nrf_ch + 2400);
			nrf_init();
			nvm_save();
			return 0;
		}
		if(!strcmp(argv[1], "chat")) {
			return cmd_nrf_chat();
		}
		if(!strcmp(argv[1], "scan")) {
			return cmd_nrf_scan();
		}
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_nrf = {"nrf", cmd_nrf_func, "nrf debug command"};
DECLARE_SHELL_CMD(cmd_nrf)
