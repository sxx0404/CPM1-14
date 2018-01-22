// cls_socket.cpp: implementation of the cls_socket class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdio.h"
#include <sys/timeb.h>
#include "stdafx.h"

#include "cls_socket.h"
//#include "mdcinfo.h"
//#include "GPRSComm.h"
#include "../include/log.h"

//#pragma comment(lib,"ws2_32.lib")

//#ifdef _DEBUG
//#undef THIS_FILE
//static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
//#endif

#define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)

typedef struct _TCP_KEEPALIVE {
    unsigned long onoff;
    unsigned long keepalivetime;
    unsigned long keepaliveinterval;
}TCP_KEEPALIVE;

int CLS_Socket::g_iSockCount=0;
extern	long			m_time;
extern	short			m_millitm;

//extern int g_tcpConnected;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLS_Socket::CLS_Socket()
{
	m_iSocket= INVALID_SOCKET;
	m_iStatus=SS_UNUSED;
//	m_iSocketError=0;
#ifdef _WIN32
	WSADATA wsaData;
	if(!g_iSockCount)
	{
		if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
			WSACleanup();
	}
	g_iSockCount++;
#endif
}

#if 0
CLS_Socket::CLS_Socket(int iSocket, int iStatus)
{
	m_iSocket=iSocket;
	m_iStatus=iStatus;
	m_iSocketError=0;

}
#endif

CLS_Socket::~CLS_Socket()
{
	CloseSocket();
#ifdef _WIN32
	g_iSockCount--;
	if(!g_iSockCount)
		WSACleanup();
#endif
}

int CLS_Socket::SetSockOpt(int iLevel, int iOptName, const char *pOptVal, int iOptLen)
{
	return setsockopt(m_iSocket, iLevel, iOptName, pOptVal, iOptLen);
};

int CLS_Socket::GetSockOpt(int iLevel, int iOptName, char *pOptVal, int * pOptLen)
{
	//return getsockopt(m_iSocket, iLevel, iOptName, pOptVal, pOptLen);
	return 0;
}

int CLS_Socket::CreateSocket()
{
	if(m_iSocket != INVALID_SOCKET)
		CloseSocket();

	m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(m_iSocket == INVALID_SOCKET)
	{
		m_iStatus = SS_UNUSED;
		return -1;
	}

	m_iStatus=SS_CREATED;

	return m_iSocket;
}

int CLS_Socket::CloseSocket()
{
	int iRet=0;
	if(m_iSocket != INVALID_SOCKET)
	{
		iRet=closesocket(m_iSocket);
		m_iSocket= INVALID_SOCKET;
	}
	m_iStatus=SS_UNUSED;
	return iRet;
}

int CLS_Socket::SetNonBlock(void)
{
	if(fcntl(m_iSocket, F_SETFL, fcntl(m_iSocket, F_GETFL, 0) | O_NONBLOCK)) {
        Log("Trans", LOG_LV_DEBUG, "Failed to set socket to nb, errno=%d.", errno);
		return -1;
	}
	return 0;
}

