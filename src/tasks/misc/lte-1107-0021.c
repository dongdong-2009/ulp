/*
 * 	peng.guo@2011 initial version
 */

#include "config.h"
#include "debug.h"
#include "ulp_time.h"
#include "stm32f10x_gpio.h"
#include "sys/task.h"
#include "74hct595.h"

#define LED_GREEN				(1<<12)
#define LED_RED					(1<<10)
#define BO_0					(1<<6)
#define BO_1					(1<<7)
#define BO_2					(1<<8)
#define BO_3					(1<<9)
#define BO_CTRL					(1<<8)
#define WLI_0					(1<<0)
#define WLI_1					(1<<1)
#define WLI_2					(1<<2)
#define WLI_3					(1<<3)
#define test1					(1<<6)
#define test2					(1<<7)
#define NSS					(1<<4)


static nxp_74hct595_t chip = {
	.bus = &spi1,
	.idx = SPI_1_NSS,
};

static int a1=1,a2=1,a3=1,i1=1,i2,i3,i4,i5,b1=1,b2=1,b3=1,b4=1,b5=1,b6=1;
static time_t d1=0,d2=0,d3=0,d4=0,d5=0,d6=0,d7=0,d8=0;
/*	a1，a2，a3让时间捕获不会重复赋值
	i1，i2,可以使能或者失能按键
	i3，i4，i5失能或者使能else中的语句
	b1~b6，不重复执行时间到达函数
	d1~d7是时间赋值的目标变量*/

void spc_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_7|GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOC->ODR=0x00000000;
	GPIOA->ODR&=~BO_CTRL;					//扬声器断电
	nxp_74hct595_Init(&chip);				//初始化移位寄存器
	nxp_74hct595_WriteByte(&chip,0x00);			//七段数码管灭
	GPIOA->ODR&=~NSS;
}

void spc_Update(void)
{
	if(((GPIOC->IDR&WLI_0)==WLI_0)&&(i1==1)) {
		i2=i3=1;					//可以发令
		i1=0;						//复位前不再有反应
		GPIOC->ODR|=LED_GREEN;				//绿灯亮
		GPIOC->ODR&=~LED_RED;				//红灯灭
		GPIOA->ODR|=BO_CTRL;				//扬声器供电
		GPIOC->ODR|=BO_2;				//发出准备声
	}
	else if(i3==1) {
		if(a1==1) {
			nxp_74hct595_WriteByte(&chip,0xda);	//显示5
			d1=time_get(1000);			//d1取1s以后的值
			d2=time_get(8000);			//给d2~d6赋值5s以后的一个值
			d3=time_get(8000);
			d4=time_get(8000);
			d5=time_get(8000);
			d6=time_get(8000);
			}
			a1=0;					//循环时不会执行上句程序
			if((time_left(d1)<0)&&b1==1) {
				GPIOC->ODR&=~BO_2;		//准备声消
				GPIOA->ODR&=~BO_CTRL;
				nxp_74hct595_WriteByte(&chip,0xcc);//显示4
				d2=time_get(1000);
				b1=0;
			}
			if((time_left(d2)<0)&&b2==1) {
				nxp_74hct595_WriteByte(&chip,0x9e);//显示3
				d3=time_get(1000);
				b2=0;				//不会重复执行
			}
			if((time_left(d3)<0)&&b3==1) {
				nxp_74hct595_WriteByte(&chip,0xb6);//显示2
				d4=time_get(1000);
				b3=0;
			}
			if((time_left(d4)<0)&&b4==1) {
				nxp_74hct595_WriteByte(&chip,0x0c);//显示1
				d5=time_get(1000);
				b4=0;
			}
			if((time_left(d5)<0)&&b5==1) {
				GPIOA->ODR|=BO_CTRL;
				GPIOC->ODR|=BO_1;		//第二种声音，即报警声
				GPIOC->ODR&=~LED_GREEN;		//绿灯灭
				GPIOC->ODR|=LED_RED;		//红灯亮
				nxp_74hct595_WriteByte(&chip,0xe2);//显示F
				d6=time_get(4000);		//持续4S
				b5=i2=0;
			}
			if((time_left(d6)<0)&&b6==1) {
				GPIOA->ODR&=~BO_CTRL;
				GPIOC->ODR&=~BO_1;		//声音消
				i3=b6=0;			//循环不会重复执行
			}
		}
	if((GPIOC->IDR&WLI_1)==WLI_1) {				//接收到犯规信号
		nxp_74hct595_WriteByte(&chip,0xe2);		//显示F
		GPIOC->ODR&=~LED_GREEN;				//绿灯灭
		GPIOC->ODR|=LED_RED;				//红灯亮
		GPIOA->ODR|=BO_CTRL;
		GPIOC->ODR|=BO_1;				//报警声响
		i1=i2=i3=0;					//不能启动,准备期间犯规，则取消准备状态
		i4=1;						//下面else的语句可以执行了
		b1=b2=b3=b4=b5=b6=0;				//同上注释，双重保护
	}
	else if(i4==1) {
			if(a2==1)
				d7=time_get(4000);
			a2=0;
			if(time_left(d7)<0) {			//持续4S
				GPIOC->ODR&=~BO_1;		//报警声消
				GPIOA->ODR&=~BO_CTRL;
				i4=0;
				a2=1;
			}
		}
	if(((GPIOC->IDR&WLI_2)==WLI_2)&&(i2==1)) {		//接收到启动信号
		GPIOA->ODR|=BO_CTRL;
		GPIOC->ODR|=BO_0;				//启动声音响
		i2=i3=0;					//准备期间犯规，则取消准备状态
		i5=1;
		b1=b2=b3=b4=b5=b6=0;				//同上注释，双重保护
		GPIOC->ODR|=LED_GREEN;				//绿灯亮
		GPIOC->ODR&=~LED_RED;
	}
	else if(i5==1) {
			if(a3==1)
				d8=time_get(1000);
			a3=0;
			if(time_left(d8)<0) {
				GPIOC->ODR&=~BO_0;		//启动声持续1S
				GPIOA->ODR&=~BO_CTRL;
				i5=0;
			}
		}
	if((GPIOC->IDR&WLI_3)==WLI_3) {				//接收到复位信号
		i1=1;						//复位后可以按准备键
		i2=i3=0;
		a1=a2=a3=1;
		b1=b2=b3=b4=b5=b6=1;
		nxp_74hct595_WriteByte(&chip,0x00);
		GPIOA->ODR&=~BO_CTRL;				//扬声器断电
		GPIOC->ODR&=~LED_RED;				//红灯灭
		GPIOC->ODR&=~LED_GREEN;				//绿灯灭
		GPIOC->ODR&=~BO_2;
		GPIOC->ODR&=~BO_1;
		GPIOC->ODR&=~BO_0;
		}
	}

void main(void)
{

	task_Init();
	spc_Init();
	while(1) {
		task_Update();
		spc_Update();
	}
}