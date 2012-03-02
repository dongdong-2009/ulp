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
/*	a1��a2��a3��ʱ�䲶�񲻻��ظ���ֵ
	i1��i2,����ʹ�ܻ���ʧ�ܰ���
	i3��i4��i5ʧ�ܻ���ʹ��else�е����
	b1~b6�����ظ�ִ��ʱ�䵽�ﺯ��
	d1~d7��ʱ�丳ֵ��Ŀ�����*/

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
	GPIOA->ODR&=~BO_CTRL;					//�������ϵ�
	nxp_74hct595_Init(&chip);				//��ʼ����λ�Ĵ���
	nxp_74hct595_WriteByte(&chip,0x00);			//�߶��������
	GPIOA->ODR&=~NSS;
}

void spc_Update(void)
{
	if(((GPIOC->IDR&WLI_0)==WLI_0)&&(i1==1)) {
		i2=i3=1;					//���Է���
		i1=0;						//��λǰ�����з�Ӧ
		GPIOC->ODR|=LED_GREEN;				//�̵���
		GPIOC->ODR&=~LED_RED;				//�����
		GPIOA->ODR|=BO_CTRL;				//����������
		GPIOC->ODR|=BO_2;				//����׼����
	}
	else if(i3==1) {
		if(a1==1) {
			nxp_74hct595_WriteByte(&chip,0xda);	//��ʾ5
			d1=time_get(1000);			//d1ȡ1s�Ժ��ֵ
			d2=time_get(8000);			//��d2~d6��ֵ5s�Ժ��һ��ֵ
			d3=time_get(8000);
			d4=time_get(8000);
			d5=time_get(8000);
			d6=time_get(8000);
			}
			a1=0;					//ѭ��ʱ����ִ���Ͼ����
			if((time_left(d1)<0)&&b1==1) {
				GPIOC->ODR&=~BO_2;		//׼������
				GPIOA->ODR&=~BO_CTRL;
				nxp_74hct595_WriteByte(&chip,0xcc);//��ʾ4
				d2=time_get(1000);
				b1=0;
			}
			if((time_left(d2)<0)&&b2==1) {
				nxp_74hct595_WriteByte(&chip,0x9e);//��ʾ3
				d3=time_get(1000);
				b2=0;				//�����ظ�ִ��
			}
			if((time_left(d3)<0)&&b3==1) {
				nxp_74hct595_WriteByte(&chip,0xb6);//��ʾ2
				d4=time_get(1000);
				b3=0;
			}
			if((time_left(d4)<0)&&b4==1) {
				nxp_74hct595_WriteByte(&chip,0x0c);//��ʾ1
				d5=time_get(1000);
				b4=0;
			}
			if((time_left(d5)<0)&&b5==1) {
				GPIOA->ODR|=BO_CTRL;
				GPIOC->ODR|=BO_1;		//�ڶ�����������������
				GPIOC->ODR&=~LED_GREEN;		//�̵���
				GPIOC->ODR|=LED_RED;		//�����
				nxp_74hct595_WriteByte(&chip,0xe2);//��ʾF
				d6=time_get(4000);		//����4S
				b5=i2=0;
			}
			if((time_left(d6)<0)&&b6==1) {
				GPIOA->ODR&=~BO_CTRL;
				GPIOC->ODR&=~BO_1;		//������
				i3=b6=0;			//ѭ�������ظ�ִ��
			}
		}
	if((GPIOC->IDR&WLI_1)==WLI_1) {				//���յ������ź�
		nxp_74hct595_WriteByte(&chip,0xe2);		//��ʾF
		GPIOC->ODR&=~LED_GREEN;				//�̵���
		GPIOC->ODR|=LED_RED;				//�����
		GPIOA->ODR|=BO_CTRL;
		GPIOC->ODR|=BO_1;				//��������
		i1=i2=i3=0;					//��������,׼���ڼ䷸�棬��ȡ��׼��״̬
		i4=1;						//����else��������ִ����
		b1=b2=b3=b4=b5=b6=0;				//ͬ��ע�ͣ�˫�ر���
	}
	else if(i4==1) {
			if(a2==1)
				d7=time_get(4000);
			a2=0;
			if(time_left(d7)<0) {			//����4S
				GPIOC->ODR&=~BO_1;		//��������
				GPIOA->ODR&=~BO_CTRL;
				i4=0;
				a2=1;
			}
		}
	if(((GPIOC->IDR&WLI_2)==WLI_2)&&(i2==1)) {		//���յ������ź�
		GPIOA->ODR|=BO_CTRL;
		GPIOC->ODR|=BO_0;				//����������
		i2=i3=0;					//׼���ڼ䷸�棬��ȡ��׼��״̬
		i5=1;
		b1=b2=b3=b4=b5=b6=0;				//ͬ��ע�ͣ�˫�ر���
		GPIOC->ODR|=LED_GREEN;				//�̵���
		GPIOC->ODR&=~LED_RED;
	}
	else if(i5==1) {
			if(a3==1)
				d8=time_get(1000);
			a3=0;
			if(time_left(d8)<0) {
				GPIOC->ODR&=~BO_0;		//����������1S
				GPIOA->ODR&=~BO_CTRL;
				i5=0;
			}
		}
	if((GPIOC->IDR&WLI_3)==WLI_3) {				//���յ���λ�ź�
		i1=1;						//��λ����԰�׼����
		i2=i3=0;
		a1=a2=a3=1;
		b1=b2=b3=b4=b5=b6=1;
		nxp_74hct595_WriteByte(&chip,0x00);
		GPIOA->ODR&=~BO_CTRL;				//�������ϵ�
		GPIOC->ODR&=~LED_RED;				//�����
		GPIOC->ODR&=~LED_GREEN;				//�̵���
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