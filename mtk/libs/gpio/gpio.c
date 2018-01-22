#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <poll.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "gpio.h"
#include "../../libs/common/common.h"
#include "../../libs/log/log.h"

#define RALINK_GPIO_GET_MODE		0x03
#define RALINK_GPIO_SET_MODE		0x04

#define	RALINK_GPIO_GET_DIR			0x10
#define	RALINK_GPIO_SET_DIR			0x11
#define	RALINK_GPIO_GET_DATA		0x12
#define	RALINK_GPIO_SET_DATA		0x13
#define	RALINK_GPIO_GET_POL			0x14
#define	RALINK_GPIO_SET_POL			0x15
#define RALINK_GPIO_SET_DIR_IN		0x16
#define RALINK_GPIO_SET_DIR_OUT		0x17
#define	RALINK_GPIO_SET				0x18
#define	RALINK_GPIO_CLR				0x19
#define	RALINK_GPIO_TOG				0x1A
#define	RALINK_GPIO24_GET_DIR		0x20
#define	RALINK_GPIO24_SET_DIR		0x21
#define	RALINK_GPIO24_GET_DATA		0x22
#define	RALINK_GPIO24_SET_DATA		0x23
#define	RALINK_GPIO24_GET_POL		0x24
#define	RALINK_GPIO24_SET_POL		0x25
#define RALINK_GPIO24_SET_DIR_IN	0x26
#define RALINK_GPIO24_SET_DIR_OUT	0x27
#define	RALINK_GPIO24_SET			0x28
#define	RALINK_GPIO24_CLR			0x29
#define	RALINK_GPIO24_TOG			0x2A
#define	RALINK_GPIO40_GET_DIR		0x30
#define	RALINK_GPIO40_SET_DIR		0x31
#define	RALINK_GPIO40_GET_DATA		0x32
#define	RALINK_GPIO40_SET_DATA		0x33
#define	RALINK_GPIO40_GET_POL		0x34
#define	RALINK_GPIO40_SET_POL		0x35
#define RALINK_GPIO40_SET_DIR_IN	0x36
#define RALINK_GPIO40_SET_DIR_OUT	0x37
#define	RALINK_GPIO40_SET			0x38
#define	RALINK_GPIO40_CLR			0x39
#define	RALINK_GPIO40_TOG			0x3A
#define	RALINK_GPIO72_GET_DIR		0x40
#define	RALINK_GPIO72_SET_DIR		0x41
#define	RALINK_GPIO72_GET_DATA		0x42
#define	RALINK_GPIO72_SET_DATA		0x43
#define	RALINK_GPIO72_GET_POL		0x44
#define	RALINK_GPIO72_SET_POL		0x45
#define RALINK_GPIO72_SET_DIR_IN	0x46
#define RALINK_GPIO72_SET_DIR_OUT	0x47
#define	RALINK_GPIO72_SET			0x48
#define	RALINK_GPIO72_CLR			0x49
#define	RALINK_GPIO72_TOG			0x4A

#define GPIO_DEV    "/dev/ralink-gpio"

static int GpioFd = -1;

typedef struct GpioIrqCmd sGpioIrqCmd;
struct GpioIrqCmd {
	sGpioIrqCmd	*Next;
	int			Pin;		// 管脚
	double		HoldT;		// 需要的保持时间
	GpioIrqCb	CbFunc;		// 回调函数
	void		*CbArg;		// 回调函数的自定义参数
};

struct GpioIrqInfo {
	sGpioIrqCmd	Cmd;
	int			OldVal;		// 之前的状态
	int			tVal;		// 中断时的状态
	double			tValTime;	// 上次中断状态的采集
};

static pthread_t thread;
static sGpioIrqCmd *pIrqCmdHead;
static sGpioIrqCmd **ppIrqCmdTail;
static pthread_mutex_t IrqCmdLock;


void TestGpioIrqCb(int Type, int Pin, void *Arg)
{
	Log("GPIO", LOG_LV_DEBUG, "TestGpioIrqCb, Type=%d, Pin=%d, Arg=%d.", Type, Pin, (int)Arg);
//	if (Type && Pin==28) {
//		GpioSet(24, 1);
//		sleep(1);
//		GpioSet(24, 0);
//	}
}

