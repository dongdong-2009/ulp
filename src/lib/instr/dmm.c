/*
 * 	miaofng@2012 initial version
 *
 */

#include "ulp/sys.h"
#include "instr/instr.h"
#include "instr/dmm.h"
#include "crc.h"

static struct dmm_s *dmm = NULL;

struct dmm_s* dmm_open(const char *name)
{
	struct instr_s *instr = instr_open(INSTR_CLASS_DMM, name);
	if(instr == NULL) {
		return NULL;
	}

	struct dmm_s *dmm_new = instr->priv;
	if(dmm_new == NULL) {
		instr->priv = dmm_new = sys_malloc(sizeof(struct dmm_s));
		sys_assert(dmm_new != NULL);
		dmm_new->instr = instr;
	}

	dmm_select(dmm_new);
	return dmm_new;
}

void dmm_close(void)
{
	instr_close(dmm->instr);
	sys_free(dmm);
	dmm = NULL;
}

int dmm_select(struct dmm_s *dmm_new)
{
	dmm = dmm_new;
	return 0;
}

int dmm_config(const char *str_cfg)
{
	int ecode;
	char mailbox[4];

	mailbox[0] = DMM_CMD_CONFIG;
	mailbox[1] = str_cfg[0];
	ecode = instr_send(mailbox, 2, 10);
	ecode += instr_recv(mailbox, 2, 10);
	ecode = (ecode == 0) ? mailbox[1] : ecode;
	return ecode;
}

int dmm_read(void)
{
	int value = 0;
	char mailbox[10];
	mailbox[0] = DMM_CMD_READ;
	int ecode = instr_send(mailbox, 1, 10);
        memset(mailbox, 0, 6);
	ecode += instr_recv(mailbox, 10, 10);
	if((!ecode) && (mailbox[0] == DMM_CMD_READ)) {
		memcpy(&value, &mailbox[2], 4);
	}
	return value;
}

#include "shell/cmd.h"

static int cmd_dmm_func(int argc, char *argv[])
{
	const char *usage = {
		"dmm config R/V		measure R or V?\n"
		"dmm read		periodly read measure result\n"
	};

	if(dmm_open(NULL) == NULL) {
		printf("dmm not available\n");
		return 0;
	}

	if(argc > 2 && !strcmp(argv[1], "config")) {
		int ecode = dmm_config(argv[2]);
		printf("%s(ecode = %d)\n", (ecode == 0) ? "PASS" : "FAIL", ecode);
		return 0;
	}

	if((argc > 1)  && (!strcmp(argv[1], "read"))) {
		return 1;
	}

	if(argc == 0) {
		int value = dmm_read();
		printf("%d        \r", value);
		return 1;
	}

	printf(usage);
	return 0;
}

const cmd_t cmd_dmm = {"dmm", cmd_dmm_func, "dmm debug cmd"};
DECLARE_SHELL_CMD(cmd_dmm)
