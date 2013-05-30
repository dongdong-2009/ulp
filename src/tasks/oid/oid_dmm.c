/*
*	dmm program based on aduc processor
*	1, do adc convert continuously
*	2, serv for spi communication
*
*/
#include "config.h"
#include "ulp/sys.h"

int main(void)
{
	sys_init();
	while(1) {
		sys_update();
	}
}

void SPI_IRQHandler(void)
{
}

