/*
 * David@2011 init version
 */
#ifndef __C131_RELAY_H_
#define __C131_RELAY_H_


//function declaration
int loop_SetRelayStatus(int loop_status);
int loop_GetRelayStatus(int * ploop_status);
int ess_SetRelayStatus(int esss_tatus);
int ess_GetRelayStatus(int * pess_status);
int led_SetRelayStatus(int led_status);
int led_GetRelayStatus(int * pled_status);
int sw_SetRelayStatus(int sw_status);
int sw_GetRelayStatus(int * psw_status);

#endif /*__C131_RELAY_H_*/
