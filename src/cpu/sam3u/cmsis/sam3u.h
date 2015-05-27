 
#ifndef __SAM3U_H_
#define __SAM3U_H_

#include "config.h"

#if CONFIG_CPU_SAM3U4 == 1
#include "AT91SAM3U4.h"
#endif

typedef enum IRQn
{
	/*cortex-m3 vectors*/
	NonMaskableInt_IRQn = -14,	/*!< 2 Non Maskable Interrupt */
	MemoryManagement_IRQn = -12,	/*!< 4 Cortex-M3 Memory Management Int   */
	BusFault_IRQn = -11,		/*!< 5 Cortex-M3 Bus Fault Interrupt	 */
	UsageFault_IRQn = -10,		/*!< 6 Cortex-M3 Usage Fault Interrupt   */
	SVCall_IRQn = -5,		/*!< 11 Cortex-M3 SV Call Interrupt	  */
	DebugMonitor_IRQn = -4,		/*!< 12 Cortex-M3 Debug Monitor Interrupt*/
	PendSV_IRQn = -2,		/*!< 14 Cortex-M3 Pend SV Interrupt	  */
	SysTick_IRQn = -1,		/*!< 15 Cortex-M3 System Tick Interrupt  */
	
	/*cpu specific vectors*/
	SUPC_IRQn = 0,		//0  SUPPLY CONTROLLER
	RSTC_IRQn,		//1  RESET CONTROLLER
	RTC_IRQn,		//2  REAL TIME CLOCK
	RTT_IRQn,		//3  REAL TIME TIMER
	WDT_IRQn,		//4  WATCHDOG TIMER
	PMC_IRQn,		//5  PMC
	EFC0_IRQn,		//6  EFC0
	EFC1_IRQn,		//7  EFC1
	DBGU_IRQn,		//8  DBGU
	HSMC4_IRQn,		//9  HSMC4
	PIOA_IRQn,		//10 Parallel IO Controller A
	PIOB_IRQn,		//11 Parallel IO Controller B
	PIOC_IRQn,		//12 Parallel IO Controller C
	USART0_IRQn, 		//13 USART 0
	USART1_IRQn, 		//14 USART 1
	USART2_IRQn, 		//15 USART 2
	USART3_IRQn, 		//16 USART 3
	MCI0_IRQn,		//17 Multimedia Card Interface
	TWI0_IRQn,		//18 TWI 0
	TWI1_IRQn,		//19 TWI 1
	SPI0_IRQn,		//20 Serial Peripheral Interface
	SSC0_IRQn,		//21 Serial Synchronous Controller 0
	TC0_IRQn,		//22 Timer Counter 0
	TC1_IRQn,		//23 Timer Counter 1
	TC2_IRQn,		//24 Timer Counter 2
	PWM_IRQn,		//25 Pulse Width Modulation Controller
	ADCC0_IRQn,		//26 ADC controller0
	ADCC1_IRQn,		//27 ADC controller1
	HDMA_IRQn,		//28 HDMA
	UDPD_IRQn,		//29 USB Device High Speed UDP_HS
	IrqHandlerNotUsed,	//30 not used
} IRQn_Type;

#include "core_cm3.h"
#endif
