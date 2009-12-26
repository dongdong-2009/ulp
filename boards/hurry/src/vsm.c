/* vsm.c
 * 	dusk@2009 initial version
 */

#include "stm32f10x.h"
#include "vsm.h"
#include "pwm.h"
#include "adc.h"

/* Private variables ---------------------------------------------------------*/
unsigned char  bSector;
unsigned short hPhaseAOffset;
unsigned short hPhaseBOffset;
unsigned short hPhaseCOffset;

unsigned char PWM4Direction=PWM2_MODE;

void vsm_Init(void)
{
	adc_Init();
	adc_GetCalibration(&hPhaseAOffset,&hPhaseBOffset,&hPhaseCOffset);
	pwm_Init();
}

void vsm_Update(void)
{
}

void vsm_SetVoltage(int Valpha,int Vbeta)
{
	int wX, wY, wZ, wUAlpha, wUBeta;
	unsigned short  hTimePhA, hTimePhB, hTimePhC;
	unsigned short  hDeltaDuty;
	wUAlpha = Valpha * T_SQRT3 ;
	wUBeta = -Vbeta * T;

	wX = wUBeta;
	wY = (wUBeta + wUAlpha)/2;
	wZ = (wUBeta - wUAlpha)/2;
   
	// Sector calculation from wX, wY, wZ
	if (wY<0){
		if (wZ<0){
			bSector = SECTOR_5;
		}
		else{
			if (wX<=0){
				bSector = SECTOR_4;
			}
			else{
			bSector = SECTOR_3;
			}
		}
	}
	else{
		if (wZ>=0){
			bSector = SECTOR_2;
		}
		else{
			if (wX<=0){  
				bSector = SECTOR_6;
			}
			else{
				bSector = SECTOR_1;
			}
		}
	}

	/* Duty cycles computation */
	switch(bSector){  
	case SECTOR_1:
			hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
			hTimePhB = hTimePhA + wZ/131072;
			hTimePhC = hTimePhB - wX/131072;
                
			// ADC Syncronization setting value             
			if ((u16)(PWM_PERIOD-hTimePhA) > TW_AFTER){
				hTimePhD = PWM_PERIOD - 1;
			}
			else{
				hDeltaDuty = (u16)(hTimePhA - hTimePhB);
                  
				// Definition of crossing point
				if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhA)*2){
					hTimePhD = hTimePhA - TW_BEFORE; // Ts before Phase A 
				}
				else{
					hTimePhD = hTimePhA + TW_AFTER; // DT + Tn after Phase A
                     
					if (hTimePhD >= PWM_PERIOD){
					// Trigger of ADC at Falling Edge PWM4
					// OCR update

					//Set Polarity of CC4 Low
					PWM4Direction=PWM1_MODE;

					hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
				}
			}
		}
                
			ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);     
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;                                         
			break;
	case SECTOR_2:
			hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
			hTimePhB = hTimePhA + wZ/131072;
			hTimePhC = hTimePhA - wY/131072;
                
			// ADC Syncronization setting value
			if ((u16)(PWM_PERIOD-hTimePhB) > TW_AFTER){
				hTimePhD = PWM_PERIOD - 1;
			}
			else{
				hDeltaDuty = (u16)(hTimePhB - hTimePhA);
                  
				// Definition of crossing point
				if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhB)*2){
					hTimePhD = hTimePhB - TW_BEFORE; // Ts before Phase B 
				}
				else{
					hTimePhD = hTimePhB + TW_AFTER; // DT + Tn after Phase B
                    
					if (hTimePhD >= PWM_PERIOD){
					// Trigger of ADC at Falling Edge PWM4
					// OCR update
                      
					//Set Polarity of CC4 Low
					PWM4Direction=PWM1_MODE;
                      
					hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
					}
				}
			}
                
			ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;

	case SECTOR_3:
			hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
			hTimePhC = hTimePhA - wY/131072;
			hTimePhB = hTimePhC + wX/131072;
		
			// ADC Syncronization setting value
			if ((u16)(PWM_PERIOD-hTimePhB) > TW_AFTER){
				hTimePhD = PWM_PERIOD - 1;
			}
			else{
				hDeltaDuty = (u16)(hTimePhB - hTimePhC);
                  
				// Definition of crossing point
				if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhB)*2) {
					hTimePhD = hTimePhB - TW_BEFORE; // Ts before Phase B 
				}
				else{
					hTimePhD = hTimePhB + TW_AFTER; // DT + Tn after Phase B
                    
					if (hTimePhD >= PWM_PERIOD){
					// Trigger of ADC at Falling Edge PWM4
					// OCR update
                      
					//Set Polarity of CC4 Low
					PWM4Direction=PWM1_MODE;
                      
					hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
					}
				}
			}
                
			ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
			//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
			ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
			//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
			break;
    
    case SECTOR_4:
			hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
			hTimePhB = hTimePhA + wZ/131072;
			hTimePhC = hTimePhB - wX/131072;
                
			// ADC Syncronization setting value
			if ((u16)(PWM_PERIOD-hTimePhC) > TW_AFTER){
				hTimePhD = PWM_PERIOD - 1;
			}
			else{
				hDeltaDuty = (u16)(hTimePhC - hTimePhB);
                  
				// Definition of crossing point
				if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhC)*2){
					hTimePhD = hTimePhC - TW_BEFORE; // Ts before Phase C 
				}
				else{
					hTimePhD = hTimePhC + TW_AFTER; // DT + Tn after Phase C
                    
					if (hTimePhD >= PWM_PERIOD){
					// Trigger of ADC at Falling Edge PWM4
					// OCR update
                      
					//Set Polarity of CC4 Low
					PWM4Direction=PWM1_MODE;
                      
					hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
				}
			}
		}
                
		ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;  
    
	case SECTOR_5:
			hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
			hTimePhB = hTimePhA + wZ/131072;
			hTimePhC = hTimePhA - wY/131072;
                
			// ADC Syncronization setting value
			if ((u16)(PWM_PERIOD-hTimePhC) > TW_AFTER){
				hTimePhD = PWM_PERIOD - 1;
			}
			else{
				hDeltaDuty = (u16)(hTimePhC - hTimePhA);
                  
				// Definition of crossing point
				if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhC)*2){
					hTimePhD = hTimePhC - TW_BEFORE; // Ts before Phase C 
				}
				else{
					hTimePhD = hTimePhC + TW_AFTER; // DT + Tn after Phase C
                    
					if (hTimePhD >= PWM_PERIOD){
					// Trigger of ADC at Falling Edge PWM4
					// OCR update
                      
					//Set Polarity of CC4 Low
					PWM4Direction=PWM1_MODE;
                      
					hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
				}
			}
		}
                
		ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL,1,ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
                
	case SECTOR_6:
			hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
			hTimePhC = hTimePhA - wY/131072;
			hTimePhB = hTimePhC + wX/131072;
                
			// ADC Syncronization setting value
			if ((u16)(PWM_PERIOD-hTimePhA) > TW_AFTER){
				hTimePhD = PWM_PERIOD - 1;
			}
			else{
				hDeltaDuty = (u16)(hTimePhA - hTimePhC);
                  
				// Definition of crossing point
				if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhA)*2){
					hTimePhD = hTimePhA - TW_BEFORE; // Ts before Phase A 
				}
				else{
					hTimePhD = hTimePhA + TW_AFTER; // DT + Tn after Phase A
                    
					if (hTimePhD >= PWM_PERIOD){
					// Trigger of ADC at Falling Edge PWM4
					// OCR update
                      
					//Set Polarity of CC4 Low
					PWM4Direction=PWM1_MODE;
                      
					hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
				}
			}
		}
                
		ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);
		//ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
		ADC_InjectedChannelConfig(ADC2, PHASE_C_ADC_CHANNEL,1, ADC_SampleTime_7Cycles5);               
		//ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
	default:
		break;
	}
  
	if (PWM4Direction == PWM2_MODE){
		//Set Polarity of CC4 High
		TIM1->CCER &= 0xDFFF;    
	}
	else{
		//Set Polarity of CC4 Low
		TIM1->CCER |= 0x2000;
	}
  
	/* Load compare registers values */ 
	TIM1->CCR1 = hTimePhA;
	TIM1->CCR2 = hTimePhB;
	TIM1->CCR3 = hTimePhC;
	TIM1->CCR4 = hTimePhD; // To Syncronyze the ADC
}

