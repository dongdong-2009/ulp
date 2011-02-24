/*
 * 	miaofng@2011 initial version
 *
 */
#ifndef __CCP_H_
#define __CCP_H_

#include "config.h"
#include "can.h"

// Message type IDs.
#define CCP_CRO_ID               (0x7E0)
#define CCP_DTO_ID               (0x7E4)

// CCP command codes.
#define CCP_CONNECT              (0x01)
#define CCP_GET_CCP_VERSION      (0x1B) // Not implemented.
#define CCP_EXCHANGE_ID          (0x17) // Not implemented.
#define CCP_GET_SEED             (0x12) // Not implemented.
#define CCP_UNLOCK               (0x13) // Not implemented.
#define CCP_SET_MTA              (0x02)
#define CCP_DNLOAD               (0x03)
#define CCP_DNLOAD_6             (0x23)
#define CCP_UPLOAD               (0x04)
#define CCP_SHORT_UP             (0x0F)
#define CCP_SELECT_CAL_PAGE      (0x11) // Not implemented.
#define CCP_GET_DAQ_SIZE         (0x14) // Not implemented.
#define CCP_SET_DAQ_PTR          (0x15) // Not implemented.
#define CCP_WRITE_DAQ            (0x16) // Not implemented.
#define CCP_START_STOP           (0x06) // Not implemented.
#define CCP_DISCONNECT           (0x07) // Not implemented.
#define CCP_SET_S_STATUS         (0x0C) // Not implemented.
#define CCP_GET_S_STATUS         (0x0D) // Not implemented.
#define CCP_BUILD_CHKSUM         (0x0E) // Not implemented.
#define CCP_CLEAR_MEMORY         (0x10) // Not implemented.
#define CCP_PROGRAM              (0x18) // Not implemented.
#define CCP_PROGRAM_6            (0x22) // Not implemented.
#define CCP_MOVE                 (0x19) // Not implemented.
#define CCP_TEST                 (0x05) // Not implemented.
#define CCP_GET_ACTIVE_CAL_PAGE  (0x09) // Not implemented.
#define CCP_START_STOP_ALL       (0x08) // Not implemented.
#define CCP_DIAG_SERVICE         (0x20) // Not implemented.
#define CCP_ACTION_SERVICE       (0x21)

#define CCP_DISC_TEMP            (0)    // Temporary disconnect.
#define CCP_DISC_EOS             (1)    // End-of-session disconnect.

#define CCP_START                (0)    // Start data transmission.
#define CCP_STOP                 (1)    // Stop data transmission.

#define CCP_DNLOAD_MAX_BYTES     (5)    // Max bytes of DNLOAD data.
#define CCP_UPLOAD_MAX_BYTES     (5)    // Max bytes of UPLOAD data.
#define CCP_PROGRAM_MAX_BYTES    (5)    // Max bytes of PROGRAM data.
#define CCP_SVC_MAX_PARAMS       (5)    // Max bytes of DIAG and ACTION SERVICE params.

// CCP timeouts.
#define CCP_TIMEOUT_25MS         (500)   // Pradeep Modified during test with CAN analyser..
#define CCP_TIMEOUT_5000MS       (5000)

// CCP response codes.
#define CCP_CMD_RTN_MSG          (0xFF) // DTO PID indicates Command Return Message.
#define CCP_EVENT_MSG            (0xFE) // DTO PID indicates Event Message.
#define CCP_NO_ERR               (0x00) // DTO ERR indicates no error.

