/*
*********************************************************************************************************
*                                                                 (C) Copyright 2009
*                                                    Copyright hangzhou engin Inc , 2002
 *                                                                 All rights reserved.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*
*
* Filename      :
* Version       : V1.00
* Programmer(s) :
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/




/* ----------------- APPLICATION GLOBALS ------------------ */


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
the transmit layer code.
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "Usart_Drv.h"
#include "stdafx.h"
#include "md5.h"
//#include "date.h"
#include "ProtocolDefine.h"
#include "ManageTransmit.h"
#include "list.h"
#include "NormalTool.h"
//#include <ucos_ii.h>
#include "BusinessProtocol.h"
//#include "RecordApp.h"
//#include "RfApp.h"
#if defined(_WIFI_MX_EMW)   // 庆科
#include "Wifi_Emw.h"
#elif defined(_WIFI_HF_LPB) // 汉枫
//#include "Wifi_LPB.h"
#endif
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
////#include "extern_variable.h"
#include "LanMiddle.h"
//#include "GPRSComm.h"
#include "ManageSecurity.h"
#include "main_linuxapp.h"
#include "../include/common.h"
#include "../include/log.h"
#include "lc/lc.h"
#include "ManageTransmit.h"
#include "TCPCommApp.h"
#include "frsdb/frsdb.h"

//#define _TEST


/* extern variables */
extern UCHAR g_WMCVersion[2];
extern UCHAR g_WMCFirmware[2];
extern UCHAR g_bResetWMC;
extern int g_BusinessDataDiagnose;
extern int g_bDoNewConnectTest;
//int g_wifi_init_ok = 1;
//extern ULONG g_lDoorID;

//char macadd[18] ="00:33:44:96:00:87";//"00:33:20:30:40:07";
extern typUpgradeNotify g_UpgradeNotify[2];
extern typSystermInfo g_SystermInfo;
//extern typGprsState g_GprsState;
extern typDeviceSetup g_DeviceSetup;
extern typDeviceParameter g_tempServerInfo;
//extern typRfControl RfControl;

extern int BusinessDecode(uchar *data, int len);
//extern void reboot(void);
//extern int DisConnectServer(void);
extern void UpgradeFile(void);
extern int RemoteSendInitializeSession(ulong seq);
int GprsSendDataTest(void);


