/*
 * 	miaofng@2011 initial version
 */

#ifndef __EMCD_H_
#define __EMCD_H_

const char *emcd_desc[] = {
	"OK",
	"Open load",
	"Short to Ground",
	"Short to Battery"
};

nest_reg_t emcd_regs[] = {
	{.ofs = 0x00, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT5", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x02, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT6", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x04, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "VSSOUT", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x06, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMOUT3", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x08, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMOUT4", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x0a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMOUT5", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x0c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT7", .bind = NULL, .desc = emcd_desc},
	{.ofs = 0x0e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT8", .bind = NULL, .desc = emcd_desc},
};

nest_chip_t emcd = {
	.name = "EMCD",
	.regs = (nest_reg_t *) &emcd_regs,
	.nr_of_regs = 8,
};

#define emcd_init() nest_chip_init(&emcd)
#define emcd_bind(reg, pin) nest_chip_bind(&emcd, reg, pin)
#define emcd_mask(reg) nest_chip_mask(&emcd, reg, 0x00)
#define emcd_trap(reg, msk, val) nest_chip_trap(&emcd, reg, msk, val)
#define emcd_verify(data) nest_chip_verify(&emcd, data)
#endif
