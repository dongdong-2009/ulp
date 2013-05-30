/*
 * 	miaofng@2012 initial version
 */

/*matrix bus connection*/
enum {
	VBUS_VIP, /*BUS 0*/
	VBUS_VIN,
	VBUS_XX0, /*NOT USED*/
	VBUS_XX1, /*NOT USED*/

	RBUS_VIP,
	RBUS_VIN,
	RBUS_IOP, /*current source 1mA*/
	RBUS_ION, /*470OHM to SGND*/
};

/*matrix row connection*/
enum {
	PIN_SHELL = 0, /*default to pin 0*/
	PIN_GRAY, /*default to pin 1, gray line*/
	PIN_BLACK, /*default to pin 2, black line*/
	PIN_WHITE1, /*default to pin 3, white line*/
	PIN_WHITE2, /*default to pin 4, white line*/

	PIN_RCAL1,
	PIN_RCAL2,
};

static const char oid_pinmap[] = {0,1,5,3,4,  4,6};

static struct oid_kcode_s {
	char lines; /*1-4*/
	char ground; /*'Y', 'N', '?'*/
	int mohm;
	int mv;

	int r[5][5];
} oid_kcode;

static int __kcode(const struct oid_kcode_s *kcode)
{
	int moh, mol, v = kcode->lines << (4 * 5);
	moh = (kcode->mohm / 1000) % 10;
	mol = (kcode->mohm / 100) % 10;
	v |= (kcode->ground == 'Y') ? (1 << (4 * 4)) : 0;
	v |= (moh << (4 * 3));
	v |= (mol << (4 * 2));
	v |= (kcode->mv / 100);
	return v;
}

/*convert signature to trans type*/
static int __tcode(const struct oid_kcode_s *kcode)
{
	int type = 0, r = kcode->mohm;
	r = ((r > 1100) && (r <= 2800)) ? 2100 : r;
	r = ((r > 2800) && (r <= 4500)) ? 3500 : r;
	r = ((r > 5000) && (r <= 7000)) ? 6000 : r;

	switch(kcode->lines) {
	case 1:
		type = 1901;
		break;

	case 2:
		type = (kcode->ground == 'Y') ? 2901 : type;
		type = (kcode->ground == 'N') ? 2902 : type;
		break;

	case 3:
		type = (r == 2100) ? 3901 : type;
		type = (r == 3500) ? 3902 : type;
		type = (r == 6000) ? 3903 : type;
		break;
	case 4:
		type = ((kcode->ground == 'Y') && (r == 2100)) ? 4901 : type;
		type = ((kcode->ground == 'Y') && (r == 3500)) ? 4902 : type;
		type = ((kcode->ground == 'Y') && (r == 6000)) ? 4903 : type;
		type = ((kcode->ground == 'N') && (r == 2100)) ? 4904 : type;
		type = ((kcode->ground == 'N') && (r == 3500)) ? 4905 : type;
		type = ((kcode->ground == 'N') && (r == 6000)) ? 4906 : type;
		break;
	default:;
	}
	return type;
}

static int oid_mohm_offset = 0;
static int oid_measure_resistor(int pin0, int pin1)
{
	int mohm = -1;
	dmm_config("R");
	matrix_reset();
	matrix_connect(RBUS_IOP, oid_pinmap[pin0]);
	matrix_connect(RBUS_VIP, oid_pinmap[pin0]);
	matrix_connect(RBUS_ION, oid_pinmap[pin1]);
	matrix_connect(RBUS_VIN, oid_pinmap[pin1]);
	if(!matrix_execute()) {
		oid_show_progress(PROGRESS_WAIT);
		#if 1
		time_t deadline = time_get(600);
		while(time_left(deadline) > 0) {
			instr_update();
		}
		#else
		sys_mdelay(300);
		#endif
		mohm = dmm_read();
		mohm += oid_mohm_offset;
		mohm = (mohm < 0) ? 0 : mohm;
	}
	return mohm;
}

static int oid_measure_voltage(int pin0, int pin1)
{
	int mv = -1;
	dmm_config("V");
	matrix_reset();
	matrix_connect(RBUS_VIP, oid_pinmap[pin0]);
	matrix_connect(RBUS_VIN, oid_pinmap[pin1]);
	if(!matrix_execute()) {
		oid_show_progress(PROGRESS_WAIT);
		#if 1
		time_t deadline = time_get(600);
		while(time_left(deadline) > 0) {
			instr_update();
		}
		#else
		sys_mdelay(300);
		#endif
		mv = dmm_read();
		mv = (mv < 0) ? (-mv) : mv;
	}
	return mv;
}

