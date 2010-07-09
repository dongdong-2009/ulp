/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : mass_mal.c
* Author             : MCD Application Team
* Version            : V3.1.0RC1
* Date               : 09/28/2009
* Description        : Medium Access Layer interface
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#ifdef USE_STM3210E_EVAL
 #include "sdcard.h"
#else
 #include "msd.h"
#endif /* USE_STM3210E_EVAL */
#include "mass_mal.h"

static SD_CardInfo SDCardInfo;
extern mmc_t spi_card;
static mmc_t *pMMC;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int MAL_Init()
{
	int status = 0;

#ifdef USE_STM3210E_EVAL
      status = SD_Init();
      status = SD_GetCardInfo(&SDCardInfo);
      status = SD_SelectDeselect((uint32_t) (SDCardInfo.RCA << 16));
      status = SD_EnableWideBusOperation(SDIO_BusWide_4b);
      status = SD_SetDeviceMode(SD_DMA_MODE);
#else
	pMMC = &spi_card;
	status = pMMC->init();
	
#endif

	return status;
}
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int MAL_Write(const unsigned char *buff, unsigned int sector, unsigned char count)
{
	int status = 0;
	sector <<= 9;
#ifdef USE_STM3210E_EVAL
      Status = SD_WriteBlock(Memory_Offset, Writebuff, Transfer_Length);
      if ( Status != SD_OK )
      {
        return MAL_FAIL;
      }      
#else
	status = pMMC->writebuf(buff, sector, count);
#endif

	return status;
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
int MAL_Read(unsigned char *buff, unsigned int sector, unsigned char count)
{
	int status = 0;
	sector <<= 9;
#ifdef USE_STM3210E_EVAL
      Status = SD_ReadBlock(Memory_Offset, Readbuff, Transfer_Length);
      if ( Status != SD_OK )
      {
        return MAL_FAIL;
      }
#else
	status = pMMC->readbuf(buff, sector, count);
#endif

	return status;
}

/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int MAL_GetStatus ()
{
	//detect the card
	return 0;
}

int MAL_GetCardInfo(void)
{
	int status = 0;
#ifdef USE_STM3210B_EVAL
	status = MSD_GetCSDRegister(&SDCardInfo.SD_csd);
	status = MSD_GetCIDRegister(&SDCardInfo.SD_cid);

	SDCardInfo.CardCapacity = (SDCardInfo.SD_csd.DeviceSize + 1) ;
 	SDCardInfo.CardCapacity *= (1 << (SDCardInfo.SD_csd.DeviceSizeMul + 2));
	SDCardInfo.CardBlockSize = 1 << (SDCardInfo.SD_csd.RdBlockLen);
	SDCardInfo.CardCapacity *= SDCardInfo.CardBlockSize;
	
	SDCardInfo.CardBlockSize = 512;
	SDCardInfo.RCA = 0;
	SDCardInfo.CardType = 0;	
#endif
	return status;
}

int MMC_disk_ioctl(unsigned ctrl, void *buff)
{
	switch(ctrl) {
		case CTRL_SYNC:
			return RES_OK;
			
		case GET_BLOCK_SIZE:
			*(unsigned short*)buff = (unsigned short)SDCardInfo.CardBlockSize;
			return RES_OK;

		case GET_SECTOR_COUNT:
			*(unsigned int*)buff = SDCardInfo.CardCapacity;
			return RES_OK;
	}
	return RES_PARERR;
}

int NOP(void)
{
	return 0;
}


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

	const char usage[] = { \
		" usage:\n" \
		" fread filename, read a file" \
	};
	 
	if(argc > 0 && argc != 2) {
		printf(usage);
		return 0;
	}
	
	FRESULT res;
	FIL file;
	unsigned int br;
	char buffer[100];

	res = f_open(&file, argv[1], FA_OPEN_EXISTING | FA_READ);
	if (res == FR_OK) {
		for (;;) {
			res = f_read(&file, buffer, sizeof(buffer), &br);
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
const cmd_t cmd_fread = {"fread", cmd_ff_fread, "read a file"};
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
	char path[] = "0:";

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

	if( MAL_GetCardInfo())
		printf("get error!\n\r");
	return 0;
}
const cmd_t cmd_getinfo = {"getinfo", cmd_sd_getinfo, "display files"};
DECLARE_SHELL_CMD(cmd_getinfo)
#endif
/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
