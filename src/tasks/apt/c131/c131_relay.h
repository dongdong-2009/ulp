/*
 * David@2011 init version
 */
#ifndef __C131_RELAY_H_
#define __C131_RELAY_H_
typedef enum {
	RELAY_OFF = 0,
	RELAY_ON
} relay_status;

//LOOP define
#define C131_LOOP1			((unsigned short)0x0001)  /*!< loop 1 selected */
#define C131_LOOP2			((unsigned short)0x0002)  /*!< loop 2 selected */
#define C131_LOOP3			((unsigned short)0x0004)  /*!< loop 3 selected */
#define C131_LOOP4			((unsigned short)0x0008)  /*!< loop 4 selected */
#define C131_LOOP5			((unsigned short)0x0010)  /*!< loop 5 selected */
#define C131_LOOP6			((unsigned short)0x0020)  /*!< loop 6 selected */
#define C131_LOOP7			((unsigned short)0x0040)  /*!< loop 7 selected */
#define C131_LOOP8			((unsigned short)0x0080)  /*!< loop 8 selected */
#define C131_LOOP9			((unsigned short)0x0100)  /*!< loop 9 selected */
#define C131_LOOP10			((unsigned short)0x0200)  /*!< loop 10 selected */
#define C131_LOOP11			((unsigned short)0x0400)  /*!< loop 11 selected */
#define C131_LOOP12			((unsigned short)0x0800)  /*!< loop 12 selected */
#define C131_LOOP			((unsigned short)0x1000)  /*!< loop  selected */

//ESS define
#define C131_ESS1			((unsigned short)0x0001)  /*!< ess 1 selected */
#define C131_ESS2			((unsigned short)0x0002)  /*!< ess 2 selected */
#define C131_ESS3			((unsigned short)0x0004)  /*!< ess 3 selected */
#define C131_ESS4			((unsigned short)0x0008)  /*!< ess 4 selected */
#define C131_ESS5			((unsigned short)0x0010)  /*!< ess 5 selected */
#define C131_ESS6			((unsigned short)0x0020)  /*!< ess 6 selected */

//LED define
#define C131_LED1_C1		((unsigned short)0x0001)  /*!< led 1 selected */
#define C131_LED1_C2		((unsigned short)0x0002)  /*!< led 1 selected */
#define C131_LED2			((unsigned short)0x0004)  /*!< led 2 selected */
#define C131_LED3			((unsigned short)0x0008)  /*!< led 3 selected */
#define C131_LED4_C1		((unsigned short)0x0010)  /*!< led 4 selected */
#define C131_LED4_C2		((unsigned short)0x0020)  /*!< led 4 selected */
#define C131_LED5			((unsigned short)0x0040)  /*!< led 5 selected */

//SW define
#define C131_SW1_C1			((unsigned short)0x0001)  /*!< SW 1 selected */
#define C131_SW1_C2			((unsigned short)0x0002)  /*!< SW 1 selected */
#define C131_SW2_C1			((unsigned short)0x0004)  /*!< SW 2 selected */
#define C131_SW2_C2			((unsigned short)0x0008)  /*!< SW 2 selected */
#define C131_SW3			((unsigned short)0x0010)  /*!< SW 3 selected */
#define C131_SW4			((unsigned short)0x0020)  /*!< SW 4 selected */
#define C131_SW5_C1			((unsigned short)0x0040)  /*!< SW 5 selected */
#define C131_SW5_C2			((unsigned short)0x0080)  /*!< SW 5 selected */

//function declaration
void c131_relay_Init(void);
int loop_SetRelayStatus(unsigned short loop_relays, int act);
int loop_GetRelayStatus(unsigned short * ploop_status);
int ess_SetRelayStatus(unsigned short ess_relays, int act);
int ess_GetRelayStatus(unsigned short * pess_status);
int led_SetRelayStatus(unsigned short led_relays, int act);
int led_GetRelayStatus(unsigned short * pled_status);
int sw_SetRelayStatus(unsigned short sw_relays, int act);
int sw_GetRelayStatus(unsigned short * psw_status);

#endif /*__C131_RELAY_H_*/
