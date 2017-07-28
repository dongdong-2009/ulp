/*
 *	dusk@2010 initial version
 */

#ifndef __TM770X_H__
#define __TM770X_H__

#include "spi.h"

enum {
	TM_AIN1 = 0,
	TM_AIN2,
};

#define TM_GAIN(g) (g)

enum {
	TM_BIPOLAR = 0,
	TM_UNIPOLAR,
};

enum {
	TM_BUF_N,
	TM_BUF_Y,
};

typedef struct {
	const spi_bus_t *bus;
	int pin_nss;
	int pin_nrdy;
	int pin_nrst;
	int mclk_hz;
} tm770x_t;

void tm770x_init(const tm770x_t *chip, int notch_hz);
void tm770x_config(const tm770x_t *chip, int gain, int buf, int bipolar);
void tm770x_cal_self(const tm770x_t *chip, int ch);
int tm770x_poll(const tm770x_t *chip);
int tm770x_read(const tm770x_t *chip, int AIN);

#endif /* __TM770X_H__ */
