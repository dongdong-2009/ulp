/*
 * this is the demo routine for nxp lpc1788 develop board
 * miaofng@2012
 *
 */

#include "config.h"
#include "ulp/sys.h"

void lpc_init(void)
{
}

void lpc_update(void)
{
}

void main(void)
{
	sys_init();
	lpc_init();
	while(1) {
		sys_update();
		lpc_update();
	}
}

