/*
*  design hints:
*  1, opcode queue is separated into groups, each group's opcode type should be the same.
*  2, slot relay action is performed when group end tag is executed
*  3, dmm trig & measurement will be implemented in case of scan/fscn group type
*
*  miaofng@2014-5-10   initial version
*  1, only matrix relay operation will be buffered in opq(bus&line sw is configured by command 'mode')
*  2, ARM value could be changed only when vm_opq_is_empty
*
* miaofng@2014-8-24
* 1, replace api vm_fetch => vm_update to solve the complex introduced by can frame.
* 2, add new cmd "ROUTE FSCN"
*/

#include "ulp/sys.h"
#include "vm.h"
#include <string.h>
#include "common/circbuf.h"
#include "err.h"
#include <ctype.h>
#include "irc.h"
#include "mxc.h"
#include "bsp.h"
#include "dps.h"

/*scan queue over group boundary support
to fixed cmdline too long issue in case of a big scan_arm such as 152
*/
#define VM_SCAN_OVER_GROUP 1

static circbuf_t vm_opq = {.data = NULL}; /*virtual machine task queue*/
static circbuf_t vm_odata = {.data = NULL};
static opcode_t vm_opcode; //current opcode
static opcode_t vm_opcode_seq; //contain line_stop of seq type
static opcode_t vm_opcode_nxt; //used by vm_prefetch only
static opcode_t vm_opcode_nul = {.type = VM_OPCODE_NULL}; //used by vm_prefetch only
static int vm_scan_arm;
static int vm_scan_cnt;
static char vm_frame_counter;
const char *vm_opcode_types;
static can_msg_t vm_msg;
static char vm_swdebug;
static int vm_measure_delay;
static struct {
	unsigned char busy : 1;
	unsigned char hv : 1;
} vm_flag;

static void vm_opcode_print(opcode_t opcode)
{
	static const char *names[] = {
		"OPEN", "CLOS", "SCAN", "FSCN", "SEQU", "GRUP", "NULL"
	};

	const char *name = "????";
	if(opcode.type <= VM_OPCODE_NULL) {
		name = names[opcode.type];
	}

	printf("%s|%04d|%02d", name, opcode.line, opcode.bus);
}

void vm_init(void)
{
	buf_free(&vm_opq);
	buf_free(&vm_odata);
	buf_init(&vm_opq, CONFIG_IRC_INSWSZ);
	buf_init(&vm_odata, 8);
	vm_opcode = vm_opcode_nul;
	vm_opcode_nxt = vm_opcode_nul;
	vm_opcode_seq = vm_opcode_nul;
	vm_scan_arm = 1;
	vm_scan_cnt = 0;
	vm_frame_counter = 0;
	vm_swdebug = 0;
	vm_flag.busy = 0;
	vm_measure_delay = 0;
}

void vm_abort(void)
{
	irc_error_clear();
}

static void vm_mdelay(int ms)
{
	if(ms > 0) {
		time_t deadline = time_get(ms);
		while(time_left(deadline) > 0) {
			irc_update();
		}
	}
}

/*trig dmm to do measurement and wait for operation finish*/
static int vm_measure(void)
{
	int ecode = -IRT_E_DMM;
	time_t deadline, suspend;

	//apply pre-trig delay
	vm_mdelay(vm_measure_delay);

	trig_set(1);
	deadline = time_get(IRC_DMM_MS);
	suspend = time_get(IRC_UPD_MS);
	while(time_left(deadline) > 0) {
		if(trig_get() == 1) { //vmcomp pulsed
			trig_set(0);
			ecode = 0;
			break;
		}

		if(time_left(suspend) < 0) {
			irc_update();
		}
	}

	irc_error(ecode);
	return ecode;
}

