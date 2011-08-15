/*
 * 	sky@2010 initial version
 */
#include <string.h>
#include "sis_decode.h"
#include "stm32f10x.h"
#include "time.h"
#include "stdio.h"
#include "stdlib.h"
#include "drivers/sss_capture.h"
#include "time.h"
#include "flash.h"
#include "sss_card.h"
#include "drivers/sss_adc.h"
#include "drivers/sss_can.h"
#include "nvm.h"
#define threshold  150
#define drop1 4000
#define drop2 100
extern uint16_t capture_value1[400];
extern uint16_t capture_value2[400];
char message_study[500];
int cishu1=0,flag=0;
int dashu;
int sensor1[9];
int xiaoshu;
int xiaoshu4 __nvm;
int xiaoshu5 __nvm;
int xiaoshu6 __nvm;
int xiaoshu7 __nvm;
int sensor4[9] __nvm;
int sensor5[9] __nvm;
int sensor6[9] __nvm;
int sensor7[9] __nvm;

char manchester_decode(unsigned short int *mes1,unsigned short int *mes2,int size)
{
	int j,i=0;
	int clearance1,clearance2;
	for(j=0;j<size;j=j+1) {
		clearance1=mes2[j]-mes1[j];
		if(clearance1<0) {
			clearance1=60000-mes1[j]+mes2[j];
		}

		clearance2=mes1[j]-mes2[j-1];
		if(clearance2<0) {
			clearance2=60000-mes2[j-1]+mes1[j];
		}

		if(clearance1>drop1||clearance1<drop2) {
			continue;
		}


		if(clearance1<=(xiaoshu+threshold)&&clearance1>=(xiaoshu-threshold)) {
			flag++;
			if(flag==1) {
				message_study[i]=0;
				i++;
			}
			else {
				if(clearance2<=(xiaoshu+threshold)&&clearance2>=(xiaoshu-threshold)) {
					message_study[i]=message_study[i-1];
					i++;
				}
				else if(clearance2<=(dashu+threshold)&&clearance2>=(dashu-threshold)) {
					message_study[i]=!message_study[i-1];
					i++;
				}
				else {
					message_study[i]=0;
					i++;
				}
			}
		}


		if(clearance1<=(dashu+threshold)&&clearance1>=(dashu-threshold)) {
			if(clearance2<=(xiaoshu+threshold)&&clearance2>=(xiaoshu-threshold)) {
				message_study[i]=message_study[i-1];
				i++;
				message_study[i]=!message_study[i-1];
				i++;
			}
			else if(clearance2<=(dashu+threshold)&&clearance2>=(dashu-threshold)) {
				message_study[i]=!message_study[i-1];
				i++;
				message_study[i]=!message_study[i-1];
				i++;
			}
		}

	}
	cishu1=i;
	return 0;
}

void sensor_study4(void)
{
	char kk,i,j,k;
	static can_msg_t msg;
	int clearence1,clearence2;
	printf("the study is processing!\r\n");

	TIM_DMAC_Init();
	capture_sss_Init();
	sensor_poweron();
	mdelay(10);
	LED_MIX_ON();//turn on mix light
	for(kk=0;kk<5;kk++) {
		mdelay(1000);
	}
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	for(kk=0;kk<10;kk++) {
		clearence1=capture_value2[kk]-capture_value1[kk];
		if(clearence1<0) {
			clearence1=60000-capture_value1[kk]+capture_value2[kk];
		}
		if(clearence1<drop1&&clearence1>drop2) {
			clearence2=capture_value1[kk+1]-capture_value2[kk];
			if(clearence2<0) {
				clearence2=60000-capture_value1[kk+1]+capture_value2[kk];
			}
			xiaoshu=(clearence1+clearence2)/2;
			dashu=xiaoshu*2;
			xiaoshu4=xiaoshu;
			break;
		}
	}
	manchester_decode(capture_value1,capture_value2,400);
	for(i=0;i<8;i++) {
		sensor4[i]&=0;
		for(j=0;j<19;j++) {
			sensor4[i]|=(message_study[j+i*19]<<j);
		}
	}

        sensor4[8]&=0;
	for(k=0;k<19;k++) {
		sensor4[8]|=message_study[k+304]<<k;
	}
	nvm_save();

	memcpy(msg.data,sensor4,8);
	msg.id=0x0002;
	msg.dlc=8;
	msg.flag=0;
	mdelay(5);
	can_send(&msg);
	mdelay(100);
	*((u32 *)0xE000ED0C) = 0x05fa0004;
}

