// TCPComm.cpp: implementation of the CTCPComm class.
//
//////////////////////////////////////////////////////////////////////
#include <list>
#include <net/route.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <linux/if.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "stdafx.h"
#include "TCPCommApp.h"
#include "NormalTool.h"
#include "list.h"
#include "types.h"
#include "main_linuxapp.h"
#include "../include/common.h"
#include "../include/log.h"


//////////////////////////////////////////////////////////////////////

static CTCPComm m_tcp;
int CnctFlg;			// 为1时，需要建立连接，会努力将LOG_STATE_IDLE变为LOG_STATE_DOING_LONGIN，为0会努力变为LOG_STATE_IDLE，为2时，不主动连接或断开，仅外部设置
typLogControl g_LogControl;
//int g_tcpConnected = 0; // 表示与服务器TCP连接是否成功

//////////////////////////////////////////////////////////////////////
#define SEND_TYPE_LOG   0   // 登录数据
#define SEND_TYPE_DATA  1   // 业务数据

CTCPComm::~CTCPComm()
{
	CloseTCP();
}

void CTCPComm::InitTCP(const char *m_strServer, int iRemotePort)
{
	SetSvrIP(m_strServer);
	SetSvrPort(iRemotePort);
}


int CTCPComm::ConnectServer(const char *Ip, uint16_t Port)
{
	InitTCP(Ip, Port);
	//   if(g_tcpConnected)return 1;
	if (IsWriteReady()) {
		Log("Trans", LOG_LV_DEBUG, "No need to connect");
		return 0;
	}
	//关掉旧的
	CloseSocket();
	if(Connect() >= 0) {
		return 0;
	} else {
		return -1;
	}
}


void CTCPComm::CloseTCP()
{
	CloseSocket();
}


static int SendGprsData(unsigned char *data, int iLens, uchar PackType)
{
	char *tStr=NULL;
//    int i;

	if (iLens <= 0) {
		return -1;
	}
	if (iLens>0 && (tStr = (char *)malloc(iLens*3+1))!=NULL) {
		Hex2Str(tStr, data, iLens, ' ');
	}
	Log("Trans", LOG_LV_DEBUG, "SendGprsData, Type=%hhu, Len=%d, Data:%s", PackType, iLens, tStr);
	if (tStr) {
		free(tStr);
		tStr = NULL;
	}

	if ((m_tcp.Send(data, iLens, 0)) == 0) {	// 发送成功
		if (PackType == SEND_TYPE_LOG) {
			g_LogControl.BufStatus = LOG_BUF_SEND_OK;
		}
	} else {	// 发送失败
		g_LogControl.LogSate = LOG_STATE_IDLE;
		if (PackType == SEND_TYPE_LOG) { // 发送普通数据
			g_LogControl.BufStatus = LOG_BUF_SEND_FAIL;
		}
		return -1;
	}

	return 0;
}

int DoTcpSendProcess(void)
{
	int len, iOnceSend= 0;
	uchar HeadData[BUSINESS_HEAD_LEN];
	uchar sendbuf[2624] = {0};
	int sendlen = 0;

	if (g_LogControl.LogSate == LOG_STATE_IDLE) {
		// 尚未连接
		return -1;
	}
	/* I.若有login指令，则要求进行数据发送*/
	if (g_LogControl.BufStatus == LOG_BUF_VALID) {
		/* 2.接着，判断协议服务器TCP连接是否建立*/
		if (!m_tcp.IsWriteReady()) {
			g_LogControl.LogSate = LOG_STATE_IDLE;
			return -2;
		}
		SendGprsData(g_LogControl.buf, g_LogControl.len, SEND_TYPE_LOG);
		return 0;
	} else if (g_LogControl.LogSate == LOG_STATE_HAVE_LONGIN) {
		/* II.判断是否有GPRS发送数据*/
		if (!IsGprsSendDataNull()) {

			/* 2.接着，判断协议服务器TCP连接是否建立*/
			if (!m_tcp.IsWriteReady()) {
				g_LogControl.LogSate = LOG_STATE_IDLE;
				//连接的事情还是由登录来触发吧
				//			g_GprsState.bDoGprsServerConnect = 1;
				return -2;
			}

			/* 3.有业务数据，则要求进行数据发送(先进行业务数据合并)*/
			len = 0;
			for (iOnceSend = 0; iOnceSend < 30; iOnceSend++) {
				len = GprsSendDataPop(sendbuf+sendlen, HeadData);
				Log("Trans", LOG_LV_DEBUG, "Cnt=%d, sendlen=%d, GprsSendDataPop ret=%d", iOnceSend+1, sendlen, len);
				if (len > 0) {	// 存在有效的发送数据
					sendlen+=len;
					if (sendlen > 1024) {
						break;
					}
				} else {	// 发送队列已经为空
					break;
				}
			}
			/* 4.存在发送数据，则进行发送*/
			if (sendlen > 0) {
				SendGprsData(sendbuf, sendlen, SEND_TYPE_DATA);
			}
		}
	}

	/* III.退出*/
	return 0;
}

