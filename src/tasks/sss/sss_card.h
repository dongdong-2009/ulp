#ifndef  _SSS_CARD_H
#define  _SSS_CARD_H

int ADDRESS_GPIO_Init(void);
int ADDRESS_GPIO_Read(void);
int sensor_poweron(void);
int simulator_poweroff(void);
int LED_RED_ON(void);
int LED_GREEN_ON(void);
int LED_MIX_ON(void);
#endif