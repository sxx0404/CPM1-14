#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <linux/rtc.h>

#include "../libs/common/common.h"
#include "../include/gpio.h"

#define CY_IO_NUM	3

static const char *Version = "1.0.1";			//版本号

static int WdtFd = -1;
static int CyIoNums[CY_IO_NUM] = {				// 常高电平
	GPIO_LED1, GPIO_LED2, GPIO_LED3
};


static void IoCb(int Type, int Pin, void *Arg)
{
	char IoName[100] = {0};

	switch (Pin) {
	case GPIO_LOCK_DTC1:
		strncpy(IoName, "NC1", 99);
		break;
	case GPIO_LOCK_DTC2:
		strncpy(IoName, "NC2", 99);
		break;
	case GPIO_RS_DTC1:
		strncpy(IoName, "DECT1", 99);
		break;
	case GPIO_RS_DTC2:
		strncpy(IoName, "DECT2", 99);
		break;
	case GPIO_BI_DTC:
		strncpy(IoName, "PRO-DT", 99);
		break;
	case GPIO_LOCK_RCTL:
		strncpy(IoName, "RCTL", 99);
		break;
	case GPIO_LOCK_BTN:
		strncpy(IoName, "BUTTON", 99);
		break;
	case GPIO_PD_DTC:
		strncpy(IoName, "PowerDown", 99);
		break;
	case GPIO_LOCK_MODE:
		strncpy(IoName, "RF-LN", 99);
		break;
	case GPIO_RST_BTN:
		strncpy(IoName, "RESET", 99);
		break;
	default:
		snprintf(IoName, 99, "%d", Pin);
		break;
	}
	printf("Pin[%s] val is %d\n", IoName, Type);
}

static int OpenSetUart(const char *Path, int Baud, int DataBit, char Parity, int StopBit, int FlowCtrl)
{
	int fd = -1;
	struct termios OldAttr= {0}, NewAttr= {0};
	double st = 0;

//	cfmakeraw(&NewAttr);

	NewAttr.c_iflag  |= IGNBRK;
	NewAttr.c_cflag  |= CREAD | CLOCAL;		// CREAD否则无法接收数据
	// 波特率设置
	if (cfsetospeed(&NewAttr, Baud)<0 || cfsetispeed(&NewAttr, Baud)<0) {
		return -1;
	}

	// 数据位设置
	switch (DataBit) {
	case 5:
		NewAttr.c_cflag |= CS5;
		break;
	case 6:
		NewAttr.c_cflag |= CS6;
		break;
	case 7:
		NewAttr.c_cflag |= CS7;
		break;
	default:
		// 默认8位
		NewAttr.c_cflag |= CS8;
		break;
	}

	// 奇偶校验设置
	switch(Parity) {
	case 'o':
	case 'O':
		// 奇校验
		NewAttr.c_cflag |= PARODD;
	case 'e':
	case 'E':
		// 偶校验
		NewAttr.c_iflag |= INPCK;
		NewAttr.c_cflag |= PARENB;
		break;
	default:
		// 默认无校验
		NewAttr.c_iflag |= IGNPAR;
		break;
	}

	// 设置停止位
	switch (StopBit) {
	case 2:
		NewAttr.c_cflag |= CSTOPB;
		break;
	default:
		NewAttr.c_cflag &= ~CSTOPB;
		break;
	}

	// 设置流控
	switch (FlowCtrl) {
	case 1:
		// 软件流控
		NewAttr.c_iflag |= IXON | IXOFF | IXANY;
		break;
	case 2:
		// 硬件流控
		NewAttr.c_cflag |= CRTSCTS;
		break;
	default:
		// 无流控
		NewAttr.c_cflag &= ~CRTSCTS;
		NewAttr.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
	}

	fd = open(Path, O_RDWR|O_NOCTTY|O_NDELAY|O_NONBLOCK);
	if (fd < 0) {
		return -1;
	}

	st = BooTime();
	while (1) {
		tcsetattr(fd, TCSANOW, &NewAttr);
		if (tcgetattr(fd, &OldAttr)==0 && memcmp(&OldAttr, &NewAttr, sizeof(struct termios))==0) {
			break;
		} else if(st+1 < BooTime()) {
			close(fd);
			fd = -1;
			break;
		}
	}
	tcflush(fd, TCIOFLUSH);

	return fd;
}