int CLS_Socket::SetSocketProp()
{
//	int	buf_size = TCP_BUFF_MAX;
	int OptV=1, Ret=-1;

	Ret = setsockopt(m_iSocket, IPPROTO_TCP, TCP_NODELAY, &OptV, sizeof(int));
//	Log("Trans", LOG_LV_DEBUG, "setsockopt, TCP_NODELAY, ret=%d, errno=%d", Ret, errno);
	Ret = setsockopt(m_iSocket, SOL_SOCKET, SO_KEEPALIVE, &OptV, sizeof(int));
//	Log("Trans", LOG_LV_DEBUG, "setsockopt, SO_KEEPALIVE, ret=%d, errno=%d", Ret, errno);
	OptV = 60;
	Ret = setsockopt(m_iSocket, IPPROTO_TCP, TCP_KEEPIDLE, &OptV, sizeof(int));
//	Log("Trans", LOG_LV_DEBUG, "setsockopt, TCP_KEEPIDLE, ret=%d, errno=%d", Ret, errno);
	OptV = 10;
	Ret = setsockopt(m_iSocket, IPPROTO_TCP, TCP_KEEPINTVL, &OptV, sizeof(int));
//	Log("Trans", LOG_LV_DEBUG, "setsockopt, TCP_KEEPINTVL, ret=%d, errno=%d", Ret, errno);
	OptV = 10;
	Ret = setsockopt(m_iSocket, IPPROTO_TCP, TCP_KEEPCNT, &OptV, sizeof(int));
//	Log("Trans", LOG_LV_DEBUG, "setsockopt, TCP_KEEPCNT, ret=%d, errno=%d", Ret, errno);
//
//	setsockopt(m_iSocket, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(int));
//	setsockopt(m_iSocket, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(int));

	return Ret;
}
/*
int CLS_Socket::Bind(const char *strIP, int iPort)
{
	struct sockaddr_in sockAddr;
	unsigned long ulIP;

	printf("bind ip:%s, iport:%d\n", strIP, iPort);
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
#if 0
	char LocalName[80];
	hostent *pHost;
	gethostname(LocalName, sizeof(LocalName)-1);
	pHost = gethostbyname(LocalName);
		      printf("start12\n");

	if (NULL == pHost)
		return -1;
#endif
	ulIP=inet_addr(strIP);
	sockAddr.sin_addr.s_addr = ulIP;
//	memcpy(&sockAddr.sin_addr.s_addr, pHost->h_addr_list[0], pHost->h_length);
	//sockAddr.sin_port = htons(iPort);
	sockAddr.sin_port = htons(0);	// 随机指定本地端口

	printf("bind start9\n");

	if(bind(m_iSocket, (struct sockaddr*)&sockAddr, sizeof(sockAddr) )<0)
	{
		printf("bind start10\n");

		CloseSocket();
		return -1;
	}
	printf("bind start11\n");

	m_iStatus=SS_BOUND;

	return 0;
}
*/
// use this function after bind() for UDP or listen or connected for TCP
char *CLS_Socket::GetLocalHost(int *pPort)
{
#if 1
	struct sockaddr_in localAddr;
	int iRet;
	socklen_t iLen;

	iLen = sizeof(localAddr);
	iRet = getsockname(m_iSocket, (struct sockaddr *)&localAddr, &iLen);
	if(iRet < 0)
		  return NULL;

	*pPort=ntohs(localAddr.sin_port);
	return inet_ntoa(localAddr.sin_addr);
#endif
        return NULL;
}

bool CLS_Socket::IsReadReady(void)
{
       fd_set rdfds;
       struct timeval tv;
       int ret;

	   FD_ZERO(&rdfds);
       FD_SET((unsigned long)m_iSocket, &rdfds);
       tv.tv_sec = 0;
       tv.tv_usec = 100*1000;
       ret = select(m_iSocket + 1, &rdfds, NULL, NULL, &tv);
       if(ret <= 0)
			return FALSE;

        return TRUE;
}

bool CLS_Socket::IsWriteReady(void)
{
    fd_set writefds;
    struct timeval tv;
    int ret;
    if(m_iSocket < 0 || m_iStatus != SS_CONNECT)
       return FALSE;

    FD_ZERO(&writefds);
    FD_SET((unsigned)m_iSocket, &writefds);
    tv.tv_sec = 0;
    tv.tv_usec = 100*1000;
    ret = select(m_iSocket+1, NULL, &writefds, NULL, &tv);
    if(ret <= 0)
       return FALSE;

   return TRUE;
}

int CLS_Socket::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	int	nCurSize = 0;
	int nRead,  ret;
	LPBYTE	pData = (LPBYTE)lpBuf;
	struct timeval tv;
    fd_set writefds;

    if(m_iSocket < 0) {
        Log("Trans", LOG_LV_DEBUG, "Wrong Socket[%d].", m_iSocket);
		return SOCKET_ERROR;
    }

	while(nCurSize < nBufLen) {
        nRead = -1;

        //构造select参数
        FD_ZERO(&writefds);
        FD_SET(m_iSocket, &writefds);
        tv.tv_sec = 0;
        tv.tv_usec = 100*1000;
		if((ret = select(m_iSocket+1, NULL, &writefds, NULL, &tv)) > 0) {
			nRead = send(m_iSocket, (uchar *)pData+nCurSize, nBufLen - nCurSize, nFlags);
			if(nRead <= 0) {
				Log("Trans", LOG_LV_DEBUG, "Failed to send, ret=%d ,errno=%d", nRead, errno);
			} else {
                nCurSize += nRead;
			}
		} else {
		    Log("Trans", LOG_LV_DEBUG, "Send failed to select, ret=%d ,errno=%d", ret, errno);
		}
        if (nRead <= 0) {
            CloseSocket();
            return -1;
        }
	}

	return 0;
}

int CLS_Socket::Receive(void* lpBuf, int nBufLen, int nFlags)
{
	int nRead = 0 ;
	LPBYTE	pData = (LPBYTE)lpBuf;
	struct timeval tv;
	fd_set readfds;

	if(m_iSocket < 0) {
        Log("Trans", LOG_LV_DEBUG, "Wrong socket:%d.", m_iSocket);
		return SOCKET_ERROR;
    }

	FD_ZERO(&readfds);
    FD_SET(m_iSocket, &readfds);
	tv.tv_sec = 0;
	tv.tv_usec = 10 * 1000;
	//printf("Hello world!\n");
	if (select(m_iSocket+1, &readfds, NULL, NULL, &tv) > 0) {
		nRead = recv(m_iSocket, (char *)pData, nBufLen, nFlags);
		if(nRead <= 0) {
            Log("Trans", LOG_LV_DEBUG, "Failed to read, ret=%d, errno=%d", nRead, errno);
            CloseSocket();
			return -1;
		}
	}

//	m_iSocketError=0;
	return nRead;
}

