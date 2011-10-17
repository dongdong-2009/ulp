#ifndef __PDI_CFG_H_
#define __PDI_CFG_H_

enum pdi_rule_type {
	PDI_RULE_DID,
	PDI_RULE_DPID,
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
	const char *name;
	unsigned int nr_of_rules;

	//private
	unsigned int crc;
	const struct pdi_rule_s *first;
};

//get the configuration by serial number, return NULL -> fail
const struct pdi_cfg_s* pdi_cfg_get(const char *sn);

//get the rule by index, return NULL -> fail
const struct pdi_rule_s* pdi_rule_get(const struct pdi_cfg_s *cfg, int index);

//cmpare the cmd echo with the expected one, return 0 -> pass
int pdi_verify(const struct pdi_rule_s *rule, const void *echo);


#endif