/* local variables */
ulong g_Sequence = 1;	// 传输层流水号
int g_DeliverCount=0, g_SubmitCount=0;
int g_RECVWELCOME = 0, g_initialized = 0;
static int MinPackLen	= 1;
static int PackLenSize	= 2;
static uint8_t DefKey[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

typTransmitControl g_TransmitControl;
typDEV_PARA_CTL g_BootPara;

/* private functions */
static int TransmitLayerAnalyze(uchar *indata, int len);
static int TransmitPacket(ulong cmd, ulong CommandState, ulong sequence, char *SourceAdd, uchar type, uchar msgID, uchar *Businessdata, int len);
static int CheckIndata(uchar *data, int chklen);
static int DoTransmitReceive(void);
int DoBusinessSend(void) ;
//static int MainSessionProcess(void);
//static void DoHeartBeat(void);
static int TestNewServerIPandPort(void);

static int head_of_transmit_layer_msg_fill(typTransmitLayerHead *head,const uchar *data)
{
	ConvertMemcpy(&head->len, data, 4);
	ConvertMemcpy(&head->cmd, data+4, 4);
	ConvertMemcpy(&head->cmdstate, data+8, 4);
	ConvertMemcpy(&head->sequence, data+12, 4);

	return 0;
}
/*
    transmit layer receive process
*/
int TransmitLayerAnalyze(uchar *indata, int len)
{
	if (indata==NULL || len<=0) {
		return -1;
	}
	uint8_t *SecurityData = NULL;
	int selen = 0;
	ushort salen = 0;				// 安全层数据长度
	int SaLenOf = 0;				// salen 计算的偏移
	typTransmitLayerHead head = {0};

	if (ProtVer/16 == 2) {
		if (len < 16) {
			return -1;
		}

		/* 1.传输层消息包头信息填充和打印 */
		head_of_transmit_layer_msg_fill(&head, indata);

		/* 2.传输层报文中包体的具体处理*/
		if (head.cmd == CMD_DELIVER_REQUEST) {
			if (len < 24) {
				return -1;
			}
			SaLenOf = strlen((char *)(indata+16));		// 目标地址字符串长度
			SecurityData = indata+SaLenOf+22;			// 指向加密数据
			SaLenOf = 24+SaLenOf;
		} else {
			switch (head.cmd) {
			case CMD_LOGIN_RESPONSE:
				g_TransmitControl.bLoginReceived = 1;
				break;
			case CMD_SUBMIT_RESPONSE:
				g_TransmitControl.bSubmitResponseReceived = 1;
				break;
			case CMD_LOGOUT_RESPONSE:
				g_TransmitControl.bLogoutReceived = 1;
				break;
			case CMD_GERENIC_NACK:
				g_TransmitControl.bGNACKReceived = 1;
				break;
			default :
				return -1;
				break;
			}
			g_TransmitControl.LastCommTime = GetSysTime();

			return 0;
		}
	} else {
		SecurityData = indata;
		if (len <= 2) {
			return -1;
		}
	}

	ConvertMemcpy(&salen, SecurityData, 2);
	if (salen>=MAX_BUSINESS_SINGLE_PACK_LEN || SaLenOf+salen>len) {
		Log("TMC", LOG_LV_DEBUG, "Frame is too long:l=%d, sl=%hu, offset=%d", len, salen, SaLenOf);
		if (ProtVer/16 == 2) {
			/* 对服务器通信中传输层deliver命令的response回复，*/
			TransmitPacket(CMD_DELIVER_RESPONSE, TRSMT_STATE_ERR_LENGTH, head.sequence, NULL, 0, 0, NULL, 0);
		}

		return -1;
	}
	uint8_t *sedataout = (uint8_t *)malloc(MAX_BUSINESS_SINGLE_PACK_LEN);

	//解密
	selen = indata+len-SecurityData;	//加密区的数据长度
	int Dret = SecurityLayerDecode(SecurityData, sedataout, &selen);
	if (Dret == TRSMT_STATE_OK) {
		char *tStr = NULL;
		if (selen>0 && (tStr=(char *)malloc(selen*3+1))!=NULL) {
			Hex2Str(tStr, sedataout, selen, ' ');
		}
		Log("BIZ", LOG_LV_DEBUG, "Decode data[%d]:%s.", selen, tStr);
		if (tStr) {
			free(tStr);
			tStr = NULL;
		}
		g_TransmitControl.LastCommTime = GetSysTime();
		if (ProtVer/16 == 2) {
			/* 对服务器通信中传输层deliver命令的response回复*/
			TransmitPacket(CMD_DELIVER_RESPONSE, TRSMT_STATE_OK, head.sequence, NULL, 0, 0, NULL, 0);
		}
		BusinessDecode(sedataout, selen);
	} else {
		Log("TMC", LOG_LV_DEBUG, "SecurityLayerDecode ret=%d.", Dret);
		if (ProtVer/16 == 2) {
			TransmitPacket(CMD_DELIVER_RESPONSE, TRSMT_STATE_ERR_SECURED_PACKET, head.sequence, NULL, 0, 0, NULL, 0);
		}
	}
	free(sedataout);

	return 0;
}

/*
transmit layer package and send to gprs send que(serial.cpp)

*/
// v2.*
int TransmitPacket(ulong cmd, ulong CommandState, ulong sequence, char *SourceAdd, uchar type, uchar msgID, uchar *Businessdata, int len)
{
	ulong SendLen = 0;
	uchar HeadData[BUSINESS_HEAD_LEN] = {0};
	uchar *SendBuf = (uchar *)malloc(MAX_BUSINESS_SINGLE_PACK_LEN);

	if (Businessdata == NULL) {
		len = 0;
	} else if (len < 0) {
		free(SendBuf);
		return TRSMT_STATE_ERR_UNKNOWN;
	}

	if (ProtVer/16 == 2) {
		SendLen = 4;
		ConvertMemcpy(SendBuf+SendLen, &cmd, 4);
		SendLen+= 4;
		ConvertMemcpy(SendBuf+SendLen, &CommandState, 4);
		SendLen+= 4;
		ConvertMemcpy(SendBuf+SendLen, &sequence, 4);
		SendLen+= 4;
	}
	// 1.根据传输层包头中的命令字，进行分类封装组包
	switch (cmd) {
	case CMD_LOGIN_REQUEST: {
		g_initialized = g_RECVWELCOME = 0;
		g_LogControl.BufStatus = LOG_BUF_UNVALID;
		CnctFlg = 1;

		double st = BooTime();

		while (st+30 > BooTime()) {
			if (g_LogControl.LogSate == LOG_STATE_DOING_LONGIN) {
				break;
			} else {
				usleep(10000);
			}
			SetDog(TASK_DOG_MANAGE);
		}
		if (st+30 <= BooTime()) {
			// 未连接成功，先断开
			CnctFlg = 0;
			free(SendBuf);
			SendBuf = NULL;
			return TRSMT_STATE_ERR_UNKNOWN;
		} else {
			CnctFlg = 2;				// 置为不主动修改状态
			if (ProtVer/16 == 2) {		// 2.x协议先发等回复
				struct tm rtctime;
				time_t NowT = time(NULL);

				// 上线认证的一些信息计算
				localtime_r(&NowT, &rtctime);
				char currtime[12] = {0};
				sprintf(currtime, "%02d%02d%02d%02d%02d", rtctime.tm_mon+1 , rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);
				uint8_t md5buf[54] = {0};
				memcpy(&md5buf[0], g_SystermInfo.macadd, 17);
				memset(&md5buf[17], 0, 9 );
				memcpy(&md5buf[26], &g_SystermInfo.ManagePassword[0], M_KEY_LEN);
				memcpy(&md5buf[26+M_KEY_LEN], &currtime[0], 10);
				uint8_t mp[16] = {0};
				Md5Data(md5buf, 52, mp);
				uint32_t ltime = strtoul(currtime, 0, 10);

				// 拷贝信息
				memcpy(SendBuf+SendLen, g_SystermInfo.macadd, 18);   //mac add
				SendLen+= 18;
				memcpy(SendBuf+SendLen, mp, 16);
				SendLen+= 16;
				ConvertMemcpy(SendBuf+SendLen, &ltime, 4);
				SendLen+= 4;
				SendBuf[SendLen++] = 0x22;
				ConvertMemcpy(SendBuf, &SendLen, 4);

				// 拷贝到发送区
				memcpy(g_LogControl.buf, SendBuf, SendLen);
				g_LogControl.len = SendLen;
				g_LogControl.BufStatus = LOG_BUF_VALID;
			}
			free(SendBuf);
			SendBuf = NULL;
			st = BooTime();
		}

		g_TransmitControl.bLoginReceived = 0;
		while (st+30 >  BooTime()) {
			DoTransmitReceive();
			if (ProtVer/16 == 2) {
				if (g_TransmitControl.bLoginReceived) {
					return TRSMT_STATE_OK;
				}
			} else {
				DoBusinessSend();
				if (g_RECVWELCOME && g_initialized) {
					return TRSMT_STATE_OK;
				}
			}
			SetDog(TASK_DOG_MANAGE);
			usleep(10000);
		}
		return TRSMT_STATE_ERR_UNKNOWN;
	}
	break;

	case CMD_LOGOUT_REQUEST: {
		// 来不及发登出指令，直接断开好了
		free(SendBuf);
		SendBuf = NULL;
		CnctFlg = 0;

		int kk = 0;
		while (g_LogControl.LogSate != LOG_STATE_IDLE) {
			SetDog(TASK_DOG_MANAGE);
			WatchDogFeed();
			if (kk++ > 10) {
				break;
			}
			sleep(1);
		}
		if (g_LogControl.LogSate == LOG_STATE_IDLE) {
			return TRSMT_STATE_OK;
		} else {
			if (type) {
				g_LogControl.LogSate = LOG_STATE_IDLE;
			}
			return TRSMT_STATE_ERR_UNKNOWN;
		}
	}
	break;

	case CMD_SUBMIT_RESQUEST: {
		if (ProtVer/16 == 2) {
			if (SourceAdd) {
				uchar tlen = strlen(SourceAdd)+1;
				memcpy(SendBuf+SendLen, SourceAdd, tlen);	//mac add
				SendLen+= tlen;
			} else {
				SendBuf[SendLen++] = 0;
			}
			memcpy(SendBuf+SendLen, &type, 1);				// 来源类型 1
			SendLen+= 1;
			ConvertMemcpy(SendBuf+SendLen, &len, 4);		// 数据长度 4
			SendLen+= 4;
			memcpy(SendBuf+SendLen, Businessdata, len);		// 数据
			SendLen+= len;
			ConvertMemcpy(SendBuf, &SendLen, 4);
		} else {
			if (len > 0 ) {
				memcpy(SendBuf, Businessdata, len);
				SendLen+= len;
			} else {
				free(SendBuf);
				SendBuf = NULL;
			}
		}

		g_SubmitCount++;
	}
	break;

	case CMD_DELIVER_RESPONSE:
		SendBuf[SendLen++] = 0;
		ConvertMemcpy(SendBuf, &SendLen, 4);
		g_DeliverCount++;
		break;

	case CMD_GERENIC_NACK:
		ConvertMemcpy(SendBuf, &SendLen, 4);
		break;

	default:
		free(SendBuf);
		return TRSMT_STATE_ERR_UNKNOWN;
	}

	if (SendBuf && SendLen>0) {
		memcpy(HeadData, &SendLen, 2);
		GprsSendDataPush(SendBuf, SendLen, HeadData);
		free(SendBuf);
	}

	return TRSMT_STATE_ERR_UNKNOWN;
}

/*
   仅检查长度，返回完整包长度to check the data in buf if legal
*/
static int CheckIndata(uchar *data, int chklen)
{
	uint32_t transmit_len = 0;
	//typProtocol protocol;

	if (chklen < MinPackLen) { // 总长度小于4, 数据太短
		printf("CheckIndata error: protocol.Length to short! chklen=%d\r\n", chklen);
		return (0);
	}

	ConvertMemcpy(&transmit_len, data, PackLenSize);
	if (transmit_len > (uint32_t)chklen) {
		printf("CheckIndata error: protocol.Length to long! transmit_len=%d, chklen=%d\r\n", transmit_len, chklen);
		return (0);
	} else {
		return (transmit_len);
	}
}

/*
     from bottom layer to receive data , then analyze it
*/
// TODO 如果接收的包不足整包，则会丢数据
static int DoTransmitReceive(void)
{
	UCHAR HeadData[BUSINESS_HEAD_LEN];
	int len, nResult, HaveProcessLen;
	UCHAR *m_udpbuf = sysmalloc(MAX_GPRS_LEN);

	memset(HeadData, 0, sizeof(HeadData));
	nResult = GprsRecvDataPop(m_udpbuf, HeadData);
	if (nResult > 0) {
		HaveProcessLen = 0;
		while (HaveProcessLen < nResult) {              // because of  many packet together, to unpacking the package
			SetDog(TASK_DOG_MANAGE);
			WatchDogFeed();

			//校验是否完整
			len = CheckIndata(&m_udpbuf[HaveProcessLen], nResult - HaveProcessLen);
			Log("TMC", LOG_LV_VERBOSE, "CheckIndata, ret=%d.", len);
			if (len >= MinPackLen) {
				TransmitLayerAnalyze(m_udpbuf+HaveProcessLen, len);
				HaveProcessLen += len;
			} else {
				break;
			}
		}
	}
	sysfree(m_udpbuf);

	return 1;
}

//send business layer data , if have data
int DoBusinessSend(void)
{
	UCHAR HeadData[BUSINESS_HEAD_LEN];
	char SourceAdd[16] = {0};
	int outlen, inlen;
	int k, type;
	UCHAR *outbuf = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
	UCHAR *inbuf = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);

	for ( k = 0; k < 2; k++) {  // 往往一个指令包会有两包回应，一个ACK和一个数据包
		memset(HeadData, 0, sizeof(HeadData));
		/* 1.业务层报文的发送队列弹出*/
		inlen = ManageBusinessDataPop(inbuf, HeadData);
		if (inlen <= 0) {
			uint32_t tSeq = g_SystermInfo.BusinessSeq;
			uint8_t *pBuf = NULL;

			inlen = FrDbGet(30, &tSeq, &pBuf);
			if (inlen > 0) {
				if (inlen < MAX_BUSINESS_SINGLE_PACK_LEN) {
					if (ProtVer/16 == 2) {
						ConvertMemcpy(pBuf+4, &tSeq, 4);
					} else {
						ConvertMemcpy(pBuf+3, &tSeq, 4);
					}
					g_SystermInfo.BusinessSeq = tSeq+1;
					memcpy(inbuf, pBuf, inlen);
					HeadData[2] = SOURCE_TYPE_ZKQ;
				} else {
					Log("TMC", LOG_LV_DEBUG, "FrDbGet=%d while bufmax=%d", inlen, MAX_BUSINESS_SINGLE_PACK_LEN);
					free(pBuf);
					break;
				}
				free(pBuf);
			} else {
				break;
			}
		}

		type = HeadData[2];
		memcpy(SourceAdd, &HeadData[3], 16);
		char tStr[inlen*3+1];

		Hex2Str(tStr, inbuf, inlen, ' ');
		Log("TMC", LOG_LV_DEBUG, "ManageBusinessDataPop ret=%d, Data:%s.", inlen, tStr);
		/* 2.对业务层报文进行安全层的加密承载*/
		if (ProtVer/16 == 2) {
			SecurityLayerEncode(inbuf, inlen, ENCRYPT_MODE_DES, outbuf, &outlen);
		} else {
			/* V3中加密方式直接使用传进去的参数(放在b0-b2中) */
			SecurityLayerEncode(inbuf, inlen, AES_MODE_CBC, outbuf, &outlen);
		}
		/* 3.进行传输层报文的组包发送*/
		TransmitPacket(CMD_SUBMIT_RESQUEST, TRSMT_STATE_OK, g_Sequence++, SourceAdd, type, 0, outbuf, outlen);
	}
	sysfree(inbuf);
	sysfree(outbuf);
	return 1;
}

