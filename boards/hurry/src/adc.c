/* adc.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "adc.h"

/*private functions*/
/*******************************************************************************
* Function Name  : SVPWM_InjectedConvConfig
* Description    : This function configure ADC1 for 3 shunt current 
*                  reading and temperature and voltage feedbcak after a 
*                  calibration of the three utilized ADC Channels
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void adc_Config(void)
{
	/* ADC1 Injected conversions configuration */ 
	ADC_InjectedSequencerLengthConfig(ADC1,2);
	ADC_InjectedSequencerLengthConfig(ADC2,2);
#if 1
	/* ADC2 Injected conversions configuration */ 
	ADC_InjectedChannelConfig(ADC2, PHASE_A_ADC_CHANNEL, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC2, TOTAL_CHANNEL, 2, ADC_SampleTime_7Cycles5);
  
	ADC_InjectedChannelConfig(ADC1,PHASE_B_ADC_CHANNEL, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1,VDC_CHANNEL,2, ADC_SampleTime_28Cycles5);
#endif
	/* ADC1 Injected conversions trigger is TIM1 TRGO */ 
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO); 
  
	ADC_ExternalTrigInjectedConvCmd(ADC2,ENABLE);

#if 0
	/* Bus voltage protection initialization*/                            
	ADC_AnalogWatchdogCmd(ADC1,ADC_AnalogWatchdog_SingleInjecEnable);
	ADC_AnalogWatchdogSingleChannelConfig(ADC1,BUS_VOLT_FDBK_CHANNEL);
	ADC_AnalogWatchdogThresholdsConfig(ADC1, OVERVOLTAGE_THRESHOLD>>3,0x00);
#endif

	/* Clear ADC1 JEOC pending interrupt bit */
	ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);  
	/* ADC1 Injected group of conversions end and Analog Watchdog interrupts enabling */
	ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
}


/*******************************************************************************
* Function Name  : adc_GetCalibration
* Description    : Store zero current converted values for current reading 
                   network offset compensation in case of 3 shunt resistors 
* Input          : uint16_t * phaseAOffset,uint16_t *phaseAOffset,uint16_t *phaseAOffset
* Output         : uint16_t * phaseAOffset,uint16_t *phaseAOffset,uint16_t *phaseAOffset
* Return         : uint16_t * phaseAOffset,uint16_t *phaseAOffset,uint16_t *phaseAOffset
*******************************************************************************/

void adc_GetCalibration(uint16_t * pPhaseAOffset,uint16_t *pPhaseBOffset,uint16_t *pPhaseCOffset)
{
	static u16 bIndex;
  
	/* ADC1 Injected group of conversions end interrupt disabling */
	ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
  
	*pPhaseAOffset=0;
	*pPhaseBOffset=0;
	*pPhaseCOffset=0;
  
	/* ADC1 Injected conversions trigger is given by software and enabled */ 
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);  
	ADC_ExternalTrigInjectedConvCmd(ADC1,ENABLE); 
  
	/* ADC1 Injected conversions configuration */ 
	ADC_InjectedSequencerLengthConfig(ADC1,3);
	ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,2,ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, PHASE_C_ADC_CHANNEL,3,ADC_SampleTime_7Cycles5);
  
	/* Clear the ADC1 JEOC pending flag */
	ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);  
	ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
   
	/* ADC Channel used for current reading are read 
     in order to get zero currents ADC values*/ 
	for(bIndex=0; bIndex <NB_CONVERSIONS; bIndex++){
		while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC)) { }
    
		*pPhaseAOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_1)>>3);
		*pPhaseBOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2)>>3);
		*pPhaseCOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_3)>>3);    
        
		/* Clear the ADC1 JEOC pending flag */
		ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);    
		ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
	}
  
	adc_Config();  
}

/**
  * @brief  Configures NVIC and Vector Table base location.
  * @param  None
  * @retval None
  */
static void ADC_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable ADC1_2 IRQChannel */
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*zf32 board: PA1/ADC_IN1, JP1 R7 POT_WHEEL 0~3v3*/
void adc_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	ADC_NVIC_Configuration();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	/* Configure PA.4 PA.5 as Analog Input,channel phase A,phase B */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure PC.4 PC.5 PC.1 as Analog Input,channel phase C,total,vdc */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* ADC1 registers reset ----------------------------------------------------*/
	ADC_DeInit(ADC1);
	/* ADC2 registers reset ----------------------------------------------------*/
	ADC_DeInit(ADC2);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
	/* Enable ADC2 */
	ADC_Cmd(ADC2, ENABLE);	

	/* ADC1 configuration ------------------------------------------------------*/
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_InjecSimult;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
   
	/* ADC2 Configuration ------------------------------------------------------*/
	ADC_StructInit(&ADC_InitStructure);  
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC2, &ADC_InitStructure);
  
	// Start calibration of ADC1
	ADC_StartCalibration(ADC1);
	// Start calibration of ADC2
	ADC_StartCalibration(ADC2);

	// Wait for the end of ADCs calibration 
	while (ADC_GetCalibrationStatus(ADC1) & ADC_GetCalibrationStatus(ADC2));

#if 0
	/* ADC1 injected channel Configuration */ 
	ADC_InjectedChannelConfig(ADC1, TOTAL_CHANNEL, 1, ADC_SampleTime_71Cycles5);
	/* Enable automatic injected conversion start after regular one */	
	//ADC_AutoInjectedConvCmd(ADC1, ENABLE);	
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO);
	/* Enable ADC1 external trigger */ 
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);

	/* Clear ADC1 JEOC pending interrupt bit */
	ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
	/* Enable JEOC interupt */
	ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
#endif
	//ADC_ITConfig(ADC1, ADC_IT_EOC|ADC_IT_AWD|ADC_IT_JEOC, DISABLE);	
}

/*ext voltage 0~3v3*/
int adc_GetVolt(uint8_t ch)
{
	int value;
	
#ifdef ADC_MODE_NORMAL
	ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	value = ADC_GetConversionValue(ADC1);
#endif

#ifdef ADC_MODE_INJECT
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
	ADC_InjectedChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
	ADC_InjectedSequencerLengthConfig(ADC1, 1);

	ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
	ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC));
	value = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
#endif

	value &= 0x0fff;
	return ADC_COUNT_TO_VOLTAGE(value);
}

float adc_GetTemp(void)
{
	int temp1 = 0;
        float temp2 = 0;
	/*int temp sensor, sampling time -> 17.1us*/
	ADC_TempSensorVrefintCmd(ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	temp1 = ADC_GetConversionValue(ADC1);
        
	temp1 &= 0x0fff;
        temp2 =(V25- (float)temp1*3300/4096)/(float)AVG_SLOPE + 25.0; 
	return temp2;
}

void adc_SetChannel(ADC_TypeDef* ADCx,uint8_t ch)
{

}

void adc_Update(void)
{

}