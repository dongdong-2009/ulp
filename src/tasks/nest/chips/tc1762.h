/*
 * 	miaofng@2011 initial version
 */

#ifndef __TC1762_H_
#define __TC1762_H_

nest_reg_t tc1762_regs[] = {
	{.ofs = 0x00, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "P4_L", .bind = NULL, .desc = NULL},
	{.ofs = 0x08, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "P3_H", .bind = NULL, .desc = NULL},
	{.ofs = 0x10, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "P2_H", .bind = NULL, .desc = NULL},
	{.ofs = 0x18, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "P1_L", .bind = NULL, .desc = NULL},
};

nest_chip_t tc1762 = {
	.name = "TC1762",
	.regs = (nest_reg_t *) &tc1762_regs,
	.nr_of_regs = 4,
};

#define tc1762_init() nest_chip_init(&tc1762)
#define tc1762_bind(reg, pin) nest_chip_bind(&tc1762, reg, pin)
#define tc1762_mask(reg) nest_chip_mask(&tc1762, reg, 0x00)
#define tc1762_trap(reg, msk, val) nest_chip_trap(&tc1762, reg, msk, val)
#define tc1762_verify(data) nest_chip_verify(&tc1762, data)

#endif
