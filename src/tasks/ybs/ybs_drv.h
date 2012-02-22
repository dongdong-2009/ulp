/*
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */

#ifndef __YBS_DRV_H_
#define __YBS_DRV_H_

#define VREF_DAC 2.5007 //unit: V
#define VREF_ADC 3.2905 //unit: V
#define VOUT_OFS 2.0000 //unit: V


//timer ctrl
void ybs_tim_set(int on);
#define ybs_start() ybs_tim_set(1)
#define ybs_stop() ybs_tim_set(0)

//mosfet
void ybs_mos_set(int on);
#define ybs_stress_press() ybs_mos_set(0)
#define ybs_stress_release() ybs_mos_set(1)

int ybs_drv_init(void);
int ybs_set_vr(int v); //set Vref
int ybs_set_vo(int v);
int ybs_get_vi(void);
int ybs_get_vi_mean(void);

#endif
