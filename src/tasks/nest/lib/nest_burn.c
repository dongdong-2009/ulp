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
#include "ulp_time.h"
#include <stdlib.h>
#include "shell/cmd.h"
#include "nvm.h"
#include "nest_core.h"
#include "nest_power.h"
#include <string.h>
#include "crc.h"

#define CONFIG_IGBT_CRC 1

//Peak Pulse Limit default setting
#define BURN_VL_DEF	400 //Vpmin unit: V
#define BURN_IL_DEF	11000 //Ipmax unit: mA
#define BURN_WL_DEF	5000 //peak width unit: nS
#define BURN_IL_MIN	2000 //ip min unit:mA

static int burn_vl __nvm;
static int burn_il __nvm;
static int burn_log = 0;
static char burn_flag;
static char burn_flag_cal;
static time_t burn_timer;
static char burn_fail_counter;
static struct {
	int ch;
	int vp; //V
	int ip; //mA
} burn_cal_para;

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
	int ch, fail;
	int try = 5;

	//poll burn board
	do {
		try --;
		fail = 0;
		for(ch = BURN_CH_COILA; ch <= BURN_CH_COILD; ch ++) {
			if(burn_read(ch, NULL)) {
				fail = -1;
				nest_message("warnning: burn board %d not response\n", ch);
			}
		}

		if(fail) {
			nest_power_reboot();
		}
	} while(try > 0 && fail);

	//check limit ok?
	burn_vl = (burn_vl == -1 || burn_vl == 0) ? BURN_VL_DEF : burn_vl;
	burn_il = (burn_il == -1 || burn_il == 0) ? BURN_IL_DEF : burn_il;

	burn_flag = 0;
	burn_fail_counter = 0;
	burn_timer = time_get(60000);
	return fail;
}

int burn_mask(int ch)
{
	burn_flag |= (1 << ch);
	return 0;
}

int burn_config(int ch, unsigned short vpm_ratio, unsigned short ipm_ratio, int save)
{
	int ret;
	char mailbox[6];
	struct mcamos_s m;
	m.can = &can1;
	m.baud = BURN_BAUD;
	m.id_cmd = 0x5e0 + (ch << 1);
	m.id_dat = m.id_cmd + 1;

	mailbox[0] = BURN_CMD_CONFIG;
	mailbox[1] = (save) ? 's' : 0;
	memcpy(mailbox + 2, &vpm_ratio, sizeof(vpm_ratio));
	memcpy(mailbox + 4, &ipm_ratio, sizeof(ipm_ratio));

	mcamos_init_ex(&m);
	mcamos_dnload(m.can, BURN_INBOX_ADDR, mailbox, sizeof(mailbox), 10);
	if(save)
		mdelay(500);
	ret = burn_wait(&m, 10);
	mcamos_init_ex(NULL); //restore!!! it's dangerous here
	return ret;
}

int burn_read(int ch, struct burn_data_s *result)
{
	int ret;
	char cmd = BURN_CMD_READ;
	struct mcamos_s m;
	m.can = &can1;
	m.baud = BURN_BAUD;
	m.id_cmd = 0x5e0 + (ch << 1);
	m.id_dat = m.id_cmd + 1;

	mcamos_init_ex(&m);
	mcamos_dnload(m.can, BURN_INBOX_ADDR, &cmd, 1, 10);
	ret = burn_wait(&m, 10);
	if((ret == 0) && result) {
		ret = mcamos_upload(m.can, BURN_OUTBOX_ADDR + 2, (char *) result, sizeof(struct burn_data_s), 10);
		#ifdef CONFIG_IGBT_CRC
		if((ret == 0) && (result->crc_flag != 0)) {
			unsigned short crc = result->crc;
			result->crc = 0;
			result->crc = cyg_crc16((unsigned char *) result, sizeof(struct burn_data_s));
			if(crc != result->crc) {
				nest_message("%s: crc fail\n", __FUNCTION__);
			}
		}
		#endif
	}

	if(result->lost != 0)
			nest_message("ch = %d, crc_flag = %d, lost = %d\n", ch, result->crc_flag, result->lost);
	mcamos_init_ex(NULL); //restore!!! it's dangerous here
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
	int tp = burn_data->tp;
	int fire = burn_data->fire;
	int lost = burn_data->lost;
	nest_message("%d %d %d %d\n", wp, tp, fire, lost);
}

