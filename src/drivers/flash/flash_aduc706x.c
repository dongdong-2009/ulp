/* flash.c
 * 	miaofng@2013/10/9 initial version
 */

#include "config.h"
#include "aduc706x.h"
#include "flash.h"
#include <string.h>

#define PAGE_SIZE (FLASH_PAGE_SZ)
#define PAGE_MASK (PAGE_SIZE - 1)

int flash_Erase(const void *dest, size_t pages)
{
	char *addr_start = (char *)dest;
	char *addr_end = addr_start + pages * PAGE_SIZE - 1;

	/* Page erasure takes about 20ms per page in which time
	any access to flash will stall the core. */
	__disable_interrupt();
	int ecode = FeeClr(addr_start, addr_end);
	__enable_interrupt();
	return (ecode != 0) ? 0 : -1;
}

/*
    00 01 02 03
00: AA BB CC DD
01: XX AA BB CC
02: XX XX AA BB
03: XX XX XX AA
*/
int flash_Write(const void *pdest, const void *src, size_t bytes)
{
	int ecode = FeeWr((char *)pdest, (char *)src, bytes);
	return (ecode != 0) ? 0 : -1;
}

int flash_Read(void *dest, const void *src, size_t n)
{
	memcpy(dest, src, n);
	return n;
}
