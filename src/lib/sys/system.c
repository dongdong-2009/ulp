/* board.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sys/system.h"
#include "led.h"
#include "console.h"
#include "spi.h"

void sys_Init(void)
{
	
	SystemInit();
	time_Init();
	led_Init();
#if (CONFIG_IAR_REDIRECT == 1) || (CONFIG_TASK_SHELL == 1)
	console_Init();
#endif

#if CONFIG_SYSTEM_SPI1 == 1
	int stm32_spi1_mode = 0;
#ifdef CONFIG_STM32_SPI1_LEN_8
	stm32_spi1_mode |= SPI_MODE_BW_8;	//data len 8 bits
#else
	stm32_spi1_mode |= SPI_MODE_BW_16;	//default 16 bits
#endif
#ifdef CONFIG_STM32_SPI1_PRESCALER_2
	stm32_spi1_mode |= SPI_MODE_PRESCALER_2;	//prescaler_2
#else
	stm32_spi1_mode |= SPI_MODE_PRESCALER_4;	//default prescaler_4
#endif
	spi_Init(1, SPI_MODE_POL_1| SPI_MODE_PHA_0| stm32_spi1_mode | SPI_MODE_MSB);
#endif

#if CONFIG_SYSTEM_SPI2 == 1
	int stm32_spi2_mode = 0;
#ifdef CONFIG_STM32_SPI2_LEN_8
	stm32_spi2_mode |= SPI_MODE_BW_8;	//data len 8 bits
#else
	stm32_spi2_mode |= SPI_MODE_BW_16;	//default 16 bits
#endif
#ifdef CONFIG_STM32_SPI2_PRESCALER_2
	stm32_spi2_mode |= SPI_MODE_PRESCALER_2;	//prescaler_2
#else
	stm32_spi2_mode |= SPI_MODE_PRESCALER_4;	//default prescaler_4
#endif
	spi_Init(2, SPI_MODE_POL_0| SPI_MODE_PHA_0| stm32_spi2_mode | SPI_MODE_MSB);
#endif
}

void sys_Update(void)
{
	led_Update();
	time_Update();
}
