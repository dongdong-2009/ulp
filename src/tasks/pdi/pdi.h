#ifndef __PDI_PDI_H_
#define __PDI_PDI_H_
//public
void pdi_fail_action();
void pdi_pass_action();
void pdi_noton_action();
void counter_fail_add();
void counter_pass_add();
void pre_check_action();
void start_action();
void pdi_relay_init();
void pdi_set_relays();

//private
int pdi_startsession();
int pdi_clear_dtc();
int pdi_mdelay(int );

#endif