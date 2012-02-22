/*
 * 	junjun@2011 initial version
 */
#include "config.h"
#include "spi.h"
#include "ulp/dac.h"
#include "ulp/debug.h"
#include "ulp/device.h"
#include "ulp/types.h"
#include "sys/malloc.h"
#include <stdlib.h>
#include <shell/cmd.h>
#include <string.h>

struct ad5663_priv_s {
	const struct ad5663_cfg_s *cfg;
	union {
		struct {
			unsigned dv : 16; //lsb
			unsigned addr : 3;
			unsigned cmd : 3;
		};
		unsigned value;
	} dacmd;
};

#define cs_set(level) cfg->spi->csel(cfg->gpio_cs, level)
#define ldac_set(level) cfg->spi->csel(cfg->gpio_ldac, level)
#define clr_set(level) cfg->spi->csel(cfg->gpio_clr, level)
#define spi_write(val) cfg->spi->wreg(cfg->gpio_cs, val)

static int ad5663_write(int fd, const void *buf, int count)
{
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	const struct ad5663_cfg_s *cfg = priv->cfg;

	priv->dacmd.dv = *(unsigned *)buf;
	priv->dacmd.cmd = 3; //write cmd of "Write to and update DAC channel n"
	spi_write(priv->dacmd.value);
	return 0;
}


static int ad5663_hw_init(struct ad5663_priv_s *priv)
{
	const struct ad5663_cfg_s *cfg = priv->cfg;
	assert(cfg != NULL);
	//spi bus init
	spi_cfg_t spi_cfg = SPI_CFG_DEF;
	spi_cfg.cpol = 1,
	spi_cfg.cpha = 0,
	spi_cfg.bits = 24,
	spi_cfg.bseq = 1, //msb
	spi_cfg.freq = 50000000;
	spi_cfg.csel = 0; //automatic control
	cfg->spi->init(&spi_cfg);
	cs_set(1);
	ldac_set(0);
        clr_set(1);
	return 0;
}


static int ad5663_init(int fd, const void *pcfg)
{
	struct ad5663_priv_s *priv;

	//malloc for device special data
	priv = sys_malloc(sizeof(struct ad5663_priv_s));
	assert(priv != NULL);

	//init cfg
	priv->cfg = pcfg;
	priv->dacmd.value = 0;
	dev_priv_set(fd, priv);
	return dev_class_register("dac",fd);
}

static int ad5663_open(int fd, int mode)
{
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	int ret;
	assert(priv != NULL);
	ret = ad5663_hw_init(priv);
	return ret;
}

static int ad5663_ioctl(int fd, int request, va_list args)
{
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	assert(priv != NULL);

	int ch, *pbits, ret = 0;
	switch(request) {
	case DAC_GET_BITS:
		pbits = va_arg(args, int *);
		*pbits = 16;
		break;
	case DAC_SET_CH:
		ret = -1;
		ch = va_arg(args, int);
		if(ch >= 0 && ch <= 1) {
			priv->dacmd.addr = ch;
			ret = 0;
		}
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

static const struct drv_ops_s ad5663_ops = {
	.init = ad5663_init,
	.open = ad5663_open,
	.ioctl = ad5663_ioctl,
	.poll = NULL,
	.read = NULL,
	.write = ad5663_write,
	.close = NULL,
};

struct driver_s ad5663_driver = {
	.name = "ad5663",
	.ops = &ad5663_ops,
};

static void __driver_init(void)
{
	drv_register(&ad5663_driver);
}
driver_init(__driver_init);


//ad5663 shell command
static int cmd_ad5663_func(int argc, char *argv[])
{
	const char *usage = {
		"ad5663 help         usage of ybs commond\n"
		"ad5663 ch           cmd of dac IC ad5663,usag:ad5663 0 xxx\n"
	};
	if(argc > 1) {
		if(argc == 3){
			int digit = atoi(argv[2]);
			int fd = dev_open("dac", 0);
			dev_ioctl(fd,DAC_SET_CH,atoi(argv[1]));
			dev_write(fd,&digit,0);
			dev_close(fd);
			return 0;
		}
	}
	printf("%s", usage);
	return 0;
}

static const cmd_t cmd_ad5663 = {"ad5663", cmd_ad5663_func, "ad5663 debug command"};
DECLARE_SHELL_CMD(cmd_ad5663)
