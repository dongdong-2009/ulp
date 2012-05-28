/*
 * 	miaofng@2011 initial version
 */

#ifndef __CSD_H_
#define __CSD_H_

const char *csd_desc[] = {
	"OK",
	"Open load",
	"Short to Ground",
	"Short to Battery"
};

nest_reg_t csd_regs[] = {
	{.ofs = 0x00, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "SOLPWRA", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x02, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "SOLPWRB", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x04, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "SPAREHSD1", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x06, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "SPAREHSD2", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x08, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMSOLA", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x0a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMSOLB", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x0c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMSOLC", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x0e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMSOLD", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x10, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMSOLE", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x12, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMSOLF", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x14, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "LPSOL", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x16, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMOUT1", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x18, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PWMOUT2", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x1a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT1", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x1c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT2", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x1e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT3", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x20, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "HSG1", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x22, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "HSG2", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x24, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "HSG3", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x26, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "HSG4", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x28, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "HSG5", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x2a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "HSG6", .bind = NULL, .desc = csd_desc},
	{.ofs = 0x2d, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "DISCOUT4", .bind = NULL, .desc = csd_desc},
};

nest_chip_t csd = {
	.name = "CSD",
	.regs = (nest_reg_t *) &csd_regs,
	.nr_of_regs = 30,
};

#define csd_init() nest_chip_init(&csd)
#define csd_bind(reg, pin) nest_chip_bind(&csd, reg, pin)
#define csd_mask(reg) nest_chip_mask(&csd, reg, 0x00)
#define csd_trap(reg, msk, val) nest_chip_trap(&csd, reg, msk, val)
#define csd_verify(data) nest_chip_verify(&csd, data)

#endif
