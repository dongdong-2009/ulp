/*
 * 	miaofng@2010 initial version
 *		This file is used to provide phy level i/f for ISO14230/keyword2000 protocol
 */
#ifndef __KWD_H_
#define __KWD_H_

#include <stddef.h>

#define KWD_BAUD 10400

void kwd_init(void);

/*special support for keyword fast init protocol*/
enum {
	KWD_WKOP_EN,
	KWD_WKOP_LO,
	KWD_WKOP_HI,
	KWD_WKOP_RS,
};

void kwd_wake(int op);
/*main keyword phy level tx/rx routine, note:
1, it's a half duplex protocol, tbuf==rbuf is possible
2, it returns immediately, pls use poll to check the status
3, current transfer will be canceled when this routine is called again
*/
int kwd_transfer(char *tbuf, size_t tn, char *rbuf, size_t rn);

/* poll current transfer status
	rx == 0, return bytes transmitted
	rx == 1, return bytes received
*/
int kwd_poll(int rx);

/* change baudrate
*/
int kwd_baud(int baud);
#endif /*__KWD_H_*/