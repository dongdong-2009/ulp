#ifndef __MATRIX_H_
#define __MATRIX_H_

#include "spi.h"

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
void matrix_handler(unsigned char cmd,char *pdata);



#endif /*__MATRIX_H_*/
