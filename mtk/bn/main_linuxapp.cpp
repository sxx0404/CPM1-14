#include <iostream>
#include <fstream>
//#include <strstream>

// C Includes
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "iniFile.h"
#include <stdlib.h>

#include "stdafx.h"

#include <pthread.h>

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>

#include <unistd.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <linux/watchdog.h>

using namespace std;
//#include "RfApp.h"
#include "cfgFile.h"
#include "NormalTool.h"
#include "main_linuxapp.h"
#include "LanMiddle.h"
//#include "CanApp.h"
//#include "device.h"
//#include "cam.h"
//#include "GPRSComm.h"
#include "ManageTransmit.h"
//#include "Rftransmit.h"
#include "TCPCommApp.h"
#include "mgt/mgt.h"
#include "lc/lc.h"
#include "nm/nm.h"
#include "../include/cam.h"
#include "../include/gpio.h"
#include "../include/common.h"
#include "frsdb/frsdb.h"
#include "sd/sdmgt.h"

//typDate g_Now;
typServerInfo g_ServerInfo;

typEvent g_event;
//CCanPort  m_commCanPort1;
#ifdef USE_WATCHDOG
static int wdt_fd;
#endif
extern uchar g_WMCFirmware[];
extern uchar g_WMCVersion[];

//////////////////////////////////////////////////////////////////////////////////////////////////////
//extern "C" void *TaskDefence(void *para);
//extern void * TaskManageComm(void *);
extern void SystemParaInit(void);
extern void OnTime(int iSignal);


//////////////////////////////////////////////////////////////////////////////////////////////////////
pthread_mutex_t  UPRINT;
pthread_mutex_t  lockListGPSend;
pthread_mutex_t  lockListGPRecv;
pthread_mutex_t  lockListMB;
pthread_mutex_t  lockListLB;
pthread_mutex_t  lockmaloc;
pthread_mutex_t  LockCameraCtrl;
pthread_mutex_t  SoftWatchdogMutex;
pthread_mutex_t  lockListSmsSend;
pthread_mutex_t  lock_take_rec_list;
pthread_mutex_t  lock_read_rec_list;
pthread_mutex_t  lock_realstream_opr;

//pthread_mutex_t  lock_recd_proc_opr[MAX_IPC_NUM];

int ProtVer		= 0x32;
int DevCodeSize	= 4;
int g_debug		= 0;

void InitMutex(void)
{
//	int cnt;

	pthread_mutex_init (&UPRINT, NULL);
	pthread_mutex_init (&lockListGPSend, NULL);
	pthread_mutex_init (&lockListGPRecv, NULL);
	pthread_mutex_init (&lockListMB, NULL);
	pthread_mutex_init (&lockListLB, NULL);
	pthread_mutex_init (&lockmaloc, NULL);
	pthread_mutex_init (&LockCameraCtrl, NULL);
	pthread_mutex_init (&SoftWatchdogMutex, NULL);
	pthread_mutex_init (&lockListSmsSend, NULL);
	pthread_mutex_init (&lock_take_rec_list, NULL);
	pthread_mutex_init (&lock_read_rec_list, NULL);
	pthread_mutex_init (&lock_realstream_opr, NULL);

//	for (cnt=0; cnt<MAX_IPC_NUM; cnt++)
//		pthread_mutex_init (&lock_recd_proc_opr[cnt], NULL);
}

void DestoyMutex(void)
{
//	int cnt;

	pthread_mutex_destroy(&UPRINT);
	pthread_mutex_destroy(&lockListGPSend);
	pthread_mutex_destroy(&lockListGPRecv);
	pthread_mutex_destroy(&lockListMB);
	pthread_mutex_destroy(&lockListLB);
	pthread_mutex_destroy(&lockmaloc);
	pthread_mutex_destroy(&LockCameraCtrl);
	pthread_mutex_destroy(&SoftWatchdogMutex);
	pthread_mutex_destroy(&lockListSmsSend);
	pthread_mutex_destroy(&lock_take_rec_list);
	pthread_mutex_destroy(&lock_read_rec_list);
	pthread_mutex_destroy(&lock_realstream_opr);

//	for (cnt=0; cnt<MAX_IPC_NUM; cnt++)
//		pthread_mutex_destroy(&lock_recd_proc_opr[cnt]);

}


