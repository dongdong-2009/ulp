/*
*	ETC CHIP PHDH(28026624) DRIVER
 * 	miaofng@2011 initial version
 */

#ifndef __PHDH_H_
#define __PHDH_H_

nest_reg_t phdh_regs[] = {
	{.ofs = 0x05, .len = 0x03, .msk = 0xff, .val = 0x00, .name = "DOUT", .bind = NULL, .desc = NULL},
};

nest_chip_t phdh = {
	.name = "PHDH",
	.regs = (nest_reg_t *) phdh_regs,
	.nr_of_regs = 1,
};

#define phdh_init() nest_chip_init(&phdh)
#define phdh_bind(reg, pin) nest_chip_bind(&phdh, reg, pin)
#define phdh_mask(reg) nest_chip_mask(&phdh, reg, 0x00)
#define phdh_trap(reg, msk, val) nest_chip_trap(&phdh, reg, msk, val)
#define phdh_verify(data) nest_chip_verify(&phdh, data)

#endif
