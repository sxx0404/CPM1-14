// UDPComm.cpp: implementation of the CUDPComm class.
//
//////////////////////////////////////////////////////////////////////
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "types.h"
#include "tcp.h"
#include "main_linuxapp.h"
#include "NormalTool.h"
#include "stdafx.h"
#include "../include/log.h"
#include "../include/common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int m_sockSend1 = -1;
int	 m_sockSend2 = -1;

ENUM_SOCK_SEND_STATE g_SockSendState1 = _sock_send_init;
ENUM_SOCK_SEND_STATE g_SockSendState2 = _sock_send_init;

struct sockaddr_in srvRemoteaddr1;
struct sockaddr_in srvLocaladdr1;

struct sockaddr_in srvRemoteaddr2;
struct sockaddr_in srvLocaladdr2;

struct timeval tv_timeout;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* private variables */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typWIRELESS_FLOW_STAT g_wireless_flow_stat;
typSYS_FLOW_STAT g_net_sys_flow_stat[MAX_NET_DEV]= {
	{0,0,},		/* eth0 */
	{0,0,},		/* ppp0 */
};

///////////////////////////////////////////////////////////////////////////////////
/* 加权平均的系数表格*/
#if 0		// 7500/10000
#define AVERAGE_SPEED_FACTOR1	(7500)		// 7500+2500
#define AVERAGE_SPEED_FACTOR2	(1875)		// 1875+625
#define AVERAGE_SPEED_FACTOR3	(470)		// 470+155
#define AVERAGE_SPEED_FACTOR4	(116)		// 116+39
#define AVERAGE_SPEED_FACTOR5	(29)			// 29+10
#define AVERAGE_SPEED_FACTOR6	(10)			// 10

static ulong factors[] = {\
						  AVERAGE_SPEED_FACTOR1,
						  AVERAGE_SPEED_FACTOR2,
						  AVERAGE_SPEED_FACTOR3,
						  AVERAGE_SPEED_FACTOR4,
						  AVERAGE_SPEED_FACTOR5,
						  AVERAGE_SPEED_FACTOR6
						 };
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	private functions prototype
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//static int sock_bind(int sock_index, const char *strIP, int iPort);
static int sock_opt_set_block(int sock_index);
//static int wireless_flow_stat_variables_init(void);
//static int ShiftSamples(typSAMPLE_FLOW_INFO *src, uint len, uint shift, uchar mode);
//static int wireless_flow_sample_stat_start(int s_bytes);
//static int sample_cost_and_speed_calc(typSAMPLE_FLOW_INFO *s);
//static ulong average_speed_calc(ulong *calc_value, unsigned char valid_cnt);
//static int wireless_flow_sample_stat_stop();
//static int sys_net_dev_flow_stat(const char *net_dev, int idx_dev);

