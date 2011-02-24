/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_CORE_H_
#define __NEST_CORE_H_

int nest_init(void);
int nest_update(void);
int nest_mdelay(int ms);

//nest maintain
struct nest_info_s {
	int id_base;
	int id_fixture;
};

struct nest_info_s* nest_info_get(void);

#endif
