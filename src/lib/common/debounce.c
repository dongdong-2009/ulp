/*
 * miaofng@2012 initial version
 * debounce algo, common used the filt the noise on digital lines like keyboard input and etc
 *
 */

#include "common/debounce.h"
#include "ulp_time.h"

void debounce_init(struct debounce_s *signal, unsigned threshold, unsigned init_lvl)
{
	signal->on = init_lvl;
	signal->off = ! signal->on;
	signal->cnt = 0;
	signal->threshold = threshold;
	signal->timer = 0;
}

void debounce_t_init(struct debounce_s *signal, unsigned threshold_ms, unsigned init_lvl)
{
	signal->on = init_lvl;
	signal->off = ! signal->on;
	signal->cnt = 0;
	signal->threshold = threshold_ms;
	signal->timer = time_get(0);
}

static int debounce_t(struct debounce_s *signal, unsigned lvl)
{
	unsigned event = 0;
	int delta_ms = - time_left(signal->timer);
	signal->timer = time_get(0);

	if(lvl != signal->on) {
		if(signal->cnt + delta_ms < signal->threshold) signal->cnt += delta_ms;
		else {
			signal->cnt = 0;
			signal->on = lvl;
			signal->off = ! signal->on;
			event = 1;
		}
	}
	else {
		if(signal->cnt >= delta_ms) signal->cnt -= delta_ms;
		else signal->cnt = 0;
	}

	return event;
}

int debounce(struct debounce_s *signal, unsigned lvl)
{
	unsigned event = 0;
	if (signal->timer != 0)
		return debounce_t(signal, lvl);

	if(lvl != signal->on) {
		if(signal->cnt + 1 < signal->threshold) signal->cnt ++;
		else {
			signal->cnt = 0;
			signal->on = lvl;
			signal->off = ! signal->on;
			event = 1;
		}
	}
	else {
		if(signal->cnt > 0) {
			signal->cnt --;
		}
	}

	return event;
}
