/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_CHIP_H_
#define __NEST_CHIP_H_

/* chip specification:
	chip can has 256bits(32bytes) memory in max, limitted by 'char ofs'
	reg can has 8 bits in max, limitted by 'char msk' and 'char val'
*/
typedef struct {
	char ofs; /*relative offset bits*/
	char len; /*nr of bits, note: [ofs, ofs + len) must not over 32 bit boundary*/
	char msk; /*bit mask*/
	char val; /*ideal value*/
	const char *name; /*register name, like "PCH1"*/
	const char *bind; /*bind to which pin??*/
	const char **desc; /*something like: ["OK", "SHORT TO BAT", "SHORT TO GND", "OPEN"] */
} nest_reg_t;

typedef struct {
	const char *name;
	nest_reg_t *regs; //point to: nest_reg_t regs[nr_of_regs]
	int nr_of_regs; //nr of reg in this chip
} nest_chip_t;

int nest_chip_init(nest_chip_t *chip);
//bind a chip pin to a specific device function
int nest_chip_bind(nest_chip_t *chip, const char *reg_name, const char *pin_name);
//in some special situation, we may cann't get the ideal value, but it's still acceptable, then this api is used to change the ideal value to a new one 
int nest_chip_trap(nest_chip_t *chip, const char *reg_name, int msk, int val);
int nest_chip_mask(nest_chip_t *chip, const char *reg_name, int msk);
//to verify the reister values are correct or not
int nest_chip_verify(nest_chip_t *chip, const char *data);

#endif
