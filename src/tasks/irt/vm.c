/*
*
*  miaofng@2014-5-10   initial version
*  1, only matrix relay operation will be buffered in opq(bus&line sw is configured by command 'mode')
*/

#include "ulp/sys.h"
#include "vm.h"
#include <string.h>
#include "common/circbuf.h"

circbuf_t vm_opq; /*virtual machine task queue*/

void vm_init(void)
{
}

int vm_update(void)
{
	return 0;
}

int vm_fetch( //non-zero if error happened
	void *buf, //rx buffer,  normally = can_msg.data[8]
	int *bytes //input buf size, output bytes filled
)
{
	return 0;
}

int vm_opgrp_is_over(void)
{
	return 0;
}

int vm_opq_is_empty(void)
{
	return 1;
}

//relay_list = (@bbllll,bbllll:llll,bbllll,bbllll:llll,...)
int vm_opq_add(int opcode, const char *relay_list)
{
	return 0;
}
