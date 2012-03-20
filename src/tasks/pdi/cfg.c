/*
 *	miaofng@2011 initial version
 *
 *
 */
#include "config.h"
#include "ulp_time.h"
#include "flash.h"
#include "crc.h"
#include "sys/malloc.h"
#include <string.h>
#include "ulp/debug.h"
#include "cfg.h"
#include "nvm.h"

#ifdef CONFIG_PDI_FILE_SZ
#define FSZ CONFIG_PDI_FILE_SZ
#else
#define FSZ FLASH_PAGE_SZ //2048bytes
#endif

#ifdef CONFIG_PDI_FILE_NR
#define FNR CONFIG_PDI_FILE_NR
#else
#define FNR 64 //64 files
#endif

static int pdi_ecode = PDI_OK;

static int align(int value, int page_size)
{
	int left = value % page_size;
	value += (left != 0) ? page_size : 0;
	value -= left;
	return value;
}

/*compate the two string, pay attention to special char '*'  , equal return 0*/
static int pdi_match(const char *d, const char *e, int n)
{
	int i;

	assert(d != NULL);
	assert(e != NULL);

	for(i = 0; (i < n) && (*d != '\0') && (*e != '\0'); i ++) {
		if((e[i] != '*') && (e[i] != d[i]))
			break;
	}

	return (i != n);
}

/* locate pid_file*/
static const void* pdi_file(int index)
{
	unsigned sz_ram, ram_pages, cfg_pages, cfg;

	assert(index < FNR);
	sz_ram = (int)__section_end(".nvm.ram") - (int)__section_begin(".nvm.ram");
	sz_ram = align(sz_ram, 4);
	ram_pages = align(sz_ram + 8, FLASH_PAGE_SZ) / FLASH_PAGE_SZ;
	cfg_pages = align(FSZ, FLASH_PAGE_SZ) / FLASH_PAGE_SZ;
	cfg = FLASH_ADDR(FLASH_PAGE_NR - ram_pages - ram_pages - cfg_pages * FNR);
	return (const void *)(cfg + FSZ * index);
}

//get the configuration by serial number, return NULL -> fail
const struct pdi_cfg_s* pdi_cfg_get(const char *sn)
{
	int i = 0;
	const struct pdi_cfg_s *cfg;

	while(1) {
		if(i >= FNR) { //not found
			pdi_ecode = - PDI_ERR_CFG_NOT_FOUND;
			return NULL;
		}

		cfg = pdi_file(i);
		i ++;
		if(sn != NULL) {
			if((cfg->flag & 0x01) == 0 && cfg->nr_of_rules > 0) {
				if(!strcmp(sn, cfg->name))
					break;
			}
		}
		else { //sn == NULL
			if((cfg->flag & 0x01) == 1 || cfg->nr_of_rules <= 0) {
				return cfg;
			}
		}
	}

	//found, to do data integrate check
	if(cyg_crc32((unsigned char *)cfg + sizeof(cfg->crc), FSZ - sizeof(cfg->crc)) != cfg->crc) {
		pdi_ecode = - PDI_ERR_CFG_DAMAGED;
		return NULL;
	}

	return cfg;
}

//get the rule by index, return NULL -> fail
const struct pdi_rule_s* pdi_rule_get(const struct pdi_cfg_s *cfg, int index)
{
	const struct pdi_rule_s *rule = (const struct pdi_rule_s *)((int)cfg + sizeof(const struct pdi_cfg_s));
	assert(cfg != NULL);

	if(index >= cfg->nr_of_rules) {
		pdi_ecode = - PDI_ERR_IN_PARA;
		return NULL;
	}

	return (rule + index);
}

