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
#include "mass_mal.h"
#include "msd.h"
#include "config.h"

//global members for mass storage
#ifdef CONFIG_LIB_UDISK
unsigned int Mass_Memory_Size[2];
unsigned int Mass_Block_Size[2];
unsigned int Mass_Block_Count[2];
volatile unsigned int Status = 0;
#endif

//private members for spi sdcard
static SD_CardInfo SDCardInfo;
extern mmc_t spi_card;
static mmc_t *pMMC;

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
* Input          : sector is sector addr,phy addr = sector << 9
* Output         : None
* Return         : None
*******************************************************************************/
int MAL_Write(const unsigned char *buff, unsigned int sector, unsigned char count)
{
	int status = 0;
	// if ver != SD2.0 HC, sector need <<9
	if(SDCardInfo.CardType != CARDTYPE_SDV2HC)
		sector = sector<<9;

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
	// if ver = SD2.0 HC, sector need <<9
	if(SDCardInfo.CardType != CARDTYPE_SDV2HC)
		sector = sector<<9;

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
	uint32_t temp_block_mul = 0;
	SD_CSD MSD_csd;
	uint32_t DeviceSizeMul = 0;

	MSD_GetCSDRegister(&MSD_csd);
	DeviceSizeMul = MSD_csd.DeviceSizeMul + 2;
	temp_block_mul = (1 << MSD_csd.RdBlockLen)/ 512;
	Mass_Block_Count[0] = ((MSD_csd.DeviceSize + 1) * (1 << (DeviceSizeMul))) * temp_block_mul;
	Mass_Block_Size[0] = 512;
	Mass_Memory_Size[0] = (Mass_Block_Count[0] * Mass_Block_Size[0]);

	return MAL_OK;
}

int MAL_GetCardInfo(void)
{
	int status = 0;
	status = pMMC->getcardinfo(&SDCardInfo);
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
/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
