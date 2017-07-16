/*
*
*  miaofng@2016-09-12 routine for usbhub ctrl board v2.x
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

#define CONFIG_CMD_WIMG 1
#define CONFIG_CMD_GPIO 1

#define HASH_EMPTY 0xff
#define HASH_CONFLICT 0xfe
#define HASH_N_ENTRY 32
static int hash_table[HASH_N_ENTRY][3];

#if CONFIG_CMD_WIMG == 1
int iomap_size = 0;
const char *iomap[32]; //at most 32 gpio
#endif

int hash1(const char *name)
{
	int sum = 0; char x;
	while((x = *name ++) != 0) {
		sum += x;
	};

	int hash = sum % HASH_N_ENTRY;
	return hash;
}

int hash2(const char *name)
{
	int i = 1, sum = 0; char x;
	while((x = *name ++) != 0) {
		sum += x * i;
		i ++;
	};

	int hash = sum % HASH_N_ENTRY;
	return hash;
}

void hash_init(void)
{
	for(int i = 0; i < HASH_N_ENTRY; i ++) {
		hash_table[i][0] = HASH_EMPTY;
		hash_table[i][1] = HASH_EMPTY;
	}
}

/*never die!!!*/
int hash_get(const char *name)
{
	int h, v;
	h = hash1(name);
	v = hash_table[h][0];
	if(v == HASH_CONFLICT) {
		h = hash2(name);
		v = hash_table[h][1];
	}
	return v;
}

void hash_add(const char *name, int value)
{
	int h, v, old;
	h = hash1(name);
	v = hash_table[h][0];
	if(v == HASH_EMPTY) {
		hash_table[h][0] = value;
		hash_table[h][2] = hash2(name);
	}
	else {
		assert(v != HASH_CONFLICT);
		old = hash_table[h][0];
		hash_table[h][0] = HASH_CONFLICT;

		//try store the old data to new location
		h = hash_table[h][2];
		v = hash_table[h][1];
		assert(v == HASH_EMPTY);
		hash_table[h][1] = old;

		//try to store new data
		h = hash2(name);
		v = hash_table[h][1];
		assert(v == HASH_EMPTY);
		hash_table[h][1] = value;
	}
}

int gpio_handle(const char *gpio_name)
{
	//gpio = PA0 or PB10
	char port = gpio_name[1];
	int pin = atoi(&gpio_name[2]);
	int handle = (port << 8) | pin;
	return handle;
}

GPIO_TypeDef *gpio_port(int gpio_handle)
{
	char port = (gpio_handle >> 8) & 0xff;
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

int gpio_pin(int gpio_handle)
{
	int pin = gpio_handle & 0xff;
	int mask = 1 << pin;
	return mask;
}

int gpio_bind(int mode, const char *gpio, const char *name )
{
	#if CONFIG_CMD_WIMG == 1
	iomap[iomap_size ++] = name;
	#endif

	int handle = gpio_handle(gpio);
	hash_add(name, handle);

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
		return -2; //gpio mode unsupport
	}

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = (GPIOMode_TypeDef) mode;
	GPIO_InitStructure.GPIO_Pin = (uint16_t) pin;
	GPIO_Init(GPIOn, &GPIO_InitStructure);

	if(level >= 0) {
		gpio_set(name, level);
	}
	return 0;
}

int gpio_set(const char *name, int high)
{
	int ecode = -1;
	int handle = hash_get(name);
	if((handle != HASH_CONFLICT) &&(handle != HASH_EMPTY)) {
		ecode = 0;

		GPIO_TypeDef *GPIOn = gpio_port(handle);
		int pin = gpio_pin(handle);
		if(high) GPIOn->BSRR = (uint16_t) pin;
		else GPIOn->BRR = (uint16_t) pin;
	}
	return ecode;
}

int gpio_get(const char *name)
{
	int result = -1;
	int handle = hash_get(name);

	//sys_assert(handle != HASH_CONFLICT);
	//sys_assert(handle != HASH_EMPTY);

	if((handle != HASH_CONFLICT) &&(handle != HASH_EMPTY)) {
		GPIO_TypeDef *GPIOn = gpio_port(handle);
		int pin = gpio_pin(handle);
		result = (GPIOn->IDR & pin) ? 1 : 0;
	}
	return result;
}

void gpio_init(void)
{
	hash_init();
	#if CONFIG_CMD_WIMG == 1
	iomap_size = 0;
	#endif
}

#if CONFIG_CMD_GPIO == 1
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
	};

	if((argc == 2) && !strcmp(argv[1], "help")) {
		printf("%s", usage);
		return 0;
	}

	int read = 0, ecode = 0;
	for(int i = 1; i < argc; i ++) {
		ecode = -2;
		if(strlen(argv[i]) >=3) {
			/*char** tokens = split(argv[i], '=');
			if(tokens) {
				char *pname = tokens[0];
				char *level = tokens[1];
			*/
			char *p = strchr(argv[i], '=');
			if(p != NULL) {
				*p = '\0';
				char *pname = argv[i];
				char *level = p + 1;
				if(pname) pname = trim(pname);
				if(level) level = trim(level);
				if(pname && level && (strlen(pname) > 0) && (strlen(level) > 0)) {
					ecode = gpio_set(pname, atoi(level));
					if(ecode) {
						printf("<%+d, pin %s not exist\n\r", ecode, pname);
						return 0;
					}
				}
			}
			else { //read
				char *pname = trim(argv[i]);
				int level = gpio_get(pname);
				if(level < 0) {
					printf("<%+d, %s not exist\n\r", level, pname);
				}
				else {
					printf("<%+d, %s=%d\n", level, pname, level);
				}
				ecode = 0;
				read = 1;
			}
		}

		if(ecode) {
			printf("<%+d, equation error\n\r", ecode);
			return 0;
		}
	}

	if(read == 0) printf("<+0, No Error\n\r");
	return 0;
}

const cmd_t cmd_gpio = {"GPIO", cmd_gpio_func, "gpio setting commands"};
DECLARE_SHELL_CMD(cmd_gpio)
#endif

#if CONFIG_CMD_WIMG == 1
static int cmd_wimg_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"WIMG HEX MSK CKSUM		CKSUM = HEX & MSK !!!\n"
	};

	int ecode = -1;
	const char *emsg = "<-1, command format error\n\r";
	const char *pname;

	if((argc == 2) && !strcmp(argv[1], "help")) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 4) {
		int img, msk, cksum, nok;
		nok = sscanf(argv[1], "%x", &img);
		nok += sscanf(argv[2], "%x", &msk);
		nok += sscanf(argv[3], "%x", &cksum);

		if((nok != 3) || (cksum != (img & msk))) {
			printf(emsg);
			return 0;
		}

		img = cksum; //cksum == img & msk
		for(int i = 0; i < iomap_size; i ++) {
			if (bit_get(i, &msk)) {
				int level = bit_get(i, &img);
				pname = iomap[i];
				ecode = gpio_set(pname, level);
				if(ecode) {
					printf("<%d, pin %s not exist\n\r", ecode, pname);
					return 0;
				}
			}
		}

		printf("<+0, No Error\n\r");
		return 0;
	}

	printf(emsg);
	return 0;
}

const cmd_t cmd_wimg = {"WIMG", cmd_wimg_func, "write gpio image"};
DECLARE_SHELL_CMD(cmd_wimg)
#endif

