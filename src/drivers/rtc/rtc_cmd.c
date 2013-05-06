/*
 * 	miaofng@2012-12-30 initial version, be carefull of the overflow bug!!!
 */

#include "config.h"
#include "ulp_time.h"
#include "time.h"
#include <string.h>
#include "shell/cmd.h"

static int cmd_date_func(int argc, char *argv[])
{
	const char *usage = {
		"date -s [2012-12-30] [21:19:09]		config current date\n"
		"date					display current date\n"
	};

	if(argc > 0) {
		time_t t;
		struct tm *now;
		int n, err = 0;
		char *fmt = NULL;

		time(&t);
		now = localtime(&t);
		for(int i = 1; i < argc; i ++) {
			switch(argv[i][1]) {
			case 's':
				++ i;
				if((i < argc) && strchr(argv[i], '-') != NULL) {
					n = sscanf(argv[i], "%d-%d-%d", &now->tm_year, &now->tm_mon, &now->tm_mday);
					err += (n == 3) ? 0 : 1;
					err += (now->tm_year < 1970) ? 1 : 0;
					err += (now->tm_mon > 12) ? 1 : 0;
					err += (now->tm_mday > 31) ? 1 : 0;
					now->tm_year -= 1900;
					now->tm_mon -= 1; //0-11
					++ i;
				}
				if(i < argc) {
					n = sscanf(argv[i], "%d:%d:%d", &now->tm_hour, &now->tm_min, &now->tm_sec);
					err += (n >= 2) ? 0 : 1;
				}
				if(err == 0) { //config new date
					t = mktime(now);
					//printf("setting time to %s", ctime(&t));
					rtc_init(mktime(now));
				}
				break;
			default:
				if(argv[i][0] == '+') { //format
					fmt = &argv[i][1];
				}
				else
					err ++;
			}
		}
		if(err == 0) {
			time(&t);
			if(fmt == NULL)
				printf("current time is %s", ctime(&t));
			else {
				char str[64];
				strftime(str, 64, fmt, localtime(&t));
				printf("%s\n", str);
			}
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_date = {"date", cmd_date_func, "date set/get cmd for rtc"};
DECLARE_SHELL_CMD(cmd_date)