void *GpioThread(void *Arg)
{
	int PinNum=0, i=0, Ret=0;
	struct GpioIrqInfo *PinIrqs = NULL;
	struct pollfd *fds = NULL;
	sGpioIrqCmd *pUnCmds = NULL;		// 刚取出未处理的CMDs
	char tStr[500] = {0};

	while (1) {
		// 检查是否有新的IrqCmd
		if (pIrqCmdHead && pthread_mutex_lock(&IrqCmdLock)==0) {
			//先读取pIrqCmdHead，未加锁，读错也没关系，后边会加锁，而且进入if后还会进一步判断，此时不加锁是为了提高效率
			if (pIrqCmdHead) {
				// 取出UnCmds
				pUnCmds = pIrqCmdHead;
				pIrqCmdHead = NULL;
				ppIrqCmdTail = &pIrqCmdHead;
			}
			pthread_mutex_unlock(&IrqCmdLock);
		}

		// 取出的UnCmds依次处理
		while (pUnCmds) {
			for (i=0; i<PinNum; i++) {
				if(PinIrqs[i].Cmd.Pin == pUnCmds->Pin) {
					// 找到，跳出，外边处理，此时为修改回调
					break;
				}
			}
			if (i >= PinNum) {
				// 没找到，需要构造
				i = PinNum;
				void *r = realloc(PinIrqs, (PinNum+1)*sizeof(struct GpioIrqInfo));
				if (r) {
					PinIrqs = r;
					PinIrqs[i].OldVal	= -1;
					PinIrqs[i].tVal		= -1;
					r = realloc(fds, (PinNum+1)*sizeof(struct pollfd));
					if (r) {
						fds = r;
						snprintf(tStr, 500, "/sys/class/gpio/gpio%d/value", pUnCmds->Pin);
						fds[i].fd = open(tStr, O_RDONLY);
						if (fds[i].fd >= 0) {
							char tBuf[100] = {0};

							read(fds[i].fd, tBuf, 100);	//清掉当前值
							PinIrqs[i].OldVal = atoi(tBuf);
							if (PinIrqs[i].OldVal!=0 && PinIrqs[i].OldVal!=1) {
								PinIrqs[i].OldVal = -1;
							}
							PinNum = i+1;
						} else {
							Log("GPIO", LOG_LV_ERROR, "Failed to open[%s], errno=%d", tStr, errno);
							fds = realloc(fds, PinNum*sizeof(struct pollfd));
							PinIrqs = realloc(PinIrqs, PinNum*sizeof(struct GpioIrqInfo));
						}
					} else {
						PinIrqs = realloc(PinIrqs, PinNum*sizeof(struct GpioIrqInfo));
						Log("GPIO", LOG_LV_ERROR, "Failed to realloc fds to %d, errno=%d", (PinNum+1)*sizeof(struct pollfd), errno);
					}
				} else {
					Log("GPIO", LOG_LV_ERROR, "Failed to realloc PinIrqs to %d, errno=%d", (PinNum+1)*sizeof(struct GpioIrqInfo), errno);
				}
			}
			if (i < PinNum) {
				PinIrqs[i].Cmd		= *pUnCmds;
				PinIrqs[i].Cmd.Next	= NULL;
			}
			// 清理
			sGpioIrqCmd *t = pUnCmds;
			pUnCmds = pUnCmds->Next;
			free(t);
		}

		// 开始poll等待触发
		if (PinNum > 0) {
			int MinT = 100;
			for (i=0; i<PinNum; i++) {
				fds[i].events	= POLLPRI;
				fds[i].revents	= 0;
				if (PinIrqs[i].tVal!=-1 && PinIrqs[i].Cmd.HoldT*1000<MinT) {
					MinT = PinIrqs[i].Cmd.HoldT*1000;
				}
			}
			if (MinT <= 0) {
				MinT = 1;
			}
			Ret = poll(fds, PinNum, MinT);
			if (Ret != 0) {
				// 有或者出错都要读
				for (i=0; i<PinNum; i++) {
					if (Ret<0 || (fds[i].revents&POLLPRI)) {
						int RLen=0;

						lseek(fds[i].fd, 0, SEEK_SET);		// 调到开始读取
						RLen = read(fds[i].fd, tStr, 99);
						if (RLen >= 0) {
							tStr[RLen] = 0;
							RLen = atoi(tStr);
							if (RLen != PinIrqs[i].tVal) {
								PinIrqs[i].tVal		= RLen;
								PinIrqs[i].tValTime	= BooTime();
							}
						} else {
							Log("GPIO", LOG_LV_DEBUG, "Failed to read gpio[%d], errno=%d.", PinIrqs[i].Cmd.Pin, errno);
						}
					}
				}
			}
			for (i=0; i<PinNum; i++) {
				if (PinIrqs[i].tVal!=-1 && (BooTime()-PinIrqs[i].tValTime)>=PinIrqs[i].Cmd.HoldT) {
					if (PinIrqs[i].tVal != PinIrqs[i].OldVal) {
						PinIrqs[i].OldVal = PinIrqs[i].tVal;
						if (PinIrqs[i].Cmd.CbFunc) {
							PinIrqs[i].Cmd.CbFunc(PinIrqs[i].OldVal, PinIrqs[i].Cmd.Pin, PinIrqs[i].Cmd.CbArg);
						}
					}
					PinIrqs[i].tVal	= -1;
				}
			}
			if (Ret < 0) {
				usleep(1000);
			}
		} else {
			usleep(10000);
		}
	}

	return NULL;
}

