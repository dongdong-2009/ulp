/* vsm.c
 * 	dusk@2009 initial version
 */

#include "stm32f10x.h"
#include "vsm.h"
#include "pwm.h"
#include "adc.h"

/* Private variables ---------------------------------------------------------*/
uint8_t  bSector;
uint16_t hPhaseAOffset;
uint16_t hPhaseBOffset;
uint16_t hPhaseCOffset;

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
#if 0
	s32 wX, wY, wZ, wUAlpha, wUBeta;
	u16  hTimePhA, hTimePhB, hTimePhC, hTimePhD;
	u16  hDeltaDuty;
    wUAlpha = Stat_Volt_Input.qV_Component1 * T_SQRT3 ;
	wUBeta = -(Stat_Volt_Input.qV_Component2 * T);

   wX = wUBeta;
   wY = (wUBeta + wUAlpha)/2;
   wZ = (wUBeta - wUAlpha)/2;
   
  // Sector calculation from wX, wY, wZ
   if (wY<0)
   {
      if (wZ<0)
      {
        bSector = SECTOR_5;
      }
      else // wZ >= 0
        if (wX<=0)
        {
          bSector = SECTOR_4;
        }
        else // wX > 0
        {
          bSector = SECTOR_3;
        }
   }
   else // wY > 0
   {
     if (wZ>=0)
     {
       bSector = SECTOR_2;
     }
     else // wZ < 0
       if (wX<=0)
       {  
         bSector = SECTOR_6;
       }
       else // wX > 0
       {
         bSector = SECTOR_1;
       }
    }
   
   /* Duty cycles computation */
  PWM4Direction=PWM2_MODE;
    
  switch(bSector)
  {  
    case SECTOR_1:
                hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhB - wX/131072;
                
                // ADC Syncronization setting value             
                if ((u16)(PWM_PERIOD-hTimePhA) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhA - hTimePhB);
                  
				  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhA)*2) 
                  {
                      hTimePhD = hTimePhA - TW_BEFORE; // Ts before Phase A 
                  }
                  else
                  {
                      hTimePhD = hTimePhA + TW_AFTER; // DT + Tn after Phase A
                     
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                // ADC_InjectedChannelConfig(ADC1, PHASE_B_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);
                ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;
                //ADC_InjectedChannelConfig(ADC2, PHASE_C_CHANNEL,1,
                //                                     ADC_SampleTime_7Cycles5);     
                ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;                                         
                break;
    case SECTOR_2:
                hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhA - wY/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhB) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhB - hTimePhA);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhB)*2) 
                  {
                    hTimePhD = hTimePhB - TW_BEFORE; // Ts before Phase B 
                  }
                  else
                  {
                    hTimePhD = hTimePhB + TW_AFTER; // DT + Tn after Phase B
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                     ADC_SampleTime_7Cycles5);
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, 
                //                   PHASE_C_CHANNEL,1,ADC_SampleTime_7Cycles5);               
				ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;

    case SECTOR_3:
                hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
				hTimePhC = hTimePhA - wY/131072;
				hTimePhB = hTimePhC + wX/131072;
		
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhB) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhB - hTimePhC);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhB)*2) 
                  {
                    hTimePhD = hTimePhB - TW_BEFORE; // Ts before Phase B 
                  }
                  else
                  {
                    hTimePhD = hTimePhB + TW_AFTER; // DT + Tn after Phase B
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_C_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);               
				ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;
    
    case SECTOR_4:
                hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
                hTimePhB = hTimePhA + wZ/131072;
                hTimePhC = hTimePhB - wX/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhC) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhC - hTimePhB);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhC)*2)
                  {
                    hTimePhD = hTimePhC - TW_BEFORE; // Ts before Phase C 
                  }
                  else
                  {
                    hTimePhD = hTimePhC + TW_AFTER; // DT + Tn after Phase C
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_B_CHANNEL,1,
                //                                     ADC_SampleTime_7Cycles5);               
				ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;  
    
    case SECTOR_5:
                hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhA - wY/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhC) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhC - hTimePhA);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhC)*2) 
                  {
                    hTimePhD = hTimePhC - TW_BEFORE; // Ts before Phase C 
                  }
                  else
                  {
                    hTimePhD = hTimePhC + TW_AFTER; // DT + Tn after Phase C
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_B_CHANNEL,1,
                //                                     ADC_SampleTime_7Cycles5);               
				ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
                
    case SECTOR_6:
                hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
				hTimePhC = hTimePhA - wY/131072;
				hTimePhB = hTimePhC + wX/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhA) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhA - hTimePhC);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhA)*2) 
                  {
                    hTimePhD = hTimePhA - TW_BEFORE; // Ts before Phase A 
                  }
                  else
                  {
                    hTimePhD = hTimePhA + TW_AFTER; // DT + Tn after Phase A
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_B_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);
                ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_C_CHANNEL,1,
                //                                    ADC_SampleTime_7Cycles5);               
				ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;
    default:
		break;
   }
  
  if (PWM4Direction == PWM2_MODE)
  {
    //Set Polarity of CC4 High
    TIM1->CCER &= 0xDFFF;    
  }
  else
  {
    //Set Polarity of CC4 Low
    TIM1->CCER |= 0x2000;
  }
  
  /* Load compare registers values */ 
  TIM1->CCR1 = hTimePhA;
  TIM1->CCR2 = hTimePhB;
  TIM1->CCR3 = hTimePhC;
  TIM1->CCR4 = hTimePhD; // To Syncronyze the ADC