/*
     the main protocol cycle process function
*/
void setDefdevice(void)
{
	printf("@@@restore default Systerm para@@@\r\n");
	memset(&g_DeviceSetup, 0, sizeof(g_DeviceSetup));

	printf("\r\n*************************************\r\n");
	printf("Deveice config list\r\n");
	printf("count=%d\r\n", g_DeviceSetup.count);

	for (int i=0; i<MAX_SUBDEVICE; i++) {
		printf("i=%2d, LogicCode=%d, address=%s, MatchCode=%04lX, ServerIP=%04lX, ServerPort=%02X, type=%d, status=%d\r\n", i, g_DeviceSetup.Device[i].LogicCode, \
			   g_DeviceSetup.Device[i].address, g_DeviceSetup.Device[i].DeviceParameter.MatchCode, g_DeviceSetup.Device[i].DeviceParameter.ServerIP, \
			   g_DeviceSetup.Device[i].DeviceParameter.ServerPort, g_DeviceSetup.Device[i].type, g_DeviceSetup.Device[i].Status);
	}

	printf("*************************************\r\n");

	memset((uchar *)&g_UpgradeNotify, 0, sizeof(g_UpgradeNotify));

	memset((uchar *)&g_SystermInfo,0, sizeof(g_SystermInfo) );

	memcpy(&g_SystermInfo.ManagePassword[0], DefKey, M_KEY_LEN);
	memcpy(&g_SystermInfo.DEKKey[0], DefKey, M_KEY_LEN);
//	g_SystermInfo.heartInterval = 600;  //10 minites, 发送心跳包时间间隔
//	g_SystermInfo.AlertAllow = 0xff;

	//ConvertMemcpy((uchar *)&(g_SystermInfo.Version), g_WMCVersion, 2);
	//g_SystermInfo.Version = *((ushort*)g_WMCVersion);
//	memcpy(&g_SystermInfo.Version[0], g_WMCVersion, 2);

	//printf("g_SystermInfo.Version=%d\r\n", g_SystermInfo.Version);
}


void StoreSystemPara(void)
{
	USHORT len, totallen;
	USHORT crc;
	UCHAR * tempbuf= sysmalloc(2048);

	SetDog(TASK_DOG_MANAGE);
	WatchDogFeed();

//	memcpy(&tempbuf[4], (uchar *)&g_SystermInfo.ManagePassword[0], 16);
//	memcpy(&tempbuf[20], (uchar *)&g_SystermInfo.UserPassword, 4);
//	memcpy(&tempbuf[24], (uchar *)&g_SystermInfo.heartInterval, 4);
//	memcpy(&tempbuf[28], (uchar *)&g_SystermInfo.AlertAllow, 1);

	//reserver 16 byte ismi
	memcpy(&tempbuf[29], (uchar *)&g_SystermInfo.StoreImsi, 16);

	memcpy(&tempbuf[45], (uchar *)&g_SystermInfo.ManagePassword[0], M_KEY_LEN);	//new added on 12.06.22
	memcpy(&tempbuf[61], (uchar *)&g_SystermInfo.DEKKey[0], M_KEY_LEN);	//new added on 12.06.22
//	memcpy(&tempbuf[77], (uchar *)&g_SystermInfo.Version[0], 2);	//new added on 12.08.13

	len = sizeof(g_DeviceSetup);
	printf("set up len :%d\r\n", len);
	memcpy(&tempbuf[79], (uchar *)&g_DeviceSetup, len);
	totallen = 79 + len;
	len = sizeof(g_UpgradeNotify);
	printf("StoreSystemPara... len=%d, sizeof(g_UpgradeNotify)=%d\r\n", len, sizeof(g_UpgradeNotify[0]));


	memcpy(&tempbuf[totallen], (uchar *)g_UpgradeNotify, len);

	totallen += len;
	memcpy(tempbuf, (uchar *)&totallen, 2);
	crc = CRC16(&tempbuf[4], totallen-4);
	memcpy(&tempbuf[2], (uchar *)&crc, 2);
	printf("store System para flash totallen:%d\r\n", totallen);
	Flash_EmulateE2prom_WriteByte(tempbuf, FLASH_SYSTERM_PARA_ADD, totallen);
	/***********************************************************************
	systerm write to flash:
	1.ulong lServerIP
	2.ushort ServerPort
	3.uchar  ManagePassword[16]
	4.ulong UserPassword
	5.ulong  heartInterval
	6.uchar AlertAllow;
	7. char  APN[30]
	************************************************************************/
	sysfree(tempbuf);
}