// CmdBase
static int GetCmdBase(int *pIoNum)
{
	if (pIoNum==NULL || *pIoNum<0) {
		return -1;
	} else if (*pIoNum < 24) {
		return RALINK_GPIO_GET_DIR;
	} else if (*pIoNum < 40) {
		*pIoNum = *pIoNum-24;
		return RALINK_GPIO24_GET_DIR;
	} else if (*pIoNum < 72) {
		*pIoNum = *pIoNum-40;
		return RALINK_GPIO40_GET_DIR;
	} else if (*pIoNum == 72) {
		*pIoNum = 0;
		return RALINK_GPIO72_GET_DIR;
	} else {
		return -1;
	}
}

//
int GpioInit(void)
{
	pIrqCmdHead = NULL;
	ppIrqCmdTail = &pIrqCmdHead;
	pthread_mutex_init(&IrqCmdLock, NULL);

	if (pthread_create(&thread, NULL, GpioThread, NULL)) {
		Log("GPIO", LOG_LV_ERROR, "Failed to create GpioThread, errno=%d.", errno);
		return -1;
	}

	GpioFd = open(GPIO_DEV, O_RDONLY);

	return GpioFd<0?-1:0;
}

// 将对应IO设置为输入模式并读取其值
int GpioGet(int IoNum)
{
	int CmdBase=-1, Val=-1;

	CmdBase = GetCmdBase(&IoNum);
	if (CmdBase < 0) {
		return -1;
	}
	if (ioctl(GpioFd, CmdBase+6, 1<<IoNum) || ioctl(GpioFd, CmdBase+2, &Val)) {
		return -1;
	}
	return ((1<<IoNum)&Val)!=0;
}

// 将对应IO设置为输出模式并设置其值
int GpioSet(int IoNum, int Val)
{
	int CmdBase = -1;

	CmdBase = GetCmdBase(&IoNum);
	if (CmdBase < 0) {
		return -1;
	}
	if (ioctl(GpioFd, CmdBase+7, 1<<IoNum) || ioctl(GpioFd, CmdBase+(Val?8:9), 1<<IoNum)) {
		return -1;
	}

	return 0;
}

// 读取对应IO的输入输出模式，0输入，1输出
int GpioDir(int IoNum)
{
	int CmdBase=-1, Val=-1;

	CmdBase = GetCmdBase(&IoNum);
	if (CmdBase < 0) {
		return -1;
	}
	if (ioctl(GpioFd, CmdBase, &Val)) {
		return -1;
	}
	return ((1<<IoNum)&Val)!=0;
}

// 读取对应IO的值，不改变其输入输出模式
int GpioStat(int IoNum)
{
	int CmdBase=-1, Val=-1;

	CmdBase = GetCmdBase(&IoNum);
	if (CmdBase < 0) {
		return -1;
	}
	if (ioctl(GpioFd, CmdBase+2, &Val)) {
		return -1;
	}
	return ((1<<IoNum)&Val)!=0;
}

