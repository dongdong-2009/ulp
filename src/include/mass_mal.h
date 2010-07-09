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
#include "diskio.h"

typedef struct       /* Card Specific Data */
{
  __IO uint8_t  CSDStruct;            /* CSD structure */
  __IO uint8_t  SysSpecVersion;       /* System specification version */
  __IO uint8_t  Reserved1;            /* Reserved */
  __IO uint8_t  TAAC;                 /* Data read access-time 1 */
  __IO uint8_t  NSAC;                 /* Data read access-time 2 in CLK cycles */
  __IO uint8_t  MaxBusClkFrec;        /* Max. bus clock frequency */
  __IO uint16_t CardComdClasses;      /* Card command classes */
  __IO uint8_t  RdBlockLen;           /* Max. read data block length */
  __IO uint8_t  PartBlockRead;        /* Partial blocks for read allowed */
  __IO uint8_t  WrBlockMisalign;      /* Write block misalignment */
  __IO uint8_t  RdBlockMisalign;      /* Read block misalignment */
  __IO uint8_t  DSRImpl;              /* DSR implemented */
  __IO uint8_t  Reserved2;            /* Reserved */
  __IO uint32_t DeviceSize;           /* Device Size */
  __IO uint8_t  MaxRdCurrentVDDMin;   /* Max. read current @ VDD min */
  __IO uint8_t  MaxRdCurrentVDDMax;   /* Max. read current @ VDD max */
  __IO uint8_t  MaxWrCurrentVDDMin;   /* Max. write current @ VDD min */
  __IO uint8_t  MaxWrCurrentVDDMax;   /* Max. write current @ VDD max */
  __IO uint8_t  DeviceSizeMul;        /* Device size multiplier */
  __IO uint8_t  EraseGrSize;          /* Erase group size */
  __IO uint8_t  EraseGrMul;           /* Erase group size multiplier */
  __IO uint8_t  WrProtectGrSize;      /* Write protect group size */
  __IO uint8_t  WrProtectGrEnable;    /* Write protect group enable */
  __IO uint8_t  ManDeflECC;           /* Manufacturer default ECC */
  __IO uint8_t  WrSpeedFact;          /* Write speed factor */
  __IO uint8_t  MaxWrBlockLen;        /* Max. write data block length */
  __IO uint8_t  WriteBlockPaPartial;  /* Partial blocks for write allowed */
  __IO uint8_t  Reserved3;            /* Reserded */
  __IO uint8_t  ContentProtectAppli;  /* Content protection application */
  __IO uint8_t  FileFormatGrouop;     /* File format group */
  __IO uint8_t  CopyFlag;             /* Copy flag (OTP) */
  __IO uint8_t  PermWrProtect;        /* Permanent write protection */
  __IO uint8_t  TempWrProtect;        /* Temporary write protection */
  __IO uint8_t  FileFormat;           /* File Format */
  __IO uint8_t  ECC;                  /* ECC code */
  __IO uint8_t  CSD_CRC;              /* CSD CRC */
  __IO uint8_t  Reserved4;            /* always 1*/
} SD_CSD;

typedef struct      /*Card Identification Data*/
{
  __IO uint8_t  ManufacturerID;       /* ManufacturerID */
  __IO uint16_t OEM_AppliID;          /* OEM/Application ID */
  __IO uint32_t ProdName1;            /* Product Name part1 */
  __IO uint8_t  ProdName2;            /* Product Name part2*/
  __IO uint8_t  ProdRev;              /* Product Revision */
  __IO uint32_t ProdSN;               /* Product Serial Number */
  __IO uint8_t  Reserved1;            /* Reserved1 */
  __IO uint16_t ManufactDate;         /* Manufacturing Date */
  __IO uint8_t  CID_CRC;              /* CID CRC */
  __IO uint8_t  Reserved2;            /* always 1 */
} SD_CID;

typedef struct
{
  SD_CSD SD_csd;
  SD_CID SD_cid;
  uint32_t CardCapacity; /* Card Capacity */
  uint32_t CardBlockSize; /* Card Block Size */
  uint16_t RCA;
  uint8_t CardType;
} SD_CardInfo;


typedef struct {
	unsigned char (*init)(void);
	unsigned char (*readbuf)(unsigned char *, unsigned int, unsigned char);
	unsigned char(*writebuf)(const unsigned char *, unsigned int, unsigned char);
	unsigned char(*getcardinfo)(SD_CardInfo *);
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
int MAL_GetCardInfo();
int MAL_Read(unsigned char *buff, unsigned int sector, unsigned char count);
int MAL_Write(const unsigned char *buff, unsigned int sector, unsigned char count);
int MMC_disk_ioctl(unsigned ctrl, void *buff);
int NOP(void);
#endif /* __MASS_MAL_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
