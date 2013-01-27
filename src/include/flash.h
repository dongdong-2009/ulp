/* flash.h
 * 	dusk@2010 initial version
 *	miaofng@2010, revise this file to a common flash api header<device independent>
 */
#ifndef __FLASH_H_
#define __FLASH_H_

#include <stddef.h>
#include <config.h>

#if CONFIG_CPU_STM32 == 1
	#define FLASH_PAGE_SZ	(2048)
	#define FLASH_PAGE_NR	((*(unsigned short *)0x1ffff7e0) >> 1)
	#define FLASH_ADDR(page)	(0x08000000 + (page << 11))
#else
	#error "flash driver not available!!!"
#endif

//erase pages of flash sectors, which is given by address of dest(must be PAGE_SIZE aligned)
int flash_Erase(const void *dest, size_t pages);

/*write/read flash, return bytes have been wrote/read
note: flash mem space must be erased before program!!!
*/
int flash_Write(const void *dest, const void *src, size_t bytes);
int flash_Read(void *dest, const void *src, size_t n);

#endif /*__FLASH_H_*/
