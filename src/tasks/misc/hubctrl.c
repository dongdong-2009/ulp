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

#define CONFIG_CMD_WIMG 1

#define HASH_EMPTY 0xff
#define HASH_CONFLICT 0xfe
#define HASH_N_ENTRY 64
static int hash_table[HASH_N_ENTRY][3];

#define LOAD_TIMER_MS 10000 //pulse width
static time_t usb1_load_timer = 0; //auto off to avoid overheat
static time_t usb2_load_timer = 0; //auto off to avoid overheat

#if CONFIG_CMD_WIMG == 1
int iomap_size = 0;
const char *iomap[32]; //at most 32 gpio
#endif

void gpio_bind(const char *gpio, const char *name, int level);
int gpio_set(const char *name, int high);
#define GPIO_BIND(gpio, name, level) gpio_bind(#gpio, #name, level)
#define GPIO_SET(name, level) gpio_set(#name, level)

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
	default: assert(1 == 0);
	}
	return NULL;
}

int gpio_pin(int gpio_handle)
{
	int pin = gpio_handle & 0xff;
	int mask = 1 << pin;
	return mask;
}

void gpio_bind(const char *gpio, const char *name, int level)
{
	#if CONFIG_CMD_WIMG == 1
	iomap[iomap_size ++] = name;
	#endif

	int handle = gpio_handle(gpio);
	hash_add(name, handle);

	GPIO_TypeDef *GPIOn = gpio_port(handle);
	int pin = gpio_pin(handle);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = (uint16_t) pin;
	GPIO_Init(GPIOn, &GPIO_InitStructure);

	gpio_set(name, level);
}

int gpio_set(const char *name, int high)
{
	if (!strcmp(name, "USB1_LOAD_EN")) {
		if(high) usb1_load_timer = time_get(LOAD_TIMER_MS);
		else usb1_load_timer = 0;
	}

	if (!strcmp(name, "USB2_LOAD_EN")) {
		if(high) usb2_load_timer = time_get(LOAD_TIMER_MS);
		else usb2_load_timer = 0;
	}

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

void board_reset(void)
{
	NVIC_SystemReset();
}

void hub_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	hash_init();
	#if CONFIG_CMD_WIMG == 1
	iomap_size = 0;
	#endif

	/*passmark power settings*/
	GPIO_BIND(PC8 , USB0_VPM_EN, 0);
	GPIO_BIND(PC9 , USB1_VPM_EN, 0);
	GPIO_BIND(PA8 , USB2_VPM_EN, 0);

	/*VBAT*/
	GPIO_BIND(PC4, VBAT_EN, 0);
	GPIO_BIND(PC7 , USB0_VCC_EN, 0);
	GPIO_BIND(PB11, CDP_EN, 0);

	/*usb0, upstream*/
	GPIO_BIND(PB1, FSUSB30_EN#, 1);
	GPIO_BIND(PB2, USB0_S0, 0);
	GPIO_BIND(PB10, USB0_S1, 0);

	/*usb1&2, downstream*/
	GPIO_BIND(PB12, FSUSB74_EN#, 1);
	GPIO_BIND(PB13, USB1_S0, 0);
	GPIO_BIND(PB14, USB1_S1, 0);
	GPIO_BIND(PB15, USB2_S0, 0);
	GPIO_BIND(PC6 , USB2_S1, 0);

	/*load settings*/
	GPIO_BIND(PA4 , USB1_LOAD_EN, 0);
	GPIO_BIND(PA5 , USB2_LOAD_EN, 0);
	GPIO_BIND(PA6 , USB1_SCP_EN, 0);
	GPIO_BIND(PA7 , USB2_SCP_EN, 0);

	/*DMM*/
	GPIO_BIND(PB0, DMM_VMEN, 0);
	GPIO_BIND(PC5, DMM_IMEN, 0);
	GPIO_BIND(PA0, DG_S0, 0);
	GPIO_BIND(PA1, DG_S1, 0);
	GPIO_BIND(PA2, DG_S2, 0);

	/*HUB RESET*/
	GPIO_BIND(PC2, HUB0_RST, 0);
	GPIO_BIND(PC3, HUB1_RST, 0);
}

void hub_update(void)
{
	if(usb1_load_timer) {
		if(time_left(usb1_load_timer) < 0) {
			usb1_load_timer = 0;
			GPIO_SET(USB1_LOAD_EN, 0);
		}
	}

	if(usb2_load_timer) {
		if(time_left(usb2_load_timer) < 0) {
			usb2_load_timer = 0;
			GPIO_SET(USB2_LOAD_EN, 0);
		}
	}
}

void main()
{
	sys_init();
	shell_mute(NULL);
	hub_init();

	//FE1.1S RESET
	//GPIO_SET(HUB0_RST, 1);
	GPIO_SET(HUB1_RST, 1);
	sys_mdelay(2);
	//GPIO_SET(HUB0_RST, 0);
	GPIO_SET(HUB1_RST, 0);

	printf("hubctrl sw v2.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		hub_update();
	}
}

char** split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

char *trim(char *str)
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

	int ecode = 0;
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
						printf("<%d, pin %s not exist\n\r", ecode, pname);
						return 0;
					}
				}
			}
		}

		if(ecode) {
			printf("<%d, equation error\n\r", ecode);
			return 0;
		}
	}

	printf("<+0, No Error\n\r");
	return 0;
}

const cmd_t cmd_gpio = {"GPIO", cmd_gpio_func, "gpio setting commands"};
DECLARE_SHELL_CMD(cmd_gpio)

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

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<0,Ulicar Technology,HubCtrl V2.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