int RestoreSystemPara(void)
{
	USHORT len=0, totallen;
	USHORT crc, readcrc;
	UCHAR tempbuf[2048];

	SetDog(TASK_DOG_MANAGE);
	WatchDogFeed();
	Flash_EmulateE2prom_ReadByte((uchar *)&len, FLASH_SYSTERM_PARA_ADD, 2);
	if (len > 2048) {
		printf("restore Systerm para len error:%d\r\n", len);
		return 0;
	}
	Flash_EmulateE2prom_ReadByte(tempbuf, FLASH_SYSTERM_PARA_ADD, len);
	memcpy((uchar *)&readcrc, &tempbuf[2], 2);
	if (len < 4) {
		return 0;
	}
	crc = CRC16(&tempbuf[4], len-4);
	if (readcrc != crc) {
		printf("restore Systerm para crc error, store:%04x, should:%04x \r\n", readcrc, crc);
		return 0;
	}
//	memcpy((uchar *)&g_SystermInfo.ManagePassword[0], &tempbuf[4], 16);
//	memcpy((uchar *)&g_SystermInfo.UserPassword, &tempbuf[20], 4);
//	memcpy((uchar *)&g_SystermInfo.heartInterval, &tempbuf[24], 4);  // old 1
//	memcpy((uchar *)&g_SystermInfo.AlertAllow, &tempbuf[28], 1);
	memcpy((uchar *)&g_SystermInfo.StoreImsi[0], &tempbuf[29], 16);

	memcpy((uchar *)&g_SystermInfo.ManagePassword[0], &tempbuf[45], M_KEY_LEN);	//new added on 12.06.22
	memcpy((uchar *)&g_SystermInfo.DEKKey[0], &tempbuf[61], M_KEY_LEN);	//new added on 12.06.22
//	memcpy((uchar *)&g_SystermInfo.Version[0], &tempbuf[77], 2);	//new added on 12.08.13

//	printf("g_SystermInfo.heartInterval=%ld\r\n", g_SystermInfo.heartInterval);

//	printf("g_SystermInfo.Version=0x%04X, %d, g_WMCVersion=0x%04X, %d\r\n", *(ushort*)g_SystermInfo.Version, *(ushort*)g_SystermInfo.Version, *(ushort*)g_WMCVersion, *(ushort*)g_WMCVersion);
//	if (memcmp((uchar *)&g_SystermInfo.Version[0], g_WMCVersion, 2) != 0 && *(ushort*)g_SystermInfo.Version < 0x0206) {	// 软件版本号小于V2.6的清空配置列表
//		printf("Version is different and the Version is less than 0206\r\n");
//		return 0;
//	}

	len = sizeof(g_DeviceSetup);
	memcpy((uchar *)&g_DeviceSetup, &tempbuf[79], len);
	totallen = 79+len;

#if 1
	printf("\r\n*************************************\r\n");
	printf("Deveice config list\r\n");
	printf("count=%d\r\n", g_DeviceSetup.count);
	for (int i=0; i<MAX_SUBDEVICE; i++) {
		printf("i=%2d, LogicCode=%d, address=%s, MatchCode=%04lX, ServerIP=%04lX, ServerPort=%02X, type=%d, status=%d\r\n", i, g_DeviceSetup.Device[i].LogicCode, \
			   g_DeviceSetup.Device[i].address, g_DeviceSetup.Device[i].DeviceParameter.MatchCode, g_DeviceSetup.Device[i].DeviceParameter.ServerIP, \
			   g_DeviceSetup.Device[i].DeviceParameter.ServerPort, g_DeviceSetup.Device[i].type, g_DeviceSetup.Device[i].Status);
	}
	printf("*************************************\r\n");
#endif

	len = sizeof(g_UpgradeNotify);
	memcpy((uchar *)g_UpgradeNotify, &tempbuf[totallen], len);

	printf("g_UpgradeNotify[FLAG_UPGRADE_CTRL]: bUpgradeNotify=%d, offset=%ld, size=%ld, downloadsuccess=%d\r\n",
		   g_UpgradeNotify[FLAG_UPGRADE_CTRL].bUpgradeNotify, g_UpgradeNotify[FLAG_UPGRADE_CTRL].offset, g_UpgradeNotify[FLAG_UPGRADE_CTRL].size, g_UpgradeNotify[FLAG_UPGRADE_CTRL].downloadsuccess);
	printf("g_UpgradeNotify[FLAG_UPGRADE_LOCK]: bUpgradeNotify=%d, offset=%ld, size=%ld, downloadsuccess=%d\r\n",
		   g_UpgradeNotify[FLAG_UPGRADE_LOCK].bUpgradeNotify, g_UpgradeNotify[FLAG_UPGRADE_LOCK].offset, g_UpgradeNotify[FLAG_UPGRADE_LOCK].size, g_UpgradeNotify[FLAG_UPGRADE_LOCK].downloadsuccess);

	//while(1);
	//g_UpgradeNotify.offset = 70648;
	return 1;
}

//int StoreBootPara(void)
//{
//	UCHAR tempbuf[100];
//	USHORT len, crc;
//
//	SetDog(TASK_DOG_MANAGE);
//	WatchDogFeed();
//	memcpy(tempbuf, &g_BootPara, sizeof(g_BootPara));
//	len = sizeof(g_BootPara) - 4;
//	crc = CRC16(&tempbuf[4], len);
//	memcpy(tempbuf, &len, 2);
//	memcpy(&tempbuf[2], &crc, 2);
//	Flash_EmulateE2prom_WriteByte(tempbuf, FLASH_BOOT_PARA_ADD, len+4);
//	printf("strore boot para len:%d\r\n", len+4);
//
//	return 1;
//}