#endif
}

void vsm_GetCurrent(short *Ialpha, short *Ibeta)
{
#if 0
	s32 wAux;

	switch (bSector){
	case 4:
	case 5: //Current on Phase C not accessible     
           // Ia = (hPhaseAOffset)-(ADC Channel 11 value)  
		wAux = (s32)(hPhaseAOffset)- ((ADC1->JDR1)<<1);          
        //Saturation of Ia 
        if (wAux < S16_MIN)
            {
              Local_Stator_Currents.qI_Component1= S16_MIN;
            }  
            else  if (wAux > S16_MAX)
                  { 
                    Local_Stator_Currents.qI_Component1= S16_MAX;
                  }
                  else
                  {
                    Local_Stator_Currents.qI_Component1= wAux;
                  }
                     
           // Ib = (hPhaseBOffset)-(ADC Channel 12 value)
            wAux = (s32)(hPhaseBOffset)-((ADC2->JDR1)<<1);
           // Saturation of Ib
            if (wAux < S16_MIN)
            {
              Local_Stator_Currents.qI_Component2= S16_MIN;
            }  
            else  if (wAux > S16_MAX)
                  { 
                    Local_Stator_Currents.qI_Component2= S16_MAX;
                  }
                  else
                  { 
                    Local_Stator_Currents.qI_Component2= wAux;
                  }
           break;
           
   case 6:
   case 1:  //Current on Phase A not accessible     
            // Ib = (hPhaseBOffset)-(ADC Channel 12 value)
            wAux = (s32)(hPhaseBOffset)-((ADC1->JDR1)<<1);
            //Saturation of Ib 
            if (wAux < S16_MIN)
            {
              Local_Stator_Currents.qI_Component2= S16_MIN;
            }  
            else  if (wAux > S16_MAX)
                  { 
                    Local_Stator_Currents.qI_Component2= S16_MAX;
                  }
                  else
                  {
                    Local_Stator_Currents.qI_Component2= wAux;
                  }
            // Ia = -Ic -Ib 
            wAux = ((ADC2->JDR1)<<1)-hPhaseCOffset-
                                            Local_Stator_Currents.qI_Component2;
            //Saturation of Ia
            if (wAux> S16_MAX)
            {
               Local_Stator_Currents.qI_Component1 = S16_MAX;
            }
            else  if (wAux <S16_MIN)
                  {
                   Local_Stator_Currents.qI_Component1 = S16_MIN;
                  }
                  else
                  {  
                    Local_Stator_Currents.qI_Component1 = wAux;
                  }
           break;
           
   case 2:
   case 3:  // Current on Phase B not accessible
            // Ia = (hPhaseAOffset)-(ADC Channel 11 value)     
            wAux = (s32)(hPhaseAOffset)-((ADC1->JDR1)<<1);
            //Saturation of Ia 
            if (wAux < S16_MIN)
            {
              Local_Stator_Currents.qI_Component1= S16_MIN;
            }  
            else  if (wAux > S16_MAX)
                  { 
                    Local_Stator_Currents.qI_Component1= S16_MAX;
                  }
                  else
                  {
                    Local_Stator_Currents.qI_Component1= wAux;
                  }
     
            // Ib = -Ic-Ia;
            wAux = ((ADC2->JDR1)<<1) - hPhaseCOffset - 
                                            Local_Stator_Currents.qI_Component1;
            // Saturation of Ib
            if (wAux> S16_MAX)
            {
              Local_Stator_Currents.qI_Component2=S16_MAX;
            }
            else  if (wAux <S16_MIN)
                  {  
                    Local_Stator_Currents.qI_Component2 = S16_MIN;
                  }
                  else  
                  {
                    Local_Stator_Currents.qI_Component2 = wAux;
                  }                     
           break;

   default:
           break;
   } 
#endif
}

void vsm_Start(void)
{
}

void vsm_Stop(void)
{
}