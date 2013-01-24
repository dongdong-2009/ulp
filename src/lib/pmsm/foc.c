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

typedef struct {
	int status;
	int frq;
	int crp;
	int pid_update; //pid has been completed
	int execute_update; //SVMWM module has executed the changes
	pmsmd_ops_t pmsmd;
	pmsm_t pmsm;

	struct {
		int theta_per_counter_q10;

		int theta_before;
		int theta_now;
		int theta_expect;
		int theta_next;
		int theta_diff;
		int theta_diff_max;

		int speed_now;
		int speed_expect;
		int speed_calc_cnt;
		int speed_calc_cnt_max;
		int speed_theta_diff;

		int acclr_now;
		int acclr_expect;
	} mech;

	struct {
		int theta;
		int ia;
		int ib;
		int ic;
		int id;
		int iq;
		int ud_expect;
		int uq_expect;
		int ualpha;
		int ubeta;
		int tpwm;
		int tpwm_multi_sqrt3;
	} elec;

	struct pid_q15_s id_pid;
	struct pid_q15_s iq_pid;
	struct pid_q15_s speed_pid;
	struct pid_q15_s theta_pid;
	struct pid_q15_s acclr_pid;
} foc_t;

static const int sector_decide[8] = {0, 2, 6, 1, 4, 3, 5, 0};

	/*
	 * theta:[-1:1]~[-180:180]
	 * theta_q15:[-32768:32768]~[-180:180]
	 */
static void theta_get(foc_t *dev)
{
	int a;
	int b;
	a = (dev->mech.theta_per_counter_q10) * dev->pmsmd.encoder_get();
	b = (a >> 10) - 0x00008000;
	dev->mech.theta_now = b;
	b += 0x000100000;
	dev->elec.theta = ((b * dev->pmsm.pp + 0x00008000) & 0x0000ffff) - 0x00008000;
}

static void theta_set(foc_t *dev, int theta_mech)
{
	dev->pmsmd.encoder_set(((theta_mech + 0x00008000) << 10) / dev->mech.theta_per_counter_q10);
}

static void speed_get(foc_t *dev)
{















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
	 * 	sqrt(3)/2 = 0.86602540378444 = 0x00006ED9(Q15)
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
		break;
	case READY:
		dev->status = RUN;
		break;
	case RUN:
		if(dev->pid_update == 0) {
			//dev.elec.uq_expect = pid(&dev.math.speed_pid, 1000 - dev.mech.speed_now);
			//id_temp = pid(speed_pid, dev.mech.speed_expect - dev.mech.speed_now);
			//ud_temp = pid(id_pid, id_temp - dev.elec.id);
			//uq_temp = pid(iq_pid, 0 - dev.elec.iq);
			dev->elec.ud_expect = 0;
			dev->elec.uq_expect = 0x00000DE6;
			dev->pid_update = 1;
		}
		break;
	case STOP:
		dev->pmsmd.svpwm_ctl(0);
		svpwm_set(dev, 0, 0);
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
	foc_t *dev = (foc_t *)handle;
	int sinval, cosval;
	int ud, uq, ualpha, ubeta;

	//mech_theta_before = mech_theta;

	theta_get(dev);
	speed_get(dev);

	sin_cos_q15(dev->elec.theta, &sinval, &cosval);
	//dev->mech.theta_now_temp = dev->mech.theta_now / 182;
	if(dev->pid_update) {
		ud = dev->elec.ud_expect;
		uq = dev->elec.uq_expect;
		dev->pid_update = 0;
	}
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
