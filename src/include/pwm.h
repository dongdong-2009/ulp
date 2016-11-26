/*
*	miaofng@2010 initial version
*/
#ifndef __PWM_H_
#define __PWM_H_

#include <stddef.h>
#include "config.h"

#define PWM_HZ_DEF	10000
#define PWM_FS_DEF	100

typedef struct {
	float hz; //desired output pwm frequency, Fclk/fs at most
	int fs; //pwm full scale value, range: 0x0000~0xFFFF
} pwm_cfg_t;

#define PWM_CFG_DEF { \
	.hz = PWM_HZ_DEF, \
	.fs = PWM_FS_DEF, \
}

typedef struct {
	int (*init)(const pwm_cfg_t *cfg);
	int (*set)(int val);
} pwm_bus_t;

#ifdef CONFIG_DRIVER_PWM0
extern const pwm_bus_t pwm0;
extern const pwm_bus_t pwm01;
extern const pwm_bus_t pwm02;
extern const pwm_bus_t pwm03;
extern const pwm_bus_t pwm04;
#endif

#ifdef CONFIG_DRIVER_PWM1
extern const pwm_bus_t pwm1;
extern const pwm_bus_t pwm11;
extern const pwm_bus_t pwm12;
extern const pwm_bus_t pwm13;
extern const pwm_bus_t pwm14;
#endif

#ifdef CONFIG_DRIVER_PWM2
extern const pwm_bus_t pwm2;
extern const pwm_bus_t pwm21;
extern const pwm_bus_t pwm22;
extern const pwm_bus_t pwm23;
extern const pwm_bus_t pwm24;
#endif

#ifdef CONFIG_DRIVER_PWM3
extern const pwm_bus_t pwm3;
extern const pwm_bus_t pwm31;
extern const pwm_bus_t pwm32;
extern const pwm_bus_t pwm33;
extern const pwm_bus_t pwm34;
#endif

#ifdef CONFIG_DRIVER_PWM4
extern const pwm_bus_t pwm4;
extern const pwm_bus_t pwm41;
extern const pwm_bus_t pwm42;
extern const pwm_bus_t pwm43;
extern const pwm_bus_t pwm44;
#endif

#endif /*__PWM_H_*/

