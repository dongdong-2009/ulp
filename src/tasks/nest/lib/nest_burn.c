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
#include "shell/cmd.h"
#include "nvm.h"
#include "nest_core.h"

//Peak Pulse Limit default setting
#define BURN_VL_DEF	410 //Vpmin unit: V
#define BURN_IL_DEF	11500 //Ipmax unit: mA
#define BURN_WL_DEF	5000 //peak width unit: nS

static int burn_vl __nvm;
static int burn_il __nvm;
static int burn_log = 0;
static char burn_flag;
static time_t burn_timer;

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

int burn_init(void)
{
	//check limit ok?
	burn_vl = (burn_vl == -1) ? BURN_VL_DEF : burn_vl;
	burn_il = (burn_il == -1) ? BURN_IL_DEF : burn_il;

	burn_flag = 0;
	burn_timer = time_get(60000);
	return 0;
}

int burn_mask(int ch)
{
	burn_flag |= (1 << ch);
	return 0;
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
	avg = burn_data->vp_avg;
	min = burn_data->vp_min;
	max = burn_data->vp_max;
	nest_message("%d %d %d	", avg, min, max);

	avg = burn_data->ip_avg;
	min = burn_data->ip_min;
	max = burn_data->ip_max;
	nest_message("%d %d %d	", avg, min, max);

	int wp = burn_data->wp;
	int fire = burn_data->fire;
	int lost = burn_data->lost;
	nest_message("%d %d %d\n", wp, fire, lost);
}

int burn_verify(unsigned short *vp, unsigned short *ip)
{
	int ret, ch;
	struct burn_data_s burn_data;

	//ignore igbt burn test?
	if(nest_ignore(PKT))
		return 0;

	//get test result from burn board through mcamos/can protocol
	for(ch = BURN_CH_COILA; ch <= BURN_CH_COILD; ch ++) {
		ret = burn_read(ch, &burn_data);
		if(ret) {
			nest_message("burn board channel %d not response\n", ch);
			return -1;
		}

		if(burn_log & (1 << ch)) {
			nest_message("%d ", ch);
			burn_disp(&burn_data);
		}

		if(burn_data.lost > 10) {
			nest_message("burn board channel %d sync lost too much(%d)\n", ch, burn_data.lost);
			return -2;
		}

		if(burn_data.wp > BURN_WL_DEF) {
			nest_message("burn board channel %d wp(=%dnS) higher than threshold(=%dnS)\n", ch, burn_data.wp, BURN_WL_DEF);
			return -3;
		}

		if(burn_data.fire > 500) { //500 * 20mS = 10S
			//save measurement result to mfg_data
			if(vp != NULL)
				vp[ch] = burn_data.vp_min;
			if(ip != NULL)
				ip[ch] = burn_data.ip_max;

			if(burn_data.vp_min < burn_vl) {
				nest_message("burn board channel %d vp(=%dV) lower than threshold(=%dV)\n", ch, burn_data.vp_min, burn_vl);
				return -4;
			}

			if(burn_data.ip_max > burn_il) {
				nest_message("burn board channel %d ip(=%dmA) higher than threshold(=%dmA)\n", ch, burn_data.ip_max, burn_il);
				return -5;
			}
		}
		else {
			if(time_left(burn_timer) < 0) {
				if((~burn_flag) & (1 << ch)) {
					nest_message("burn board channel %d not fire\n", ch);
					return -6;
				}
			}
		}
	}

	return 0;
}

//burn shell command
static int cmd_burn_func(int argc, char *argv[])
{
	const char *usage = {
		"burn log 0|1|2|3|all|none		print ch 0/1/2/3/all log message\n"
		"burn vl 410				vp low threshold\n"
		"burn il 11000				ip high threshold\n"
		"burn save\n"
	};

	if(argc >= 2) {
		if(!strcmp(argv[1], "log")) {
			for(int i = 2; i < argc; i ++) {
				int log = -1;
				if(!strcmp(argv[i], "all")) {
					log = 0x0f;
				}
				if(!strcmp(argv[i], "none")) {
					log = 0x00;
					burn_log = 0x00;
				}
				if(log == -1) {
					log = atoi(argv[i]);
					log = 1 << (log % 4);
				}

				burn_log |= log;
			}
			nest_message("burn_log = 0x%02x\n", burn_log);
			return 0;
		}

		if(!strcmp(argv[1], "vl")) {
			if(argc == 3)
				burn_vl = atoi(argv[2]);
			nest_message("burn_vl = %dV\n", burn_vl);
			return 0;
		}

		if(!strcmp(argv[1], "il")) {
			if(argc == 3)
				burn_il = atoi(argv[2]);
			nest_message("burn_il = %dmA\n", burn_il);
			return 0;
		}
	}

	if(argc == 2) {
		if(!strcmp(argv[1], "save")) {
			nvm_save();
			return 0;
		}
	}

	nest_message("%s", usage);
	nest_message("\ncurrent settings:\n");
	nest_message("burn_log = 0x%02x\n", burn_log);
	nest_message("burn_vl = %dV\n", burn_vl);
	nest_message("burn_il = %dmA\n", burn_il);
	return 0;
}

const static cmd_t cmd_burn = {"burn", cmd_burn_func, "burn debug command"};
DECLARE_SHELL_CMD(cmd_burn)

