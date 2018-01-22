#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include "../main_linuxapp.h"
#include "../aes.h"
#include "../LanMiddle.h"
#include "../mgt/mgt.h"
#include "../NormalTool.h"
#include "../BusinessProtocol.h"
#include "../ProtocolDefine.h"
#include "../ManageTransmit.h"
#include "../../include/common.h"
#include "../../include/log.h"
#include "rf.h"
#include "../up/up.h"
#include "../../include/gpio.h"
#include "../cfgFile.h"

#define DEF_RF_MODE_INTVL				600
#define DEF_RF_ADDR_INTVL				60
#define DEF_RF_SGNL_INTVL				150
#define LC_BUSINESS_PACK_HEAD_LENGTH_V2	10
#define LC_BUSINESS_PACK_HEAD_LENGTH_V3	5
#define LC_STAT_INTVL					180
#define LC_STAT_MAX_RETRY				10
#define LC_FWD_SEND_TIMEO				30
#define LC_FWD_TOO_OLD					600		// 10分钟之前的就不再重试了
#define LC_FWD_SEND_MAX					3

// �ϻ�����
//#define DBG_RMT_OPEN_AND_RCD

typedef struct McuCmdInfo sMcuCmdInfo;
struct McuCmdInfo {
	sMcuCmdInfo *Next;
	int			Type;
	union {
		sMcuCmdStd		Std;
		sMcuCmdSetDev	SetDev;
		sMcuCmdRdFwd	RdFwd;
	} Arg;
};

typedef struct HexData sHexData;
struct HexData {
	sHexData	*Next;
	uint8_t		*Data;
	uint32_t	DtLen;
};

typedef struct FwdCache {
	struct FwdCache	*Next;
	double			DeadT;		// 需要（重）发送的时间点
	uint8_t			SdCnt;		// 发送的次数
	sMcuCmdRdFwd	Fwd;
} sFwdCache;

typedef struct RfMod sRfMod;
struct RfMod {
	uint8_t				DevAddr;		//该模块的地址
	pthread_rwlock_t	LockMutex;		// 该mutex用来保护LockNum和Locks
	uint8_t				LockNum;
	sLockInfo			*Locks;

	ModInit				Init;
	ModRun				Run;
	ModRecvCb			RecvCb;
	ModCmd				Cmd;

	pthread_mutex_t	Mutex;			// 保护RecvHead pRecvTail CmdHead pCmdTail
	pthread_cond_t	Cond;			// 以上的条件通知变量
	sHexData		*RecvHead;
	sHexData		**pRecvTail;
	sMcuCmdInfo		*CmdHead;
	sMcuCmdInfo		**pCmdTail;
	sFwdCache		*pFwdCache;		// 仅当前线程使用，不需加锁控制
	double			NextStatT;		// 门锁下次状态更新时间，仅适用与1个门锁的操作，多个门锁需要重新设计
	uint8_t			StatToCnt;		// 门锁状态更新超时次数
	uint16_t		WarnTo;			// 蜂鸣器报警超时
};

typedef struct BeepMgt {
	double		to;					// 超时时间，不需要加锁保护了，设置都在同一个线程，读取线程，偶尔读错一次完全可以接受
} sBeepMgt;

static sBeepMgt BpInfo;
volatile int RstProc = 0;
	
static void BtnCb(int Type, int Pin, int32_t Arg)
{
	Log("RFLOCK", LOG_LV_DEBUG, "Remote Ctrl[%d], ProtVer=%X.", Type, ProtVer);

	if (Type==0 && LcGetOnline(Arg)==1) {
		uint8_t RmOpenFr[100] = {0};
		if (ProtVer/16 == 2) {
			uint8_t tFr[100] = {
				0x00, 0x18, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x01, 0x02, 0x00, 0x64, 0x0A, 0x01, 0x1A,
				0x78, 0x06, 0x2D, 0x01, 0x01, 0x2E, 0x01, 0x01
			};

			memcpy(RmOpenFr, tFr, tFr[0]*256+tFr[1]);
		} else {
			uint8_t tFr[100] = {
				0x00, 0x2B, 0x04, 0x20, 0xBA, 0xEC, 0x50, 0x05,
				0x16, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00,
				0x00, 0x00, 0x64, 0x52, 0x01, 0x00, 0x53, 0x13,
				0x00, 0x13, 0x02, 0x00, 0x50, 0x01, 0x1A, 0x01,
				0x04, 0x00, 0x00, 0x00, 0x64, 0x2D, 0x01, 0x01,
				0x2E, 0x01, 0x01
			};

			memcpy(RmOpenFr, tFr, tFr[0]*256+tFr[1]);
		}
		// 门锁在线时，IO触发远程开门
		LcCmdForward(Arg, 0, RmOpenFr, RmOpenFr[0]*256+RmOpenFr[1]);
	}
}

static void BeepThread(void)
{
#ifdef DBG_RMT_OPEN_AND_RCD
	double intvl = 0 - 60;
#endif
	while (1) {
		int IoV = 0;
		double st = BooTime();
#ifdef DBG_RMT_OPEN_AND_RCD
		if (intvl + 15 < BooTime()) {
			intvl = BooTime();
			BtnCb(0, 0, 100ul);
		}
#endif
		while (BooTime() < BpInfo.to) {
			if (BooTime() < st+20) {
				GpioSet(GPIO_BUZZ, IoV=!IoV);
			} else if (BooTime() < st+30) {
				GpioSet(GPIO_BUZZ, 0);
			} else {
				st = BooTime();
			}
			usleep(200000);
		}

		GpioSet(GPIO_BUZZ, 0);
		usleep(100000);
	}
}

static void RstThread(void)
{
	double PushLstT = 0;
	int oVal = 0, nVal = 0, i = 0; 
	while (1) {
		usleep(100000);
		nVal = GpioGet(GPIO_RST_BTN);
		if (!nVal) {
			if (oVal == 1) {
				// ��һ�ΰ���
 				PushLstT = BooTime();
			} else {
				if (PushLstT + 3 < BooTime()) {
					// ���³���3��
					RstProc = 1;
					for (i = 0; i < 30; i++) {
						LedsCtrl(LED_ONLINE, i%2);
						LedsCtrl(LED_RUN, i%2);
						LedsCtrl(LED_DOOR_LINK, i%2);
						usleep(100000);
					}
					system("/home/chgdefault.sh");
					Log("RFLOCK", LOG_LV_WARN, "Reset button push and chg default config.");
					reboot();
				}
			}
		} 
		oVal = nVal;
 	}
}

