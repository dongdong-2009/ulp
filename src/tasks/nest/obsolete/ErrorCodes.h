/* START LIBRARY DESCRIPTION **************************************************
ErrorCodes.lib
*.
*.COPYRIGHT (c) 2005 Delphi Corporation, All Rights Reserved
*.
*.Security Classification: DELPHI CONFIDENTIAL
*.
*.Description: This library contains error and status code definitions.
*.
*.Functions: none
*.
*.Language: Z-World Dynamic C ( ANSII C with extensions and variations )
*.
*.Target Environment: Rabbit Semiconductor, Rabbit 3000 microprocessor used in
*.                    RCM3200 core module.
*.
*.Notes:
*. >
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE     USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.01FEB06  qz78ws  > Added E_EE_VERIFY.
*.31JAN06  qz78ws  > Added FIS Error Codes.
*.                 > Added Conditioning Error Codes.
*.11JAN06  qz78ws  > Added Upload Error Codes.
*.11NOV05  qz78ws  > Added E_MCU_CHECKSUM.
*.04NOV05  qz78ws  > Deleted two blank lines.
*.27OCT05  qz78ws  > Added E_COMM_ codes.
*.13MAY05  qz78ws  > Added E_OK, E_ERR, and E_OOPS at special request.
*.13APR05  qz78ws  > Original release.
*.
END DESCRIPTION ***************************************************************/

/*** BeginHeader */
#ifndef ERRORCODES_LIB
#define ERRORCODES_LIB
/*** EndHeader */


/*** BeginHeader */
#include "TypesPlus.h"
/*** EndHeader */

/*** BeginHeader */
// Error/Status Code Definitions
// Generic Codes
#define E_SUCCESS          (0x00)   // Successful execution.
#define E_OK               (0x00)   // Successful execution.
#define NEST_OOPS          (0xF0)   // Error due to nest
#define DUT_OOPS           (0xF1)   // Error due to product
#define E_UNDEFINED        (0xFF)   // Undefined status/error code.
#define E_ERR              (0xFF)   // Undefined status/error code.
#define E_OOPS             (0xFF)   // Undefined status/error code.
#define NEST_ERROR ( 0x8000 ) //Indicates if error is related to nest vs DUT

// Hardware Engine Error Codes
#define E_HW_DEFAULT       (0x10)   // Generic default error.
#define E_HW_PARAMETER     (0x11)   // Generic parameter error.
#define E_HW_COMMAND       (0x12)   // Invalid engine command.
#define E_HW_INIT          (0x13)   // Generic initialization error.
#define E_HW_SPITIME       (0x14)   // SPI comms timeout.
#define E_HW_SCITIME       (0x15)   // SCI comms timeout.
#define E_HW_SCIRX         (0x16)   // SCI receive error.
#define E_HW_SCITX         (0x17)   // SCI transmit error.
#define E_HW_SCIECHO       (0x18)   // SCI transmit and receive echo error.
#define E_HW_CANRX         (0x19)   // CAN receive error.
#define E_HW_CANTX         (0x1A)   // CAN transmit error.
#define E_HW_CANBUSY       (0x1B)   // CAN busy error.

// DUT-specific Engine Error Codes
#define E_DUT_DEFAULT      (0x20)   // Generic default error.
#define E_DUT_PARAMETER    (0x21)   // Generic parameter error.
#define E_DUT_COMMAND      (0x22)   // Invalid engine command.
#define E_DUT_INIT         (0x23)   // Generic initialization error.

// Test Engine Error Codes
#define E_TEST_DEFAULT     (0x30)   // Generic default error.
#define E_TEST_PARAMETER   (0x31)   // Generic parameter error.
#define E_TEST_COMMAND     (0x32)   // Invalid engine command.
#define E_TEST_INIT        (0x33)   // Generic initialization error.
#define E_TEST_TPUINITTIME (0x34)   // TPU3 init timeout.
#define E_TEST_TPUCHAN     (0x35)   // Invalid TPU3 channel.
#define E_TEST_MIOSCHAN    (0x36)   // Invalid MIOS1 channel.
#define E_TEST_CTMICTIME   (0x37)   // MIOS1 input capt timeout.
#define E_TEST_QADCTIME    (0x38)   // QADC64 timeout.

// Program Engine Error Codes
#define E_PROG_DEFAULT     (0x40)   // Generic default error.
#define E_PROG_PARAMETER   (0x41)   // Generic parameter error.
#define E_PROG_COMMAND     (0x42)   // Invalid engine command.
#define E_PROG_INIT        (0x43)   // Generic initialization error.