///////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* extern function prototype,定义 开始*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* 远程录像文件TCP转发服务器*/
int Recd_InitTCP( const char *m_strServer, int iRemotePort )
{
	if ( m_sockSend1 >= 0 ) {
		close(m_sockSend1);
	}

	/* init srvRemoteaddr(连接的服务器IP和端口初始化) */
	bzero(&srvRemoteaddr1, sizeof(srvRemoteaddr1));
	srvRemoteaddr1.sin_family = AF_INET;
	srvRemoteaddr1.sin_port = htons(iRemotePort);		//将主机的无符号短整形数转换成网络字节顺序
	if (inet_pton(AF_INET, m_strServer, &srvRemoteaddr1.sin_addr) <= 0) {
		g_SockSendState1 = _sock_send_init_fail;
		return -1;
	}

	// 1.创建套接口
	m_sockSend1 = socket(AF_INET, SOCK_STREAM, 0);

	if ( m_sockSend1 > 0 ) {
		//套接口创建成功
		// 2.套接口阻塞特性设置
		sock_opt_set_block(0);
		// 超时设置为30秒
		struct timeval t = {.tv_sec=30,.tv_usec=0};

		setsockopt(m_sockSend1, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(struct timeval));
		// 6.套接口发送缓冲区查询和修改
		socklen_t sendbuflen = 0;
		socklen_t len = sizeof(sendbuflen);
		if ( (getsockopt(m_sockSend1, SOL_SOCKET, SO_SNDBUF, (void*)&sendbuflen, &len)) == -1 ) {
			printf("getsockopt tcp_wmem is fail\n");
			goto ERR1;
		}
		printf("default,sendbuf:%d\n", sendbuflen);
//		sendbuflen = 4096;
//		int Ret = setsockopt(m_sockSend1, IPPROTO_TCP, TCP_MAXSEG, &sendbuflen, len);
//		Log("D", LOG_LV_DEBUG, "setsockopt ret=%d, errno:%d", Ret, errno);
#if 0
		sendbuflen = 4096;
		if ( setsockopt(m_sockSend1, SOL_SOCKET, SO_SNDBUF, (void*)&sendbuflen, len) == -1 ) {
			printf("setsockopt tcp_wmem is fail\n");
			goto ERR1;
		}

		if ( getsockopt(m_sockSend1, SOL_SOCKET, SO_SNDBUF, (void*)&sendbuflen, &len) == -1 ) {
			printf("getsockopt tcp_wmem is fail\n");
			goto ERR1;
		}
		printf("now,sendbuf:%d\n", sendbuflen);
#endif
ERR1:
		g_SockSendState1 = _sock_send_connecting;

		return 1;
	} else {
		return 0;    // socket fail
	}
}

int Recd_WriteTCPData(unsigned char *data, int iLens)
{
	int recont=0;

	if ( iLens<=0 ) {
		return 0;
	}

	if ( m_sockSend1 < 0 ) {
		g_SockSendState1 = _sock_send_init;
#ifdef _TCP_SEND_DATA_PRT2
		printf("####%s:%d####:err1\n\r", __func__, __LINE__);
#endif
		return -1;
	}

	if ( g_SockSendState1 == _sock_send_connecting ) {
		/* connect to server */
		int servlen = sizeof(srvRemoteaddr1);
#ifdef _TCP_SEND_DATA_PRT2
		printf("####%s:%d####:now connect server...\n\r", __func__, __LINE__);
#endif
		/* 最多尝试链接10次*/
		for (recont = 0; recont < 10; recont++) {
			if( (connect(m_sockSend1, (struct sockaddr *)&srvRemoteaddr1, servlen) == -1) ) {
#ifdef _TCP_SEND_DATA_PRT2
				printf("####%s:%d####:connect server err1,recont:%d\n\r", __func__, __LINE__, recont);
#endif
				g_SockSendState1 = _sock_send_connect_err;
				continue;
			}
#ifdef _TCP_SEND_DATA_PRT2
			printf("####%s:%d####:connect server success...\n\r", __func__, __LINE__);
#endif
			/* connect操作成功*/
			g_SockSendState1 = _sock_send_connect_ok;
			break;
		}
	}

	if ( g_SockSendState1 == _sock_send_connect_ok ) {
		int nResult = -1, SendLen=0;
		int i=0;
#ifdef use_flow_stat
		wireless_flow_sample_stat_start(iLens);
#endif
		/* LED指示，用于反应当前网速*/
		for (i=0; i<5; i++) {
			LedsCtrl(LED_SEND_FILE, LED_OPEN_OPR);	// 点亮
			nResult = send(m_sockSend1, data+SendLen, iLens-SendLen, 0);
			LedsCtrl(LED_SEND_FILE, LED_CLOSE_OPR);	// 熄灭
//			char *tStr = NULL;
//
//			if (nResult>0 && (tStr=malloc(nResult*3+1))!=NULL) {
//				Hex2Str(tStr, data+SendLen, nResult, ' ');
//			}
//			Log("D", LOG_LV_DEBUG, "RcdSrv, Send Data[w=%d, r=%d]:%s.", iLens-SendLen, nResult, tStr);
//			if (tStr) {
//				free(tStr);
//				tStr = NULL;
//			}
			//int nResult = write(m_sockSend, data, iLens);
#ifdef _TCP_SEND_DATA_PRT2
			printf("nResult1:%d\n\r", nResult);
#endif
			if ( nResult<0 ) {
				// TODO:错误状态处理
				g_SockSendState1 = _sock_send_connect_err;
#ifdef _TCP_SEND_DATA_PRT2
				printf("####%s:%d####:err3, errno=%d\n\r", __func__, __LINE__, errno);
#endif
				g_SockSendState1 = _sock_send_init;
				return -3;
			} else if (nResult > 0) {
				SendLen+=nResult;
				if (SendLen >= iLens) {
					break;
				}
			}
#ifdef use_flow_stat
			wireless_flow_sample_stat_stop();
#endif
		}
		return SendLen;
	}

	/* 未知错误*/
	return -4;
}

