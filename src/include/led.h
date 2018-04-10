/* led.h
 * 	miaofng@2009 initial version
 */
#ifndef __LED_H_
#define __LED_H_

#ifdef CONFIG_LED_UPDATE_MS
#define LED_FLASH_PERIOD	CONFIG_LED_UPDATE_MS
#else
#define LED_FLASH_PERIOD	1000 /*unit ms*/
#endif

/*for led ecode use*/
#define LED_ERROR_PERIOD	200 /*unit: mS*/
#define LED_ERROR_IDLE		10

typedef enum {
	LED_R,
	LED_G,
	LED_Y,
	LED_W,

	/*misc purpose*/
	LED_E,
	LED_X,

	LED_A,
	LED_B,
	LED_C,
	LED_D,

	LED_1,
	LED_2,
	LED_3,
	LED_4,
	LED_5,
	LED_6,
	LED_7,
	LED_8,

	/*normally used as front panel indicator*/
	LED_EXT_R,
	LED_EXT_G,
	LED_EXT_Y,
	NR_OF_LED,

	//ALIAS
	LED_GREEN = LED_G,
	LED_RED = LED_R,
	LED_YELLOW = LED_Y,

	LED_ERR = LED_R,
	LED_SYS = LED_G
} led_t;

typedef enum {
	LED_OFF = 0,
	LED_ON
} led_status_t;

#define led_y led_on
#define led_n led_off
#define led_f led_flash
#define led_e led_error

void led_Init(void);
void led_Update(void);
void led_Update_Immediate(int status, int mask);
void led_combine(int mask);
void led_on(led_t led);
void led_off(led_t led);
void led_inv(led_t led);
void led_flash(led_t led);
void led_error(int ecode);

/*hw led driver routines*/
void led_hwInit(void);
void led_hwSetStatus(led_t led, led_status_t status);
#endif /*__LED_H_*/
