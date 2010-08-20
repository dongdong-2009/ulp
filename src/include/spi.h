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
#define SPI_MODE_PRESCALER_2 (0<<4)
#define SPI_MODE_PRESCALER_4 (1<<4)

/*bus,range: 1, 2, ...*/
int spi_Init(int bus, int mode);
int spi_Write(int bus, int val);
int spi_DMA_Write(int bus, unsigned char *pbuf, int len);
#endif /*__SPI_H_*/

