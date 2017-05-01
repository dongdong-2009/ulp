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

#define CONFIG_OC_MS 500 //UPx_ERR# overcurrent protection delay
#define CONFIG_CMD_WIMG 1

#define HASH_EMPTY 0xff
#define HASH_CONFLICT 0xfe
#define HASH_N_ENTRY 32
static int hash_table[HASH_N_ENTRY][3];

#define LOAD_TIMER_MS 10000 //pulse width
static time_t usb1_load_timer = 0; //auto off to avoid overheat
/* static time_t usb2_load_timer = 0; //auto off to avoid overheat */
static time_t pdi_oc_timer = 0;

#if CONFIG_CMD_WIMG == 1
int iomap_size = 0;
const char *iomap[32]; //at most 32 gpio
#endif

void gpio_bind(const char *gpio, const char *name, int level);
int gpio_set(const char *name, int high);
#define GPIO_BIND(gpio, name, level) gpio_bind(#gpio, #name, level)
#define GPIO_SET(name, level) gpio_set(#name, level)

static float pdi_vdd = 0;
static float pdi_ifb = 0;
static float pdi_v_dn1 = 0;
static float pdi_v_err = 0;
static float pdi_v_rdy4tst = 0;

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
	if (!strcmp(name, "DN1_LOAD_EN")) {
		if(high) usb1_load_timer = time_get(LOAD_TIMER_MS);
		else usb1_load_timer = 0;
	}

/* 	if (!strcmp(name, "DN2_LOAD_EN")) {
		if(high) usb2_load_timer = time_get(LOAD_TIMER_MS);
		else usb2_load_timer = 0;
	} */

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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	hash_init();
	#if CONFIG_CMD_WIMG == 1
	iomap_size = 0;
	#endif

	GPIO_BIND(PE10, UPx_VCC_EN, 0);
	GPIO_BIND(PE8 , UPx_IMEN, 0);
	GPIO_BIND(PB1 , DN1_VPM_EN, 0);
	GPIO_BIND(PB0 , DN1_LOAD_EN, 0);

	GPIO_BIND(PA7 , EXT_RO, 0);
	GPIO_BIND(PA6 , EXT_GO, 0);
	GPIO_BIND(PA5 , EXT_YO, 0);
	GPIO_BIND(PA4 , EXT_BO, 0);

	GPIO_BIND(PE13, HUB_RST, 0);

	/*
	PE9 IN UPx_ERR#
	PA3 IN RDY4TST
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_9;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	#define err_upx_oc() ((GPIOE->IDR & GPIO_Pin_9) ? 0 : 1)
	#define rdy4tst() ((GPIOA->IDR & GPIO_Pin_3) ? 0 : 1)

	/*
	PC0  ADC VDD_+5V @ADC123_IN10
	PC1  ADC DN1_VCC @ADC123_IN11
	PC2  ADC UPx_IFB @ADC123_IN12
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//ADC INIT, ADC1 INJECTED CH, CONT SCAN MODE
	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	ADC_DeInit(ADC1);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 3); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_239Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	#define adc_vdd_get() (ADC1->JDR1 << 1)
	#define adc_dn1_get() (ADC1->JDR2 << 1)
	#define adc_ifb_get() (ADC1->JDR3 << 1)
}

void hub_update(void)
{
	int d;
	float vref = 3.0;

	/* vdd_+5v
	100K+100K,100N => 10000 uS = 10mS
	*/
	d = adc_vdd_get();
	d = d << 1;
	pdi_vdd = d * (vref / 65536);

	/* dn1_vcc
	100K+100K,100N => 10000 uS = 10mS
	*/
	d = adc_dn1_get();
	d = d << 1;
	pdi_v_dn1 = d * (vref / 65536);

	/* upx_ifb
	INA193(20V/V), Rsense = 1R
	100K*100N = 10000uS = 10mS
	*/
	d = adc_ifb_get();
	d = d / 20 / 1;
	pdi_ifb = d * (vref / 65536);

	//gpio emulated pin level
	pdi_v_err = err_upx_oc() ? 0.0 : 3.3;
	pdi_v_rdy4tst = rdy4tst() ? 0.0 : 3.3;

	if (err_upx_oc()) {
		pdi_oc_timer = time_get(CONFIG_OC_MS);
	}

	if(pdi_oc_timer) {
		if(!err_upx_oc()) pdi_oc_timer = 0;
		else {
			if(time_left(pdi_oc_timer) < 0) {
				pdi_oc_timer = 0;
				GPIO_SET(UPx_VCC_EN, 0);
				GPIO_SET(UPx_IMEN, 0);
			}
		}
	}

	if(usb1_load_timer) {
		if(time_left(usb1_load_timer) < 0) {
			usb1_load_timer = 0;
			GPIO_SET(DN1_LOAD_EN, 0);
		}
	}

/* 	if(usb2_load_timer) {
		if(time_left(usb2_load_timer) < 0) {
			usb2_load_timer = 0;
			GPIO_SET(DN2_LOAD_EN, 0);
		}
	} */
}

void main()
{
	sys_init();
	shell_mute(NULL);
	hub_init();
	printf("hubctrl sw v2.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		hub_update();
	}
}

static int cmd_pdi_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"pdi vdd		read VDD_+5V\n"
		"pdi dn1		read DN1_VCC\n"
		"pdi ifb		read UPx_IFB\n"
		"pdi err		read UPx_ERR#(GPIO)\n"
		"pdi rdy		read RDY4TST(GPIO)\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	switch(argv[1][0]) {
	case 'v':
		printf("<%+.3f, VDD_+5V(unit: V)\n", pdi_vdd);
		break;
	case 'd':
		printf("<%+.3f, DN1_VCC(unit: V)\n", pdi_v_dn1);
		break;
	case 'i':
		printf("<%+.3f, UPx_IFB(unit: V)\n", pdi_ifb);
		break;
	case 'e':
		printf("<%+.3f, UPx_ERR#(unit: V, GPIO)\n", pdi_v_err);
		break;
	case 'r':
		printf("<%+.3f, RDY4TST(unit: V, GPIO)\n", pdi_v_rdy4tst);
		break;
	default:
		printf("<-1, sub cmd not supported\n");
		break;
	}

	return 0;
}

cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi i/f commands"};
DECLARE_SHELL_CMD(cmd_pdi)

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
		printf("<0,Ulicar Technology,HUBPDI V1.x,%s,%s\n\r", __DATE__, __TIME__);
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