void Recd_CloseTCP()
{
	if (m_sockSend1 >= 0) {
		close(m_sockSend1);
		m_sockSend1 = -1;
	}
}


/* 远程视频帧TCP转发服务器*/
int RealStream_InitTCP( const char *m_strServer, int iRemotePort )
{
	if ( m_sockSend2 >= 0 ) {
		close(m_sockSend2);
	}

	/* init srvRemoteaddr(连接的服务器IP和端口初始化) */
	bzero(&srvRemoteaddr2, sizeof(srvRemoteaddr2));
	srvRemoteaddr2.sin_family = AF_INET;
	srvRemoteaddr2.sin_port = htons(iRemotePort);		//将主机的无符号短整形数转换成网络字节顺序
	if ( inet_pton(AF_INET, m_strServer, &srvRemoteaddr2.sin_addr) <= 0 ) {
		g_SockSendState2 = _sock_send_init_fail;
		return -1;
	}

	// 1.创建套接口
	m_sockSend2 = socket(AF_INET, SOCK_STREAM, 0);
	if ( m_sockSend2 > 0 ) {
		//套接口创建成功
		// 2.套接口阻塞特性设置
		sock_opt_set_block(0);
		g_SockSendState2 = _sock_send_connecting;
		return 1;
	} else {
		return 0;    // socket fail
	}
}

int RealStream_WriteTCPData(unsigned char *data, int iLens)
{
	if ( iLens<=0 ) {
		return 0;
	}

	if ( m_sockSend2 < 0 ) {
		g_SockSendState2 = _sock_send_init;
#ifdef _TCP_SEND_DATA_PRT2
		printf("####%s:%d####:err1\n\r", __func__, __LINE__);
#endif
		return -1;
	}

	if ( g_SockSendState2== _sock_send_connecting ) {
		/* connect to server */
		int servlen = sizeof(srvRemoteaddr2);
#ifdef _TCP_SEND_DATA_PRT2
		printf("####%s:%d####:now connect server...\n\r", __func__, __LINE__);
#endif
		if( (connect(m_sockSend2, (struct sockaddr *)&srvRemoteaddr2, servlen) == -1) ) {
			g_SockSendState2 = _sock_send_connect_err;
#ifdef _TCP_SEND_DATA_PRT2
			printf("####%s:%d####:err2\n\r", __func__, __LINE__);
#endif
			g_SockSendState2 = _sock_send_init;
			return -2;
		}
#ifdef _TCP_SEND_DATA_PRT2
		printf("####%s:%d####:connect server success...\n\r", __func__, __LINE__);
#endif
		/* connect操作成功*/
		g_SockSendState2 = _sock_send_connect_ok;
	}

	if ( g_SockSendState2 == _sock_send_connect_ok ) {
		int nResult = -1;
#ifdef _TCP_SEND_DATA_PRT1
		printf("####%s:%d####:send len:%d, hex:\n\r", __func__, __LINE__, iLens);
		for ( i = 0; i < ((iLens<100) ? iLens : 100); i++ ) {
			printf("%02x ", data[i]);
		}
#endif
		LedsCtrl(LED_SEND_FILE, LED_OPEN_OPR);	// 点亮
		nResult = send(m_sockSend2, data, iLens, 0);		// flag:0, 则send等同于write
		LedsCtrl(LED_SEND_FILE, LED_CLOSE_OPR);	// 熄灭
		//int nResult = write(m_sockSend, data, iLens);
#ifdef _TCP_SEND_DATA_PRT2
		printf("nResult1:%d\n\r", nResult);
#endif
		if ( nResult<0 ) {
			g_SockSendState2 = _sock_send_connect_err;
#ifdef _TCP_SEND_DATA_PRT2
			printf("####%s:%d####:err3\n\r", __func__, __LINE__);
#endif
			g_SockSendState2 = _sock_send_init;
			return -3;
		}
		return nResult;
	}
	/* 未知错误*/
	return -4;
}

