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

union ad5663_cmd_s {
	struct cmd_s {
	unsigned dv : 16;
	unsigned addr : 3;
	unsigned cmd : 3;
	unsigned resv : 2;
	}cmd_s;
	unsigned value;
};

struct ad5663_priv_s {
	struct ad5663_cfg_s *cfg;
	union ad5663_cmd_s *buffer;
};

static int fd;

#define cs_set(level) cfg->spi->csel(cfg->gpio_cs, level)
#define ldac_set(level) cfg->spi->csel(cfg->gpio_ldac, level)
#define clr_set(level) cfg->spi->csel(cfg->gpio_clr, level)
#define spi_write(val) cfg->spi->wreg(0, val)

#define ad5663_write_buf(buf) __ad5663_write_buf(cfg, buf,3)
static int __ad5663_write_buf(const struct ad5663_cfg_s *cfg, const unsigned int buf, int count)
{
	int temp;
	cs_set(0);
	while(count > 0) {
		temp = buf>>(8*(count-1));
		spi_write(temp);
		count--;
	}
	cs_set(1);
	return 0;
}

static int ad5663_write(int fd, const void *buf, int count)
{
	unsigned int ret;
	unsigned int dv;
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	assert(priv != NULL);
	struct ad5663_cfg_s *cfg = priv->cfg;
	assert(cfg != NULL);
	union ad5663_cmd_s *buffer;
	buffer = priv->buffer;
	assert(buffer != NULL);
	buffer->cmd_s.cmd = 3; //write cmd of "Write to and update DAC channel n"
	dv = atoi(buf);
	buffer->cmd_s.dv = dv;
	ret = ad5663_write_buf(buffer->value);
	return ret;
}


static int ad5663_hw_init(struct ad5663_priv_s *priv)
{
	struct ad5663_cfg_s *cfg = priv->cfg;
	assert(cfg != NULL);
	//spi bus init
	spi_cfg_t spi_cfg = SPI_CFG_DEF;
	spi_cfg.cpol = 1,
	spi_cfg.cpha = 0,
	spi_cfg.bits = 8,
	spi_cfg.bseq = 1,     //LSB 0-1...-6-7
	spi_cfg.freq = 8000000;
	spi_cfg.csel = 1;
	cfg->spi->init(&spi_cfg);
        cs_set(1);
	ldac_set(0);
        clr_set(1);
	return 0;
}


static int ad5663_init(int fd, const void *pcfg)
{
	struct ad5663_priv_s *priv;
	struct ad5663_cfg_s *cfg;
	union ad5663_cmd_s *buffer;
	const struct ad5663_cfg_s *temp = (struct ad5663_cfg_s *) pcfg;
	char *pmem;

	//malloc for device special data
	pmem = sys_malloc(sizeof(struct ad5663_priv_s) + sizeof(struct ad5663_cfg_s)+ sizeof(union ad5663_cmd_s));
	assert(pmem != NULL);
	priv = (struct ad5663_priv_s *)pmem;
	cfg = (struct ad5663_cfg_s *)(pmem + sizeof(struct ad5663_priv_s));
	buffer = (union ad5663_cmd_s *)(pmem + sizeof(struct ad5663_priv_s) + sizeof(struct ad5663_cfg_s));

	//init cfg
	cfg->spi = temp->spi;
	cfg->gpio_cs = temp->gpio_cs;
	cfg->gpio_ldac = temp->gpio_ldac;
	cfg->gpio_clr = temp->gpio_clr;
	cfg->ch = temp->ch;

	//priv
	priv->cfg = cfg;
	priv->buffer = buffer;
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
	unsigned int ch;		//digital v
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	struct ad5663_cfg_s *cfg;
	union ad5663_cmd_s *buffer;
	assert(priv != NULL);
	cfg = priv->cfg;
	assert(cfg != NULL);
	buffer = priv->buffer;
	assert(buffer != NULL);

	int ret = 0;
	switch(request) {
	case DAC_GET_BITS:
		return 16;
	case DAC_CHOOSE_CH:
		ch = (unsigned int) va_arg(args,int);
		buffer->cmd_s.addr = ch;
		cfg->ch = ch;
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
			fd = dev_open("dac", 0);
			dev_ioctl(fd,DAC_CHOOSE_CH,atoi(argv[1]));
			dev_write(fd,argv[2],0);
			return dev_close(fd);
		}
	}
	printf("%s", usage);
	return 0;
}

const static cmd_t cmd_ad5663 = {"ad5663", cmd_ad5663_func, "ad5663 debug command"};
DECLARE_SHELL_CMD(cmd_ad5663)
