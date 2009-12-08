/* led.h
 * 	miaofng@2009 initial version
 */
#ifndef __LED_H_
#define __LED_H_

void led_init(void);
void led_update(void);
void led_flash(void);
#define led_on() do { GPIO_SetBits(GPIOB, GPIO_Pin_5); } while(0)
#define led_off() do { GPIO_SetBits(GPIOB, GPIO_Pin_5); } while(0) 
#endif /*__LED_H_*/
