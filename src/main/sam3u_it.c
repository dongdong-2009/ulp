/* miaofng@2010
*/

#include "config.h"
#include "sys/task.h"
#include "sam3u.h"

#pragma language=extended
#pragma segment="CSTACK"

static void NmiSR(void);
static void FaultISR(void);
static void IntDefaultHandler(void);

//iar reserved function
extern void __iar_program_start(void);

//*****************************************************************************
//
// A union that describes the entries of the vector table.  The union is needed
// since the first entry is the stack pointer and the remainder are function
// pointers.
//
//*****************************************************************************
typedef union {
	void (*pfnHandler)(void);
	void *__ptr;
} uVectorEntry;

//*****************************************************************************
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
//
//*****************************************************************************
__root const uVectorEntry __vector_table[] @ ".intvec" =
{
	{ .__ptr = __sfe( "CSTACK" ) }, // The initial stack pointer
	__iar_program_start,
	NmiSR,
	FaultISR,
	
	IntDefaultHandler, //MemManage_Handler,
	IntDefaultHandler, //BusFault_Handler,
	IntDefaultHandler, //UsageFault_Handler,
	0, 0, 0, 0, // Reserved
	IntDefaultHandler, //SVC_Handler,
	IntDefaultHandler, //DebugMon_Handler,
	0, // Reserved
	IntDefaultHandler, //PendSV_Handler,
	task_Isr, //SysTick_Handler,

	// Configurable interrupts
	IntDefaultHandler, //SUPC_IrqHandler, // 0  SUPPLY CONTROLLER
	IntDefaultHandler, //RSTC_IrqHandler, // 1  RESET CONTROLLER
	IntDefaultHandler, //RTC_IrqHandler,	 // 2  REAL TIME CLOCK
	IntDefaultHandler, //RTT_IrqHandler,	 // 3  REAL TIME TIMER
	IntDefaultHandler, //WDT_IrqHandler,  // 4  WATCHDOG TIMER
	IntDefaultHandler, //PMC_IrqHandler, // 5  PMC
	IntDefaultHandler, //EFC0_IrqHandler, // 6  EFC0
	IntDefaultHandler, //EFC1_IrqHandler, // 7  EFC1
	IntDefaultHandler, //DBGU_IrqHandler, // 8  DBGU
	IntDefaultHandler, //HSMC4_IrqHandler, // 9  HSMC4
	IntDefaultHandler, //PIOA_IrqHandler, // 10 Parallel IO Controller A
	IntDefaultHandler, //PIOB_IrqHandler, // 11 Parallel IO Controller B
	IntDefaultHandler, //PIOC_IrqHandler, // 12 Parallel IO Controller C
	IntDefaultHandler, //USART0_IrqHandler,  // 13 USART 0
	IntDefaultHandler, //USART1_IrqHandler,  // 14 USART 1
	IntDefaultHandler, //USART2_IrqHandler,  // 15 USART 2
	IntDefaultHandler, //USART3_IrqHandler,  // 16 USART 3
	IntDefaultHandler, //MCI0_IrqHandler, // 17 Multimedia Card Interface
	IntDefaultHandler, //TWI0_IrqHandler, // 18 TWI 0
	IntDefaultHandler, //TWI1_IrqHandler, // 19 TWI 1
	IntDefaultHandler, //SPI0_IrqHandler, // 20 Serial Peripheral Interface
	IntDefaultHandler, //SSC0_IrqHandler, // 21 Serial Synchronous Controller 0
	IntDefaultHandler, //TC0_IrqHandler, // 22 Timer Counter 0
	IntDefaultHandler, //TC1_IrqHandler, // 23 Timer Counter 1
	IntDefaultHandler, //TC2_IrqHandler, // 24 Timer Counter 2
	IntDefaultHandler, //PWM_IrqHandler, // 25 Pulse Width Modulation Controller
	IntDefaultHandler, //ADCC0_IrqHandler, // 26 ADC controller0
	IntDefaultHandler, //ADCC1_IrqHandler, // 27 ADC controller1
	IntDefaultHandler, //HDMA_IrqHandler, // 28 HDMA
	IntDefaultHandler, //UDPD_IrqHandler, // 29 USB Device High Speed UDP_HS
	IntDefaultHandler, //IrqHandlerNotUsed, // 30 not used
};

static void NmiSR(void)
{
	while(1) {
	}
}

static void FaultISR(void)
{
	while(1) {
	}
}

static void IntDefaultHandler(void)
{
	while(1) {
	}
}
