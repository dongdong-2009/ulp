/*
 * 	miaofng@2013-7-6 initial version
 */

#ifndef __ADUC_MATRIX_H_
#define __ADUC_MATRIX_H_

#define RELAY_V_MODE() do {matrix_relay(1 << 15, 1 << 15);} while(0)
#define RELAY_R_MODE() do {matrix_relay(0 << 15, 1 << 15);} while(0)

/*
RESISTANCE H BAND	current source = aduc iexc, 20uA ~ 2mA
RESISTANCE L BAND	current source = fixed <> 29mA
*/
#define RELAY_H_BAND() do {matrix_relay(1 << 14, 1 << 14);} while(0)
#define RELAY_L_BAND() do {matrix_relay(0 << 14, 1 << 14);} while(0)

void matrix_init(void);
void matrix_relay(int image, int mask);
void matrix_pick(int pin0, int pin1);

#endif
