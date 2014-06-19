/*
 * 	miaofng@2013-2-21 initial version
 *	used for oid hardware version 2.0 board
 */

#include "ulp/sys.h"
#include "ybs_mcd.h"
#include "common/vchip.h"
#include "spi.h"
#include <stdlib.h>
#include "shell/cmd.h"
#include "linux/sort.h"

static int __aduc_write(char byte)
{
        sys_mdelay(1);
	return spi2.wreg(SPI_2_NSS, byte);
}

static int __aduc_read(char *byte)
{
        sys_mdelay(1);
	*byte = spi2.rreg(SPI_2_NSS);
	return 0;
}

static const spi_cfg_t spi_cfg = {
	.cpol = 0,
	.cpha = 0,
	.bits = 8,
	.bseq = 1,
	.freq = 1000000,
};

static const vchip_slave_t aduc = {
	.wb = __aduc_write,
	.rb = __aduc_read,
};

int mcd_init(void)
{
	int ecode;
	spi2.init(&spi_cfg);
	/*to test aduc communication*/
	for(int i = 0; i < 3; i ++) { //try at most 2 times
		ecode = vchip_outl(&aduc, DMM_REG_DATA, 0x12345);
		if(ecode == 0)
			break;
	}
	spi_cs_set(SPI_CS_PB6, 0); //LE = 0
	spi_cs_set(SPI_CS_PB7, 0); //OE = 0
	mcd_relay(0x0000); //all relay off
	return ecode;
}

static int bank_old = -1;
int mcd_mode(char bank)
{
	int ecode = 0, value, ms, j = 0;
	struct dmm_mode_s *mode = (struct dmm_mode_s *) &value;
	while(j < 3) {
		j ++;
		ecode = vchip_outl(&aduc, DMM_REG_MODE, bank);
		if(!ecode) {
			ecode = vchip_inl(&aduc, DMM_REG_MODE, &value);
			ecode = (ecode == 0) ? mode->ecode : ecode;
			ecode = (ecode == 0) ? ((value & 0xff) != bank) : ecode;
			if(ecode == 0) {
				ms = ((bank_old & 0xf0) != (bank & 0xf0)) ? 500 : 0;
				bank_old = bank;
				break;
			}
		}
	}
	sys_mdelay(ms);
	return ecode;
}


int mcd_read(int *uv_or_mohm)
{
	#define RMN 5
	int j = 0, ecode = 0, array[RMN], value;
	struct dmm_mode_s *mode = (struct dmm_mode_s *) &value;
	for(int i = 0; i < RMN;) {
		while(ecode == 0) {
			sys_mdelay(20);
			ecode = vchip_inl(&aduc, DMM_REG_MODE, &value);
			//printf("mode = 0x%08x .. %s\n", value, (ecode == 0) ? "pass" : "fail");
			ecode = (ecode == 0) ? mode->ecode : ecode;
			if((ecode == 0) && mode->ready) {
				//value = 0;
				//mode->clr = 1;
				//vchip_outl(&aduc, DMM_REG_MODE, value); /*clr ready bit*/
				ecode = vchip_inl(&aduc, DMM_REG_DATA, &value);
				//printf("v = %d .. %s\n", value, (ecode == 0) ? "pass" : "fail");
				break;
			}
		}
		if(ecode != 0) {
			j ++;
			if(j > 3) break;
			ecode = 0;
		}
		else {
			array[i] = value;
			i ++;
		}
	}

	if(ecode == 0) {
		sort(array, RMN, sizeof(int), NULL, NULL);
		value = array[(int)(RMN / 2)];
		*uv_or_mohm = value;
	}
	return ecode;
}

static int image_old = -1;
void mcd_relay(int image)
{
	if(image != image_old) {
		image_old = image;
		char byte = (char)(image >> 8);
		spi2.wreg(SPI_CS_DUMMY, byte); //msb
		byte = (char) image;
		spi2.wreg(SPI_CS_DUMMY, byte); //lsb
		spi_cs_set(SPI_CS_PB6, 1);
		spi_cs_set(SPI_CS_PB6, 0);
		sys_mdelay(1000);
	}
}

/*
1, please short each group of 4 signals first, such as R0V/R0I, R1V/R1I, R2V/R2I ...
2, then you will have 6 pins, 0..5,
3, call mcd_pick to measure the voltage/resistor of a diode from pin0(+) to pin1(-)
*/
void mcd_pick(int pin0, int pin1)
{
	int image = 0;
	pin0 <<= 1;
	pin1 <<= 1;
	image |= 1 << (pin0 + 0);
	image |= 1 << (pin1 + 1);
	mcd_relay(image);
}

int mcd_xread(int ch, int *mv)
{
	if(ch > 0) mcd_relay(1 << ch);
	int uv = 0, e = mcd_read(&uv);
	uv = - uv; //hw bug
	uv *= 25;
	*mv = uv / 1000; //unit: mV
	return e;
}

static int cmd_mcd_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mcd -i			mcd init\n"
		"mcd -m	[R/V/S/O]	change test mode before read\n"
		"mcd -y	0..15		turn on relay s0..15 before read\n"
		"mcd -p 01		pick(pin0, pin1) before read\n"
		"mcd -r [100]		read [100] times\n"
		"mcd -x			unit: mv, gain=25 for ybs monitor use only\n"
	};

	if(argc > 0) {
		char mode = DMM_R_AUTO;
		int r = 0, v, ch, e = 0;
		for(int j, i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'i':
				e = mcd_init();
				if(e) {
					printf("mcd init error\n");
					return 0;
				}
				break;
			case 'm':
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					mode = (argv[++ i][0] == 'V') ? DMM_V_AUTO : mode;
					mode = (argv[i][0] == 'S') ? DMM_R_SHORT : mode;
					mode = (argv[i][0] == 'O') ? DMM_R_OPEN : mode;
				}
				e = mcd_mode(mode);
				if(e) {
					printf("mcd mode change error\n");
					return 0;
				}
				break;

			case 'y':
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					int relay = atoi(argv[++ i]);
					mcd_relay(1 << relay);
				}
				break;

			case 'p':
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					int pin = atoi(argv[++ i]);
					pin %= 100;
					mcd_pick(pin/10, pin%10);
				}
				break;

			case 'r':
				r = 1;
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					r = atoi(argv[++ i]);
				}
				break;

			case 'x':
				ch = -1;
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					ch = atoi(argv[++ i]);
				}
				e = mcd_xread(ch, &v);
				if(!e) printf("%d mV\n", v);
				else printf("ecode = %d\n", e);
				break;

			default:
				e ++;
			}
		}

		if(e) {
			printf("%s", usage);
			return 0;
		}

		while(r > 0) {
			r --;
			e = mcd_read(&v);
			printf("read %s(val = %d)\n", (e == 0) ? "pass" : "fail", v);
		}
	}
	return 0;
}

const cmd_t cmd_mcd = {"mcd", cmd_mcd_func, "mcd debug cmds"};
DECLARE_SHELL_CMD(cmd_mcd)
