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

typedef struct {
	void (*init)(void);
	void (*readbuf)(unsigned char *, unsigned int, unsigned short);
	void (*writebuf)(unsigned char *, unsigned int, unsigned short)
} mmc_t;

#define MMC_disk_initialize MAL_Init
#define MMC_disk_status MAL_GetStatus
#define MMC_disk_read MAL_Read
#define MMC_disk_write MAL_Write


/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

int MAL_Init ();
int MAL_GetStatus ();
int MAL_Read(unsigned char *buff, unsigned int sector, unsigned char count);
int MAL_Write(unsigned char *buff, unsigned int sector, unsigned char count);
#endif /* __MASS_MAL_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
