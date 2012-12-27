/*
 *  miaofng@2012 initial version
 */

#ifndef __MATRIX_H_
#define __MATRIX_H_

#include "instr/instr.h"

struct matrix_s {
	unsigned char bus;
	unsigned char row;
	unsigned char image_bytes;
	void *image;
	struct instr_s *instr;
};

enum {
	MATRIX_CMD_GET_SIZE = INSTR_CMD_END,
	MATRIX_CMD_SET_IMAGE,
};

/* each instrument can only be opened once
 * name  could be dmm0(class_name + index), xxx(uuid),
 *       or yyy[:0](name[:index]), or null
 */
struct matrix_s* matrix_open(const char *name);
int matrix_select(struct matrix_s *matrix_new);
int matrix_reset(void);
int matrix_connect(int bus, int row);
int matrix_execute(void);
void matrix_close(void);

#endif /* __MATRIX_H_ */
