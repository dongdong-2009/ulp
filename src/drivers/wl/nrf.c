/*
 * 	miaofng@2010 initial version
  *		nrf24l01, 2.4g wireless gfsk transceiver
 *		interface: spi, 8Mhz, 8bit, CPOL=0(sck low idle), CPHA=0(strobe at 1st edge of sck), msb first
 *		-tx fifo: 32bytes x 3, rx fifo 32bytes x 3
 *	miaofng@2011 almost completely rewritten all
*		-AA with payload is selected as main communication method
*		-modify for new driver architecture
 *	- always enable Auto ACK with Payload
 *	- always enable Dynamic Payload Length(through command R_RX_PL_WID)
*	- always setup addr length to 4 bytes
*	- always use pipe 0 for tx&rx
*	- to support multi communication, each machine need an phy address to avoid ack issue
*	- byte0 of payload is used as frame type, such as "D" for normal data frame
*	- one chip can handle multiple rx pipe, but only pipe0 support tx/rx dual direction, multi pipe and dynamic create not supported yet
*	- ptx always sent a frame at each time update, in order to read the echo from prx
*	- prx ack without payload if there's no data to tx
 */

#include "config.h"
#include "spi.h"
#include "nrf.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "ulp/wl.h"
#include "ulp/device.h"
#include "ulp/types.h"
#include "common/circbuf.h"
#include "linux/list.h"
#include "sys/malloc.h"
#include <string.h>

struct nrf_pipe_s {
	char index; //0-5
	char timeout; // unit: ms
	char cf_timeout; //custom frame send timeout
	char cf_ecode; //custom frame err code
	time_t timer; //send timer for both ptx and prx mode, it counts the ms elapsed that hw tx fifo keeps full
	unsigned addr; //note: only low 8 bit effective in case not pipe0
	circbuf_t tbuf; //mv to hw fifo by poll()
	circbuf_t rbuf; //mv from hw fifo by poll()
	struct list_head list;
	int (*onfail)(int ecode, ...); //exception process func
	char *cf; //custom frame to be sent, will be modified to NULL when sent
};

struct nrf_chip_s {
	const spi_bus_t *spi;
	char gpio_cs;
	char gpio_ce;
	char gpio_irq;
	char mode;
	int freq;
	struct list_head pipes;
};

/*ulp device driver architecture*/
struct nrf_priv_s  {
	struct nrf_chip_s *chip;
	struct nrf_pipe_s *pipe; //point to current pipe
};

#define cs_set(level) chip->spi->csel(chip->gpio_cs, level)
#define ce_set(level) chip->spi->csel(chip->gpio_ce, level)
#define spi_write(val) chip->spi->wreg(0, val)
#define spi_read() chip->spi->rreg(0)

#define nrf_write_buf(cmd, buf, cnt) __nrf_write_buf(chip, cmd, buf, cnt)
static int __nrf_write_buf(const struct nrf_chip_s *chip, int cmd, const char *buf, int count)
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

#define nrf_read_buf(cmd, buf, cnt) __nrf_read_buf(chip, cmd, buf, cnt)
static int __nrf_read_buf(const struct nrf_chip_s *chip, int cmd, char *buf, int count)
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

#define nrf_write_reg(reg, val) __nrf_write_reg(chip, reg, val)
static void __nrf_write_reg(const struct nrf_chip_s *chip, char reg, char value)
{
	cs_set(0);
	spi_write(W_REGISTER(reg));
	spi_write(value);
	cs_set(1);
}

#define nrf_read_reg(reg) __nrf_read_reg(chip, reg)
static char __nrf_read_reg(const struct nrf_chip_s *chip, char reg)
{
	cs_set(0);
	spi_write(R_REGISTER(reg));
	char value = spi_read();
	cs_set(1);
	return value;
}

static int nrf_hw_set_freq(const struct nrf_chip_s *chip, int mhz)
{
	int ch = mhz - 2400;
	nrf_write_reg(RF_CH, ch); //rf channel = 0
	return 0;
}

static int nrf_hw_set_addr(const struct nrf_chip_s *chip, int index, int addr)
{
	nrf_write_buf(W_REGISTER(TX_ADDR), (char *)(&addr), 4);
	nrf_write_buf(W_REGISTER(RX_ADDR_P0), (char *)(&addr), 4);
	nrf_write_reg(EN_RXADDR, 1);
	return 0;
}

