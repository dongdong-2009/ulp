/*
 * 	king@2013 initial version
 */

#include "stm32f10x.h"
#include "ulp_time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pmsm/foc.h"
#include "pmsm/math.h"
#include "pmsm/pmsm.h"
#include "ulp/pmsmd.h"
#include "common/pid.h"
#include <stdarg.h>

enum {
	READY,
	FIX_THETA,
	RUN,
	STOP,
	STANDBY,
};

static const int sector_decide[8] = {0, 2, 6, 1, 4, 3, 5, 0};

	/*
	 * theta:[-1:1]~[-180:180]
	 * theta_q15:[-32768:32768]~[-180:180]
	 */
static void theta_get(foc_t *dev)
{
	int a;
	int now, before, diff;
	a = (dev->mech.theta_per_counter_q10) * dev->pmsmd.encoder_get();
	now = (a >> 10) - 0x00008000;
	before = dev->mech.theta_now;

	if(now >= before) {
		diff = now - before;
		if(diff > (1 << 15)) {
			diff = now - before - (1 << 16);
		}
	}
	else if(now < before) {
		diff = now - before;
		if(diff < -(1 << 15)) {
			diff = now + (1 << 16) - before;
		}
	}
	dev->mech.theta_diff += diff;
	dev->mech.theta_before = before;
	dev->mech.theta_now = now;
	now += 0x000100000;
	dev->elec.theta = ((now * dev->pmsm.pp + 0x00008000) & 0x0000ffff) - 0x00008000;
}

static void theta_set(foc_t *dev, int theta_mech)
{
	dev->pmsmd.encoder_set(((theta_mech + 0x00008000) << 10) / dev->mech.theta_per_counter_q10);
}

void speed_get(foc_t *dev, int ms)
{
	dev->mech.speed_now = dev->mech.theta_diff * 1875 / ms / (1 << 11); // dev->mech.theta_diff * 60000 / ms / (1 << 16);
	dev->mech.theta_diff = 0;
}

static void I_get(foc_t *dev, int *iu, int *iv)
{
	dev->pmsmd.I_get(iu, iv);
	*iu -= 2048;
	*iv -= 2048;
}

/*
 * 	alpha:per-unit(Q15)
 * 	beta:per-unit(Q15)
 */
static void svpwm_set(foc_t *dev, int alpha, int beta)
{
	int u1, u2, u3;
	int sector_now;
	int temp;
	int t[8], ch[3];
	int tpwm = dev->elec.tpwm;
	int tpwm_multi_sqrt3 = dev->elec.tpwm_multi_sqrt3;

	/*
	 * 	u1 = beta
	 * 	u2 = sqrt(3)*alpha/2 - beta/2
	 * 	u3 = -sqrt(3)*alpha/2 - beta/2
	 *
	 * 	sqrt(3)/2 = 0.86602540378444 = 0x00006ED9(Q15) ×î´ó4E62 ?? sqrt(3)/2/sqrt(2)
	 */

	u1 = beta;
	u2 = ((0x00006ED9 * alpha) >> 15) - (beta / 2);
	u3 = ((-0x00006ED9 * alpha) >> 15) - (beta / 2);

	/* 	decide the sector	 */

	temp = u1 >= 0 ? 1 : 0;
	temp |= u2 >= 0 ? 2 : 0;
	temp |= u3 >= 0 ? 4 : 0;
	sector_now = sector_decide[temp];

	/*
	 * 	calculate time and config timer1 registers
	 */

	switch(sector_now) {
	case 0:
		ch[0] = tpwm;
		ch[1] = tpwm;
		ch[2] = tpwm;
		break;
	case 1:
		t[4] = (tpwm_multi_sqrt3 * u2) >> 15;
		t[6] = (tpwm_multi_sqrt3 * u1) >> 15;
		t[7] = (tpwm - t[4] - t[6]) >> 1;
		ch[2] = t[7];
		ch[1] = ch[2] + t[6];
		ch[0] = ch[1] + t[4];
		break;
	case 2:
		t[2] = (-tpwm_multi_sqrt3 * u2) >> 15;
		t[6] = (-tpwm_multi_sqrt3 * u3) >> 15;
		t[7] = (tpwm - t[2] - t[6]) >> 1;
		ch[2] = t[7];
		ch[0] = ch[2] + t[6];
		ch[1] = ch[0] + t[2];
		break;
	case 3:
		t[2] = (tpwm_multi_sqrt3 * u1) >> 15;
		t[3] = (tpwm_multi_sqrt3 * u3) >> 15;
		t[7] = (tpwm - t[2] - t[3]) >> 1;
		ch[0] = t[7];
		ch[2] = ch[0] + t[3];
		ch[1] = ch[2] + t[2];
		break;
	case 4:
		t[1] = (-tpwm_multi_sqrt3 * u1) >> 15;
		t[3] = (-tpwm_multi_sqrt3 * u2) >> 15;
		t[7] = (tpwm - t[1] - t[3]) >> 1;
		ch[0] = t[7];
		ch[1] = ch[0] + t[3];
		ch[2] = ch[1] + t[1];
		break;
	case 5:
		t[1] = (tpwm_multi_sqrt3 * u3) >> 15;
		t[5] = (tpwm_multi_sqrt3 * u2) >> 15;
		t[7] = (tpwm - t[1] - t[5]) >> 1;
		ch[1] = t[7];
		ch[0] = ch[1] + t[5];
		ch[2] = ch[0] + t[1];
		break;
	case 6:
		t[4] = (-tpwm_multi_sqrt3 * u3) >> 15;
		t[5] = (-tpwm_multi_sqrt3 * u1) >> 15;
		t[7] = (tpwm - t[4] - t[5]) >> 1;
		ch[1] = t[7];
		ch[2] = ch[1] + t[5];
		ch[0] = ch[2] + t[4];
		break;
	case 7:
		ch[0] = tpwm;
		ch[1] = tpwm;
		ch[2] = tpwm;
		break;
	default :
		//error
		break;
	}
	dev->pmsmd.time_set(ch[0], ch[1], ch[2]);
}