// 检查数据包是否完整，同时将完整包放到缓冲开头，修改缓冲剩余长度，如果有完整包，返回完整包长度，没有完整包返回0，出错返回-1(传递的参数错误等)
static int McuFrCheck(uint8_t *pData, uint32_t *pLen)
{
	if (pData==NULL || pLen==NULL || *pLen<=0) {
		return -1;
	}

	for (; *pLen>0; memmove(pData, pData+1, (*pLen = *pLen-1))) {
		if (pData[0] != 0x58) {
			// 包头不对
			continue;
		}
		if (*pLen < 9) {
			// 基本包长度不够
			break;
		}

		if ((pData[1]&0x80)) {
			// 回应给CPU的，SrcAddr必须是0
			if (pData[2] != 0) {
				continue;
			}
		} else {
			// 主发给CPU的，DstAddr必须是0
			if (pData[3] != 0) {
				continue;
			}
		}

		uint32_t DataLen = pData[5]+pData[6]*256;	// 数据域长度

		if (*pLen < DataLen+9) {
			// 还未接收完整
			break;
		}
		if (pData[DataLen+8] != 0x96) {
			// 包尾不对
			continue;
		}
		if ((uint8_t)RvCheckSum(pData+1, DataLen+7) != 0) {
			// 校验出错
			continue;
		} else {
			// 找出了一个完整包
			memmove(pData, pData+1, *pLen-1);	// 去除包头，解析时没有用
			*pLen = *pLen-1;

			return DataLen+8;
		}
	}

	return 0;
}


static void *WdtThread(void *Arg)
{

	while (ioctl(WdtFd, WDIOC_KEEPALIVE,0) == 0) {
		int i=0;

		for (i=0; i<CY_IO_NUM; i++) {
			GpioSet(CyIoNums[i], 0);
			usleep(400000);
		}
		for (i=0; i<CY_IO_NUM; i++) {
			GpioSet(CyIoNums[i], 1);
			usleep(400000);
		}
	}
	printf("Failed to WDIOC_KEEPALIVE, errno=%d\n", errno);

	return NULL;
}

static int CheckSd(void)
{
	int RdLen=-1, fd=open("/proc/mounts", O_RDONLY);
	char tBuf[500] = {0};

	if (fd < 0) {
		printf("Failed to open /proc/mounts, errno=%d\n", errno);
		exit(EXIT_FAILURE);
	}

	do {
		RdLen = read(fd, tBuf, 499);
		if (RdLen>0 && RdLen<=499) {
			tBuf[RdLen] = 0;
			if (strstr(tBuf, "mmcblk0")) {
				tBuf[499] = 1;
				break;
			}
		}
	} while (RdLen >= 499);

	close(fd);

	if (tBuf[499] == 0) {
		return -1;
	} else {
		return 0;
	}

}

void TestSd(void)
{
	int Ret = -1;

	Ret = CheckSd();
	if (Ret < 0) {
		system("/etc/init.d/fstab restart > /dev/null 2>&1;sleep 2");
		Ret =  CheckSd();
	}
	if (Ret) {
		printf("SD is BAD\n");
	} else {
		printf("SD is GOOD\n");
	}
}

int Test3G(void)
{
	printf("Press Enter to cancel Test 3G...");
	fflush(stdout);
	while (1) {
		if (system("ping -qc 1 -I 3g-wan -W 10 114.114.114.114 > /dev/null 2>&1") == 0) {
			printf("\n3G is GOOD\n");
			return 0;
		} else {
			char tData[30];
			struct pollfd tfds = {0};

			tfds.fd = STDIN_FILENO;
			tfds.events = POLLIN | POLLPRI | POLLERR;
			if (poll(&tfds, 1, 1000)>0 && fgets(tData, 30, stdin)) {
				return -1;
			}
		}
	}
}