//
//int SetDefaultBootPara(void)
//{
//	uchar tempbuf0[]= {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
//	uchar mac[] = {0x00, 0x33, 0x44, 0x96, 0x00, 0x00}; //0x77
//	printf("@@@restore default boot para@@@\r\n");
//	//uchar mac[] = {0x00, 0x33, 0x44, 0x96, 0x00, 0x87};
//	memcpy(&g_BootPara.cApn[0], "CMNET", 5);            //apn
//	g_BootPara.cApn[5] = '\0';
//	memcpy(&g_BootPara.cComKey[0], tempbuf0, 16);  //comkey
//	memcpy(&g_BootPara.NetInfo.cMac[0], mac, 6);
//	g_BootPara.NetInfo.port = 2019;					// 155:2018    43:2019
//	g_BootPara.NetInfo.ip = (218ul<< 24) +(108ul<<16)+(52ul<<8)+(30);  //"122.224.226.42", 2018        218.108.52.30          //0x7AE0E22A;  // 122.224.226.42
//
//	g_BootPara.DevId.cFactory = 0;
//	g_BootPara.DevId.cDevType = 6;
//	memset(g_BootPara.DevId.cProdSeries, 0, 2);
//	memset(g_BootPara.DevId.cSerialNo, 0, 8);
//
//#if defined(_PCB_2_0) || defined(_PCB_3_1)
//	sprintf(g_SystermInfo.macadd, "%02X:%02X:%02X:%02X:%02X:%02X", g_BootPara.DevId.cSerialNo[2], g_BootPara.DevId.cSerialNo[3],
//			g_BootPara.DevId.cSerialNo[4], g_BootPara.DevId.cSerialNo[5], g_BootPara.DevId.cSerialNo[6], g_BootPara.DevId.cSerialNo[7]);
//	/*
//		memcpy(g_SystermInfo.macadd, (uchar *)&g_BootPara.DevId, sizeof(g_BootPara.DevId));
//		memset(g_SystermInfo.macadd+sizeof(g_BootPara.DevId), 0, 18-sizeof(g_BootPara.DevId));
//		//memcpy(g_SystermInfo.DevId, (uchar *)&g_BootPara.DevId, sizeof(g_BootPara.DevId));
//		*/
//#elif defined(_PCB_1_2)
//	sprintf(g_SystermInfo.macadd, "%02X:%02X:%02X:%02X:%02X:%02X", g_BootPara.NetInfo.cMac[0], g_BootPara.NetInfo.cMac[1],
//			g_BootPara.NetInfo.cMac[2], g_BootPara.NetInfo.cMac[3], g_BootPara.NetInfo.cMac[4], g_BootPara.NetInfo.cMac[5]);
//#else
//#error No defined PCB type
//#endif
//
//	return 1;
//}
//
//int RestoreBootPara(void)
//{
//	int i, len;
//	uchar tempbuf[1000];
//
//	WatchDogFeed();
//	len =  sizeof(typDEV_PARA_CTL);
//
//	printf("restore boot para len:%d\r\n", len);
//
//	Flash_EmulateE2prom_ReadByte((uchar *)&g_BootPara, FLASH_BOOT_PARA_ADD, len);
//	if (g_BootPara.quantity > 1000) {
//		printf("restore para len error:%d\r\n", len);
//		return 0;
//	} else if (g_BootPara.quantity < 5) {
//		return 0;
//	}
//	memcpy(tempbuf, &g_BootPara, len);
//	printf("quality:%d\r\n", g_BootPara.quantity);
//
//	if (g_BootPara.crc == CRC16(&tempbuf[4], g_BootPara.quantity)) {
//		printf("boot para crc true\r\n");
//	} else {
//		printf("boot para crc error\r\n");
//		return 0;
//	}
//
//	printf("boot para:\r\n");
//	printf("com key:");
//	for (i = 0; i < 16; i++) {
//		printf("%02X ",g_BootPara.cComKey[i]);
//	}
//	printf("\r\n");
//	printf("APN:");
//	for (i = 0; i < 30; i++) {
//		if (g_BootPara.cApn[i] == 0) {
//			break;
//		}
//		printf("%02X ",g_BootPara.cApn[i]);
//	}
//	printf("\r\n");
//	printf("mac addr:");
//	for (i = 0; i < 6; i++) {
//		printf("%02X ", g_BootPara.NetInfo.cMac[i]);
//	}
//	printf("\r\n");
//	ulong ip = g_BootPara.NetInfo.ip;
//	printf("server ip:%ld.%ld.%ld.%ld\r\n", (ip>>24)&0xff, (ip>>16)&0xff,(ip>>8)&0xff, ip&0xff);
//	ushort port = g_BootPara.NetInfo.port;
//	printf("server port:%d\r\n", port);
//	//printf("device ID,year:%d, month:%d,seriaNo:%d\r\n", g_BootPara.DevId.cYear, g_BootPara.DevId.cMon, g_BootPara.DevId.SerialNo);
//	//printf("device ID,cFactory:%d, cDevType:%d, cProdSeries:%s, cSerialNo:%s\r\n", g_BootPara.DevId.cFactory, g_BootPara.DevId.cDevType, g_BootPara.DevId.cProdSeries, g_BootPara.DevId.cSerialNo);
//
//	sprintf(g_SystermInfo.macadd, "%02X:%02X:%02X:%02X:%02X:%02X", g_BootPara.DevId.cSerialNo[2], g_BootPara.DevId.cSerialNo[3],
//			g_BootPara.DevId.cSerialNo[4], g_BootPara.DevId.cSerialNo[5], g_BootPara.DevId.cSerialNo[6], g_BootPara.DevId.cSerialNo[7]);
//	printf("series NO: %s\r\n", g_SystermInfo.macadd);
//
//	return 1;
//}

//int SetDefaultCameraPara(void)
//{
//	g_CamPara.TimeAlmCapt = 600;
//	g_CamPara.TimeNormalCapt = 600;
//
//	return 1;
//}

/* camera captue time config */
//int RestoreCameraPara(void)
//{
//	int len;
//	uchar tempbuf[1000];
//
//	WatchDogFeed();
//	len = sizeof(typCAMPARA);
//
//	printf("restore camera para len:%d\r\n", len);
//
//	Flash_EmulateE2prom_ReadByte( (uchar *)&g_CamPara, FLASH_CAM_PARA_ADD, len );
//	if (g_CamPara.quantity > 512) {
//		printf("restore para len error:%d\r\n", len);
//		return 0;
//	} else if (g_CamPara.quantity < 5) {
//		return 0;
//	}
//	memcpy(tempbuf, (uchar *)&g_CamPara, len);
//	printf("quality:%d\r\n", g_CamPara.quantity);
//
//	if (g_CamPara.crc == CRC16(&tempbuf[4], g_CamPara.quantity)) {
//		printf("camera para crc true\r\n");
//	} else {
//		printf("camera para crc error\r\n");
//		return 0;
//	}
//
//	printf("camera para:\r\n");
//	printf( "Time of alm capt:%ld\n\r", g_CamPara.TimeAlmCapt );
//	printf( "Time of normal capt:%ld\n\r", g_CamPara.TimeNormalCapt );
//
//	return 1;
//}

//int StoreCameraPara(void)
//{
//	UCHAR tempbuf[100];
//	USHORT len, crc;
//
//	WatchDogFeed();
//	memcpy(tempbuf, &g_CamPara, sizeof(g_CamPara));
//	len = sizeof(g_CamPara) - 4;
//	crc = CRC16(&tempbuf[4], len);
//	memcpy(tempbuf, &len, 2);
//	memcpy(&tempbuf[2], &crc, 2);
//	Flash_EmulateE2prom_WriteByte(tempbuf, FLASH_CAM_PARA_ADD, len+4);
//	//printf("StoreCameraPara len:%d\n\r", len+4);
//	//ser_printf("TimeAlmCapt=%d, TimeNormalCapt=%d...\n\r", g_CamPara.TimeAlmCapt, g_CamPara.TimeNormalCapt);
//
//	return 1;
//}

