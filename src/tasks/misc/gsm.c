/*
 * 	miaofng@2012 initial version
 *
 */

#include "ulp/sys.h"
#include "led.h"
#include "key.h"
#include "stm32f10x.h"
#include "shell/shell.h"
#include "console.h"
#include <string.h>

#define gsm_cnsl ((struct console_s *) &uart1)

/*led1-4 for key event display
when key1 is pressed, led1 will be turned off.
it keeps off state until the gsm message been sent.
it flashes the ecode in case of op fail

led5-8 is controlled by gsm message only
*/
const char led_pinmap[] = {0,6,7,10,11,12,13,14,15};
void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	//led gpio init, pb6~7, pb10~pb15
	GPIO_InitStructure.GPIO_Pin = 0xfcc0; //1111 1100 1100 0000
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/*para: led = 1~8, on=0/1*/
#define led_set_status(led, on) do { \
	 GPIOB->BSRR = (1 << (led_pinmap[led] + ((on) ? 0 : 16))); \
} while(0)

void led_set_binary(int value)
{
	for(int i = 0; i < 8; i ++) {
		led_set_status(i + 1, value & (1 << i));
	}
}

/*gpio keyboard hardware driver*/
int key_hwinit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	//key init, s1=pb0, s2=pc13, s3=pb1, s4=pb5
	GPIO_InitStructure.GPIO_Pin = 0x0023; //0000 0000 0010 0011
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = 1 << 13;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	return 0;
}

/*return keycode: 1~4 or NOKEY*/
int key_hwgetvalue(void)
{
	int value = 0;
	if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0))
		value = 1;
	else if(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13))
		value = 2;
	else if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1))
		value = 3;
	else if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5))
		value = 4;
	else
		value = NOKEY;

	return value;
}

static char gsm_line[128];
const char phone_nr[] = "+8615062360545";//"+8615250499838"; //; //;//

static void gsm_flush(void)
{
	while(console_IsNotEmpty()) {
		console_getchar();
	}
}

static void gsm_send(const char *cmd)
{
	gsm_flush();
	printf("%s\n", cmd);
	mdelay(500);
}

static int gsm_read(char *echo, int ms)
{
	int result = -1;
	time_t deadline = time_get(ms);
	while(time_left(deadline) > 0) {
		if(shell_ReadLine(NULL, echo)) {
			if(echo[0] != 0) { //to avoid an empty line
				result = 0;
				break;
			}
		}
	}
	return result;
}

static int gsm_msg_send(const char *phone_nr, const char *text)
{
	int result = -1;
	sprintf(gsm_line, "AT+CMGS=\"%s\"", phone_nr);
	gsm_send(gsm_line);
	gsm_flush();
	sprintf(gsm_line, "%s\x1a\x1a\x1a", text);
	gsm_send(gsm_line);
	if(!gsm_read(gsm_line, 5000)) {
		int matched, bytes = 0;
		matched = sscanf(gsm_line, "+CMGS: %d", &bytes);
		if(matched == 1) {
			printf("GSM: %d bytes has been sent successfully!!!\n", bytes);
			gsm_read(gsm_line, 500);
			result ++;
		}
	}
	return result;
}

void gsm_led_update(void)
{
	int led, result;
	gsm_send("AT+CMGL=\"ALL\"");
	while(!gsm_read(gsm_line, 500)) {
		if(!strcmp(gsm_line, "ERROR"))
			break;
		if(!strcmp(gsm_line, "OK"))
			break;

		if(!strncmp(gsm_line, "led=", 4)) {
			result = sscanf(gsm_line, "led=%d", &led);
			if(result == 1) {
				led_set_binary(led);
			}
		}
	}
}

int gsm_init(void)
{
	int result = -2;
	gsm_send("ATE 0");
	for(int i = 0; i < 3; i ++) {
		gsm_send("AT");
		if(!gsm_read(gsm_line, 500)) {
			if(!strcmp(gsm_line, "OK")) {
				result ++;
				printf("GSM: gsm module detected\n");
				break;
			}
		}
	}
	if(result == -1) {
		//check simcard status
		gsm_send("AT+CSMINS?");
		if(!gsm_read(gsm_line, 500)) {
			if(!strcmp(gsm_line, "+CSMINS: 0,1")) {
				printf("GSM: sim card detected\n");
				result ++;
			}
		}
	}

	mdelay(300);
	gsm_send("AT+GSV");
	mdelay(300);
	gsm_flush();
	return result;
}

void gsm_update(void)
{
	static time_t gsm_timer = 0;
	if(time_left(gsm_timer) < 0) {
		gsm_timer = time_get(10000);
		gsm_led_update();
	}
}

void main(void)
{
	sys_init();
	led_init();
	key_Init();
	gsm_init();
	for(int i = 1; i < 8; i ++)
		led_set_status(i, 1);

	while(1) {
		int key= key_GetKey();
		if(key != 0 && key != 255) {
			char text[32];
			led_set_status(key, 0);
			sprintf(text, "gsm demo: key S%d is pressed!!! reply 'led=0~255' to control the led.\n", key);
			gsm_msg_send(phone_nr, text);
			led_set_status(key, 1);
		}
		sys_update();
		gsm_update();
	}
}