void TestRTC(void)
{
	if (system("hwclock -s")==0 && system("hwclock -w")==0) {
		if (time(NULL) < 1420041600) {	// 时间小于2015年，修改为2015年
			struct timeval t = {0};

			t.tv_sec	= 1420041600;
			t.tv_usec	= 0;
			settimeofday(&t, NULL);
			system("hwclock -w");
		}
		printf("RTC[rw] is GOOD\n");
	} else {
		printf("RTC[rw] is BAD\n");
	}
}

int TestMCU(void)
{
	int fd = OpenSetUart("/dev/ttyS0", B230400, 8, 'N', 1, 0);
	uint8_t tData[100]= {0}, ErrCnt=0;

	if (fd >= 0) {
		while (ErrCnt < 3) {
			// 构造查询版本包
			tData[0] = 0x58;
			tData[1] = 0x01;
			tData[2] = 0x00;
			tData[3] = 0x01;
			tData[4] = 0x00;
			tData[5] = 0x00;
			tData[6] = 0x00;
			tData[7] = RvCheckSum(tData+1, 6);
			tData[8] = 0x96;

			// 发送出去
			if (write(fd, tData, 9) == 9) {
				double st = BooTime();
				int tLen = -1;
				uint32_t DataLen = 0;

				while (st+1 > BooTime()) {
					tLen = read(fd, tData+DataLen, 100-DataLen);
					if (tLen < 0) {
						break;
					} else if (tLen > 0) {
						DataLen+= tLen;
						while ((tLen=McuFrCheck(tData, &DataLen)) > 0) {
							if (tData[0]==0x81 && tData[1]==0x00 && tData[2]==0x01 &&
									tData[3]==0x00 && tData[4]==0x05 && tData[5]==0x00 && tData[6]==0x00) {
								printf("MCU[comm] is GOOD[%d], Soft Ver=%hhu.%hhu, Hard Ver=%hhu.%hhu\n",
									   ErrCnt, tData[8], tData[7], tData[10], tData[9]);
								close(fd);
								return 0;
							}
							DataLen -= tLen;
							memmove(tData, tData+DataLen, DataLen);
						}
					} else {
						usleep(1000);
					}
				}
			}

			ErrCnt++;
		}
		close(fd);
	}
	printf("MCU[comm] is BAD\n");

	return -1;
}

void TestRs485(void)
{
	int fd = OpenSetUart("/dev/ttyS0", B230400, 8, 'N', 1, 0);
	uint8_t tData[100]= {0}, ErrCnt=0;

	if (fd >= 0) {
		while (ErrCnt < 10) {
			// 构造查询版本包
			tData[0]	= 0x58;
			tData[1]	= 0x02;
			tData[2]	= 0x00;
			tData[3]	= 0x03;
			tData[4]	= 0x01;
			tData[5]	= 0x06;
			tData[6]	= 0x00;
			tData[7]	= 0x58;
			tData[8]	= 0x01;
			tData[9]	= 0x02;
			tData[10]	= 0x00;
			tData[11]	= 0x5B;
			tData[12]	= 0x96;
			tData[13]	= RvCheckSum(tData+1, 12);
			tData[14]	= 0x96;

			// 发送出去
			if (write(fd, tData, 15) == 15) {
				double st = BooTime();
				int tLen=-1;
				uint32_t DataLen=0, Rs485DataLen=0;
				uint8_t tRs485Data[30] = {0};

				while (st+0.1 > BooTime()) {
					tLen = read(fd, tData+DataLen, 100-DataLen);
					if (tLen < 0) {
						break;
					} else if (tLen > 0) {
						DataLen+= tLen;
						while ((tLen=McuFrCheck(tData, &DataLen)) > 0) {
							if (tData[0]==0x03 && tData[1]==0x03 && tData[2]==0x00 && tData[3]==0x00 &&
									tData[4]!=0 && tData[4]<(30-Rs485DataLen) && tData[5]==0) {
								memcpy(tRs485Data+Rs485DataLen, tData+6, tData[4]);
								Rs485DataLen+=tData[4];
								for (; Rs485DataLen>=15; memmove(tRs485Data, tRs485Data+1, --Rs485DataLen)) {
									if (tRs485Data[0]!=0x58 || tRs485Data[1]!=0x01 || tRs485Data[2]!=0x82 ||
											tRs485Data[3]!=0x09 || tRs485Data[4]!=0x00 ||
											tRs485Data[13]!=(uint8_t)CheckSum(tRs485Data, 13) || tRs485Data[14]!=0x96) {
										continue;
									}
									printf("RS485 is GOOD[%d], CardReader's Soft Ver=%hhu.%hhu, Hard Ver=%hhu.%hhu\n",
										   ErrCnt, tRs485Data[9], tRs485Data[10], tRs485Data[11], tRs485Data[12]);
									close(fd);
									return;
								}
							}
							DataLen -= tLen;
							memmove(tData, tData+DataLen, DataLen);
						}
					} else {
						usleep(1000);
					}
				}
			}

			ErrCnt++;
		}
		close(fd);
	}
	printf("RS485 is BAD\n");
}

