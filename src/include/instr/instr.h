/*
 *  miaofng@2012 initial version
 *	instrument communication protocol v1.0
 *	principal - time division multiplex:
 *	1, instrument power-up, newbie req broadcast with INSTR_ID_PUB
 *	2, master monitor the req and assign the newbie with an unique ID
 *	3, instrument switch to normal work mode - can only tx/rx with mcamos protocol
 *	4, master !may! poll each instrument's status periodly.
 *
 */

#ifndef __INSTR_H_
#define __INSTR_H_

#include "linux/list.h"
#include "priv/mcamos.h"
#include "ulp_time.h"

#define INSTR_BAUD MCAMOS_BAUD
#define INSTR_INBOX_ADDR MCAMOS_INBOX_ADDR
#define INSTR_OUTBOX_ADDR  MCAMOS_OUTBOX_ADDR
#define INSTR_ID_BROADCAST 0x010
#define INSTR_NAME_LEN_MAX (31)
#define INSTR_PIPE_BROADCAST 1
#define INSTR_PIPE_MCAMOS 0
#define INSTR_UPDATE_MS 1000

/*instrument work modes*/
enum {
	INSTR_MODE_INIT,
	INSTR_MODE_NORMAL,
	INSTR_MODE_FAIL,
};

/*common instrument commands*/
enum {
	INSTR_CMD_GET_NAME = 0x01,
	INSTR_CMD_UPDATE,
	INSTR_CMD_END = 0x10,
};

/*instrument classes*/
enum {
	INSTR_CLASS_DMM, /*multi meter*/
	INSTR_CLASS_MATRIX,
	INSTR_CLASS_POWER,
};

/*common instrument inbox structure*/
struct instr_inbox_s {
	unsigned char cmd;
};

/*common instrument outbox structure*/
struct instr_outbox_s {
	unsigned char cmd;
	unsigned char ecode;
};

struct instr_s {
	union instr_uuid {
		unsigned value;
		struct {
			unsigned crc_name : 16;
			unsigned index : 8;
			unsigned class : 8;
		};
	} uuid;
	unsigned id_cmd : 16;
	unsigned id_dat : 16;
	unsigned char mode;
	unsigned char ecode; /*nreq->ecode, self test code*/
	char dummy[2];
	char name[INSTR_NAME_LEN_MAX + 1];
	struct list_head list; /*point to next instrument*/
	time_t timer; /*update timer*/
	void *priv;
};

/*newbie broadcast message structure*/
struct instr_nreq_s {
	unsigned uuid; /*device specific id*/
	unsigned char ecode; /*self test status*/
};

struct instr_echo_s {
	unsigned uuid;
	unsigned id_cmd : 16;
	unsigned id_dat : 16;
};

void instr_init(void);
void instr_update(void);
struct instr_s* instr_open(int instr_class, const char *name);
void instr_close(struct instr_s *instr);

int instr_select(struct instr_s *instr);
int instr_send(const void *data, int n, int ms); /*success return 0*/
int instr_recv(void *data, int n, int ms); /*success return 0*/

#endif /* __INSTR_H_ */