static int RfLcSend(sRfMod *pRf, uint8_t BusinessType, uint8_t cmd, uint8_t Status, uint32_t sequence,
					const uint8_t *bodydata, uint16_t len, const uint8_t *ReserveData, uint8_t rhl)
{
	uint16_t packlen;
	uint8_t *tempBuf = malloc(MAX_BUSINESS_SINGLE_PACK_LEN);

	// 1.门锁通信的发送给门锁的业务层报文包头组包
	if (ProtVer/16 == 2) {
		tempBuf[0] = BusinessType;
		packlen = len+9+rhl;
		ConvertMemcpy(tempBuf+1, &packlen, 2);
		packlen++;
		tempBuf[3] = cmd;
		tempBuf[4] = Status;
		ConvertMemcpy(tempBuf+5, &sequence, 4);
		tempBuf[9] = rhl;
		if (ReserveData && rhl) {
			memcpy(tempBuf+10, ReserveData, rhl);
		}
		if (bodydata && len) {
			memcpy(tempBuf+10+rhl, bodydata, len);
		}
	} else {
		tempBuf[0] = BusinessType;
		tempBuf[3] = 0x04;
		tempBuf[4] = 0x20;
		if ((cmd&0x80)) {
			tempBuf[7]	= TAG_M_response;
			tempBuf[8]	= 6;
			tempBuf[14]	= Status;
		} else {
			tempBuf[7]	= TAG_M_request;
			tempBuf[8]	= 5;
		}
		packlen		= tempBuf[8]+len+8;
		ConvertMemcpy(tempBuf+1, &packlen, 2);
		packlen++;
		// 因为锁那里判断为RCMD_DownloadFile，因此不加0x80
		if ((cmd&0x7F) == RCMD_DownloadFile) {
			tempBuf[9]	= cmd&0x7F;
		} else {
			tempBuf[9]	= cmd;
		}
		ConvertMemcpy(tempBuf+10, &sequence, 4);
		if (bodydata && len) {
			memcpy(tempBuf+packlen-len, bodydata, len);
		}
		uint16_t crc = CRC16(tempBuf+7, packlen-7);
		ConvertMemcpy(tempBuf+5, &crc, 2);
	}

	// 调试信息输出
	char *tStr = NULL;
	if (packlen>0 && (tStr=(char*)malloc(packlen*3+1))!=NULL) {
		Hex2Str(tStr, tempBuf, packlen, ' ');
	}
	Log("RFLOCK", LOG_LV_DEBUG, "Send l=%hu, Data:%s.", packlen, tStr);
	if (tStr) {
		free(tStr);
		tStr = NULL;
	}

	// 发送出去
	McuSendFrRetry(0x02, 0, pRf->DevAddr, 1, tempBuf, packlen, 0.5, 3);

	free(tempBuf);
	return 0;
}

static int GetLockDownloadData(uint32_t offset, uint32_t size, uchar *buf, void *crc)
{
	if (buf==NULL || crc==NULL) {
		return -1;
	}
	int Ret = -1;

	Ret =  UpgradeFileRead(FLAG_UPGRADE_LOCK, offset, buf, size);
	if (Ret >= 0) {
		// 大于0说明读的数据超出了文件大小，后边的数据随便补充吧
		if (ProtVer/16 == 2) {
			*((uint16_t *)crc) = CRC16(buf, size);
		} else {
			*((uint32_t *)crc) = CRC32(buf, size);
		}
	}

	return Ret;
}

static int RfSecurityLayerDecode(uint8_t *datain, int *lenin)
{
	if (datain==NULL || lenin==NULL || *lenin<=0) {
		return -1;
	}
	strAES_PROC_BODY str_aes_decode = {0};
	int ret = -1;
	uint8_t EnMode=0;

	// 加密方式
	EnMode = datain[0]&ENCODETYPE_BIT_MASK;
	switch (EnMode) {
	case AES_ENCODE:
		str_aes_decode.pData= datain+1;
		str_aes_decode.len	= *lenin-1;
		ret=PackAesEmulateDecode(&str_aes_decode);
		if (ret) {
			Log("RFLOCK", LOG_LV_DEBUG, "Faile[%d] to PackAesEmulateDecode.", ret);
			return ret;
		}
		*lenin = str_aes_decode.len + 1;
		break;
	// 以下两种 暂不支持
	case TEA_ENCODE:
	case DES_ENCODE:
		Log("RFLOCK", LOG_LV_DEBUG, "Unknown EnMode:%hhu.", EnMode);
		return -1;
		break;
	default:
	case NO_ENCODE:
		break;
	}
	return 0;
}