void RealStream_CloseTCP()
{
	if (m_sockSend2 >= 0) {
		close(m_sockSend2);
		m_sockSend2 = -1;
	}
}

/*
static int sock_bind(int sock_index, const char *strIP, int iPort)
{
	struct sockaddr_in *pSock_addr;
	unsigned long ulIP;
	int m_iSocket;

	printf("####%s(%d)####:bind sockidx:%d,ip:%s,iport:%d\n", __func__, __LINE__, sock_index, strIP, iPort);
	switch(sock_index)
	{
		case 0:
			{
				pSock_addr = &srvLocaladdr1;
			}
			break;

		case 1:
			{
				pSock_addr = &srvLocaladdr2;
			}
			break;

		default:
			break;
	}
	bzero(pSock_addr, sizeof(struct sockaddr_in));
	pSock_addr->sin_family = AF_INET;
#if 0
	char LocalName[80];
	hostent *pHost;
	gethostname(LocalName, sizeof(LocalName)-1);
	pHost = gethostbyname(LocalName);
		      printf("start12\n");

	if (NULL == pHost)
		return -1;
#endif
	ulIP = inet_addr(strIP);
	pSock_addr->sin_addr.s_addr = ulIP;
	//memcpy(&sockAddr.sin_addr.s_addr, pHost->h_addr_list[0], pHost->h_length);
	pSock_addr->sin_port = htons(iPort);
	switch(sock_index)
	{
		case 0:
			{
				m_iSocket = m_sockSend1;
			}
			break;

		case 1:
			{
				m_iSocket = m_sockSend2;
			}
			break;

		default:
			break;
	}
	if ( bind(m_iSocket, (struct sockaddr*)pSock_addr, sizeof(struct sockaddr_in)) < 0 )
	{
		perror("bind is fail\n");
		return -1;
	}
	return 0;
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	description:套接字阻塞属性设置
*	input:
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int sock_opt_set_block(int sock_index)
{
	int flags,ret;
	int m_iSocket;

	switch(sock_index) {
	case 0: {
		m_iSocket = m_sockSend1;
	}
	break;

	case 1: {
		m_iSocket = m_sockSend2;
	}
	break;

	default:
		break;
	}
	/* 配置成阻塞模式*/
	flags = fcntl(m_iSocket, F_GETFL, 0);
	ret = fcntl(m_iSocket, F_SETFL, flags & ~O_NONBLOCK);
	return ret;
}

#ifdef use_flow_stat
static int wireless_flow_stat_variables_init(void)
{
	bzero(&g_wireless_flow_stat, sizeof(g_wireless_flow_stat));
	return 0;
}

static int ShiftSamples(typSAMPLE_FLOW_INFO *src, uint len, uint shift, uchar mode)
{
	uint i;
	if(len<shift) {
		return;
	}

	if (mode == S_LEFT_SHIFT) {
		for(i=0; i<(len-shift); i++) {
			src[i]=src[i+shift];
		}
	} else if (mode == S_RIGHT_SHIFT) {
		for(i=(len-shift); i>0; i--) {
			src[i-1+shift]=src[i-1];
		}
	}
}