void sensor_study5(void)
{
	char kk,i,j,k;
	static can_msg_t msg;
	int clearence1,clearence2;
	printf("the study is processing!\r\n");
	TIM_DMAC_Init();
	capture_sss_Init();
	sensor_poweron();
	mdelay(10);

	LED_MIX_ON();//turn on mix light

	for(kk=0;kk<5;kk++) {
		mdelay(1000);
	}
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	for(kk=0;kk<10;kk++) {
		clearence1=capture_value2[kk]-capture_value1[kk];
		if(clearence1<0) {
			clearence1=60000-capture_value1[kk]+capture_value2[kk];
		}
		if(clearence1<drop1&&clearence1>drop2) {
			clearence2=capture_value1[kk+1]-capture_value2[kk];
			if(clearence2<0) {
				clearence2=60000-capture_value1[kk+1]+capture_value2[kk];
			}
			xiaoshu=(clearence1+clearence2)/2;
			dashu=xiaoshu*2;
			xiaoshu5=xiaoshu;
			break;
		}
	}
	manchester_decode(capture_value1,capture_value2,400);
	for(i=0;i<8;i++) {
		sensor5[i]&=0;
		for(j=0;j<19;j++) {
			sensor5[i]|=message_study[j+i*19]<<j;
		}
	}

        sensor5[8]&=0;
	for(k=0;k<19;k++) {
		sensor5[8]|=message_study[k+304]<<k;
	}
	nvm_save();

	memcpy(msg.data,sensor5,8);
	msg.id=0x0002;
	msg.dlc=8;
	msg.flag=0;
	mdelay(5);
	can_send(&msg);
	mdelay(100);
	*((u32 *)0xE000ED0C) = 0x05fa0004;
}

void sensor_study6(void)
{
	char kk,i,j,k;
	static can_msg_t msg;
	int clearence1,clearence2;
	printf("the study is processing!\r\n");
	LED_MIX_ON();
	TIM_DMAC_Init();
	capture_sss_Init();
	sensor_poweron();
	for(kk=0;kk<5;kk++) {
		mdelay(1000);
	}
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	for(kk=0;kk<10;kk++) {
		clearence1=capture_value2[kk]-capture_value1[kk];
		if(clearence1<0) {
			clearence1=60000-capture_value1[kk]+capture_value2[kk];
		}
		if(clearence1<drop1&&clearence1>drop2) {
			clearence2=capture_value1[kk+1]-capture_value2[kk];
			if(clearence2<0) {
				clearence2=60000-capture_value1[kk+1]+capture_value2[kk];
			}
			xiaoshu=(clearence1+clearence2)/2;
			dashu=xiaoshu*2;
			xiaoshu6=xiaoshu;
			break;
		}
	}
	manchester_decode(capture_value1,capture_value2,400);
	for(i=0;i<8;i++) {
		sensor6[i]&=0;
		for(j=0;j<19;j++) {
			sensor6[i]|=message_study[j+i*19]<<j;
		}
	}
        sensor6[8]&=0;
	for(k=0;k<19;k++) {
		sensor6[8]|=message_study[k+304]<<k;
	}
	nvm_save();

	memcpy(msg.data,sensor6,8);
	msg.id=0x0002;
	msg.dlc=8;
	msg.flag=0;
	mdelay(5);
	can_send(&msg);
	mdelay(100);

	*((u32 *)0xE000ED0C) = 0x05fa0004;
}

void sensor_study7(void)
{
	char kk,i,j,k;
	static can_msg_t msg;
	int clearence1,clearence2;
	printf("the study is processing!\r\n");
	TIM_DMAC_Init();
	capture_sss_Init();
	sensor_poweron();
	mdelay(10);

	LED_MIX_ON();//turn on mix light

	for(kk=0;kk<5;kk++) {
		mdelay(1000);
	}
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	for(kk=0;kk<10;kk++) {
		clearence1=capture_value2[kk]-capture_value1[kk];
		if(clearence1<0) {
			clearence1=60000-capture_value1[kk]+capture_value2[kk];
		}
		if(clearence1<drop1&&clearence1>drop2) {
			clearence2=capture_value1[kk+1]-capture_value2[kk];
			if(clearence2<0) {
				clearence2=60000-capture_value1[kk+1]+capture_value2[kk];
			}
			xiaoshu=(clearence1+clearence2)/2;
			dashu=xiaoshu*2;
			xiaoshu7=xiaoshu;
			break;
		}
	}
	manchester_decode(capture_value1,capture_value2,400);
	for(i=0;i<8;i++) {
		sensor7[i]&=0;
		for(j=0;j<19;j++) {
			sensor7[i]|=message_study[j+i*19]<<j;
		}
	}

        sensor7[8]&=0;
	for(k=0;k<19;k++) {
		sensor7[8]|=message_study[k+304]<<k;
	}
	nvm_save();

	memcpy(msg.data,sensor7,8);
	msg.id=0x0002;
	msg.dlc=8;
	msg.flag=0;
	mdelay(5);
	can_send(&msg);
	mdelay(100);
	*((u32 *)0xE000ED0C) = 0x05fa0004;
}
