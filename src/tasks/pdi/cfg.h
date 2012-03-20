#ifndef __PDI_CFG_H_
#define __PDI_CFG_H_

enum {
	PDI_OK,
	PDI_ERR_CFG_NOT_FOUND,
	PDI_ERR_CFG_DAMAGED,
	PDI_ERR_IN_PARA, /*function input para incorrect*/
	PDI_ERR_NO_MEM,
	PDI_ERR_OP_NO_ALLOWED,
	PDI_ERR_TOO_MANY_FILES,
	PDI_ERR_CFG_FILE_TOO_BIG,
};

enum pdi_rule_type {
	PDI_RULE_JAMA,
	PDI_RULE_DID,
	PDI_RULE_DPID,
	PDI_RULE_ERROR,
	PDI_RULE_PART,
	PDI_RULE_UNDEF,
};

enum pdi_echo_type {
	PDI_ECHO_HEX,
	PDI_ECHO_HEX2DEC,
	PDI_ECHO_ASCII,
	PDI_ECHO_UNDEF,
};

struct pdi_rule_s { /*8 bytes per rule*/
	unsigned char type; //ref pdi_rule_type
	unsigned char para; //rule para, such as DID number
	unsigned char echo_size; //echo data bytes

	//private
	unsigned char echo_type; //ascii, hex, hex2dec
	const char *echo_expect; // always string
};

struct pdi_cfg_s {
	unsigned crc;
	unsigned flag; //bit 0 => 1 empty

	//public
	const char *name;
	int nr_of_rules;
	unsigned relay;
	unsigned relay_ex;
};

//get the configuration by serial number, return NULL -> fail
const struct pdi_cfg_s* pdi_cfg_get(const char *sn);

//get the rule by index, return NULL -> fail
const struct pdi_rule_s* pdi_rule_get(const struct pdi_cfg_s *cfg, int index);

//cmpare the cmd echo with the expected one, return 0 -> pass
int pdi_verify(const struct pdi_rule_s *rule, const void *echo);


#endif
