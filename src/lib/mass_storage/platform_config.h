/*
 *	dusk@initial version
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PLATFORM_CONFIG_H
#define __PLATFORM_CONFIG_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

#define USE_STM3210B_EVAL

#define USB_DISCONNECT                    GPIOD  
#define USB_DISCONNECT_PIN                GPIO_Pin_2//GPIO_Pin_9
#define RCC_APB2Periph_GPIO_DISCONNECT    RCC_APB2Periph_GPIOD

#endif /* __PLATFORM_CONFIG_H */