static int RfLcDecode(sRfMod *pRf, int DevCode, uint8_t *data, int len)
{
	if (data==NULL || len<min(LC_BUSINESS_PACK_HEAD_LENGTH_V2, LC_BUSINESS_PACK_HEAD_LENGTH_V3)) {
		Log("RFLOCK", LOG_LV_DEBUG, "LCBusinessDecode arg illegal:%d, %d", (int)data, len);
		return -1;
	}
	uint8_t cmd, cmdstatus;
	uint16_t businesslen;
	uint32_t seq;      // 门锁传上来的流水号(传给服务器需先修改掉流水号再上传)
	int TagTotalLen, Ret=-1;
	int DataTagOf = 0;
	int HaveProcessLen = {0};
	typTLV tlv = {0};

	// 2.安全层数据解密处理，得到业务层数据
	//解密的过程会校验CRC并去除CRC字段
	Ret = RfSecurityLayerDecode(data, &len);
	if (Ret || len<=0) {
		Log("RFLOCK", LOG_LV_DEBUG, "RfSecurityLayerDecode failed:r=%d, l=%d", Ret, len);
		return -1;
	}
	// 调试解密后数据输出
	char *tStr = NULL;
	if (len>0 && (tStr=(char*)malloc(len*3+1))!=NULL) {
		Hex2Str(tStr, data, len, ' ');
	}
	Log("RFLOCK", LOG_LV_DEBUG, "Recv l=%d, Data:%s.", len, tStr);
	if (tStr) {
		free(tStr);
		tStr = NULL;
	}

	ConvertMemcpy(&businesslen, data+1, 2);
	if (businesslen+1 > len) {
		Log("RFLOCK", LOG_LV_DEBUG, "businesslen too long:bl=%d, dl=%d", businesslen, len);
		return -1;
	}

	if (ProtVer/16 == 2) {
		cmd			= data[3];      // keyword
		cmdstatus	= data[4];      // 命令状态
		ConvertMemcpy(&seq, data+5, 4);
		DataTagOf	= LC_BUSINESS_PACK_HEAD_LENGTH_V2;
	} else {
		HaveProcessLen	= data[3]+3;
		if ((data[4]&0x07)) {
			// 加密方式
			Log("RFLOCK", LOG_LV_DEBUG, "Encode[%hhu] not supportted!", data[4]);
			return -1;
		}
		if ((data[4]&0x20)) {
			uint16_t crc =  CRC16(data+HaveProcessLen, businesslen+1-HaveProcessLen);
			if (crc != data[5]*256+data[6]) {
				Log("RFLOCK", LOG_LV_DEBUG, "Crc16[c=0x%04X] not match!", crc);
				return -1;
			}
		}
		tlv.NextTag		= data+HaveProcessLen;
		TLVDecode(&tlv, &HaveProcessLen);
		if (tlv.tag==TAG_M_request && tlv.vlen==5) {
			cmd = tlv.vp[0];
			ConvertMemcpy(&seq, tlv.vp+1, 4);
		} else if (tlv.tag == TAG_M_response && tlv.vlen==6) {
			cmd			= tlv.vp[0];
			cmdstatus	= tlv.vp[5];
			ConvertMemcpy(&seq, tlv.vp+1, 4);
		} else {
			return -1;
		}
		DataTagOf = HaveProcessLen;
	}
	TagTotalLen = businesslen+1-DataTagOf;
	if (TagTotalLen < 0) {
		return -1;
	}
	// 去除加密标识
	data[0] &= ~ENCODETYPE_BIT_MASK;
	Log("RFLOCK", LOG_LV_DEBUG, "Lc BT=%hhu!",data[0]);
	// 4.业务层报文中包体的具体处理
	switch (data[0]) {
	case BUSINESS_TYPE_LCM_RECORD: { //Upload Record
		int bCap = -1;		// 用来标识是否需要录像
		uint32_t Time = 0;	// 录像的时间
		int HaveProcessLen0 = 0;
		typTLV tlv0 = {.NextTag=data+DataTagOf};

		while (HaveProcessLen0 < TagTotalLen) {
			TLVDecode(&tlv0, &HaveProcessLen0);
			if (tlv0.tag == TAG_D_record) {
				int HaveProcessLen1 = 0;
				typTLV tlv1 = {.NextTag=tlv0.vp};

				while (HaveProcessLen1 < tlv0.vlen) {
					TLVDecode(&tlv1, &HaveProcessLen1);
					if (tlv1.tag==TAG_D_code && tlv1.vlen>=2 && tlv1.vlen<=4) {
//						memset(&DevCode, 0, sizeof(DevCode));
//						ConvertMemcpy(tlv1.vp, &DevCode, tlv1.vlen);	//Modify D_code to this lock (0x0064)
					} else if (tlv1.tag == TAG_D_encryptInfo) { // 加密类型
						Log("RFLOCK", LOG_LV_DEBUG, "TAG_D_encryptInfo not support:l=%hu, v=0x%X", tlv1.vlen, (tlv1.vp[0]<<16)|(tlv1.vp[1]<<8)|tlv1.vp[2]);
					} else if (tlv1.tag == TAG_M_secureHead) { // 加密类型
						Log("RFLOCK", LOG_LV_DEBUG, "TAG_M_secureHead info:l=%hu, v=0x%X", tlv1.vlen, tlv1.vp[0]);
						if ((tlv1.vp[0]&0x07)) {
							Log("RFLOCK", LOG_LV_DEBUG, "TAG_M_secureHead encode not supported.");
							return -1;
						}
					} else if ((ProtVer/16==2 && tlv1.tag==TAG_D_data) ||
							   (ProtVer/16!=2 && tlv1.tag==TAG_M_secureData)) { // 详细数据
						uint8_t RecType=0, RstC=0;
						uint8_t *p;
						int HaveProcessLen2 = 0;
						typTLV tlv2 = {.NextTag=tlv1.vp};

						while (HaveProcessLen2 < tlv1.vlen) {// D_data, 目前部支持加密
							TLVDecode(&tlv2, &HaveProcessLen2);
							if (tlv2.tag == TAG_L_openRecord3) { // 开门或巡更记录 (紧凑格式)
								p = tlv2.vp;

								RecType	= p[0];			// Record Type
								RstC 	= p[1];			// Result Code
								ConvertMemcpy(&Time, p+2, 4);

								if (0 == RstC) {// 0 代表成功
									if ((RecType & 0x7) <= 1) { // 001 - 开门记录
										if (0 == (RecType & 0x8)) {   // bit3=0, 软信号组合
											bCap = NORMAL_RECORD;
										} else {  // bit3=1, 其他自定义
											switch (RecType & 0xF0) {  // Bit4-bit7：
											case 0x60:  // 0110     强行闯入开门
												bCap = ALARM_RECORD;
												break;
											default:
											case 0x10:  // 0001     机械钥匙开门,
											case 0x20:  // 0010     出门按钮开门
											case 0x30:  // 0011     把手开门
											case 0x40:  // 0100     外部信号（对讲机）开门
											case 0x50:  // 0101     远程命令开门
												bCap = NORMAL_RECORD;
												break;
											}
										}
										// 其他记录不录像
									}
								}
							} else if (tlv2.tag == TAG_L_alertRecord3) { // 告警记录 (紧凑格式)
								p = tlv2.vp;

								RecType = p[0];		// Alert Type
								ConvertMemcpy(&Time, p+1, 4);

								switch (RecType) {
								case ALERT_TYPE_DESTORY:		// 防拆报警
								case ALERT_TYPE_FORCED_ENTER:	// 强行闯入报警
								case ALERT_TYPE_MANUAL: 		// 手动报警
								case ALERT_TYPE_SMOKE:			// 烟感报警
								case ALERT_TYPE_WINDOW:			// 窗磁报警
									bCap = ALARM_RECORD;
									break;
								default:
									break;
								}
							}
							// 其他不处理
						}
					}
				}
			}
		}

		if (seq == 0xBBBBBBBB) {
			// 此时为脱机记录，不需要录像
			bCap = -1;
		}
		MgtLockRecord(bCap, Time, data+DataTagOf, TagTotalLen);
	}
	break;

	case BUSINESS_TYPE_LOCK_FORWARD: {
		// 通过上层转发给Srv
		BusinessPackage(cmd|0x80, cmdstatus, seq, data+DataTagOf, TagTotalLen, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
		// 去除成功的FwdCache
		if (pRf->pFwdCache && pRf->pFwdCache->Fwd.DevCode==DevCode && pRf->pFwdCache->Fwd.Seq==seq) {
			sFwdCache *pFwdC = pRf->pFwdCache;

			pRf->pFwdCache = pFwdC->Next;
			free(pFwdC->Fwd.Data);
			free(pFwdC);
		}
	}
	break;

	case BUSINESS_TYPE_LC_LOCAL: {  // 门锁状态
		int CommFlag = 0;	// 0是已处理，其他是通用处理
		int DataOffset = 0;
		int DataTotalLen = 0;

		if (ProtVer/16 == 2) {
			switch (cmd&0x7F) {
			case LCCMD_QueryLockStatus:
			case LCCMD_LockStatusChange:
			case LCCMD_Download_File: {
				HaveProcessLen	= 0;
				tlv.NextTag		= data+DataTagOf;
				while (HaveProcessLen < TagTotalLen) {
					TLVDecode(&tlv, &HaveProcessLen);
					if (tlv.tag == TAG_D_data) {
						DataOffset	= tlv.vp-data;
						DataTotalLen= tlv.vlen;
						break;
					}
				}
			}
			break;
			case LCCMD_Read_Data: {
				uint32_t up_seq = seq>=0x80000000?(seq-0x80000000):seq;

				if (cmdstatus) {
					// 如果错误, 上传错误代码
					BusinessPackage(RCMD_ReadData|0x80, cmdstatus, up_seq, NULL, 0, NULL, 0, NULL, BUSINESS_TYPE_CM);
				} else { // 正确, 上传TLV数据
					BusinessPackage(RCMD_ReadData|0x80, BUSINESS_STATE_OK, up_seq, data+DataTagOf, TagTotalLen, NULL, 0, NULL, BUSINESS_TYPE_CM);
				}
			}
			break;

			default:
				CommFlag = 1;
				break;
			}
		} else {
			switch (cmd&0x7F) {
			case RCMD_QueryStatus:
			case RCMD_UploadStatus:
			case RCMD_DownloadFile:
				DataOffset	= DataTagOf;
				DataTotalLen= TagTotalLen;
				break;
			default:
				CommFlag = 1;
				break;
			}
		}
		if (DataTotalLen > 0) {
			if ((ProtVer/16==2 && ((cmd&0x7F)==LCCMD_QueryLockStatus || (cmd&0x7F)==LCCMD_LockStatusChange)) ||
					(ProtVer/16!=2 && ((cmd&0x7F)==RCMD_QueryStatus || (cmd&0x7F)==RCMD_UploadStatus))) {
				uint16_t StatChgFlag = 0;	// StatChgFlag用来标记哪些信息改变了，从而主动上传给Srv
				int HaveProcessLen0 = 0;
				typTLV tlv0 = {.NextTag=data+DataOffset};

				while (HaveProcessLen0 < DataTotalLen) {
					TLVDecode(&tlv0, &HaveProcessLen0);
					if (tlv0.tag == TAG_D_statusInfo) {
						sLockInfo *pLock = NULL;
						int i=0;
						int TimeFlag = 0;	// 时间需要同步的标识
						int HaveProcessLen1 = 0;
						typTLV tlv1 = {.NextTag=tlv0.vp};

						pRf->NextStatT = BooTime()+LC_STAT_INTVL;
						pRf->StatToCnt = 0;
						if (pthread_rwlock_wrlock(&pRf->LockMutex)) {
							break;
						}
						for (i=0; i<pRf->LockNum; i++) {
							if(pRf->Locks[i].DevCode == DevCode) {
								pLock = pRf->Locks+i;
								break;
							}
						}
						if (pLock == NULL) {
							pthread_rwlock_unlock(&pRf->LockMutex);
							break;
						}

						while (HaveProcessLen1 < tlv0.vlen) {
							TLVDecode(&tlv1, &HaveProcessLen1);

							if (tlv1.tag==TAG_L_lockerFlags && tlv1.vlen==2) { //和门锁通信bit4有效,表示时间是否设置过
								uint16_t nStatus = 0;			// 门锁当前状态

								ConvertMemcpy(&nStatus, tlv1.vp, 2);
								if ((pLock->StatMask&LC_SM_STAT)==0 || memcmp(&(pLock->Status), &nStatus, 2)!=0) {
									StatChgFlag 	|= LC_SM_STAT;		// 记录该状态改变
								}
								if ((nStatus&0x08) == 0) {	// 长开门报警取消
									BpInfo.to = -1;
								} else if (BpInfo.to < 0) {	// 如果未鸣叫过
									BpInfo.to = BooTime()+pRf->WarnTo;
								}
								// 有更新，先更新到本结构体中
								pLock->Status 	= nStatus;
								pLock->StatMask	|= LC_SM_STAT;
							} else if (tlv1.tag == TAG_L_battery) {		// 电池信息
								if (((pLock->StatMask&LC_SM_BAT) == 0) || pLock->BatStat!=tlv1.vp[0]) {
									StatChgFlag 	|= LC_SM_BAT;		// 记录该状态改变
								}
								// 有更新，先更新到本结构体中
								pLock->BatStat 	= tlv1.vp[0];
								pLock->StatMask|= LC_SM_BAT;
							} else if (tlv1.tag == TAG_L_temperature) {
								uint16_t nTemp = 0;

								ConvertMemcpy(&nTemp, tlv1.vp, 2);
								if ((pLock->StatMask&LC_SM_TEMP)==0 || pLock->Temp!=nTemp) {
									StatChgFlag |= LC_SM_TEMP;		// 记录该状态改变
								}
								// 有更新，先更新到本结构体中
								pLock->Temp		= nTemp;
								pLock->StatMask|= LC_SM_TEMP;

							} else if(tlv1.tag == TAG_M_time) {
								uint32_t LcTime = 0;
								time_t nTime = time(NULL);

								ConvertMemcpy(&LcTime, tlv1.vp, 4);
								// 时间与主控器（前提主控器时间正确）对比，相差超限时同步时间
								if ((LcTime < (uint32_t)nTime-30) || (LcTime > (uint32_t)nTime+30)) {
									TimeFlag = 1;
								}

							} else if (tlv1.tag == TAG_M_version) {
								if ((pLock->StatMask&LC_SM_SOFTV)==0 || memcmp(&(pLock->SoftVer), tlv1.vp, 2)!=0) {
									StatChgFlag |= LC_SM_SOFTV;		// 记录该状态改变
								}
								memcpy(&(pLock->SoftVer), tlv1.vp, 2);
								pLock->StatMask|= LC_SM_SOFTV;
							} else if (tlv1.tag == TAG_D_firmware) {
								if ((pLock->StatMask&LC_SM_HARDV)==0 || memcmp(&(pLock->HardVer), tlv1.vp, 2)!=0) {
									StatChgFlag |= LC_SM_HARDV;		// 记录该状态改变
								}
								memcpy(&(pLock->HardVer), tlv1.vp, 2);
								pLock->StatMask|= LC_SM_HARDV;
							} else if (tlv1.tag == TAG_L_popedomCapacity) {
								uint16_t nPermsCap = 0;

								ConvertMemcpy(&nPermsCap, tlv1.vp, 2);
								if ((pLock->StatMask&LC_SM_PCAP)==0 || pLock->PermsCap!=nPermsCap) {
									StatChgFlag |= LC_SM_PCAP;		// 记录该状态改变
								}
								pLock->PermsCap	= nPermsCap;
								pLock->StatMask|= LC_SM_PCAP;
							} else if (tlv1.tag == TAG_L_popedomCount) {
								uint16_t nPermsCnt = 0;

								ConvertMemcpy(&nPermsCnt, tlv1.vp, 2);
								if ((pLock->StatMask&LC_SM_PCNT)==0 || pLock->PermsCnt!=nPermsCnt) {
									StatChgFlag |= LC_SM_PCNT;		// 记录该状态改变
								}
								pLock->PermsCnt	= nPermsCnt;
								pLock->StatMask|= LC_SM_PCNT;
							} else if (tlv1.tag == TAG_L_fingerCapacity) {
								uint16_t nFgPermsCap = 0;

								ConvertMemcpy(&nFgPermsCap, tlv1.vp, 2);
								if ((pLock->StatMask&LC_SM_FGPCAP)==0 || pLock->FgPermCap!=nFgPermsCap) {
									StatChgFlag |= LC_SM_FGPCAP;		// 记录该状态改变
								}
								pLock->FgPermCap= nFgPermsCap;
								pLock->StatMask|= LC_SM_FGPCAP;
							} else if (tlv1.tag == TAG_L_fingerCount) {
								uint16_t nFgPermsCnt = 0;

								ConvertMemcpy(&nFgPermsCnt, tlv1.vp, 2);
								if ((pLock->StatMask&LC_SM_FGPCNT)==0 || pLock->FgPermCnt!=nFgPermsCnt) {
									StatChgFlag |= LC_SM_FGPCNT;		// 记录该状态改变
								}
								pLock->FgPermCnt= nFgPermsCnt;
								pLock->StatMask|= LC_SM_FGPCNT;
							}
						}

						// 更新采集时间
						time_t nTime = time(NULL);

						pLock->CollTime = nTime;

						pthread_rwlock_unlock(&pRf->LockMutex);

						// 需要校时且系统时间正确
						if (TimeFlag && nTime > 1420041600) {
							uint8_t tSendBuf[8] = {0};
							int tDataLen = 0;

							if (ProtVer/16 == 2) {
								tSendBuf[0]	= TAG_D_data;
								tSendBuf[1]	= 6;
								tDataLen	= 2;
							}
							tDataLen+=TLVEncode(TAG_M_time, 4, &nTime, 1, tSendBuf+tDataLen);

							RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, ProtVer/16==2?LCCMD_TimeSynchronize:RCMD_TimeSynchronize, BUSINESS_STATE_OK, 1, tSendBuf, tDataLen, NULL, 0);
						}
					}
				}
				// 门锁主动上传的，需要给门锁回复
				if (ProtVer/16==2 && (cmd&0x7F)!=LCCMD_QueryLockStatus) {	// query lock status response
					uint8_t tBuf[30]= {0}, tLen=0;

					tLen = TLVEncode(TAG_D_resultCode, 1, &tLen, 0, tBuf);
					RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, cmd|0x80, BUSINESS_STATE_OK, seq, tBuf, tLen, NULL, 0);
				} else if (ProtVer/16!=2 && (cmd&0x7F)!=RCMD_QueryStatus) {
					RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, cmd|0x80, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0);
				}
				// 如果有变化，立即上传
				if (StatChgFlag) {
//				if ((StatChgFlag&LC_SM_SGNL)) {
//					// 在此处发生信号改变，肯定时从掉线变为了上线
//					SendOnLineNotify(1);
//				}
					LcSendStatus(DevCode, 0, StatChgFlag);
				}
			} else if ((ProtVer/16==2 && (cmd&0x7F)==LCCMD_Download_File) ||
					   (ProtVer/16!=2 && (cmd&0x7F)==RCMD_DownloadFile)) {
				uint32_t offset = ~0;
				uint32_t size = ~0;
				int ltp=0, tp=0;
				uint16_t crc = 0;
				uint32_t Crc32 = 0;
				uint8_t tempbuf1[1100] = {0}, tempbuf2[1100] = {0};

				int HaveProcessLen0 = 0;
				typTLV tlv0 = {.NextTag=data+DataOffset};

				while (HaveProcessLen0 < DataTotalLen) {
					TLVDecode(&tlv0, &HaveProcessLen0);

					if (tlv0.tag == TAG_M_offset) {
						ConvertMemcpy(&offset, tlv0.vp, 4);
					} else if (tlv0.tag == TAG_M_size) {
						ConvertMemcpy(&size, tlv0.vp, 4);
//						} else if (tlv2.tag == TAG_M_file) {	// 暂时没有特殊作用，暂时不用
//							strncpy(filename, (char *)tlv2.vp, tlv2.vlen);
					}
				}
				if (size<=0 || size>1024 || offset>MAX_LOCK_UPGRADE_FILE_SIZE) {
					Log("RFLOCK", LOG_LV_DEBUG, "LOCK Download_File Args arg illegal:o=%llu, l=%llu", offset, size);
					break;
				}
				if (ProtVer/16 == 2) {
					GetLockDownloadData(offset, size, tempbuf1, &crc);
				} else {
					GetLockDownloadData(offset, size, tempbuf1, &Crc32);
				}

				ltp = 0;
				ltp +=TLVEncode(TAG_M_offset, 4, &offset, 1, tempbuf2+ltp);
				ltp +=TLVEncode(TAG_M_content, size, tempbuf1, 0, tempbuf2+ltp);
				if (ProtVer/16 == 2) {
					ltp +=TLVEncode(TAG_M_crc16, 2, &crc, 1, tempbuf2+ltp);
				} else {
					ltp +=TLVEncode(TAG_M_crc32, 4, &Crc32, 1, tempbuf2+ltp);
				}

				if (ProtVer/16 == 2) {
					// lock response
					tp = 0;
					tp += TLVEncode(TAG_D_resultCode, 1, &tp, 0, tempbuf1+tp);     //big tag tp =3
					tp += TLVEncode(TAG_D_data, ltp, tempbuf2, 0, tempbuf1+tp);

					RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, cmd|0x80, BUSINESS_STATE_OK, seq, tempbuf1, tp, NULL, 0);
				} else {
					RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, cmd|0x80, BUSINESS_STATE_OK, seq, tempbuf2, ltp, NULL, 0);
				}
			}
		} else if (CommFlag) {
			// 回复指令不需要做
		}
	}
	break;

	default:
		break;
	}
	return 0;
}

