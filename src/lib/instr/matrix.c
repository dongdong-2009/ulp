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

	struct matrix_s *matrix_new = instr->priv;
	if(matrix_new == NULL) {
		instr->priv = matrix_new = sys_malloc(sizeof(struct matrix_s));
		sys_assert(matrix_new != NULL);
		matrix_new->bus = 0;
		matrix_new->row = 0;
		matrix_new->image_bytes = 0;
		matrix_new->image = NULL;
		matrix_new->instr = instr;
		matrix_select(matrix_new);
		if(matrix_cmd_get_size()) {
			//communication fail ..
			sys_free(matrix_new);
                        instr->priv = NULL;
			return NULL;
		}

		unsigned bytes = sys_align(matrix_new->bus * matrix_new->row, 8) / 8;
		matrix_new->image_bytes = bytes;
		matrix_new->image = sys_malloc(bytes);
		sys_assert(matrix_new->image != NULL);

		//matrix_reset();
		//matrix_execute();
	}

	matrix_select(matrix_new);
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

#include "shell/cmd.h"

static int cmd_matrix_func(int argc, char *argv[])
{
	const char *usage = {
		"matrix conn 31 150	all off, then on row3bus1, row15bus0\n"
		"matrix scan [5] [100]	scan all node, 5mS on 5mS off, idle 100mS\n"
		"matrix short [5000]	open/short test, 5000mS on, 5000 mS off\n"
	};

	if(matrix_open(NULL) == NULL) {
		printf("matrix not available\n");
		return 0;
	}

	if(argc > 1 && !strcmp(argv[1], "conn")) {
		matrix_reset();
		for(int i = 2; i < argc; i ++) {
			int value = atoi(argv[i]);
			matrix_connect(value % 10, value / 10);
		}
		int ecode = matrix_execute();
		printf("%s(ecode = %d)\n", (ecode == 0) ? "PASS" : "FAIL", ecode);
		return 0;
	}

	if(argc >= 2  && !strcmp(argv[1], "scan")) {
		int ecode, ms = (argc > 2) ? atoi(argv[2]) : 5;
		int idle = (argc > 3) ? atoi(argv[3]) : 100;
		while(1) {
			for(int row = 0; row < matrix->row; row ++) {
				for(int bus = 0; bus < matrix->bus; bus ++) {
					sys_mdelay(ms);
					matrix_reset();
					ecode = matrix_execute();
					if(ecode) {
						printf("OP FAIL(ecode = %d)\n", ecode);
						return 0;
					}

					sys_mdelay(ms);
					matrix_connect(bus, row);
					ecode = matrix_execute();
					if(ecode) {
						printf("OP FAIL(ecode = %d)\n", ecode);
						return 0;
					}
				}
			}
			sys_mdelay(idle);
		}
	}

	if(argc >= 2 && !strcmp(argv[1], "short")) {
		int ecode, ms = (argc > 2) ? atoi(argv[2]) : 5000;
		while(1) {
			sys_mdelay(ms);
			memset(matrix->image, 0xff, matrix->image_bytes);
			ecode = matrix_execute();
			if(ecode) {
				printf("OP FAIL(ecode = %d)\n", ecode);
				return 0;
			}

			sys_mdelay(ms);
			memset(matrix->image, 0x00, matrix->image_bytes);
			ecode = matrix_execute();
			if(ecode) {
				printf("OP FAIL(ecode = %d)\n", ecode);
				return 0;
			}
		}
	}

	printf(usage);
	return 0;
}

const cmd_t cmd_matrix = {"matrix", cmd_matrix_func, "matrix debug cmd"};
DECLARE_SHELL_CMD(cmd_matrix)
