/*
*	miaofng@2010 initial version
*/
#ifndef __SPI_H_
#define __SPI_H_

#define SPI_MODE_POL_0 (0 << 0)
#define SPI_MODE_POL_1 (1 << 0)
#define SPI_MODE_PHA_0 (0 << 1)
#define SPI_MODE_PHA_1 (1 << 1)
#define SPI_MODE_BW_8 (0 << 2)
#define SPI_MODE_BW_16 (1 << 2)
#define SPI_MODE_MSB (0 << 3)
#define SPI_MODE_LSB (1 << 3)

typedef struct {
	void *addr; /*addr of first register*/
	int mode;
} spi_t;

void spi_Init(spi_t *bus);
int spi_Write(spi_t *bus, int reg, int val);
int spi_Read(spi_t *bus, int reg);
#endif /*__SPI_H_*/