static void vm_emit(int can_id, int do_measure)
{
	memset(&vm_msg, 0x00, sizeof(vm_msg));
	vm_msg.dlc = buf_size(&vm_odata);
	vm_msg.id = can_id;
	vm_msg.id += vm_frame_counter & 0x0F;
	if(vm_msg.dlc > 0) {
		vm_frame_counter = (can_id == CAN_ID_CMD) ? 0 : vm_frame_counter + 1;
		buf_pop(&vm_odata, &vm_msg.data, buf_size(&vm_odata));

		if(vm_swdebug) {
			printf("\n%s: ", (can_id == CAN_ID_CMD) ? "CAN_ID_CMD":"CAN_ID_DAT");
			for(int i = 0; i < vm_msg.dlc; i += sizeof(opcode_t)) {
				opcode_t opcode;
				memcpy(&opcode.value, vm_msg.data+i, sizeof(opcode_t));
				vm_opcode_print(opcode);
				printf(" ");
			}
			printf("\n");
			sys_mdelay(1000);
		}
		else {
			int ecode = irc_send(&vm_msg);
			irc_error(ecode);

			if(can_id == CAN_ID_CMD) {
				ecode = mxc_latch();
				irc_error(ecode);
			}

			if(do_measure) {
				if(vm_flag.hv) {
					/*!!!do not power-up hv until relay settling down
					1, vm_mdelay 5mS
					2, mos gate delay 1mS
					*/
					vm_mdelay(5);

					dps_hv_start();
					vm_measure();
					dps_hv_stop();
				}
				else {
					vm_measure();
				}
			}
		}
	}
}

static void vm_execute(opcode_t opcode, opcode_t seq)
{
	int type = opcode.type;
	int left = buf_left(&vm_odata);
	static int opcode_type_saved;
	int scan, canid;

	vm_flag.busy = 1;

	switch(type) {
	case VM_OPCODE_GRUP:
		scan = (opcode_type_saved == VM_OPCODE_SCAN) || (opcode_type_saved == VM_OPCODE_FSCN);
		canid = (VM_SCAN_OVER_GROUP && scan && vm_scan_cnt) ? CAN_ID_DAT : CAN_ID_CMD;
		vm_emit(canid, scan);
		break;

	default:
		left -= sizeof(opcode_t);
		left -= (type == VM_OPCODE_SEQU) ? sizeof(opcode_t) : 0;
		if(left < 0) {
			//send out the data
			vm_emit(CAN_ID_DAT, 0);
		}

		//space is enough now
		opcode_type_saved = opcode.type;
		buf_push(&vm_odata, &opcode, sizeof(opcode_t));
		if(type == VM_OPCODE_SEQU) {
			opcode_type_saved = seq.type;
			buf_push(&vm_odata, &seq, sizeof(opcode_t));
		}
	}

	vm_flag.busy = 0;
}

int vm_is_opc(void)
{
	int n = (vm_flag.busy) ? 1 : 0;
	if(n == 0) {
		n += buf_size(&vm_opq);
		n += buf_size(&vm_odata);
		n += (vm_opcode.type != VM_OPCODE_NULL) ? 1 : 0;
		n += (vm_opcode_seq.type != VM_OPCODE_NULL) ? 1 : 0;
		n += (vm_opcode_nxt.type != VM_OPCODE_NULL) ? 1 : 0;
		n += vm_scan_cnt;
	}

	return !n;
}

/*
1, destroy vm_opcode
2, modify seq type opcode
3, vm_opcode_nxt is 'seen' only in this function
*/
static void vm_prefetch(opcode_t *result)
{
	opcode_t opcode = vm_opcode_nxt;
	vm_opcode_nxt.type = VM_OPCODE_NULL;

	if(opcode.type == VM_OPCODE_NULL) {
		while(buf_size(&vm_opq) == 0) {
			irc_update();
		}
		buf_pop(&vm_opq, &opcode, sizeof(opcode_t));
	}

	if(opcode.type != VM_OPCODE_GRUP) { //prefectch next
		while(buf_size(&vm_opq) == 0) {
			irc_update();
		}
		buf_pop(&vm_opq, &vm_opcode_nxt, sizeof(opcode_t));
	}

	if(vm_opcode_nxt.type == VM_OPCODE_SEQU) { //exchange
		opcode_t x = opcode;
		opcode_t y = vm_opcode_nxt;

		opcode.type = y.type;
		opcode.bus = x.bus; //bus start
		opcode.line = x.line; //line start

		vm_opcode_nxt.type = x.type;
		vm_opcode_nxt.bus = y.bus; //bus end
		vm_opcode_nxt.line = y.line; //line end

		if(opcode.type == VM_OPCODE_SCAN) {
			if(x.bus != y.bus) {
				irc_error(-IRT_E_OPCODE);
				x.bus = y.bus;
			}
		}
	}

	*result = opcode;
}

