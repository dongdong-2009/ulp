/*
 * (C) Copyright 2000-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
  * (C) Copyright 2010
 * miaofng@gmail.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Serial up- and download support
 */
#include "config.h"
#include "common/kermit.h"
#include "ulp/debug.h"
#include "err.h"
#include <string.h>
#include "sys/sys.h"

#define XON_CHAR        17
#define XOFF_CHAR       19
#define START_CHAR      0x01
#define ETX_CHAR	0x03
#define END_CHAR        0x0D
#define SPACE           0x20
#define K_ESCAPE        0x23
#define SEND_TYPE       'S'
#define DATA_TYPE       'D'
#define FILE_TYPE	'F'
#define ACK_TYPE        'Y'
#define NACK_TYPE       'N'
#define BREAK_TYPE      'B'
#define tochar(x) ((char) (((x) + SPACE) & 0xff))
#define untochar(x) ((int) (((x) - SPACE) & 0xff))

enum {
	KERMIT_STM_INIT,
	KERMIT_STM_START,
	KERMIT_STM_GET_LEN_EXT,
	KERMIT_STM_DATA,
};

static char ktrans (char in);
static void send_ack (kermit_t *k);
static void send_nak (kermit_t *k);
static int handle_data_packet (kermit_t *k);
static int handle_send_packet (kermit_t *k);

/* converts escaped kermit char to binary char */
static char ktrans (char in)
{
	if ((in & 0x60) == 0x40) {
		return (char) (in & ~0x40);
	}
	else if ((in & 0x7f) == 0x3f) {
		return (char) (in | 0x40);
	}
	else
		return in;
}

int kermit_init(kermit_t *k, circbuf_t *ibuf, circbuf_t *obuf)
{
	assert(k != NULL);
	assert(ibuf != NULL);
	assert(obuf != NULL);

	/* initialize some protocol parameters */
	k -> his_eol = END_CHAR; /* default end of line character */
	k -> his_pad_count = 0;
	k -> his_pad_char = '\0';
	k -> his_quote = K_ESCAPE;

	/* initialize the k_recv and k_data state machine */
	k -> state_saved = k -> state = 0;
	k -> escape_saved = k -> escape = 0;
	k -> n = 0; /* just to get rid of a warning */
	k -> last_n = -1;

	k -> stm = KERMIT_STM_INIT;
	k -> ibuf = ibuf;
	k -> obuf = obuf;
	buf_init(&k -> tbuf, 0);
	memcpy(&k -> obuf_saved, k -> obuf, sizeof(circbuf_t));
	memset(k -> fname, 0, 32);
	return 0;
}

#define kermit_read(var)  do { \
	char __cr; \
	buf_pop(k -> ibuf, &__cr, 1); \
	k -> length --; \
	k -> cksum += __cr & 0xff; \
	if ((__cr & 0xE0) == 0) \
		goto packet_error; \
	var = __cr; \
} while(0)

/* try to recv a full packet from kermit line:
With each packet, start summing the bytes starting with the length.
Save the current sequence number.
Note the type of the packet.
If a character less than SPACE (0x20) is received - error.
 */
