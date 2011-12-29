/*
 * 	miaofng@2010 initial version
  *		nrf24l01, 2.4g wireless gfsk transceiver
 *		interface: spi, 8Mhz, 8bit, CPOL=0(sck low idle), CPHA=0(strobe at 1st edge of sck), msb first
 *		-tx fifo: 32bytes x 3, rx fifo 32bytes x 3
 */
#ifndef __NRF_H_
#define __NRF_H_

#include "ulp/device.h"

//misc cmds
#define NOP (0xff)
#define ACTIVATE (0x50) // + 0X73, pd mode or stdby
#define R_REGISTER(reg) (0x00 | (reg))
#define W_REGISTER(reg) (0x20 | (reg)) //pd mode or stdby mode

//fx fifo ops
#define R_RX_PAYLOAD (0x61)
#define R_RX_PL_WID (0x60)
#define FLUSH_RX (0xe2)

//tx fifo ops
#define W_TX_PAYLOAD (0xa0)
#define W_TX_PAYLOAD_NOACK (0xb0)
#define W_ACK_PAYLOAD(pipe) (0xa8 | (pipe))
#define REUSE_TX_PL (0xe3) //ptx device
#define FLUSH_TX (0xe1)

enum {
	CONFIG = 0x00,
	EN_AA,
	EN_RXADDR,
	SETUP_AW,
	SETUP_RETR,
	RF_CH,
	RF_SETUP,
	STATUS, /* - | RX_DR | TX_DS | MAX_RT | RX_P_NO 3.. 1 | TX_FULL*/

#define RX_DR 0x40
#define TX_DS 0x20
#define MAX_RT 0x10

	OBSERVE_TX,
	CD,
	RX_ADDR_P0,
	RX_ADDR_P1,
	RX_ADDR_P2,
	RX_ADDR_P3,
	RX_ADDR_P4,
	RX_ADDR_P5,

	TX_ADDR = 0x10,
	RX_PW_P0,
	RX_PW_P1,
	RX_PW_P2,
	RX_PW_P3,
	RX_PW_P4,
	RX_PW_P5,
	FIFO_STATUS, /* -|TX_REUSE|TX_FULL|TX_EMPTY| - | - | RX_FULL | RX_EMPTY*/

#define TX_FULL 0x20
#define TX_EMPTY 0x10
#define RX_FULL 0x02
#define RX_EMPTY 0x01

	DYNPD = 0x1c,
	FEATURE = 0x1d,
};

#endif /*__NRF_H_*/
