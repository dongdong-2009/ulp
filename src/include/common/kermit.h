/*
 * miaofng (C) Copyright 2010
 *
 *
 */

#ifndef __KERMIT_H__
#define __KERMIT_H__

#include "common/circbuf.h"

typedef struct kermit_s {
	circbuf_t *ibuf; /*kermit line recv buf*/
	circbuf_t *obuf; /*file data output buffer*/
	circbuf_t *echo; /*kermit line send buf*/
	circbuf_t obuf_saved;
	circbuf_t tbuf; /*temp buf for file data output*/
	char fname[32];

	//kermit protocol para
	char his_eol;        /* character he needs at end of packet */
	char his_pad_count;  /* number of pad chars he needs */
	char his_pad_char;   /* pad chars he needs */
	char his_quote;      /* quote chars he'll use */

	//recv state machine
	char stm; /*packet state machine*/
	char state, state_saved;
	char n, last_n; /*sequency nr*/
	char escape, escape_saved;
	int cksum, length;
} kermit_t;

/*note:
ibuf	input buf, space is allocated by caller, typical size is 128Bytes
obuf	output buf, space is allocated by caller, typical size is the flash sector size 1024?
echo	echo frame, space is allocated by kermit, freed by called 
*/
int kermit_init(kermit_t *k, circbuf_t *ibuf, circbuf_t *obuf);
int kermit_recv(kermit_t *k, circbuf_t *echo);
#define kermit_fname(k) ((k) -> fname)

#endif
