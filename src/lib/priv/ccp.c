/*
* CCP communication protocol routine
*
* The CAN Calibration Protocol is part of the ASAP standards. It was developed and
* introduced by Ingenieurburo Helmut Kleinknecht, a manufacturer of calibration systems,
* and is used in several applications in the automative industry. The CCP was taken over by
* ASAP working group and enhanced with optional functions.
*
* The CCP requires at least two message objects, one for each direction: Command Receive
* Object(CRO) and Data Transmission Object(DTO). The CRO is used for reception of command
* codes and related parameters. The reception of a command has to be prompted with a handshake
* message using Data Transmission Object, and in this case a DTO is called a Command Return
* Message. The return code of this DTO message is used to determing whether the corresponding
* command has been successfully completed or not.
*
* A CRO is sent from the master device of one of the slave devices. The slave device answers with a
* DTO containing a CRM(command return message). The data length code of the CRO must always be
* 8. Unused data bytes may have arbitrary value. byte 0 is command code, and byte 1 is command counter.
*
*
* miaofng@2011 initial version
*/

#include "config.h"
#include "err.h"
#include "priv/ccp.h"
#include "can.h"
#include "time.h"
#include <string.h>
#include "debug.h"

static const can_bus_t *ccp_can = NULL;
static char ccp_ctr = 0; //command counter

int ccp_Init(const can_bus_t *can, int baud)
{
	ccp_can = can;
	assert(ccp_can != NULL);

	can_cfg_t cfg = CAN_CFG_DEF;
	cfg.baud = baud;
	return ccp_can -> init(&cfg);
}

static int ccp_dispatch(can_msg_t *msg, char waitflag, int timeout)
{
	int ret;
	ccp_dto_t *dto = (ccp_dto_t *) msg -> data;
	time_t deadline = time_get(timeout);
	assert(ccp_can != NULL);

	//send CRO
	msg -> id = CCP_CRO_ID;
	msg -> flag = 0;
	do {
		ret = ccp_can -> send(msg);
		if(time_left(deadline) < 0)
			return -ERR_TIMEOUT;
	} while(ret);

	//recv DTO?
	if(waitflag) {
		do {
			ret = ccp_can -> recv(msg);
			if(time_left(deadline) < 0)
				return -ERR_TIMEOUT;
		} while(ret);

		ret |= (dto -> byPID != (char) CCP_CMD_RTN_MSG);
		ret |= (dto -> byERR != CCP_NO_ERR);
		ret |= (dto -> byCTR != ccp_ctr);
	}

	ccp_ctr ++;
	return ret;
}

/*Establish continuous logical point-to-point connection with the target. */
int ccp_Connect(const ccp_cro_t *cro)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;

	crolib -> byCMD = CCP_CONNECT;
	crolib -> byCTR = ccp_ctr;
	crolib -> Params.pbyData[0] = (char) (cro -> Params.Connect.wStnAddr >> 8);
	crolib -> Params.pbyData[1] = (char) (cro -> Params.Connect.wStnAddr);
	msg.dlc = sizeof(crolib -> Params.Connect) + 2;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	return status;
}

int ccp_SetMTA(const ccp_cro_t *cro)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;

	crolib -> byCMD = CCP_SET_MTA;
	crolib -> byCTR = ccp_ctr;
	crolib -> Params.pbyData[0] = cro -> Params.SetMTA.byMTA;
	crolib -> Params.pbyData[1] = cro -> Params.SetMTA.byAddrExt;
	crolib -> Params.pbyData[2] = (char) (cro -> Params.SetMTA.dwAddr >> 24);
	crolib -> Params.pbyData[3] = (char) (cro -> Params.SetMTA.dwAddr >> 16);
	crolib -> Params.pbyData[4] = (char) (cro -> Params.SetMTA.dwAddr >> 8);
	crolib -> Params.pbyData[5] = (char) (cro -> Params.SetMTA.dwAddr);
	msg.dlc = sizeof(crolib -> Params.SetMTA) + 2;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	return status;
}

