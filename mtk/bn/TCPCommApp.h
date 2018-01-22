// TCPComm.h: interface for the CTCPComm class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TCPCOMM_H__C1D4CE5A_4240_4C02_BDDA_DB1159D691D6__INCLUDED_)
#define AFX_TCPCOMM_H__C1D4CE5A_4240_4C02_BDDA_DB1159D691D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if 0
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include	<string>
#include <net/if.h>
#endif

//#include "rijndael.h"
#include <sys/socket.h>
#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include  "cls_socket.h"
#include <string>
#define MAXLINE 80
#define SERV_PORT 8888

using namespace std;

#define MAX_TCPSIZE  4000	// 单包有可能比较大

/* 调试:有线网络与平台通信(实际程序屏蔽该宏定义) */
//#define _debug_use_eth0_bussiness

class CTCPComm: public CLS_TCPClient
{
public:
	void	CloseTCP();
//	int		RecvData(unsigned char *data);
//	int		WriteTCPData(unsigned char *data, int iLens);
//    int     ResetTCPRecv();
//    int     SendTcpPack(void);
	int     ConnectServer(const char *Ip, uint16_t Port);

	CTCPComm() {}
	virtual ~CTCPComm();

private:
	void	InitTCP(const char *strIP,  int iRemotePort);

//	list<typTcpData> m_ListTcpData;
//	int		m_sockSend;
//	int		m_sockRecv;
//	int		msecond_sockSend;
//	int       mlog_sockSend ;

//	struct	sockaddr_in srvRemoteaddr;
//	struct	sockaddr_in srvLocaladdr;
//	struct	sockaddr_in srvClintaddr;
//	struct	sockaddr_in srvSecondRemoteaddr;
//	struct	sockaddr_in srvLogRemoteaddr;

//      struct	sockaddr_in servaddr;
//	struct	sockaddr_in   cliaddr;
//	struct 	timeval		tv_timeout;

//	string		m_strServer;

//	char 		g_AESKey[16];

//	fd_set readfds, bakupfds;

};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern int CnctFlg;			// 为1时，需要建立连接，会努力将LOG_STATE_IDLE变为LOG_STATE_DOING_LONGIN，为0会努力变为LOG_STATE_IDLE，-2，不主动连接或断开，仅外部设置
extern typLogControl g_LogControl;

extern void * TaskTCPComm(void *count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !defined(AFX_TCPCOMM_H__C1D4CE5A_4240_4C02_BDDA_DB1159D691D6__INCLUDED_)