#define MAX_TCPREADLEN  3000
int DoRecvTCPData(void)
{
	uchar HeadData[BUSINESS_HEAD_LEN];
	//char buff[256];
	//int tret = 0;
	int i;
//	int divtime = 500*1000;
	//int bIPD= 0;
	int readlen;
	uchar readbuf[MAX_TCPSIZE];

	if (g_LogControl.LogSate == LOG_STATE_IDLE) {
		// 尚未连接
		return -1;
	}

	if (!m_tcp.IsConnect()) {
		g_LogControl.LogSate = LOG_STATE_IDLE;
		return 0;
	}
	for (i = 0; i < 30; i++) {
//		WatchDogFeed();
		readlen =m_tcp.Receive(readbuf, MAX_TCPSIZE, 0);
		if (readlen > 0) {
//			if(g_ServerInfo.bServerOnline != 1)
//			{
//				g_ServerInfo.bServerOnline = 1;         //do sever on line
//				DoServerOnline();
//			}
//			lastrecvtime = time_now;
			char tStr[readlen*3+1];

			Hex2Str(tStr, readbuf, readlen, ' ');
			Log("Trans", LOG_LV_DEBUG, "Recv Len=%d, Data:%s.", readlen, tStr);
			if(readlen  < 3) {
				continue;       //心跳包不用放入队列处理
			}
			memcpy(HeadData, &readlen, 2);
			GprsRecvDataPush(readbuf, readlen, HeadData);
//            Log("Trans", LOG_LV_DEBUG, "GprsRecvDataPush ret=%d", push_len);
		} else if (readlen < 0) {
			g_LogControl.LogSate = LOG_STATE_IDLE;
			break;
		} else {
			break;
		}
	}

	return 0;
}

extern volatile int RstProc;
static void server_connect_state_led_indicate(void)
{
	if (RstProc) {
		return;
	}
	// 1.服务器联机(login)，则亮灯
	if (g_LogControl.LogSate == LOG_STATE_HAVE_LONGIN) {
		LedsCtrl(LED_ONLINE, LED_OPEN_OPR);
	} else {
		LedsCtrl(LED_ONLINE, LED_CLOSE_OPR);
	}
}


static void F_do_connect_server(void)
{
	if (CnctFlg == 1) {
		if (g_LogControl.LogSate==LOG_STATE_IDLE || !m_tcp.IsWriteReady()) {
			Log("Trans", LOG_LV_DEBUG, "ip=%s", g_ServerInfo.strServerIP);
			g_LogControl.LogSate = m_tcp.ConnectServer(g_ServerInfo.strServerIP, g_ServerInfo.ServerPort)==0?LOG_STATE_DOING_LONGIN:LOG_STATE_IDLE;
			Log("Trans", LOG_LV_DEBUG, "Connect ret=%hhu.", g_LogControl.LogSate);
		}
	}

}

static void F_do_disconnect_server(void)
{
	if (CnctFlg == 0) {
		if (g_LogControl.LogSate != LOG_STATE_IDLE) {
			Log("Trans", LOG_LV_DEBUG, "Disonnecting CnctFlg=%hhu", CnctFlg);
		}
		// 多操作不会出错
		m_tcp.CloseTCP();
		g_LogControl.LogSate = LOG_STATE_IDLE;
	}
}

void * TaskTCPComm(void *count)
{
	Log("Trans", LOG_LV_WARN, "TaskTCPComm is started.");
//    g_GprsState.SIMCardState = GRPS_MODULE_STATE_NORMAL;
	while (1) {
		/* 与协议服务器断开socket连接*/
		F_do_disconnect_server();

		/* 与协议服务器建立socket连接*/
		F_do_connect_server();

		/* 将业务数据发送给服务器*/
		DoTcpSendProcess();

		/* 系统联机指示灯指示*/
		server_connect_state_led_indicate();

		/* 3G模块状态异常，业务数据暂停处理*/
//        if (F_3g_module_status_abnormal_process() != 0) {
//            continue;
//        }

		// 接收数据
		DoRecvTCPData();

		/* 软件看门狗喂狗*/
		SetDog(TASK_DOG_GPRS);
		usleep(10000);
	}
	return NULL;
}



