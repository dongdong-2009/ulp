/*
 * 	miaofng@2012 initial version
 */

#include "ulp/sys.h"
#include "ybs_dc.h"
#include <string.h>

static int dc_ch;
static struct dc_fifo_s
{
	/*for dc_save/dc_read*/
	int wn;
	int rn;
	short data[DC_FIFO_NP];
	time_t time;

	/*for dc_keep usage*/
	time_t keep_ms_i; //the moment of keep cmd issued
	time_t keep_ms_e; //the moment of keep cmd excuted
	int keep_start;
	int keep_end;
} dc_fifos[DC_CH_NR], *dc_fifo;
#define DC_FIFO_SZ (sizeof(struct dc_fifo_s))

static int sect_empty_nr;
static struct list_head queue_empty;
static struct list_head queue_temp;
static struct list_head queue_keep;
static struct list_head queue_ybs[DC_CH_NR];

#define sect_get(idx) (struct dc_sect_s *)(DC_BASE + DC_SECT_SZ * idx)
static inline void sect_init(struct dc_sect_s *sect)
{
	sect->offset = 0;
	INIT_LIST_HEAD(&sect->list_empty);
	INIT_LIST_HEAD(&sect->list_keep);
	INIT_LIST_HEAD(&sect->list_temp);
	INIT_LIST_HEAD(&sect->list_ybs);
	//INIT_LIST_HEAD(&sect->queue_hash); //!!! do not init me!!!!
	INIT_LIST_HEAD(&sect->list_hash);
}

static int __hash(int key)
{
	key = key % DC_SECT_NR;
	return key;
}

int dc_init(void)
{
	int i;
	struct dc_sect_s *sect;

	//create empty sector queue
	sect_empty_nr = DC_SECT_NR;
	INIT_LIST_HEAD(&queue_empty);
	for(i = 0; i < DC_SECT_NR; i ++) {
		sect = sect_get(i);
		sect_init(sect);
		INIT_LIST_HEAD(&sect->queue_hash); //note: only execute one time!!!
		list_add_tail(&queue_empty, &sect->list_empty);
	}

	INIT_LIST_HEAD(&queue_keep);
	INIT_LIST_HEAD(&queue_temp);
	for(i = 0; i < DC_CH_NR; i ++) {
		INIT_LIST_HEAD(&queue_ybs[i]);
		sect = list_first_entry(&queue_empty, dc_sect_s, list_empty);
		list_del(&sect->list_empty);
		sect_empty_nr --;
		sect->ybs = i;
		list_add(&queue_temp, &sect->list_temp);
		list_add(&queue_ybs[i], &sect->list_ybs);
	}
	return 0;
}

static int __save(int ch)
{
	int i, j, n, ofs, words;
	struct dc_sect_s *sect;
	struct dc_fifo_s fifo;
	short *dst;

	//to ensure atom ops, to avoid irq occurs when we are reading ..
	memcpy(&fifo, &dc_fifos[ch], DC_FIFO_SZ);
	if(memcmp(&fifo, &dc_fifos[ch], DC_FIFO_SZ)) {
		memcpy(&fifo, &dc_fifos[ch], DC_FIFO_SZ);
	}

	time_t time = fifo.time;
	words = fifo.wn - fifo.rn;
	words += (words < 0) ? DC_FIFO_NP : 0;
	for(i = 0; i < words; i ++) {
		//always mv tension data from fifo to sectors
		sect = list_first_entry(&queue_ybs[dc_ch], dc_sect_s, list_ybs);
		n = DC_SECT_NP - sect->offset;
		n = (n < words) ? n : words;
		dst = &sect->data[sect->offset];
		for(j = 0; j < n; j ++) {
			ofs = fifo.rn + i + j;
			ofs -= (ofs >= DC_FIFO_NP) ? DC_FIFO_NP : 0;
			*dst = fifo.data[ofs];
		}

		sect->offset += n;
		words -= n;
		i += n;

		if(sect->offset >= DC_SECT_NP) {
			sect = list_first_entry(&queue_empty, dc_sect_s, list_empty);
			list_del(&sect->list_empty);
			sect_empty_nr --;
			sect->ybs = ch;
			sect->time = time - DC_SAVE_MS * (words - i);
			list_add_tail(&queue_temp, &sect->list_temp);
			list_add_tail(&queue_ybs[dc_ch], &sect->list_ybs);
		}
	}
	n = fifo.rn + words;
	dc_fifos[ch].rn = (n >= DC_FIFO_NP) ? (n - DC_FIFO_NP) : n;
	return words;
}