int oid_instr_cal(void)
{
	struct dmm_s *oid_dmm = NULL;
	struct matrix_s *oid_matrix = NULL;

	oid_dmm = dmm_open(NULL);
	oid_matrix = matrix_open(NULL);
	if(oid_dmm == NULL || oid_matrix == NULL) {
		oid_error((oid_dmm == NULL) ? E_DMM_INIT : E_OK);
		oid_error((oid_matrix == NULL) ? E_MATRIX_INIT : E_OK);
		return 1;
	}

	#define RCAL 4700
	oid_mohm_offset = 0;
	int mohm = oid_measure_resistor(PIN_RCAL1, PIN_RCAL2);
	/*note: 1800 is predict offset according to experiment been done*/
	if((mohm - 1800 > (int)(RCAL*1.2)) || (mohm - 1800 < (int)(RCAL*0.8))) {
		oid_error(E_RES_CAL);
		return 1;
	}

	oid_mohm_offset = RCAL - mohm;
	//matrix scan to avoid short circuit???
	return 0;
}

int oid_cold_test(struct oid_config_s *cfg)
{
	char i, j, n, e = 0;
	int r;

	memset(&oid_kcode, 0x00, sizeof(oid_kcode));
	oid_kcode.lines = cfg->lines;
	oid_kcode.ground = '?';
	oid_kcode.mohm = 0;
	oid_kcode.mv = 0;

	if(cfg->mode == 'i') {
		/*measure resistors*/
		for(i = 0; i <= cfg->lines; i ++) {
			for(j = i + 1; j <= cfg->lines; j ++) {
				oid_kcode.r[i][j] = oid_measure_resistor(i, j);
			}
		}

		//how many lines short to o2 sensor shell ground?
		for(n = 0, j = 1; j <= cfg->lines; j ++) {
			r = oid_kcode.r[0][j];
			n += (r < 10000) ? 1 : 0;
		}

		oid_kcode.ground = (n > 0) ? 'Y' : 'N';
		if(n > 1) { //more than 1 lines shorten to shell ground, maybe black/gray/white???
			e = 1;
			if(cfg->mode == 'i') {
				oid_error(E_UNDEF);
			}
		}

		else if((oid_kcode.ground != cfg->ground[0]) && (cfg->ground[0] != '?')) {
			e = 1;
			if(oid_kcode.ground == 'Y') {
				oid_error(20202); //gray shorten to shell gnd
			}
			else {
				oid_error(20102); //gray break to shell gnd
			}
		}

		//how many pins are shorten?
		for(n = 0, i = 1; i <= cfg->lines; i ++) {
			for(j = i + 1; j <= cfg->lines; j ++) {
				r = oid_kcode.r[i][j];
				if(r < 50000) {
					n ++;
					oid_kcode.mohm = r;
				}
			}
		}

		if((n > 1) || ((cfg->lines == 2) && (n > 0))) {
			oid_error(20101); /*gray & black is shorten*/
			e = 1;
		}

		if((n == 0) && (cfg->lines > 2)) {
			oid_error(30102); //white-white break
			e = 1;
		}
	}

	if(!e) {
		int tcode = __tcode(&oid_kcode);
		int kcode = __kcode(&oid_kcode);
		if(tcode == 0) {
			//resistor incorrect ???
			oid_error(E_UNDEF);
			e = 1;
		}

		if(!e) {
			oid_show_status(tcode, kcode);
		}
	}
	return e;
}

int oid_hot_test(struct oid_config_s *cfg)
{
	char i, j, e = 0;
	int mv;

	oid_kcode.mv = 0;

	/*measure voltage*/
	if(cfg->mode == 'i') {
		for(i = 0; i <= cfg->lines; i ++) {
			for(j = i + 1; j <= cfg->lines; j ++) {
				mv = oid_measure_voltage(i, j);
				if(mv > 500) {
					break;
				}
			}
		}
	}
	else { //diagnostic mode
		if((cfg->lines == 1) ||(cfg->lines == 3)) {
			mv = oid_measure_voltage(PIN_SHELL, PIN_BLACK);
		}
		else {
			mv = oid_measure_voltage(PIN_GRAY, PIN_BLACK);
		}
	}

	oid_kcode.mv = mv;
	if(mv < 500) {
		e = 1;
		oid_error(30104); //voltage abnormal
	}
	return e;
}

