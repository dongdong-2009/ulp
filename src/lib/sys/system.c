/* board.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sys/system.h"
#include "led.h"
#include "console.h"
#include "stm32_mac.h"
#include "spi.h"

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void NVIC_Configuration(void);
void cpu_Init(void);

void sys_Init(void)
{
	SystemInit();
	time_Init();
	led_Init();
	console_Init();
#if CONFIG_SYSTEM_SPI1 == 1
	spi_Init(1, SPI_MODE_POL_1| SPI_MODE_PHA_0| SPI_MODE_BW_16 | SPI_MODE_MSB);
#endif
#if CONFIG_SYSTEM_SPI2 == 1
	spi_Init(2, SPI_MODE_POL_0| SPI_MODE_PHA_0| SPI_MODE_BW_16 | SPI_MODE_MSB);
#endif

#if CONFIG_DRIVER_MAC_STM32 == 1
	stm32mac_Init();
#endif

	/*indicates board init finish*/
	led_on(LED_RED);
	mdelay(100);
	led_off(LED_RED);
	
	led_on(LED_GREEN);
	mdelay(100);
	led_off(LED_GREEN);
}

void sys_Update(void)
{
	led_Update();
	time_Update();
}