//cmpare the cmd echo with the expected one, return 0 -> pass
int pdi_verify(const struct pdi_rule_s *rule, const void *echo)
{
	int i, n, v, ret = -1;
	char *echo_text;

	assert(rule != NULL);
	assert(echo != NULL);

	switch(rule->echo_type) {
	case PDI_ECHO_HEX:
		n = rule->echo_size * 2;
		echo_text = sys_malloc(n + 1);
		memset(echo_text, '\0', n);
		for(i = 0; i < rule->echo_size; i ++) {
			v = *((unsigned char *) echo + i);
			sprintf(&echo_text[i << 1], "%02X", v & 0xff);
		}
		ret = pdi_match(echo_text, rule->echo_expect, n);
		sys_free(echo_text);
		break;
	case PDI_ECHO_HEX2DEC:
		for(v = 0, i = 0; i < rule->echo_size; i ++) {
			v <<= 8;
			v += *((unsigned char *) echo + i);
		}
		ret = (v != atoi(rule->echo_expect));
		break;
	case PDI_ECHO_ASCII:
		ret = pdi_match(echo, rule->echo_expect, rule->echo_size);
		break;
	default:
		break;
	}

	return ret;
}

#include "shell/cmd.h"

static char *pdi_buf = NULL;
static char *pdi_str = NULL;

static int file_open(const char *name)
{
	struct pdi_cfg_s *cfg;
	pdi_buf = (pdi_buf == NULL) ? sys_malloc(FSZ) : pdi_buf;
	if(pdi_buf == NULL) {
		pdi_ecode = - PDI_ERR_NO_MEM;
		return pdi_ecode;
	}

	pdi_str = pdi_buf + FSZ;
	pdi_str -= strlen(name) + 1;
	strcpy(pdi_str, name);

	cfg = (struct pdi_cfg_s *) pdi_buf;
	memset(cfg, '\0', sizeof(struct pdi_cfg_s));
	cfg->name = pdi_str;
	return 0;
}

static int file_save(void)
{
	struct pdi_cfg_s *cfg = (struct pdi_cfg_s *)pdi_buf;
	struct pdi_rule_s *rule = (struct pdi_rule_s *) (cfg + 1);
	const void *tar;
	int i, offset;

	if(pdi_buf == NULL) {
		pdi_ecode = - PDI_ERR_OP_NO_ALLOWED;
		return pdi_ecode;
	}

	tar = pdi_cfg_get(cfg->name);
	tar = (tar == NULL) ? pdi_cfg_get(NULL) : tar;
	if(tar == NULL) {
		pdi_ecode = - PDI_ERR_TOO_MANY_FILES;
		return pdi_ecode;
	}

	//redirect str pointers to flash
	offset = (int)cfg - (int)tar;
	cfg->name -= offset;
	for(i = 0; i < cfg->nr_of_rules; i ++) {
		rule->echo_expect -= offset;
		rule ++;
	}

	//calculate crc
	cfg->crc = cyg_crc32((unsigned char *)cfg + sizeof(cfg->crc), FSZ - sizeof(cfg->crc));

	//flash write
	flash_Erase(tar, align(FSZ, FLASH_PAGE_SZ)/FLASH_PAGE_SZ);
	flash_Write(tar, cfg, FSZ);

	//free memory
	sys_free(pdi_buf);
	pdi_buf = NULL;
	pdi_str = NULL;
	return 0;
}

static int file_rule_add(const struct pdi_rule_s *rule, const char *expect)
{
	struct pdi_cfg_s *cfg = (struct pdi_cfg_s *)pdi_buf;
	struct pdi_rule_s *tar = (struct pdi_rule_s *) (cfg + 1);

	if(pdi_buf == NULL) {
		pdi_ecode = - PDI_ERR_OP_NO_ALLOWED;
		return pdi_ecode;
	}

	pdi_str -= strlen(expect) + 1;
	strcpy(pdi_str, expect);
	tar += cfg->nr_of_rules;
	if((char *)(tar + 1) > pdi_str) {
		pdi_ecode = - PDI_ERR_CFG_FILE_TOO_BIG;
		return pdi_ecode;
	}

	memcpy(tar, rule, sizeof(struct pdi_rule_s));
	tar->echo_expect = pdi_str;
	cfg->nr_of_rules ++;
	return 0;
}