void vm_update(void)
{
	/*indicates relay_list processing is finished*/
	int finish = 1;

	if(irc_error_get()) {
		return;
	}

	if(vm_opcode.type == VM_OPCODE_NULL) {
		vm_prefetch(&vm_opcode);
		if(vm_opcode.type == VM_OPCODE_SEQU) {
			vm_prefetch(&vm_opcode_seq);
		}
	}

	int type = vm_opcode.type;
	type = (type == VM_OPCODE_SEQU) ? vm_opcode_seq.type : type;

	//handle scan operation
	if((type == VM_OPCODE_SCAN) || (type == VM_OPCODE_FSCN)) {
		if(vm_opcode.type == VM_OPCODE_SEQU) {
			if(type == VM_OPCODE_SCAN) {
				//only scan one bus - seq.bus
				int n = vm_opcode_seq.line - vm_opcode.line + 1;
				if(n < 0) {
					irc_error(-IRT_E_OPCODE);
				}

				int scan_left = vm_scan_arm - vm_scan_cnt;
				if(n > scan_left) {
					n = scan_left;
					finish = 0;

					opcode_t target = vm_opcode;
					target.line += n - 1;
					target.type = vm_opcode_seq.type;
					vm_execute(vm_opcode, target);
					vm_scan_cnt += n;

					//modify scan base
					vm_opcode.line += n;
				}
				else {
					vm_execute(vm_opcode, vm_opcode_seq);
					vm_scan_cnt += n;
				}
			}

			if(type == VM_OPCODE_FSCN) {
				//scan sequence: line0: bus0..3, line1:bus0..3, ...
				opcode_t target = vm_opcode_seq;
				target.type = vm_opcode.type;
				int n = target.value - vm_opcode.value + 1;
				if(n < 0) {
					irc_error(-IRT_E_OPCODE);
				}

				int scan_left = vm_scan_arm - vm_scan_cnt;
				if(n > scan_left) {
					n = scan_left;
					finish = 0;

					//make new scan target
					target = vm_opcode;
					target.value += n - 1;
					target.type = vm_opcode_seq.type;
					vm_execute(vm_opcode, target);
					vm_scan_cnt += n;

					//modify scan base
					vm_opcode.value += n;
				}
				else {
					vm_execute(vm_opcode, vm_opcode_seq);
					vm_scan_cnt += n;
				}
			}
		}
		else { //non seq type
			vm_scan_cnt ++;
			vm_execute(vm_opcode, vm_opcode_nul);
		}

		//count == arm? insert opcode_grp
		if(vm_scan_cnt == vm_scan_arm) {
			vm_scan_cnt = 0;
			opcode_t opcode_grp = {.type = VM_OPCODE_GRUP};
			vm_execute(opcode_grp, vm_opcode_nul);
		}
	}
	else {
		if(vm_scan_cnt) {
			if((!VM_SCAN_OVER_GROUP) || (type != VM_OPCODE_GRUP)) {
				irc_error(IRT_E_OPCODE);
				vm_scan_cnt = 0;
			}
		}
		vm_execute(vm_opcode, vm_opcode_seq);
	}

	if(finish) {
		vm_opcode = vm_opcode_seq = vm_opcode_nul;
	}
}