// 失败返回全FF
uint32_t GpioModeGet(void)
{
	uint32_t Val = 0;

	if (ioctl(GpioFd, RALINK_GPIO_GET_MODE, &Val)) {
		return ~0;
	}

	return Val;
}

// 注意，请谨慎使用
int GpioModeSet(uint32_t Mode)
{
	if (ioctl(GpioFd, RALINK_GPIO_SET_MODE, Mode)) {
		return -1;
	}

	return 0;
}
// HoldT为改变保持了多长时间，注册后应该很快(HoldT后)回调（注册改变则不会很快回调），以指示当前值
int GpioRegIrq(int Pin, double HoldT, GpioIrqCb Cb, void *Arg)
{
	if (Pin<0 || Pin>72 || Cb==NULL) {
		return -1;
	}
	char tStr[100] = {0};
	int Fd;
	sGpioIrqCmd *pCmd = NULL;

	// sys下初始化设置
	Fd = open("/sys/class/gpio/export", O_WRONLY);
	if (Fd >= 0) {
		snprintf(tStr, 99, "%d", Pin);
		write(Fd, tStr, strlen(tStr));
		close(Fd);
	}
	snprintf(tStr, 100, "/sys/class/gpio/gpio%d/direction", Pin);
	Fd = open(tStr, O_WRONLY);
	if (Fd < 0) {
		return -1;
	}
	if (write(Fd, "in", strlen("in")) != strlen("in")) {
		close(Fd);
		return -1;
	}
	close(Fd);
	snprintf(tStr, 100, "/sys/class/gpio/gpio%d/edge", Pin);
	Fd = open(tStr, O_WRONLY);
	if (Fd < 0) {
		return -1;
	}
	if (write(Fd, "both", strlen("both")) != strlen("both")) {
		close(Fd);
		return -1;
	}
	close(Fd);

	// Cmd开始构造
	pCmd = calloc(1, sizeof(sGpioIrqCmd));
	pCmd->Pin	= Pin;
	pCmd->HoldT	= HoldT;
	pCmd->CbFunc= Cb;
	pCmd->CbArg	= Arg;
	pCmd->Next	= NULL;

	if (pthread_mutex_lock(&IrqCmdLock) == 0) {
		*ppIrqCmdTail	= pCmd;
		ppIrqCmdTail	= &(pCmd->Next);
		pthread_mutex_unlock(&IrqCmdLock);
	} else {
		free(pCmd);
		return -1;
	}

	return 0;
}


