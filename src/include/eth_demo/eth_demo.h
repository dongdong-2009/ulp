/*
 * 	dusk@2010 initial version
 */

#ifndef __ETH_DEMO_H__
#define __ETH_DEMO_H__

#define SYSTEMTICK_PERIOD_MS  50

void eth_demo_Init(void);
void eth_demo_Update(void);
void eth_demo_isr(void);
void eth_demo_systick_isr(void);

#endif /* __ETH_DEMO_H__ */