int __vm_opq_add(circbuf_t *opq, int tcode, int bus, int line)
{
	opcode_t opcode;
	opcode.value = 0;
	opcode.type = tcode;
	opcode.bus = bus;
	opcode.line = line;

	int ecode = - IRT_E_VM_OPQ_FULL;
	if(buf_left(opq) > sizeof(opcode)) {
		buf_push(opq, &opcode.value, sizeof(opcode));
		ecode = 0;
	}
	return ecode;
}

int _vm_opq_add(circbuf_t *opq, int tcode, const char *relay)
{
	int ecode = 0;

	//verify all 6 char followed are digit
	for(int i = 1; i < 7; i ++) {
		if(!isdigit(relay[i])) {
			ecode = - IRT_E_CMD_FORMAT;
			return ecode;
		}
	}

	//str to int
	int bus, line;
	sscanf(relay + 1, "%d", &bus);
	line = bus % 10000;
	bus /= 10000;
	if((line >= NR_OF_LINE_MAX) || (bus >= NR_OF_BUS_MAX)) {
		ecode = -IRT_E_CMD_PARA;
		return ecode;
	}

	switch(relay[0]) {
	case '@': //first one
		__vm_opq_add(opq, VM_OPCODE_GRUP, 0, 0);
		ecode = __vm_opq_add(opq, tcode, bus, line);
		break;
	case ',': //list
		ecode = __vm_opq_add(opq, tcode, bus, line);
		break;
	case ':': //seq
		ecode = __vm_opq_add(opq, VM_OPCODE_SEQU, bus, line);
		break;
	default:
		ecode = -IRT_E_CMD_FORMAT;
	};
	return ecode;
}

/*<relay_list> = (@bbllll,bbllll:bbllll,bbllll,bbllll:bbllll,...)
*/
int vm_opq_add(int tcode, const char *relay_list)
{
	int ecode = - IRT_E_CMD_FORMAT;
	int i, n = strlen(relay_list);
	if((n < 2) || (relay_list[0] != '(') || (relay_list[n-1] != ')')) {
		return ecode;
	}

	circbuf_t opq_temp;
	memcpy(&opq_temp, &vm_opq, sizeof(vm_opq));
	for(i = 1; i + 7 < n; i += 7) {
		ecode = _vm_opq_add(&opq_temp, tcode, relay_list + i);
		if(ecode) break;
	}
	if(!ecode) {
		if(i + 1 != n) {
			ecode = - IRT_E_CMD_FORMAT;
		}
		else {
			//group is over
			ecode = __vm_opq_add(&opq_temp, VM_OPCODE_GRUP, 0, 1);
			if(!ecode) { //ready? let's go!!!
				memcpy(&vm_opq, &opq_temp, sizeof(vm_opq));
			}
		}
	}
	return ecode;
}

/*arm is used to notify the group length in scan operation,
op will fail if  vm_opq_is_not_empty*/
static int vm_scan_set_arm(int arm)
{
	int ecode = 0;
	ecode = (arm < 1) ? -IRT_E_CMD_PARA : ecode;
	ecode = (!vm_is_opc()) ? -IRT_E_OP_REFUSED : ecode;
	if(!ecode) {
		vm_scan_arm = arm;
	}
	return ecode;
}

int vm_mode(int mode)
{
	int ecode = vm_scan_set_arm(1);
	if(!ecode) {
		vm_flag.hv = 0;
		if(mode == IRC_MODE_HVR) {
			vm_flag.hv = 1;
		}
	}
	return ecode;
}

