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
#include "shell/cmd.h"
#include "ulp/debug.h"

#ifdef CONFIG_TASK_APTC131
static const can_msg_t req_flow_msg = {
	.id = 0x7a2,
	.dlc = 8,
	.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
#endif

#ifdef CONFIG_PDI_SDM10
static const can_msg_t req_flow_msg = {
	.id = 0x247,
	.dlc = 8,
	.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
#endif

#ifdef CONFIG_PDI_DM
static const can_msg_t req_flow_msg = {
	.id = 0x607,
	.dlc = 8,
	.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
#endif

static const can_bus_t *can_bus;

int usdt_Init(can_bus_t const *pcan)
{
	can_bus = pcan;

	return 0;
}

//return the length of can frame, -1 means wrong
int usdt_GetDiagFirstFrame(can_msg_t const *pReq, int req_len, can_filter_t const *pResFilter, can_msg_t *pRes, int *p_msg_len)
{
	time_t over_time;
	int num_frame, data_len, i, mf_flag = 0;

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

#ifdef CONFIG_PDI_DM
	can_filter_t filter[] = {
		{
			.id = 0x7DA,
			.mask = 0xffff,
			.flag = 0,
		},
		{
			.id = 0x608,
			.mask = 0xffff,
			.flag = 0,
		},
	};

	if (pResFilter == NULL)
		pResFilter = filter;

	can_bus -> filt(pResFilter, 2);
#endif

	assert(pReq);
	assert(pRes);

	can_bus -> flush();
	if(req_len == 1)
		can_bus -> send(pReq);						//send request
	else if (req_len > 1) {
		can_bus -> send(pReq);
		over_time = time_get(50);					//get the flow control msg reponse
		do {
			if (time_left(over_time) < 0)
				return 1;
			if (can_bus -> recv(pRes) == 0)
				break;
		} while(1);
		for (i = 1; i < req_len; i++) {
			can_bus -> send(pReq + i);
		}
	} else {
		return 1;
	}

#ifdef CONFIG_TASK_APTC131
	if (req_len > 1) {								//remove the "03 7f 23 78 0 0 0 0" useless msg
		over_time = time_get(200);
		do {
			if (time_left(over_time) < 0)
				return 1;
			if (can_bus -> recv(pRes) == 0) {
				if(pRes->data[3] == 0x7f)
					return 1;
				else
					break;
			}
		} while(1);
	}
#endif

	//recv reponse, get the first frame
	over_time = time_get(200);
	do {
		if (time_left(over_time) < 0)
			return 1;
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
		data_len |= (unsigned char)pRes -> data[1];
		num_frame = (data_len - 6)/7;			//remove the first frame
		if ((data_len - 6) % 7)					//add the last frame
			num_frame ++;
		num_frame ++;							//add the first frame
	} else {
		can_bus -> filt(NULL, 0);				//close the filter
		num_frame = 1;
	}

	*p_msg_len = num_frame;

	return 0;
}

/*start a commnication process, send request and receive the response*/
int usdt_GetDiagLeftFrame(can_msg_t *pRes, int msg_len)
{
	time_t over_time;
	int i;

	assert(pRes);

	/* receive the left can frame */
	can_bus -> send(&req_flow_msg);			//send flow control
	over_time = time_get(100);
	i = 1;
	do {
		if (time_left(over_time) < 0)
			return -1;
		if (can_bus -> recv((can_msg_t *)(pRes + i)) == 0) {
			i ++;
			over_time = time_get(100);
		}
	} while(i < msg_len);

	can_bus -> filt(NULL, 0);				//close the filter

	return 0;
}

#if 1

int cmd_usdt_func(int argc, char *argv[])
{
	int i, msg_len = 0, req_len = 0;
	can_msg_t msg_req[2], msg_res, *pRes;

	const char * usage = { \
		" usage:\n" \
		" usdt init, init the usdt with can_bus\n" \
		" usdt req can_id b0 ... b7, send usdt diag request\n"
	};

	if (argv[1][0] == 'i') {
		usdt_Init(&can1);
		printf("Init Ok!\n");
	}

	//for geting diagnosis data
	if (strcmp(argv[1], "req") == 0) {
		//get the input can msg
		req_len = (argc - 3) >> 3;
		msg_req[0].dlc = argc - 3;
		if (argc > 3) {
			sscanf(argv[2], "%x", &msg_req[0].id);		//id
		}
		msg_req[0].flag = 0;

		if (req_len == 1) {
			for(i = 0; i < msg_req[0].dlc; i ++)
				sscanf(argv[3 + i], "%x", (int *)&msg_req[0].data[i]);
		} else if (req_len > 1) {
			msg_req[1].id = msg_req[0].id;
			msg_req[1].dlc = msg_req[0].dlc - 8;
			msg_req[1].flag = 0;
			msg_req[0].dlc = 8;
			for(i = 0; i < msg_req[0].dlc; i ++)
				sscanf(argv[3 + i], "%x", (int *)&msg_req[0].data[i]);
			for(i = 0; i < msg_req[1].dlc; i ++)
				sscanf(argv[11 + i], "%x", (int *)&msg_req[1].data[i]);
		} else
			return 0;

		//get the diagnostic info
		if (usdt_GetDiagFirstFrame(msg_req, req_len, NULL, &msg_res, &msg_len)) {
			printf("Get Diag Info Error!\n");
			return 0;
		}
		if (msg_len > 1) {
			pRes = (can_msg_t *) sys_malloc(msg_len * sizeof(can_msg_t));
			if (pRes == NULL) {
				printf("Buffer Run out\n");
				return 0;
			}
			*pRes = msg_res;
			if(usdt_GetDiagLeftFrame(pRes, msg_len))
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
		}
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
