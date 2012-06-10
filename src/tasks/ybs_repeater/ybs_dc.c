/*
 * 	miaofng@2012 initial version
 */

#include "ulp/sys.h"
#include "ybs_dc.h"

static int dc_ch;
static struct dc_fifo_s
{
	/*for dc_save/dc_read*/
	int wn;
	int rn;
	short *data[DC_FIFO_SZ];
	time_t time;

	/*for dc_keep usage*/
	time_t keep_ms_i; //the moment of keep cmd issued
	time_t keep_ms_e; //the moment of keep cmd excuted
	int keep_start;
	int keep_end;
} dc_fifos[DC_CH], *dc_fifo;

static int sect_empty_nr;
static struct list_head queue_empty;
static struct list_head queue_temp;
static struct list_head queue_keep;
static struct list_head queue_ybs[DC_CH];

static inline void sect_init(struct dc_sect_s *sect)
{
	sect->offset = sizeof(struct dc_sect_s);
	INIT_LIST_HEAD(&sect->list_empty);
	INIT_LIST_HEAD(&sect->list_keep);
	INIT_LIST_HEAD(&sect->list_temp);
	INIT_LIST_HEAD(&sect->list_ybs);
	INIT_LIST_HEAD(&sect->list_hash);
}

int dc_init(void)
{
	int i;
	struct dc_sect_s *sect;

	//create empty sector queue
	sect_empty_nr = DC_SECT_NR;
	INIT_LIST_HEAD(&queue_empty);
	for(i = 0; i < DC_SECT_NR; i ++) {
		sect = (struct dc_sect_s *)(DC_BASE + DC_SECT_SZ * i);
		sect_init(sect);
		list_add_tail(&queue_empty, &sect->list_empty);
	}

	INIT_LIST_HEAD(&queue_keep);
	INIT_LIST_HEAD(&queue_temp);
	for(i = 0; i < DC_CH; i ++) {
		INIT_LIST_HEAD(&queue_ybs[i]);
		sect = list_first_entry(&queue_empty);
		list_del(&sect->list_empty);
		sect_empty_nr --;
		sect->ybs = i;
		list_add(&queue_temp, &sect->list_temp);
		list_add(&queue_ybs[i], &sect->list_ybs);
	}
}

static int __save(int ch)
{
	int i, j, n, ofs, words;
	struct dc_sect_s *sect;
	struct dc_fifo_s fifo;
	short *dst;

	//to ensure atom ops, to avoid irq occurs when we are reading ..
	memcpy(&fifo, &dc_fifos[ch], sizeof(struct dc_fifo_s));
	if(memcmp(&fifo, &dc_fifos[ch], sizeof(struct dc_fifo_s))) {
		memcpy(&fifo, &dc_fifos[ch], sizeof(struct dc_fifo_s));
	}

	time_t time = fifo.time;
	words = fifo.wn - fifo.rn;
	words += (words < 0) ? DC_FIFO_SZ : 0;
	for(i = 0; i < words; i ++) {
		//always mv tension data from fifo to sectors
		sect = list_first_entry(&queue_ybs[dc_ch]);
		n = (DC_SECT_SZ - sect->offset) >> 1;
		n = (n < words) ? n : words;
		dst = (short *) ((int)sect + sect->offset);
		for(j = 0; j < n; j ++) {
			ofs = fifo.rn + i + j;
			ofs -= (ofs >= DC_FIFO_SZ) ? DC_FIFO_SZ : 0;
			*dst = fifo.data[ofs];
		}

		sect->offset += (n << 1);
		words -= n;
		i += n;

		if(sect->offset + 2 >= DC_SECT_SZ) {
			sect = list_first_entry(&queue_empty);
			list_del(&sect->list_empty);
			sect_empty_nr --;
			sect->ch = ch;
			sect->time = time - DC_SAVE_MS * (words - i - 1);
			list_add_tail(&queue_temp, &sect->list_temp);
			list_add_tail(&queue_ybs[dc_ch], &sect->list_ybs);
		}
	}
	n = fifo.rn + words;
	dc_fifos[ch].rn = (n >= DC_FIFO_SZ) ? (n - DC_FIFO_SZ) : n;
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
	memcpy(&fifo, &dc_fifos[ch], sizeof(struct dc_fifo_s));
	if(memcmp(&fifo, &dc_fifos[ch], sizeof(struct dc_fifo_s))) {
		memcpy(&fifo, &dc_fifos[ch], sizeof(struct dc_fifo_s));
	}

	if(fifo.keep_ms_i <= fifo.keep_ms_e) //no keep request
		return 0;

	//warnning: the time diff is ignored here, it should be a big problem ..
	p = nr = 0;
	list_for_each_prev(pos, &queue_ybs[ch]) {
		sect = list_entry(pos, dc_sect_s, list_ybs);
		words = (sect->offset - sizeof(struct dc_sect_s)) >> 1;
		ps = p - words;
		pe = p;
		if(((ps > fifo.keep_start) && (ps < fifo.keep_end)) || \
			((pe > fifo.keep_start) && (pe < fifo.keep_end))) {
			//found a sector to be keeped
			list_del(&sect->list_temp);
			list_add_tail(&queue_keep, &sect->list_keep);
			nr ++;
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
	n = (n >= DC_FIFO_SZ) ? 0 : n;
	dc_fifo->wn = n;
	dc_fifo->time = time_get(0);
	sys_assert(n != dc_fifo->rn); //fifo overflow???
	return 0;
}

int dc_read(int offset)
{
	int n = dc_fifo->wn;
	n += offset;
	n += (n < 0) ? DC_FIFO_SZ : 0;
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

struct pbuf* dc_read_pbuf(time_t time, int n)
{
}
