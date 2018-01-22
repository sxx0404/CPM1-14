#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <linux/videodev.h>
#include <linux/watchdog.h>
#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "Dapp.h"

/*for capture task*/
#define PIC_NORMAL_MANUAL  0
#define PIC_ALARM_MANUAL 1
#define PIC_ALARM_AUTO  2
#define PIC_ALARM_COMPARE 3
#define PIC_HIGH_SPEED 4

typedef struct
{
	uchar   link;  // 1:on  0:off
	uchar   id;
	ushort delay;   //unit: 1s
	uchar   valid;
	uchar   bOpen;
	uchar   bClose;
	ushort	OpenStartTime;
	uchar   bLightState;
}MANUAL_OPEN_LIGHT;

typedef enum {
	_sock_send_init = 0,
	_sock_send_init_fail,
	_sock_send_connecting,
	_sock_send_connect_ok,
	_sock_send_connect_err
}ENUM_SOCK_SEND_STATE;

#define MAX_LIGHT_ID 5
#define DIV_100MS_S 10

/////////////////////////////////////////////////////////////////////////////////
/* TCP录像和实时流调试打印，用于打印发送的原始数据。实际项目去掉。*/
//#define _TCP_SEND_DATA_PRT1
/* TCP录像和实时流调试打印，用于socket状态的打印。*/
#define _TCP_SEND_DATA_PRT2

#endif