int RfInit(sMcuMod **ppMod)
{
	if (ppMod==NULL || *ppMod==NULL) {
		return -1;
	}
	sRfMod *pRf = NULL;

	BpInfo.to = -1;
	pRf = realloc(*ppMod, sizeof(sRfMod));
	if (pRf == NULL) {
		return -1;
	}
	pRf->RecvHead	= NULL;
	pRf->pRecvTail	= &(pRf->RecvHead);
	pRf->CmdHead	= NULL;
	pRf->pCmdTail	= &(pRf->CmdHead);
	pRf->pFwdCache	= NULL;
	pRf->NextStatT	= -1;
	pRf->StatToCnt	= 0;

	char *p = GetCfgStr(CFG_FTYPE_CFG, "Lock", "WarnTimeOut", "600");
	pRf->WarnTo	= strtol(p, NULL, 10);
	if (p) {
		free(p);
		p = NULL;
	}

	// 认为肯定会成功
	pthread_mutex_init(&pRf->Mutex, NULL);
	pthread_cond_init(&pRf->Cond, NULL);

	*ppMod = (sMcuMod *)pRf;

	return 0;
}

// 供别的线程调用
int RfCmd(sMcuMod *pMod, int Type, void *Arg)
{
	if (pMod==NULL || Arg==NULL) {
		return -1;
	}
	sRfMod *pRf = (sRfMod *)pMod;
	sMcuCmdInfo *pCmd = calloc(1, sizeof(sMcuCmdInfo));

	if (pCmd == NULL) {
		return -1;
	}

	// TODO DevCode匹配判断

	pCmd->Type = Type;
	switch(Type) {
	case MCU_CMD_SETTIME:
	case MCU_CMD_RESET:
	case MCU_CMD_UPGRADE: {
		sMcuCmdStd *pStd= Arg;
		pCmd->Arg.Std	= *pStd;
		break;
	}
	case MCU_CMD_SETDEV: {
		sMcuCmdSetDev *pSet	= Arg;
		pCmd->Arg.SetDev	= *pSet;
		break;
	}
	case MCU_CMD_READ:
	case MCU_CMD_FORWARD: {
		sMcuCmdRdFwd *pRdFwd= Arg;
		pCmd->Arg.RdFwd		= *pRdFwd;
		pCmd->Arg.RdFwd.Data= malloc(pRdFwd->Len);
		memcpy(pCmd->Arg.RdFwd.Data, pRdFwd->Data, pRdFwd->Len);
		break;
	}
	default:
		free(pCmd);
		return -1;
		break;
	}

	if (pthread_mutex_lock(&pRf->Mutex)) {
		if (Type==MCU_CMD_READ || Type==MCU_CMD_FORWARD) {
			free(pCmd->Arg.RdFwd.Data);
		}
		free(pCmd);
		return -1;
	}
	*(pRf->pCmdTail)	= pCmd;
	pRf->pCmdTail		= &(pCmd->Next);
	pthread_cond_signal(&pRf->Cond);
	pthread_mutex_unlock(&pRf->Mutex);

	return 0;
}

