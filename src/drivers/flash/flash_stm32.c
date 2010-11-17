/* flash.c
 * 	dusk@2010 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "flash.h"
#include <string.h>

/* Private define */
/* Define the STM32F10x FLASH Page Size depending on the used STM32 device */
#ifdef CONFIG_STM32F10X_LD
  #define PAGE_BITS    10
#elif defined CONFIG_STM32F10X_MD
  #define PAGE_BITS    10
#elif defined CONFIG_STM32F10X_HD
  #define PAGE_BITS    11
#elif defined CONFIG_STM32F10X_CL
  #define PAGE_BITS    11
#endif

/*STM32 flash start address*/
#define PAGE_SIZE (1 << PAGE_BITS)
#define PAGE_MASK (PAGE_SIZE - 1)

int flash_Erase(void *dest, size_t n)
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

int flash_Write(void *dest, const void *src, size_t n)
{
	const int *psrc;
	int i, idest;

	psrc = src;
	idest = (int)dest;

	/*data align check*/
	if(idest & 0x03)
		return 0;

	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

	for(i = 0; n - i >= 4; i += 4) {
		if(FLASH_ProgramWord(idest, *psrc) != FLASH_COMPLETE)
			break;

		psrc ++;
		idest += 4;
	}

	FLASH_Lock();
	return i;
}

int flash_Read(void *dest, const void *src, size_t n)
{
	memcpy(dest, src, n);
	return n;
}
