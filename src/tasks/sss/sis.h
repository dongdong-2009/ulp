#ifndef __SIS_H
#define __SIS_H
//#pragma once 

#include "stdio.h"
#include "string.h"
#include "stm32f10x.h"
#define  s_8kmps

extern uint16_t size;
extern char okmode;

typedef  char ad_flag;
enum  message_speed{s_1k,s_2k,s_3k,s_4k};

#ifdef	s_8kmps
#define  b0 198;
#define  b1 396;
#define  bit_time 11; //us
#endif

#ifdef   s_4kmps
#define  b0  396
#define  b1  792  
#define  bit_time 22;  //us
#endif

#ifdef   s_2kmps
#define  b0  253
#define  b1  506
#define  bit_time  14; //us
#endif

#ifdef   s_1kmps
#define  b0  517
#define  b1  1034
#define  bit_time  29; //us
#endif


#endif
