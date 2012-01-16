/*
 * 	junjun@2011 initial version
 */
#include "config.h"
#include "spi.h"
#include "ad5663.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "ulp/device.h"
#include "ulp/types.h"
#include "common/circbuf.h"
#include "linux/list.h"
#include "sys/malloc.h"
#include <stdlib.h>


#define cs_set(level) chip->spi->csel(chip->gpio_cs, level)
#define ldac_set(level) chip->spi->csel(chip->gpio_ldac, level)
#define clr_set(level) chip->spi->csel(chip->gpio_clr, level)
#define spi_write(val) chip->spi->wreg(0, val)

#define ad5663_write_buf(buf) __ad5663_write_buf(chip, buf, 3)
static int __ad5663_write_buf(const struct ad5663_chip_s *chip, const char *buf, int count)
{
	//ldac_set(0);
	cs_set(0);
	while(count > 0) {
		printf("%x",(*buf)&0xff);
		spi_write(*buf++);
		count--;
	}
	cs_set(1);
	//ldac_set(1);
	return 0;
}

static int ad5663_write(int fd, const void *buf, int count)
{
	unsigned int ret;
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	assert(priv != NULL);
	struct ad5663_chip_s *chip = priv->chip;
	assert(chip != NULL);
	struct ad5663_buffer_s *buffer;
	buffer = priv->buffer;
	assert(buffer != NULL);
	ret = ad5663_write_buf((char *)buffer);
	return ret;
}


static int ad5663_hw_init(struct ad5663_priv_s *priv)
{
	struct ad5663_chip_s *chip = priv->chip;
	assert(chip != NULL);
	//spi bus init
	spi_cfg_t spi_cfg = SPI_CFG_DEF;
	spi_cfg.cpol = 0,
	spi_cfg.cpha = 1,
	spi_cfg.bits = 8,
	spi_cfg.bseq = 0,     //LSB 0-1...-6-7
	spi_cfg.freq = 8000000;
	spi_cfg.csel = 1;
	chip->spi->init(&spi_cfg);
	cs_set(0);
        cs_set(1);
	ldac_set(0);
        //ldac_set(1);
        clr_set(0);
        clr_set(1);
	return 0;
}


static int ad5663_init(int fd, const void *pcfg)
{
	struct ad5663_priv_s *priv;
	struct ad5663_chip_s *chip;
	struct ad5663_buffer_s *buffer;
	const struct ad5663_chip_s *cfg = pcfg;
	char *pmem;

	//malloc for device special data
	pmem = sys_malloc(sizeof(struct ad5663_priv_s) + sizeof(struct ad5663_chip_s)+ sizeof(struct ad5663_buffer_s));
	assert(pmem != NULL);
	priv = (struct ad5663_priv_s *)pmem;
	chip = (struct ad5663_chip_s *)(pmem + sizeof(struct ad5663_priv_s));
	buffer = (struct ad5663_buffer_s *)(pmem + sizeof(struct ad5663_priv_s) + sizeof(struct ad5663_chip_s));
	
	//init chip
	chip->spi = cfg->spi;
	chip->gpio_cs = cfg->gpio_cs;
	chip->gpio_ldac = cfg->gpio_ldac;
	chip->gpio_clr = cfg->gpio_clr;
	
	//priv
	priv->chip = chip;
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
	unsigned int dv;		//digital v
	struct ad5663_priv_s *priv = dev_priv_get(fd);
	struct ad5663_chip_s *chip;
	struct ad5663_buffer_s *buffer;
	assert(priv != NULL);
	chip = priv->chip;
	assert(chip != NULL);
	buffer = priv->buffer;
	assert(chip != NULL);
	//preprocess
	int ret = 0;
	switch(request) {
	case DAC_RESET:
		buffer->cmd = DAC_RESET;
		break;
	case DAC_WRIN_UPDN:
		buffer->cmd = DAC_WRIN_UPDN;
		dv = (unsigned int) va_arg(args,int);
		buffer->dv = dv;
		break;
	case DAC_CHOOSE_CHANNEL1:
		buffer->addr = 0;
		break;
	case DAC_CHOOSE_CHANNEL2:
		buffer->addr = 1;
		break;
	default:
		ret = -1;
		break;
	}
	//post process
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
