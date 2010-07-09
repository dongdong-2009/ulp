/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "mass_mal.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/*-----------------------------------------------------------------------*/

#define ATA		1
#define MMC		0
#define USB		2



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_initialize();
		if (!result)
			stat = RES_OK;
		else
			stat = STA_NOINIT;
		return stat;

	case MMC :
		result = MMC_disk_initialize();
		if (!result) {
			stat = RES_OK;
		} else {
			stat = STA_NOINIT;
		}
		if(MAL_GetCardInfo())
			stat = STA_NOINIT;
		return stat;

	case USB :
		result = USB_disk_initialize();
		if (!result)
			stat = RES_OK;
		else
			stat = STA_NOINIT;
		return stat;
	}

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_status();
		if (!result)
			stat = RES_OK;
		else
			stat = STA_NOINIT;
		return stat;

	case MMC :
		result = MMC_disk_status();
		if (!result)
			stat = RES_OK;
		else
			stat = STA_NODISK;
		return stat;

	case USB :
		result = USB_disk_status();
		if (!result)
			stat = RES_OK;
		else
			stat = STA_NOINIT;
		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_read(buff, sector, count);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;

	case MMC :
		if(MMC_disk_status())
			return RES_NOTRDY;
		result = MMC_disk_read(buff, sector, count);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;

	case USB :
		result = USB_disk_read(buff, sector, count);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;
	}
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
/* The FatFs module will issue multiple sector transfer request
/  (count > 1) to the disk I/O layer. The disk function should process
/  the multiple sector transfer properly Do. not translate it into
/  multiple single sector transfers to the media, or the data read/write
/  performance may be drasticaly decreased. */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_write(buff, sector, count);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;

	case MMC :
		if(MMC_disk_status())
			return RES_NOTRDY;
		result = MMC_disk_write(buff, sector, count);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;

	case USB :
		result = USB_disk_write(buff, sector, count);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;
	}
	return RES_PARERR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		// pre-process here

		result = ATA_disk_ioctl(ctrl, buff);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;

	case MMC :
		// pre-process here

		result = MMC_disk_ioctl(ctrl, buff);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;

	case USB :
		// pre-process here

		result = USB_disk_ioctl(ctrl, buff);
		if (!result)
			res = RES_OK;
		else
			res = RES_ERROR;
		return res;
	}
	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* User defined function to give a current time to fatfs module          */
/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */                                                                                                                                                                                                                                                
DWORD get_fattime (void)
{
#if 1
	return	((2010UL-1980) << 25)	      // Year = 2006
			| (7UL << 21)	      // Month = 7
			| (9UL << 16)	      // Day = 9
			| (12U << 11)	      // Hour = 12
			| (0U << 5)	      // Min = 0
			| (0U >> 1)	      // Sec = 0
			;
#endif
}


