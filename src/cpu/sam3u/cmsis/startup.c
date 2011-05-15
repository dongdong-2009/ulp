/* miaofng@2011, ori from atmel board_lowlevel.c
*/

#include "config.h"
#include "sam3u.h"

static void SetDefaultMaster(unsigned char enable);

/* Settings at 48/48MHz */
#define AT91C_CKGR_MUL_SHIFT		 16
#define AT91C_CKGR_OUT_SHIFT		 14
#define AT91C_CKGR_PLLCOUNT_SHIFT	 8
#define AT91C_CKGR_DIV_SHIFT		  0

#define BOARD_OSCOUNT		 (AT91C_CKGR_MOSCXTST & (0x3F << 8))
#define BOARD_PLLR ((1 << 29) | (0x7 << AT91C_CKGR_MUL_SHIFT) | (0x0 << AT91C_CKGR_OUT_SHIFT) | \
	(0x3f << AT91C_CKGR_PLLCOUNT_SHIFT) | (0x1 << AT91C_CKGR_DIV_SHIFT))
#define BOARD_MCKR ( AT91C_PMC_PRES_CLK_2 | AT91C_PMC_CSS_PLLA_CLK )

/* Define clock timeout */
#define CLOCK_TIMEOUT		   0xFFFFFFFF

/*
	After POR, at91sam3u device is running on 4MHz internal RC
	Performs the low-level initialization of the chip. This includes EFC, master
	clock, IRQ & watchdog configuration.
*/
int __low_level_init( void )
{
	unsigned int timeout = 0;

	/* Set 2 WS for Embedded Flash Access */
	AT91C_BASE_EFC0->EFC_FMR = AT91C_EFC_FWS_2WS;
	AT91C_BASE_EFC1->EFC_FMR = AT91C_EFC_FWS_2WS;

	/* Watchdog initialization */
	AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;

	/* Select external slow clock */
	if ((AT91C_BASE_SUPC->SUPC_SR & AT91C_SUPC_SR_OSCSEL_CRYST) != AT91C_SUPC_SR_OSCSEL_CRYST) {
		AT91C_BASE_SUPC->SUPC_CR = AT91C_SUPC_CR_XTALSEL_CRYSTAL_SEL | (0xA5 << 24);
		timeout = 0;
		while (!(AT91C_BASE_SUPC->SUPC_SR & AT91C_SUPC_SR_OSCSEL_CRYST) && (timeout++ < CLOCK_TIMEOUT));
	}

	/* Initialize main oscillator */
	if(!(AT91C_BASE_PMC->PMC_MOR & AT91C_CKGR_MOSCSEL)) {
		AT91C_BASE_PMC->PMC_MOR = (0x37 << 16) | BOARD_OSCOUNT | AT91C_CKGR_MOSCRCEN | AT91C_CKGR_MOSCXTEN;
		timeout = 0;
		while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCXTS) && (timeout++ < CLOCK_TIMEOUT));
	}

	/* Switch to moscsel */
	AT91C_BASE_PMC->PMC_MOR = (0x37 << 16) | BOARD_OSCOUNT | AT91C_CKGR_MOSCRCEN | AT91C_CKGR_MOSCXTEN | AT91C_CKGR_MOSCSEL;
	timeout = 0;
	while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCSELS) && (timeout++ < CLOCK_TIMEOUT));
	AT91C_BASE_PMC->PMC_MCKR = (AT91C_BASE_PMC->PMC_MCKR & ~AT91C_PMC_CSS) | AT91C_PMC_CSS_MAIN_CLK;
	timeout = 0;
	while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY) && (timeout++ < CLOCK_TIMEOUT));

	/* Initialize PLLA */
	AT91C_BASE_PMC->PMC_PLLAR = BOARD_PLLR;
	timeout = 0;
	while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCKA) && (timeout++ < CLOCK_TIMEOUT));

	/* Initialize UTMI for USB usage */
	AT91C_BASE_CKGR->CKGR_UCKR |= (AT91C_CKGR_UPLLCOUNT & (3 << 20)) | AT91C_CKGR_UPLLEN;
	timeout = 0;
	while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCKU) && (timeout++ < CLOCK_TIMEOUT));

	/* Switch to fast clock */
	AT91C_BASE_PMC->PMC_MCKR = (BOARD_MCKR & ~AT91C_PMC_CSS) | AT91C_PMC_CSS_MAIN_CLK;
	timeout = 0;
	while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY) && (timeout++ < CLOCK_TIMEOUT));

	AT91C_BASE_PMC->PMC_MCKR = BOARD_MCKR;
	timeout = 0;
	while (!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY) && (timeout++ < CLOCK_TIMEOUT));

	/* Enable clock for UART */
	AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_DBGU);

	/* Optimize CPU setting for speed */
	SetDefaultMaster(1);
	return 1;
}

/* Enable or disable default master access */
void SetDefaultMaster(unsigned char enable)
{
	AT91PS_HMATRIX2 pMatrix = AT91C_BASE_MATRIX;

	// Set default master
	if (enable == 1) {
		/* Set default master: SRAM0 -> Cortex-M3 System */
		pMatrix->HMATRIX2_SCFG0 |= AT91C_MATRIX_FIXED_DEFMSTR_SCFG0_ARMS | \
			AT91C_MATRIX_DEFMSTR_TYPE_FIXED_DEFMSTR;

		/* Set default master: SRAM1 -> Cortex-M3 System */
		pMatrix->HMATRIX2_SCFG1 |= AT91C_MATRIX_FIXED_DEFMSTR_SCFG1_ARMS | \
			AT91C_MATRIX_DEFMSTR_TYPE_FIXED_DEFMSTR;

		/* Set default master: Internal flash0 -> Cortex-M3 Instruction/Data */
		pMatrix->HMATRIX2_SCFG3 |= AT91C_MATRIX_FIXED_DEFMSTR_SCFG3_ARMC | \
			AT91C_MATRIX_DEFMSTR_TYPE_FIXED_DEFMSTR;
	} else {
		/* Clear default master: SRAM0 -> Cortex-M3 System */
		pMatrix->HMATRIX2_SCFG0 &= (~AT91C_MATRIX_DEFMSTR_TYPE);

		// Clear default master: SRAM1 -> Cortex-M3 System
		pMatrix->HMATRIX2_SCFG1 &= (~AT91C_MATRIX_DEFMSTR_TYPE);

		// Clear default master: Internal flash0 -> Cortex-M3 Instruction/Data
		pMatrix->HMATRIX2_SCFG3 &= (~AT91C_MATRIX_DEFMSTR_TYPE);
	}
}