int CLS_Socket::GetStatus()
{
	return m_iStatus;
}

CLS_TCPClient::CLS_TCPClient()
//	:CLS_Socket()
{
	mSrvIP[0]=0;
	mSrvPort=-1;
//	m_strLocalIP[0]=0;
//	m_iLocalPort=0;
}

CLS_TCPClient::~CLS_TCPClient()
{
}

void CLS_TCPClient::SetSvrIP(const char *strIP)
{
	strcpy(mSrvIP, strIP);
}

char *CLS_TCPClient::GetSvrIP()
{
	return mSrvIP;
}

void CLS_TCPClient::SetSvrPort(int iPort)
{
	mSrvPort=iPort;
}

int CLS_TCPClient::GetSvrPort()
{
	return mSrvPort;
}
/*
void CLS_TCPClient::SetLocalIP(const char *strLocalIP)
{
	strcpy(m_strLocalIP, strLocalIP);
}
*/
/*
char *CLS_TCPClient::GetLocalIP()
{
	return m_strLocalIP;
}
*/
/*
void CLS_TCPClient::SetLocalPort(int iPort)
{
	m_iLocalPort=iPort;
}
*/
/*
int CLS_TCPClient::GetLocalPort()
{
	return m_iLocalPort;
}
*/
/*
int CLS_TCPClient::Bind()
{
	if(m_strLocalIP && m_strLocalIP[0])
		return CLS_Socket::Bind(m_strLocalIP, m_iLocalPort);
	return 0;
}
*/

bool CLS_TCPClient::IsConnect()
{
	return m_iStatus == SS_CONNECT;
}


int CLS_TCPClient::Connect()
{
	int ret;
    struct sockaddr_in sockAddr;
    static int ErrCnt = 0;

//	/* 首先判断PPP拨号是否建立，未建立则直接退出*/
//	if (g_ppp_status != _ppp_connect) {
//        Log("Trans", LOG_LV_DEBUG, "Failed to Connect, g_ppp_status=%hhd.", g_ppp_status);
//        return -1;
//	}

	/* 1.如果状态是未使用，则创建socket */
	if(m_iStatus == SS_UNUSED) {
        ret = CreateSocket();
        if (ret < 0) {
            Log("Trans", LOG_LV_DEBUG, "Failed to CreateSocket, ret=%d.", ret);
            return -1;
        }
	} else {
        Log("Trans", LOG_LV_DEBUG, "m_iStatus=%d.", m_iStatus);
	}


	/* 2.服务器IP和端口填充*/
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family         = AF_INET;
	sockAddr.sin_addr.s_addr    = inet_addr(mSrvIP);
	sockAddr.sin_port           = htons(mSrvPort);

	struct timeval to = {0};

	to.tv_sec = 30;
	setsockopt(m_iSocket, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(struct timeval));
	/* 3.connect服务器*/
	ret = connect(m_iSocket, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
	if (ret < 0) {
        ErrCnt++;
        Log("Trans", LOG_LV_DEBUG, "Failed to connect[%s:%d], fd=%d, ret=%d, cnt=%d, errno=%d:%s.", mSrvIP, mSrvPort, m_iSocket, ret, ErrCnt, errno, strerror(errno));
        sleep(1);
	} else {
        /* 4.socket缓冲区等属性设置*/
        SetSocketProp();
        /* 5.设置非阻塞方式socket */
        SetNonBlock();
        m_iStatus=SS_CONNECT;
        /* check 套接字是否有效*/
        if (IsWriteReady() != TRUE) {
            ErrCnt++;
            Log("Trans", LOG_LV_DEBUG, "Socket couldn't be writed, cnt=%d, errno=%d.", ErrCnt, errno);
        } else {
            Log("Trans", LOG_LV_DEBUG, "Connect socket:%d", m_iSocket);
//            m_iSocketError=0;
            ErrCnt = 0;
        }
    }


    if (ErrCnt > 0) {
        CloseSocket();
        if (ErrCnt >= 5) {
            // 连续超过5次连接失败, TODO需要特殊除立吗？
//            g_ppp_status = _ppp_err_1;
            ErrCnt = 0;
//            g_tcpConnected = 0;
        }

        return -1;
    } else {
        return 0;
    }
}

void CLS_TCPClient::HeartBeat()
{
	return ;
}