static int nrf_hw_flush(const struct nrf_chip_s *chip)
{
	//note: 3 fifos for each one
	nrf_write_buf(FLUSH_RX, 0, 0); //flush rx fifo
	nrf_write_buf(FLUSH_RX, 0, 0); //flush rx fifo
	nrf_write_buf(FLUSH_RX, 0, 0); //flush rx fifo
	nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
	nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
	nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
	return 0;
}

static int nrf_hw_set_mode(const struct nrf_chip_s *chip, int mode)
{
	char cfg, ard;
	cfg = (mode & WL_MODE_PRX) ? 0x0f : 0x0e;
	nrf_write_reg(CONFIG, cfg);  //enable 2 bytes CRC | PWR_UP | mode

	cfg = 0x0f; //2Mbps + 0dBm + LNA enable
	ard = 0x0f; //auto retx delay = 250uS, count = 15
	if(mode & WL_MODE_1MBPS) {
		cfg = 0x07; //1Mbps + 0dBm + LNA enable
		ard = 0x1f; //auto retx delay = 500uS, count = 15
		nrf_write_reg(RF_SETUP, cfg);
		nrf_write_reg(SETUP_RETR, ard);
	}
	return 0;
}

static int nrf_hw_init(struct nrf_priv_s *priv)
{
	struct nrf_pipe_s *pipe;
	struct nrf_chip_s *chip;
	pipe = priv->pipe;
	assert(pipe != NULL);
	chip = priv->chip;
	assert(chip != NULL);

	//spi bus init
	spi_cfg_t spi_cfg = SPI_CFG_DEF;
	spi_cfg.cpol = 0,
	spi_cfg.cpha = 0,
	spi_cfg.bits = 8,
	spi_cfg.bseq = 1,
	spi_cfg.freq = 8000000;
	spi_cfg.csel = 1;
	chip->spi->init(&spi_cfg);
	cs_set(1);
	ce_set(0);

	//SELF DIAG
	nrf_write_reg(TX_ADDR, 0x11);
	char val = nrf_read_reg(TX_ADDR);
	assert(val == 0x11);

	//setup pipe
	nrf_write_reg(EN_AA, 0x01); //enable auto ack of pipe 0
	nrf_write_reg(EN_RXADDR, 0x00); //disable all rx pipe
	nrf_write_reg(SETUP_AW, 0x02); //addr width = 4bytes
	nrf_write_reg(SETUP_RETR, 0x0f); //auto retx delay = 250uS, count = 15

	char para = 0x73;
	nrf_write_buf(ACTIVATE, &para, 1); //activate ext function
	nrf_write_reg(DYNPD, 0x01); //enable dynamic payload len of pipe 0
	nrf_write_reg(FEATURE, 0x07); //enable dpl | Ack with Payload | NACK cmd

	nrf_hw_set_freq(chip, chip->freq);
	nrf_hw_set_mode(chip, chip->mode);
	nrf_hw_set_addr(chip, pipe->index, pipe->addr);

	nrf_write_reg(STATUS, RX_DR | TX_DS | MAX_RT ); //clear rx_dr irq flag
	return 0;
}

int nrf_flush(struct nrf_priv_s *priv)
{
	struct nrf_pipe_s *pipe;
	struct nrf_chip_s *chip;
	assert(priv != NULL);
	pipe = priv->pipe;
	assert(pipe != NULL);
	chip = priv->chip;
	assert(chip != NULL);

	buf_init(&pipe->tbuf, -1);
	buf_init(&pipe->rbuf, -1);
	nrf_hw_flush(chip);
	return 0;
}

