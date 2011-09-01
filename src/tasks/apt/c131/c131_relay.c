/*
 * David@2011 init version
 */
#include "c131_relay.h"
#include "spi.h"
#include "config.h"
#include "time.h"
#include "stm32f10x.h"
#include "mbi5025.h"
#include <stdio.h>
#include <string.h>


int loop_SetRelayStatus(int loop_status);
int loop_GetRelayStatus(int * ploop_status);
int ess_SetRelayStatus(int esss_tatus);
int ess_GetRelayStatus(int * pess_status);
int led_SetRelayStatus(int led_status);
int led_GetRelayStatus(int * pled_status);
int sw_SetRelayStatus(int sw_status);
int sw_GetRelayStatus(int * psw_status);
