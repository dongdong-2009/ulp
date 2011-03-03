/*
 * 	miaofng@2011 initial version
 */

#ifndef __HFPS_H_
#define __HFPS_H_

nest_reg_t hfps_regs[] = {
	{.ofs = 0x00, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "STATUS_H", .bind = NULL, .desc = NULL},
	{.ofs = 0x08, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "STATUS_L", .bind = NULL, .desc = NULL},
	{.ofs = 0x10, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "DIAG1_H", .bind = NULL, .desc = NULL},
	{.ofs = 0x18, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "DIAG1_L", .bind = NULL, .desc = NULL},
	{.ofs = 0x20, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "DIAG0_H", .bind = NULL, .desc = NULL},
	{.ofs = 0x28, .len = 0x08, .msk = 0xff, .val = 0x00, .name = "DIAG0_L", .bind = NULL, .desc = NULL},
};

nest_chip_t hfps = {
	.name = "HFPS",
	.regs = (nest_reg_t *) &hfps_regs,
	.nr_of_regs = 6,
};

#define hfps_init() nest_chip_init(&hfps)
#define hfps_bind(reg, pin) nest_chip_bind(&hfps, reg, pin)
#define hfps_trap(reg, msk, val) nest_chip_trap(&hfps, reg, msk, val)
#define hfps_verify(data) nest_chip_verify(&hfps, data)

#endif