/*to move keeped sectors from list_temp to list_keep*/
static int __keep(int ch)
{
	struct list_head *pos;
	struct dc_sect_s *sect;
	struct dc_fifo_s fifo;
	int p, words, ps, pe, nr;

	//to ensure atom ops, to avoid irq occurs when we are reading ..
	memcpy(&fifo, &dc_fifos[ch], DC_FIFO_SZ);
	if(memcmp(&fifo, &dc_fifos[ch], DC_FIFO_SZ)) {
		memcpy(&fifo, &dc_fifos[ch], DC_FIFO_SZ);
	}

	if(fifo.keep_ms_i <= fifo.keep_ms_e) //no keep request
		return 0;

	//warnning: the time diff is ignored here, it should not be a big problem ..
	p = nr = 0;
	list_for_each_prev(pos, &queue_ybs[ch]) {
		sect = list_entry(pos, dc_sect_s, list_ybs);
		words = sect->offset;
		ps = p - words;
		pe = p;
		if(((ps > fifo.keep_start) && (ps < fifo.keep_end)) || \
			((pe > fifo.keep_start) && (pe < fifo.keep_end))) {
			//found a sector to be keeped
			list_del(&sect->list_temp);
			list_add_tail(&queue_keep, &sect->list_keep);
			nr ++;

			//hash search support
			int idx = __hash(sect->time / DC_HASH_MS);
			struct dc_sect_s *hash = sect_get(idx);
			list_add_tail(&hash->queue_hash, &sect->list_hash);
		}
		p -= words;
		if(p < fifo.keep_end)
			break;
	}
	return nr;
}

static int __clear(int left)
{
	struct list_head *pos;
	struct dc_sect_s *sect;

	//are there any temp sectors could be released???
	list_for_each(pos, &queue_temp) {
		sect = list_entry(pos, dc_sect_s, list_temp);
		if(time_left(sect->time) > -DC_TEMP_MS)
			break;

		list_del(&sect->list_hash);
		list_del(&sect->list_temp);
		list_del(&sect->list_ybs);
		list_add(&queue_empty, &sect->list_empty);
		sect_init(sect);
		sect_empty_nr ++;
		left ++;
	}

	//release keeped sectors until there are enough empty sectors
	list_for_each(pos, &queue_keep) {
		sect = list_entry(pos, dc_sect_s, list_keep);
		if(left > 0)
			break;

		list_del(&sect->list_hash);
		list_del(&sect->list_keep);
		list_del(&sect->list_ybs);
		list_add(&queue_empty, &sect->list_empty);
		sect_init(sect);
		sect_empty_nr ++;
		left ++;
	}
	return 0;
}

int dc_update(void)
{
	int ch, left;
	for(ch = 0; ch < DC_CH_NR; ch ++) {
		__save(ch);
		__keep(ch);
	}

	left = sect_empty_nr - DC_SECT_EMPTY_TH;
	if(left < 0) {
		__clear(left);
	}
	return 0;
}

void dc_config_ybs(int ch)
{
	dc_ch = ch;
	dc_fifo = &dc_fifos[ch];
}

int dc_save(short tension)
{
	int n = dc_fifo->wn;
	dc_fifo->data[n] = tension;
	n ++;
	n = (n >= DC_FIFO_NP) ? 0 : n;
	dc_fifo->wn = n;
	dc_fifo->time = time_get(0);
	sys_assert(n != dc_fifo->rn); //fifo overflow???
	return 0;
}

