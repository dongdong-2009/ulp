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
#include "ulp_time.h"
#include <string.h>
#include "ulp/debug.h"

static const can_bus_t *ccp_can = NULL;
static char ccp_ctr = 0; //command counter

int ccp_Init(const can_bus_t *can, int baud)
{
	ccp_cro_t cro;
	ccp_can = can;
	assert(ccp_can != NULL);

	can_cfg_t cfg = CAN_CFG_DEF;
	cfg.baud = baud;
	ccp_can -> init(&cfg);
	
	cro.Params.Connect.wStnAddr = 0;
	return ccp_Connect(&cro);
}

static int ccp_dispatch(can_msg_t *msg, char waitflag, int timeout)
{
	int ret;
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

		ret |= (msg -> data[0] != (char) CCP_CMD_RTN_MSG);
		ret |= (msg -> data[1] != CCP_NO_ERR);
		ret |= (msg -> data[2] != ccp_ctr);
	}

	ccp_ctr ++;
	return ret;
}

/*Establish continuous logical point-to-point connection with the target. */
int ccp_Connect(const ccp_cro_t *cro)
{
	int status;
	can_msg_t msg;

	msg.data[0] = CCP_CONNECT;
	msg.data[1] = ccp_ctr;
	msg.data[2] = (char) (cro -> Params.Connect.wStnAddr >> 8);
	msg.data[3] = (char) (cro -> Params.Connect.wStnAddr);
	msg.dlc = 4;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	return status;
}

int ccp_SetMTA(const ccp_cro_t *cro)
{
	int status;
	can_msg_t msg;

	msg.data[0] = CCP_SET_MTA;
	msg.data[1] = ccp_ctr;
	msg.data[2] = cro -> Params.SetMTA.byMTA;
	msg.data[3] = cro -> Params.SetMTA.byAddrExt;
	msg.data[4] = (char) (cro -> Params.SetMTA.dwAddr >> 24);
	msg.data[5] = (char) (cro -> Params.SetMTA.dwAddr >> 16);
	msg.data[6] = (char) (cro -> Params.SetMTA.dwAddr >> 8);
	msg.data[7] = (char) (cro -> Params.SetMTA.dwAddr);
	msg.dlc = 8;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	return status;
}

int ccp_Dnload(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	assert(cro -> Params.Dnload.byByteCnt <= CCP_DNLOAD_MAX_BYTES);

	msg.data[0] = CCP_DNLOAD;
	msg.data[1] = ccp_ctr;
	msg.data[2] = cro -> Params.Dnload.byByteCnt;
	memcpy(&msg.data[3], cro -> Params.Dnload.pbyData, cro -> Params.Dnload.byByteCnt);
	msg.dlc = cro -> Params.Dnload.byByteCnt + 3;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	if(dto != NULL) {
		dto -> Params.Dnload.byAddrExt = msg.data[3];
		dto -> Params.Dnload.dwAddr = ((int) msg.data[4] << 24);
		dto -> Params.Dnload.dwAddr += ((int) msg.data[5] << 16);
		dto -> Params.Dnload.dwAddr += ((int) msg.data[6] << 8);
		dto -> Params.Dnload.dwAddr += ((int) msg.data[7]);
	}
	return status;
}

int ccp_Dnload6(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;

	msg.data[0] = CCP_DNLOAD_6;
	msg.data[1] = ccp_ctr;
	memcpy(&msg.data[2], cro -> Params.Dnload6.pbyData, 6);
	msg.dlc = 8;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	if(dto != NULL) {
		dto -> Params.Dnload6.byAddrExt = msg.data[3];
		dto -> Params.Dnload6.dwAddr = ((int) msg.data[4] << 24);
		dto -> Params.Dnload6.dwAddr += ((int) msg.data[5] << 16);
		dto -> Params.Dnload6.dwAddr += ((int) msg.data[6] << 8);
		dto -> Params.Dnload6.dwAddr += ((int) msg.data[7]);
	}
	return status;
}

int ccp_Upload(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	assert(cro -> Params.Upload.byByteCnt <= CCP_UPLOAD_MAX_BYTES);

	msg.data[0] = CCP_UPLOAD;
	msg.data[1] = ccp_ctr;
	msg.data[2] = cro -> Params.Upload.byByteCnt;
	msg.dlc = 3;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	if(dto != NULL) {
		memcpy(dto -> Params.Upload.pbyData, &msg.data[3], \
			cro -> Params.Upload.byByteCnt);
	}
	return status;
}

/*Upload a variable-size data block*/
int ccp_ShortUp(const ccp_cro_t *cro, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	assert(cro -> Params.ShortUp.byByteCnt <= CCP_UPLOAD_MAX_BYTES);

	msg.data[0] = CCP_SHORT_UP;
	msg.data[1] = ccp_ctr;
	msg.data[2] = cro -> Params.ShortUp.byByteCnt;
	msg.data[3] = cro -> Params.ShortUp.byAddrExt;
	msg.data[4] = (char) (cro -> Params.ShortUp.dwAddr >> 24);
	msg.data[5] = (char) (cro -> Params.ShortUp.dwAddr >> 16);
	msg.data[6] = (char) (cro -> Params.ShortUp.dwAddr >> 8);
	msg.data[7] = (char) (cro -> Params.ShortUp.dwAddr);
	msg.dlc = 8;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_25MS);
	if(dto != NULL) {
		memcpy(dto -> Params.ShortUp.pbyData, &msg.data[3], \
			cro -> Params.ShortUp.byByteCnt);
	}
	return status;
}

int ccp_ActionService(const ccp_cro_t *cro, int npara, char waitflag, ccp_dto_t *dto)
{
	int status;
	can_msg_t msg;
	assert(npara <= CCP_SVC_MAX_PARAMS);

	msg.data[0] = CCP_ACTION_SERVICE;
	msg.data[1] = ccp_ctr;
	msg.data[2] = cro -> Params.ActionService.bySvc;
	memcpy(&msg.data[3], cro -> Params.ActionService.pbyParams, npara);
	msg.dlc = npara + 3;

	// Send the command and receive the response.
	status = ccp_dispatch(&msg, 1, CCP_TIMEOUT_5000MS);
	if(waitflag && dto != NULL) {
		dto -> Params.ActionService.byParamCnt = msg.data[3];
		dto -> Params.ActionService.byType = msg.data[4];
	}
	return status;
}
