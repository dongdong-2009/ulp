/*
 * miaofng@2017-06-10 initial version
 */

#ifndef __GPIO_H_
#define __GPIO_H_

/*
gpio is named as: PA0 port A, pin 0 note:
PA1 == PA01 == PA001 == PA0001
    == PA.1 == PA.01, not supported yet
*/

#define GPIO_BIND(mode, gpio, name) gpio_bind(mode, #gpio, #name);
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

int gpio_bind(int mode, const char *gpio, const char *name);
int gpio_set(const char *name, int high);
int gpio_get(const char *name);

/*usage demo:

void main(void) {
	GPIO_BIND(GPIO_PP0, PE10, UPx_VCC_EN)
	GPIO_BIND(GPIO_DIN, PE08, TRIG_IN)

	GPIO_SET(UPx_VCC_EN, 0)
	GPIO_GET(TRIG_IN)
	...
}

*/

//auto called by sys_init
void gpio_init(void);
#endif /* __GPIO_H_ */