void TestRFFunc(void)
{
	int fd = OpenSetUart("/dev/ttyS0", B230400, 8, 'N', 1, 0);
	uint8_t tData[100]= {0}, ErrCnt=0;

	if (fd >= 0) {
		while (ErrCnt < 2) {
			// 构造功能测试控制包
			tData[0] = 0x58;
			tData[1] = 0x02;
			tData[2] = 0x00;
			tData[3] = 0x02;
			tData[4] = 0x04;
			tData[5] = 0x01;
			tData[6] = 0x00;
			tData[7] = 0x01;
			tData[8] = RvCheckSum(tData+1, 7);
			tData[9] = 0x96;

			// 发送出去
			if (write(fd, tData, 10) == 10) {
				double st=BooTime(), to=1;
				int tLen = -1;
				uint32_t DataLen = 0;

				while (st+to > BooTime()) {
					tLen = read(fd, tData+DataLen, 100-DataLen);
					if (tLen < 0) {
						break;
					} else if (tLen > 0) {
						DataLen+= tLen;
						while ((tLen=McuFrCheck(tData, &DataLen)) > 0) {
							if (to<10 && tData[0]==0x82 && tData[1]==0x00 && tData[2]==0x02 &&
									tData[3]==0x04 && tData[4]==0x01 && tData[5]==0x00 && tData[6]==0x00) {
								to = 30;
								if (ErrCnt == 0) {
									printf("Press Enter to cancel Test RF Func...");
									fflush(stdout);
								}
							} else if (to>=10 && tData[0]==0x03 && tData[1]==0x02 && tData[2]==0x00 &&
									   tData[3]==0x05 && tData[4]==0x01 && tData[5]==0x00) {
								if (tData[6] == 0x00) {
									printf("\nRF Func is GOOD\n");
								} else {
									printf("\nRF Func is BAD\n");
								}
								close(fd);
								return;
							}
							DataLen -= tLen;
							memmove(tData, tData+DataLen, DataLen);
						}
					} else {
						usleep(1000);
					}

					struct pollfd tfds = {0};

					tfds.fd = STDIN_FILENO;
					tfds.events = POLLIN | POLLPRI | POLLERR;
					if (poll(&tfds, 1, 1)>0 && fgets((char *)tData, 30, stdin)) {
						close(fd);
						return;
					}

				}
			}

			ErrCnt++;
		}
		close(fd);
	}
	printf("\nRF Func is BAD\n");
}

