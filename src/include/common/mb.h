/*
 * 	miaofng@2013-9-30 initial version
 *	RS485 modbus protocol
 *
 */
#ifndef __MB_H_
#define __MB_H_

#include "config.h"
#include "uart.h"

#ifdef CONFIG_MB_FRAME_BYTES
#define MB_FRAME_BYTES CONFIG_MB_FRAME_BYTES
#else
#define MB_FRAME_BYTES 64
#endif

typedef enum {
	MB_RTU,
	MB_ASCII,
	MB_TCP
} mb_mode_t;

enum {
	MB_E_NONE = 0x00,
	MB_E_ILLEGAL_FUNCTION = 0x01, //no function
	MB_E_ILLEGAL_DATA_ADDRESS = 0x02, //no register
	MB_E_ILLEGAL_DATA_VALUE = 0x03, //nregs error
	MB_E_SLAVE_DEVICE_FAILURE = 0x04, //undefine error
	MB_E_ACKNOWLEDGE = 0x05,
	MB_E_SLAVE_BUSY = 0x06, //access timeout
	MB_E_MEMORY_PARITY_ERROR = 0x08,
};

#define MB_FUNC_NONE                          (  0 )
#define MB_FUNC_READ_COILS                    (  1 )
#define MB_FUNC_READ_DISCRETE_INPUTS          (  2 )
#define MB_FUNC_WRITE_SINGLE_COIL             (  5 )
#define MB_FUNC_WRITE_MULTIPLE_COILS          ( 15 )

#define MB_FUNC_READ_HOLDING_REGISTER         (  3 )
#define MB_FUNC_READ_INPUT_REGISTER           (  4 )
#define MB_FUNC_WRITE_REGISTER                (  6 )
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS      ( 16 )
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS  ( 23 )
#define MB_FUNC_DIAG_READ_EXCEPTION           (  7 )
#define MB_FUNC_DIAG_DIAGNOSTIC               (  8 )
#define MB_FUNC_DIAG_GET_COM_EVENT_CNT        ( 11 )
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG        ( 12 )
#define MB_FUNC_OTHER_REPORT_SLAVEID          ( 17 )
#define MB_FUNC_ERROR                         ( 128 )

/*modbus data frame is big endian except crc bytes
*/

struct mb_req_s {
	char device;
	char func;
	unsigned short addr;
	union {
		unsigned short nregs; /*note: 1 regs = 2 bytes*/
		unsigned short value;
	};
};

struct mb_rsp_s {
	char device;
	char func;
	union {
		char bytes;
		char ecode;
	};
};

int mb_init(uart_bus_t *bus, mb_mode_t mode, int addr, int baud);
int mb_update(void);

/*callback, should be provided by caller, note:
	!!!nregs=1 <=> 2bytes!!!
*/
void mb_bus_write_enable(int enable);
int mb_ireg_cb(int addr, char *buf, int nregs, int rd); //input register access
int mb_hreg_cb(int addr, char *buf, int nregs, int rd); //hold register access
int mb_eframe_cb(const char *buf, int bytes); //error frame handling

#endif /*__MB_H_*/