// Internal Flash Program Engine Error Codes
#define E_MCU_DEFAULT      (0x50)   // Generic default error.
#define E_MCU_PARAMETER    (0x51)   // Generic parameter error.
#define E_MCU_COMMAND      (0x52)   // Invalid engine command.
#define E_MCU_INIT         (0x53)   // Generic initialization error.
#define E_MCU_EXE          (0x54)   // Execution error.
#define E_MCU_ERASE        (0x55)   // Erase error.
#define E_MCU_BLANKCHECK   (0x56)   // Blank check error.
#define E_MCU_PROGRAM      (0x57)   // Program error.
#define E_MCU_CENSORSHIP   (0x58)   // Error setting censorship.
#define E_MCU_BYTECOUNT    (0x59)   // Byte count multiple incorrect.
#define E_MCU_LOCKED       (0x5A)   // Attempt to program locked block.
#define E_MCU_ID           (0x5B)   // ID mismatch error.
#define E_MCU_CHECKSUM     (0x5C)   // Checksumming error.

// Serial EEPROM Program Engine Error Codes
#define E_EE_DEFAULT       (0x60)   // Generic default error.
#define E_EE_PARAMETER     (0x61)   // Generic parameter error.
#define E_EE_COMMAND       (0x62)   // Invalid engine command.
#define E_EE_INIT          (0x63)   // Generic initialization error.
#define E_EE_PROGTIME      (0x64)   // Prog function timeout.
#define E_EE_PROGSTAT      (0x65)   // Programming function fail.
#define E_EE_BULKTIME      (0x66)   // Bulk prog func timeout.
#define E_EE_BULKSTAT      (0x67)   // Bulk prog function fail.
#define E_EE_BADLENGTH     (0x68)   // Byte count out of range.
#define E_EE_BADRANGE      (0x69)   // Address is out of range.
#define E_EE_VERIFY        (0x6A)   // Data verify error.

// External Flash Program Engine Error Codes
#define E_FLASH_DEFAULT    (0x70)   // Generic default error.
#define E_FLASH_PARAMETER  (0x71)   // Generic parameter error.
#define E_FLASH_COMMAND    (0x72)   // Invalid engine command.
#define E_FLASH_INIT       (0x73)   // Generic initialization error.
#define E_FLASH_VPP        (0x74)   // VPP error detected.
#define E_FLASH_ERASE      (0x75)   // Erase error.
#define E_FLASH_PROGRAM    (0x76)   // Timeout before program complete.
#define E_FLASH_ID         (0x77)   // Error reading Flash ID.
#define E_FLASH_ADDRESS    (0x78)   // Data pointer on odd address boundary.
#define E_FLASH_BYTECOUNT  (0x79)   // Byte count multiple incorrect.
#define E_FLASH_BUSY       (0x7A)   // Status register indicates "busy".
#define E_FLASH_LOCKED     (0x7B)   // Attempt to program locked block.
#define E_FLASH_CMD_SEQ    (0x7C)   // Invalid device command sequence.
#define E_FLASH_TIMEOUT    (0x7D)   // Timeout before operation complete.
#define E_FLASH_BLANKCHECK (0x7E)   // Blank check error.

// Main Health Co-processor Error Codes
#define E_MHC_DEFAULT      (0x80)   // Generic default error.
#define E_MHC_PARAMETER    (0x81)   // Generic parameter error.
#define E_MHC_COMMAND      (0x82)   // Invalid engine command.
#define E_MHC_INIT         (0x83)   // Generic initialization error.
#define E_MHC_ERASE        (0x84)   // Erase error.
#define E_MHC_PROGRAM      (0x85)   // Timeout before program complete.
#define E_MHC_ADDRESS      (0x86)   // Data pointer on odd address boundary.
#define E_MHC_BYTECOUNT    (0x87)   // Byte count multiple incorrect.
#define E_MHC_BUSY         (0x88)   // Status register indicates "busy".
#define E_MHC_LOCKED       (0x89)   // Attempt to program locked block.
#define E_MHC_CMD_SEQ      (0x8A)   // Invalid device command sequence.
#define E_MHC_TIMEOUT      (0x8B)   // Timeout before operation complete.
#define E_MHC_BLANKCHECK   (0x8C)   // Blank check error.
#define E_MHC_CHECKSUM     (0x8D)   // Checksumming error.

// Error codes related to PIC
#define E_NO_INIT_XON      (0x90)
#define E_SERPUTC          (0x91)
#define E_PIC_DWNLDR       (0x92) // HEX32 line length not 16 bytes, etc.
#define E_SERF_TX_TO       (0x93)
#define E_DONE_TO          (0x94)
#define E1_FILE_REG_RD     (0x95) // File register read error
#define E2_FILE_REG_RD     (0x96) // File register read error
#define E_RDA_VERIFY       (0x97) // File reg. read did not match expected value
#define E_RDB_VERIFY       (0x98) // File reg. read did not match expected value

// Error codes related to User Block
#define E_UB_INVALID_ADD   (0xA1)
#define E_UB_NOT_FOUND     (0xA2)
#define E_UB_FLASH_WR_ERR  (0xA3)

