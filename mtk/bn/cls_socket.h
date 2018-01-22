// CLS_Socket.h: interface for the CLS_Socket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLS_Socket_H__B49AE83F_4761_4DC9_A418_3F009DBA7FE5__INCLUDED_)
#define AFX_CLS_Socket_H__B49AE83F_4761_4DC9_A418_3F009DBA7FE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef CLS_Socket__
#define CLS_Socket__

//#include <WINSOCK2.H>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>

#include "../include/log.h"

typedef int    SOCKET;
typedef socklen_t socklen;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define BOOL int
#define TRUE 1
#define FALSE 0
#define uchar unsigned char
#define LPBYTE  uchar *

#define MAX_IP_LEN		32		// IP Address buffer length
#define MAX_CONNECTION	64		// Maximium Connections A Server can support
#define MAX_SOCK		10		// Maximium try times when a (nonblocking) socket send data
#define SOCK_SLEEP		50		// Sleep wait block is disappeared
#define TCP_BUFF_MAX	0x100000	// TCP recv buffer length of each socket
#define SEND_TIMEOUT    3000
#define MAX_MSGSIZE		4096

//Socket status
#define SS_UNUSED	0			// unused socket
#define SS_CREATED	1			// socket is created
#define SS_BOUND	2			// socket is bound a IP address
#define SS_LISTEN	3			// socket is listening for client connecting in
#define SS_CONNECT	4			// socket is connected

class CLS_Socket
{
protected:
	int m_iSocket;				// socket id
	int m_iStatus;				// socket status
	static int g_iSockCount;
public:
//	int m_iSocketError;
public:
	CLS_Socket();
	//CLS_Socket(int iSocket, int iStatus=SS_CREATED);	// constructor from a socket id
	virtual ~CLS_Socket();
public:
	int SetSockOpt(int iLevel, int iOptName, const char *pOptVal, int iOptLen);
/*************************************************************************
Decription: set all kind of options of the socket
iLevel: 	big group of option
iOptName:	option's name
pOptVal:	the pointer to the buffer for the option's value
iOptLen:	the length of buffer for option's value
Return:		Success:0; Fail: other value, (mostly -1)
*************************************************************************/
	int GetSockOpt(int iLevel, int iOptName, char *pOptVal, int *pOptLen);
/*************************************************************************
Decription: get all kind of options of the socket
iLevel: 	big group of option
iOptName:	option's name
pOptVal:	the pointer to the buffer for the option's value
pOptLen:	the length of buffer for option's value, must be filled by correct
Return:		Success:0; Fail: other value, (mostly -1)
*************************************************************************/

public:
	virtual int CreateSocket();
/*************************************************************************
Decription: Create Socket for the class, default is a TCP(STREAM) socket
Return:		Success: socket value; Fail: -1
*************************************************************************/
	int CloseSocket();
/*************************************************************************
Decription: Close Socket
Return:		Success: 0; Fail: -1
*************************************************************************/
	int SetNonBlock(void);
/*************************************************************************
Decription: Set the Socket as a nonblocking socket
Return:		Success: socket value; Fail: -1
*************************************************************************/
	virtual int SetSocketProp();
/*************************************************************************
Decription: Set Socket set some option, default set SO_REUSEADDR,
			SO_SNDBUF, SO_RCVBUF
Return:		Success: socket value; Fail: <0
*************************************************************************/
public:
//	int Bind(const char *strIP, int iPort);
/*************************************************************************
Decription: Bind socket to the specified IP and Port
strIP:		IP address the socket to be bound, local IP Address
iPort:		Port the socket to be bound
Return:		Success: 0; Fail: -1
*************************************************************************/
	char *GetLocalHost(int *pPort);
/*************************************************************************
Decription: Get Local Host's IP address and Port of the socket,
			use this function after bind() or listen or connected
pPort:	 	local Port on return
Return:		local IP address on success; Fail: NULL
*************************************************************************/
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
/*************************************************************************
Decription: Send data on the socket
pData:		data to be send
iMaxLen:	data's length
Return:		Success: iMaxLen; Fail: <0
*************************************************************************/
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
/*************************************************************************
Decription: Receive data on the socket
pData:		data to be received
iMaxLen:	data's length
Return:		Success: iMaxLen; Fail: <0
*************************************************************************/
	bool    IsReadReady(void);
	bool    IsWriteReady(void);
//  virtual  int OnReceive();
/************************************************************************
Decription: Inform the socket haved data come
return :    Sucesss: 0; Fail : -1;
*************************************************************************/
//	virtual int SendTo(const char *strPeerAddr, int iPeerPort, void *pData, int iMaxLen);
/*************************************************************************
Decription: Send data on the socket, specify peer's IP address and port
strPeerAddr:Peer's IP address
iPeerPort:	Peer's Port
pData:		data to be send
iMaxLen:	data's length
Return:		Success: iMaxLen; Fail: <0
*************************************************************************/
//	virtual int RecvFrom(char *strPeerAddr, int *pPeerPort, void *pData, int iMaxLen);
/*************************************************************************
Decription: Receive data on the socket, can get peer's IP address and port
strPeerAddr:Peer's IP address
pPeerPort:	Peer's Port
pData:		data to be received
iMaxLen:	data's length
Return:		Success: iMaxLen; Fail: <0
*************************************************************************/
public:
	int GetStatus();
	int GetSocket()
	{
		return m_iSocket;
	}
//	int GetSocketError()
//	{
//		return m_iSocketError;
//	}
};

// TCP Client Class
class CLS_TCPClient:public CLS_Socket{
//	char m_strLocalIP [MAX_IP_LEN];	// Local IP Address
//	int m_iLocalPort;				// Local Port
public:
	char    mSrvIP[MAX_IP_LEN];	// Server's IP Addres to connect
	int     mSrvPort;					// Server's Port
	CLS_TCPClient();
	virtual ~CLS_TCPClient();
public:								// Client's property
	void SetSvrIP(const char *strIP);
	char *GetSvrIP();
	void SetSvrPort(int iPort);
	int GetSvrPort();
//	void SetLocalIP(const char *strLocalIP);
//	char *GetLocalIP();
//	void SetLocalPort(int iPort);
//	int GetLocalPort();
public:								// Client's opertion
	//int SetSocketProp();
/************************************************************************
Decription: set socket operation
Return: Success: 0; Fail: -1
************************************************************************/
//	int Bind();
/*************************************************************************
Decription: Bind default IP and Port to local
Return:		Success: 0; Fail: -1
*************************************************************************/
	int Connect();
/*************************************************************************
Decription: Connect to Server
Return:		Success: 0; Fail: <0
*************************************************************************/
	bool IsConnect();
/*************************************************************************
Decription: IS connect to Server
Return:		Success: true; else: false
*************************************************************************/
	void HeartBeat(void);
/*************************************************************************

Decription: Cheching the state of socket that Connect to Server
Return:		Success: > 0; Fail: <0
*************************************************************************/

//	virtual void OnReceive(int nErrorCode);
/*************************************************************************
Decription: Notifies a listening socket that there is data to be retrieved
			by calling Receive.
nErrorCode: socket id
Return :	Sucess: 0; Fail: WSAENETDOWN
*************************************************************************/
};


#endif

#endif // !defined(AFX_CLS_Socket_H__B49AE83F_4761_4DC9_A418_3F009DBA7FE5__INCLUDED_)

