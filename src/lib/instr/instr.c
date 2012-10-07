/*
 * 	miaofng@2012 initial version
 *
 */

#include "ulp/sys.h"
#include "instr/instr.h"
#include "priv/mcamos.h"
#include "crc.h"

static struct instr_s *instr; /*current selected instrument*/
static struct list_head instr_list; /*all discoved instruments*/
static const can_bus_t *instr_can = &can1;
static char instr_update_lock = 0;

void instr_init(void)
{
	INIT_LIST_HEAD(&instr_list);
	instr_update_lock = 0;
	can_cfg_t cfg = CAN_CFG_DEF;
	instr_can->init(&cfg);
	/*
		INSTR_PIPE_DAT => instrument private communication
		INSTR_PIPE_BUS => instrument discovery & irq transaction
	*/

	/*disable mcamos communication pipe at first*/
	const can_filter_t filter_mcamos = {.id = 0x0000, .mask = 0xffff};
	instr_can->efilt(INSTR_PIPE_MCAMOS, &filter_mcamos, 1);

	/*enable rbuf1 for newbie requestion*/
	const can_filter_t filter_broadcast = {.id = INSTR_ID_BROADCAST, .mask = 0xffff};
	instr_can->efilt(INSTR_PIPE_BROADCAST, &filter_broadcast, 1);
}

static struct instr_s* __instr_search(unsigned uuid, unsigned mask)
{
	struct list_head *pos;
	struct instr_s *p;

	list_for_each(pos, &instr_list) {
		p = list_entry(pos, instr_s, list);
		if((p->uuid.value & mask) == (uuid & mask)) {
			return p;
		}
	}
	return NULL;
}

static unsigned short instr_id = 0x100;
static int __instr_newbie(struct instr_nreq_s *nreq)
{
	struct instr_s *newbie = __instr_search(nreq->uuid, -1UL);
	if(newbie == NULL) {
		newbie = sys_malloc(sizeof(struct instr_s));
		sys_assert(newbie != NULL);
		list_add(&newbie->list, &instr_list);
		newbie->uuid.value = nreq->uuid;
		//assign can id
		newbie->id_cmd = instr_id ++;
		newbie->id_dat = instr_id ++;
		newbie->priv = NULL;
	}

	newbie->ecode = nreq->ecode;
	newbie->mode = (nreq->ecode == 0) ? INSTR_MODE_INIT : INSTR_MODE_FAIL;
	newbie->timer = 0;

	//change to normal work mode
	can_msg_t msg;
	struct instr_echo_s *echo = (struct instr_echo_s *) msg.data;
	echo->uuid = newbie->uuid.value;
	echo->id_cmd = newbie->id_cmd;
	echo->id_dat = newbie->id_dat;
	msg.id = INSTR_ID_BROADCAST;
	msg.dlc = sizeof(struct instr_echo_s);
	msg.flag = 0;
	time_t deadline = time_get(10);
	while(instr_can->send(&msg)) {
		if(time_left(deadline) < 0) {
			newbie->mode = INSTR_MODE_FAIL;
			return -1;
		}
	}

	//wait for slave to change its work mode
	sys_mdelay(100);

	//get instrument name .. to ensure mcamos works ok now
	unsigned char cmd = INSTR_CMD_GET_NAME;
	instr_select(newbie);
	instr_send(&cmd, 1, 10);
	int ecode = instr_recv(newbie->dummy, INSTR_NAME_LEN_MAX + 3, 10);
	if(ecode == 0) {
		if(cyg_crc16(newbie->name, strlen(newbie->name)) == newbie->uuid.crc_name) {
			newbie->mode = INSTR_MODE_NORMAL;
			printf("INSTR %08x: %04x %04x %s\n", newbie->uuid.value, newbie->id_cmd, newbie->id_dat, newbie->name);
			return 0;
		}
	}

	newbie->mode = INSTR_MODE_FAIL;
	return -1;
}

static void __instr_update(void)
{
	unsigned char inbox[2], cmd = INSTR_CMD_UPDATE;
	instr->timer = time_get(INSTR_UPDATE_MS);
	instr->ecode = 0xff; /*not response*/
	instr->mode = INSTR_MODE_FAIL;

	instr_send(&cmd, 1, 10);
	int ecode = instr_recv(inbox, 2, 10);
	if(ecode == 0) {
		instr->ecode = inbox[1];
		if(instr->ecode == 0)
			instr->mode = INSTR_MODE_NORMAL;
	}
}