int nrf_update(struct nrf_priv_s *priv)
{
	struct nrf_pipe_s *pipe;
	struct nrf_chip_s *chip;
	char status, fifo_status, n, frame[32];
	int (*onfail)(int ecode, ...);
	int rbuf_full = 0, ecode = WL_ERR_OK;
	//assert(priv != NULL);
	pipe = priv->pipe;
	//assert(pipe != NULL);
	chip = priv->chip;
	//assert(chip != NULL);
	onfail = pipe->onfail;


	/* STATUS REG:  - | RX_DR | TX_DS | MAX_RT | RX_P_NO 3.. 1 | TX_FULL
	The RX_DR IRQ is asserted by a new packet arrival event. The procedure for handling this interrupt
	should be: 1) read payload through SPI, 2) clear RX_DR IRQ, 3) read FIFO_STATUS to check if there
	are more payloads available in RX FIFO, 4) if there are more data in RX FIFO, repeat from 1)
	*/
	status = nrf_read_reg(STATUS);
	fifo_status = nrf_read_reg(FIFO_STATUS);

	/*handle recv
	prx: there's no enough space in rbuf, do not recv,
	then prx will ignore,  which lead to ptx send fail and resend,
	so there is two reason which may lead to ptx send fail:
	1, prx lost link
	2, prx rfifo full(maybe cpu is dead?:))

	ptx: handle recv, from the ptx flow chart, the payload attached with ack may lost in case of ptx rxfifo is full???
	because it doesn' check rxfifo is full or not, pls refer to chart page 31. From shockburst communication protocol point of view,
	ptx cann't notice prx 'i'm full', that may lead to deadloop. So we need to check the rfifo to ensure we can recv before ptx send
	the frame

	ptx: there's no enough space in ptx rbuf, do not recv,
	then ptx should give up send a frame at next step. */

	if(status & RX_DR) {
		do {
			//handle recv
			nrf_read_buf(R_RX_PL_WID, &n, 1);
			if(n > 0) {
				if(buf_left(&pipe->rbuf) < n - 1) {
					rbuf_full = 1;
					break;
				}
				nrf_read_buf(R_RX_PAYLOAD, frame, n);
				nrf_write_reg(STATUS, RX_DR);
				if(frame[0] == WL_FRAME_DATA) {
					if(n > 1)
						buf_push(&pipe->rbuf, frame + 1, n - 1);
				}
				else {
					ecode = WL_ERR_RX_FRAME;
					onfail(ecode, frame, n);
				}
			}
			else {
				ecode = WL_ERR_RX_HW;
				onfail(ecode);
			}
		fifo_status = nrf_read_reg(FIFO_STATUS);
		} while(!(fifo_status & RX_FIFO_EMPTY));
	}

	//send
	fifo_status = nrf_read_reg(FIFO_STATUS);
	if(1) { //if(status & (TX_DS|MAX_RT)) {
		do {
			if(status & MAX_RT) {
				//resend until timeout
				ce_set(0);
				//delay at least 10uS here ...
				nrf_write_reg(STATUS, MAX_RT);
				ce_set(1);
			}

			if(fifo_status & TX_FIFO_FULL) {
				if(pipe->timer == 0)
					pipe->timer = time_get(pipe->timeout);
				if(time_left(pipe->timer) < 0) { //timeout, flush?
					ecode = WL_ERR_TX_TIMEOUT;
					onfail(ecode, pipe->addr);
					pipe->timer = 0;
				}
				break;
			}
			pipe->timer = 0;
			if(pipe->cf != NULL)
				break;

			if(chip->mode & WL_MODE_PRX) {
				frame[0] = WL_FRAME_DATA;
				n = (chip->mode & WL_MODE_1MBPS) ? 31 : 14;
				n = buf_pop(&pipe->tbuf, frame + 1, n);
				if(n > 0) {
					nrf_write_buf(W_ACK_PAYLOAD(0), frame, n + 1); //nrf count the bytes automatically
					nrf_write_reg(STATUS, TX_DS);
				}
			}
			else {
				if(!rbuf_full) { //no space to recv now
					frame[0] = WL_FRAME_DATA;
					n = buf_pop(&pipe->tbuf, frame + 1, 31); //!!! alway send a frame event n == 0
					nrf_write_buf(W_TX_PAYLOAD, frame, n + 1); //nrf count the bytes automatically
					nrf_write_reg(STATUS, TX_DS);
				}
			}

			//no more data to send
			if(buf_size(&pipe->tbuf) == 0)
				break;

			fifo_status = nrf_read_reg(FIFO_STATUS);
		} while(!(fifo_status & TX_FIFO_FULL));
	}

	//prx send custom frame
	if(pipe->cf != NULL && chip->mode == WL_MODE_PRX) {
		if(fifo_status & TX_FIFO_EMPTY) {
			n = pipe->cf[0];
			memcpy(frame, pipe->cf, n);
			nrf_write_buf(W_ACK_PAYLOAD(0), frame, n);
			nrf_write_reg(STATUS, TX_DS);

			//wait until send out or max rt
			pipe->timer = time_get(pipe->cf_timeout);
			while(1) {
				status = nrf_read_reg(STATUS);
				if(status & TX_DS) { //success
					nrf_write_reg(STATUS, TX_DS);
					pipe->cf = NULL;
					pipe->cf_ecode = 0;
					break;
				}

				if(time_left(pipe->timer) < 0) { //send timeout
					nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
					pipe->timer = 0;
					pipe->cf = NULL;
					pipe->cf_ecode = WL_ERR_TX_TIMEOUT;
					break;
				}
			}
			pipe->timer = 0;
		}
		return 0;
	}

	//ptx send custom frame
	if(pipe->cf != NULL && chip->mode == WL_MODE_PTX) {
		if(fifo_status & TX_FIFO_EMPTY) {
			n = pipe->cf[0];
			memcpy(frame, pipe->cf + 1, n);
			nrf_write_buf(W_TX_PAYLOAD, frame, n); //nrf count the bytes automatically
			nrf_write_reg(STATUS, TX_DS);
			//wait until send out or max rt
			pipe->timer = time_get(pipe->cf_timeout);
			while(1) {
				status = nrf_read_reg(STATUS);
				if(status & TX_DS) { //success
					nrf_write_reg(STATUS, TX_DS);
					pipe->cf = NULL;
					pipe->cf_ecode = 0;
					break;
				}
				if(status & MAX_RT) { //fail, resend until timeout
					if(time_left(pipe->timer) > 0) {
						ce_set(0);
						//delay at least 10uS here ...
						nrf_write_reg(STATUS, MAX_RT);
						ce_set(1);
					}
					else { //send timeout
						nrf_write_reg(STATUS, MAX_RT);
						nrf_write_buf(FLUSH_TX, 0, 0); //flush tx fifo(prx ack payload)
						pipe->timer = 0;
						pipe->cf = NULL;
						pipe->cf_ecode = WL_ERR_TX_TIMEOUT;
						break;
					}
				}
			}
			pipe->timer = 0;
		}
		return 0;
	}

	return 0;
}

