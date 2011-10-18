#ifndef __PDI_DRV_H_
#define __PDI_DRV_H_

int pin_init();
int led_fail_on();
int led_fail_off();
int led_pass_on();
int led_pass_off();
int beep_on();
int beep_off();
int check_start();
int counter_fail_add();
int counter_total_add();
int target_on();
int pdi_swcan_mode();


#endif
