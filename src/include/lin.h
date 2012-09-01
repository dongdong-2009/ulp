/*
 *  David@2012 initial version
 *  Only support Unconditional frame
 */
#ifndef LIN_H
#define LIN_H

#define LIN_MAX_DATA		8              // Max bytes lin frame data

#define LIN_CFG_DEF { \
	.baud = 9600, \
	.option = 0, \
	.pid_callback = NULL, \
}

typedef enum {
	LIN_IDLE,
	LIN_BREAK,
	LIN_SYNC,
	LIN_PID,
	LIN_RECEIVING,
	LIN_TRANSMITING,
	LIN_CHECKSUM,
	LIN_ERROR,
} lin_state_t;

typedef enum {
	ERROR_NONE,
	ERROR_BREAK,
	ERROR_SYNC,
	ERROR_PID,
	ERROR_CS,
} lin_status_t;

typedef enum {
	LIN_SLAVE_RX_FRAME,
	LIN_SLAVE_TX_FRAME,
} lin_frame_type;

enum {
	LIN_SLAVE_NODE,
	LIN_MASTER_NODE,
};

enum {
	LIN_SLAVE_TASK_NO_RESPONSE,
	LIN_SLAVE_TASK_RESPONSE,
};

typedef struct {
	unsigned char pid;
	unsigned char dlc;
	unsigned char data[LIN_MAX_DATA];
} lin_msg_t;

typedef struct {
	lin_msg_t msg;
	unsigned char cs;
	lin_status_t status;
	lin_state_t state;
} lin_frame_t;

typedef struct {
	long baud;
	int option;
	int node; //node type, LIN_SLAVE_NODE or LIN_MASTER_NODE
	int (*pid_callback)(unsigned char pid, lin_msg_t *p);
} lin_cfg_t;

typedef struct {
	int (*init)(lin_cfg_t * cfg);
	int (*recv)(lin_msg_t *pmsg);
	int (*send)(unsigned char pid);
	int (*ckstate)(void);
	int (*ckstatus)(void); //call this function will reset the status
} lin_bus_t;

extern lin_bus_t lin0;
extern lin_bus_t lin1;

#endif