/*
*here,*Ialpha and *Ibeta seems to be negative,and the ADC values are multiplied 16 times
*i am confused,so just accept it now.
*/
void vsm_GetCurrent(short *Ialpha, short *Ibeta)
{
	s32 wAux;

	switch (bSector){
	case 4:
	case 5: //Current on Phase C not accessible     
			// Ia = (hPhaseAOffset)-(ADC Channel 11 value)  
			wAux = (s32)(hPhaseAOffset)- ((ADC1->JDR1)<<1);          
			//Saturation of Ia 
			if (wAux < S16_MIN){
				*Ialpha = S16_MIN;
			}
            else  if (wAux > S16_MAX){ 
				*Ialpha = S16_MAX;
			}
			else{
				*Ialpha = wAux;
			}                     
			// Ib = (hPhaseBOffset)-(ADC Channel 12 value)
			wAux = (s32)(hPhaseBOffset)-((ADC2->JDR1)<<1);
			// Saturation of Ib
			if (wAux < S16_MIN){
				*Ibeta = S16_MIN;
			}
			else  if (wAux > S16_MAX){ 
				*Ibeta = S16_MAX;
			}
			else{ 
				*Ibeta = wAux;
			}
			break;
			
	case 6:
	case 1:  //Current on Phase A not accessible     
			// Ib = (hPhaseBOffset)-(ADC Channel 12 value)
			wAux = (s32)(hPhaseBOffset)-((ADC1->JDR1)<<1);
			//Saturation of Ib 
			if (wAux < S16_MIN){
				*Ibeta = S16_MIN;
			}  
			else  if (wAux > S16_MAX){ 
				*Ibeta = S16_MAX;
			}
			else{
				*Ibeta = wAux;
			}
			// Ia = -Ic -Ib 
			wAux = ((ADC2->JDR1)<<1)-hPhaseCOffset- (*Ibeta );
			//Saturation of Ia
			if (wAux> S16_MAX){
				*Ialpha = S16_MAX;
			}
			else  if (wAux <S16_MIN){
				*Ialpha = S16_MIN;
			}
			else{  
				*Ialpha = wAux;
			}
			break;
           
	case 2:
	case 3:  // Current on Phase B not accessible
			// Ia = (hPhaseAOffset)-(ADC Channel 11 value)     
			wAux = (s32)(hPhaseAOffset)-((ADC1->JDR1)<<1);
			//Saturation of Ia 
			if (wAux < S16_MIN){
				*Ialpha = S16_MIN;
            }  
            else  if (wAux > S16_MAX){ 
				*Ialpha = S16_MAX;
			}
			else{
				*Ialpha = wAux;
			}
			
			// Ib = -Ic-Ia;
			wAux = ((ADC2->JDR1)<<1) - hPhaseCOffset - (*Ialpha) ;
			// Saturation of Ib
			if (wAux> S16_MAX){
				*Ibeta =S16_MAX;
			}
			else  if (wAux <S16_MIN){  
				*Ibeta = S16_MIN;
			}
			else{
				*Ibeta = wAux;
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