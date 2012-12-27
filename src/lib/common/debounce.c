/*
 * miaofng@2012 initial version
 * debounce algo, common used the filt the noise on digital lines like keyboard input and etc
 *
 */

 #include "common/debounce.h"

void debounce_init(struct debounce_s *signal, unsigned threshold, unsigned init_lvl)
{
	signal->on = init_lvl;
	signal->off = !init_lvl;
	signal->cnt = 0;
	signal->threshold = threshold;
}

int debounce(struct debounce_s *signal, unsigned lvl)
{
	unsigned event = 0;
	if(lvl != signal->on) {
		if(signal->cnt < signal->threshold) {
			signal->cnt ++;
			if(signal->cnt >= signal->threshold) {
				signal->cnt = 0;
				event = 1;
			}
		}
	}
	else {
		if(signal->cnt > 0) {
			signal->cnt --;
		}
	}

	if(event) {
		signal->on = ! signal->on;
		signal->off = ! signal->off;
	}

	return event;
}
