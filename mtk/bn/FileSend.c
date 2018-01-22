#include "stdafx.h"
#include "types.h"
#include "tcp.h"
#include "FileSend.h"
#include "NormalTool.h"
#include "ManageTransmit.h"
#include "BusinessProtocol.h"
#include "../include/log.h"


//报文类型定义
enum {
	IPC_PACKET_TYPE_DATASTREAM	= 0, //数据流（上传文件的数据或者实时视频数据）
	IPC_PACKET_TYPE_REGISTER	= 1,   //实时视频的登记
	IPC_PACKET_TYPE_FILEUPLOAD	= 2, //上传文件
	IPC_PACKET_TYPE_MAX
};

#ifndef UINT32
#define UINT32  unsigned int
#endif

/*
 *  type = IPC_PACKET_TYPE_DATASTREAM时，不管是上传文件还是实时视频流，data[0]
 *  直接填充数据即可
 *  type = IPC_PACKET_TYPE_REGISTER时，用于实时视频流时的宣告此连接的数据的
 d_t*  来源：主控器MAC及摄像头索引，data[0]即为struct packet_register_body
 *  type = IPC_PACKET_TYPE_FILEUPLOAD时，用于上传文件前通知服务器，该文件要保存
 *  在哪个目录下、文件名，data[0]即为struct packet_fileupload_body
 */
struct packet_general_header {
	UINT32      type;       // network byte order, big-endian
	UINT32      length;     // length of data, in bytes, network byte order, big-endian
	char        data[0];
};

/*
 *  type = IPC_PACKET_TYPE_REGISTER,
 *  @data is struct packet_register_body
 */
struct packet_register_body {
	char    mac[18];        // format 00:11:22:33:44:55
	UINT32  index;          // index of IPC, zero-based
};

struct packet_fileupload_body {
	char    username[12];	// yinjie
	char    password[12];		// yinjie
	char    path[64];      		// format: YYMMDD/001122334455
	char    filename[64];
};		//B152


#define SERVER_IP                   122.224.226.42  //均连接此IP
#define SERVER_REALSTREAM_PORT      9601            //实时视频主控器连接此端口
#define SERVER_REALSTREAM_CLIENT_PORT 9602          //实时视频客户端程序连接此端口,供平台使用
#define SERVER_FILEUPLOAD_PORT      9603            //文件上传连接此端口

#define FILE_OPR_ERR_MAX_CNT (3)
#define USER			"yinjie"
#define PASSWD		"yinjie"
/* 录像文件拆包发送的字节大小 */
/* 725KB文件，分包1024B，则需要38S，平均：19KB/S。分包4096B，则需要15S，平均：50KB/S。分包8192B，则需要10S，平均70KB/S。*/
//#define MAXSIZE		(1024)
//#define MAXSIZE     (4096)
//#define MAXSIZE     (8192)
#define MAXSIZE		4096

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static const char SetTable[] = "8893005288930052";

struct text {
	int len;
	unsigned char b_buf[MAXSIZE];
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	Output:
*				-1:记录文件操作错误	-2:TCP 发送失败
*//* 文件绝对路径:/home/record/A-20140723-170000-N.bin */
//////////////////////////////////////////////////////////////////////////////////////////////////////
int RecordFileUpload (const char * FlPath)
{
	int Ret=-1, RecdFile_fd=-1, UnpackLen=0;
	int TcpSendRet = -1;
	UINT32 temp = 0;
	char DateString[9] = {0};
	const char *pFileNm = NULL;
	uint8_t tData[MAXSIZE] = {0};
	struct stat buf;
	struct packet_fileupload_body RecordFileBody = {0};
	struct packet_general_header *pSendPack = NULL;

	// 先检查文件存在否
	if (stat(FlPath, &buf)<0 || buf.st_size<=0) {
		return -1;
	}
	Log("Cam", LOG_LV_INFO, "Size of File[%s] is %dB.", FlPath, buf.st_size);

	// 构造IPC_PACKET_TYPE_FILEUPLOAD信息，先准备struct packet_fileupload_body
	strncpy(RecordFileBody.username, USER, 12);
	strncpy(RecordFileBody.password, PASSWD, 12);
	/* 文件名解析出年月日T-YYYYMMDD-HHMMSS-N.h264找到文件名起始位置 */
	pFileNm = strrchr(FlPath, '/');
	if (pFileNm == NULL) {
		pFileNm = FlPath;
	}
	if ((pFileNm-FlPath)<(strlen(FlPath)-20)) {
		// 解析成功
		strncpy(DateString, pFileNm+3, 8);
	} else {
		return -1;
	}
	/* MAC地址转换*/
	sprintf(RecordFileBody.path, "%.8s/%02X%02X%02X%02X%02X%02X", DateString, g_BootPara.NetInfo.cMac[0], g_BootPara.NetInfo.cMac[1], g_BootPara.NetInfo.cMac[2],\
			g_BootPara.NetInfo.cMac[3], g_BootPara.NetInfo.cMac[4],g_BootPara.NetInfo.cMac[5]);
	/* 录像文件名字:A-20140723-170000-N.h264*/
	strncpy(RecordFileBody.filename, pFileNm+1, 64);

	// 然后准备发送数据
	pSendPack = realloc(pSendPack, sizeof(struct packet_general_header) + sizeof(struct packet_fileupload_body));
	temp = IPC_PACKET_TYPE_FILEUPLOAD;
	ConvertMemcpy(&pSendPack->type, &temp, 4);
	temp = sizeof(struct packet_fileupload_body);
	ConvertMemcpy(&pSendPack->length, &temp, 4);
	memcpy(pSendPack->data, &RecordFileBody, sizeof(struct packet_fileupload_body));
	// TCP发送数据
	TcpSendRet = Recd_WriteTCPData((uchar *)pSendPack, (sizeof(struct packet_general_header) + sizeof(struct packet_fileupload_body)));
	free(pSendPack);
	pSendPack = NULL;
	if (TcpSendRet !=  (sizeof(struct packet_general_header) + sizeof(struct packet_fileupload_body))) {
		return -1;
	}

	// 然后打开文件准备发送文件数据
	RecdFile_fd = open(FlPath, O_RDONLY);
	if (RecdFile_fd < 0) {
		return -1;
	}

	/* 文件分包传输开始*/
	do {
		// 先把数据读出来
		UnpackLen = read(RecdFile_fd, tData, MAXSIZE);
		if (UnpackLen < 0) {
			break;
		} else if (UnpackLen == 0) {
			Ret = 0;
			break;
		}

		// 再组发送数据包
		pSendPack = realloc(pSendPack, sizeof(struct packet_general_header) + UnpackLen);
		temp = IPC_PACKET_TYPE_DATASTREAM;
		ConvertMemcpy(&pSendPack->type, &temp, 4);
		temp = UnpackLen;
		ConvertMemcpy(&pSendPack->length, &temp, 4);
		memcpy(pSendPack->data, tData, UnpackLen);
		/* TCP发送数据*/
		TcpSendRet = Recd_WriteTCPData((uchar *)pSendPack, (sizeof(struct packet_general_header)+UnpackLen));
		if (TcpSendRet != (sizeof(struct packet_general_header)+UnpackLen)) {
			break;
		}
	} while(1);

	close(RecdFile_fd);
	if (pSendPack) {
		free(pSendPack);
	}

	return Ret;
}
