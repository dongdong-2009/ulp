/*
* mcamos communication protocol routine
*
* david@2011 initial version
*/
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "can.h"
#include "ulp_time.h"
#include "sys/sys.h"

#ifdef CONFIG_TASK_APTC131
static const can_msg_t req_flow_msg = {
	.id = 0x7a2,
	.dlc = 8,
	.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
#endif

static const can_bus_t *can_bus;

int usdt_Init(can_cfg_t * cfg)
{
	can_bus = &can1;
	can_bus -> init(cfg);

	return 0;
}

//return the length of can frame, -1 means wrong
int usdt_GetDiagFirstFrame(can_msg_t *pReq, can_filter_t *pResFilter, can_msg_t *pRes)
{
	time_t over_time;
	int num_frame, data_len, mf_flag = 0;

#ifdef CONFIG_TASK_APTC131
	can_filter_t filter = {
		.id = 0x7c2,
		.mask = 0xffff,
		.flag = 0,
	};

	if (pResFilter == NULL)
		pResFilter = &filter;
	can_bus -> filt(pResFilter, 1);
#endif

#ifdef CONFIG_PDI_SDM10
	can_filter_t filter[] = {
		{
			.id = 0x547,
			.mask = 0xffff,
			.flag = 0,
		},
		{
			.id = 0x647,
			.mask = 0xffff,
			.flag = 0,
		},
	};

	if (pResFilter == NULL)
		pResFilter = filter;

	can_bus -> filt(pResFilter, 2);
#endif


	can_bus -> flush();
	can_bus -> send(pReq);						//send request

	//recv reponse
	over_time = time_get(50);
	//get the first frame
	do {
		if (time_left(over_time) < 0)
			return -1;
		if (can_bus -> recv(pRes) == 0) {
			if (pRes -> data[0]>>4 == 0)
				mf_flag = 0;
			else
				mf_flag = 1;
			break;
		}
	} while(1);

	if (mf_flag) {
		data_len = pRes -> data[0] & 0x0f;
		data_len <<= 8;
		data_len |= pRes -> data[1];
		num_frame = (data_len - 6)/7;			//remove the first frame
		if ((data_len - 6) % 7)					//add the last frame
			num_frame ++;
		num_frame ++;							//add the first frame
	} else {
		can_bus -> filt(NULL, 0);				//close the filter
		num_frame = 1;
	}

	return num_frame;
}

/*start a commnication process, send request and receive the response*/
int usdc_GetDiagLeftFrame(can_msg_t *pRes, int msg_len)
{
	time_t over_time;
	int i;

	/* receive the left can frame */
	can_bus -> send(&req_flow_msg);			//send flow control
	over_time = time_get(50);
	i = 1;
	do {
		if (time_left(over_time) < 0)
			return -1;
		if (can_bus -> recv((can_msg_t *)(pRes + i)) == 0) {
			i ++;
			over_time = time_get(50);
		}
	} while(i < msg_len);

	can_bus -> filt(NULL, 0);				//close the filter

	return 0;
}

#if 1
#include "shell/cmd.h"

int cmd_usdt_func(int argc, char *argv[])
{
	int i, msg_len;
	can_msg_t msg_req, msg_res, *pRes;
	can_cfg_t cfg = CAN_CFG_DEF;


	const char * usage = { \
		" usage:\n" \
		" usdt init baud, init the usdt layer\n" \
		" usdt req can_id b0 ... b7, send usdt diag request\n"
	};

	if (argv[1][0] == 'i') {
		sscanf(argv[2], "%d", &cfg.baud); //baud
		usdt_Init(&cfg);
		printf("Init Ok!\n");
	}

	//for geting diagnosis data
	if (strcmp(argv[1], "req") == 0) {

		//get the input can msg
		msg_req.dlc = argc - 3;
		msg_req.flag = 0;
		if (argc > 3)
			sscanf(argv[2], "%x", &msg_req.id); //id
		else 
			return 0;
		for(i = 0; i < msg_req.dlc; i ++) {
				sscanf(argv[3 + i], "%x", (int *)&msg_req.data[i]);
		}

		//get the diagnostic info
		msg_len = usdt_GetDiagFirstFrame(&msg_req, NULL, &msg_res);
		// can_msg_print((can_msg_t const *)&msg_res, "\n");
		// printf("can frame length : %d\n", msg_len);
		if (msg_len > 1) {
			pRes = (can_msg_t *) sys_malloc(msg_len * sizeof(can_msg_t));
			if (pRes == NULL) {
				printf("Buffer Run out\n");
				return 0;
			}
			*pRes = msg_res;
			if(usdc_GetDiagLeftFrame(pRes, msg_len))
				printf("Get Diag Info Error!\n");
			else {
				for (i = 0; i < msg_len; i++)
					can_msg_print((can_msg_t const *)(pRes + i), "\n");
				printf("Get Diag Info Successful\n");
			}
			sys_free(pRes);
		} else if (msg_len == 1) {
			can_msg_print(&msg_res, "\n");
			printf("Get Diag Info Successful\n");
		} else
			printf("Get Diag Info Error!\n");
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}

const cmd_t cmd_usdt = {"usdt", cmd_usdt_func, "usdt cmds"};
DECLARE_SHELL_CMD(cmd_usdt)
#endif