// Command Receive Object
typedef struct {
   char byCMD;               // Command code.
   char byCTR;               // Command counter.
   union {
      struct {
         short wStnAddr;     // Station address.
      } Connect;
      struct {
         char  byVersion;    // Main protocol version desired.
         char  byRelease;    // Release within version desired.
      } GetCCPVersion;
      struct {
         char  pbyID[6];     // Master device ID info (up to 6 bytes).
      } ExchangeID;
      struct {
         char  byPriv;       // Requested master privilege level.
      } GetSeed;
      struct {
         int dwKey;        // Key for corresponding seed.
      } Unlock;
      struct {
         char  byMTA;        // MTA number.
         char  byAddrExt;    // Address extension.
         int dwAddr;       // Base address.
      } SetMTA;
      struct {
         char  byByteCnt;    // Size of data block in bytes (<= 5).
         char  pbyData[CCP_DNLOAD_MAX_BYTES];
      } Dnload; // To MTA0.
      struct {
         char  pbyData[6];   // Data to be transferred (6 bytes).
      } Dnload6; // To MTA0.
      struct {
         char  byByteCnt;    // Size of data block in bytes (<= 5).
      } Upload; // From MTA0.
      struct {
         char  byByteCnt;    // Size of data block in bytes (<= 5).
         char  byAddrExt;    // Source address extension.
         int dwAddr;       // Source address.
      } ShortUp;
      struct {
         char  byDAQ;        // DAQ list number.
         char  byDummy;      // Dummy byte (prevent pack issues).
         int dwID;         // ID of DTO dedicated to list number.
      } GetDAQSize;
      struct {
         char  byDAQ;        // DAQ list number.
         char  byODT;        // Object descriptor table (ODT) number.
         char  byElement;    // Element within ODT.
      } SetDAQPtr;
      struct {
         char  byByteCnt;    // Size of DAQ element in bytes (1,2,4).
         char  byAddrExt;    // Address extension.
         int dwAddr;       // Base address of DAQ element.
      } WriteDAQ;
      struct {
         char  byAction;     // Start (0x00) or stop (0x01) data transmission.
         char  byDAQ;        // DAQ list number.
         char  byLast;       // Last data packet number to transmit.
         char  byIntvl;      // Desired line trans interval (ms) for DAQ list.
      } StartStop;
      struct {
         char  byDuration;   // Temporary (0x00) or end-of-session (0x01).
         char  byDummy;      // Dummy byte (prevent pack issues).
         short wSlvAddr;     // Slave device address.
      } Disconnect;
      struct {
         char  byStat;       // Session status bits.
      } SetSStatus;
      struct {
         int dwByteCnt;    // Block size in bytes.
      } BuildChksum; // Of MTA0.
      struct {
         int dwByteCnt;    // Block size in bytes.
      } ClearMemory; // Of MTA0???
      struct {
         char  byByteCnt;    // Size of data block in bytes (<= 5).
         char  pbyData[CCP_PROGRAM_MAX_BYTES];
      } Program; // To MTA0.
      struct {
         char  pbyData[6];   // Data to be programmed (6 bytes).
      } Program6; // To MTA0.
      struct {
         int dwByteCnt;    // Block size in bytes.
      } Move; // From MTA0 to MTA1.
      struct {
         short wStnAddr;     // Station address.
      } Test;
      struct {
         char  byAction;     // Start (0x00) or stop (0x01) data transmission.
      } StartStopAll;
      struct {
         char  bySvc;        // Diagnostic service number.
         char  pbyParams[CCP_SVC_MAX_PARAMS];
      } DiagService;
      struct {
         char  bySvc;        // Action service number
         char  pbyParams[CCP_SVC_MAX_PARAMS];
      } ActionService;
      char pbyData[6];       // Generic parameter data ref (up to 6 bytes).
   } Params;
} CCP_CRO_MSG_T, ccp_cro_t;

// Data Transmission Object (Command Return or Event Messages)
typedef struct {
   char byPID;               // Packet ID.
   char byERR;               // Command return code.
   char byCTR;               // Command counter.
   union {
      struct {
         char  byVersion;    // Main protocol version.
         char  byRelease;    // Release within version.
      } GetCCPVersion;
      struct {
         char  byByteCnt;    // Length of slave ID in bytes.
         char  byType;       // Data type qualifier.
      } ExchangeID;
      struct {
         char  byStat;       // Protection status (TRUE, FALSE).
         int dwSeed;       // Seed data.
      } GetSeed;
      struct {
         char  byPriv;       // Master privilege level.
      } Unlock;
      struct {
         char  byAddrExt;    // MTA0 address extension.
         int dwAddr;       // MTA0 address (after post-increment).
      } Dnload;
      struct {
         char  byAddrExt;    // MTA0 address extension.
         int dwAddr;       // MTA0 address (after post-increment).
      } Dnload6;
      struct {
         char  pbyData[CCP_UPLOAD_MAX_BYTES];
      } Upload;
      struct {
         char  pbyData[CCP_UPLOAD_MAX_BYTES];
      } ShortUp;
      struct {
         char  bySize;       // DAQ list size (in number of full size ODTs).
         char  byPID;        // First PID of DAQ list.
      } GetDAQSize;
      struct {
         char  byStat;       // Session status bits.
      } GetSStatus;
      struct {
         char  byChecksum;   // Checksum of memory block.
      } BuildChksum;
      struct {
         char  byAddrExt;    // MTA0 address extension.
         int dwAddr;       // MTA0 address (after post-increment).
      } Program;
      struct {
         char  byAddrExt;    // MTA0 address extension.
         int dwAddr;       // MTA0 address (after post-increment).
      } Program6;
      struct {
         char  byAddrExt;    // Address extension.
         int dwAddr;       // Base address of calibration page.
      } GetActiveCalPage;
      struct {
         char  byParamCnt;   // Length of return information in bytes.
         char  byType;       // Data type qualifier.
      } DiagService; // To MTA0.
      struct {
         char  byParamCnt;   // Length of return information in bytes.
         char  byType;       // Data type qualifier.
      } ActionService; // To MTA0.
      char pbyData[5];       // Generic parameter data ref (up to 5 bytes).
   } Params;
} CCP_DTO_MSG_T, ccp_dto_t;

/* api i/f policy:
* 1, api caller should fill CRO.Params.<cmd_name> as input, then get response from DTO
* 2, api will fill can_msg_t.data with the data from cro and can_msg_t.dlc, BTW take care of data alignment issue
* 3, ccp_dispatch() will do the real CAN packet i/o operations
*/
int ccp_Init(const can_bus_t *can, int baud);
int ccp_Connect(const ccp_cro_t *cro);
int ccp_SetMTA(const ccp_cro_t *cro);
int ccp_Dnload(const ccp_cro_t *cro, ccp_dto_t *dto);
int ccp_Dnload6(const ccp_cro_t *cro, ccp_dto_t *dto);
int ccp_Upload(const ccp_cro_t *cro, ccp_dto_t *dto);
int ccp_ShortUp(const ccp_cro_t *cro, ccp_dto_t *dto);
int ccp_ActionService(const ccp_cro_t *cro, int npara, char waitflag, ccp_dto_t *dto);

#endif /*__CCP_H_*/