/* 设置短信发送默认参数 */
//int SetDefaultSmsPara(void)
//{
//	memset((char *)&g_SmsPara, 0 ,sizeof(g_SmsPara));
//
//	return 1;
//}

/* sms para config */
//int RestoreSmsPara(void)
//{
//	int len;
//	uchar tempbuf[1000];
//
//	WatchDogFeed();
//	len = sizeof(typSMS_PARA);
//
//	printf("restore camera para len:%d\r\n", len);
//
//	Flash_EmulateE2prom_ReadByte( (uchar *)&g_SmsPara, FLASH_SMS_PARA_ADD, len );
//	if (g_SmsPara.quantity > FLASH_SMS_PARA_MAX_LEN) {
//		printf("restore para len error:%d\r\n", len);
//		return 0;
//	} else if (g_SmsPara.quantity < 5) {
//		return 0;
//	}
//	memcpy(tempbuf, (uchar *)&g_SmsPara, len);
//	printf("quality:%d\r\n", g_SmsPara.quantity);
//
//	if (g_SmsPara.crc == CRC16(&tempbuf[4], g_SmsPara.quantity)) {
//		printf("camera para crc true\r\n");
//	} else {
//		printf("camera para crc error\r\n");
//		return 0;
//	}
//
//	return 1;
//}
//
//int StoreSmsPara(void)
//{
//	UCHAR tempbuf[100];
//	USHORT len, crc;
//
//	WatchDogFeed();
//	memcpy(tempbuf, (uchar *)&g_SmsPara, sizeof(g_SmsPara));
//	len = sizeof(g_SmsPara) - 4;
//	crc = CRC16(&tempbuf[4], len);
//	memcpy(tempbuf, &len, 2);
//	memcpy(&tempbuf[2], &crc, 2);
//	Flash_EmulateE2prom_WriteByte(tempbuf, FLASH_SMS_PARA_ADD, len+4);
//	//printf("StoreCameraPara len:%d\n\r", len+4);
//	//ser_printf("TimeAlmCapt=%d, TimeNormalCapt=%d...\n\r", g_SmsPara.TimeAlmCapt, g_SmsPara.TimeNormalCapt);
//
//	return 1;
//}

/* 设置设备编码默认参数 */
//int SetDefaultDeviceCodePara(void)
//{
//	memset((char *)&g_DeviceCodePara, 0 ,sizeof(g_DeviceCodePara));
//
//	return 1;
//}

/* DeviceCode para config */
//int RestoreDeviceCodePara(void)
//{
//	int len;
//	uchar tempbuf[1000];
//
//	WatchDogFeed();
//	len = sizeof(typDEV_PARA_DEVICECODE);
//
//	sp_printf("restore DeviceCode para len:%d\r\n", len);
//
//	Flash_EmulateE2prom_ReadByte( (uchar *)&g_DeviceCodePara, FLASH_DCODE_PARA_ADD, len );
//	if (g_DeviceCodePara.quantity > FLASH_DCODE_PARA_MAX_LEN) {
//		sp_printf("restore para len error:%d\r\n", len);
//		return 0;
//	} else if (g_DeviceCodePara.quantity < 5) {
//		return 0;
//	}
//	memcpy(tempbuf, (uchar *)&g_DeviceCodePara, len);
//	sp_printf("quality:%d\r\n", g_DeviceCodePara.quantity);
//
//	if (g_DeviceCodePara.crc == CRC16(&tempbuf[4], g_DeviceCodePara.quantity)) {
//		sp_printf("DeviceCode para crc true\r\n");
//	} else {
//		sp_printf("DeviceCode para crc error\r\n");
//		return 0;
//	}
//
//	return 1;
//}

//int StoreDeviceCodePara(void)
//{
//	UCHAR tempbuf[100];
//	USHORT len, crc;
//	WatchDogFeed();
//	memcpy(tempbuf, (uchar *)&g_DeviceCodePara, sizeof(g_DeviceCodePara));
//	len = sizeof(g_DeviceCodePara) - 4;
//	crc = CRC16(&tempbuf[4], len);
//	memcpy(tempbuf, &len, 2);
//	memcpy(&tempbuf[2], &crc, 2);
//	Flash_EmulateE2prom_WriteByte(tempbuf, FLASH_DCODE_PARA_ADD, len+4);
//	//sp_printf("StoreDeviceCodePara len:%d\n\r", len+4);
//	//ser_printf("IDcode=%d, AddrCode=%d...\n\r", g_DeviceCodePara.IDcode, g_DeviceCodePara.AddrCode);
//
//	return 1;
//}



/* 摄像头联动拍摄时间设置 */
//int WMCParaConfig (void)
//{
//	/* restore camera para */
//	if ( !RestoreCameraPara() ) {
//		SetDefaultCameraPara();
//		StoreCameraPara();
//		ser_printf("RestoreCameraPara fail, TimeAlmCapt=%ld, TimeNormalCapt=%ld...\n\r", g_CamPara.TimeAlmCapt, g_CamPara.TimeNormalCapt);
//	} else {
//		ser_printf("RestoreCameraPara OK, TimeAlmCapt=%ld, TimeNormalCapt=%ld...\n\r", g_CamPara.TimeAlmCapt, g_CamPara.TimeNormalCapt);
//	}
//
//	/* restore sms para */
//	if ( !RestoreSmsPara() ) {
//		SetDefaultSmsPara();
//		StoreSmsPara();
//		ser_printf("RestoreSmsPara fail.\r\n");
//	} else {
//		ser_printf("RestoreSmsPara OK, sms para:\r\n");
//
//		printf("sms DoorName:%s\n\r", g_SmsPara.DoorName);
//		printf("sms PhoneNumber:%s\n\r", g_SmsPara.PhoneNumber);
//		//printf("sms SCA:%s\n\r", g_SmsPara.SCA);
//
//#ifdef _FLOW_STATISTICS
//#if 1
//		g_SmsPara.g_total_flow = 0;
//		StoreSmsPara();
//#endif
//		printf("sms g_total_flow:%dB(%fKB)\r\n", g_SmsPara.g_total_flow, g_SmsPara.g_total_flow/1024.0);
//#endif
//	}
//
//
//	WatchDogFeed();
//
//	return 1;
//}

void CheckDeviceCount()
{
	uchar i, count;

	for (i=0, count=0; i<MAX_SUBDEVICE; i++) {
		if (g_DeviceSetup.Device[i].Status) {
			count++;
		}
	}

	printf("CheckDeviceCount finished. count=%d, g_DeviceSetup.count=%d\r\n", count, g_DeviceSetup.count);
	if (g_DeviceSetup.count != count) {
		g_DeviceSetup.count = count;
//		OS_CPU_SR cpu_sr;
				StoreSystemPara();
				//StoreSystemPara();
	}
}

