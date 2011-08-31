#ifndef _SSS_H
#define _SSS_H
char can1_int(void);
char can_updata(void);
void ADDRESS_GPIO_Init(void);
int ADDRESS_GPIO_Read(void);
void RESET_Init(void);
#define SSS_INBOX_ADDR 0x0F000000
#define SSS_OUTBOX_ADDR 0x0F000100

enum {
	SSS_CMD_NONE,
	SSS_CMD_QUERY,
	SSS_CMD_LEARN,
	SSS_CMD_SELECT,
};

struct sensor_nvm_s {
	char name[16];
	char ID[16];
	char speed[8];
};

#endif