int RfRecvCb(sMcuMod *pMod, const uint8_t *Data, int DtLen)
{
	if (pMod==NULL || Data==NULL || DtLen<=0) {
		return -1;
	}
	sRfMod *pRf = (sRfMod *)pMod;
	sHexData *pHexD = NULL;

	pHexD = calloc(1, sizeof(sHexData));
	if (pHexD == NULL) {
		return -1;
	}
	pHexD->Next = NULL;
	pHexD->Data = malloc(DtLen);
	if (pHexD->Data == NULL) {
		free(pHexD);
		return -1;
	}
	memcpy(pHexD->Data, Data, DtLen);
	pHexD->DtLen = DtLen;

	if (pthread_mutex_lock(&pRf->Mutex)) {
		free(pHexD->Data);
		free(pHexD);
		return -1;
	}
	*(pRf->pRecvTail)	= pHexD;
	pRf->pRecvTail		= &(pHexD->Next);
	pthread_cond_signal(&pRf->Cond);
	pthread_mutex_unlock(&pRf->Mutex);

	return 0;
}

void* RfRun(void *pMod)
{
	if (pMod == NULL) {
		return NULL;
	}
	sRfMod *pRf = (sRfMod *)pMod;
	int Ret = -1;
	double LastModeT = 0-DEF_RF_MODE_INTVL;		// 用来记录工作模式查询时间
	double LastAddrT = 0-DEF_RF_ADDR_INTVL;		// 用来记录门锁地址查询时间
	double LastSgnlT = 0-DEF_RF_SGNL_INTVL;		// 用来记录信号强度查询时间
	double ReDelay = 1;							// 等待的超时
	sMcuCmdInfo *pOldCmd = NULL;				// 用来记录当前处理的任务
	pthread_t tid, tid1;

	pthread_create(&tid, NULL, (void*(*)(void*))BeepThread, NULL);
	pthread_create(&tid1, NULL, (void*(*)(void*))RstThread, NULL);
	// 置为正常模式
	Ret = 0;
	McuSendFrRetry(0x02, 0, pRf->DevAddr, 4, &Ret, 1, 0, 0);
	GpioRegIrq(GPIO_LOCK_RCTL, 0.1, (GpioIrqCb)BtnCb, (void*)100);

	while (1) {
		// 如果时间到，需要查询门锁地址
		if (LastAddrT+DEF_RF_ADDR_INTVL < BooTime()) {
			double tt = LastAddrT;
			// 发送门锁地址查询包
			McuSendFrRetry(0x01, 0, pRf->DevAddr, 2, NULL, 0, 0, 0);
			LastAddrT = BooTime()-DEF_RF_ADDR_INTVL+1;	// 如此设置可以保证1秒内不会多次发送该查询包
			Log("RF", LOG_LV_DEBUG, "l=%G, b=%G, n=%G", tt, BooTime(), LastAddrT);
		}

		// 如果时间到，需要查询信号强度
		if (LastSgnlT+DEF_RF_SGNL_INTVL < BooTime()) {
			double tt = LastSgnlT;
			// 发送信号强度查询包
			McuSendFrRetry(0x01, 0, pRf->DevAddr, 3, NULL, 0, 0, 0);
			LastSgnlT = BooTime()-DEF_RF_SGNL_INTVL+1;	// 如此设置可以保证1秒内不会多次发送该查询包
			Log("RF", LOG_LV_DEBUG, "l=%G, b=%G, n=%G", tt, BooTime(), LastSgnlT);
		}

		// 如果时间到，需要查询工作模式
		if (LastModeT+DEF_RF_MODE_INTVL < BooTime()) {
			double tt = LastModeT;
			// 发送工作模式查询包
			McuSendFrRetry(0x01, 0, pRf->DevAddr, 4, NULL, 0, 0, 0);
			LastModeT = BooTime()-DEF_RF_MODE_INTVL+1;	// 如此设置可以保证1秒内不会多次发送该查询包
			Log("RF", LOG_LV_DEBUG, "l=%G, b=%G, n=%G", tt, BooTime(), LastModeT);
		}

		// 取出命令或门锁包
		if (pthread_mutex_lock(&pRf->Mutex)) {
			continue;
		}
		double st = BooTime();
		sHexData	*pRecv= NULL;
		sMcuCmdInfo *pCmd = NULL;

		while (1) {
			if (pOldCmd==NULL && pRf->CmdHead) {
				// 外部命令
				pCmd		= pRf->CmdHead;
				pRf->CmdHead= pCmd->Next;
				if (pCmd->Next == NULL) {
					pRf->pCmdTail = &(pRf->CmdHead);
				}
			}
			if (pRf->RecvHead) {
				// 门锁数据包
				pRecv			= pRf->RecvHead;
				pRf->RecvHead	= pRecv->Next;
				pRecv->Next		= NULL;
				if (pRecv->Next == NULL) {
					pRf->pRecvTail = &(pRf->RecvHead);
				}
			}
			double Intvl = ClockTime(CLOCK_REALTIME)+st+ReDelay-BooTime();
			if (pCmd || pRecv || Intvl<ClockTime(CLOCK_REALTIME)) {
				break;
			} else {
				struct timespec to = {0};

				to.tv_sec	= Intvl;
				to.tv_nsec	= (Intvl-to.tv_sec)*1000000000LL;

				pthread_cond_timedwait(&(pRf->Cond), &(pRf->Mutex), &to);
			}
		}

		pthread_mutex_unlock(&pRf->Mutex);

		// 门锁数据包解析处理
		if (pRecv) {
			sMcuFrmHd *pHead = (sMcuFrmHd*)pRecv->Data;
			char *tStr=NULL;

			if ((tStr = malloc(pRecv->DtLen*3+1))) {
				Hex2Str(tStr, pRecv->Data, pRecv->DtLen, ' ');
			}
			Log("RF", LOG_LV_DEBUG, "RF Recv[%dB]:%s.", pRecv->DtLen, tStr);
			if (tStr) {
				free(tStr);
				tStr=NULL;
			}
			switch (pHead->Tag) {
			case 0:
				if (pHead->Func == 0x03) {
					// 先回复一下，标识收到
					Ret = 0;
					McuSendFr(0x83, pRf->DevAddr, 0, pHead->Tag, &Ret, 1);
					uint32_t LcDevCode = 0;

					if (pthread_rwlock_rdlock(&pRf->LockMutex)) {
						break;
					}
					if (pRf->LockNum == 1) {
						LcDevCode = pRf->Locks[0].DevCode;
					}
					pthread_rwlock_unlock(&pRf->LockMutex);
					if (LcDevCode) {
						// TODO 校验完整包
						// 按门锁协议解包
						RfLcDecode(pRf, LcDevCode, pRecv->Data+6, pHead->DtLen);
					}
				}
				break;
			case 2:
				if (pHead->Func == 0x81 && pHead->DtLen==6) {
					uint64_t nLockAddr = 0, oLockAddr = 0;

					if (pthread_rwlock_rdlock(&pRf->LockMutex)) {
						break;
					}
					if (pRf->LockNum == 1) {
						oLockAddr = pRf->Locks[0].Addr;
					}
					pthread_rwlock_unlock(&pRf->LockMutex);
					memcpy(&nLockAddr, pRecv->Data+7, 5);

					if (nLockAddr != (oLockAddr&0xFFFFFFFFFF)) {
						// 有门锁且地址不同设置门锁地址
						McuSendFrRetry(0x02, 0, pRf->DevAddr, 2, &oLockAddr, 5, 0, 0);
						LastAddrT = BooTime()-DEF_RF_ADDR_INTVL+1;
					} else {
						LastAddrT = BooTime();
					}
				}
				break;
			case 3:
				// 这里Ret临时被作为信号强度的偏移用
				Ret = -1;
				if (pHead->Func == 0x03) {
					if (pHead->DtLen == 1) {
						Ret = 0;
					} else {
						Ret = -1;
					}
					McuSendFr(0x83, pRf->DevAddr, 0, pHead->Tag, &Ret, 1);
					if (Ret == 0) {
						Ret = 6;
					} else {
						Ret = -1;
					}
				} else if (pHead->Func==0x81 && pHead->DtLen==2) {
					Ret = 7;
				}
				if (Ret > 0) {
					uint8_t SgnlChged = 0;
					uint32_t LcDevCode=0;

					if (pthread_rwlock_wrlock(&pRf->LockMutex)) {
						break;
					}
					if (pRf->LockNum == 1) {
						LcDevCode = pRf->Locks[0].DevCode;
						if (pRecv->Data[Ret]) {
							LedsCtrl(LED_DOOR_LINK, LED_OPEN_OPR);
							if ((pRf->Locks[0].StatMask&LC_SM_SGNL)==0 ||
									pRf->Locks[0].Signal!=(100-pRecv->Data[Ret]*50)) {
								// 信号无效或不相等
								SgnlChged = 1;
							}
							if (pRf->Locks[0].Online == 0) {
								SgnlChged = 2;	// 标记从不上线到上线
							}
							pRf->Locks[0].Signal 	= 100-pRecv->Data[Ret]*50;
							pRf->Locks[0].StatMask |= LC_SM_SGNL;
							pRf->Locks[0].Online	= 1;
						} else {
							if ((pRf->Locks[0].StatMask&LC_SM_SGNL)) {
								// 信号有效
								SgnlChged = 1;
							}
							LedsCtrl(LED_DOOR_LINK, LED_CLOSE_OPR);
							pRf->Locks[0].Online	= 0;
							pRf->Locks[0].StatMask	= 0;
						}
						pRf->Locks[0].CollTime	= time(NULL);
					}
					pthread_rwlock_unlock(&pRf->LockMutex);
					// 上线下线通知
					if (SgnlChged) {
						// 如果是从掉线到上线，先发上线通知
						if (SgnlChged == 2) {
							SendOnLineNotify(1);
						}
						// 信号强度有变化，则主动上传
						LcSendStatus(LcDevCode, 0, LC_SM_SGNL);
					}
					LastSgnlT = BooTime();
				}
				break;
			case 4:
				if (pHead->Func==0x81 && pHead->DtLen==2) {
					if (pRecv->Data[7] == 0) {
						LastModeT = BooTime();
					} else {
						Ret = 0;
						McuSendFrRetry(0x02, 0, pRf->DevAddr, pHead->Tag, &Ret, 1, 0, 0);
						LastModeT = BooTime()-DEF_RF_MODE_INTVL+1;
					}
				}
				break;
			default:
				break;
			}
			free(pRecv->Data);
			free(pRecv);
			pRecv = NULL;
		}

		// 外部指令解析处理
		if (pCmd) {
			// TODO 主动发给门锁的指令序号
			Log("RF", LOG_LV_DEBUG, "CmdType=%d", pCmd->Type);
			switch (pCmd->Type) {
			case MCU_CMD_SETTIME: {
				time_t nTime = time(NULL);
				int DataLen = 0;
				uint8_t tSendBuf[8] = {0};

				if (nTime < 1420041600) {
					Log("RF", LOG_LV_WARN, "Wrong Time");
					break;
				}
				if (ProtVer/16 == 2) {
					tSendBuf[0]	= TAG_D_data;
					tSendBuf[1]	= 6;
					DataLen		= 2;
				}
				DataLen+=TLVEncode(TAG_M_time, 4, &nTime, 1, tSendBuf+DataLen);

				RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, ProtVer/16==2?LCCMD_TimeSynchronize:RCMD_TimeSynchronize, BUSINESS_STATE_OK, 1, tSendBuf, DataLen, NULL, 0);
				break;
			}
			case MCU_CMD_RESET:
				RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, ProtVer/16==2?LCCMD_Reset_Notify:RCMD_ResetNotify, BUSINESS_STATE_OK, 2, NULL, 0, NULL, 0);
				break;
			case MCU_CMD_UPGRADE: {
				uint32_t FileSize = 0, CheckSum = 0x00;
				int ltp;
				char filename[20] = "pwlms.bin";
				uint8_t tSendBuf[100]= {0}, version[2]= {0};

				// TODO 暂时不判断成功与否了
				UpgradeFileRead(FLAG_UPGRADE_LOCK, 0, tSendBuf, CONTENT_OFFSET);
				memcpy(&CheckSum, tSendBuf+CHKSUM_OFFSET, 4);
				memcpy(&FileSize, tSendBuf+FILESIZE_OFFSET, 4);
				FileSize+=CONTENT_OFFSET;
				memcpy(version, tSendBuf+SOFT_VER_OFFSET, 2);

				if (ProtVer/16 == 2) {
					ltp = 2;
				} else {
					ltp = 0;
				}
				ltp += TLVEncode(TAG_D_code, DevCodeSize, &(pCmd->Arg.Std.DevCode), 1, tSendBuf+ltp);
				ltp += TLVEncode(TAG_M_file, strlen(filename)+1, filename, 0, tSendBuf+ltp);
				ltp += TLVEncode(TAG_M_version, 2, version, 0 , tSendBuf+ltp);
				ltp += TLVEncode(TAG_M_size, 4, &FileSize, 1 , tSendBuf+ltp);
				ltp += TLVEncode(TAG_WL_CheckSum, 4, &CheckSum, 1 , tSendBuf+ltp);

				if (ProtVer/16 == 2) {
					tSendBuf[0] = TAG_D_data;
					tSendBuf[1] = ltp-2;

					RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, LCCMD_Upgrade_Notify, BUSINESS_STATE_OK, 3, tSendBuf, ltp, NULL, 0);
				} else {
					RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, RCMD_UpgradeNotify, BUSINESS_STATE_OK, 3, tSendBuf, ltp, NULL, 0);
				}
				break;
			}
			case MCU_CMD_SETDEV: {
				if (pthread_rwlock_wrlock(&(pRf->LockMutex))) {
					Log("RF", LOG_LV_DEBUG, "Failed to wrlock MCU_CMD_SETDEV, errno=%d", errno);
					break;
				}
				Log("RF", LOG_LV_DEBUG, "SetDev.Type=%hhu", pCmd->Arg.SetDev.Type);
				Ret = -1;	// 用来标识是需要后续操作
				switch (pCmd->Arg.SetDev.Type) {
				case 0:
					// 增或改
					if (pRf->LockNum == 1) {
						if (pRf->Locks[0].DevCode == pCmd->Arg.SetDev.DevCode) {
							if (pRf->Locks[0].Addr != pCmd->Arg.SetDev.Addr) {
								memset(pRf->Locks, 0, sizeof(sLockInfo));
								pRf->Locks[0].Addr = pCmd->Arg.SetDev.Addr;
								pRf->Locks[0].DevCode = pCmd->Arg.SetDev.DevCode;
								Ret = 0;
							}
						} else {
							Log("RF", LOG_LV_DEBUG, "Unspport Add cmd[%], nDCODE=%d,nDCODE=%d", pRf->Locks[0].DevCode, pCmd->Arg.SetDev.DevCode);
						}
//					} else if (pRf->LockNum == 0) {
					} else {
						void *p = realloc(pRf->Locks, sizeof(sLockInfo));
						if (p == NULL) {
							Log("RF", LOG_LV_ERROR, "Failled to realloc pRf->Locks, errno=%d", errno);
						}
						pRf->Locks = p;
						memset(pRf->Locks, 0 ,sizeof(sLockInfo));
						pRf->Locks[0].Addr = pCmd->Arg.SetDev.Addr;
						pRf->Locks[0].DevCode = pCmd->Arg.SetDev.DevCode;
						pRf->LockNum	= 1;
						Ret = 0;
					}
					break;
				case 1:
				case 3:
					pRf->LockNum = 0;
					pRf->Locks=realloc(pRf->Locks, 0);
					Ret = 0;
					LedsCtrl(LED_DOOR_LINK, LED_CLOSE_OPR);
					break;
				default:
					break;
				}

				pthread_rwlock_unlock(&(pRf->LockMutex));
				if (Ret == 0) {
					LedsCtrl(LED_DOOR_LINK, LED_CLOSE_OPR);
					// 修改了门锁，缓冲的也没有必要继续发了
					while (pRf->pFwdCache) {
						sFwdCache *pC = pRf->pFwdCache;

						pRf->pFwdCache = pC->Next;
						free(pC->Fwd.Data);
						free(pC);
					}
					if (pCmd->Arg.SetDev.Type == 0) {
						McuSendFrRetry(0x02, 0, pRf->DevAddr, 2, &(pCmd->Arg.SetDev.Addr), 5, 0, 0);
						// 1秒内暂时先不查询信号强度
						if (LastSgnlT+DEF_RF_SGNL_INTVL-1 < BooTime()) {
							LastSgnlT = BooTime()-DEF_RF_SGNL_INTVL+2;
						}
						//LastSgnlT = 0-DEF_RF_SGNL_INTVL;
						// TODO 查询门锁状态，此时立刻查询也会无响应
					} else {
						// 删除
						pCmd->Arg.SetDev.Addr = 0;
						McuSendFrRetry(0x02, 0, pRf->DevAddr, 2, &(pCmd->Arg.SetDev.Addr), 5, 0, 0);
					}

				}

				break;
			}
			case MCU_CMD_READ:
				RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, ProtVer/16==2?LCCMD_Read_Data:RCMD_ReadData, BUSINESS_STATE_OK, pCmd->Arg.RdFwd.Seq, pCmd->Arg.RdFwd.Data, pCmd->Arg.RdFwd.Len, NULL, 0);
				free(pCmd->Arg.RdFwd.Data);
				break;
			case MCU_CMD_FORWARD: {
				// 添加到Fwd缓冲中
				sFwdCache **ppTail = &(pRf->pFwdCache);

				while (*ppTail) {
					ppTail = &((*ppTail)->Next);
				}
				ppTail[0]		= calloc(1, sizeof(sFwdCache));
				ppTail[0]->DeadT= BooTime();
				ppTail[0]->Fwd	= pCmd->Arg.RdFwd;
				ppTail[0]->Next	= NULL;
				ppTail[0]->SdCnt= 0;

				break;
			}
			default:
				break;
			}
			free(pCmd);
		}

		// Fwd缓冲包处理
		while (pRf->pFwdCache) {
			sFwdCache *pFwdC = pRf->pFwdCache;

			if (pFwdC->DeadT <= BooTime()) {
				if (pFwdC->SdCnt>=LC_FWD_SEND_MAX || pFwdC->DeadT+LC_FWD_TOO_OLD<BooTime()) {
					// 放弃重试
					pRf->pFwdCache = pFwdC->Next;
					free(pFwdC->Fwd.Data);
					free(pFwdC);
					continue;
				}
			}
			break;
		}
		if (pRf->pFwdCache) {
			sFwdCache *pFwdC = pRf->pFwdCache;

			if (pFwdC->DeadT <= BooTime()) {
				uint8_t *tSendBuf = malloc(pFwdC->Fwd.Len+1);

				tSendBuf[0] = BUSINESS_TYPE_LOCK_FORWARD;
				memcpy(tSendBuf+1, pFwdC->Fwd.Data, pFwdC->Fwd.Len);
				char *tStr = NULL;
				if (pFwdC->Fwd.Len>0 && (tStr=(char*)malloc(pFwdC->Fwd.Len*3+4))!=NULL) {
					Hex2Str(tStr, tSendBuf, pFwdC->Fwd.Len+1, ' ');
				}
				Log("RFLOCK", LOG_LV_DEBUG, "Send l=%hu, Forward[%hhu] Data:%s.", pFwdC->Fwd.Len+1, pFwdC->SdCnt, tStr);
				if (tStr) {
					free(tStr);
					tStr = NULL;
				}
				McuSendFrRetry(0x02, 0, pRf->DevAddr, 1, tSendBuf, pFwdC->Fwd.Len+1, pFwdC->Fwd.Len>300?1:0.5, 3);
				free(tSendBuf);
				pFwdC->SdCnt++;
				pFwdC->DeadT = BooTime()+LC_FWD_SEND_TIMEO;
			}
			ReDelay = max(0.001, min(1, pFwdC->DeadT-BooTime()));
		} else {
			ReDelay = 1;
		}

		// 状态需要更新
		if (pRf->NextStatT <= BooTime()) {
			Ret = -1;
			if (pthread_rwlock_wrlock(&(pRf->LockMutex))) {
				Log("RF", LOG_LV_DEBUG, "Failed to wrlock StatToCnt, errno=%d", errno);
				continue;
			}
			if (pRf->LockNum==1 && pRf->Locks) {
				Ret = 0;
				if (pRf->StatToCnt >= LC_STAT_MAX_RETRY) {
//					// 标识门锁脱机
//					if ((pRf->Locks[0].StatMask&LC_SM_SGNL)) {
//						// 信号有效
//						Ret = pRf->Locks[0].DevCode;
//					}
//					LedsCtrl(LED_DOOR_LINK, LED_CLOSE_OPR);
//					pRf->Locks[0].Online	= 0;
//					pRf->Locks[0].StatMask	= 0;

					pRf->StatToCnt = LC_STAT_MAX_RETRY;
				} else {
					pRf->StatToCnt++;
				}
			}
			pthread_rwlock_unlock(&(pRf->LockMutex));
			if (Ret >= 0) {
//				if (Ret) {
//					// 由联机变为脱机
//					LcSendStatus(Ret, 0, LC_SM_SGNL);
//				}
				// 存在锁才重试
				RfLcSend(pRf, BUSINESS_TYPE_LC_LOCAL, RCMD_QueryStatus, BUSINESS_STATE_OK, 4, NULL, 0, NULL, 0);
			}
			pRf->NextStatT = BooTime()+5;			// 5秒后再重试
		}
	}

	return NULL;
}