int nrf_onfail(int ecode, ...)
{
	return 0;
}

static int nrf_init(int fd, const void *pcfg)
{
	struct nrf_priv_s *priv;
	struct nrf_pipe_s *pipe;
	struct nrf_chip_s *chip;
	const struct nrf_cfg_s *cfg = pcfg;
	char *pmem;

	//!!!pls take care of alignment issue
	pmem = sys_malloc(sizeof(struct nrf_priv_s) + sizeof(struct nrf_pipe_s) \
		+ sizeof(struct nrf_chip_s));
	assert(pmem != NULL);
	priv = (struct nrf_priv_s *)pmem;
	pipe = (struct nrf_pipe_s *)(pmem + sizeof(struct nrf_priv_s));
	chip = (struct nrf_chip_s *)(pmem + sizeof(struct nrf_priv_s) + sizeof(struct nrf_pipe_s));

	//init pipe0
	pipe->index = 0;
	pipe->timeout = CONFIG_WL_MS; //5ms
	pipe->timer = 0;
	pipe->addr = CONFIG_WL_ADDR;
	buf_init(&pipe->tbuf, CONFIG_WL_TBUF_SZ);
	buf_init(&pipe->rbuf, CONFIG_WL_RBUF_SZ);
	INIT_LIST_HEAD(&pipe->list);

	//init chip
	chip->spi = cfg->spi;
	chip->gpio_cs = cfg->gpio_cs;
	chip->gpio_ce = cfg->gpio_ce;
	chip->gpio_irq = cfg->gpio_irq;
	chip->mode = WL_MODE_PRX; //default to prx mode
	chip->freq = CONFIG_WL_MHZ; //unit: MHz
	INIT_LIST_HEAD(&chip->pipes);
	list_add_tail(&pipe->list, &chip->pipes);
	pipe->onfail = nrf_onfail; //default onfail func
	pipe->cf = NULL;

	//priv
	priv->chip = chip;
	priv->pipe = pipe;
	dev_priv_set(fd, priv);
	return dev_class_register("wl", fd);
}

static int nrf_open(int fd, int mode)
{
	struct nrf_priv_s *priv = dev_priv_get(fd);
	int ret;
	assert(priv != NULL);
	ret = nrf_hw_init(priv);
	nrf_flush(priv);
	return ret;
}

