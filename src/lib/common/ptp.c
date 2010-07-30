/*
 * 	miaofng@2010 initial version
 *		This file is used to decode and convert motorolar S-record to binary
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "common/print.h"
#include "common/ptp.h"

int ptp_init(ptp_t *ptp)
{
	return 0;
}

int ptp_read(ptp_t *ptp, void *buff, int btr, int *br)
{
	return 0;
}

int ptp_seek(ptp_t *ptp, int ofs)
{
	return 0;
}

int ptp_close(ptp_t *ptp)
{
	return 0;
}

//misc op
int ptp_size(ptp_t *ptp)
{
	return 0;
}
