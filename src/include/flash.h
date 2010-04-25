/* flash.h
 * 	dusk@2010 initial version
 */
#ifndef __FLASH_H_
#define __FLASH_H_

#include "stm32f10x.h"

/* Private define */
/* Define the STM32F10x FLASH Page Size depending on the used STM32 device */
#ifdef STM32F10X_LD
  #define FLASH_PAGE_BITS    10
#elif defined STM32F10X_MD
  #define FLASH_PAGE_BITS    10
#elif defined STM32F10X_HD
  #define FLASH_PAGE_BITS    11
#elif defined STM32F10X_CL
  #define FLASH_PAGE_BITS    11  
#endif /* STM32F10X_LD */

/*STM32 flash start address*/
#define FLASH_PAGE_SIZE ((uint16_t)(1 << FLASH_PAGE_BITS))
#define FLASH_PAGE_MASK ((uint16_t)(FLASH_PAGE_SIZE - 1))
#define STM32_FLASH_STARTADDR 0x08000000

typedef enum {
	FAILED = 0,
	PASSED = !FAILED
} FlashOpStatus;

FlashOpStatus flash_Write(uint32_t OffsetAddress,uint8_t * sour_addr, uint8_t data_size);
FlashOpStatus flash_Read(uint32_t OffsetAddress,uint8_t * dest_addr,uint32_t data_size);

#endif /*__FLASH_H_*/
