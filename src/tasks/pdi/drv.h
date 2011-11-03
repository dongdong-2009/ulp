#ifndef __PDI_DRV_H_
#define __PDI_DRV_H_

int pdi_drv_Init();
int led_fail_on();
int led_fail_off();
int led_pass_on();
int led_pass_off();
int beep_on();
int beep_off();
int check_start();
int counter_fail_add();
int counter_pass_add();
int target_on();
int pdi_swcan_mode();
int pdi_batt_on();
int pdi_batt_off();
int pdi_IGN_on();
int pdi_IGN_off();
int start_botton_on();
int start_botton_off();
int init_OK();
int pdi_mdelay(int );

#endif
