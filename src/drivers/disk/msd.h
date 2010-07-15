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

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "mass_mal.h"

/* Pins related Hardware Configuration defines */
#if defined (USE_STM3210B_EVAL)

#define MSD_SPI                       SPI1
#define MSD_SPI_PORT                  GPIOA
#define MSD_SPI_GPIO_PORT_CLOCK       RCC_APB2Periph_GPIOA
#define MSD_SPI_PIN_SCK               GPIO_Pin_5
#define MSD_SPI_PIN_MISO              GPIO_Pin_6
#define MSD_SPI_PIN_MOSI              GPIO_Pin_7
#define MSD_CS_PORT                   GPIOA
#define MSD_CS_GPIO_PORT_CLOCK        RCC_APB2Periph_GPIOA
#define MSD_CS_PIN                    GPIO_Pin_4


#elif defined (USE_STM3210E_EVAL)

/* Uncomment this line to enable remapping of SPI pins */
//#define SPI_REMAPPED 

#define MSD_SPI                       0
#define MSD_SPI_PORT                  0
#define MSD_SPI_GPIO_PORT_CLOCK       0
#define MSD_SPI_PIN_SCK               0
#define MSD_SPI_PIN_MISO              0
#define MSD_SPI_PIN_MOSI              0
#define MSD_SPI_GPIO_REMAP            0
#define MSD_CS_PORT                   0
#define MSD_CS_GPIO_PORT_CLOCK        0
#define MSD_CS_PIN                    0

#elif defined (USE_STM3210C_EVAL)

/* Uncomment this line to enable remapping of SPI pins */
#define SPI_REMAPPED 

#define MSD_SPI                       SPI3
#define MSD_SPI_PORT                  GPIOC
#define MSD_SPI_GPIO_PORT_CLOCK       RCC_APB2Periph_GPIOC
#define MSD_SPI_PIN_SCK               GPIO_Pin_10
#define MSD_SPI_PIN_MISO              GPIO_Pin_11
#define MSD_SPI_PIN_MOSI              GPIO_Pin_12
#define MSD_SPI_GPIO_REMAP            GPIO_Remap_SPI3
#define MSD_CS_PORT                   GPIOA
#define MSD_CS_GPIO_PORT_CLOCK        RCC_APB2Periph_GPIOA
#define MSD_CS_PIN                    GPIO_Pin_4

#endif /* USE_STM3210B_EVAL */

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
uint8_t MSD_Init(void);
uint8_t MSD_WriteBlock(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
uint8_t MSD_ReadBlock(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint8_t MSD_WriteBuffer(const uint8_t* pBuffer, uint32_t WriteAddr, uint8_t NbrOfBlock);
uint8_t MSD_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint8_t NbrOfBlock);
uint8_t MSD_GetCSDRegister(SD_CSD* MSD_csd);
uint8_t MSD_GetCIDRegister(SD_CID* MSD_cid);
uint8_t MSD_GetCardInfo(SD_CardInfo * pSDCardInfo);

/*----- Medium layer function -----*/
void MSD_SendCmd(uint8_t Cmd, uint32_t Arg, uint8_t Crc);
uint8_t MSD_GetResponse(uint8_t Response);
uint8_t MSD_GetDataResponse(void);
uint8_t MSD_GoIdleState(void);
uint16_t MSD_GetStatus(void);

/*----- Low layer function -----*/
void MSD_WriteByte(uint8_t byte);
uint8_t MSD_ReadByte(void);

#endif /* __MSD_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
