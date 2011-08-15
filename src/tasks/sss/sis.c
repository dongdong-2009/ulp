
/*
 *	sky@2011 initial version
 */
#include "time.h"
#include "sis.h"
#include "stm32f10x.h"
#include "drivers/sss_pwm.h"
#include "drivers/sss_adc.h"
#include "sys/task.h"
#include "sis_code.h"
#include "flash.h"
#include "sis_can.h"
#include "sss_card.h"
#include "config.h"
#include "drivers/sss_uart.h"
#include "nvm.h"
int test1[10] __nvm;

//....................................................................................
int hex_message1[8] = {
	0x21820,0x58120,0x1c020,0x788a0,0x1c020,0x402a0,0x563a0,0x5f060
};
int hex_message2[8] = {
	0x44420,0x58120,0x1c020,0x7f3a0,0x73260,0x58120,0x563a0,0x08260
};
int hex_message3[8] = {
	0x35020,0x58120,0x16420,0x3dd20,0x67020,0x011a0,0x4b220,0x60b20
};
//...................................................
int card_address;
extern uint16_t size2,message[300];
extern char can_query[8];
extern char messageb[19];
extern char message_study[500];
extern int xiaoshu4,xiaoshu5,xiaoshu6,xiaoshu7;
extern int sensor4[9],sensor5[9],sensor6[9],sensor7[9];
int hex_message[9],message_speed,message_delay;
char okmode=0;
char acceleration=0;
uint16_t okmessage[68] = {
	100,200,300,400,500,600,700,800,900,1000,1100,1200,1300,1500,1700,
	1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000,
	3100,3300,3400,3500,3600,3700,4300,4400,4500,4600,4700,4800,4900,5000,
	5100,5200,5300,5400,5500,5700,5900,6000,6100,6200,6300,6400,6500,
	6600,6700,6800,6900,7000,7100,7200,7300,7500,7600,7700,7800,7900
};

uint16_t acc_message[34] = {
	100,200,300,400,500,700,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,
	2300,2400,2500,2600,2700,2900,3000,3100,3200,3300,3400,3500,3600,3700
};


int send_message(void)
{
	DMA1_Channel7->CCR&=~(1<<0); //关闭DMA传输
	DMA1_Channel7->CNDTR=34;  //DMA1,传输数据量
	DMA1_Channel7->CCR|=1<<0;
	TIM_SetCounter(TIM2,0);
	return 0;
}

void manchester_init(void)
{
	ADDRESS_GPIO_Init();
	card_address=ADDRESS_GPIO_Read();
	ADC1_Init();
	TIM3_init();
	TIM3ch3_init();
	TIM3ch4_init();
	printf("the int is OK!\r\n");

}
  //....................................................................
