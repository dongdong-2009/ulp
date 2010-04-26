/* lcd1602.h
 * 	dusk@2010 initial version
 */
#ifndef __LCD1602_H_
#define __LCD1602_H_

#include <stddef.h>

typedef struct{
	uint8_t addr;
	uint8_t busy;
}lcd1602_status;

int lcd1602_Init(void);
int lcd1602_WriteChar(uint8_t row,uint8_t column,int8_t ch);
int lcd1602_WriteString(uint8_t row,uint8_t offset,char *s);
int lcd1602_ReadChar(uint8_t row,uint8_t column,int8_t &ch);
int lcd1602_ReadString(uint8_t row,uint8_t offset,char *s);

#endif /*__LCD1602_H_*/
