/*
*	miaofng@2010 initial version
		this routine is used to provide the device driver for vvt knock control
*/

#include "stm32f10x.h"
#include "knock.h"
#include "ad9833.h"
#include "mcp41x.h"
#include "spi.h"

static spi_t spi1 = {
	.addr = SPI1,
	.mode = SPI_MODE_POL_1| SPI_MODE_PHA_0| SPI_MODE_BW_16 | SPI_MODE_MSB,
};

static spi_t spi2 = {
	.addr = SPI2,
	.mode = SPI_MODE_POL_0| SPI_MODE_PHA_0| SPI_MODE_BW_16,
};

#define DECLARE_IO_FUNC(chip) \
	int chip##_write_reg(int reg, int val) \
	{ \
		int ret; \
		GPIO_WriteBit(chip##_band, chip##_pin, Bit_RESET); \
		ret = spi_Write(chip##_bus, reg, val); \
		GPIO_WriteBit(chip##_band, chip##_pin, Bit_SET); \
		return ret; \
	} \
	int chip##_read_reg(int reg) \
	{ \
		int ret; \
		GPIO_WriteBit(chip##_band, chip##_pin, Bit_RESET); \
		ret = spi_Read(chip##_bus, reg); \
		GPIO_WriteBit(chip##_band, chip##_pin, Bit_SET); \
		return ret; \
	}

/*pin map*/
#define knock_dds_bus	&spi1
#define knock_dds_band	GPIOC //CS_DDS_KNOCK
#define knock_dds_pin	GPIO_Pin_4
DECLARE_IO_FUNC(knock_dds)

#define knock_pot0_bus	&spi2
#define knock_pot0_band	GPIOE //CS_VR_KS0
#define knock_pot0_pin	GPIO_Pin_11
DECLARE_IO_FUNC(knock_pot0)

#define knock_pot1_bus	&spi2
#define knock_pot1_band	GPIOE //CS_VR_KS1
#define knock_pot1_pin	GPIO_Pin_12
DECLARE_IO_FUNC(knock_pot1)

#define knock_pot2_bus	&spi2
#define knock_pot2_band	GPIOE //CS_VR_KS2
#define knock_pot2_pin	GPIO_Pin_13
DECLARE_IO_FUNC(knock_pot2)

#define knock_pot3_bus	&spi2
#define knock_pot3_band	GPIOE //CS_VR_KS3
#define knock_pot3_pin	GPIO_Pin_14
DECLARE_IO_FUNC(knock_pot3)

static ad9833_t knock_dds = {
	.io = {
		.write_reg = knock_dds_write_reg,
		.read_reg = knock_dds_read_reg,
	},
	.option = AD9833_OPT_OUT_SIN,
};

static mcp41x_t knock_pot[NR_OF_KS] = {
	[0] = {
		.io = {
			.write_reg = knock_pot0_write_reg,
			.read_reg = knock_pot0_read_reg,
		},
	},

	[1] = {
		.io = {
			.write_reg = knock_pot1_write_reg,
			.read_reg = knock_pot1_read_reg,
		},
	},

	[2] = {
		.io = {
			.write_reg = knock_pot2_write_reg,
			.read_reg = knock_pot2_read_reg,
		},
	},

	[3] = {
		.io = {
			.write_reg = knock_pot3_write_reg,
			.read_reg = knock_pot3_read_reg,
		},
	},
};

void knock_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/*chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET); //cs_dds_knock = 1

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOE, GPIO_Pin_11, Bit_SET); //cs_vr_ks0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_12, Bit_SET); //cs_vr_ks1 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_13, Bit_SET); //cs_vr_ks2 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_14, Bit_SET); //cs_vr_ks3 = 1

	/*chip init*/
	spi_Init(&spi1);
	spi_Init(&spi2);
	ad9833_Init(&knock_dds);
	mcp41x_Init(&knock_pot[KS1]);
	mcp41x_Init(&knock_pot[KS2]);
	mcp41x_Init(&knock_pot[KS3]);
	mcp41x_Init(&knock_pot[KS4]);
}

void knock_Update(void)
{
}

void knock_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_KNOCK_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&knock_dds, fw);
}

void knock_SetVolt(knock_ch_t ch, short mv)
{
	short pos;
	pos = (mv * 255) / CONFIG_DRIVER_KNOCK_MVPP_MAX;
	pos = (pos > 255) ? 255 : pos;
	mcp41x_SetPos(&knock_pot[ch], pos);
}
