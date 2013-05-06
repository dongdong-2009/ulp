/* flash.c
 * 	dusk@2010 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "flash.h"
#include <string.h>

/*STM32 flash start address*/
#define PAGE_SIZE (FLASH_PAGE_SZ)
#define PAGE_MASK (PAGE_SIZE - 1)

int flash_Erase(const void *dest, size_t n)
{
	int i, idest = (int)dest;

	/*must be page aligned, to avoid mistakes*/
	if(idest & PAGE_MASK)
		return 0;

	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	for(i = 0; i < n; i ++) {
		if(FLASH_ErasePage(idest) != FLASH_COMPLETE)
			break;

		idest += PAGE_SIZE;
	}

	FLASH_Lock();
	return i;
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
	const char *psrc;
	int i, n, x, dest, word;

	psrc = src;
	dest = (int) pdest;

	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	for(i = 0; i < bytes; i += n) {
		#if 0
		word = 0xffffffff;
		n = bytes - i;
		n = (n > 4) ? 4 : n;
		x = dest & 0x03;
		if(x != 0) { //read-before-write
			n = (n > 4 - x) ? (4 - x) : n;
			dest -= x;
			word = *(int *) dest;
		}
		#else
		x = 0;
		n = 4;
		#endif
		memcpy((char *)&word + x, psrc, n);
		if(FLASH_ProgramWord(dest, word) != FLASH_COMPLETE) {
			break;
		}

		psrc += n;
		dest += 4;
	}

	FLASH_Lock();
	return i;
}

int flash_Read(void *dest, const void *src, size_t n)
{
	memcpy(dest, src, n);
	return n;
}
