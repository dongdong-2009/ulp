/*
 * 	miaofng@2010 initial version
 *		This file is used to provide non-volatile data storage function
 *
 *	to avoid data lost in case of unwanted power down, principal:
 *		the other rom contains correct data before erase operation!!!
 *
 */

#include "config.h"
#include <stdlib.h>
#include <stddef.h>
#include "nvm.h"
#include "flash.h"
#include "ulp/debug.h"

#define NVM_MAGIC ((int)(0x13572468))
char nvm_flag_null;

static int align(int value, int page_size)
{
	int left = value % page_size;
	value += (left != 0) ? page_size : 0;
	value -= left;
	return value;
}

int nvm_init(void)
{
	char *src, *dst, *bak;
	int sz_ram, magic, sz_nvm, pages;

	dst = __section_begin(".nvm.ram");
	sz_ram = (int)__section_end(".nvm.ram") - (int)__section_begin(".nvm.ram");
	if(sz_ram == 0) { //no nvm var is used
		nvm_flag_null = 0;
		return 0;
	}

	sz_ram = align(sz_ram, 4);
	pages = align(sz_ram + 8, FLASH_PAGE_SZ) / FLASH_PAGE_SZ;
	src = (char *)FLASH_ADDR(FLASH_PAGE_NR - pages); //rom 1
	bak = (char *)FLASH_ADDR(FLASH_PAGE_NR - pages - pages); // rom 2

	//rom 1 -> ram, always read & erase rom 1 in case of "rom1 not null!!!"
	flash_Read(&magic, src, 4);
	flash_Read(&sz_nvm, src + 4, 4);

	if(magic == NVM_MAGIC && sz_nvm == sz_ram) {
		flash_Read(dst, src + 8, sz_ram);

		//rom1 data is ok, rom1 -> rom 2
		flash_Erase(bak, pages);
		flash_Write(bak + 4, &sz_nvm, 4);
		flash_Write(bak + 8, dst, sz_ram);
		flash_Write(bak + 0, &magic, 4);
		flash_Erase(src, pages); //erase rom 1
		nvm_flag_null = 0;
		return 0;
	}

	//to avoid one more time erase op on an empty flash page
	if(sz_nvm != -1) {
		flash_Erase(src, pages); //erase rom 1
	}

	//rom 2 -> ram
	src = bak;
	flash_Read(&magic, src, 4);
	flash_Read(&sz_nvm, src + 4, 4);
	if(magic == NVM_MAGIC && sz_nvm == sz_ram) {
		flash_Read(dst, src + 8, sz_ram);
		nvm_flag_null = 0;
		return 0;
	}

	//fail ...
	nvm_flag_null = 1;
	return -1;
}

int nvm_save(void)
{
	char *src, *dest, *bak;
	int magic = NVM_MAGIC;
	int sz_ram, pages;

	src = __section_begin(".nvm.ram");
	sz_ram = (int)__section_end(".nvm.ram") - (int)__section_begin(".nvm.ram");
	if(sz_ram == 0)
		return 0;
	sz_ram = align(sz_ram, 4);
	pages = align(sz_ram + 8, FLASH_PAGE_SZ) / FLASH_PAGE_SZ;
	dest = (char *)FLASH_ADDR(FLASH_PAGE_NR - pages); //rom 1
	bak = (char *)FLASH_ADDR(FLASH_PAGE_NR - pages - pages); // rom 2

	//ram -> rom 1
	flash_Write(dest + 4, &sz_ram, 4);
	flash_Write(dest + 8, src, sz_ram);
	flash_Write(dest + 0, &magic, 4);

	//ram -> rom 2
	flash_Erase(bak, pages);
	flash_Write(bak + 4, &sz_ram, 4);
	flash_Write(bak + 8, src, sz_ram);
	flash_Write(bak + 0, &magic, 4);

	//erase rom 1
	flash_Erase(dest, pages);
	return 0;
}

/*clear nvm strorage to default state*/
int nvm_clear(void)
{
	char *src, *dest, *bak;
	int sz_ram, pages;

	src = __section_begin(".nvm.ram");
	sz_ram = (int)__section_end(".nvm.ram") - (int)__section_begin(".nvm.ram");
	if(sz_ram == 0)
		return 0;
	sz_ram = align(sz_ram, 4);
	pages = align(sz_ram + 8, FLASH_PAGE_SZ) / FLASH_PAGE_SZ;
	dest = (char *)FLASH_ADDR(FLASH_PAGE_NR - pages); //rom 1
	bak = (char *)FLASH_ADDR(FLASH_PAGE_NR - pages - pages); // rom 2

	//erase rom 2
	flash_Erase(bak, pages);

	//erase rom 1
	flash_Erase(dest, pages);
	return 0;
}

//for power-down auto save function
void nvm_isr(void)
{
	nvm_save();
}

int nvm_is_null(void)
{
	return nvm_flag_null;
}

#include "shell/cmd.h"
#include <string.h>

int cmd_nvm_func(int argc, char *argv[])
{
	const char *usage = {
		"nvm save	.nvm.ram -> .nvm.rom\n"
		"nvm clear	.nvm.rom = 0xff\n"
	};

	if((argc == 2) && (!strncmp("save", argv[1], 2))) {
		nvm_save();
		return 0;
	}

	if((argc == 2) && (!strncmp("clear", argv[1], 2))) {
		nvm_clear();
		return 0;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_nvm = {"nvm", cmd_nvm_func, "nvm operation cmds"};
DECLARE_SHELL_CMD(cmd_nvm)
