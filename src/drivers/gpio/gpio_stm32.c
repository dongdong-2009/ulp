/*
*
*  miaofng@2016-09-12 routine for usbhub ctrl board v2.x
*  miaofng@2017-07-15 remove hash algo
*  limit:
*  at most support 32 gpio lines
*
*/

#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "common/bitops.h"
#include "stm32f10x.h"
#include "gpio.h"
#include "common/debounce.h"

//bind := "PA0" or "PB10"
static GPIO_TypeDef *gpio_port(const char *bind)
{
	char port = bind[1];
	switch(port){
	case 'A': return GPIOA;
	case 'B': return GPIOB;
	case 'C': return GPIOC;
	case 'D': return GPIOD;
	case 'E': return GPIOE;
	case 'F': return GPIOF;
	case 'G': return GPIOG;
	default:;
	}
	return NULL;
}

//bind := "PA0" or "PB10"
static int gpio_pin(const char *bind)
{
	int pin = atoi(&bind[2]);
	int mask = 1 << pin;
	return mask;
}

static int gpio_set_hw(const gpio_t *gpio, int high)
{
	int ecode = 0;
	GPIO_TypeDef *GPIOn = gpio_port(gpio->bind);
	int msk = gpio_pin(gpio->bind);

	switch(gpio->mode) {
	case GPIO_PP0:
	case GPIO_PP1:
	case GPIO_OD0:
	case GPIO_OD1:
		if(high) GPIOn->BSRR = (uint16_t) msk;
		else GPIOn->BRR = (uint16_t) msk;
		break;
	default:
		ecode = -1;
	}
	return ecode;
}

static int gpio_get_hw(const gpio_t *gpio)
{
	GPIO_TypeDef *GPIOn = gpio_port(gpio->bind);
	int msk = gpio_pin(gpio->bind);
	int yes = (GPIOn->IDR & msk) ? 1 : 0;
	return yes;
}

static int gpio_config_hw(const gpio_t *gpio)
{
	sys_assert(gpio != NULL);
	GPIO_TypeDef *GPIOn = gpio_port(gpio->bind);
	int pin = gpio_pin(gpio->bind);

	struct {
		GPIO_TypeDef *port;
		int periph;
	} periph_list[] = {
		{GPIOA, RCC_APB2Periph_GPIOA},
		{GPIOB, RCC_APB2Periph_GPIOB},
		{GPIOC, RCC_APB2Periph_GPIOC},
		{GPIOD, RCC_APB2Periph_GPIOD},
		{GPIOE, RCC_APB2Periph_GPIOE},
		{GPIOF, RCC_APB2Periph_GPIOF},
		{GPIOG, RCC_APB2Periph_GPIOG},
	};

	for(int i = 0; i < sizeof(periph_list) / sizeof(periph_list[0]); i ++) {
		if(periph_list[i].port == GPIOn) {
			RCC_APB2PeriphClockCmd(periph_list[i].periph, ENABLE);
		}
	}

	int mode, level = -1;
	switch(gpio->mode) {
	case GPIO_AIN: mode = GPIO_Mode_AIN; break;
	case GPIO_DIN: mode = GPIO_Mode_IN_FLOATING; break;
	case GPIO_IPD: mode = GPIO_Mode_IPD; break;
	case GPIO_IPU: mode = GPIO_Mode_IPU; break;
	case GPIO_PP0:
		mode = GPIO_Mode_Out_PP;
		level = 0;
		break;
	case GPIO_PP1:
		mode = GPIO_Mode_Out_PP;
		level = 1;
		break;
	case GPIO_OD0:
		mode = GPIO_Mode_Out_OD;
		level = 0;
		break;
	case GPIO_OD1:
		mode = GPIO_Mode_Out_OD;
		level = 1;
		break;
	default:
		sys_assert(1 == 0); //gpio mode unsupport
	}

	//set before port init
	if(level >= 0) {
		gpio_set_hw(gpio, level);
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = (GPIOMode_TypeDef) mode;
	GPIO_InitStructure.GPIO_Pin = (uint16_t) pin;
	GPIO_Init(GPIOn, &GPIO_InitStructure);
	return 0;
}

const gpio_drv_t gpio_stm32 = {
	.name = "stm32",
	.config = gpio_config_hw,
	.set = gpio_set_hw,
	.get = gpio_get_hw,
};
