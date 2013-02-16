/*
*	miaofng@2013 ybs demo program based on oid board
*/
#include "config.h"
#include "ulp/sys.h"
#include "aduc706x.h"

void ybs_init(void)
{
}

void ybs_update(void)
{
}

int main(void)
{
	sys_init();
	ybs_init();
	while(1) {
		sys_update();
		ybs_update();
	}
}