void linux_SystemParaInit(void)
{
	g_BootPara.NetInfo.ip = inet_addr(g_ServerInfo.strServerIP);
	g_BootPara.NetInfo.port = g_ServerInfo.ServerPort;
	GetLocalMac(g_BootPara.NetInfo.cMac);
//	memcpy(&g_BootPara.DevId.cSerialNo[2], g_BootPara.NetInfo.cMac, 6);

	sprintf(g_SystermInfo.macadd, "%02X:%02X:%02X:%02X:%02X:%02X", g_BootPara.NetInfo.cMac[0], g_BootPara.NetInfo.cMac[1], \
			g_BootPara.NetInfo.cMac[2], g_BootPara.NetInfo.cMac[3], g_BootPara.NetInfo.cMac[4], g_BootPara.NetInfo.cMac[5]);

	printf("series NO: %s\r\n", g_SystermInfo.macadd);
//	printf("mac addr: %s\r\n", g_SystermInfo.macadd);

}

#ifdef USE_LINUX
extern int CPortDeviceOperate(typDeviceOperate strDeviceOperate, int port);
#endif

void SystemParaInit(void)
{
	memset((uchar *)&g_TransmitControl,0, sizeof(g_TransmitControl));

	memset(cveci, 0, 10);
	if (ProtVer/16 == 2) {
		DevCodeSize = 2;
	} else {
		DevCodeSize = 4;
		strcpy(cveci, "engin168");
	}
#ifdef _OPEN_ALARM
	printf("OPEN_ALARM: ON, ");
#else
	printf("OPEN_ALARM: OFF, ");
#endif

#ifdef _USE_UPDATE_MODE
	printf("UPDATE_MODE: ON, ");
#else
	printf("UPDATE_MODE: OFF, ");
#endif

#ifdef _USE_DOG_SOFTWATCHDOG
	printf("SOFT_WATCHDOG: ON.\r\n");
#else
	printf("SOFT_WATCHDOG: OFF.\r\n");
#endif

	if (RestoreSystemPara()!= 1) {
		setDefdevice();
//		OS_CPU_SR cpu_sr;
				StoreSystemPara();
				//StoreSystemPara();
		//RemoteSendOnlineNotify(0);
	}
#if 1

	//memset((char*)g_UpgradeNotify, 0, sizeof(g_UpgradeNotify));
//#ifdef _DEBUF_USE_TEST_BOOT_PARA
//	SetDefaultBootPara();
//	//   StoreBootPara();
//#else
//
//	if (!RestoreBootPara()) {
//		SetDefaultBootPara();
//		StoreBootPara();
//	}
//#endif


	CheckDeviceCount();
	InitGprsSendBuffer();
	InitGprsRecvBuffer();
	InitMangeBusinessBuffer();
	InitLockBusinessBuffer();
//	RecordControlInit();
	memset(&g_LogControl, 0, sizeof(g_LogControl));
#ifdef USE_LINUX
	linux_SystemParaInit();


#endif
#endif
}

/* 只上传主控器上线
 * type: 0-上传主控器和门锁上线信息, 2-只上传主控器上线(默认)
 */
void SendOnLineNotify(int type)
{
	RemoteSendOnlineNotify(type);

	return ;
}

//extern int g_tcpConnected;
//static int MainSessionProcess(void)

//#ifdef _debug_alway_connect_server
//void always_connect_server_heartbeat()
//{
//	ulong currtime;
//
//	/* 测试时候，心跳跑调整到30秒一次(实际程序关闭该宏定义)*/
//	/* for test */
//#ifdef _debug_alway_connect_server
//	g_SystermInfo.heartInterval = 90;   // 1分半钟
//#else
//	g_SystermInfo.heartInterval = 600;	// 心跳包传输时间间隔(单位秒)
//#endif
//	currtime = GetSysTime();
//	//printf("DoHeartBeat, diff ConnectTime=%ld, diff BeatSendTime=%ld\r\n", g_TransmitControl.LastConnectTime-currtime, g_TransmitControl.LastBeatSendTime-currtime);
//
//	TimeSync();
//}
//#endif

