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

static void burn_disp(const struct burn_data_s *burn_data)
{
	int avg, min, max;
	//R18 = 22K, R17 = 100Ohm, Vref = 2V, 16bit unsigned ADC => ratio = 1/148.2715= 256/37957
	avg = (burn_data.vp_avg << 8)/37957;
	min = (burn_data.vp_min << 8)/37957;
	max = (burn_data.vp_max << 8)/37957;
	printf("%d %d %d	", avg, min, max);

	//R = 0.01Ohm, G = 20V/V, Vref = 2V5, 16bit unsigned ADC => ratio = 1/5242.88= 256/1342177
	avg = (burn_data.ip_avg << 8) / 1342;
	min = (burn_data.ip_min << 8) / 1342;
	max = (burn_data.ip_max << 8) / 1342;
	printf("%d %d %d	", avg, min, max);

	int wp = burn_data.wp;
	int fire = burn_data.fire;
	int lost = burn_data.lost;
	printf("%d %d %d\n", wp, fire, lost);	
}

int burn_verify(void *result)
{
	int ret, ch;
	struct burn_data_s burn_data;
	for(ch = BURN_CH_COILA; ch <= BURN_CH_COILD; ch ++) {
		ret = burn_read(ch, &burn_data);
		if(!ret) {
			burn_disp(&burn_data);
		}
	}
	return 0;
}

