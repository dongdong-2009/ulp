/*
 * 	dusk@2010 initial version
 */

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include "ff.h"

static FATFS fs;

static int cmd_ff_mount(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" mount ,mount a disk " \
	};
	
	if(argc > 0 && argc != 1) {
		printf(usage);
		return 0;
	}
	
	if(!f_mount(0, &fs))
		printf("mount ok!\n\r");
	else
		printf("mount fail!\n\r");

	return 0;	
}
const cmd_t cmd_mount = {"mount", cmd_ff_mount, "mount a disk"};
DECLARE_SHELL_CMD(cmd_mount)

static int cmd_ff_umount(int argc, char *argv[])
{
	const char usage[] = { \
		" usage:\n" \
		" umount ,umount a disk " \
	};
	
	if(argc > 0 && argc != 1) {
		printf(usage);
		return 0;
	}
	
	if(!f_mount(0, NULL))
		printf("umount ok!\n\r");
	else
		printf("umount fail!\n\r");

	return 0;	
}
const cmd_t cmd_umount = {"umount", cmd_ff_umount, "umount a disk"};
DECLARE_SHELL_CMD(cmd_umount)

static int cmd_ff_fread(int argc, char *argv[])
{
	const char *usage = { \
		" usage:\n" \
		" cat filename, read a file\n" \
	};
	 
	if(argc > 0 && argc != 2) {
		printf(usage);
		return 0;
	}
	
	FRESULT res;
	FIL file;
	unsigned int br;
	char buffer[9];

	res = f_open(&file, argv[1], FA_OPEN_EXISTING | FA_READ);
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
	return 0;
}
const cmd_t cmd_fread = {"cat", cmd_ff_fread, "read a file"};
DECLARE_SHELL_CMD(cmd_fread)

static int cmd_ff_fwrite(int argc, char *argv[])
{

	const char usage[] = { \
		" usage:\n" \
		" fwrite filename, write a file" \
	};
	 
	if(argc > 0 && argc != 2) {
		printf(usage);
		return 0;
	}
	
	FRESULT res;
	FIL file;
	unsigned int br;
	char Context[] = "this is new file,created by dusk!";

	res = f_open(&file, argv[1], FA_CREATE_ALWAYS |FA_WRITE);
	if (res == FR_OK) {
			res = f_write(&file, Context, sizeof(Context), &br);
	} else {
		printf("operation failed!\n\r");
	}
	f_close(&file);

	return 0;
}
const cmd_t cmd_fwrite = {"fwrite", cmd_ff_fwrite, "write a file"};
DECLARE_SHELL_CMD(cmd_fwrite)

static int cmd_ff_ls(int argc, char *argv[])
{

	const char usage[] = { \
		" usage:\n" \
		" dir, display files" \
	};
	 
	if(argc > 0 && argc != 1) {
		printf(usage);
		return 0;
	}
	
	FRESULT res;
	FILINFO fileinfo;
	DIR fdir;
	char *filename;
	char path[] = "";

	res = f_opendir(&fdir, path);
	if (res == FR_OK) {
		for (;;) {
			res = f_readdir(&fdir, &fileinfo);
			if (res != FR_OK || fileinfo.fname[0] == 0) break;
			if (fileinfo.fname[0] == '.') continue;
			filename = fileinfo.fname;
			printf("%s/%s \n\r",path, filename);
		}
	} else {
		printf("operation failed!\n\r");
	}

	return 0;
}
const cmd_t cmd_ls = {"ls", cmd_ff_ls, "display files"};
DECLARE_SHELL_CMD(cmd_ls)

static int cmd_ff_cd(int argc, char *argv[])
{

	const char usage[] = { \
		" usage:\n" \
		" cd subdir" \
	};
	 
	if(argc > 0 && argc != 2) {
		printf(usage);
		return 0;
	}
	
	FRESULT res;

	res = f_chdir(argv[1]);
	if (res != FR_OK) {
		printf("operation failed!\n\r");
	}

	return 0;
}
const cmd_t cmd_cd = {"cd", cmd_ff_cd, "cd sub directroy"};
DECLARE_SHELL_CMD(cmd_cd)

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
