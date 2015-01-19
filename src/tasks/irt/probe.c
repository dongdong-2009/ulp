#include "config.h"
#include "ulp/sys.h"
#include "shell/cmd.h"
#include "led.h"
#include "mbi5025.h"
#include <string.h>

#include "stm32f10x.h"
#include "common/debounce.h"
#include "irc.h"

const can_bus_t *probe_can = &can1;
int probe_flag_key_msg_sent;
int probe_flag_key_msg_echo;
int probe_flag_offline;

/*PINMAP
PA0/ADC123_IN0	VIN
PE15		MODE KEY
*/
void board_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//MODE KEY PULL UP
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//AIN
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

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

	ADC_InjectedSequencerLengthConfig(ADC1, 1); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	const can_cfg_t can_cfg = {.baud = CAN_BAUD, .silent = 0};
	probe_can->init(&can_cfg);

	can_filter_t filter = {.id = CAN_ID_PRB, .mask = 0x0f00, .flag = 0};
	probe_can->filt(&filter, 1);
}

//return mode key level, PE15
int key_get(void)
{
	return (GPIOE->IDR & GPIO_Pin_15) ? 1 : 0;
}

float adc_get(void)
{
	float v = ADC1->JDR1 << 0;
	v *= 2.5 / 32768;
	return v;
}

#define NLEDS 4
static char leds_line[NLEDS];
static int leds_widx = 0;
static const mbi5025_t leds = {
	.bus = &spi1,
	.load_pin = SPI_CS_PA4,
	.oe_pin = SPI_CS_PC4,
};

void leds_init(void)
{
	mbi5025_Init(&leds);
	leds.bus->csel(leds.oe_pin, 0);
}

void leds_show(void)
{
	mbi5025_write_and_latch(&leds, leds_line, NLEDS);
}

void leds_emit(char c)
{
	const char fonts[] = {
		0x00, //32 ' '
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //33, 34, 35, 36, 37, 38, 39
		0x9c, //40 '('
		0xf0, //41 ')'
		0x00, 0x00, 0x00, 0x02, //42, 43, 44, '-'
		0x01, //46 '.'
		0x4a, //47 '/'
		0xfc, //48 '0'
		0x60, //49 '1'
		0xda, //50 '2'
		0xf2, //51 '3'
		0x66, //52 '4'
		0xb6, //53 '5'
		0xbe, //54 '6'
		0xe0, //55 '7'
		0xfe, //56 '8'
		0xf6, //57 '9'
	};

	#define ascii_2_font(c) fonts[c - 32]
	#define fdot ascii_2_font('.')
	#define fnul ascii_2_font(' ')

	switch(c) {
	case '\n':
		for(int i = 0; i < NLEDS; i ++) {
			leds_line[i] = fnul;
		}
	case '\r':
		leds_widx = 0;
		return;
	case '.':
		if(leds_widx > 0) { //check previous digit dp exist?
			char has_dp = leds_line[leds_widx - 1] & fdot;
			if(!has_dp) {
				leds_line[leds_widx - 1] |= fdot;
				return;
			}
		}
	default:
		//only show the first 4 digits
		if(leds_widx < NLEDS) {
			leds_line[leds_widx] = ascii_2_font(c);
			leds_widx ++;
		}
		else { //overflow ... :( ... shift left all chars
			sys_mdelay(100);
			for(int i = 1; i < NLEDS; i ++) {
				leds_line[i - 1] = leds_line[i];
			}
			leds_line[NLEDS - 1] = ascii_2_font(c);
			leds_show();
		}
	}
}

int leds_printf(const char *fmt, ...)
{
	//char *pstr;
	int n = 0;
	va_list ap;

	#define LSZ (NLEDS << 3)
	//pstr = sys_malloc(LSZ);
	//sys_assert(pstr != NULL);
	static char pstr[LSZ];

	va_start(ap, fmt);
	n += vsnprintf(pstr + n, LSZ - n, fmt, ap);
	va_end(ap);

	for(int i = 0; i < n; i ++) {
		leds_emit(pstr[i]);
	}

	leds_show();
	//sys_free(pstr);
	return n;
}

static struct debounce_s probe_key;

int probe_send(const can_msg_t *msg)
{
	int ecode = -1;
	time_t deadline = time_get(IRC_CAN_MS);
	probe_can->flush_tx();
	if(!probe_can->send(msg)) { //wait until message are sent
		while(time_left(deadline) > 0) {
			if(probe_can->poll(CAN_POLL_TBUF) == 0) {
				ecode = 0;
				break;
			}
		}
	}

	return ecode;
}

void probe_init(void)
{
	debounce_init(&probe_key, 50, 0);
	probe_flag_key_msg_sent = 0;
	probe_flag_offline = 0;
}

void probe_update(void)
{
	can_msg_t msg;

	//handle key event
	if(debounce(&probe_key, key_get() == 0)) {
		if(probe_key.off) { //key released
			probe_flag_key_msg_sent = 0;
			if(!probe_flag_key_msg_echo) {
				leds_printf("\n");
			}
		}
	}

	if(probe_key.on) { //key pressed
		if(!probe_flag_key_msg_sent) {
			//display 8888
			leds_printf("\n%s", probe_flag_offline ? "...." : "8888");
			probe_flag_key_msg_echo = 0;

			if(!probe_flag_offline) {
				//send msg to irc board
				msg.id = CAN_ID_PRB;
				msg.dlc = 0;

				int ecode = probe_send(&msg);
				probe_flag_key_msg_sent = (ecode == 0) ? 1 : 0;
			}
		}
	}

	//handle can message
	if(!probe_flag_offline) {
		int ecode = probe_can -> recv(&msg);
		if(!ecode) { //message received
			int type = msg.id & 0x0ff;
			switch(type) {
			case 0:
				probe_flag_key_msg_echo = 1;
			case 1:
				msg.data[4] = '\0'; //4 char at most
				leds_printf("\n%s", msg.data);
				break;
			default:
				break;
			}
		}
	}
}

int main(void)
{
	board_init();
	sys_init();
	leds_init();
	probe_init();

	printf("probe sw v1.0, build: %s %s\n\r", __DATE__, __TIME__);
	leds_printf("888888888888.... 9876543210\n");

	while(1) {
		sys_update();
		probe_update();
	}
}

static int cmd_probe_func(int argc, char *argv[])
{
	const char *usage = {
		"  usage: \r\n"
		"  probe print 123	led digits test\r\n"
		"  probe offline  	give-up can bus, for debug purpose\n"
		"  probe online  	restore normal mode\n"
	};

	if(argc == 3) {
		if(!strcmp(argv[1], "print")) {
			leds_printf("\n%s", argv[2]);
			return 0;
		}
	}

	if(argc == 2) {
		if(!strcmp(argv[1], "online")) {
			probe_flag_offline = 0;
			return 0;
		}
		if(!strcmp(argv[1], "offline")) {
			probe_flag_offline = 1;
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}

cmd_t cmd_probe = {"probe", cmd_probe_func, "commands for probe"};
DECLARE_SHELL_CMD(cmd_probe)