#define LEDS_DEV		"/dev/leds"
//int Leds_fd = -1;
static int Leds_Device_Open(void)
{
	return GpioInit();
/*
	Leds_fd = open(LEDS_DEV, O_RDWR);
	printf("Leds_fd:%d\r\n", Leds_fd);
	if ( Leds_fd < 0 )
	{
		perror("can't open led dev!\r\n");
		return -1;
	}
	return 0;
	*/
}

/*

*/
void LedsCtrl( int num, int oper )
{
	int Pin = -1;

	oper=(oper==LED_OPEN_OPR?0:1);
    switch (num) {
	case LED_ONLINE:
		Pin = GPIO_LED1;
		break;
	case LED_RUN:
		Pin = GPIO_LED2;
		break;
	case LED_DOOR_LINK:
		Pin = GPIO_LED3;
		break;
	case LED_CAM1_ONLINE:
		Pin = GPIO_LED4;
		break;
	case LED_CAM2_ONLINE:
		Pin = GPIO_LED5;
		break;
	case LED_3G_NET:
		Pin = GPIO_LED6;
		break;
	case LED_SEND_FILE:
		Pin = GPIO_LED7;
		break;
	case LED_LOCK_MODE:
		Pin = GPIO_LED8;
		break;
	default:
		break;
    }
    if (Pin >= 0) {
		GpioSet(Pin, oper);
    }

/*
#ifdef _use_new_kernel
	if ( oper==LED_OPEN_OPR ) ioctl(Leds_fd, GPIO_LED_OPEN_OPR, num);
	else if ( oper==LED_CLOSE_OPR ) ioctl(Leds_fd, GPIO_LED_CLOSE_OPR, num);
#else
	if ( oper==LED_OPEN_OPR ) ioctl(Leds_fd, LED_OPEN_OPR, num);
	else if ( oper==LED_CLOSE_OPR ) ioctl(Leds_fd, LED_CLOSE_OPR, num);
#endif
*/
}

void all_led_close()
{
  int cnt;

  for ( cnt=0; cnt<LED_NUM; cnt++ )
  {
    LedsCtrl( cnt, LED_CLOSE_OPR);
  }
}
void wdt_keepalive(void)
{
#ifdef USE_WATCHDOG
    ioctl(wdt_fd, WDIOC_KEEPALIVE,0);
#endif
}

void tesetmcucmdcb(void *Arg, int ret)
{
	Log("MCU", LOG_LV_DEBUG, "ARG=%d, ret=%d", (int)Arg, ret);
}