static int file_relay_add(int relay, int on)
{
	unsigned mask, value;
	struct pdi_cfg_s *cfg = (struct pdi_cfg_s *)pdi_buf;

	if(pdi_buf == NULL) {
		pdi_ecode = - PDI_ERR_OP_NO_ALLOWED;
		return pdi_ecode;
	}

	if(relay < 32) { //relay
		mask = 1 << relay;
		value = (on) ? (1 << relay) : 0;
		cfg->relay &= (~mask);
		cfg->relay |= value;
	}
	else { //relay_ex
		relay -= 32;
		mask = 1 << relay;
		value = (on) ? (1 << relay) : 0;
		cfg->relay_ex &= (~mask);
		cfg->relay_ex |= value;
	}

	return 0;
}


/*handling pdi file download protocol*/
static int cmd_file_func(int argc, char *argv[])
{
	int ret = -1;
	const char *usage = {
		"usage:\n"
		"file name 20TAS5636E	file dnload start\n"
		"file end		file dnload ended\n"
		"file list		list all cfg files\n"
		"file rm name		remove specified cfg file\n"
	};

	if(argc == 3 && !strcmp(argv[1], "name")) {
		ret = file_open(argv[2]);
	}

	if(argc == 2 && !strcmp(argv[1], "end")) {
		ret = file_save();
	}

	if(argc == 2 && !strcmp(argv[1], "list")) {
		const struct pdi_cfg_s *cfg;
		for(int i = 0; i < FNR; i ++) {
			cfg = pdi_file(i);
			if((cfg->flag & 0x01) == 0 && cfg->nr_of_rules > 0) {
				printf("file %03d:	%s;\n", i, cfg->name);
			}
		}
		ret = 0;
	}

	if(argc == 3 && !strcmp(argv[1], "list")) {
		const struct pdi_cfg_s *cfg = pdi_cfg_get(argv[2]);
		const struct pdi_rule_s *rule;
		if(cfg == NULL) {
			pdi_ecode = - PDI_ERR_CFG_NOT_FOUND;
			ret = pdi_ecode;
		}
		else { //print detail
			for(int i = 0; i < 32; i ++) {
				if(cfg->relay & (1 << i))
					printf("relay S%02d %s\n", i, "on");
			}
			for(int i = 0; i < 32; i ++) {
				if(cfg->relay_ex & (1 << i))
					printf("relay S%02d %s\n", i+32, "on");
			}
			for(int i = 0; i < cfg->nr_of_rules; i ++) {
				rule = pdi_rule_get(cfg, i);
				printf("verify ");
				if(rule->type == PDI_RULE_DID) printf("DID ");
				if(rule->type == PDI_RULE_DPID) printf("DPID ");
				if(rule->type == PDI_RULE_JAMA) printf("JAMA ");
				if(rule->type == PDI_RULE_ERROR) printf("ERROR ");

				printf("%02X ", rule->para);

				if(rule->echo_type == PDI_ECHO_HEX) printf("HEX ");
				if(rule->echo_type == PDI_ECHO_HEX2DEC) printf("HEX2DEC ");
				if(rule->echo_type == PDI_ECHO_ASCII) printf("ASCII ");

				printf("%d ", rule->echo_size);
				printf("%s\n", rule->echo_expect);
			}
			ret = 0;
		}
	}

	if(argc == 3 && !strcmp(argv[1], "rm")) {
		const struct pdi_cfg_s *cfg = pdi_cfg_get(argv[2]);
		if(cfg == NULL) { //not found
			pdi_ecode = - PDI_ERR_CFG_NOT_FOUND;
			ret = pdi_ecode;
		}
		else {
			flash_Erase(cfg, align(FSZ, FLASH_PAGE_SZ)/FLASH_PAGE_SZ);
			ret = 0;
		}
	}

	if(ret == 0) {
		printf("##OK##\n");
	}
	else {
		printf("%s", usage);
		printf("##%d##ERROR##\n", ret);
	}
	return ret;
}

const cmd_t cmd_file = {"file", cmd_file_func, "file cmd i/f"};
DECLARE_SHELL_CMD(cmd_file)

