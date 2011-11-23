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
#include "ulp_time.h"
#include "wl.h"

static const spi_bus_t *nrf_spi;
static char nrf_idx_cs;
static char nrf_idx_ce;
static char nrf_prx_mode;

#define cs_set(level) nrf_spi -> csel(nrf_idx_cs, level)
#define ce_set(level) nrf_spi -> csel(nrf_idx_ce, level)
#define spi_write(val) nrf_spi -> wreg(0, val)
#define spi_read() nrf_spi -> rreg(0)

static int nrf_write_buf(int cmd, const char *buf, int count);
static int nrf_read_buf(int cmd, char *buf, int count);
static void nrf_write_reg(char reg, char value);
static char nrf_read_reg(char reg);
static int nrf_set_addr(int addr);
static int nrf_set_mode_tx(void);
static int nrf_set_mode_rx(void);

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

static int nrf_init(const wl_cfg_t *cfg)
{
	int ch = cfg -> mhz - 24000;
	spi_cfg_t spi_cfg = SPI_CFG_DEF;
	spi_cfg.cpol = 0,
	spi_cfg.cpha = 0,
	spi_cfg.bits = 8,
	spi_cfg.bseq = 1,
	spi_cfg.freq = 8000000;
	spi_cfg.csel = 1;

#if CONFIG_TASK_NEST == 1
	nrf_spi = &spi1;
	nrf_idx_cs = SPI_1_NSS;
	nrf_idx_ce = SPI_CS_PA12;
#else
	nrf_spi = NULL;
#endif

	assert(nrf_spi != NULL);
	nrf_spi -> init(&spi_cfg);
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
	nrf_write_reg(RF_CH, ch); //rf channel = 0
	if(cfg -> bps > 1000000) {
		nrf_write_reg(RF_SETUP, 0x0f); //2Mbps + 0dBm + LNA enable
	}
	else {
		nrf_write_reg(RF_SETUP, 0x07); //1Mbps + 0dBm + LNA enable
	}

	char para = 0x73;
	nrf_write_buf(ACTIVATE, &para, 1); //activate ext function
	nrf_write_reg(DYNPD, 0x01); //enable dynamic payload len of pipe 0
	nrf_write_reg(FEATURE, 0x07); //enable dpl | Ack with Payload | NACK cmd

	nrf_write_buf(FLUSH_TX, 0, 0);
	nrf_write_buf(FLUSH_RX, 0, 0);

	nrf_prx_mode = 99; //any val except 1 & 0
	nrf_set_addr(0);
	mdelay(2); //power on delay
	return 0;
}

static int nrf_set_mode_tx(void)
{
	if(nrf_prx_mode == 0)
		return 0;

	ce_set(0);
	nrf_prx_mode = 0;
	nrf_write_reg(CONFIG, 0x0e); //enable 2 bytes CRC | PWR_UP | mode
	nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
	nrf_write_reg(STATUS, TX_DS | MAX_RT); //clear tx_ds & max_rt irq flag
	return 0;
}

static int nrf_set_mode_rx(void)
{
	if(nrf_prx_mode == 1)
		return 0;

	ce_set(0);
	nrf_prx_mode = 1;
	nrf_write_reg(CONFIG, 0x0f); //enable 2 bytes CRC | PWR_UP | mode
	nrf_write_buf(FLUSH_RX, 0, 0); //flush rx fifo
	nrf_write_reg(STATUS, RX_DR); //clear rx_dr irq flag
	ce_set(1); //prx automatic enter into recv mode
	return 0;
}

static int nrf_set_addr(int addr)
{
	ce_set(0);
	nrf_write_buf(W_REGISTER(TX_ADDR), (char *)(&addr), 4);
	nrf_write_buf(W_REGISTER(RX_ADDR_P0), (char *)(&addr), 4);
	nrf_write_reg(EN_RXADDR, 1);
	nrf_set_mode_tx();
	return nrf_set_mode_rx(); //automatically enter into prx mode
}

/*
	"send - wait - return" method is used, so tx fifo is useless, advantage: simple,  shortcoming: wait loop,
	in system level 'master poll' topology, this won't take longer than 1 ms in normal situation
	buf	data to be sent
	count	n bytes to send()
	ms	timeout, 3ms is recommended
*/
static int nrf_send(const char *buf, int count, int ms)
{
	int status, n, i;
	time_t deadline = time_get(ms);

#if 0 /*ack with payload won't be supported*/
	//always flush tx fifo, if ptx do not get what he want, he should resend the request
	nrf_write_buf(FLUSH_TX, 0, 0);
	nrf_write_buf(W_ACK_PAYLOAD(0), buf, count);
#endif

	nrf_set_mode_tx();
	for(i = 0, n = 0; i < count;) {
		if(n == 0) {
			//write to tx fifo, max 32 bytes
			n = (count - i > 32) ? 32 : (count - i);
			nrf_write_buf(W_TX_PAYLOAD, buf + i, n);

			//start transmit ..
			deadline = time_get(ms);
			ce_set(1);
		}

		if(time_left(deadline) <= 0) { //timeout
			ce_set(0);
			//flush tx fifo
			nrf_write_buf(FLUSH_TX, 0, 0);
			break;
		}
		
		status = nrf_read_reg(STATUS);
		if(status & TX_DS) {
			ce_set(0);
			nrf_write_reg(STATUS, TX_DS);
			i += n;
			n = 0; //indicates: new packet can be sent in next loop
			continue;
		}
		else if(status & MAX_RT) {
			ce_set(0);
			nrf_write_reg(STATUS, MAX_RT);

			//resend
			ce_set(1);
			continue;
		}
	}

	return i;
}

static int nrf_poll(void)
{
	int status;
	nrf_set_mode_rx();
	status = nrf_read_reg(FIFO_STATUS);
	return (status & RX_EMPTY) ? 0 : 1;
}

/*
	recv from nrf chip fifo
*/
static int nrf_recv(char *buf, int count, int ms)
{
	char n;
	int i;
	time_t deadline = time_get(ms);

	i = 0;
	n = 0;
	nrf_set_mode_rx();
	do {
		if(!nrf_poll()) {
			continue;
		}

		deadline = time_get(ms);
		nrf_read_buf(R_RX_PL_WID, &n, 1);
		if(n > 0) {
			if(i + n > count)
				break;
			//recv
			nrf_read_buf(R_RX_PAYLOAD, buf + i, n);
			i += n;
		}
	} while(time_left(deadline) > 0);

	//clear rx_dr isr flag
	nrf_write_reg(STATUS, RX_DR);
	return i;
}

const wl_bus_t wl0 = {
	.init = nrf_init,
	.send = nrf_send,
	.recv = nrf_recv,
	.poll = nrf_poll,
	.select = nrf_set_addr,
};
