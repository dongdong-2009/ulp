/* flash.c
 * 	dusk@2010 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "flash.h"
#include "time.h"


/*
 *adopt FLASH_ProgramHalfWord function,so where the data_size is odd,full with zero
 *para:OffsetAddress should be even
 */
FlashOpStatus flash_Write(uint32_t OffsetAddress,uint8_t * sour_addr, uint8_t data_size)
{
	uint32_t page_addr;
	uint16_t half_word = 0;
	volatile FLASH_Status flash_status = FLASH_COMPLETE;
	uint32_t i;
	
	if((OffsetAddress % 2) != 0)
		return FAILED;

	/*get page address*/
	page_addr = (OffsetAddress / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;
		
	/* Unlock the Flash Program Erase controller */
	FLASH_Unlock();

	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
  
	/* Erase the FLASH pages */
	if(FLASH_ErasePage( page_addr ) != FLASH_COMPLETE)
		return FAILED;
    
	for(i=0;i<data_size;i=i+2){
		if((i+2)>data_size)
			half_word = 0;
		else
			half_word |= *(sour_addr+i+1);
			
		half_word <<= 8;
		half_word |= *(sour_addr+i);		
		
		if(FLASH_ProgramHalfWord(OffsetAddress + i, half_word) != FLASH_COMPLETE)
			return FAILED;
		half_word = 0;
	}
			
	FLASH_Lock();
	
	return PASSED;
}

FlashOpStatus flash_Read(uint32_t OffsetAddress,uint8_t * dest_addr,uint32_t data_size)
{
	uint32_t i;
	for(i=0;i<data_size;i++){
		* (dest_addr + i) = *(__IO uint8_t*) (OffsetAddress + i);
	}
	
	return PASSED;
}