int dc_read(int offset)
{
	int n = dc_fifo->wn;
	n += offset;
	n += (n < 0) ? DC_FIFO_NP : 0;
	return dc_fifo->data[n];
}

int dc_keep(int start, int end)
{
	int n = dc_fifo->keep_ms_i - dc_fifo->keep_ms_e;
	if(n > 0) { //merge the keep range
		start = dc_fifo->keep_start - n;
	}
	dc_fifo->keep_ms_i = time_get(0);
	dc_fifo->keep_start = start;
	dc_fifo->keep_end = end;
	return 0;
}

struct dc_sect_s *__search(int ch, time_t time)
{
	struct dc_sect_s *hash, *sect;
	struct list_head *pos;
	int idx = __hash(time / DC_HASH_MS);
	hash = sect_get(idx);
	list_for_each(pos, &hash->queue_hash) {
		sect = list_entry(pos, dc_sect_s, list_hash);
		if(sect->ybs == ch) {
			return sect;
		}
	}
	return NULL;
}

static const short dummy[] = {
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,

	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
	DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA, DC_DATA_NA,
};

int dc_send_tcp_dummy(struct tcp_pcb *pcb, int nwords)
{
	int n;
	#define DUMMY_NP (sizeof(dummy) / 2)
	for(; nwords > 0; nwords -= DUMMY_NP) {
		n = (nwords > DUMMY_NP) ? DUMMY_NP : nwords;
		tcp_write(pcb, dummy, n << 1, TCP_WRITE_FLAG_MORE);
	}
	return 0;
}

int dc_send_tcp(struct tcp_pcb *pcb, int ch, time_t start, time_t end)
{
	struct dc_sect_s *sect, *sect_prev;
	int ms, n, words_sent, words;
	time_t time;

	words = time_diff(end, start) / DC_SAVE_MS;
	if((words <= 0) || (words >= DC_SEND_TCP_LIMIT)) {
		assert(0);
		return -1;
	}

	/*note: time is not the para start!!!*/
	time = time_shift(start, -DC_SECT_MS);
	while(time_diff(end, time) > 0) {
		sect = __search(ch, time);
		if(sect == NULL) { //not found
			time = time_shift(time, DC_HASH_MS);
			continue;
		}
	}

	if(sect == NULL) { //not found, send dummy data
		dc_send_tcp_dummy(pcb, words);
		return words;
	}

	//found the sect :) walk through the ybs chain
	words_sent = 0;
	sect_prev = NULL;
	struct list_head *pos = &sect->list_ybs;
	for (; pos != &queue_ybs[ch]; pos = pos->next) {
		sect = list_entry(pos, dc_sect_s, list_ybs);
		time = time_shift(sect->time, DC_SECT_MS); //shift time to sect tail
		ms = time_diff(time, start);
		if(ms > 0) {
			//current sect is earlier than the start time, try next ..
			continue;
		}

		if(sect_prev != NULL) {
			ms = time_diff(sect->time, sect_prev->time);
			ms -= DC_SECT_MS;
			if(ms > 0) { //these two sector is discontinuous on time
				n = ms / DC_SAVE_MS;
				dc_send_tcp_dummy(pcb, n);
				words_sent += n;
			}
		}

		sect_prev = sect;

		ms = time_diff(start, sect->time);
		if(ms > 0) {
			//current sect is later than the start time, insert dummy data first...
			n = ms / DC_SAVE_MS;
			dc_send_tcp_dummy(pcb, n);
			words_sent += n;
		}

		n = (ms < 0) ? (- ms / DC_SAVE_MS) : 0;
		short *data = &sect->data[n];
		n = DC_SECT_NP - n;
		n = (n > (words - words_sent)) ? (words - words_sent) : n;
		tcp_write(pcb, data, n * sizeof(short), TCP_WRITE_FLAG_MORE);
		words_sent += n;
		if(words_sent >= words)
			break;
	}

	return words_sent;
}
