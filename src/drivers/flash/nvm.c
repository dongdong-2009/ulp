/*
 * 	miaofng@2010 initial version
 *		This file is used to provide non-volatile data storage function
 */

#include "config.h"
#include <stdlib.h>
#include <stddef.h>
#include "nvm.h"
#include "flash.h"
#include "debug.h"

#define NVM_MAGIC ((int)(0x13572468))

//move __nvm data from section ".nvm.flash" to ".nvm.ram"
int nvm_init(void)
{
	char *src, *dst, *bak;
	int sz_rom, sz_ram;
	int magic, sz_nvm;

	src = __section_begin(".nvm.rom");
	dst = __section_begin(".nvm.ram");
	sz_ram = (int)__section_end(".nvm.ram") - (int)__section_begin(".nvm.ram");
	sz_rom = (int)__section_end(".nvm.rom") - (int)__section_begin(".nvm.rom");
	
	sz_rom >>= 1;
	sz_ram = (sz_ram + 3) & ( ~ 0x03); //align to 4 bytes boundary
	assert(sz_ram <= sz_rom);
	bak = src + sz_rom;

	//rom 1 -> ram, always read & erase rom 1
	flash_Read(&magic, src, 4);
	flash_Read(&sz_nvm, src + 4, 4);
	flash_Read(dst, src + 8, sz_ram);
	flash_Erase(src, (sz_rom + FLASH_PAGE_SZ - 1) / FLASH_PAGE_SZ);
	if(magic == NVM_MAGIC && sz_nvm == sz_ram) {		
		//rom1 -> rom 2
		flash_Erase(bak, (sz_rom + FLASH_PAGE_SZ - 1) / FLASH_PAGE_SZ);
		flash_Write(bak + 8, dst, sz_ram);
		flash_Write(bak + 4, &sz_nvm, 4); 
		flash_Write(bak + 0, &magic, 4);  
		return 0;
	}
	
	//rom 2 -> ram
	src = bak;
	flash_Read(&magic, src, 4);
	flash_Read(&sz_nvm, src + 4, 4);
	if(magic == NVM_MAGIC && sz_nvm == sz_ram) {
		flash_Read(dst, src + 8, sz_ram); 
		return 0;
	}
	
	//fail ...
	return -1;
}

//move __nvm data from ".nvm.ram" to section ".nvm.flash"
int nvm_save(void)
{
	char *src, *dest;
	int magic = NVM_MAGIC;
	int sz_ram;

	dest = __section_begin(".nvm.rom");
	src = __section_begin(".nvm.ram");
	sz_ram = (int)__section_end(".nvm.ram") - (int)__section_begin(".nvm.ram");
	sz_ram = (sz_ram + 3) & ( ~ 0x03); //align to 4 bytes boundary
	
	flash_Write(dest + 8, src, sz_ram);
	flash_Write(dest + 4, &sz_ram, 4);
	flash_Write(dest + 0, &magic, 4);
	return 0;
}

//for power-down auto save function
void nvm_isr(void)
{
	nvm_save();
}