int foc_new(void)
{
	unsigned int a = sizeof(foc_t);
	foc_t *new = (foc_t*)malloc(a);
	memset(new, 0, a);
	return (int)new;
}

void foc_cfg(int handle, ...)
{
	va_list ap;
	char *p;

	foc_t *dev = (foc_t *)handle;

	va_start(ap, handle);

	do {
		p = va_arg(ap, char *);

		if(strstr(p, "end")) {
			break;
		}
		else if(strstr(p, "frq")) {
			dev->frq = va_arg(ap, int);
		}
		else if(strstr(p, "pmsm")) {
			dev->pmsm = *(va_arg(ap, const pmsm_t *));
		}
		else if(strstr(p, "board")) {
			dev->pmsmd = *va_arg(ap, const pmsmd_ops_t *);
		}
		else if(strstr(p, "id_pid")) {
			dev->id_pid = *va_arg(ap, struct pid_q15_s *);
		}
		else if(strstr(p, "iq_pid")) {
			dev->iq_pid = *va_arg(ap, struct pid_q15_s *);
		}
		else if(strstr(p, "speed_pid")) {
			dev->speed_pid = *va_arg(ap, struct pid_q15_s *);
		}
		else if(strstr(p, "theta_pid")) {
			dev->theta_pid = *va_arg(ap, struct pid_q15_s *);
		}
		else if(strstr(p, "acclr_pid")) {
			dev->acclr_pid = *va_arg(ap, struct pid_q15_s *);
		}
		else {
			//error
			break;
		}

	} while(1);

	va_end(ap);
}



void foc_init(int handle)
{
	int temp[2];
	foc_t *dev = (foc_t *)handle;
	dev->pid_flag = 0;
	dev->pmsmd.init(dev->pmsm.crp, dev->frq, &temp[0], &temp[1]);
	dev->elec.tpwm = temp[0];
	dev->crp = temp[1];
	dev->elec.tpwm_multi_sqrt3 = (0x00006ED9 * temp[0]) >> 14; //sqrt(3) = 0x00006ED9(Q14);
	dev->mech.theta_per_counter_q10 = (0x00010000 << 10) / temp[1];

	dev->pmsmd.encoder_ctl(0);
	dev->pmsmd.svpwm_ctl(0);
	dev->pmsmd.isr_ctl(0);

	pid_q15_init(&dev->id_pid);
	pid_q15_init(&dev->iq_pid);
	pid_q15_init(&dev->speed_pid);

	dev->status = FIX_THETA;
}

void foc_update(int handle)
{
	foc_t *dev = (foc_t *)handle;
	switch(dev->status) {
	case FIX_THETA:
		svpwm_set(dev, 0x000009E6, 0);
		dev->pmsmd.svpwm_ctl(1);
		mdelay(1000);
		theta_set(dev, 0);
		dev->pmsmd.encoder_ctl(1);
		svpwm_set(dev, 0, 0);
		dev->pmsmd.isr_ctl(1);
		dev->status = READY;
		printf("ok!\n");
		break;
	case READY:
		dev->status = RUN;
		break;
	case RUN:
		break;
	case STOP:
		svpwm_set(dev, 0, 0);
		mdelay(1000);
		dev->pmsmd.svpwm_ctl(0);
		dev->pmsmd.isr_ctl(0);
		dev->status = STANDBY;
		break;
	case STANDBY:
		break;
	default:
		break;
	}
}

void foc_isr(int handle)
{
	static int ud, uq;
	static int cnt, iq_expect;
	foc_t *dev = (foc_t *)handle;
	int sinval, cosval;
	int iu, iv;
	int id, iq;
	int ualpha, ubeta;
	int ialpha, ibeta;

	theta_get(dev);

	sin_cos_q15(dev->elec.theta, &sinval, &cosval);

	if(++cnt >= 20) {
		speed_get(dev, 1);
		//iq_expect = pid_q15(&dev->speed_pid, dev->mech.speed_expect - dev->mech.speed_now);
		cnt = 0;
	}
	I_get(dev, &iu, &iv);
	clarke_q15(iu, iv, &ialpha, &ibeta);
	park_q15(ialpha, ibeta, &id, &iq, sinval, cosval);DAC_SetChannel1Data(DAC_Align_12b_R, 10*iq+2048);DAC_SetChannel2Data(DAC_Align_12b_R, 10*id+2048);
	ud = dev->mech.speed_expect;//pid_q15(&dev->id_pid, -id);
	uq = 0;//dev->mech.speed_expect;//pid_q15(&dev->iq_pid, dev->mech.speed_expect - iq);//uq = pid_q15(&dev->iq_pid, iq_expect - iq);
	inv_park_q15(ud, uq, &ualpha, &ubeta, sinval, cosval);
	svpwm_set(dev, ualpha, ubeta);
}


void foc_speed_set(int handle, int rpm)
{
	((foc_t *)handle)->mech.speed_expect = rpm;
}

void foc_theta_set(int handle, int theta)
{
	((foc_t *)handle)->mech.theta_expect = theta;
}

void foc_acclr_set(int handle, int acclr)
{
	((foc_t *)handle)->mech.acclr_expect = acclr;
}
