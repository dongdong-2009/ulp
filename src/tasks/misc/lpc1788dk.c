/*
 * this is the demo routine for nxp lpc1788 develop board
 * SDRAM: MT48LC8M32LFB5 32MB, 32BIT
 * CONSOLE: UART0 115200BPS
 *
 * miaofng@2012 stack is moved to sdram, size 32MB
 *
 */

#include "config.h"
#include "ulp/sys.h"

#include "lpc177x_8x_emc.h"
#include "lpc177x_8x_pinsel.h"

#define SDRAM_BASE_ADDR	0xA0000000
#define SDRAM_SIZE	0x10000000
void bsp_init(void)
{
	uint32_t i, dwtemp;

	/* Initialize EMC */
	EMC_Init();

	//Configure memory layout, but MUST DISABLE BUFFERs during configuration
	LPC_EMC->DynamicConfig0    = 0x00004480; /* 256MB, 8Mx32, 4 banks, row=12, column=9 */

	/*Configure timing for  Micron SDRAM MT48LC8M32LFB5-8 */

	//Timing for 48MHz Bus
	LPC_EMC->DynamicRasCas0    = 0x00000201; /* 1 RAS, 2 CAS latency */
	LPC_EMC->DynamicReadConfig = 0x00000001; /* Command delayed strategy, using EMCCLKDELAY */
	LPC_EMC->DynamicRP         = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
	LPC_EMC->DynamicRAS        = 0x00000002; /* ( n + 1 ) -> 3 clock cycles */
	LPC_EMC->DynamicSREX       = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
	LPC_EMC->DynamicAPR        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
	LPC_EMC->DynamicDAL        = 0x00000002; /* ( n ) -> 2 clock cycles */
	LPC_EMC->DynamicWR         = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
	LPC_EMC->DynamicRC         = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
	LPC_EMC->DynamicRFC        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
	LPC_EMC->DynamicXSR        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
	LPC_EMC->DynamicRRD        = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
	LPC_EMC->DynamicMRD        = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */

	mdelay(100);						   /* wait 100ms */
	LPC_EMC->DynamicControl    = 0x00000183; /* Issue NOP command */

	mdelay(200);						   /* wait 200ms */
	LPC_EMC->DynamicControl    = 0x00000103; /* Issue PALL command */
	LPC_EMC->DynamicRefresh    = 0x00000002; /* ( n * 16 ) -> 32 clock cycles */

	for(i = 0; i < 0x80; i++);	           /* wait 128 AHB clock cycles */

	//Timing for 48MHz Bus
	LPC_EMC->DynamicRefresh    = 0x0000002E; /* ( n * 16 ) -> 736 clock cycles -> 15.330uS at 48MHz <= 15.625uS ( 64ms / 4096 row ) */

	LPC_EMC->DynamicControl    = 0x00000083; /* Issue MODE command */

	//Timing for 48/60/72MHZ Bus
	dwtemp = *((volatile uint32_t *)(SDRAM_BASE_ADDR | (0x22<<(2+2+9)))); /* 4 burst, 2 CAS latency */
	LPC_EMC->DynamicControl    = 0x00000000; /* Issue NORMAL command */

	//[re]enable buffers
	LPC_EMC->DynamicConfig0    = 0x00084480; /* 256MB, 8Mx32, 4 banks, row=12, column=9 */
}

void lpc_init(void)
{
	int *p = sys_malloc(1000000);
	memset(p, 0x99, 32);
}

extern void lwip_lib_isr(void);
void lpc_update(void)
{
}

void main(void)
{
	sys_init();
	lpc_init();
	while(1) {
		sys_update();
		lpc_update();
	}
}

