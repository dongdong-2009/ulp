/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : mass_mal.h
* Author             : MCD Application Team
* Version            : V3.1.0RC1
* Date               : 09/28/2009
* Description        : Header for mass_mal.c file.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MASS_MAL_H
#define __MASS_MAL_H

#include "stm32f10x.h"

typedef struct {
	unsigned char (*init)(void);
	unsigned char (*readbuf)(unsigned char *, unsigned int, unsigned char);
	unsigned char(*writebuf)(const unsigned char *, unsigned int, unsigned char);
} mmc_t;

#define ATA_disk_initialize() NOP()
#define USB_disk_initialize() NOP()
#define ATA_disk_status() NOP()
#define USB_disk_status() NOP()
#define USB_disk_read(buff, sector, count) NOP()
#define ATA_disk_read(buff, sector, count) NOP()
#define ATA_disk_write(buff, sector, count) NOP()
#define USB_disk_write(buff, sector, count) NOP()
#define ATA_disk_ioctl(ctrl, buff) NOP()
#define USB_disk_ioctl(ctrl, buff) NOP()

#define MMC_disk_initialize MAL_Init
#define MMC_disk_status MAL_GetStatus
#define MMC_disk_read MAL_Read
#define MMC_disk_write MAL_Write

#define USE_STM3210B_EVAL
#undef USE_STM3210E_EVAL


/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

int MAL_Init ();
int MAL_GetStatus ();
int MAL_Read(unsigned char *buff, unsigned int sector, unsigned char count);
int MAL_Write(const unsigned char *buff, unsigned int sector, unsigned char count);
int MMC_disk_ioctl(unsigned ctrl, void *buff);
int NOP(void);
#endif /* __MASS_MAL_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
