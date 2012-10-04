/*
 * 	miaofng@2012 initial version
 *	- set relay image mode is used only
 *
 */

#include "ulp/sys.h"
#include "instr/instr.h"
#include "instr/matrix.h"
#include "common/bitops.h"
#include "crc.h"

#define MBSZ 64
static struct matrix_s *matrix = NULL;
static unsigned char matrix_mailbox[MBSZ];

static int matrix_cmd_get_size(void)
{
	int ecode;
	matrix_mailbox[0] = MATRIX_CMD_GET_SIZE;
	ecode = instr_send(matrix_mailbox, 1, 10);
	if(ecode == 0) {
		ecode = instr_recv(matrix_mailbox, 4, 10);
		if(ecode == 0) {
			matrix->bus = matrix_mailbox[2];
			matrix->row = matrix_mailbox[3];
			return matrix_mailbox[1];
		}
	}
	return -1;
}

static int matrix_cmd_set_image(void)
{
	int ecode, crc16;

	crc16 = cyg_crc16(matrix->image, matrix->image_bytes);
	matrix_mailbox[0] = MATRIX_CMD_SET_IMAGE;
	matrix_mailbox[1] = matrix->image_bytes;
	matrix_mailbox[2] = (unsigned char)(crc16 >> 8);
	matrix_mailbox[3] = (unsigned char)(crc16 >> 0);
	memcpy(&matrix_mailbox[4], matrix->image, matrix->image_bytes);
	ecode = instr_send(matrix_mailbox, matrix->image_bytes + 4, 10);
	if(ecode == 0) {
		ecode = instr_recv(matrix_mailbox, 2, 100);
		if(ecode == 0) {
			return matrix_mailbox[1];
		}
	}
	return -1;
}

struct matrix_s* matrix_open(const char *name)
{
	struct instr_s *instr = instr_open(INSTR_CLASS_MATRIX, name);
	if(instr == NULL) {
		return NULL;
	}

	struct matrix_s *matrix_new = sys_malloc(sizeof(struct matrix_s));
	sys_assert(matrix_new != NULL);
	matrix_select(matrix_new);
	if(matrix_cmd_get_size()) {
		//communication fail ..
		sys_free(matrix_new);
		return NULL;
	}

	unsigned bytes = sys_align(matrix_new->bus * matrix_new->row, 8) / 8;
	matrix_new->image_bytes = bytes;
	matrix_new->image = sys_malloc(bytes);
	sys_assert(matrix_new->image != NULL);

	matrix_reset();
	matrix_execute();
	return matrix_new;
}

void matrix_close(void)
{
	sys_assert(matrix != NULL);
	instr_close(matrix->instr);
	sys_free(matrix->image);
	sys_free(matrix);
	matrix = NULL;
}

int matrix_select(struct matrix_s *matrix_new)
{
	sys_assert(matrix_new != NULL);
	matrix = matrix_new;
	return instr_select(matrix->instr);
}

int matrix_reset(void)
{
	sys_assert(matrix != NULL);
	memset(matrix->image, 0x00, matrix->image_bytes);
	return 0;
}

int matrix_connect(int bus, int row)
{
	sys_assert(bus < matrix->bus);
	sys_assert(row < matrix->row);

	int index =  row * matrix->bus + bus;
	bit_set(index, matrix->image);
	return 0;
}

int matrix_execute(void)
{
	return matrix_cmd_set_image();
}
