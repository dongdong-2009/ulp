/*
 *  miaofng@2010 initial version
 */

#ifndef __WL_H_
#define __WL_H_

#include "config.h"
#include "common/circbuf.h"

//ecode
enum {
	WL_ERR_OK,
	WL_ERR_HW, //onfail(ecode)
	WL_ERR_RX_HW, //onfail(ecode)
	WL_ERR_RX_FRAME, //onfail(ecode, unsigned char * frame, unsigned char len)
	WL_ERR_TX_HW, //onfail(ecode)
	WL_ERR_TX_TIMEOUT, //onfail(ecode, addr), caller may need to flush the tbuf&fifo?
};

//FRAME TYPE, note: for nrf, byte0 is frame type as below
enum {
	WL_FRAME_DATA = 'D',
	WL_FRAME_PING = 'P',
	WL_FRAME_INVALID,
};

//IOCTL MODE PARA
enum {
	WL_MODE_PTX = 0,
	WL_MODE_PRX = 1, //or ptx
	WL_MODE_2MBPS = 0,
	WL_MODE_1MBPS = 2, //or 2MBPS
};

/*IOCTL CMDS - there is no specific call sequency requirements*/
enum {
	WL_GET_FAIL, /*get retry counter*/
	WL_ERR_TXMS, /*set send timeout*/
	WL_ERR_FUNC,
	WL_SET_MODE, /*prx?*/
	WL_SET_ADDR,
	WL_SET_FREQ, /*unit: MHz*/
	WL_SET_TBUF, /*tbuf, bytes*/
	WL_SET_RBUF, /*rbuf, bytes*/
	WL_FLUSH,
	/*to insert a custom frame to send fifo, ioctl(fd, WL_SEND, frame, timeout) buf, format: len(note: max 15bytes when 2MBPS), type, d0, d1 ..
	to wait until the frame is sent to hw fifo*/
	WL_SEND,
	WL_START,
	WL_STOP,
};

/*nrf24l01*/
struct nrf_cfg_s {
	const void *spi;
	char gpio_cs;
	char gpio_ce;
	char gpio_irq;
};

#endif /* __WL_H_ */
