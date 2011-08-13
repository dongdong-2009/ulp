/*
 * 	dusk@2010 initial version
 */

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"
#include "sys/sys.h"

static FATFS fs;

static int cmd_fatfs_func(int argc, char *argv[])
{
	FRESULT res;
	FIL file;
	FILINFO fileinfo;
	DIR fdir;
#ifdef CONFIG_FATFS_LFN
	char lfname_buf[32];
	fileinfo.lfsize = 32;
	fileinfo.lfname = lfname_buf;
#endif


	char *filename;
	char path[] = "";
	char buffer[9];
	unsigned int br;
	
	const char *usage = { \
		"usage:\n " \
		"fatfs mount      ,mount a disk \n " \
		"fatfs umount     ,umount a disk \n " \
		"fatfs cd subdir  ,enter a sub directory \n " \
		"fatfs pwd        ,display current full path \n " \
		"fatfs ls         , display files \n " \
		"fatfs vi fn text ,write context to a file\n " \
		"fatfs cat fn     ,read a text file \n" \
	};
	
	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (!strcmp(argv[1], "mount")) {
		if(!f_mount(0, &fs))
			printf("mount ok!\n\r");
		else
			printf("mount fail!\n\r");
	}

	if (!strcmp(argv[1], "umount")) {
		if(!f_mount(0, NULL))
			printf("umount ok!\n\r");
		else
			printf("umount fail!\n\r");
	}

	if (!strcmp(argv[1], "cd")) {
		res = f_chdir(argv[2]);
		if (res != FR_OK)
			printf("operation failed!\n\r");
	}

	if (!strcmp(argv[1], "pwd")) {
		filename = (char *)sys_malloc(30);
		memset(filename, 0, 30);
		res = f_getcwd(filename, 30);
		if (res != FR_OK)
			printf("operation failed!\n\r");
		else
			printf("%s\n\r", filename);
		sys_free(filename);
	}

	if (!strcmp(argv[1], "ls")) {
		res = f_opendir(&fdir, path);
		if (res == FR_OK) {
			for (;;) {
				res = f_readdir(&fdir, &fileinfo);
				if (res != FR_OK || fileinfo.fname[0] == 0) break;
				if (fileinfo.fname[0] == '.') continue;
				if (fileinfo.lfname[0] == 0)
					printf("%s/%s \n\r",path, fileinfo.fname);
				else
					printf("%s/%s \n\r",path, fileinfo.lfname);
			}
		} else {
			printf("operation failed!\n\r");
		}
	}

	if (!strcmp(argv[1], "vi")) {
		res = f_open(&file, argv[2], FA_CREATE_ALWAYS |FA_WRITE);
		if (res == FR_OK) {
				res = f_write(&file, argv[3], strlen(argv[3]), &br);
				printf("%s\n\r", argv[3]);
				printf("%d Bytes to be written!\n\r", strlen(argv[3]));
				printf("%d Bytes has been written!\n\r", br);
		} else {
			printf("operation failed!\n\r");
		}
		f_close(&file);
	}

	if (!strcmp(argv[1], "cat")) {
		res = f_open(&file, argv[2], FA_OPEN_EXISTING | FA_READ);
		if (res == FR_OK) {
			for (;;) {
				res = f_read(&file, buffer, sizeof(buffer) - 1, &br);
				if (res || br == 0) break; /* error or eof */
				buffer[br] = '\0';
				printf("%s",buffer);
			}
			printf("\n\r");
		} else {
			printf("operation failed!\n\r");
		}
		f_close(&file);
	}
	return 0;
}
const cmd_t cmd_fatfs = {"fatfs", cmd_fatfs_func, "fatfs operation cmds"};
DECLARE_SHELL_CMD(cmd_fatfs)

#if 0
static int cmd_sd_getinfo(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" dir, display files" \
	};
	 
	if(argc > 0 && argc != 1) {
		printf(usage);
		return 0;
	}

	if( MAL_GetCardInfo()) {
		printf("get error!\n\r");
	} else {
		printf("SD Card type is %s\n\r", SDCardInfo.CardType == CARDTYPE_SDV2HC? "SD High Capacity":"SD NOMAL");
		if(SDCardInfo.CardType == CARDTYPE_SDV2HC)
			printf("SD Capacity is %d MBytes\n\r", SDCardInfo.CardCapacity>>10);
		else
			printf("SD Capacity is %d MBytes\n\r", SDCardInfo.CardCapacity>>20);
	}
	return 0;
}
const cmd_t cmd_getinfo = {"getinfo", cmd_sd_getinfo, "display files"};
DECLARE_SHELL_CMD(cmd_getinfo)
#endif
#endif