void manchester_update(void)
{
	int dely;
	can1_updata();
	LED_RED_ON();
	if (adc_getvalue()>1500) {
		char i;
		int mode=0;
		char sensor_select;
		LED_GREEN_ON();//turn on green light
		mdelay(136);
		size2=0;
		TIM_Cmd(TIM2, DISABLE);
		sensor_select=can_query[2];
		if(sensor_select==1) {
			message_speed=15;
			pwm_init(message_speed);
			memcpy(hex_message,hex_message1,32);
			for(i=0;i<8;i++) {
				value_change(hex_message[i]);
				manchester_valu(messageb);
			}
			okmode=1;
			acceleration=0;
			printf("the sensor is 1\r\n");
		}
		else if(sensor_select==2) {
			message_speed=7;
			pwm_init(message_speed);
			memcpy(hex_message,hex_message2,32);
			for(i=0;i<8;i++){
				value_change(hex_message[i]);
				manchester_valu(messageb);
			}
			okmode=1;
			acceleration=0;
			printf("the sensor is 2\r\n");
		}
		else if(sensor_select==3) {
			message_speed=7;
			pwm_init(message_speed);
			memcpy(hex_message,hex_message3,32);
			for(i=0;i<8;i++) {
				value_change(hex_message[i]);
				manchester_valu(messageb);
			}
			acceleration=1;
			okmode=0;
			printf("the sensor is 3\r\n");
		}
		else if(sensor_select==4) {
			if(xiaoshu4>1578&&xiaoshu4<1678)
				message_speed=15;
			else if(xiaoshu4>763&&xiaoshu4<863)
				message_speed=7;
			else if(xiaoshu4>356&&xiaoshu4<456)
				message_speed=3;
			else
				message_speed=1;
			pwm_init(message_speed);
			for(i=0;i<8;i++) {
				value_change(sensor4[i]);
				manchester_valu(messageb);
			}
			mode=sensor4[8]&(1<<3);
			if(mode==8) {
				acceleration=1;
				okmode=0;
			}
			else{
				acceleration=0;
				okmode=1;
			}
			printf("the sensor is 4\r\n");
		}
		else if(sensor_select==5){
			if(xiaoshu5>1578&&xiaoshu5<1678)
				message_speed=15;
			else if(xiaoshu5>763&&xiaoshu5<863)
				message_speed=7;
			else if(xiaoshu5>356&&xiaoshu5<456)
				message_speed=3;
			else
				message_speed=1;
			pwm_init(message_speed);
			for(i=0;i<8;i++) {
				value_change(sensor5[i]);
				manchester_valu(messageb);
			}
			mode=sensor5[8]&(1<<3);
			if(mode==8) {
				acceleration=1;
				okmode=0;
			}
			else{
				acceleration=0;
				okmode=1;
			}
			printf("the sensor is 5\r\n");
		}
		else if(sensor_select==6) {
			mode=0;
			if(xiaoshu6>1578&&xiaoshu6<1678)
				message_speed=15;
			else if(xiaoshu6>763&&xiaoshu6<863)
				message_speed=7;
			else if(xiaoshu6>356&&xiaoshu6<456)
				message_speed=3;
			else
				message_speed=1;
			pwm_init(message_speed);
			for(i=0;i<8;i++) {
				value_change(sensor6[i]);
				manchester_valu(messageb);
			}
			mode=sensor6[8]&(1<<3);
			if(mode==8){
				acceleration=1;
				okmode=0;
			}else{
				acceleration=0;
				okmode=1;
			}
			printf("the sensor is 4\r\n");
		}
		else if(sensor_select==7) {
			if(xiaoshu7>1578&&xiaoshu7<1678)
				message_speed=15;
			else if(xiaoshu7>763&&xiaoshu7<863)
				message_speed=7;
			else if(xiaoshu7>356&&xiaoshu7<456)
				message_speed=3;
			else
				message_speed=1;
			pwm_init(message_speed);
			for(i=0;i<8;i++) {
				value_change(sensor7[i]);
				manchester_valu(messageb);
			}
			mode=sensor7[8]&(1<<3);
			if(mode==8){
				acceleration=1;
				okmode=0;
			}else{
				acceleration=0;
				okmode=1;
			}
			printf("the sensor is 4\r\n");
		}
		else {
			message_speed=15;
			pwm_init(message_speed);
			memcpy(hex_message,hex_message1,32);
			for(i=0;i<8;i++) {
				value_change(hex_message[i]);
				manchester_valu(messageb);
			}
			okmode=1;
			acceleration=0;
			printf("the sensor is not existed!\r\n");
		}
		dely=850*(message_speed+1)/16;
		TIM_Cmd(TIM2, DISABLE);
		TIM_SetCounter(TIM2,0);
		TIM_DMA_Init(message,size2,DMA_Mode_Normal);
		ch2_init(message);
		while((DMA_GetFlagStatus(DMA1_FLAG_TC7)==RESET)||(TIM_GetCounter(TIM2)<=message[size2-1]));
		TIM_Cmd(TIM2, DISABLE);
		mdelay(17);
		TIM_Cmd(TIM2, DISABLE);
		TIM_DMA_Init(message,size2,DMA_Mode_Normal);
		TIM_SetCounter(TIM2,0);
		ch2_init(message);
		while((DMA_GetFlagStatus(DMA1_FLAG_TC7)==RESET)||(TIM_GetCounter(TIM2)<=message[size2-1]));
		TIM_Cmd(TIM2, DISABLE);
		mdelay(17);
		while(adc_getvalue()>1500) {
			if(okmode==1) {
				TIM_Cmd(TIM2, DISABLE);
				TIM_SetCounter(TIM2,0);
				TIM_DMA_Init(okmessage,68,DMA_Mode_Normal);
				ch2_init(okmessage);
				while((DMA_GetFlagStatus(DMA1_FLAG_TC7)==RESET)||(TIM_GetCounter(TIM2)<=okmessage[67]));
				TIM_Cmd(TIM2, DISABLE);
				mdelay(1000);
			}
			if (acceleration==1) {
				TIM_Cmd(TIM2, DISABLE);
				TIM_SetCounter(TIM2,0);
				TIM_DMA_Init(acc_message,34,DMA_Mode_Normal);
				ch2_init(acc_message);
				while((DMA_GetFlagStatus(DMA1_FLAG_TC7)==RESET)||(TIM_GetCounter(TIM2)<=okmessage[33]));
				TIM_Cmd(TIM2, DISABLE);
				udelay(dely);
			}
			can1_updata();
		}
	}
}

int main(void)
{
	task_Init();
	can_int();
	manchester_init();
	for(;;) {
		task_Update();
		manchester_update();
	}
}



