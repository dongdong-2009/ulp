/* START LIBRARY DESCRIPTION **************************************************
com_mcamOS.lib
*.
*.COPYRIGHT (c) 2005 Delphi Corporation, All Rights Reserved
*.
*.Security Classification: DELPHI CONFIDENTIAL
*.
*.Description: Provide mcamOS functions for Z-World's RCM3200 using Microchip's
*.             MCP2515 Stand-Alone CAN Controller With SPI Interface.
*.
*.Functions:
*.    mcamOSInit
*.    mcamOSDownload
*.    mcamOSUpload
*.    mcamOSExecute
*.    FormatCmdMsg
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
*.07MAR06  qz78ws  > Changed MCP2515.lib to hw_MCP2515.lib.
*.28FEB06  qz78ws  > Added CAN channel parameter.
*.10NOV05  qz78ws  > Changed wTimeout parameter to milli-seconds.
*.27OCT05  qz78ws  > Added 200kbaud settings.
*.                 > Moved config settings into mcamOSInit.
*.                 > Added wMCAMOSkBaud as pass parameter to mcamOSInit.
*.                 > Added return parameter to mcamOSInit.
*.21SEP05  qz78ws  > Added extern to CmdMsg declaration in module header.
*.05MAY05  qz78ws  > Original release.
*.
END DESCRIPTION ***************************************************************/

/*** BeginHeader */
#ifndef COM_MCAMOS_LIB
#define COM_MCAMOS_LIB
/*** EndHeader */


/*** BeginHeader */
#include "TypesPlus.h"
#include "ErrorCodes.h"
#include "priv/mcamos.h"
/*** EndHeader */

static inline ERROR_CODE mcamOSInit(int nCANChan, UINT16 wMCAMOSkBaud)
{
	nest_can_sel(nCANChan);
	return (ERROR_CODE) mcamos_init(&can1, wMCAMOSkBaud);
}

static inline ERROR_CODE mcamOSDownload(int nCANChan, UINT32 dwAddr, const UINT8 *pbySrc, UINT16 wByteCnt, UINT16 wTimeout)
{
	return (ERROR_CODE) mcamos_dnload(&can1, dwAddr, (const char *)pbySrc, wByteCnt, wTimeout);
}

static inline ERROR_CODE mcamOSUpload(int nCANChan, UINT8 *pbyDest, UINT32 dwAddr, UINT16 wByteCnt, UINT16 wTimeout)
{
	return (ERROR_CODE) mcamos_upload(&can1, dwAddr, (char *)pbyDest, wByteCnt, wTimeout);
}

static inline ERROR_CODE mcamOSExecute(int nCANChan, UINT32 dwAddr, UINT16 wTimeout)
{
	return (ERROR_CODE) mcamos_execute(&can1, dwAddr, wTimeout);
}

#endif
