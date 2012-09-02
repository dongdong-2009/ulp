/*
 * 	king@2012 initial version
 */

// #include "config.h"
// #include "ulp/debug.h"
// #include "ulp_time.h"
// #include "sys/task.h"
// #include "ulp/sys.h"
// #include "shell/cmd.h"
// #include <stdlib.h>
#include "lpc177x_8x_emc.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_clkpwr.h"

#define SDRAM_BASE_ADDR	0xA0000000
#define SDRAM_SIZE	0x10000000

static void sdram_init()
{
	unsigned int i, dwtemp;

	/* Initialize EMC */
	EMC_Init();

	//Configure memory layout, but MUST DISABLE BUFFERs during configuration
	LPC_EMC->DynamicConfig0    = 0x00000680; /* 256MB, 16Mx16, 4 banks, row=13, column=9 */

	//Timing for 48MHz Bus
	LPC_EMC->DynamicRasCas0    = 0x00000303;//???????? /* 1 RAS, 2 CAS latency */
	LPC_EMC->DynamicReadConfig = 0x00000001;//???????? /* Command delayed strategy, using EMCCLKDELAY */
	LPC_EMC->DynamicRP         = 0x00000002; /* ( n + 1 ) -> 3 clock cycles */
	LPC_EMC->DynamicRAS        = 0x00000005; /* ( n + 1 ) -> 6 clock cycles */
	LPC_EMC->DynamicSREX       = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
	LPC_EMC->DynamicAPR        = 0x00000000;//???????? /* ( n + 1 ) -> 1 clock cycles */
	LPC_EMC->DynamicDAL        = 0x00000004; /* ( n ) -> 5 clock cycles */
	LPC_EMC->DynamicWR         = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
	LPC_EMC->DynamicRC         = 0x00000007; /* ( n + 1 ) -> 8 clock cycles */
	LPC_EMC->DynamicRFC        = 0x00000007; /* ( n + 1 ) -> 8 clock cycles */
	LPC_EMC->DynamicXSR        = 0x00000004;//???????? /* ( n + 1 ) -> 5 clock cycles */
	LPC_EMC->DynamicRRD        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
	LPC_EMC->DynamicMRD        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */

	mdelay(100);						   /* wait 100ms */
	LPC_EMC->DynamicControl    = 0x00000183; /* Issue NOP command */

	mdelay(200);						   /* wait 200ms */
	LPC_EMC->DynamicControl    = 0x00000103; /* Issue PALL command */
	LPC_EMC->DynamicRefresh    = 0x00000002;//???????? /* ( n * 16 ) -> 32 clock cycles */

	for(i = 0; i < 0x80; i++);	           /* wait 128 AHB clock cycles */

	//Timing for 60MHz Bus
	LPC_EMC->DynamicRefresh    = 25; /* ( n * 16 ) -> 400 clock cycles -> 6.667uS at 60MHz <= 7.8125uS ( 64ms / 8192 row ) */

	LPC_EMC->DynamicControl    = 0x00000083; /* Issue MODE command */

	//Timing for 48/60/72MHZ Bus
	dwtemp = *((volatile unsigned int *)(SDRAM_BASE_ADDR | (0x33<<12))); /* 8 burst, 3 CAS latency */
	LPC_EMC->DynamicControl    = 0x00000000; /* Issue NORMAL command */

	//[re]enable buffers
	LPC_EMC->DynamicConfig0    = 0x00080680; /* 256MB, 16Mx16, 4 banks, row=13, column=9 */
}

/*
 * 	use USB clock to drive ethernet
 */
static void ethernet_clk_init()
{
	PINSEL_ConfigPin(1, 27, 4);
	LPC_SC->PLL1CON = 0x00;			/* PLL1 Disable */
	LPC_SC->PLL1CFG = 24;
	LPC_SC->PLL1CON = 0x01;			/* PLL1 Enable */
	LPC_SC->PLL1FEED = 0xAA;

	LPC_SC->PLL1FEED = 0x55;
	while (!(LPC_SC->PLL1STAT & (1 << 10)));		/* Wait for PLOCK1 */
	LPC_SC->USBCLKSEL = (0x00000006 | (0x02 << 8)); /* Setup USB Clock Divider */
	LPC_SC->CLKOUTCFG = 3 | 0x00000100;			//set USB clock as clkout
}

void bsp_init()
{
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);		//enable gpio clk
	ethernet_clk_init();
	sdram_init();
}
