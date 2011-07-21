/*
 * 	miaofng@2011 initial version
 *	current scheme limitation:
 *		the can bus phy settings for dut communication, such as baudrate and etc, must be 500Kbaud
 *		no hw filter is allowed
 */

#include "config.h"
#include "nest_message.h"
#include "nest_burn.h"
#include "priv/mcamos.h"
#include "time.h"
#include <stdlib.h>

static int burn_wait(struct mcamos_s *m, int timeout)
{
	int ret;
	char cmd, outbox[2];
	time_t deadline = time_get(timeout);

	do {
		ret = mcamos_upload(m ->can, BURN_INBOX_ADDR, &cmd, 1, 10);
		if(ret) {
			return -1;
		}

		if(time_left(deadline) < 0) {
			return -1;
		}
	} while(cmd != 0);

	ret = mcamos_upload(m->can, BURN_OUTBOX_ADDR, outbox, 2, 10);
	if(ret)
		return -1;

	return outbox[1];
}

int burn_config(int ch, const struct burn_data_s *config)
{
	int ret;
	char cmd = BURN_CMD_CONFIG;
	struct mcamos_s m;
	m.can = &can1;
	m.baud = 500000;
	m.id_cmd = 0x5e0 + (ch << 1);
	m.id_dat = m.id_cmd + 1;

	mcamos_init_ex(&m);
	mcamos_dnload(m.can, BURN_INBOX_ADDR + 1, (char *) config, sizeof(struct burn_data_s ), 10);
	mcamos_dnload(m.can, BURN_INBOX_ADDR, &cmd, 1, 10);
	ret = burn_wait(&m, 10);
	mcamos_init_ex(NULL); //restore!!! it's dangerious here
	return ret;
}

int burn_read(int ch, struct burn_data_s *result)
{
	int ret;
	char cmd = BURN_CMD_READ;
	struct mcamos_s m;
	m.can = &can1;
	m.baud = 500000;
	m.id_cmd = 0x5e0 + (ch << 1);
	m.id_dat = m.id_cmd + 1;

	mcamos_init_ex(&m);
	mcamos_dnload(m.can, BURN_INBOX_ADDR, &cmd, 1, 10);
	ret = burn_wait(&m, 10);
	if(ret)
		return -1;

	ret = mcamos_upload(m.can, BURN_OUTBOX_ADDR + 2, (char *) result, sizeof(struct burn_data_s), 10);
	mcamos_init_ex(NULL); //restore!!! it's dangerious here
	return ret;
}