static int burn_calibrate(void)
{
	int ret, ch, vp, ip, vpm_ratio, ipm_ratio;
	struct burn_data_s burn_data;
	time_t timer;

	for(ch = BURN_CH_COILA; ch <= BURN_CH_COILD; ch ++) {
		if(!(burn_cal_para.ch & (1 << ch))) {
			continue;
		}

		//preset vpm/ipm_ratio_cal = 1
		ret = burn_config(ch, 1 << 12, 1 << 12, 1);
		if(ret) {
			nest_message("burn_config preset fail(ch = %d)!!!\n", ch);
			break;
		}

		timer = time_get(60000);
		while(1) {
			//get burn_data and display
			ret = burn_read(ch, &burn_data);
			if(ret) {
				nest_message("burn board channel %d not response\n", ch);
				ret = -1;
				break;
			}

			nest_message("%d ", ch);
			burn_disp(&burn_data);

			if(burn_data.lost > 10) {
				nest_message("burn board channel %d sync lost too much(%d)\n", ch, burn_data.lost);
				ret = -2;
				break;
			}

			if(burn_data.wp > BURN_WL_DEF) {
				nest_message("burn board channel %d wp(=%dnS) higher than threshold(=%dnS)\n", ch, burn_data.wp, BURN_WL_DEF);
				ret = -3;
				break;
			}

			if(burn_data.fire > 1000) { //1000 * 20mS = 20S
				//ok, got the data what we want
				ret = 0;
				break;
			}
			else {
				if(time_left(timer) < 0) {
						nest_message("burn board channel %d not fire\n", ch);
						ret = -6;
						break;
				}
			}
		}

		if(ret)
			break;

		//to calculate the ratio
		vp = (burn_data.vp_min + burn_data.vp_max) >> 1;
		ip = (burn_data.ip_min + burn_data.ip_max) >> 1;
		vpm_ratio = (burn_cal_para.vp << 12) / vp;
		ipm_ratio = (burn_cal_para.ip << 12) / ip;
		nest_message("burn cal %d: vpm_ratio_cal = %d, ipm_ratio_cal = %d\n", ch, vpm_ratio, ipm_ratio);

		//write the calibrate data back
		ret = burn_config(ch, (unsigned short)vpm_ratio, (unsigned short)ipm_ratio, 1);
		if(ret) {
			nest_message("burn_config calibrate fail(ch = %d)!!!\n", ch);
			break;
		}
	}

	burn_flag_cal = 0;
	return ret;
}

static unsigned char burn_wp[BURN_CH_NR] = {0, 0, 0, 0};
int burn_verify(unsigned short *vp, unsigned short *ip, unsigned char *wp)
{
	int ret, ch;
	struct burn_data_s burn_data;

	if(burn_flag_cal) {
		return burn_calibrate();
	}

	//ignore igbt burn test?
	if(nest_ignore(PKT))
		return 0;

	//get test result from burn board through mcamos/can protocol
	for(ch = BURN_CH_COILA; ch <= BURN_CH_COILD; ch ++) {
		ret = burn_read(ch, &burn_data);
		if(ret) {
			nest_message("burn board channel %d not response\n", ch);
			ret = -1;
			break;
		}

		if(burn_log & (1 << ch)) {
			nest_message("%d ", ch);
			burn_disp(&burn_data);
		}

		if(burn_data.lost > 10) {
			nest_message("burn board channel %d sync lost too much(%d)\n", ch, burn_data.lost);
			ret = -2;
			break;
		}

		if(burn_data.wp > BURN_WL_DEF) {
			nest_message("burn board channel %d wp(=%dnS) higher than threshold(=%dnS)\n", ch, burn_data.wp, BURN_WL_DEF);
			ret = -3;
			break;
		}

		if(burn_data.fire > 500) { //500 * 20mS = 10S
			//save measurement result to mfg_data
			if(vp != NULL)
				vp[ch] = burn_data.vp_min;
			if(ip != NULL)
				ip[ch] = burn_data.ip_max;
			if(wp != NULL) {
				unsigned char us = (unsigned char)(burn_data.wp / 1000);
				wp[ch] = burn_wp[ch] = (us > burn_wp[ch]) ? us : burn_wp[ch];
			}

			if(burn_data.vp_min < burn_vl) {
				nest_message("burn board channel %d vp(=%dV) lower than threshold(=%dV)\n", ch, burn_data.vp_min, burn_vl);
				ret = -4;
				break;
			}

			if(burn_data.ip_max > burn_il) {
				nest_message("burn board channel %d ip(=%dmA) higher than threshold(=%dmA)\n", ch, burn_data.ip_max, burn_il);
				ret = -5;
				break;
			}
			if(burn_data.ip_max < BURN_IL_MIN) {
				nest_message("burn board channel %d ip(=%dmA) lower than threshold(=%dmA)\n", ch, burn_data.ip_max, BURN_IL_MIN);
				ret = -6;
				break;
			}
		}
		else {
			if(time_left(burn_timer) < 0) {
				if((~burn_flag) & (1 << ch)) {
					nest_message("burn board channel %d not fire\n", ch);
					ret = -6;
					break;
				}
			}
		}
	}

	if((ret == -2) || (ret == -3)) { //fail asap in case of 'sync lost too much' or 'pulse width fail'
		return ret;
	}
	burn_fail_counter += (ret < 0) ? 1 : 0;
	ret = (burn_fail_counter > 30) ? ret : 0;
	return ret;
}

//burn shell command
static int cmd_burn_func(int argc, char *argv[])
{
	const char *usage = {
		"burn log 0|1|2|3|all|none		print ch 0/1/2/3/all log message\n"
		"burn vl 410				vp low threshold\n"
		"burn il 11000				ip high threshold\n"
		"burn cal ch vp(V) ip(mA)		calibration, ch=0,1-2,3,all.. \n"
		"burn save\n"
	};

	if(argc >= 2) {
		if(!strcmp(argv[1], "cal")) {
			if(argc == 5) {
				burn_cal_para.ch = cmd_pattern_get(argv[2]);
				burn_cal_para.vp = atoi(argv[3]);
				burn_cal_para.ip = atoi(argv[4]);
				burn_flag_cal = 1;
			}
			else {
				printf("%s", usage);
			}
			return 0;
		}

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

