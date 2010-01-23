/* vsm.c
 * 	dusk@2009 initial version
 */

#include "stm32f10x.h"
#include "vsm.h"
#include "pwm.h"
#include "adc.h"

/* Private variables */
static int sector;
unsigned short hPhaseAOffset;
unsigned short hPhaseBOffset;
unsigned short hPhaseCOffset;

#define PHASE_A_ADC_CHANNEL     ADC_Channel_4
#define PHASE_B_ADC_CHANNEL     ADC_Channel_5
#define PHASE_C_ADC_CHANNEL     ADC_Channel_14
#define VDC_CHANNEL				ADC_Channel_11

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
void vsm_SetVoltage(short alpha, short beta)
{
	int vp, x, y, z, tmp;
	unsigned short cmp_a, cmp_b, cmp_c;
	
	/*get normalized bus/phase voltage*/
	tmp = 17330; /*vb*/
	tmp = NOR_VOL(tmp) * divSQRT_3; /*vp = vb / (3^0.5)*/
	vp = tmp >> 15;
	
	/*basic voltage vector calcu*/
	tmp = alpha * sqrt3DIV_2;
	alpha = (short)(tmp1 >> 15); /*= ((3^0.5)/2)¦Á*/
	tmp = beta >> 1; /* = (1/2)¦Â*/
	
	x = beta;
	y = alpha - beta; /*((3^0.5)¦Á-¦Â)/2*/
	z = - alpha - beta; /*(-(3^0.5)¦Á-¦Â)/2*/
	
	/*sector calcu*/
	tmp =  (x > 0)?1:0;
	tmp += (y > 0)?2:0;
	tmp += (z > 0)?4:0;
	sector = tmp;
	
	/*basic time vector calcu*/
	x *= T;
	x /= vp; /*¦Â*/
	y *= T;
	y /= vp; /*((3^0.5)¦Á-¦Â)/2*/
	z *= T;
	z /= vp; /*(-(3^0.5)¦Á-¦Â)/2*/
	
	/*u-v-w duty cycles computation */
	switch(sector){
	case SECTOR_1:
		/*T1 = - y; (-(3^0.5)¦Á+¦Â)/2*/
		/*T2 = - z; (+(3^0.5)¦Á+¦Â)/2*/
		/*T0 = T - T1 - T2*/
		
		cmp_b = (unsigned short)((T + y + z) >> 1); /*= T0/2*/
		cmp_a = (unsigned short)(cmp_b - y); /*+ T1*/
		cmp_c = (unsigned short)(cmp_a - z); /*+ T2*/	
	
		ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;
		ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);     
		//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;                                         
		break;
	case SECTOR_2:	
		/*T1 = - z; (+(3^0.5)¦Á+¦Â)/2*/
		/*T2 = - x; -¦Â*/
		/*T0 = T - T1 - T2*/
			
		cmp_a = (unsigned short)((T + z + x) >> 1); /*T0/2*/
		cmp_c = (unsigned short)(cmp_a - z); /*+ T1*/	
		cmp_b = (unsigned short)(cmp_c - x); /*+ T2*/

		ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;

	case SECTOR_3:
		/*T1 = y; (+(3^0.5)¦Á-¦Â)/2*/
		/*T2 = x; ¦Â*/
		/*T0 = T - T1 - T2*/
			
		cmp_a = (unsigned short)((T - y - x) >> 1); /*T0/2*/
		cmp_b = (unsigned short)(cmp_a + y); /*+ T1*/
		cmp_c = (unsigned short)(cmp_b + x); /*+ T2*/

		ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
    
    case SECTOR_4:
		/*T1 = - x; -¦Â*/
		/*T2 = - y; (-(3^0.5)¦Á+¦Â)/2*/
		/*T0 = T - T1 - T2*/
			
		cmp_c = (unsigned short)((T + x + y) >> 1); /*T0/2*/
		cmp_b = (unsigned short)(cmp_c - x); /*+ T1*/
		cmp_a = (unsigned short)(cmp_b - y); /*+ T2*/

		ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;  

	case SECTOR_5:
		/*T1 = x; ¦Â*/
		/*T2 = z; (-(3^0.5)¦Á-¦Â)/2*/
		/*T0 = T - T1 - T2*/
			
		cmp_b = (unsigned short)((T - x -z) >> 1); /*T0/2*/
		cmp_c = (unsigned short)(cmp_b + x); /*+ T1*/
		cmp_a = (unsigned short)(cmp_c + z); /*+ T2*/

		ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
                
	case SECTOR_6:
		/*T1 = z; (-(3^0.5)¦Á-¦Â)/2*/
		/*T2 = y; (+(3^0.5)¦Á-¦Â)/2*/
		/*T0 = T - T1 - T2*/
			
		cmp_c = (unsigned short)((T - z - y) >> 1); /*T0/2*/
		cmp_a = (unsigned short)(cmp_c + z); /*+ T1*/
		cmp_b = (unsigned short)(cmp_a + y); /*+ T2*/			

		ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
	default:
		break;
	}
 
	/* Load compare registers values */ 
	TIM1->CCR1 = cmp_a;
	TIM1->CCR2 = cmp_b;
	TIM1->CCR3 = cmp_c;
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