int kermit_recv(kermit_t *k, circbuf_t *echo)
{
	char new_char;
	int len_lo, len_hi;
	int sum;

	//transfer data from tbuf to obuf
	while(buf_size(&k -> tbuf) > 0) {
		if(buf_left(k -> obuf) == 0) {
			return ERR_OVERFLOW;
		}

		buf_pop(&k -> tbuf, &new_char, 1);
		buf_push(k -> obuf, &new_char, 1);
	}

	//free tbuf
	buf_free(&k -> tbuf);

	k -> echo = echo;
	switch (k -> stm) {
	case KERMIT_STM_INIT:
		do {
			/* wait for the starting character or ^C */
			if(buf_size(k -> ibuf) < 1)
				return ERR_RETRY;

			buf_pop(k -> ibuf, &new_char, 1);
			if(new_char == ETX_CHAR)
				return ERR_ABORT;
			if(new_char == START_CHAR) {
				break;
			}
		} while(1);
		k -> stm = KERMIT_STM_START;

	case KERMIT_STM_START:
		k -> cksum = 0;
		if(buf_size(k -> ibuf) < 3)
			return ERR_RETRY;

		kermit_read(new_char); /* get length of packet */
		k -> length = untochar(new_char);

		kermit_read(new_char); /* get sequence number */
		k -> n = untochar(new_char);

		/* NEW CODE - check sequence numbers for retried packets */
		/* Note - this new code assumes that the sequence number is correctly
		 * received.  Handling an invalid sequence number adds another layer
		 * of complexity that may not be needed - yet!  At this time, I'm hoping
		 * that I don't need to buffer the incoming data packets and can write
		 * the data into memory in real time.
		 */
		if (k -> n == k -> last_n) {
			/* same sequence number, restore the previous state */
			k -> state = k -> state_saved;
			k -> escape = k -> escape_saved;
			memcpy(k -> obuf, &k -> obuf_saved, sizeof(circbuf_t));
		} else {
			/* new sequence number, checkpoint the download */
			k -> last_n = k -> n;
			k -> state_saved = k -> state;
			k -> escape_saved = k -> escape;
			memcpy(&k -> obuf_saved, k -> obuf, sizeof(circbuf_t));
		}
		/* END NEW CODE */

		kermit_read(new_char); /* get packet type */
		k -> state = new_char;
		k -> stm = KERMIT_STM_GET_LEN_EXT;

	case KERMIT_STM_GET_LEN_EXT:
		/* check for extended length */
		/* new length includes only data and block check to come */
		if (k -> length == -2) {
			if(buf_size(k -> ibuf) < 3)
				return ERR_RETRY;

			kermit_read(len_hi);
			kermit_read(len_lo);

			/* check header checksum */
			sum = k -> cksum;
			kermit_read(new_char);
			if (new_char != tochar ((sum + ((sum >> 6) & 0x03)) & 0x3f))
				goto packet_error;

			k -> length = len_hi * 95 + len_lo;
		}
		k -> stm = KERMIT_STM_DATA;

	case KERMIT_STM_DATA:
		if(buf_size(k -> ibuf) < k -> length + 1)
			return ERR_RETRY;

		/* bring in rest of packet */
		switch (k -> state) {
		case FILE_TYPE:
			memset(k -> fname, 0, 32);
		case DATA_TYPE:
			if(handle_data_packet(k)) {
				goto packet_error;
			}
			break;
		case SEND_TYPE:
			if(handle_send_packet(k)) {
				goto packet_error;
			}
			break;
		default:
			while(k -> length > 1) {
				kermit_read(new_char);
			}
		}

		/* get and validate checksum character */
		sum = k -> cksum;
		kermit_read(new_char);
		if (new_char != tochar ((sum + ((sum >> 6) & 0x03)) & 0x3f))
			goto packet_error;

		/* get END_CHAR, not included in length byte*/
		buf_pop(k -> ibuf, &new_char, 1);
		if (new_char != END_CHAR)
			goto packet_error;

		//frame over
		k -> stm = KERMIT_STM_INIT;
		if (k -> state == SEND_TYPE) {
			return 0;
		}
		else {
			/* send simple acknowledge packet in */
			send_ack (k);
			return (k -> state == BREAK_TYPE) ? ERR_QUIT : ERR_OK;
		}
	default:
		goto packet_error;
	}

packet_error:
	k -> stm = KERMIT_STM_INIT;
	/* restore state machines */
	k -> state = k -> state_saved;
	k -> escape = k -> escape_saved;
	memcpy(k -> obuf, &k -> obuf_saved, sizeof(circbuf_t));
	/* send a negative acknowledge packet in */
	send_nak (k);
	return ERR_PACKET;
}

#define kermit_write(var)  do { \
	char __cr = var; \
	buf_push(k -> echo, &__cr, 1); \
	cksum += __cr; \
} while(0)

static void send_ack (kermit_t *k)
{
	int n, cksum;
	buf_free(k -> echo);
	buf_init(k -> echo, k -> his_pad_count + 8);

	//fill pad bytes
	for(n = 0; n < k -> his_pad_count; n ++) {
		kermit_write(k -> his_pad_char);
	}

	//fill packet
	kermit_write(START_CHAR);
	cksum = 0;
	kermit_write(tochar (3));
	kermit_write(tochar (k -> n));
	kermit_write(ACK_TYPE);
	kermit_write(tochar ((cksum + ((cksum >> 6) & 0x03)) & 0x3f));
	kermit_write(k -> his_eol);
}

static void send_nak (kermit_t *k)
{
	int n, cksum;
	buf_free(k -> echo);
	buf_init(k -> echo, k -> his_pad_count + 8);

	//fill pad bytes
	for(n = 0; n < k -> his_pad_count; n ++) {
		kermit_write(k -> his_pad_char);
	}

	//fill packet
	kermit_write(START_CHAR);
	cksum = 0;
	kermit_write(tochar (3));
	kermit_write(tochar (k -> n));
	kermit_write(NACK_TYPE);
	kermit_write(tochar ((cksum + ((cksum >> 6) & 0x03)) & 0x3f));
	kermit_write(k -> his_eol);
}

