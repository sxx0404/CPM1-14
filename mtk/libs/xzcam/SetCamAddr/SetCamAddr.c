#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "../XzCamApi.h"
//../../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc SetCamAddr.c -o SetCamAddr -lxzcam -L./ -lcommon -L../common
int main(int argc, const char * argv[])
{
	if (argc < 3) {
		printf("Usage:%s [IFNAME] [PORT] [CAMMAC] [CAMIP]\n", argv[0]);
		return -1;
	}
	int Ret = -1;
	uint8_t Mac[6]= {0};
	sXzCamHost mHost = {0};

	Ret = XzCamInitHost(&mHost, strtol(argv[2], NULL, 10));
	if (Ret) {
		printf("Failed to XzCamInitHost\n");
		return -1;
	}
	Ret = XzCamSearchCams(&mHost, argv[1], 0);
	printf("XzCamSearchCams ret=%d\n", Ret);
	for (Ret=0; Ret<mHost.CamNum; Ret++) {
		printf("Cam[%d] info:a=%s, p=%hu, m=%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", Ret, inet_ntoa(mHost.pCams[Ret].Addr.sin_addr), ntohs(mHost.pCams[Ret].Addr.sin_port),
			   mHost.pCams[Ret].Mac[0], mHost.pCams[Ret].Mac[1], mHost.pCams[Ret].Mac[2], mHost.pCams[Ret].Mac[3], mHost.pCams[Ret].Mac[4], mHost.pCams[Ret].Mac[5]);
	}
	if (argc < 5) {
		printf("Usage:%s [IFNAME] [PORT] [CAMMAC] [CAMIP]\n", argv[0]);
		return -1;
	}
	Ret = sscanf(argv[3], "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", Mac, Mac+1, Mac+2, Mac+3, Mac+4, Mac+5);
	if (Ret != 6) {
		printf("Wrong MAC[XX:XX:XX:XX:XX:XX] Format:%s\n", argv[3]);
		return -1;
	}
	Ret = XzCamSetAddr(&mHost, argv[1], argv[4], Mac);
	printf("XzCamSetAddr Ret=%d\n", Ret);

	return Ret;
}
