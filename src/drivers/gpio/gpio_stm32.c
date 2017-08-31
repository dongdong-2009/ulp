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

#define GPIO_N 31

typedef struct {
	const char *name;
	int mode;
	int handle;
} gpio_t;

static int gpio_n = 0;
static gpio_t gpios[GPIO_N];

static gpio_t *gpio_search(const char *name)
{
	gpio_t *p = NULL;
	for(int i = 0; i < gpio_n; i ++) {
		if(!strcmp(gpios[i].name, name)) {
			p = &gpios[i];
			break;
		}
	}
	return p;
}

static int gpio_handle(const char *gpio)
{
	//gpio = PA0 or PB10
	char port = gpio[1];
	int pin = atoi(&gpio[2]);
	int handle = (port << 8) | pin;
	return handle;
}

GPIO_TypeDef *gpio_port(int handle)
{
	char port = (handle >> 8) & 0xff;
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

int gpio_pin(int handle)
{
	int pin = handle & 0xff;
	int mask = 1 << pin;
	return mask;
}

void gpio_init(void)
{
	gpio_n = 0;
}

int gpio_bind(int mode, const char *gpio, const char *name)
{
	int handle = gpio_handle(gpio);
	gpio_t *p = gpio_search(name);

	sys_assert(p == NULL); //name should not identify
	sys_assert(gpio_n < GPIO_N);

	p = & gpios[gpio_n ++];
	p->name = name;
	p->mode = mode;
	p->handle = handle;

	GPIO_TypeDef *GPIOn = gpio_port(handle);
	int pin = gpio_pin(handle);

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

	int level = -1;
	switch(mode) {
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
		gpio_set(name, level);
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = (GPIOMode_TypeDef) mode;
	GPIO_InitStructure.GPIO_Pin = (uint16_t) pin;
	GPIO_Init(GPIOn, &GPIO_InitStructure);

	//return the index of current gpio, 0 - 30
	return gpio_n - 1;
}

static int gpio_set_hw(const gpio_t *gpio, int high)
{
	int ecode = 0;
	int handle = gpio->handle;
	GPIO_TypeDef *GPIOn = gpio_port(handle);
	int msk = gpio_pin(handle);

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

int gpio_set(const char *name, int high)
{
	int ecode = -1;
	gpio_t *gpio = gpio_search(name);
	if(gpio != NULL) {
		ecode = gpio_set_hw(gpio, high);
	}
	return ecode;
}

int gpio_set_h(int hgpio, int high)
{
	sys_assert(hgpio < GPIO_N);
	gpio_t *gpio = &gpios[hgpio];
	return gpio_set_hw(gpio, high);
}

static int gpio_get_hw(const gpio_t *gpio)
{
	int handle = gpio->handle;
	GPIO_TypeDef *GPIOn = gpio_port(handle);
	int msk = gpio_pin(handle);
	return (GPIOn->IDR & msk) ? 1 : 0;
}

int gpio_get(const char *name)
{
	int level, ecode = -1;
	gpio_t *gpio = gpio_search(name);
	if(gpio != NULL) {
		ecode = 0;
		level = gpio_get_hw(gpio);
	}

	return ecode ? ecode : level;
}

int gpio_get_h(int hgpio)
{
	sys_assert(hgpio < GPIO_N);
	gpio_t *gpio = &gpios[hgpio];
	return gpio_get_hw(gpio);
}

int gpio_wimg(int img, int msk)
{
	int ecode = 0;
	for(int i = 0; i < gpio_n; i ++) {
		if (bit_get(i, &msk)) {
			gpio_t *gpio = &gpios[i];
			int handle = gpio->handle;
			GPIO_TypeDef *GPIOn = gpio_port(handle);
			int pin = gpio_pin(handle);
			int high = bit_get(i, &img);

			switch(gpio->mode) {
			case GPIO_PP0:
			case GPIO_PP1:
			case GPIO_OD0:
			case GPIO_OD1:
				if(high) GPIOn->BSRR = (uint16_t) pin;
				else GPIOn->BRR = (uint16_t) pin;
				break;
			default:
				ecode = -1;
			}
		}
	}
	return ecode;
}

int gpio_rimg(int msk)
{
	int img = 0;
	for(int i = 0; i < gpio_n; i ++) {
		if (bit_get(i, &msk)) {
			int handle = gpios[i].handle;
			GPIO_TypeDef *GPIOn = gpio_port(handle);
			int pin = gpio_pin(handle);

			int high = (GPIOn->IDR & pin) ? 1 : 0;
			if(high) bit_set(i, &img);
			else bit_clr(i, &img);
		}
	}
	return img;
}

void gpio_dumps(void)
{
	for(int i = 0; i < gpio_n; i ++) {
		const char *mode;
		switch(gpios[i].mode) {
		case GPIO_AIN: mode = "AIN"; break;
		case GPIO_DIN: mode = "DIN"; break;
		case GPIO_IPD: mode = "IPD"; break;
		case GPIO_IPU: mode = "IPU"; break;
		case GPIO_PP0: mode = "PP0"; break;
		case GPIO_PP1: mode = "PP1"; break;
		case GPIO_OD0: mode = "OD0"; break;
		case GPIO_OD1: mode = "OD1"; break;
		default: mode = "???";
		}

		char port = gpios[i].handle >> 8;
		char pin = gpios[i].handle & 0xff;
		printf("%02d: %s, gpio = P%c%02d, name = %s\n", i, mode, port, pin, gpios[i].name);
	}
}

static char *trim(char *str)
{
	char *p = str;
	char *p1;
	if(p) {
		p1 = p + strlen(str) - 1;
		while(*p && isspace(*p)) p++;
		while(p1 > p && isspace(*p1)) *p1-- = '\0';
	}
	return p;
}

static int cmd_gpio_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"GPIO USB0_VCC_EN=0\n"
		"GPIO USB0_VCC_EN=0 USB1_VPM_EN=1\n"
		"GPIO HELP\n"
		"GPIO LIST\n"

	};

	if((argc == 2) && !strcmp(argv[1], "HELP")) {
		printf("%s", usage);
		return 0;
	}

	if((argc == 2) && !strcmp(argv[1], "LIST")) {
		gpio_dumps();
		return 0;
	}

	int e, ecode = 0;
	for(int i = 1; i < argc; i ++) {
		char *p = strchr(argv[i], '=');
		if(p != NULL) { //write
			e = -1;

			*p = '\0';
			char *pname = argv[i];
			char *level = p + 1;
			if(pname) pname = trim(pname);
			if(level) level = trim(level);

			int high, n = sscanf(level, "%d", &high);
			if(n == 1) {
				e = gpio_set(pname, high);
			}

			ecode += e;
		}
		else { //read, only support 1
			if(ecode == 0) { //write error may exist
				char *pname = trim(argv[i]);
				if(gpio_search(pname) == NULL) {
					printf("<-2, pin %s not exist\n", pname);
				}
				else {
					int level = gpio_get(pname);
					printf("<%+d, %s = %d\n", level, pname, level);
				}
				return 0;
			}
		}
	}

	if(ecode) printf("<%+d, undefined error\n", ecode);
	else printf("<%+d, OK!\n", ecode);
	return 0;
}

const cmd_t cmd_gpio = {"GPIO", cmd_gpio_func, "gpio setting commands"};
DECLARE_SHELL_CMD(cmd_gpio)

static int cmd_wimg_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"WIMG HEX MSK CKSUM		CKSUM = HEX & MSK, optional\n"
		"WIMG HEX MSK			note: input pin write op is ignored\n"
		"WIMG MSK			read masked gpio status\n"
	};

	const char *emsg = "<-1, command format error\n\r";
	int img, msk, cksum, nok;

	if((argc == 2) && !strcmp(argv[1], "help")) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		nok = sscanf(argv[1], "%x", &msk);
		if(nok == 1) {
			int status = gpio_rimg(msk);
			printf("<%+d, 0x%08x@msk=0x%08x\n\r", status, status, msk);
			return 0;
		}
	}

	if(argc >= 3) {
		nok = sscanf(argv[1], "%x", &img);
		nok += sscanf(argv[2], "%x", &msk);
		if(argc == 4) nok += sscanf(argv[3], "%x", &cksum);
		else {
			cksum = img & msk;
			nok ++;
		}

		if((nok != 3) || (cksum != (img & msk))) {
			printf(emsg);
			return 0;
		}

		int ecode = gpio_wimg(img, msk);
		if(!ecode) {
			printf("<+0, No Error\n\r");
			return 0;
		}
		else {
			printf("<+0, wimg to input pin is ignored\n\r");
			return 0;
		}
	}

	printf(emsg);
	return 0;
}

const cmd_t cmd_wimg = {"WIMG", cmd_wimg_func, "write gpio image"};
DECLARE_SHELL_CMD(cmd_wimg)

