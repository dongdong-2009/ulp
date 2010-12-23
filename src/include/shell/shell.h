/*
 * 	miaofng@2009 initial version
 */
#ifndef __SHELL_H_
#define __SHELL_H_

#include "config.h"

#ifndef CONFIG_SHELL_PROMPT
#define CONFIG_SHELL_PROMPT "bldc# "
#endif

#ifndef CONFIG_BLDC_BANNER
#define CONFIG_BLDC_BANNER "Welcome to Brushless Motor Console"
#endif

#ifndef CONFIG_SHELL_LEN_CMD_MAX
#define CONFIG_SHELL_LEN_CMD_MAX 32
#endif

#ifndef CONFIG_SHELL_NR_PARA_MAX
#define CONFIG_SHELL_NR_PARA_MAX 8
#endif

#ifndef CONFIG_SHELL_LEN_HIS_MAX
#define CONFIG_SHELL_LEN_HIS_MAX 64
#endif

void shell_Init(void);
void shell_Update(void);

/*read a line from console, return 0 if not finish*/
int shell_ReadLine(const char *prompt, char *str);

#endif /*__SHELL_H_*/
