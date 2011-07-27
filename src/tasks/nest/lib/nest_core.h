/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_CORE_H_
#define __NEST_CORE_H_

int nest_init(void);
int nest_update(void);
int nest_mdelay(int ms);

enum {
	PSV = 1 << 0,
	BMR = 1 << 1,
	RLY = 1 << 2, /*CYC IGN RELAY ON/OFF*/
};
int nest_ignore(int mask);

int nest_wait_plug_in(void);
int nest_wait_pull_out(void);

//nest maintain
struct nest_info_s {
	int id_base;
	int id_fixture;
};

struct nest_info_s* nest_info_get(void);

//handle data structure of mapping
struct nest_map_s {
	const char *str;
	int id;
};
#define END_OF_MAP {NULL, -1}
int nest_map(const struct nest_map_s *, const char *str);

#endif
