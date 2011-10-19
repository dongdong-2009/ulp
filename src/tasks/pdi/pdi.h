#ifndef __PDI_H_
#define __PDI_H_

#include "Mbi5025.h"

void pdi_init(void);
void pdi_update(void);
int pdi_check(const struct pdi_cfg_s *sr);
int pdi_mdelay(int ms);
int pdi_pass_action();
int pdi_fail_action();

#endif
