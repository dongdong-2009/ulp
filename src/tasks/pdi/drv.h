#ifndef __PDI_DRV_H_
#define __PDI_DRV_H_

int pdi_drv_Init();
int beep_on();
int beep_off();
int check_start();
int counter_fail_rise();
int counter_fail_down();
int counter_pass_rise();
int counter_pass_down();
int target_on();
int pdi_swcan_mode();
int pdi_batt_on();
int pdi_batt_off();
int pdi_IGN_on();
int pdi_IGN_off();
int start_botton_on();
int start_botton_off();
int JAMA_on();

#endif