static int cmd_verify_func(int argc, char *argv[])
{
	int v, ret = -1;
	struct pdi_rule_s rule;
	const char *usage = {
		"add limit settings to config file, usage:\n"
		"verify DID B4 ASCII 16 20TAS5636E********* //explatation, optional\n"
		"verify DPID 28 HEX 7 000000000000000000 //explation, optional\n"
		"verify JAMA HEX 1 01 //explation, optional\n"
		"verify ERROR HEX 2 8181 //explation, optional\n"
		"verify PART HEX 17 444D2020202039353931302d345A303030 //part NO.\n"
	};

	if(argc == 6) {
		rule.type = PDI_RULE_UNDEF;
		rule.type = (!strcmp(argv[1], "DID")) ? PDI_RULE_DID : rule.type;
		rule.type = (!strcmp(argv[1], "DPID")) ? PDI_RULE_DPID : rule.type;

		sscanf(argv[2], "%x", &v);
		rule.para = v & 0xff;

		rule.echo_type = PDI_ECHO_UNDEF;
		rule.echo_type= (!strcmp(argv[3], "HEX")) ? PDI_ECHO_HEX : rule.echo_type;
		rule.echo_type = (!strcmp(argv[3], "HEX2DEC")) ? PDI_ECHO_HEX2DEC : rule.echo_type;
		rule.echo_type = (!strcmp(argv[3], "ASCII")) ? PDI_ECHO_ASCII : rule.echo_type;

		sscanf(argv[4], "%d", &v);
		rule.echo_size = v & 0xff;

		if(rule.type == PDI_RULE_UNDEF || rule.echo_type == PDI_ECHO_UNDEF || rule.echo_size == 0) {
			pdi_ecode = - PDI_ERR_IN_PARA;
			ret = pdi_ecode;
		}
		else {
			ret = file_rule_add(&rule, argv[5]);
		}
	}

	if(argc == 5) {
		rule.type = PDI_RULE_UNDEF;
		rule.type = (!strcmp(argv[1], "JAMA")) ? PDI_RULE_JAMA : rule.type;
		rule.type = (!strcmp(argv[1], "ERROR")) ? PDI_RULE_ERROR : rule.type;
		rule.type = (!strcmp(argv[1], "PART")) ? PDI_RULE_PART : rule.type;

		rule.echo_type = PDI_ECHO_UNDEF;
		rule.echo_type= (!strcmp(argv[2], "HEX")) ? PDI_ECHO_HEX : rule.echo_type;

		sscanf(argv[3], "%d", &v);
		rule.echo_size = v & 0xff;

		if(rule.type == PDI_RULE_UNDEF || rule.echo_type == PDI_ECHO_UNDEF || rule.echo_size == 0) {
			pdi_ecode = - PDI_ERR_IN_PARA;
			ret = pdi_ecode;
		}
		else {
			ret = file_rule_add(&rule, argv[4]);
		}
	}

	if(ret == 0) {
		printf("##OK##\n");
	}
	else {
		printf("%s", usage);
		printf("##%d##ERROR##\n", ret);
	}
	return ret;
}

const cmd_t cmd_verify = {"verify", cmd_verify_func, "verify cmd i/f"};
DECLARE_SHELL_CMD(cmd_verify)

static int cmd_relay_func(int argc, char *argv[])
{
	int relay, value, ret = -1;
	const char *usage = {
		"add relay settings to config file, usage:\n"
		"relay s00 on\n"
		"relay s01 off\n"
	};

	if(argc == 3) {
		relay = atoi(argv[1] + 1);
		value = (!strcmp(argv[2], "on")) ? 1 : 0;
		ret = file_relay_add(relay, value);
	}

	if(ret == 0) {
		printf("##OK##\n");
	}
	else {
		printf("%s", usage);
		printf("##%d##ERROR##\n", ret);
	}
	return ret;
}

const cmd_t cmd_relay = {"relay", cmd_relay_func, "relay cmd i/f"};
DECLARE_SHELL_CMD(cmd_relay)

