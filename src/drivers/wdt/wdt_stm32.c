/*
 * david@2012 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "wdt.h"
#include <stdlib.h>
#include <string.h>
#include "ulp_time.h"
#include "shell/cmd.h"

/*
 *init the watch dog reset period(ms) time
 *the range of watch dog update time is 1 - 800(ms)
 *set the cpu automaticly stop the wdt when in debug mode
 */
int wdt_init(int period)
{
#ifdef CONFIG_DRIVER_WDT

	if (period < 1 || period > 800)
		return 1;

	//disable wdt when in debug mode, this is a good point
	DBGMCU_Config(DBGMCU_IWDG_STOP, ENABLE);

	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	/* IWDG counter clock: 40KHz(LSI) / 8 = 5 KHz */
	IWDG_SetPrescaler(IWDG_Prescaler_8);
	/* Set counter reload value to 349 */
	IWDG_SetReload(5 * period);
	/* Reload IWDG counter */
	IWDG_ReloadCounter();
#ifdef CONFIG_STM32_WDTSW
	IWDG_Enable();
#endif
#ifdef CONFIG_STM32_WDTHW_AUTO
	unsigned int temp;
	//FLASH User Option Bytes values:IWDG_SW(Bit0), RST_STOP(Bit1) and RST_STDBY(Bit2).
	temp = FLASH_GetUserOptionByte();
	if (temp & OB_IWDG_SW)
		wdt_enable();
#endif

#ifdef CONFIG_STM32_WDTHW_AUTO_OFF
	unsigned int temp;
	//FLASH User Option Bytes values:IWDG_SW(Bit0), RST_STOP(Bit1) and RST_STDBY(Bit2).
	temp = FLASH_GetUserOptionByte();
	temp &= OB_IWDG_SW;
	if (temp != OB_IWDG_SW) {
		//disable hw wdt on flash
		wdt_disable();

		//reboot, there is no other method to stop wdt ..
		void (*reset)(void);
		reset = 0;
		reset();
	}
#endif

#endif
	return 0;
}

void wdt_update(void)
{
#ifdef CONFIG_DRIVER_WDT
#if CONFIG_STM32_WDTHW_AUTO_OFF != 1
	IWDG_ReloadCounter();
#endif
#endif
}

//enable the watch time via "hardware option bit" in flash
int wdt_enable(void)
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_EraseOptionBytes();
	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	if ( FLASH_UserOptionByteConfig(OB_IWDG_HW, OB_STOP_NoRST, OB_STDBY_NoRST) == FLASH_COMPLETE) {
		FLASH_Lock();
		return 0;
	} else {
		FLASH_Lock();
		return 1;
	}
}

//disable the watch time via "hardware option bit" in flash
int wdt_disable(void)
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_EraseOptionBytes();
	if ( FLASH_UserOptionByteConfig(OB_IWDG_SW, OB_STOP_NoRST, OB_STDBY_NoRST) == FLASH_COMPLETE) {
		FLASH_Lock();
		return 0;
	} else {
		FLASH_Lock();
		return 1;
	}
}

#if 1

int cmd_wdt_func(int argc, char *argv[])
{
	int temp;

	const char * usage = { \
		" usage:\n" \
		" wdt init pd(ms), init the watch with period(int ms)\n" \
		" wdt enable,      enable iwdt in flash option bit\n"
		" wdt disable,     disable iwdt in flash option bit\n"
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (argv[1][0] == 'i') {
		sscanf(argv[2], "%d", &temp);
		if (wdt_init(temp))
			printf("Init failed!\n");
		else
			printf("Init Ok!\n");
	}

	if (argv[1][0] == 'e') {
		if (wdt_enable())
			printf("operation failed!\n");
		else
			printf("operation successful!\n");
	}

	if (argv[1][0] == 'd') {
		if (wdt_disable())
			printf("operation failed!\n");
		else
			printf("operation successful!\n");
	}

	return 0;
}

const cmd_t cmd_wdt = {"wdt", cmd_wdt_func, "wdt cmds"};
DECLARE_SHELL_CMD(cmd_wdt)
#endif