void TestRFPerf(void)
{
	int fd = OpenSetUart("/dev/ttyS0", B230400, 8, 'N', 1, 0);
	uint8_t tData[100]= {0}, ErrCnt=0;

	if (fd >= 0) {
		while (ErrCnt < 2) {
			// 构造性能测试控制包
			tData[0] = 0x58;
			tData[1] = 0x02;
			tData[2] = 0x00;
			tData[3] = 0x02;
			tData[4] = 0x04;
			tData[5] = 0x01;
			tData[6] = 0x00;
			tData[7] = 0x02;
			tData[8] = RvCheckSum(tData+1, 7);
			tData[9] = 0x96;

			// 发送出去
			if (write(fd, tData, 10) == 10) {
				double st=BooTime(), to=1;
				int tLen = -1;
				uint32_t DataLen = 0;

				printf("Press Enter to cancel Test RF Perf...");
				fflush(stdout);
				while (to>10 || st+to>BooTime()) {
					tLen = read(fd, tData+DataLen, 100-DataLen);
					if (tLen < 0) {
						break;
					} else if (tLen > 0) {
						DataLen+= tLen;
						while ((tLen=McuFrCheck(tData, &DataLen)) > 0) {
							if (to<10 && tData[0]==0x82 && tData[1]==0x00 && tData[2]==0x02 &&
									tData[3]==0x04 && tData[4]==0x01 && tData[5]==0x00 && tData[6]==0x00) {
								to = 30;
								if (ErrCnt == 0) {
									printf("Started...");
									fflush(stdout);
								}
							}
							DataLen -= tLen;
							memmove(tData, tData+DataLen, DataLen);
						}
					} else {
						usleep(1000);
					}

					struct pollfd tfds = {0};

					tfds.fd = STDIN_FILENO;
					tfds.events = POLLIN | POLLPRI | POLLERR;
					if (poll(&tfds, 1, 1)>0 && fgets((char *)tData, 30, stdin)) {
						// 构造切回正常模式控制包
						tData[0] = 0x58;
						tData[1] = 0x02;
						tData[2] = 0x00;
						tData[3] = 0x02;
						tData[4] = 0x04;
						tData[5] = 0x01;
						tData[6] = 0x00;
						tData[7] = 0x00;
						tData[8] = RvCheckSum(tData+1, 7);
						tData[9] = 0x96;
						write(fd, tData, 10);
						close(fd);
						printf("Stoped...\n");
						return;
					}
				}
			}

			ErrCnt++;
		}
		close(fd);
	}
	printf("\nTest RF Perf is BAD\n");
}

void PrintMenu(void)
{
	printf("### Test Items:\n"
		   "1.Beep          2.Test WiFi     3.Test RTC\n"
		   "4.Reset MCU     5.Test MCU      6.Test RF Func\n"
		   "7.Test RF Perf\n"
		   "Your choice is:");
	fflush(stdout);
}

void ResetMCU(void)
{
	GpioSet(GPIO_MCU_RST, 0);
	int fd = OpenSetUart("/dev/ttyS0", B230400, 8, 'N', 1, 0);

	if (fd >= 0) {
		double st = BooTime();
		int tLen = -1;
		uint32_t DataLen = 0;
		uint8_t tData[100] = {0};

		usleep(1000);
		GpioSet(GPIO_MCU_RST, 1);
		while (st+5 > BooTime()) {
			tLen = read(fd, tData+DataLen, 100-DataLen);
			if (tLen < 0) {
				break;
			} else if (tLen > 0) {
				DataLen+= tLen;
				while ((tLen=McuFrCheck(tData, &DataLen)) > 0) {
					if (tData[0]==0x03 && tData[1]==0x01 && tData[2]==0x00 &&
							tData[3]==0x02 && tData[4]==0x01 && tData[5]==0x00 && tData[6]==0x01) {
						printf("Succeed to reset MCU\n");
						close(fd);
						return;
					}
					DataLen -= tLen;
					memmove(tData, tData+DataLen, DataLen);
				}
			} else {
				usleep(1000);
			}
		}
		close(fd);
		printf("Failed[2] to reset MCU\n");
	} else {
		GpioSet(GPIO_MCU_RST, 1);
		printf("Failed[1] to reset MCU\n");
	}
}

void TestWiFi(void)
{
	system("mount -o remount /;rm -rf /tmp/tcfg;mkdir -p /tmp/tcfg;cp -rf /overlay/etc/config/* /tmp/tcfg/");
	system("rm -rf /etc/config/*;cp -rf /rom/etc/config/* /etc/config/;cp -f /home/twcfg/* /etc/config/");
	system("/etc/init.d/network restart > /dev/null");
	sleep(3);
	system("rm -rf /overlay/etc/config/*;cp -rf /tmp/tcfg/* /overlay/etc/config/;mount -o remount /");
	printf("Press Enter to cancel Test WiFi...");
	fflush(stdout);

	double st = BooTime();
	char tStr[100];

	while (st+30 > BooTime()) {
		if (system("route -n | grep wlan0 > /dev/null") == 0) {
			printf("\nWiFi is GOOD\n");
			return;
		}
		struct pollfd tfds = {0};

		tfds.fd = STDIN_FILENO;
		tfds.events = POLLIN | POLLPRI | POLLERR;
		if (poll(&tfds, 1, 1)>0 && fgets(tStr, 30, stdin)) {
			return;
		}
	}
	printf("\nWiFi is BAD\n");
}

