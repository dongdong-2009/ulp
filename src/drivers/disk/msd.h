/******************** (C) COPYRIGHT 2009 STMicroelectronics ********************
* File Name          : msd.h
* Author             : MCD Application Team
* Version            : V3.1.0RC1
* Date               : 09/28/2009
* Description        : Header for msd.c file.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MSD_H
#define __MSD_H

/* Block Size */
#define BLOCK_SIZE    512

/* Dummy byte */
#define DUMMY   0xFF

/* Start Data tokens  */
/* Tokens (necessary because at nop/idle (and CS active) only 0xff is on the data/command line) */
#define MSD_START_DATA_SINGLE_BLOCK_READ 0xFE  /* Data token start byte, Start Single Block Read */
#define MSD_START_DATA_MULTIPLE_BLOCK_READ  0xFE  /* Data token start byte, Start Multiple Block Read */
#define MSD_START_DATA_SINGLE_BLOCK_WRITE 0xFE  /* Data token start byte, Start Single Block Write */
#define MSD_START_DATA_MULTIPLE_BLOCK_WRITE 0xFD  /* Data token start byte, Start Multiple Block Write */
#define MSD_STOP_DATA_MULTIPLE_BLOCK_WRITE 0xFD  /* Data toke stop byte, Stop Multiple Block Write */

/* MSD functions return */
#define MSD_SUCCESS       0x00
#define MSD_FAIL          0xFF

/* MSD reponses and error flags */
#define MSD_RESPONSE_NO_ERROR      0x00
#define MSD_IN_IDLE_STATE          0x01
#define MSD_ERASE_RESET            0x02
#define MSD_ILLEGAL_COMMAND        0x04
#define MSD_COM_CRC_ERROR          0x08
#define MSD_ERASE_SEQUENCE_ERROR   0x10
#define MSD_ADDRESS_ERROR          0x20
#define MSD_PARAMETER_ERROR        0x40
#define MSD_RESPONSE_FAILURE       0xFF

/* Data response error */
#define MSD_DATA_OK                0x05
#define MSD_DATA_CRC_ERROR         0x0B
#define MSD_DATA_WRITE_ERROR       0x0D
#define MSD_DATA_OTHER_ERROR       0xFF

/* Commands: CMDxx = CMD-number | 0x40 */
#define MSD_GO_IDLE_STATE          (0x40+0)   /* CMD0=0x40 */
#define	MSD_SEND_OP_COND           (0x40+1)   /* CMD1=0x41 */
#define	MSD_SEND_IF_COND           (0x40+8)   /* SEND_IF_COND */
#define MSD_SEND_CSD               (0x40+9)   /* CMD9=0x49 */
#define MSD_SEND_CID               (0x40+10)  /* CMD10=0x4A */
#define MSD_STOP_TRANSMISSION      (0x40+12)  /* CMD12=0x4C */
#define MSD_SEND_STATUS            (0x40+13)  /* CMD13=0x4D */
#define MSD_SET_BLOCKLEN           (0x40+16)  /* CMD16=0x50 */
#define MSD_READ_SINGLE_BLOCK      (0x40+17)  /* CMD17=0x51 */
#define MSD_READ_MULTIPLE_BLOCK    (0x40+18)  /* CMD18=0x52 */
#define MSD_SET_BLOCK_COUNT        (0x40+23)  /* CMD23=0x57 */
#define MSD_WRITE_BLOCK            (0x40+24)  /* CMD24=0x58 */
#define MSD_WRITE_MULTIPLE_BLOCK   (0x40+25)  /* CMD25=0x59 */
#define MSD_PROGRAM_CSD            (0x40+27)  /* CMD27=0x5B */
#define MSD_SET_WRITE_PROT         (0x40+28)  /* CMD28=0x5C */
#define MSD_CLR_WRITE_PROT         (0x40+29)  /* CMD29=0x5D */
#define MSD_SEND_WRITE_PROT        (0x40+30)  /* CMD30=0x5E */
#define MSD_TAG_SECTOR_START       (0x40+32)  /* CMD32=0x60 */
#define MSD_TAG_SECTOR_END         (0x40+33)  /* CMD33=0x61 */
#define MSD_UNTAG_SECTOR           (0x40+34)  /* CMD34=0x62 */
#define MSD_TAG_ERASE_GROUP_START  (0x40+35)  /* CMD35=0x63 */
#define MSD_TAG_ERASE_GROUP_END    (0x40+36)  /* CMD36=0x64 */
#define MSD_UNTAG_ERASE_GROUP      (0x40+37)  /* CMD37=0x65 */
#define MSD_ERASE                  (0x40+38)  /* CMD38=0x66 */
#define MSD_READ_OCR               (0x40+39)  /* CMD39=0x67 */
#define MSD_CRC_ON_OFF             (0x40+40)  /* CMD40=0x68 */
#define MSD_APP_CMD                (0x40+55)  /* APP_CMD */
#define MSD_APP_READ_OCR           (0x40+58)  /* APP_READ_OCR */
#define MSD_SEND_ACMD41            (0x40+41)  /* send app operation condition SDC*/

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/*----- High layer function -----*/
unsigned char MSD_Init(void);
unsigned char MSD_WriteBlock(unsigned char* pBuffer, unsigned WriteAddr, unsigned short NumByteToWrite);
unsigned char MSD_ReadBlock(unsigned char* pBuffer, unsigned ReadAddr, unsigned short NumByteToRead);
unsigned char MSD_WriteBuffer(const unsigned char* pBuffer, unsigned WriteAddr, unsigned char NbrOfBlock);
unsigned char MSD_ReadBuffer(unsigned char* pBuffer, unsigned ReadAddr, unsigned char NbrOfBlock);
unsigned char MSD_GetCSDRegister(SD_CSD* MSD_csd);
unsigned char MSD_GetCIDRegister(SD_CID* MSD_cid);
unsigned char MSD_GetCardInfo(SD_CardInfo * pSDCardInfo);

/*----- Medium layer function -----*/
void MSD_SendCmd(unsigned char Cmd, unsigned Arg, unsigned char Crc);
unsigned char MSD_GetResponse(unsigned char Response);
unsigned char MSD_GetDataResponse(void);
unsigned char MSD_GoIdleState(void);
unsigned short MSD_GetStatus(void);

/*----- Low layer function -----*/
void MSD_WriteByte(unsigned char byte);
unsigned char MSD_ReadByte(void);

#endif /* __MSD_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
