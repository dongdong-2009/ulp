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
	// if ver = SD2.0 HC, sector need <<9
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
	//detect the card
	return 0;
}

int MAL_GetCardInfo(void)
{
	int status = 0;
#ifdef USE_STM3210B_EVAL
	status = pMMC->getcardinfo(&SDCardInfo);
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

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
