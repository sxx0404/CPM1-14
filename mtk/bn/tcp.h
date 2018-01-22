#ifndef __TCP_H__
#define __TCP_H__
#include <stdint.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Private macros */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
#define RECD_TCP_SERVER_IP			"122.224.226.42"
#define RECD_TCP_SERVER_PORT		9603
#endif
// test
#if 0
#define RECD_TCP_SERVER_IP			"192.168.0.40"
#define RECD_TCP_SERVER_PORT		9603
#endif
// test,ppp0
#if 1
#define RECD_TCP_SERVER_IP			"123.157.158.94"
#define RECD_TCP_SERVER_PORT		20000
#endif

#define REALSTREAM_TCP_SERVER_IP				"122.224.226.42"
#define REALSTREAM_TCP_SERVER_PORT		9601

#define MAX_UDPSIZE 		2048

#define MAX_WIRELESS_FLOW_SAMPLES			6
#define S_LEFT_SHIFT			0
#define S_RIGHT_SHIFT		1

#define MAX_NET_DEV		2

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Private typedef */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(push)
#pragma pack(1)

typedef struct {
	int send_bytes;					// 发送字节数
	struct timeval start_tv;		// 开始记录时间
	struct timeval stop_tv;	// 结束记录时间
	uint32_t cost_usec;	// 耗时，unit:微秒
	uint32_t speed;			// unit:Bytes/Second
}typSAMPLE_FLOW_INFO;

typedef struct {
	typSAMPLE_FLOW_INFO t_sample;
	typSAMPLE_FLOW_INFO samples[MAX_WIRELESS_FLOW_SAMPLES];
	unsigned char valid_samples;		// 有效采样点数量
	uint32_t total_send_bytes;				// 总发送字节数
	uint32_t average_speed;					// 平均网速(除了采样点的网速参数，再加入权值，更加真实得反应当前网速)
}typWIRELESS_FLOW_STAT;

typedef struct {
	uint32_t send_bytes;
	uint32_t recv_bytes;
}typSYS_FLOW_STAT;

#pragma pack()
#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* 功能选择*/
/* 用于流量统计*/
//#define use_flow_stat	1
/* 调试打印*/
#define flow_stat_printf_i


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* 实际使用的时候，关闭以下2个宏定义*/
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* 有线网络传输录像文件速度调试*/
//#define _debug_use_eth0_send_record
/* 测试网速，调试时候用于调整socket send缓冲区的大小(服务器下发readFile指令)*/
//#define _socket_send_buf_modify_test

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* extern functions prototypes */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int Recd_InitTCP( const char *m_strServer, int iRemotePort );
extern int Recd_WriteTCPData(unsigned char *data, int iLens);
extern void Recd_CloseTCP();
extern int RealStream_InitTCP( const char *m_strServer, int iRemotePort );
extern int RealStream_WriteTCPData(unsigned char *data, int iLens);
extern void RealStream_CloseTCP();
//extern int get_local_ip(int sock_id, char *ip_data, const char *net_dev);


#ifdef __cplusplus
}
#endif

#endif