//extern void print_trace (int signo);
int main(int argc, char* argv[])
{
	if (argc==2 && strcmp(argv[1], "-v")==0) {
		printf("Hard Ver:%hhu.%hhu, Soft Ver:%hhu.%hhu\nCompile Time:%s %s\n",
				g_WMCFirmware[1], g_WMCFirmware[0], g_WMCVersion[1], g_WMCVersion[0], __DATE__, __TIME__);
		exit(EXIT_SUCCESS);
	}
	int ret;
	//signal(SIGSEGV, &dump);			// for insystem debug
	//signal(SIGSEGV, &print_trace);			// for insystem debug
	signal(SIGPIPE, SIG_IGN);
	//signal(SIGCHLD, SIG_IGN);

	//打印编译版本
	printf("compile:%s %s\n", __DATE__, __TIME__);

	/* 打开LEDS设备文件*/
	ret = Leds_Device_Open();
	if ( ret!=0 ) exit(EXIT_FAILURE);
	//关闭所有指示灯
    all_led_close();
	// 2.初始化时间
//	time_t timep;
//	struct tm *pTime;
//	time(&timep);
//	pTime=localtime(&timep);

//	g_Now.date = (pTime->tm_year - 100) * 10000 + ((pTime->tm_mon + 1) * 100) + pTime->tm_mday;
//	g_Now.time = pTime->tm_hour * 100 + pTime->tm_min;
//	g_Now.week = pTime->tm_wday;

	// 3.初始化配置参数

	CIniFile iniFile(INIFILE);
	iniFile.ReadFile();
	string strServer = iniFile.GetValue("Sys", "ServerIP", "192.168.11.179");
	strcpy(g_ServerInfo.strServerIP, strServer.c_str());
	strServer = iniFile.GetValue("Sys", "ServerPort", "9811");
	//strServer = iniFile.GetValue("Sys", "ServerPort", "2218");
	g_ServerInfo.ServerPort = atol(strServer.c_str());
//	g_ServerInfo.LocalPort  = 0;
//	g_ServerInfo.bServerOnline = false;
  /* 视频录像和实时流IP地址和端口 */
  strServer = iniFile.GetValue("Sys", "RecdIP", DEF_RCD_SRV_IP);
  strcpy(g_RecdServerInfo.cRecd_ip, strServer.c_str());
  strServer = iniFile.GetValue("Sys", "RecdPort", DEF_RCD_SRV_PORT);
  //strServer = iniFile.GetValue("Sys", "RecdPort", "9603");
  g_RecdServerInfo.iRecd_port = atol(strServer.c_str());

  strServer = iniFile.GetValue("Sys", "RealIP", DEF_LIV_SRV_IP);
  strcpy(g_RecdServerInfo.cReal_ip, strServer.c_str());
  //strServer = iniFile.GetValue("Sys", "RealPort", "9811");
  strServer = iniFile.GetValue("Sys", "RealPort", DEF_LIV_SRV_PORT);
  g_RecdServerInfo.iReal_port = atol(strServer.c_str());

	/* 录像时间配置*/
	strServer = iniFile.GetValue("Sys", "NormalRecdTime", DEF_NORMAL_RECORD_TIME);
	g_RecdServerInfo.normal_recd_time = atol(strServer.c_str());
	strServer = iniFile.GetValue("Sys", "AlarmRecdTime", DED_ALARM_RECORD_TIME);
	g_RecdServerInfo.alarm_recd_time = atol(strServer.c_str());

  printf("g_ServerInfo.strServerIP=%s, g_ServerInfo.ServerPort=%d\n", g_ServerInfo.strServerIP, g_ServerInfo.ServerPort);
  printf("RecdIP=%s,RecdPort=%d...RealIP=%s,RealPort=%d\n",\
    g_RecdServerInfo.cRecd_ip, g_RecdServerInfo.iRecd_port,\
    g_RecdServerInfo.cReal_ip, g_RecdServerInfo.iReal_port);
	printf("normal recd time=%d,alarm recd time=%d\n",\
   g_RecdServerInfo.normal_recd_time, g_RecdServerInfo.alarm_recd_time);

  char v[100];

	/* 软件版本*/
	sprintf(v, "CBM%d.%d", g_WMCVersion[1], g_WMCVersion[0]);

	printf( "The current program version is:%s\n", v);
	iniFile.SetValue("Sys", "Version", v);
	iniFile.WriteFile();

	char *p = GetCfgStr(CFG_FTYPE_SYS, "Sys", "ProtVer", "32");
	ProtVer = strtol(p, NULL, 16);
	free(p);
	Log("INIT", LOG_LV_DEBUG, "Protocol Version:%X", ProtVer);
	InitRecordFile();
	LcInit();
	SystemParaInit();
	ret = MgtInit();
	if (ret) {
		Log("INIT", LOG_LV_DEBUG, "Failed[%d] to MgtInit.", ret);
		exit(1);
	}
	Log("INIT", LOG_LV_DEBUG, "FrDbInit=%d", FrDbInit());
	ret = SdInit();
	unsigned char tbuf[6];
    char macadd[18];
	GetLocalMac(tbuf);
	sprintf(macadd,"%02x:%02x:%02x:%02x:%02x:%02x",
	 tbuf[0],  tbuf[1],  tbuf[2], tbuf[3], tbuf[4], tbuf[5]);
	printf("local mac:%s \n",macadd);
	/* 互斥锁的初始化*/
	InitMutex();
	// 1.初始化设备,指示灯、看门狗等
	// to do 指示灯
#ifdef USE_WATCHDOG
	wdt_fd = open("/dev/watchdog1", O_WRONLY);
	if (wdt_fd == -1) {
		// fail to open watchdog device
		printf("error to open the watch dog\n");
	}
	int timeout = 15;  // 单位秒
	ioctl(wdt_fd, WDIOC_SETTIMEOUT, &timeout);
#endif

// 5. 创建线程

	pthread_t thread1, thread2, thread5, thread8, mthread;

	ret = pthread_create(&mthread, NULL, MgtThread, NULL);
	if (ret) {
		perror("Failed to create MgtThread");
		exit(EXIT_FAILURE);
	}
	if ((ret=pthread_create(&thread5, NULL, LcThread, NULL)) != 0)
	{
		perror("can not create thread 5!\n");
		exit(EXIT_FAILURE);
	}
#if 1
	if ((ret=pthread_create(&thread2, NULL, TaskTCPComm, (void *)NULL)) != 0)
	{
		perror("can not create thread 2!\n");
		exit(EXIT_FAILURE);
	}
#endif
#if 1
	if ((ret=pthread_create(&thread1, NULL, TaskManageComm, (void *)NULL)) != 0)
	{
		perror("can not create thread 1!\n");
		exit(EXIT_FAILURE);
	}
#endif
	if ( (ret = pthread_create(&thread8, NULL, NetManageThread, (void *)NULL)) != 0 )
	{
		perror("can not create thread 8!\n");
		exit(EXIT_FAILURE);
	}
#if 0
	if ( (ret = pthread_create(&sPid, NULL, SdThread, (void *)NULL)) != 0 )
	{
		perror("can not create SdThread!\n");
		exit(EXIT_FAILURE);
	}
#endif
	// 6.创建100ms定时器
	signal(SIGALRM, OnTime);
	struct itimerval itv, oldtv;
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 100*1000;
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 100*1000;
	ret = setitimer(ITIMER_REAL, &itv, &oldtv);  //定时单位100毫秒
	if (ret<0)
	{
		printf("settimer fail\n");
		exit(EXIT_FAILURE);
	}

	//	sleep_t(1);
	// 7.进入主程序
	double t = BooTime();
	while (1)
	{
        sleep(10);
//		check_soft_watchdog();
        if (t+3 < BooTime()) {
			FrDbPrint();
			t = BooTime();
//			McuReset(tesetmcucmdcb, (void*)1);
        }
	}
	DestoyMutex();
	pthread_join(thread5, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread1, NULL);

	return 1;
}

////////////////////////////////////////////////////////////////
#if 0
}
#endif

/**
*1ms定时处理函数
**/
extern volatile UINT lSystemHms ;

static int run_led_state = 0;
extern volatile ulong lSys_ms;
extern volatile int RstProc;

//#define debug_settimer

void OnTime(int iSignal)
{
#ifdef debug_ipc_list_I
	static int bDo = 0;
	REC_EVENT rec_event;
	ulong cur_time;
#endif
	lSystemHms++;	// 100ms interval
	if (!(lSystemHms%10)) {		// 1s process onetime
		if (!RstProc) {
			LedsCtrl(LED_RUN, run_led_state);
		}
		run_led_state = !run_led_state;
		wdt_keepalive();
	}
}