static int nrf_ioctl(int fd, int request, va_list args)
{
	struct nrf_priv_s *priv = dev_priv_get(fd);
	struct nrf_pipe_s *pipe;
	struct nrf_chip_s *chip;
	assert(priv != NULL);
	pipe = priv->pipe;
	assert(pipe != NULL);
	chip = priv->chip;
	assert(chip != NULL);

	int (*onfail)(int ecode, ...);
	char mode;
	unsigned addr;
	int freq;

	int ret = 0;
	switch(request) {
	case WL_ERR_TXMS:
		pipe->timeout = va_arg(args, unsigned);
		break;
	case WL_ERR_FUNC:
		addr = va_arg(args, int);
		onfail = (int (*)(int ecode, ...))addr;
		pipe->onfail = (onfail == NULL) ? nrf_onfail : onfail;
		break;

	case WL_SET_MODE:
		mode = (char) va_arg(args, int);
		if(mode != chip->mode) {
			ret = nrf_hw_set_mode(chip, mode);
			chip->mode = (char) mode;
			nrf_flush(priv);
		}
		break;

	case WL_SET_ADDR:
		addr = va_arg(args, unsigned);
		if(addr != pipe->addr) {
			ret = nrf_hw_set_addr(chip, pipe->index, addr);
			pipe->addr = addr;
			nrf_flush(priv);
		}
		break;

	case WL_SET_FREQ:
		freq = va_arg(args, int);
		if(freq != chip->freq) {
			ret = nrf_hw_set_freq(chip, freq);
			chip->freq = freq;
		}
		break;

	case WL_FLUSH:
		nrf_flush(priv);
		break;

	case WL_SEND: //to send a custom frame in blocked, unbuffered method
		pipe->cf = va_arg(args, char *);
		pipe->cf_timeout = (char) va_arg(args, int);
		while(1) {
			nrf_update(priv);
			if(pipe->cf == NULL)
				break;
		}
		ret = pipe->cf_ecode;
		break;
	case WL_START:
		ce_set(1);
		break;
	case WL_STOP:
		ce_set(0);
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

static int nrf_poll(int fd, int event)
{
	struct nrf_priv_s *priv = dev_priv_get(fd);
	struct nrf_pipe_s *pipe;
	assert(priv != NULL);
	pipe = priv->pipe;
	assert(pipe != NULL);

	nrf_update(priv);

	int bytes = 0;
	bytes = (event == POLLIN) ? buf_size(&pipe->rbuf) : bytes;
	bytes = (event == POLLOUT) ? buf_size(&pipe->tbuf) : bytes;
	bytes = (event == POLLIBUF) ? buf_left(&pipe->rbuf) : bytes;
	bytes = (event == POLLOBUF) ? buf_left(&pipe->tbuf) : bytes;
	return bytes;
}

static int nrf_read(int fd, void *buf, int count)
{
	struct nrf_priv_s *priv = dev_priv_get(fd);
	struct nrf_pipe_s *pipe;
	assert(priv != NULL);
	pipe = priv->pipe;
	assert(pipe != NULL);
	do {
		nrf_update(priv);
	} while(buf_size(&pipe->rbuf) < count); //you may call poll to avoid deadloop here
	buf_pop(&pipe->rbuf, buf, count);
	return count;
}

static int nrf_write(int fd, const void *buf, int count)
{
	struct nrf_priv_s *priv = dev_priv_get(fd);
	struct nrf_pipe_s *pipe;
	assert(priv != NULL);
	pipe = priv->pipe;
	assert(pipe != NULL);

	do { //!!!bug: deadloop never get out of here when rbuf also full!!!
		nrf_update(priv);
	} while(buf_left(&pipe->tbuf) < count); //you may call poll to avoid deadloop here
	buf_push(&pipe->tbuf, buf, count);
	return count;
}

static const struct drv_ops_s nrf_ops = {
	.init = nrf_init,
	.open = nrf_open,
	.ioctl = nrf_ioctl,
	.poll = nrf_poll,
	.read = nrf_read,
	.write = nrf_write,
	.close = NULL,
};

struct driver_s nrf_driver = {
	.name = "nrf",
	.ops = &nrf_ops,
};

static void __driver_init(void)
{
	drv_register(&nrf_driver);
}
driver_init(__driver_init);
