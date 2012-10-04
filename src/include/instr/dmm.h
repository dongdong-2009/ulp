/*
 *  miaofng@2012 initial version
 */

#ifndef __DMM_H_
#define __DMM_H_

#include "instr/instr.h"

struct dmm_s {
	struct instr_s *instr;
};

enum {
	DMM_CMD_CONFIG,
	DMM_CMD_READ, /*INBOX: CMD BYTES CRC16 IMG0 IMG1 ..., OUTBOX: CMD ECODE*/
};

struct dmm_mailbox_s {
	unsigned char cmd;
	union {
		struct {
			unsigned char bytes;
			unsigned short crc16;
		} set_image;

		/*outbox*/
		struct {
			unsigned char status;
			union {
				struct {
					unsigned char bus;
					unsigned char row;
				} get_size;
			};
		};
	};
};

struct dmm_s* dmm_open(const char *uuid);
int dmm_select(struct dmm_s *dmm_new);
int dmm_read(int ch);
void dmm_close(void);

#endif /* __DMM_H_ */
