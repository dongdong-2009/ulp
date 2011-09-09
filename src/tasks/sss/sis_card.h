/*
 *	miaofng@2011 initial version
 */
#ifndef __SIS_CARD_H_
#define __SIS_CARD_H_

void card_init(void);
int card_getslot(void);
int card_getpower(void); //power on return 0

//pulse generator(music player? ^_^)
int card_player_init(int min, int max, int div); /*min/max is current, unit: mA, div = 72MHz/Counter Freq*/
int card_player_start(void *fifo, int n, int repeat);
int card_player_left(void);
int card_player_stop(void); //used when in repeat mode or force stop

//learn mode, recorder?
int card_recorder_init(void *cfg);
int card_recorder_start(void *fifo, int n, int repeat);
int card_recorder_left(void);
int card_recorder_stop(void);
#endif
