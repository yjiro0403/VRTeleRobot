#include "stdafx.h"

void ipc_init(void)
{
	/* Connect to the central server */
	if (IPC_connect(MODULE_NAME) != IPC_OK) {
		fprintf(stderr, "IPC_connect: ERROR!!\n");
		exit(-1);
	}

	IPC_defineMsg(HOGE01_MSG, IPC_VARIABLE_LENGTH, HOGE01_MSG_FMT);
}

void ipc_close(void)
{
	/* close IPC */
	fprintf(stderr, "Close IPC connection\n");
	IPC_disconnect();
}