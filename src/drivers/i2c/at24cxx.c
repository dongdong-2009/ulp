/*
 *	dusk@2010 initial version
 */

#include "i2c.h"
#include "at24cxx.h"

static int EEPROM_PAGE_SIZE = 8;

void at24cxx_Init(at24cxx_t *chip)
{
	i2c_cfg_t cfg = {
		10000,		//10K
		NULL,
	};
	
	EEPROM_PAGE_SIZE = chip->page_size;

	chip->bus->init(&cfg);
}

int at24cxx_WriteByte(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer)
{
	return chip->bus->wreg(chip_addr, addr, alen, buffer);
}

int at24cxx_WriteBuffer(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer, int len)
{
	int NumOfPage = 0, NumOfSingle = 0, count = 0;
	int reg_addr = 0;

	reg_addr = addr % EEPROM_PAGE_SIZE;
	count = EEPROM_PAGE_SIZE - reg_addr;
	NumOfPage =  len / EEPROM_PAGE_SIZE;
	NumOfSingle = len % EEPROM_PAGE_SIZE;

	if (reg_addr == 0) {
		if(NumOfPage == 0) {
			chip->bus->wbuf(chip_addr, addr, alen, buffer, NumOfSingle);
			chip->bus->WaitStandByState(chip_addr);
		} else {
			while (NumOfPage--) {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, EEPROM_PAGE_SIZE);
				chip->bus->WaitStandByState(chip_addr);
				addr += EEPROM_PAGE_SIZE;
				buffer += EEPROM_PAGE_SIZE;
			}
			if (NumOfSingle != 0) {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, NumOfSingle);
				chip->bus->WaitStandByState(chip_addr);
			}
		}
	} else {
		if (NumOfPage == 0) {
			if (len > count) {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, count);
				chip->bus->WaitStandByState(chip_addr);
				
				chip->bus->wbuf(chip_addr, (addr + count), alen, (buffer + count), (len - count));
				chip->bus->WaitStandByState(chip_addr);
			} else {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, len);
				chip->bus->WaitStandByState(chip_addr);
			}
		} else {
			len -= count;
			NumOfPage = len / EEPROM_PAGE_SIZE;
			NumOfSingle = len % EEPROM_PAGE_SIZE;

			if (count != 0) {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, count);
				chip->bus->WaitStandByState(chip_addr);
				addr += count;
				buffer += count;
			}

			while (NumOfPage--) {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, EEPROM_PAGE_SIZE);
				chip->bus->WaitStandByState(chip_addr);
				addr += EEPROM_PAGE_SIZE;
				buffer += EEPROM_PAGE_SIZE;
			}

			if (NumOfSingle != 0) {
				chip->bus->wbuf(chip_addr, addr, alen, buffer, NumOfSingle);
				chip->bus->WaitStandByState(chip_addr);
			}
		}
		
	}

	return 0;
}

int at24cxx_ReadByte(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer)
{
	return chip->bus->rbuf(chip_addr, addr, alen, buffer, 1);
}

int at24cxx_ReadBuffer(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer, int len)
{
	return chip->bus->rbuf(chip_addr, addr, alen, buffer, len);
}

#if 1
#include "ulp/sys.h"
#include "ulp/ulib.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static at24cxx_t at24c02 = {
	.bus = &i2cs,
	.page_size = 8,
};

static int cmd_at24cxx_func(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" at24 w addr string		addr = string\n" \
		" at24 r addr [nbytes]		read nbytes from addr, default = 1 byte\n" \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	int addr = atoi(argv[2]);
	int alen = 1;
	int size = 16;
	char temp[16];

	at24cxx_Init(&at24c02);
	if (!strcmp(argv[1], "w")) {
		at24cxx_WriteBuffer(&at24c02, 0xA0, addr, alen, argv[3], strlen(argv[3]));
	}

	if (!strcmp(argv[1], "r")) {
		if(argc > 3) size = atoi(argv[3]);
		at24cxx_ReadBuffer(&at24c02, 0xA0, addr, alen, temp, size);
		hexdump("eeprom", temp, size);
	}

	return 0;
}
const cmd_t cmd_at24cxx = {"at24", cmd_at24cxx_func, "at24cxx cmd"};
DECLARE_SHELL_CMD(cmd_at24cxx)
#endif
