/*
 * 	dusk@2010 initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/system.h"
#include "stm32f10x.h"
#include "flash.h"

/*for flash read*/
static int cmd_flash_mr(int argc, char *argv[])
{
	uint8_t readbuffer;
	uint32_t i;
	uint32_t offset;
	uint32_t data_size;
	
	if(argc != 3){
		printf("uasge:\n");
		printf(" flashmr offset size\n");
		return 0;
	}
	
	offset = (uint32_t)atoi(argv[1]);
	data_size = (uint32_t)atoi(argv[2]);

	for(i = 0; i < data_size ; i++ )
	{
		flash_Read( i + offset , &readbuffer , 1);
		printf("0x%x, ",readbuffer);
		if(i%4 == 3)
			printf("\n");
	}
	printf("\n");	
	
	return 0;
}
const cmd_t cmd_flashmr = {"flashmr", cmd_flash_mr, "for reading user flash memory"};
DECLARE_SHELL_CMD(cmd_flashmr)

/*for flash write*/
static int cmd_flash_mw(int argc, char *argv[])
{
	uint8_t writebuffer[4];
	uint32_t offset;
	uint32_t data;
	
	if(argc != 3){
		printf("uasge:\n");
		printf(" flashmw offset data\n");
		return 0;
	}
	
	offset = (uint32_t)atoi(argv[1]);
	data = (uint32_t)atoi(argv[2]);
	writebuffer[0] = (uint8_t)data;
	writebuffer[1] = (uint8_t)(data>>8);
	writebuffer[2] = (uint8_t)(data>>16);
	writebuffer[3] = (uint8_t)(data>>24);

	flash_Write(offset , writebuffer , 4);
	
	return 0;
}
const cmd_t cmd_flashmw = {"flashmw", cmd_flash_mw, "for writing user flash memory"};
DECLARE_SHELL_CMD(cmd_flashmw)