/*handle newbie requestion*/
void instr_update(void)
{
	can_msg_t msg;
	if(instr_update_lock) /*to avoid recursive call*/
		return;

	instr_update_lock = 1;

	/*handle newbie request*/
	while(!instr_can->erecv(INSTR_PIPE_BROADCAST, &msg)) {
		if(msg.id == INSTR_ID_BROADCAST) {
			__instr_newbie((struct instr_nreq_s *) msg.data);
		}
	}

	/*handle instr status update*/
	struct list_head *pos;
	struct instr_s *p, *instr_save = instr;

	list_for_each(pos, &instr_list) {
		p = list_entry(pos, instr_s, list);
		if(time_left(p->timer) < 0) {
			instr_select(p);
			__instr_update();
		}
	}

	if(instr != instr_save) {
		instr_select(instr_save);
	}
	instr_update_lock = 0;
}

struct instr_s* instr_open(int class, const char *name)
{
	unsigned uuid = class << 24;
	struct instr_s*p = __instr_search(uuid, 0xff << 24);
	p = (p->mode == INSTR_MODE_NORMAL) ? p : NULL;
	return p;
}

void instr_close(struct instr_s *i)
{
}

int instr_select(struct instr_s *instr_new)
{
	sys_assert(instr_new != NULL);
	instr = instr_new;

	struct mcamos_s instr_mcamos;
	instr_mcamos.can = instr_can;
	instr_mcamos.baud = INSTR_BAUD;
	instr_mcamos.id_cmd = instr->id_cmd;
	instr_mcamos.id_dat = instr->id_dat;
	instr_mcamos.timeout = 100;
	mcamos_init_ex(&instr_mcamos);

	//reconfigure the can filter settings
	can_filter_t filter_mcamos[] = {
		{.id = instr->id_cmd, .mask = 0xffff},
		{.id = instr->id_dat, .mask = 0xffff},
	};
	instr_can->efilt(INSTR_PIPE_MCAMOS, filter_mcamos, 2);

	/*enable rbuf1 for newbie requestion*/
	const can_filter_t filter_broadcast = {.id = INSTR_ID_BROADCAST, .mask = 0xffff};
	instr_can->efilt(INSTR_PIPE_BROADCAST, &filter_broadcast, 1);
	return 0;
}

int instr_send(const void *data, int n, int ms)
{
	return mcamos_dnload(instr_can, INSTR_INBOX_ADDR, data, n, ms);
}

static int __instr_wait(int ms)
{
	int ret;
	char cmd, outbox[2];
	time_t deadline = time_get(ms);

	do {
		ret = mcamos_upload(instr_can, INSTR_INBOX_ADDR, &cmd, 1, 10);
		if(ret) {
			return -1;
		}

		if(time_left(deadline) < 0) {
			return -1;
		}
	} while(cmd != 0);

	ret = mcamos_upload(instr_can, INSTR_OUTBOX_ADDR, outbox, 2, 10);
	if(ret)
		return -1;

	return outbox[1];
}

int instr_recv(void *data, int n, int ms)
{
	int ecode = __instr_wait(ms);
	if(!ecode) {
		return mcamos_upload(instr_can, INSTR_OUTBOX_ADDR, data, n, 10);
	}
	return -1;
}

#include "shell/cmd.h"

static int cmd_instr_func(int argc, char *argv[])
{
	const char *usage = {
		"instr list	list all instruments\n"
	};

	if(argc > 1 && !strcmp(argv[1], "list")) {
		struct list_head *pos;
		struct instr_s *p;

		list_for_each(pos, &instr_list) {
			p = list_entry(pos, instr_s, list);
			printf("%08x: %04x %04x %02x %s\n", p->uuid.value, p->id_cmd, p->id_dat, p->ecode, p->name);
		}
		return 0;
	}

	printf(usage);
	return 0;
}

const cmd_t cmd_instr = {"instr", cmd_instr_func, "instr cmds"};
DECLARE_SHELL_CMD(cmd_instr)
