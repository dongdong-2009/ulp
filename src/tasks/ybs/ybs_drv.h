/*
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */

#ifndef __YBS_DRV_H_
#define __YBS_DRV_H_

int ybs_drv_init(void);
int ybs_set_vr(int v); //set Vref
int ybs_set_vo(int v);
int ybs_get_vi(void);
int ybs_get_vi_mean(void);

//mosfet
int ybs_mos_set(int on);
#define ybs_mos_on() ybs_mos_set(1)
#define ybs_mos_off() ybs_mos_set(0)

#endif