int ccp_Dnload(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;
	ccp_dto_t *dtolib = (ccp_dto_t *) msg.data;
	assert(cro -> Params.Dnload.byByteCnt <= CCP_DNLOAD_MAX_BYTES);

	crolib -> byCMD = CCP_DNLOAD;
	crolib -> byCTR = ccp_ctr;
	crolib -> Params.pbyData[0] = cro -> Params.Dnload.byByteCnt;
	memcpy(&crolib -> Params.pbyData[1], cro -> Params.Dnload.pbyData, cro -> Params.Dnload.byByteCnt);
	msg.dlc = cro -> Params.Dnload.byByteCnt + 3;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	dto -> Params.Dnload.byAddrExt = dtolib -> Params.pbyData[0];
	dto -> Params.Dnload.dwAddr = ((int) dtolib -> Params.pbyData[1] << 24);
	dto -> Params.Dnload.dwAddr += ((int) dtolib -> Params.pbyData[2] << 16);
	dto -> Params.Dnload.dwAddr += ((int) dtolib -> Params.pbyData[3] << 8);
	dto -> Params.Dnload.dwAddr += ((int) dtolib -> Params.pbyData[4]);
	return status;
}

int ccp_Dnload6(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;
	ccp_dto_t *dtolib = (ccp_dto_t *) msg.data;

	crolib -> byCMD = CCP_DNLOAD_6;
	crolib -> byCTR = ccp_ctr;
	memcpy(&crolib -> Params.pbyData[0], cro -> Params.Dnload6.pbyData, sizeof(crolib -> Params.pbyData));
	msg.dlc = sizeof(crolib -> Params.pbyData) + 2;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	dto -> Params.Dnload6.byAddrExt = dtolib -> Params.pbyData[0];
	dto -> Params.Dnload6.dwAddr = ((int) dtolib -> Params.pbyData[1] << 24);
	dto -> Params.Dnload6.dwAddr += ((int) dtolib -> Params.pbyData[2] << 16);
	dto -> Params.Dnload6.dwAddr += ((int) dtolib -> Params.pbyData[3] << 8);
	dto -> Params.Dnload6.dwAddr += ((int) dtolib -> Params.pbyData[4]);
	return status;
}

int ccp_Upload(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;
	ccp_dto_t *dtolib = (ccp_dto_t *) msg.data;
	assert(cro -> Params.Upload.byByteCnt <= CCP_UPLOAD_MAX_BYTES);

	crolib -> byCMD = CCP_UPLOAD;
	crolib -> byCTR = ccp_ctr;
	crolib -> Params.pbyData[0] = cro -> Params.Upload.byByteCnt;
	msg.dlc = sizeof(crolib -> Params.Upload) + 2;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	memcpy(dto -> Params.Upload.pbyData, &dtolib -> Params.pbyData[0], cro -> Params.Upload.byByteCnt);
	return status;
}

/*Upload a variable-size data block*/
int ccp_ShortUp(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;
	ccp_dto_t *dtolib = (ccp_dto_t *) msg.data;
	assert(cro -> Params.ShortUp.byByteCnt <= CCP_UPLOAD_MAX_BYTES);

	crolib -> byCMD = CCP_SHORT_UP;
	crolib -> byCTR = ccp_ctr;
	crolib -> Params.pbyData[0] = cro -> Params.ShortUp.byByteCnt;
	crolib -> Params.pbyData[1] = cro -> Params.ShortUp.byAddrExt;
	crolib -> Params.pbyData[2] = (char) (cro -> Params.ShortUp.dwAddr >> 24);
	crolib -> Params.pbyData[3] = (char) (cro -> Params.ShortUp.dwAddr >> 16);
	crolib -> Params.pbyData[4] = (char) (cro -> Params.ShortUp.dwAddr >> 8);
	crolib -> Params.pbyData[5] = (char) (cro -> Params.ShortUp.dwAddr);
	msg.dlc = sizeof(crolib -> Params.ShortUp) + 2;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	memcpy(dto -> Params.ShortUp.pbyData, &dtolib -> Params.pbyData[0], cro -> Params.ShortUp.byByteCnt);
	return status;
}

int ccp_ActionService(const ccp_cro_t *cro, int npara, char waitflag, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	ccp_cro_t *crolib = (ccp_cro_t *) msg.data;
	ccp_dto_t *dtolib = (ccp_dto_t *) msg.data;
	assert(npara <= CCP_SVC_MAX_PARAMS);

	crolib -> byCMD = CCP_ACTION_SERVICE;
	crolib -> byCTR = ccp_ctr;
	crolib -> Params.ActionService.bySvc = cro -> Params.ActionService.bySvc;
	memcpy(crolib -> Params.ActionService.pbyParams, cro -> Params.ActionService.pbyParams, npara);
	msg.dlc = npara + 3;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_5000MS);
	if(waitflag) {
		dto -> Params.ActionService.byParamCnt = dtolib -> Params.pbyData[0];
		dto -> Params.ActionService.byType = dtolib -> Params.pbyData[1];
	}
	return status;
}
