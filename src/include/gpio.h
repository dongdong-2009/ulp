/*
 * miaofng@2017-06-10 initial version
 */

#ifndef __GPIO_H_
#define __GPIO_H_

#include "common/debounce.h"

/*
gpio is named as: PA0 port A, pin 0 note:
PA1 == PA01 == PA001 == PA0001
    == PA.1 == PA.01, not supported yet
*/

#define GPIO_BIND(mode, gpio, name) gpio_bind(mode, #gpio, #name);
#define GPIO_FILT(name, ms) gpio_filt(#name, ms);
#define GPIO_SET(name, high) gpio_set(#name, high);
#define GPIO_GET(name) gpio_get(#name)

/*gpio mode list*/
enum {
	/*input*/
	GPIO_AIN, //adc input
	GPIO_DIN,
	GPIO_IPD, //pull down
	GPIO_IPU, //pull up

	/*output*/
	GPIO_PP0, //pp & init out 0
	GPIO_PP1, //pp & init out 1
	GPIO_OD0, //od & init out 0
	GPIO_OD1, //od & init out 1
};

//bind := "PA0" or "PB10"
int gpio_bind(int mode, const char *bind, const char *name);
int gpio_filt(const char *name, int ms); //filt the pulse widht <ms, 0 = disable

int gpio_set(const char *name, int high);
int gpio_get(const char *name);
int gpio_wimg(int img, int msk);
int gpio_rimg(int msk);

#define GPIO_INVALID -2 //exist or not exist
#define GPIO_NONE -1 //not exist
int gpio_handle(const char *name); //return hgpio or GPIO_NONE
int gpio_set_h(int gpio, int high);
int gpio_get_h(int gpio);

/*usage demo:

void main(void) {
	GPIO_BIND(GPIO_PP0, PE10, UPx_VCC_EN)
	GPIO_BIND(GPIO_DIN, PE08, TRIG_IN)

	GPIO_SET(UPx_VCC_EN, 0)
	GPIO_GET(TRIG_IN)
	...
}

*/

//priv
typedef struct {
	const void *drv;
	const char *name; //"LED_RED"
	const char *bind; //"PA0" or "PB10" or "mcp0:PA0"
	int mode;

#if CONFIG_GPIO_FILTER == 1
	struct debounce_s gfilt;
#endif
} gpio_t;

typedef struct gpio_drv_s {
	const char *name;
	const char *bind;
	int (*config)(const gpio_t *gpio);
	int (*set)(const gpio_t *gpio, int high);
	int (*get)(const gpio_t *gpio);
} gpio_drv_t;

extern const gpio_drv_t stm32;

//auto called by sys_init
void gpio_init(void);
#endif /* __GPIO_H_ */
