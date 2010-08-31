/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name					: mass_mal.h
* Author						 : MCD Application Team
* Version						: V3.1.0RC1
* Date							 : 09/28/2009
* Description				: Header for mass_mal.c file.
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

#include "diskio.h"

typedef struct			 /* Card Specific Data */
{
	unsigned char	CSDStruct;						/* CSD structure */
	unsigned char	SysSpecVersion;			 /* System specification version */
	unsigned char	Reserved1;						/* Reserved */
	unsigned char	TAAC;								 /* Data read access-time 1 */
	unsigned char	NSAC;								 /* Data read access-time 2 in CLK cycles */
	unsigned char	MaxBusClkFrec;				/* Max. bus clock frequency */
	unsigned short CardComdClasses;			/* Card command classes */
	unsigned char	RdBlockLen;					 /* Max. read data block length */
	unsigned char	PartBlockRead;				/* Partial blocks for read allowed */
	unsigned char	WrBlockMisalign;			/* Write block misalignment */
	unsigned char	RdBlockMisalign;			/* Read block misalignment */
	unsigned char	DSRImpl;							/* DSR implemented */
	unsigned char	Reserved2;						/* Reserved */
	unsigned DeviceSize;					 /* Device Size */
	unsigned char	MaxRdCurrentVDDMin;	 /* Max. read current @ VDD min */
	unsigned char	MaxRdCurrentVDDMax;	 /* Max. read current @ VDD max */
	unsigned char	MaxWrCurrentVDDMin;	 /* Max. write current @ VDD min */
	unsigned char	MaxWrCurrentVDDMax;	 /* Max. write current @ VDD max */
	unsigned char	DeviceSizeMul;				/* Device size multiplier */
	unsigned char	EraseGrSize;					/* Erase group size */
	unsigned char	EraseGrMul;					 /* Erase group size multiplier */
	unsigned char	WrProtectGrSize;			/* Write protect group size */
	unsigned char	WrProtectGrEnable;		/* Write protect group enable */
	unsigned char	ManDeflECC;					 /* Manufacturer default ECC */
	unsigned char	WrSpeedFact;					/* Write speed factor */
	unsigned char	MaxWrBlockLen;				/* Max. write data block length */
	unsigned char	WriteBlockPaPartial;	/* Partial blocks for write allowed */
	unsigned char	Reserved3;						/* Reserded */
	unsigned char	ContentProtectAppli;	/* Content protection application */
	unsigned char	FileFormatGrouop;		 /* File format group */
	unsigned char	CopyFlag;						 /* Copy flag (OTP) */
	unsigned char	PermWrProtect;				/* Permanent write protection */
	unsigned char	TempWrProtect;				/* Temporary write protection */
	unsigned char	FileFormat;					 /* File Format */
	unsigned char	ECC;									/* ECC code */
	unsigned char	CSD_CRC;							/* CSD CRC */
	unsigned char	Reserved4;						/* always 1*/
} SD_CSD;

typedef struct			/*Card Identification Data*/
{
	unsigned char	ManufacturerID;			 /* ManufacturerID */
	unsigned short OEM_AppliID;					/* OEM/Application ID */
	unsigned ProdName1;						/* Product Name part1 */
	unsigned char	ProdName2;						/* Product Name part2*/
	unsigned char	ProdRev;							/* Product Revision */
	unsigned ProdSN;							 /* Product Serial Number */
	unsigned char	Reserved1;						/* Reserved1 */
	unsigned short ManufactDate;				 /* Manufacturing Date */
	unsigned char	CID_CRC;							/* CID CRC */
	unsigned char	Reserved2;						/* always 1 */
} SD_CID;

typedef struct
{
	SD_CSD SD_csd;
	SD_CID SD_cid;
	unsigned CardCapacity; /* Card Capacity */
	unsigned CardBlockSize; /* Card Block Size */
	unsigned short RCA;
	unsigned char CardType;
} SD_CardInfo;

typedef struct {
	unsigned char (*init)(void);
	unsigned char (*readbuf)(unsigned char *, unsigned int, unsigned char);
	unsigned char(*writebuf)(const unsigned char *, unsigned int, unsigned char);
	unsigned char(*getcardinfo)(SD_CardInfo *);
} mmc_t;

#define CARDTYPE_MMC			0x00
#define CARDTYPE_SDV1			0x01
#define CARDTYPE_SDV2			0x02
#define CARDTYPE_SDV2HC			0x04

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

#define MAL_OK   0
#define MAL_FAIL 1
#define MAX_LUN  1

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

int MAL_Init ();
int MAL_GetStatus ();
int MAL_GetCardInfo();
int MAL_Read(unsigned char *buff, unsigned int sector, unsigned char count);
int MAL_Write(const unsigned char *buff, unsigned int sector, unsigned char count);
int MMC_disk_ioctl(unsigned ctrl, void *buff);
int NOP(void);

#endif /* __MASS_MAL_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