void * TaskManageComm(void *count)
{
	int Ret = -1;
	ulong currtime;

	if (ProtVer/16 == 2) {
		MinPackLen	= 16;
		PackLenSize	= 4;
	} else {
		MinPackLen	= 4;
		PackLenSize	= 2;
	}

	g_TransmitControl.TimeSync.LastWMCSyncTime = -43200;
	while (1) {//OSTimeDly(20);
		g_DeliverCount = g_SubmitCount = 0;
		SetDog(TASK_DOG_MANAGE);

		/* 1.设备上线通知从未发送过则发送Online Notify指令，置位联网请求标志位*/
		/* 2.有业务数据发送，则会置位联网标志位，则进行login和业务数据发送*/
		/* 代码如下*/
		if (!g_TransmitControl.bSendOnlineNotify || g_TransmitControl.bRequestConnect ||
				(g_TransmitControl.LastCommTime+10 < GetSysTime())) {
			Log("TMC", LOG_LV_DEBUG, "bRequestConnect=%d, g_bDoNewConnectTest=%d",
				g_TransmitControl.bRequestConnect, g_bDoNewConnectTest);

			/////////////////////////////////////////////////////////////////////////////
			/* 首先发送login指令,循环体代码如下*/
			int Cnt = 0;        // 记录重试次数
			do {
				// 先断开
				TransmitPacket(CMD_LOGOUT_REQUEST, TRSMT_STATE_OK, g_Sequence++, NULL, 1, 0, NULL, 0);
				Log("TMC", LOG_LV_DEBUG, "Start to login.");
				// 100为主要锁的ID
				Ret = LcGetOnline(100);
				printf("LcGetOnline ret=%d\n", Ret);
				if (Ret != 1) {
					Ret = 0;
				}
				// 阻塞式登录
				Ret = TransmitPacket(CMD_LOGIN_REQUEST, (TRSMT_STATE_OK|(Ret << 8)|0x200),
									 g_Sequence++, NULL, 0, 0, NULL, 0);
				Log("TMC", LOG_LV_DEBUG, "Login ret=%d, cnt=%d.", Ret, Cnt);
				SetDog(TASK_DOG_MANAGE);
				if (Ret == TRSMT_STATE_OK) {	// 登录成功
					break;
				} else {	// 登录失败
					sleep(5);   //if login fail ,should 10 seconds  to reconnect
				}
			} while (++Cnt < 5);    // 最多尝试登录5次, 超过5次则认为本次登录失败
			/* 首先发送login指令,循环体代码如上*/
			/////////////////////////////////////////////////////////////////////////////

			// 登陆失败, the server ip or port is incorrect，退回到大循环体
			if (Cnt >= 5) {
				TransmitPacket(CMD_LOGOUT_REQUEST, TRSMT_STATE_OK, g_Sequence++, NULL, 1, 0, NULL, 0);

				g_TransmitControl.bRequestConnect=1;		// 该标志位用于协议服务器连接(connect server)
				continue;
			} else {	// has login OK
				g_LogControl.LogSate = LOG_STATE_HAVE_LONGIN;

				// TODO 使用别的方法避免重复吧，否则重要数据会丢掉
				// TODO:登录成功后，考虑先清空ManageBzBuf业务层数据队列
				/* 若不清空，则列入SIM卡拔出情况或者欠费情况下，脱机刷卡记录或者报警记录
				会首先弹入到服务器业务层报文队列中而且不会去做传输层封装弹入到
				无线网络进行发送。若业务报文发送超时后，则会弹入脱机数据存储队列。
				当无线网络连接成功后，则会出现业务报文队列和脱机数据存储队列
				中的报文重复发送的情况*/
//				InitMangeBusinessBuffer();

			}

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			while (g_LogControl.LogSate == LOG_STATE_HAVE_LONGIN) {
				/* 登入成功标志位清除*/
				g_TransmitControl.bRequestConnect = 0;
				// 时间同步
				TimeSync();

				if (g_bDoNewConnectTest) {	// has receive sms to change serverIP or port
					Log("TMC", LOG_LV_DEBUG, "g_bDoNewConnectTest=%d", g_bDoNewConnectTest);
										g_TransmitControl.bRequestConnect=1;
										break;
				}
				/* 软喂狗操作*/
				SetDog(TASK_DOG_MANAGE);
				//sp_printf("SetDog TASK_DOG_MANAGE 5...\r\n");
				WatchDogFeed();

				if (!g_TransmitControl.bSendOnlineNotify) {
					SendOnLineNotify(0);
					//OS_CPU_SR cpu_sr;
										g_TransmitControl.bSendOnlineNotify = 1;
									}

				// 主要的业务发送接收
				DoTransmitReceive();
				DoBusinessSend();

				/* 程序升级处理*/
				if (g_UpgradeNotify[FLAG_UPGRADE_CTRL].bUpgradeNotify || g_UpgradeNotify[FLAG_UPGRADE_LOCK].bUpgradeNotify) { // whether need upgrade
					UpgradeFile();
				}

				// 重启
				if (1 == g_bResetWMC) {
					break;
				}

				usleep(10000);

				/* 每隔1秒，打印一下时间，时间到了则断开服务器连接*/
				currtime = GetSysTime();
////////////////////////////////////////////////////////////
				if (GPRS_BUSINESS_WAIT_TIME>0 && (currtime > g_TransmitControl.LastCommTime+GPRS_BUSINESS_WAIT_TIME)) { // 一定时间无收发数据，断开连接
					if (g_DeliverCount == g_SubmitCount) {
						printf("####%s(%d)####:time is over and to disconnect server case1\n",__func__,__LINE__);
						break;
					} else if (currtime > g_TransmitControl.LastCommTime + GPRS_BUSINESS_WAIT_TIME + GPRS_BUSINESS_DELIVER_WAIT_TIME) {   // 收发数据不对等, 则多延时一些时间
						printf("####%s(%d)####:time is over and to disconnect server case2\n",__func__,__LINE__);
						break;
					}
				}
			}
			/* 登陆成功后，业务数据发送和接收处理循环体,代码如上*/
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (g_LogControl.LogSate == LOG_STATE_HAVE_LONGIN) {
				// 登出和断开
				Log("TMC", LOG_LV_DEBUG, "Logouting.");
				TransmitPacket(CMD_LOGOUT_REQUEST, TRSMT_STATE_OK, g_Sequence++, NULL, 1, 0, NULL, 0);
			} else {
				Log("TMC", LOG_LV_DEBUG, "Connection is broken.");
				g_TransmitControl.bRequestConnect = 1;
			}
		}
		/* 代码如上*/
		/* 2.有业务数据发送，则会置位联网标志位，则进行login和业务数据发送*/
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TEST_NEW_IP_PORT:
		/* 3.需要测试新的IP和端口, 并且SIM卡正常, 则开始测试新IP和端口 */
		if (g_bDoNewConnectTest) {
			printf("g_bDoNewConnectTest\r\n");

			// 做disConnect，而不是直接改状态
			TransmitPacket(CMD_LOGOUT_REQUEST, TRSMT_STATE_OK, g_Sequence++, NULL, 1, 0, NULL, 0);

			if (0 == TestNewServerIPandPort()) {	// if new Server IP and port is valid, send online notify again
//				g_SystermInfo.bImsiChangedRequestSend = 1;
				g_TransmitControl.bSendOnlineNotify = 0;    // 改到了新的服务器，这些信息需要重新上传一下
			}
			g_bDoNewConnectTest = 0;
		}

		/* 4.需要复位主控器*/
		if (g_bResetWMC) {
			SetDog(TASK_DOG_MANAGE);
			WatchDogFeed();
			sleep(3);
			Log("TMC", LOG_LV_WARN, "Dev is rebooting.");
			reboot();
		}

		SetDog(TASK_DOG_MANAGE);
		WatchDogFeed();
		usleep(20000);
	}

	return NULL;
}

void TimeSync(void)
{
	if (g_TransmitControl.TimeSync.LastWMCSyncTime+43200<BooTime() ||
			g_TransmitControl.TimeSync.LastWMCSyncTime>BooTime()+30) {
		RemoteSendTimeSynchronize();
		// 设置靠前一些，收到会置为当前时间
		g_TransmitControl.TimeSync.LastWMCSyncTime = BooTime()-43190;
	}
}

/* the funcion to test whether the new Server IP and Port is valid
*
*	name: TestNewServerIPandPort()
*	return: 0 - success, 1 - failed
*
*/
int TestNewServerIPandPort(void)
{
	ULONG tempIP;
	int count = 0;
	int loginfail = 0;
	USHORT tempPort;

	tempIP = g_BootPara.NetInfo.ip;
	tempPort = g_BootPara.NetInfo.port;
	g_BootPara.NetInfo.ip = g_tempServerInfo.ServerIP;
	g_BootPara.NetInfo.port = g_tempServerInfo.ServerPort;

	printf("start to  new IP and port...\n\r");
	PrintfTime();

	printf("[CMD_LOGIN_REQUEST]send to server:start connect new server\r\n");
	while (1) {
		int Ret = LcGetOnline(100)==1;	//	100 为默认门锁ID

		// 做disConnect，而不是直接改状态
		TransmitPacket(CMD_LOGOUT_REQUEST, TRSMT_STATE_OK, g_Sequence++, NULL, 1, 0, NULL, 0);

		if (TransmitPacket(CMD_LOGIN_REQUEST, (TRSMT_STATE_OK|(Ret << 8)|0x200), g_Sequence++, NULL, 0, 0, NULL, 0) == TRSMT_STATE_OK) {
			break;
		}

		count++;
		printf("**************************\r\n login failed count=%d\r\n***********************************\r\n", count);
		if (count > 2) {
			loginfail = 1;
			break;
		}
	}

	if (loginfail) {	// login failed, restore the server ip and port
		g_BootPara.NetInfo.ip= tempIP;
		g_BootPara.NetInfo.port = tempPort;

		printf("**************************\r\n login failed, restore server IP and port \r\n ***********************************\r\n");
		printf("g_BootPara.NetInfo.ip=%04lx, g_BootPara.NetInfo.port=%d\r\n", g_BootPara.NetInfo.ip, g_BootPara.NetInfo.port);
		PrintfTime();
	} else {	// login success
		ser_printf("test OK...\n\r");
		PrintfTime();
//		StoreBootPara();
		SetDog(TASK_DOG_MANAGE);
		WatchDogFeed();
		sleep(2);

//		g_GprsState.bDoDisconnecWorking = 1;
		ser_printf("[CMD_LOGOUT_REQUEST]send to server:start connect new server\r\n");
		TransmitPacket(CMD_LOGOUT_REQUEST, TRSMT_STATE_OK, g_Sequence++, NULL, 1, 0, NULL, 0);

		echo_printf("log out...\r\n");

		SetDog(TASK_DOG_MANAGE);
		WatchDogFeed();
		sleep(2);
	}

	return loginfail;
}