static int wireless_flow_sample_stat_start(int s_bytes)
{
	int ret;
	struct timeval tv;
	typWIRELESS_FLOW_STAT *pWire_flow_stat;
	unsigned char s_cnt;

	// 获取目前的时间
	if ( (ret = gettimeofday(&tv,NULL)) < 0 ) {
		printf("####%s(%d)####:error\n", __func__, __LINE__);
		return -1;
	}
	pWire_flow_stat = &g_wireless_flow_stat;
	pWire_flow_stat->t_sample.send_bytes = s_bytes;		// 该采样点的发送字节数
	pWire_flow_stat->t_sample.start_tv = tv;						// 记录开始时间
	return 0;
}

static int sample_cost_and_speed_calc(typSAMPLE_FLOW_INFO *s)
{
	ulong total_ms, sub_sec, sub_msec, sub_usec;
	ulong k, j;

	printf("start_tv,tv_sec:%ld,tv_usec:%ld\n", s->start_tv.tv_sec, s->start_tv.tv_usec);
	printf("stop_tv,tv_sec:%ld,tv_usec:%ld\n", s->stop_tv.tv_sec, s->stop_tv.tv_usec);

	if ( s->stop_tv.tv_sec > s->start_tv.tv_sec ) {
		// 存在秒差异
		sub_sec = s->stop_tv.tv_sec - s->start_tv.tv_sec;
		sub_usec = (1000*1000 - s->start_tv.tv_usec) +  s->stop_tv.tv_usec;
		sub_msec = sub_usec/1000;
		printf("case1:sub_sec is %ld,sub_usec is %ld,sub_msec is %ld\n", sub_sec, sub_usec, sub_msec);
		s->speed = ( (s->send_bytes)*1000 / (sub_sec*1000+sub_msec) );				// unit:bytes/second
		printf("speed is %ld bytes/second\n", s->speed);
		return 0;
	} else if ( s->stop_tv.tv_sec == s->start_tv.tv_sec ) {
		// 秒没有差异
		if ( s->stop_tv.tv_usec < s->start_tv.tv_usec ) {
			return -1;
		}
		sub_usec = s->stop_tv.tv_usec - s->start_tv.tv_usec;
		sub_msec = sub_usec/1000;
		printf("case2:sub_usec is %ld,sub_msec is %ld\n", sub_usec, sub_msec);
		if (sub_msec) {
			// 达到ms单位
			s->speed = ((s->send_bytes)*1000/sub_msec);				// unit:bytes/second
			printf("speed is %ld bytes/second\n", s->speed);
			return 0;
		} else {
			sub_usec = sub_usec%1000;
			printf("case3:sub_usec is %ld\n", sub_usec);
			s->speed = ((s->send_bytes)*1000*1000/sub_usec);		// unit:bytes/second
			printf("speed is %ld BYTES/SECOND\n", s->speed);
			return 0;
		}
	} else {
		return -2;
	}
}

static ulong average_speed_calc(ulong *calc_value, unsigned char valid_cnt)
{
	int cnt;
	ullong sum=0;
	typSAMPLE_FLOW_INFO *pArrary;

	if (valid_cnt<=0) {
		return -1;
	}
	pArrary = &g_wireless_flow_stat.samples[0];
#ifdef flow_stat_printf_i
	for (cnt=0; cnt<MAX_WIRELESS_FLOW_SAMPLES; cnt++) {
		printf("idx:%d,speed:%ld\n", cnt, g_wireless_flow_stat.samples[cnt].speed);
	}
#endif
	if ( valid_cnt < MAX_WIRELESS_FLOW_SAMPLES ) {
		//采样点还未填充满，则直接求算数平均
		for (cnt = 0; cnt < valid_cnt; cnt++) {
			sum += (ullong)(pArrary->speed);
			pArrary++;
#ifdef flow_stat_printf_i
			printf("index:%d,sum:%lld\n", cnt, sum);
#endif
		}
		sum /=valid_cnt;
	} else if ( valid_cnt==MAX_WIRELESS_FLOW_SAMPLES ) {
		//采样点还未填充满，则直接求加权平均
		for (cnt = 0; cnt < valid_cnt; cnt++) {
			sum += ((ullong)factors[cnt] * (ullong)(pArrary->speed) /10000);
			pArrary++;
#ifdef flow_stat_printf_i
			printf("index:%d,sum:%lld\n", cnt, sum);
#endif
		}
	}
	*calc_value = (ulong)sum;
	return 0;
}

