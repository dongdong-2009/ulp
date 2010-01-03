/* vsm.c
 * 	dusk@2009 initial version
 */

#include "stm32f10x.h"
#include "vsm.h"
#include "pwm.h"
#include "adc.h"

/*extern varible*/
extern int g_BusVoltage;
/* Private variables */
static unsigned char  bSector;
unsigned short hPhaseAOffset;
unsigned short hPhaseBOffset;
unsigned short hPhaseCOffset;

void vsm_Init(void)
{
	adc_Init();
	adc_GetCalibration(&hPhaseAOffset,&hPhaseBOffset,&hPhaseCOffset);
	pwm_Init();
}

void vsm_Update(void)
{
}

/* config the duty cycle */
void vsm_SetVoltage(int alpha,int beta)
{
	unsigned short  hTimePhA, hTimePhB, hTimePhC;
        unsigned short PhaseVoltage;
	unsigned short T1,T2,T0;
	int wX, wY, wZ, wUAlpha, wUBeta;
	int tX,tY,tZ;
	char a,b,c;
	/*BusVoltage = 1.7321*PhaseVoltage,vq<=10000*/
	g_BusVoltage = 17330;
        /* PhaseVoltage = g_BusVoltage/1.7321*/
	PhaseVoltage = 10000;
	
	wUAlpha = (int)((float)alpha * SQRT_3);
	wUBeta = beta;
	
	/*for calc duty time*/
	tX = wUBeta;
	tY = (wUAlpha + wUBeta)/2;
	tZ = (-wUAlpha + wUBeta)/2;
	
	/*for calc sector*/
	wX = wUBeta;
	wY = (-wUBeta + wUAlpha)/2;
	wZ = (-wUBeta - wUAlpha)/2;
	if(wX > 0)
		a = 1;
	else
		a = 0;
	if(wY > 0)
		b = 1;
	else
		b = 0;
	if(wZ > 0)
		c = 1;
	else
		c = 0;
	bSector = 4*c + 2*b + a;

	/* Duty cycles computation */
	switch(bSector){  
	case SECTOR_1:
			T1 = tZ*PWM_PERIOD/PhaseVoltage;
			T2 = tY*PWM_PERIOD/PhaseVoltage;
			T0 = PWM_PERIOD - T1 - T2;
			
			hTimePhB = T0/2;
			hTimePhA = hTimePhB + T1;
			hTimePhC = hTimePhA + T2;		
	
			ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);     
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;                                         
			break;
	case SECTOR_2:	
			T1 = tY*PWM_PERIOD/PhaseVoltage;
			T2 = (-tX)*PWM_PERIOD/PhaseVoltage;
			T0 = PWM_PERIOD - T1 - T2;
			
			hTimePhA = T0/2;
			hTimePhC = hTimePhA + T1;	
			hTimePhB = hTimePhC + T2;

			ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;

	case SECTOR_3:
			T1 = (-tZ)*PWM_PERIOD/PhaseVoltage;
			T2 = tX*PWM_PERIOD/PhaseVoltage;
			T0 = PWM_PERIOD - T1 - T2;
			
			hTimePhA = T0/2;
			hTimePhB = hTimePhA + T1;
			hTimePhC = hTimePhB + T2;

			ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;
    
    case SECTOR_4:
			T1 = (-tX)*PWM_PERIOD/PhaseVoltage;
			T2 = tZ*PWM_PERIOD/PhaseVoltage;
			T0 = PWM_PERIOD - T1 - T2;
			
			hTimePhC = T0/2;
			hTimePhB = hTimePhC + T1;
			hTimePhA = hTimePhB + T2;					

			ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;  

	case SECTOR_5:
			T1 = tX*PWM_PERIOD/PhaseVoltage;
			T2 = (-tY)*PWM_PERIOD/PhaseVoltage;
			T0 = PWM_PERIOD - T1 - T2;
			
			hTimePhB = T0/2;
			hTimePhC = hTimePhB + T1;
			hTimePhA = hTimePhC + T2;			

			ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;
                
	case SECTOR_6:
			T1 = (-tY)*PWM_PERIOD/PhaseVoltage;
			T2 = (-tZ)*PWM_PERIOD/PhaseVoltage;
			T0 = PWM_PERIOD - T1 - T2;
			
			hTimePhC = T0/2;	
			hTimePhA = hTimePhC + T1;
			hTimePhB = hTimePhA + T2;			

			ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;
	default:
			break;
	}
 
	/* Load compare registers values */ 
	TIM1->CCR1 = hTimePhA;
	TIM1->CCR2 = hTimePhB;
	TIM1->CCR3 = hTimePhC;
	//TIM1->CCR4 = hTimePhD; // To Syncronyze the ADC
}

/*
*
*/
void vsm_GetCurrent(int *Ia, int *Ib)
{
	s32 wAux;

	switch (bSector){
	case 4:
	case 5: //Current on Phase C not accessible     
			// Ia = (hPhaseAOffset)-(ADC Channel 11 value)  
			wAux = (s32)(hPhaseAOffset)- ((ADC1->JDR1)>>3);          
			//Saturation of Ia 
			if (wAux < S16_MIN){
				*Ia = S16_MIN;
			}
            else  if (wAux > S16_MAX){ 
				*Ia = S16_MAX;
			}
			else{
				*Ia = wAux;
			}                     
			// Ib = (hPhaseBOffset)-(ADC Channel 12 value)
			wAux = (s32)(hPhaseBOffset)-((ADC2->JDR1)>>3);
			// Saturation of Ib
			if (wAux < S16_MIN){
				*Ib = S16_MIN;
			}
			else  if (wAux > S16_MAX){ 
				*Ib = S16_MAX;
			}
			else{ 
				*Ib = wAux;
			}
			break;
			
	case 6:
	case 1:  //Current on Phase A not accessible     
			// Ib = (hPhaseBOffset)-(ADC Channel 12 value)
			wAux = (s32)(hPhaseBOffset)-((ADC1->JDR1)>>3);
			//Saturation of Ib 
			if (wAux < S16_MIN){
				*Ib = S16_MIN;
			}  
			else  if (wAux > S16_MAX){ 
				*Ib = S16_MAX;
			}
			else{
				*Ib = wAux;
			}
			// Ia = -Ic -Ib 
			wAux = ((ADC2->JDR1)>>3)-hPhaseCOffset- (*Ib );
			//Saturation of Ia
			if (wAux> S16_MAX){
				*Ia = S16_MAX;
			}
			else  if (wAux <S16_MIN){
				*Ia = S16_MIN;
			}
			else{  
				*Ia = wAux;
			}
			break;
           
	case 2:
	case 3:  // Current on Phase B not accessible
			// Ia = (hPhaseAOffset)-(ADC Channel 11 value)     
			wAux = (s32)(hPhaseAOffset)-((ADC1->JDR1)>>3);
			//Saturation of Ia 
			if (wAux < S16_MIN){
				*Ia = S16_MIN;
            }  
            else  if (wAux > S16_MAX){ 
				*Ia = S16_MAX;
			}
			else{
				*Ia = wAux;
			}
			
			// Ib = -Ic-Ia;
			wAux = ((ADC2->JDR1)>>3) - hPhaseCOffset - (*Ia) ;
			// Saturation of Ib
			if (wAux> S16_MAX){
				*Ib =S16_MAX;
			}
			else  if (wAux <S16_MIN){  
				*Ib = S16_MIN;
			}
			else{
				*Ib = wAux;
			}                     
			break;
			
	default:
			break;
   } 
}

void vsm_Start(void)
{
}

void vsm_Stop(void)
{
}