// Communication Engine Error Codes
#define E_COMM_DEFAULT     (0xB0)   // Generic default error.
#define E_COMM_PARAMETER   (0xB1)   // Generic parameter error.
#define E_COMM_COMMAND     (0xB2)   // Invalid engine command.
#define E_COMM_INIT        (0xB3)   // Generic initialization error.
#define E_COMM_CRC         (0xB4)   // CRC error.
#define E_COMM_INCOMPLETE  (0xB5)   // Incomplete message error.
#define E_COMM_TIMING      (0xB6)   // Bit or message timing error.
#define E_COMM_BREAK       (0xB7)   // Break receive error.
#define E_COMM_OVERRUN     (0xB8)   // Overrun error.
#define E_COMM_UNDERRUN    (0xB9)   // Underrun error.
#define E_COMM_HARDWARE    (0xBA)   // Hardware error.
#define E_COMM_ARBITRATION (0xBB)   // Arbitration error.
#define E_COMM_MISMATCH    (0xBC)   // Data mismatch error.

// Upload Error Codes
#define E_UPLD_DEFAULT     (0xC0)   // Generic upload error.
#define E_UPLD_DATA_FILE   (0xC1)   // Data file error.
#define E_UPLD_MCRC_FILE   (0xC2)   // MCRC file error.
#define E_UPLD_ADDR_FILE   (0xC3)   // Address file error.

// Factory Information System Error Codes
#define E_FIS_DEFAULT      (0xD0)   // Generic default error.
#define E_FIS_PARAMETER    (0xD1)   // Generic parameter error.
#define E_FIS_COMMAND      (0xD2)   // Invalid command.
#define E_FIS_INIT         (0xD3)   // Generic initialization error.
#define E_FIS_IP           (0xD4)   // IP address error.
#define E_FIS_TX           (0xD5)   // Transmit message error.
#define E_FIS_RX           (0xD6)   // Receive message error.
#define E_FIS_ID           (0xD7)   // ID mis-match error.
#define E_FIS_LOG          (0xD8)   // Data log error.
#define E_FIS_TIMEOUT      (0xD9)   // Timeout error.
#define E_FIS_CONFIG       (0xDA)   // Configuration error.
#define E_FIS_FAIL         (0xDB)   // FIS returned FAIL status.

// Conditioning Error Codes
#define E_CND_DEFAULT      (0xE0)   // Generic default error.
#define E_CND_PARAMETER    (0xE1)   // Generic parameter error.
#define E_CND_COMMAND      (0xE2)   // Invalid engine command.
#define E_CND_INIT         (0xE3)   // Generic initialization error.
#define E_CND_PSV          (0xE4)   // Prior step verification error.
#define E_CND_OC           (0xE5)   // Output cycling error.
#define E_CND_MEM_VERIFY   (0xE6)   // Memory verify error.
#define E_CND_PRE_OC_END (0xE7)     // Output cycling error.
#define E_INVALID_BASE_MODEL_ID (0xE8)  // Invalid Base Model ID
#define E_CND_ADC_START_THW  (0x30)   // ADC VREF ERROR
#define E_CND_ADC_START_TH01 (0x31)   // ADC VREF ERROR
#define E_CND_ADC_START_THA  (0x32)   // ADC VREF ERROR
#define E_CND_ADC_START_VTA1 (0x33)   // ADC VREF ERROR
#define E_CND_ADC_START_VTA2 (0x34)   // ADC VREF ERROR
#define E_CND_ADC_START_PPMP (0x35)   // ADC VREF ERROR
#define E_CND_ADC_START_VG   (0x36)   // ADC VREF ERROR
#define E_CND_ADC_START_VPA  (0x37)   // ADC VREF ERROR
#define E_CND_ADC_START_VPA2 (0x38)   // ADC VREF ERROR

#define E_CND_ADC_END_THW  (0x40)   // ADC VREF ERROR
#define E_CND_ADC_END_TH01 (0x41)   // ADC VREF ERROR
#define E_CND_ADC_END_THA  (0x42)   // ADC VREF ERROR
#define E_CND_ADC_END_VTA1 (0x43)   // ADC VREF ERROR
#define E_CND_ADC_END_VTA2 (0x44)   // ADC VREF ERROR
#define E_CND_ADC_END_PPMP (0x45)   // ADC VREF ERROR
#define E_CND_ADC_END_VG   (0x46)   // ADC VREF ERROR
#define E_CND_ADC_END_VPA  (0x47)   // ADC VREF ERROR
#define E_CND_ADC_END_VPA2 (0x48)   // ADC VREF ERROR

#define E_CND_FAULT_REFERENCE_MISMATCH (0x49) // Fault data mismatch with reference.

/*** EndHeader */


/*** BeginHeader */
typedef UINT8 ERROR_CODE;
typedef UINT8 HW_ERROR;
typedef UINT8 TEST_ERROR;
typedef UINT8 PROG_ERROR;
typedef UINT8 MCU_ERROR;
typedef UINT8 EE_ERROR;
typedef UINT8 FLASH_ERROR;
typedef UINT8 DSS_ERROR;
typedef UINT8 MHC_ERROR;
typedef UINT8 CAN_ERROR;
/*** EndHeader */


/*** BeginHeader */
#endif   // ERRORCODES_LIB
/*** EndHeader */