static int wireless_flow_sample_stat_stop()
{
	struct timeval tv;
	typWIRELESS_FLOW_STAT *pWire_flow_stat;
	int bCalc_suc;

	// 1. 获取目前的时间
	gettimeofday(&tv,NULL);
	pWire_flow_stat = &g_wireless_flow_stat;
	pWire_flow_stat->t_sample.stop_tv = tv;						// 记录开始时间
	// 2. 计算该采样点的耗时和平均速率
	bCalc_suc = sample_cost_and_speed_calc(&(pWire_flow_stat->t_sample));
	if (bCalc_suc == 0) {
		// 计算成功
		// 2.1 采样点存储数组移位
		ShiftSamples(&(pWire_flow_stat->samples[0]), MAX_WIRELESS_FLOW_SAMPLES, 1, S_RIGHT_SHIFT);
		pWire_flow_stat->samples[0] = pWire_flow_stat->t_sample;
		pWire_flow_stat->valid_samples++;
		if ( pWire_flow_stat->valid_samples>=MAX_WIRELESS_FLOW_SAMPLES ) {
			pWire_flow_stat->valid_samples = MAX_WIRELESS_FLOW_SAMPLES;
		}
		pWire_flow_stat->total_send_bytes += pWire_flow_stat->t_sample.send_bytes;	// 数据发送总字节数
		// 2.2 平均速率加权求平均
		average_speed_calc(&(pWire_flow_stat->average_speed), pWire_flow_stat->valid_samples);
		printf("average speed:%ld BYTES/SECOND\n", pWire_flow_stat->average_speed);
		return 0;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	功能:	获取系统统计的网络发送和接收数据
*	返回:
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int sys_net_dev_flow_stat(const char *net_dev, int idx_dev)
{
	FILE	*fp;
	char cmd[256] = {0};
	char	buf[256]= {0};
	char *pch=NULL;
	ulong s_bytes,r_bytes;

	sprintf(cmd, "ifconfig %s|grep \"RX bytes\"", net_dev);
	fp = popen(cmd, "r");
	fgets(buf, sizeof(buf), fp);
	pclose(fp);
	printf("net dev:%s,flow stat string:%s\n", net_dev, buf);
	pch = strstr(buf, "RX bytes");
	if (pch==NULL) {
		return -1;
	}
	r_bytes = atol(pch+9);
	pch = strstr(buf, "TX bytes");
	if (pch==NULL) {
		return -2;
	}
	s_bytes = atol(pch+9);
	printf("RX bytes:%ld,TX bytes:%ld\n", r_bytes, s_bytes);
	switch(idx_dev) {
	case 0: {	// eth0
		g_net_sys_flow_stat[0].recv_bytes = r_bytes;
		g_net_sys_flow_stat[0].send_bytes = s_bytes;
	}
	break;

	case 1: {	// ppp0
		g_net_sys_flow_stat[1].recv_bytes = r_bytes;
		g_net_sys_flow_stat[1].send_bytes = s_bytes;
	}
	break;

	default:
		break;
	}
	return	0;
}
#endif

#if 0
int UDP_RecvData(unsigned char *data)
{
	if (m_sockRecv < 0) {
		return -1;
	}

	fd_set fs_read;

	FD_ZERO (&fs_read);
	FD_SET (m_sockRecv, &fs_read);

	socklen_t iLen = sizeof(srvClintaddr);
	tv_timeout.tv_sec = 0;
	tv_timeout.tv_usec = 100 * 1000;
	int retval = select (m_sockRecv + 1, &fs_read, NULL, NULL, &tv_timeout);
	if (retval) {
		int n = recvfrom(m_sockRecv, data, MAX_UDPSIZE, 0, (struct sockaddr *)&srvClintaddr, &iLen);
		return n;
	} else {
		return -2;
	}
}
#endif

