#ifndef __MATRIX_H_
#define __MATRIX_H_

#include "spi.h"

//Error define
#define ERROR_OK					0
#define ERROR_TIMEOUT				-1
#define ERROR_BOARD_NOT_EXIST		-2
#define ERROR_CHANNEL_NOT_EXIST		-3
#define ERROR_BUSSW_NOT_EXIST		-4
#define ERROR_CHSW_NOT_EXIST		-5

typedef struct {
	const spi_bus_t *bus;
	int idx; //index of chip in the specified bus
} matrix_t;

typedef struct
{
	char bMode;
	int iBoard;
	int iChannel;
	int iBus;
	char bValue;
	char bBus;
	char bChannel;
	char log[128]; //max 64*8 relays
} DiagVar_Type;

typedef struct
{
	char bExist;
	int iLen; //shift register chain length
	int iNrChannel;
	int iNrBus;
	char bBusSWExist;
	char bChSWExist;
	char strDescription[32];
} bdInfo_t;

enum{
	MATRIX_CMD_DISCONNECT = 0,
	MATRIX_CMD_CONNECT,
	MATRIX_CMD_DISCONNECTALL
};

int matrix_init();
int matrix_handler(unsigned char cmd,char *pdata);

#endif /*__MATRIX_H_*/