void Reset3G(void)
{
	// 断电
	GpioSet(GPIO_3G_PWR, 0);
	double st = BooTime();
	while ((st+5)>BooTime() && access("/dev/ttyUSB0", F_OK)==0) {
		usleep(100000);
	}
	if (access("/dev/ttyUSB0", F_OK)==0) {
		printf("Failed to PowerDown 3G, errno:1\n");
		GpioSet(GPIO_3G_PWR, 1);
		return;
	}
	sleep(5);
	// 上电
	GpioSet(GPIO_3G_PWR, 1);
	st = BooTime();
	while ((st+10)>BooTime() && access("/dev/ttyUSB0", F_OK)!=0) {
		usleep(100000);
	}
	if (access("/dev/ttyUSB0", F_OK)!=0) {
		printf("Failed to PowerUp 3G, errno:2\n");
	} else {
		usleep(500000);
		printf("Succeed to reset 3G\n");
	}
}

int main(int argc, const char *argv[])
{
	if (argc != 1) {
		if (argc==2 && strcmp(argv[1],"-v")==0) {
			printf("Soft Ver:%s\nCompile Time:%s %s\n", Version, __DATE__, __TIME__);
			exit(EXIT_SUCCESS);
		} else {
			printf("Usage:\t %s [-v]\n", argv[0]);
			printf("\t-v\t print vesion info.\n");
			exit(EXIT_FAILURE);
		}
	}
	system("date");
	system("killall -q bn");
	int Ret=0, i=0;
	pthread_t Wdt;
	char tInStr[100] = {0};

	while (1) {
		WdtFd = open("/dev/watchdog1", O_WRONLY);
		if (WdtFd>=0 || Ret++>10) {
			break;
		}
		sleep(1);
	}
	if (WdtFd < 0) {
		printf("Failed to Open WDT, errno=%d\n", errno);
		exit(EXIT_FAILURE);
	}
	Ret = 15;
	ioctl(WdtFd, WDIOC_SETTIMEOUT, &Ret);

	GpioInit();
	// 全高电平
	for (i=0; i<CY_IO_NUM; i++) {
		GpioSet(CyIoNums[i], 1);
	}
	// 喂狗和循环IO
	if (pthread_create(&Wdt, NULL, WdtThread, NULL)) {
		printf("Failed to pthread_create WdtThread, errno=%d\n", errno);
		exit(EXIT_FAILURE);
	}

	GpioRegIrq(GPIO_BI_DTC, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_RS_DTC1, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_RS_DTC2, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_LOCK_DTC1, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_LOCK_DTC2, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_LOCK_BTN, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_LOCK_RCTL, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_PD_DTC, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_LOCK_MODE, 0.1, IoCb, NULL);
	GpioRegIrq(GPIO_RST_BTN, 0.1, IoCb, NULL);

	// SD卡检测
	//TestSd();
	// RTC芯片通讯测试
	TestRTC();
	// MCU重启测试
	ResetMCU();
	// MCU通讯测试
	if (TestMCU() == 0) {
		//TestRs485();
		TestRFFunc();
	}

	while (1) {
		PrintMenu();
		if (fgets(tInStr, 100, stdin)) {
			switch (strtol(tInStr, NULL, 10)) {
			case 1:
				GpioSet(GPIO_BUZZ, 1);
				printf("Press Enter to cancel Beep...");
				fflush(stdout);
				fgets(tInStr, 100, stdin);
				GpioSet(GPIO_BUZZ, 0);
				break;
			case 2:
				TestWiFi();
				break;
			case 3:
				TestRTC();
				break;
			case 4:
				ResetMCU();
				break;
			case 5:
				TestMCU();
				break;
			case 6:
				TestRFFunc();
				break;
			case 7:
				TestRFPerf();
				break;
			default:
				printf("Unkown str:%s", tInStr);
				fflush(stdout);
				break;
			}
		}
	}

	return 0;
}