void vm_dump(void)
{
	//odata
	const char *id_type = ((vm_msg.id & 0xFF0) == CAN_ID_CMD) ? "CAN_ID_CMD":"CAN_ID_???";
	id_type = ((vm_msg.id & 0xFF0) == CAN_ID_DAT) ? "CAN_ID_DAT": id_type;

	printf("ARM: %d/%d\r\n", vm_scan_cnt, vm_scan_arm);
	printf("CAN: %s(0x%03x) ", id_type, vm_msg.id);
	for(int i = 0; i < vm_msg.dlc; i += sizeof(opcode_t)) {
		opcode_t opcode;
		memcpy(&opcode.value, vm_msg.data+i, sizeof(opcode_t));
		vm_opcode_print(opcode);
		printf(" ");
	}
	printf("\n");

	printf("CUR: "); vm_opcode_print(vm_opcode); printf("\r\n");
	printf("SEQ: "); vm_opcode_print(vm_opcode_seq); printf("\r\n");
	printf("NXT: "); vm_opcode_print(vm_opcode_nxt); printf("\r\n");

	//opcode queue
	circbuf_t q;
	memcpy(&q, &vm_opq, sizeof(q));
	opcode_t opcode;
	printf("OPQ: ");
	for(int i = 0; buf_size(&q) > 0;) {
		buf_pop(&q, &opcode, sizeof(opcode));
		vm_opcode_print(opcode); printf(" ");
		if((opcode.type == VM_OPCODE_GRUP) && (opcode.line == 1)) {
			i ++;
			printf("\r\n");
			printf("%03d: ", i);
		}
	}
	printf("\r\n");
	_irc_error_print(irc_error_get(), NULL, 0);
}

#include "shell/cmd.h"
#if 1
static int cmd_route_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"ROUTE [CLOS|OPEN|SCAN|FSCN] (@bbllll:bbllll,bbllll,bbllll)\n"
		"ROUTE ARM 2\n"
		"ROUTE DELAY 16\n"
		"ROUTE DUMP\n"
		"ROUTE DEBUG\n"
	};

	int match = !strcmp(argv[1], "OPEN");
	match |= !strcmp(argv[1], "CLOS");
	match |= !strcmp(argv[1], "SCAN");
	match |= !strcmp(argv[1], "FSCN");
	match |= !strcmp(argv[1], "ARM");
	match |= !strcmp(argv[1], "DELAY");
	if(match && irc_error_get()) {
		irc_error_print(-IRT_E_OP_REFUSED_DUETO_ESYS);
		return 0;
	}

	int ecode = 0, tcode = 0;
	if(!strcmp(argv[1], "OPEN")) {
		tcode = VM_OPCODE_OPEN;
		ecode = vm_opq_add(tcode, argv[2]);
	}
	else if(!strcmp(argv[1], "CLOS")) {
		tcode = VM_OPCODE_CLOS;
		ecode = vm_opq_add(tcode, argv[2]);
	}
	else if(!strcmp(argv[1], "SCAN")) {
		tcode = VM_OPCODE_SCAN;
		ecode = vm_opq_add(tcode, argv[2]);
	}
	else if(!strcmp(argv[1], "FSCN")) {
		tcode = VM_OPCODE_FSCN;
		ecode = vm_opq_add(tcode, argv[2]);
	}
	else if(!strcmp(argv[1], "ARM")) {
		int arm = atoi(argv[2]);
		ecode = vm_scan_set_arm(arm);
	}
	else if(!strcmp(argv[1], "ARM?")) {
		printf("<%+d\n\r", vm_scan_arm);
		return 0;
	}
	else if(!strcmp(argv[1], "DELAY")) {
		vm_measure_delay = atoi(argv[2]);
	}
	else if(!strcmp(argv[1], "DELAY?")) {
		printf("<%+d\n\r", vm_measure_delay);
		return 0;
	}
	else if(!strcmp(argv[1], "DUMP")) {
		vm_dump();
		return 0;
	}
	else if(!strcmp(argv[1], "DEBUG")) {
		vm_swdebug = !vm_swdebug;
		printf("vm_swdebug is %s\r\n", vm_swdebug ? "on" : "off");
		return 0;
	}
	else {
		printf("%s", usage);
		return 0;
	}

	irc_error_print(ecode);
	return 0;
}

const cmd_t cmd_route = {"ROUTE", cmd_route_func, "route related commands"};
DECLARE_SHELL_CMD(cmd_route)
#endif
