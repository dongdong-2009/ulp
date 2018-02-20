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

#define GPIO_N 63

static int gpio_n = 0;
static gpio_t gpios[GPIO_N];

static gpio_t *bind_search(const char *bind)
{
	gpio_t *p = NULL;
	for(int i = 0; i < gpio_n; i ++) {
		if(!strcmp(gpios[i].bind, bind)) {
			p = &gpios[i];
			break;
		}
	}
	return p;
}

static gpio_t *name_search(const char *name)
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

int gpio_handle(const char *name)
{
	int idx = GPIO_NONE;
	for(int i = 0; i < gpio_n; i ++) {
		if(!strcmp(gpios[i].name, name)) {
			idx = i;
			break;
		}
	}
	return idx;
}

void gpio_init(void)
{
	gpio_n = 0;
}

int gpio_bind_inv(int mode, const char *bind, const char *name)
{
	int handle = gpio_bind(mode, bind, name);
	gpios[handle].invt = 1;
	return handle;
}

int gpio_bind(int mode, const char *bind, const char *name)
{
	gpio_t *gpio = NULL;
	const gpio_drv_t *driver = NULL;

	#if CONFIG_CPU_STM32 == 1
	driver = &gpio_stm32;
	#endif

	#if CONFIG_GPIO_MCP == 1
	if(!strncmp(bind, gpio_mcp.name, 3)) {
		driver = &gpio_mcp;
	}
	#endif

	//gpio should not been binded
	gpio = bind_search(bind);
	if(gpio) {
		printf("A: %s => %s\n", gpio->bind, gpio->name);
		printf("B: %s => %s\n", bind, name);
		sys_assert(gpio == NULL);
	}

	//name should not been used
	gpio = name_search(name);
	if(gpio) {
		printf("A: %s => %s\n", gpio->bind, gpio->name);
		printf("B: %s => %s\n", bind, name);
		sys_assert(gpio == NULL);
	}

	gpio = & gpios[gpio_n ++];
	sys_assert(gpio_n < GPIO_N);
	gpio->drv = driver;
	gpio->name = name;
	gpio->bind = bind;
	gpio->mode = mode;

#if CONFIG_GPIO_FILTER == 1
	debounce_t_init(&gpio->gfilt, 0, 0);
#endif

	driver->config(gpio);
	return gpio_n - 1; //return the index of current gpio, 0 - 30
}

#if CONFIG_GPIO_FILTER == 1
int gpio_filt(const char *name, int ms)
{
	int ecode = -1;
	gpio_t *gpio = name_search(name);
	if(gpio != NULL) {
		debounce_t_init(&gpio->gfilt, ms, 0);
	}
	return ecode;
}
#endif

static int gpio_set_hw(gpio_t *gpio, int high)
{
	const gpio_drv_t *driver = gpio->drv;
	gpio->high = high;
	return driver->set(gpio, high ^ gpio->invt);
}

int gpio_set(const char *name, int high)
{
	int ecode = -1;
	gpio_t *gpio = name_search(name);
	if(gpio != NULL) {
		ecode = gpio_set_hw(gpio, high);
	}
	return ecode;
}

int gpio_set_h(int handle, int high)
{
	sys_assert(handle < GPIO_N);
	gpio_t *gpio = &gpios[handle];
	return gpio_set_hw(gpio, high);
}

int gpio_inv(const char *name)
{
	int ecode = -1;
	gpio_t *gpio = name_search(name);
	if(gpio != NULL) {
		ecode = gpio_set_hw(gpio, !gpio->high);
	}
	return ecode;
}

int gpio_inv_h(int handle)
{
	sys_assert(handle < GPIO_N);
	gpio_t *gpio = &gpios[handle];
	return gpio_set_hw(gpio, !gpio->high);
}

static int gpio_get_hw(const gpio_t *gpio)
{
	const gpio_drv_t *driver = gpio->drv;
	int yes = driver->get(gpio);
	yes ^= gpio->invt;
#if CONFIG_GPIO_FILTER == 1
	debounce((struct debounce_s *)(&gpio->gfilt), yes);
	yes = gpio->gfilt.on;
#endif
	return yes;
}

int gpio_get(const char *name)
{
	int level, ecode = -1;
	gpio_t *gpio = name_search(name);
	if(gpio != NULL) {
		ecode = 0;
		level = gpio_get_hw(gpio);
	}

	return ecode ? ecode : level;
}

int gpio_get_h(int handle)
{
	sys_assert(handle < GPIO_N);
	gpio_t *gpio = &gpios[handle];
	return gpio_get_hw(gpio);
}

int gpio_wimg(int img, int msk)
{
	return gpio_wbits(&img, &msk, 32);
}

int gpio_wbits(const void *img, const void *msk, int nbits)
{
	int ecode = 0;
	int N = (nbits > gpio_n) ? gpio_n : nbits;

	for(int i = 0; i < N; i ++) {
		if (bit_get(i, msk)) {
			gpio_t *gpio = &gpios[i];
			int high = bit_get(i, img);
			ecode = gpio_set_hw(gpio, high);
			if(ecode) break;
		}
	}

	return ecode;
}

int gpio_rimg(int msk)
{
	int img = 0;
	gpio_rbits(&img, &msk, 32);
	return img;
}

int gpio_rbits(void *img, const void *msk, int nbits)
{
	int N = (nbits > gpio_n) ? gpio_n : nbits;

	for(int i = 0; i < N; i ++) {
		if (bit_get(i, msk)) {
			int yes = gpio_get_hw(&gpios[i]);
			if(yes) bit_set(i, img);
			else bit_clr(i, img);
		}
	}
	return 0;
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

		const char *invt = (gpios[i].invt) ? "-" : "+";
		printf("%02d: %s, %s%s => %s\n", i, mode, invt, gpios[i].bind, gpios[i].name);
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
				if(name_search(pname) == NULL) {
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