static int handle_data_packet (kermit_t *k)
{
	int fname_idx = 0;
	char new_char;
	while(k -> length > 1) {
		kermit_read(new_char);
		if (k -> escape) {
			/* last char was escape - translate this character */
			new_char = ktrans (new_char);
			k -> escape = 0;
		}
		else if (new_char == k -> his_quote) {
			/* this char is escape - remember */
			k -> escape = 1;
			continue;
		}

		//file name ???
		if(k -> state == FILE_TYPE && fname_idx < 31) {
			k -> fname[fname_idx ++] = new_char;
			continue;
		}

		//save char to obuf
		if(buf_left(k -> obuf) > 0) {
			buf_push(k -> obuf, &new_char, 1);
			continue;
		}

		//save char to tbuf
		if(buf_size(&k -> tbuf) == 0) {
			buf_init(&k -> tbuf, k -> length); //note: k -> length - 1 ( cksum char) + 1 ( char has been read)
		}

		buf_push(&k -> tbuf, &new_char, 1);
	}

	return 0;

packet_error:
	return ERR_PACKET;
}

/* handle_send_packet interprits the protocol info and builds and
   sends an appropriate ack for what we can do */
static int handle_send_packet (kermit_t *k)
{
	int cksum, n;
	char new_char;

	buf_free(k -> echo);
	buf_init(k -> echo, k -> his_pad_count + 16 + 3);

	/* initialize some protocol parameters */
	k -> his_eol = END_CHAR; /* default end of line character */
	k -> his_pad_count = 0;
	k -> his_pad_char = '\0';
	k -> his_quote = K_ESCAPE;

	//fill pad bytes
	for(n = 0; n < k -> his_pad_count; n ++) {
		kermit_write(k -> his_pad_char);
	}

	//fill frame header
	kermit_write(START_CHAR);
	cksum = 0;
	kermit_write(tochar(16));
	kermit_write(tochar(k -> n)); /*seq nr .. byte 1*/
	kermit_write(ACK_TYPE); /*type .. byte 2*/

	/* handle MAXL - max length */
	/* ignore what he says - most I'll take (here) is 94 */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write(tochar(94)); /*.. byte 3*/

	/* handle TIME - time you should wait for my packets */
	/* ignore what he says - don't wait for my ack longer than 3 second */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write(tochar(3)); /*.. byte 4*/

	/* handle NPAD - number of pad chars I need */
	/* remember what he says - I need none */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write(tochar(0)); /*.. byte 5*/
	k -> his_pad_count = untochar (new_char);

	/* handle PADC - pad chars I need */
	/* remember what he says - I need none */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write(0x40); /*.. byte 6*/
	k -> his_pad_char = ktrans (new_char);

	/* handle EOL - end of line he needs */
	/* remember what he says - I need CR */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write(tochar(END_CHAR)); /*.. byte 7*/
	k -> his_eol = untochar (new_char);

	/* handle QCTL - quote control char he'll use */
	/* remember what he says - I'll use '#' */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write('#'); /*.. byte 8*/
	k -> his_quote = new_char;

	/* handle QBIN - 8-th bit prefixing */
	/* ignore what he says - I refuse */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write('N'); /*.. byte 9*/

	/* handle CHKT - the clock check type */
	/* ignore what he says - I do type 1 (for now) */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write('1'); /*.. byte 10*/

	/* handle REPT - the repeat prefix */
	/* ignore what he says - I refuse (for now) */
	if(k -> length > 1) kermit_read(new_char);
	kermit_write('N'); /*.. byte 11*/

	/* handle CAPAS - the capabilities mask */
	/* ignore what he says - I only do long packets - I don't do windows */
	if(k -> length > 1) kermit_read(new_char);
	if(k -> length > 1) kermit_read(new_char);
	if(k -> length > 1) kermit_read(new_char);
	if(k -> length > 1) kermit_read(new_char);
	kermit_write(tochar (2));	/* only long packets  .. byte 12*/
	kermit_write(tochar (0));	/* no windows  .. byte 13*/
	kermit_write(tochar (94));	/* large packet msb .. byte 14*/
	kermit_write(tochar (94));	/* large packet lsb .. byte 15*/

	//any dummy?
	while(k -> length > 1) {
		kermit_read(new_char);
	}

	//fill tail
	kermit_write(tochar ((cksum + ((cksum >> 6) & 0x03)) & 0x3f)); /*cksum .. byte 16*/
	kermit_write(k -> his_eol);
	return 0;

packet_error:
	return ERR_PACKET;
}