/*
void GpioPrintReg(void)
{
    int Ret = -1;
    uint32_t Val = -1;
    double t=BooTime();

	fprintf(stderr, "t=%.13G\n", t);
    Ret = ioctl(GpioFd, RALINK_GPIO24_GET_DIR, &Val);
    fprintf(stderr, "RALINK_GPIO24_GET_DIR, ret=%d, v=%08X\n", Ret, Val);
    Ret = ioctl(GpioFd, RALINK_GPIO24_GET_DATA, &Val);
    fprintf(stderr, "RALINK_GPIO24_GET_DATA, ret=%d, v=%08X\n", Ret, Val);
    Ret = ioctl(GpioFd, RALINK_GPIO24_GET_POL, &Val);
    fprintf(stderr, "RALINK_GPIO24_GET_POL, ret=%d, v=%08X\n", Ret, Val);
    Ret = ioctl(GpioFd, RALINK_GPIO40_GET_DIR, &Val);
    fprintf(stderr, "RALINK_GPIO40_GET_DIR, ret=%d, v=%08X\n", Ret, Val);
    Ret = ioctl(GpioFd, RALINK_GPIO40_GET_DATA, &Val);
    fprintf(stderr, "RALINK_GPIO40_GET_DATA, ret=%d, v=%08X\n", Ret, Val);
    Ret = ioctl(GpioFd, RALINK_GPIO40_GET_POL, &Val);
    fprintf(stderr, "RALINK_GPIO40_GET_POL, ret=%d, v=%08X\n", Ret, Val);
}
*/
/*
int main(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}
	GpioInit();
	switch (argv[1][0]) {
	case 'w':
		if (argc == 4) {
			printf("ret=%d\n",GpioSet(atoi(argv[2]), atoi(argv[3])));
		}
		break;
	case 'r':
		if (argc == 3) {
			int v = GpioGet(atoi(argv[2]));
			printf("v=%d\n", v);
		} else {
			int v=0, Ret=-1;

			Ret = ioctl(GpioFd,RALINK_GPIO_GET_DIR, &v);
			printf("r=%d,DIR00=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO_GET_DATA, &v);
			printf("r=%d,DAT00=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO_GET_POL, &v);
			printf("r=%d,POL00=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO24_GET_DIR, &v);
			printf("r=%d,DIR24=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO24_GET_DATA, &v);
			printf("r=%d,DAT24=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO24_GET_POL, &v);
			printf("r=%d,POL24=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO40_GET_DIR, &v);
			printf("r=%d,DIR40=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO40_GET_DATA, &v);
			printf("r=%d,DAT40=%08X\n", Ret, v);
			Ret = ioctl(GpioFd,RALINK_GPIO40_GET_POL, &v);
			printf("r=%d,POL40=%08X\n", Ret, v);
		}
		break;
	case 'i':
		if (argc >= 3) {
			int i = 0;

			for (i=2; i<argc; i++) {
				int Pin = atoi(argv[i]);

				printf("GpioRegIrq ret=%d\n", GpioRegIrq(Pin, 0.001, TestGpioIrqCb, (void*)i-1));

			}
			while (1) {
				sleep(1);
			}
//			char tStr[100] = {0};
//			int Fd;
//			struct pollfd fds = {0};
//
//			Fd = open("/sys/class/gpio/export", O_WRONLY);
//			if (Fd >= 0) {
//				write(Fd, argv[2], strlen(argv[2]));
//				close(Fd);
//			}
//
//			snprintf(tStr, 100, "/sys/class/gpio/gpio%d/direction", Pin);
//			Fd = open(tStr, O_WRONLY);
//			if (Fd < 0) {
//				printf("Failed to open, errno=%d\n", errno);
//				return -1;
//			}
//			write(Fd, "in", strlen("in"));
//			close(Fd);
//
//			snprintf(tStr, 100, "/sys/class/gpio/gpio%d/edge", Pin);
//			Fd = open(tStr, O_WRONLY);
//			if (Fd < 0) {
//				printf("Failed to open, errno=%d\n", errno);
//				return -1;
//			}
//			write(Fd, "both", strlen("both"));
//			close(Fd);
//
//			snprintf(tStr, 100, "/sys/class/gpio/gpio%d/value", Pin);
//			Fd = open(tStr, O_RDONLY);
//			if (Fd < 0) {
//				printf("Failed to open, errno=%d\n", errno);
//				return -1;
//			}
//
//			fds.fd		= Fd;
//			fds.events	= POLLPRI;
//			int OldV=0, tV=-1;
//			double tVt=tClockTime(7);
//			int Cnt = 0;
//			while (1) {
//				int Ret = 0;
//
//				Ret = poll(&fds, 1, 1);
//				if ((fds.revents&POLLPRI)) {
//					lseek(Fd, 0, SEEK_SET);
//					Ret = read(Fd, tStr, 99);
//					if (Ret >= 0) {
//						tStr[Ret] = 0;
//						Ret = atoi(tStr);
//						if (Ret != tV) {
//							tV = Ret;
//							tVt = tClockTime(7);
//							Cnt++;
//						}
//					}
//				}
//				if (tV!=-1 && (tClockTime(7)-tVt > 0.01)) {
//					if (tV != OldV) {
//						OldV = tV;
//						printf("t=%.13G, V=%d, Cnt=%d\n", tClockTime(7), OldV, Cnt);
//					} else {
//						printf("t=%.13G, sV=%d, Cnt=%d\n", tClockTime(7), OldV, Cnt);
//					}
//					tV = -1;
//					Cnt = 0;
//				}
//
//				//printf("t=%.13G, poll ret=%d, errno=%d, re=%hd\n", tClockTime(7), Ret, errno, fds.revents);
//			}
		}
		break;
//	default:
//		usage(argv[0]);
	}

	return 0;
}
*/
