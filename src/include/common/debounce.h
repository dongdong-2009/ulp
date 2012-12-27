/*
 *	miaofng@2012 initial version
 *
 */
#ifndef __COMMON_DEBOUNCE_H_
#define __COMMON_DEBOUNCE_H_

struct debounce_s {
	unsigned on : 1;
	unsigned off : 1;
	unsigned cnt : 15;
	unsigned threshold : 15;
};

void debounce_init(struct debounce_s *signal, unsigned threshold, unsigned init_lvl);
int debounce(struct debounce_s *signal, unsigned lvl);

#endif /*__COMMON_DEBOUNCE_H_*/
