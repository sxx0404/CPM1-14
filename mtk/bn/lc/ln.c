#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../LanMiddle.h"
#include "../mgt/mgt.h"
#include "../cfgFile.h"
#include "../NormalTool.h"
#include "../BusinessProtocol.h"
#include "../ProtocolDefine.h"
#include "../main_linuxapp.h"
#include "../ManageTransmit.h"
#include "../../include/gpio.h"
#include "../../include/common.h"
#include "../../include/log.h"
#include "ln.h"
#include "../up/up.h"

#define LN_CMD_IO			10	// Arg使用sMcuCmdStd，此时DevCode中指定IO号

#define PERMS_INFO_FILE		"/home/Perms.info"
#define RS485_TIMEOUT		0.1
#define RS485_MAX_RETRY		40
#define RS485_RDVER_INTVL	60

#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐
typedef struct PermInfo {
	uint8_t		Head;			// 0-3bit为权限类型:1权限;2巡更;4bit为有效项1有效，5-7bit为格式版本，本版为0
	uint8_t		Rcs;			// 校验码，本条权限(从Head到PermID结束)求和取反
	uint32_t	StartT;			// 权限有效起始时间，同time_t
	uint32_t	EndT;			// 权限有效结束时间，同time_t
	uint64_t	PermID;			// 卡号
} sPermInfo;
#pragma pack(pop)               //恢复原对齐方式

typedef struct McuCmdInfo {
	struct McuCmdInfo *Next;
	int			Type;
	union {
		sMcuCmdStd		Std;
		sMcuCmdSetDev	SetDev;
		sMcuCmdRdFwd	RdFwd;
	} Arg;
} sMcuCmdInfo;

typedef struct HexData {
	struct HexData	*Next;
	uint8_t			*Data;
	uint32_t		DtLen;
} sHexData;

typedef struct Ln485Task {
	uint8_t		Addr;			// 地址
	uint8_t		Type;			// 0读卡号，1重启，2读版本，3升级，优先级为1320
	uint8_t		ToCnt;			// 超时计数，本次任务的超时记录
	uint8_t		SigVal[10];		// 第1个字节为序号，后边的字节按位记录成功失败
	double		VerTime;		// 上次读取版本的时间
	int32_t		OffSet;			// 升级时使用，为下次发送的偏移地址，小于0发送起始，大于等于FlSize发送结束
	int32_t		FlSize;			// 升级时使用，为本次升级包的总大小
} sLn485Task;

typedef struct LnMod {
	uint8_t				DevAddr;		//该模块的地址
	pthread_rwlock_t	LockMutex;		// 该mutex用来保护LockNum和Locks
	uint8_t				LockNum;
	sLockInfo			*Locks;

	ModInit				Init;
	ModRun				Run;
	ModRecvCb			RecvCb;
	ModCmd				Cmd;

	pthread_mutex_t	Mutex;
	pthread_cond_t	Cond;
	sHexData		*RecvHead;
	sHexData		**pRecvTail;
	sMcuCmdInfo		*CmdHead;
	sMcuCmdInfo		**pCmdTail;

	double			CloseDelay;			// 开门后的关门延时，开门后经过这段时间就会关门
	uint32_t		PermsNum;
	sPermInfo		*Perms;
} sLnMod;

// pPctVal记录的成功率， Type为0不修改只计算，负数先记录失败，再计算；正数先记录成功，再计算，返回成功率百分比0-100
static uint8_t CalcPct(uint8_t *pPctVal, int Type)
{
	if(pPctVal == NULL) {
		return 0;
	}
	if (Type == 0) {
		int i=0, sNum=0;
		for (i=1; i<10; i++) {
			int j=0;
			for (j=0; j<8; j++) {
				if ((pPctVal[i]&(1<<j))) {
					sNum++;
				}
			}
		}
		return sNum;
	} else {
		pPctVal[0]++;
		if (pPctVal[0]>=72) {
			pPctVal[0] = 0;
		}
		int i=pPctVal[0]/8+1, j=pPctVal[0]%8;

		if (Type < 0) {
			pPctVal[i] &= ~(1<<j);
		} else {
			pPctVal[i] |= 1<<j;
		}

		return CalcPct(pPctVal, 0);
	}
}

static int CalcSignal(uint8_t *pPctVal1, uint8_t *pPctVal2)
{
	int a=0, b=0;

	a = CalcPct(pPctVal1, 0);
	b = CalcPct(pPctVal2, 0);
	if (a <= 0) {
		a = 0;
	} else if (a>72) {
		a = 4;
	} else {
		a = a/24+1;
	}
	if (b <= 0) {
		b = 0;
	} else if (b>72) {
		b = 4;
	} else {
		b = b/24+1;
	}

	return ((a+b)*10+a+16);
}

// Buf请保证30字节以上，放到BUS中去？
static int MkOpenRecd(uint8_t *Buf, int DevCode, int Type, int Ret, int32_t Time,
					  int LcStat, int AlFlg, int CardType, uint64_t CardID)
{
	if (Buf==NULL || DevCode<0) {
		return -1;
	}
	int Len = 2;
	uint32_t t = 0;
	uint8_t tRecd[30] = {0};

	if (ProtVer/16 == 2) {
		Buf[0]	= TAG_D_record;
		Buf[2]	= TAG_D_code;
		Buf[3]	= 2;
		ConvertMemcpy(Buf+4, &DevCode, 2);
		Buf[6]	= TAG_D_data;
		Buf[8]	= TAG_L_openRecord3;
		Buf[10]	= Type;
		Buf[11]	= Ret;
		ConvertMemcpy(Buf+12, &Time, 4);
		Buf[16]	= LcStat;
		Buf[17]	= AlFlg;
		if ((Type&0x08) == 0) {
			Buf[18]	= 0x03;
			Buf[19]	= CardType;
			Buf[20]	= 8;
			memcpy(Buf+21, &CardID, 8);
			Buf[9]	= 19;
		} else {
			Buf[9]	= 8;
		}
		Buf[7]	= Buf[9]+2;
		Buf[1]	= Buf[7]+6;

		return Buf[1]+2;
	} else {
		Buf[0]	= TAG_D_record;
		Len		+= TLVEncode(TAG_D_code, DevCodeSize, &DevCode, 1, Buf+Len);
		Len		+= TLVEncode(TAG_M_secureHead, 1, &t, 1, Buf+Len);
		tRecd[0]= TAG_L_openRecord3;
		tRecd[2]= Type;
		tRecd[3]= Ret;
		ConvertMemcpy(tRecd+4, &Time, 4);
		tRecd[8]= LcStat;
		tRecd[9]= AlFlg;
		if ((Type&0x08) == 0) {
			tRecd[10]	= 0x03;
			tRecd[11]	= CardType;
			tRecd[12]	= 8;
			memcpy(tRecd+13, &CardID, 8);
			tRecd[1]	= 19;
		} else {
			tRecd[1]	= 8;
		}
		Len		+= TLVEncode(TAG_M_secureData, tRecd[1]+2, tRecd, 0, Buf+Len);
		Buf[1]	= Len-2;

		return Len;
	}
}

// Buf请保证30字节以上
static int MkAlmRecd(uint8_t *Buf, int DevCode, int Type, int32_t Time)
{
	if (Buf==NULL || DevCode<0) {
		return -1;
	}
	Buf[0]	= TAG_D_record;
	Buf[2]	= TAG_D_code;
	Buf[3]	= 2;
	ConvertMemcpy(Buf+4, &DevCode, 2);
	Buf[6]	= TAG_D_data;
	Buf[8]	= TAG_L_alertRecord3;
	Buf[10]	= Type;
	ConvertMemcpy(Buf+11, &Time, 4);
	Buf[9]	= 5;
	Buf[7]	= Buf[9]+2;
	Buf[1]	= Buf[7]+6;

	return Buf[1]+2;
}

static void IoCallBack(int Type, int Pin, sLnMod *pLn)
{
	if (pLn==NULL) {
		return;
	}
	sMcuCmdInfo *pCmd = calloc(1, sizeof(sMcuCmdInfo));

	if (pCmd == NULL) {
		return;
	}

	pCmd->Type				= LN_CMD_IO;
	pCmd->Next				= NULL;
	switch (Pin) {
	case GPIO_LOCK_BTN:		// 门锁按钮
	case GPIO_LOCK_RCTL:	// 对讲等远程控制
		if (Type != 0) {
			// 只关注闭合事件
			free(pCmd);
			return;
		}
		pCmd->Arg.Std.DevCode	= Pin;
		break;
	case GPIO_LOCK_DTC1:	// 门磁检测1
	case GPIO_LOCK_DTC2:	// 门磁检测2
		if (Type) {
			// 断开，立刻产生指令
			pCmd->Arg.Std.DevCode	= GPIO_LOCK_DTC1+0x8000;
		} else if (GpioStat(Pin==GPIO_LOCK_DTC1?GPIO_LOCK_DTC2:GPIO_LOCK_DTC1) == 0) {
			// 2者都是闭合才算闭合
			pCmd->Arg.Std.DevCode	= GPIO_LOCK_DTC1;
		}
		break;
	default:
		free(pCmd);
		return;
	}

	if (pthread_mutex_lock(&pLn->Mutex)) {
		free(pCmd);
		return;
	}
	*(pLn->pCmdTail)	= pCmd;
	pLn->pCmdTail		= &(pCmd->Next);
	pthread_cond_signal(&pLn->Cond);
	pthread_mutex_unlock(&pLn->Mutex);
}

static int LnSavePerms(sLnMod *pLn)
{
	int i=0, Fd=-1, Ret=-1;
	sPermInfo tPerm = {0};

	Fd = open(PERMS_INFO_FILE, O_RDWR|O_CREAT, 0666);

	if (Fd < 0) {
		Log("LN", LOG_LV_ERROR, "Failed to open %s, errno=%d.", PERMS_INFO_FILE, errno);
		return -1;
	}
	for (i=0; i<pLn->PermsNum; i++) {
		pLn->Perms[i].Rcs = 0;
		pLn->Perms[i].Rcs = RvCheckSum(pLn->Perms+i, sizeof(sPermInfo));
		if (read(Fd, &tPerm, sizeof(sPermInfo))!=sizeof(sPermInfo) ||
				memcmp(&tPerm, pLn->Perms+i, sizeof(sPermInfo))!=0) {
			if (lseek(Fd, i*sizeof(sPermInfo), SEEK_SET) != i*sizeof(sPermInfo)) {
				Log("LN", LOG_LV_ERROR, "Failed to lseek %s to %d, errno=%d.", PERMS_INFO_FILE, i*sizeof(sPermInfo), errno);
				break;
			}
			if (write(Fd, pLn->Perms+i, sizeof(sPermInfo)) != sizeof(sPermInfo)) {
				Log("LN", LOG_LV_ERROR, "Failed to write to %s, errno=%d.", PERMS_INFO_FILE, errno);
				break;
			}
		}
		// 移到下一个读取的位置
		if (lseek(Fd, (i+1)*sizeof(sPermInfo), SEEK_SET) != (i+1)*sizeof(sPermInfo)) {
			Log("LN", LOG_LV_ERROR, "Failed to lseek %s to %d, errno=%d.", PERMS_INFO_FILE, (i+1)*sizeof(sPermInfo), errno);
			break;
		}
	}
	if (i >= pLn->PermsNum) {
		// 说明存储完了，文件大小设置一下
		Ret = ftruncate(Fd, pLn->PermsNum*sizeof(sPermInfo));
	}
	close(Fd);

	return Ret;
}

int LnInit(sMcuMod **ppMod)
{
	if (ppMod==NULL || *ppMod==NULL) {
		return -1;
	}
	int Fd=-1;
	sLnMod *pLn = NULL;

	pLn = realloc(*ppMod, sizeof(sLnMod));
	if (pLn == NULL) {
		return -1;
	}
	pLn->RecvHead	= NULL;
	pLn->pRecvTail	= &(pLn->RecvHead);
	pLn->CmdHead	= NULL;
	pLn->pCmdTail	= &(pLn->CmdHead);

	// 认为肯定会成功
	pthread_mutex_init(&pLn->Mutex, NULL);
	pthread_cond_init(&pLn->Cond, NULL);
	// 开门后的关门延时
	char *p = GetCfgStr(CFG_FTYPE_CFG, "Lock", "Delay", "5");
	pLn->CloseDelay	= strtod(p, NULL);
	if (p) {
		free(p);
		p = NULL;
	}
	if (pLn->CloseDelay<1 || pLn->CloseDelay>3600) {
		pLn->CloseDelay = 5;
	}
	// 权限数据初始化
	pLn->PermsNum	= 0;
	pLn->Perms		= NULL;

	Fd = open(PERMS_INFO_FILE, O_RDONLY);
	if (Fd >= 0) {
		int i = 0;
		sPermInfo tPerm = {0};

		while (read(Fd, &tPerm, sizeof(sPermInfo)) == sizeof(sPermInfo)) {
			if ((uint8_t)RvCheckSum(&tPerm, sizeof(sPermInfo)) == 0) {
				for (i=0; i<pLn->PermsNum; i++) {
					if (pLn->Perms[i].PermID == tPerm.PermID) {
						break;
					}
				}
				if (i >= pLn->PermsNum) {
					// 没有重复的，开始添加
					void *p = realloc(pLn->Perms, (pLn->PermsNum+1)*sizeof(sPermInfo));

					if (p == NULL) {
						Log("LN", LOG_LV_ERROR, "Failed to realloc pLn->Perms to %d, errno=%d.", (pLn->PermsNum+1)*sizeof(sPermInfo), errno);
						// 此时如果继续运行，则权限数据会丢失，已产生记录，最好的方法还是退出，以重启设备
						exit(EXIT_FAILURE);
					}
					pLn->Perms = p;
					pLn->Perms[pLn->PermsNum++] = tPerm;
				}
			}
			if (pLn->PermsNum >= UINT16_MAX) {
				break;
			}
		}

		close(Fd);
	}
	pLn->Locks[0].Online	= 1;
	pLn->Locks[0].PermsCnt	= pLn->PermsNum;
	pLn->Locks[0].PermsCap	= UINT16_MAX;
	pLn->Locks[0].CollTime	= time(NULL);
	pLn->Locks[0].StatMask	= LC_SM_PCAP;	// LC_SM_PCNT在初次运行时会设置
	int j=0;
	for (j=0; j<pLn->PermsNum; j++) {
		Log("LN", LOG_LV_DEBUG, "Perms[%d] ID=0x%08llX", j+1, pLn->Perms[j].PermID);
	}

	*ppMod = (sMcuMod *)pLn;
	return 0;
}

// 供别的线程调用
int LnCmd(sMcuMod *pMod, int Type, void *Arg)
{
	if (pMod==NULL || Arg==NULL) {
		return -1;
	}
	sLnMod *pLn = (sLnMod *)pMod;
	sMcuCmdInfo *pCmd = calloc(1, sizeof(sMcuCmdInfo));

	if (pCmd == NULL) {
		return -1;
	}

	pCmd->Type = Type;
	switch(Type) {
	case MCU_CMD_READ:
	case MCU_CMD_FORWARD: {
		sMcuCmdRdFwd *pRdFwd= Arg;

		pCmd->Arg.RdFwd		= *pRdFwd;
		pCmd->Arg.RdFwd.Data= malloc(pRdFwd->Len);
		memcpy(pCmd->Arg.RdFwd.Data, pRdFwd->Data, pRdFwd->Len);
		break;
	}
	case MCU_CMD_RESET:
	case MCU_CMD_UPGRADE: {
		sMcuCmdStd *pStd= Arg;

		pCmd->Arg.Std = *pStd;
		break;
	}
	// 有线模式不支持的指令
	case MCU_CMD_SETDEV:
		if (pCmd->Arg.SetDev.Type == 0) {
			// 通知门锁上线
			SendOnLineNotify(1);
		}
	case MCU_CMD_SETTIME:
	default:
		free(pCmd);
		return -1;
		break;
	}

	if (pthread_mutex_lock(&pLn->Mutex)) {
		if (Type==MCU_CMD_READ || Type==MCU_CMD_FORWARD) {
			free(pCmd->Arg.RdFwd.Data);
		}
		free(pCmd);
		return -1;
	}
	*(pLn->pCmdTail)	= pCmd;
	pLn->pCmdTail		= &(pCmd->Next);
	pthread_cond_signal(&pLn->Cond);
	pthread_mutex_unlock(&pLn->Mutex);

	return 0;
}

int LnRecvCb(sMcuMod *pMod, const uint8_t *Data, int DtLen)
{
	if (pMod==NULL || Data==NULL || DtLen<=0) {
		return -1;
	}
	sLnMod *pLn = (sLnMod *)pMod;
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

	if (pthread_mutex_lock(&pLn->Mutex)) {
		free(pHexD->Data);
		free(pHexD);
		return -1;
	}
	*(pLn->pRecvTail)	= pHexD;
	pLn->pRecvTail		= &(pHexD->Next);
	pthread_cond_signal(&pLn->Cond);
	pthread_mutex_unlock(&pLn->Mutex);

	return 0;
}
// 返回门锁状态改变的掩码，LC_SM_STAT代表需要开门，LC_SM_PCNT标识权限有改变
static int LnCmdParse(const uint8_t *Data, uint32_t DtLen, sLnMod *pLn)
{
	int KeyWd=-1, RdLen1=0, PermsChged=0, Ret=-1;
	typTLV tTlv1 = {0};
	uint32_t Seq = 0;
	uint16_t LcChgFlg = 0;

	ConvertMemcpy(&Seq, Data+4, 4);
	RdLen1			= Data[8]+9;
	tTlv1.NextTag	= (uint8_t*)Data+RdLen1;
	while (RdLen1 < DtLen-1) {
		TLVDecode(&tTlv1, &RdLen1);
		if (tTlv1.NextTag > Data+DtLen) {	// 避免后边读取超出数据
			break;
		}
		if (tTlv1.tag==TAG_D_encryptInfo && tTlv1.vlen==3 &&
				(tTlv1.vp[0]*65536+tTlv1.vp[1]*256+tTlv1.vp[2])!=0) {
			// 加密了，不支持，返回失败
			break;
		}
		if (tTlv1.tag==TAG_D_keyword && tTlv1.vlen==1) {
			KeyWd = tTlv1.vp[0];
		} else if (tTlv1.tag == TAG_D_data) {
			int RdLen2 = 0;
			typTLV tTlv2 = {0};

			RdLen2			= 0;
			tTlv2.NextTag	= tTlv1.vp;
			while (RdLen2 < tTlv1.vlen-1) {
				TLVDecode(&tTlv2, &RdLen2);
				if (tTlv2.NextTag > tTlv1.NextTag) {
					break;
				}
				if (KeyWd==RCMD_SetupParameter && tTlv2.tag==TAG_D_parameter) {
					// setup设置门锁
					int RdLen3 = 0;
					typTLV tTlv3 = {0};

					RdLen3			= 0;
					tTlv3.NextTag	= tTlv2.vp;
					while (RdLen3 < tTlv2.vlen-1) {
						TLVDecode(&tTlv3, &RdLen3);
						if (tTlv3.NextTag > tTlv2.NextTag) {
							break;
						}
						if (tTlv3.tag==TAG_L_openDelay && tTlv3.vlen==1 && tTlv3.vp[0]>=1) {
							char tStr[10] = {0};
							snprintf(tStr, 10, "%hhu", tTlv3.vp[0]);

							Ret = SetCfgStr(CFG_FTYPE_CFG, "Lock", "Delay", tStr);
							if (Ret == 0) {
								pLn->CloseDelay = tTlv3.vp[0];
							}
						}
					}
				} else if (KeyWd==RCMD_PutPopedom && tTlv2.tag == TAG_L_popedom) {
					int Opr=-1, Type=-1;
					uint64_t PermID = 0;
					uint32_t StartT=0, EndT=0;
					uint8_t *p = tTlv2.vp;

					// 操作类型
					if (p>=tTlv2.NextTag || *p++!=TAG_M_operate || *p++!=1) {
						continue;
					}
					Opr = *p++;
					if (Opr != 3) {
						// 权限类型
						if (p>=tTlv2.NextTag || *p++!=TAG_L_popedomType || *p++!=1) {
							continue;
						}
						Type = *p++;
						if (Type<=0 || Type>3) {
							continue;
						}
						// 卡号
						if (p>=tTlv2.NextTag || *p++!=TAG_L_cardNo || *p++!=8) {
							continue;
						}
						memcpy(&PermID, p, 8);
						p += 8;
						if (Opr != 1) {
							// 开始时间
							if (p>=tTlv2.NextTag || *p++!=TAG_M_startTime || *p++!=4) {
								continue;
							}
							ConvertMemcpy(&StartT, p, 4);
							p += 4;
							// 截止时间
							if (p>=tTlv2.NextTag || *p++!=TAG_M_endTime || *p++!=4) {
								continue;
							}
							ConvertMemcpy(&EndT, p, 4);
							p += 4;
						}
					}

					int i=0;

					for (i=0; Opr!=3 && i<pLn->PermsNum; i++) {
						if (pLn->Perms[i].PermID == PermID) {
							break;
						}
					}
					Log("LN", LOG_LV_DEBUG, "PutPopedom, opr=%d", Opr);
					switch (Opr) {
					case 0:
						if (i < pLn->PermsNum) {
							// 修改
							pLn->Perms[i].Head	= 0x10+Type;	// 重新修改权限后，已挂失也接挂
							pLn->Perms[i].StartT= StartT;
							pLn->Perms[i].EndT	= EndT;
						} else {
							// 找到一个完整权限，开始添加
							void *rp = realloc(pLn->Perms, (pLn->PermsNum+1)*sizeof(sPermInfo));

							if (rp == NULL) {
								Log("LN", LOG_LV_ERROR, "Failed to realloc pLn->Perms to %d, errno=%d.", (pLn->PermsNum+1)*sizeof(sPermInfo), errno);
								continue;
							}
							i = pLn->PermsNum;
							pLn->Perms = rp;
							pLn->PermsNum++;
							pLn->Perms[i].Head	= 0x10 + Type;
							pLn->Perms[i].PermID= PermID;
							pLn->Perms[i].StartT= StartT;
							pLn->Perms[i].EndT	= EndT;
						}
						PermsChged = 1;
						Ret = 0;
						break;
					case 1:
						if (i < pLn->PermsNum) {
							// 删除
							memmove(pLn->Perms+i, pLn->Perms+i+1, (pLn->PermsNum-i-1)*sizeof(sPermInfo));
							pLn->Perms = realloc(pLn->Perms, (pLn->PermsNum-1)*sizeof(sPermInfo));
							pLn->PermsNum--;
							PermsChged = 1;
							Ret = 0;
						}
						break;
					case 3:
						free(pLn->Perms);
						pLn->Perms		= NULL;
						pLn->PermsNum	= 0;
						PermsChged		= 1;
						Ret = 0;
						break;
					case 4:
						if (i < pLn->PermsNum) {
							// 挂失
							pLn->Perms[i].Head &= 0xEF;
							PermsChged = 1;
							Ret = 0;
						}
						Log("LN", LOG_LV_DEBUG, "lost, i=%d, n=%u, nH=%hhu", i, pLn->PermsNum, pLn->Perms[i].Head);
						break;
					case 5:
						if (i < pLn->PermsNum) {
							// 解挂
							pLn->Perms[i].Head |= 0x10;
							PermsChged = 1;
							Ret = 0;
						}
						break;
					default:
						break;
					}
				} else if (KeyWd == RCMD_GetPopedom) {
					// TODO 获取权限列表
				} else if (KeyWd == RCMD_RemoteControl) {
					// 远程控制
					if (tTlv2.tag==TAG_D_controlAction && tTlv2.vlen==1 && tTlv2.vp[0]==1) {
						Ret = 1;
						LcChgFlg|= LC_SM_STAT;
					}
				}
			}
		}
	}

	uint8_t tData[10];

	tData[0] = TAG_D_resultCode;
	tData[1] = 1;
	tData[2] = Ret<0? BUSINESS_STATE_ERROR_NOT_SUPPORTED_KEYWORD:0;
	BusinessPackage(Data[2]|0x80, BUSINESS_STATE_OK, Seq, tData, 3, NULL, 0, NULL, SOURCE_TYPE_ZKQ);

	if (PermsChged) {
		LcChgFlg|= LC_SM_PCNT;
		LnSavePerms(pLn);
	}

	return LcChgFlg;
}

void* LnRun(void *pMod)
{
	if (pMod == NULL) {
		return NULL;
	}
	if (ProtVer/16 != 2) {
		Log("LN", LOG_LV_DEBUG, "Ln Mode is not supported in ProtVer[%X] now.", ProtVer);
		return NULL;
	}
	sLn485Task RdTasks[2] = {0};
//	int ToCnt1=0, ToCnt2=0;			// 记录超时次数，发送后增加，接收到清0
//	uint8_t LastType=1;				// 记录上次操作，高4bit为类型0读卡号，1重启，2读版本，3升级，低4bit为设备地址
	int i=0, j=0;					// i用来标记当前任务
	double st = -1;					// 记录发送时间
	uint8_t tData[100]= {0}, *RecvData=NULL, nSgnl=0;
	uint32_t RecvLen = 0;
	sLnMod *pLn = pMod;
	double DrOpenT = -1;
	uint16_t LcStatChgFl=0, nLcStat=0, HardVer=0, SoftVer=0;

	// 一些初始操作，注册按钮、对讲、和门磁中断
	for (i=0; i<2; i++) {
		RdTasks[i].Addr		= i+1;
		RdTasks[i].Type		= 2;		// 读版本
		RdTasks[i].VerTime	= -1;
		for (j=0; j<10; j++) {
			RdTasks[i].SigVal[j]=0xFF;
		}
	}
	i=0;
	if (GpioRegIrq(GPIO_LOCK_BTN, 0.1, (GpioIrqCb)IoCallBack, pLn)) {
		Log("LN", LOG_LV_ERROR, "Failed to GpioRegIrq %d.", GPIO_LOCK_BTN);
		exit(EXIT_FAILURE);
	}
	if (GpioRegIrq(GPIO_LOCK_RCTL, 0.1, (GpioIrqCb)IoCallBack, pLn)) {
		Log("LN", LOG_LV_ERROR, "Failed to GpioRegIrq %d.", GPIO_LOCK_RCTL);
		exit(EXIT_FAILURE);
	}
	if (GpioRegIrq(GPIO_LOCK_DTC1, 0.1, (GpioIrqCb)IoCallBack, pLn)) {
		Log("LN", LOG_LV_ERROR, "Failed to GpioRegIrq %d.", GPIO_LOCK_DTC1);
		exit(EXIT_FAILURE);
	}
	if (GpioRegIrq(GPIO_LOCK_DTC2, 0.1, (GpioIrqCb)IoCallBack, pLn)) {
		Log("LN", LOG_LV_ERROR, "Failed to GpioRegIrq %d.", GPIO_LOCK_DTC1);
		exit(EXIT_FAILURE);
	}
	if (GpioGet(GPIO_LOCK_DTC1)!=0 && GpioGet(GPIO_LOCK_DTC2)!=0) {
		nLcStat |= 0x0001;
	}
	LcStatChgFl	|= LC_SM_STAT|LC_SM_PCAP|LC_SM_PCNT;
	LedsCtrl(LED_DOOR_LINK, LED_OPEN_OPR);
	// 通知门锁上线
	SendOnLineNotify(1);
	while (1) {
		if (i<0 || i>1) {
			i = 0;
		}
		// 发送查询包
		if (st+RS485_TIMEOUT < BooTime()) {
			uint16_t SendLen = 0;
			tData[0] = 0x58;
			tData[1] = RdTasks[i].Addr;
			switch (RdTasks[i].Type) {
			case 0:		// 查询卡号
			default:
				RdTasks[i].Type = 0;
				tData[3] = 0x00;
				if (RdTasks[i].ToCnt > 0) {	// 重查
					tData[2] = 0x15;
				} else {					// 查询
					tData[2] = 0x05;
				}
				SendLen = 6;
				break;
			case 1:		// 重启
				tData[2] = 0x01;
				tData[3] = 0x00;
				SendLen	 = 6;
				break;
			case 2:		// 读版本
				tData[2] = 0x02;
				tData[3] = 0x00;
				SendLen	 = 6;
				break;
			case 3:		// 升级
				if (RdTasks[i].OffSet < 0) {
					tData[2] = 0x06;
					tData[3] = 0x02;
					tData[4] = RdTasks[i].FlSize/256;
					tData[5] = RdTasks[i].FlSize%256;
					SendLen	 = 8;
				} else if (RdTasks[i].OffSet >= RdTasks[i].FlSize) {
					tData[2] = 0x08;
					tData[3] = 0x02;
					tData[4] = RdTasks[i].FlSize/256;
					tData[5] = RdTasks[i].FlSize%256;
					SendLen	 = 8;
				} else {
					tData[2] = 0x07;
					tData[3] = 0x12;
					tData[4] = RdTasks[i].OffSet/256;
					tData[5] = RdTasks[i].OffSet%256;
					uint8_t tSize = (RdTasks[i].FlSize-RdTasks[i].OffSet)>16?16:(RdTasks[i].FlSize-RdTasks[i].OffSet);

					// TODO 暂时不判断成功与否了
					UpgradeFileRead(FLAG_UPGRADE_LOCK, RdTasks[i].OffSet, tData+6, tSize);
//					Flash_EmulateE2prom_ReadByte(tData+6, PWLMS_UPDATE_START_ADD+RdTasks[i].OffSet, tSize);
					SendLen	 = 24;
				}
				break;
			}
			tData[SendLen-2]= CheckSum(tData, SendLen-2);
			tData[SendLen-1]= 0x96;

			if (RdTasks[i].Type != 0) {
				char *tStr = NULL;
				if (SendLen>0 && (tStr=malloc(SendLen*3+1)) != NULL) {
					Hex2Str(tStr, tData, SendLen, ' ');
				}
				Log("LN", LOG_LV_DEBUG, "Send Data[%d]:%s.", SendLen, tStr);
				if (tStr) {
					free(tStr);
				}
			}
			McuSendFrRetry(0x02, 0, pLn->DevAddr, 1, tData, SendLen, 0, 0);
			if (++RdTasks[i].ToCnt > RS485_MAX_RETRY) {
				RdTasks[i].ToCnt = RS485_MAX_RETRY;
			}
			st = BooTime();
		}
		// 等待回应和CMD

		// 取出命令或门锁包
		if (pthread_mutex_lock(&pLn->Mutex)) {
			continue;
		}
		sHexData	*pRecv= NULL;
		sMcuCmdInfo *pCmd = NULL;

		while (1) {
			if (pLn->CmdHead) {
				// 外部命令
				pCmd		= pLn->CmdHead;
				pLn->CmdHead= pCmd->Next;
				if (pCmd->Next == NULL) {
					pLn->pCmdTail = &(pLn->CmdHead);
				}
			}
			if (pLn->RecvHead) {
				// 取出所有门锁数据包
				pRecv			= pLn->RecvHead;
				pLn->RecvHead	= NULL;
				pLn->pRecvTail	= &(pLn->RecvHead);
			}
			if (pCmd || pRecv || (st+RS485_TIMEOUT)<BooTime()) {
				break;
			} else {
				struct timespec to = {0};
				double nT = ClockTime(CLOCK_REALTIME)+RS485_TIMEOUT/2;

				to.tv_sec = nT;
				to.tv_nsec= (nT-to.tv_sec)*1000000000LL;

				pthread_cond_timedwait(&(pLn->Cond), &(pLn->Mutex), &to);
			}
		}

		pthread_mutex_unlock(&pLn->Mutex);

		while (pRecv) {
			int RdOs = 1;			// 用来记录有效数据的偏移
			sHexData *p = pRecv;
			sMcuFrmHd *pF = (sMcuFrmHd *)pRecv->Data;

			// 先取出当前数据
			if (pF->Func!=0x03 || pF->Tag!=0) {
				if (pF->Func == 0x03) {
					RdOs = -1;
					McuSendFr(0x83, pLn->DevAddr, 0, pF->Tag, &RdOs, 1);
				}
				free(pRecv->Data);
				pRecv = pRecv->Next;
				free(p);
				continue;
			}
			RdOs = 0;
			McuSendFr(0x83, pLn->DevAddr, 0, pF->Tag, &RdOs, 1);
			RecvData = realloc(RecvData, RecvLen+pF->DtLen);
			memcpy(RecvData+RecvLen, pRecv->Data+6, pF->DtLen);
			RecvLen += pF->DtLen;
			free(pRecv->Data);
			pRecv = pRecv->Next;
			free(p);

			// 校验是否完整出错
			for (; RecvLen>=6; memmove(RecvData, RecvData+RdOs, (RecvLen-=RdOs))) {
				RdOs = 1;
				if (RecvData[0]!=0x58 || RecvData[1]==0 || RecvData[1]>2 || (RecvData[2]&0x80)==0) {
					continue;
				}
				if (RecvData[3]+6 > RecvLen) {
					// 数据长度不够
					break;
				}
				if (RecvData[RecvData[3]+5] != 0x96) {
					continue;
				}
				if ((uint8_t)CheckSum(RecvData, RecvData[3]+4) != RecvData[RecvData[3]+4]) {
					continue;
				}
				RdOs = RecvData[3]+6;

				const uint8_t tNo[8] = {0};
				if (RecvData[2]!=0x85 || (RecvData[3]>0 && RecvData[3]<9) ||
						(RecvData[3]>=9 && memcmp(RecvData+5, tNo, 8)!=0)) {
					char *tStr = NULL;
					if (RdOs>0 && (tStr=malloc(RdOs*3+1)) != NULL) {
						Hex2Str(tStr, RecvData, RdOs, ' ');
					}
					Log("LN", LOG_LV_DEBUG, "Recv Data[%d]:%s.", RdOs, tStr);
					if (tStr) {
						free(tStr);
					}
				}
				// 此时是完整包
				switch (RecvData[2]) {
				case 0x81:
					if (RecvData[1]==RdTasks[i].Addr && RdTasks[i].Type==1) {
						RdTasks[i].ToCnt = 0;
						st = -1;
					}
					break;
				case 0x82:
					if (RecvData[3]==0x09 && RecvData[4]==0x00) {
						RdTasks[RecvData[1]-1].VerTime = BooTime();
						if ((RecvData[1]-1)==i && RdTasks[i].Type==2) {
							RdTasks[i].ToCnt = 0;
							st = -1;
						}
						if (RecvData[1] == 1) {
							if (SoftVer != RecvData[9]*256+RecvData[10]) {
								ConvertMemcpy(&SoftVer, RecvData+9, 2);
								LcStatChgFl |= LC_SM_SOFTV;
							}
							if (HardVer != RecvData[11]*256+RecvData[12]) {
								ConvertMemcpy(&HardVer, RecvData+11, 2);
								LcStatChgFl |= LC_SM_HARDV;
							}
//						} else {
//							// 1的读到了，且版本不匹配，产生低压报警
//							if (memcmp(&SoftVer, RecvData+9, 2) == 0) {
//								if ((nLcStat&0x0800)) {
//									nLcStat		&= ~0x0800;
//									LcStatChgFl	|= LC_SM_STAT;
//								}
//							} else if (SoftVer!=0 && (nLcStat&0x0800)==0) {
//								nLcStat		|= 0x0800;
//								LcStatChgFl	|= LC_SM_STAT;
//							}
						}
					}
					break;
				case 0x85: {
					int j=0;

					if (RdTasks[RecvData[1]-1].Type == 0) {
						RdTasks[RecvData[1]-1].ToCnt = 0;
						if ((RecvData[1]-1) == i) {
							st = -1;
						}
					}
					if (RecvData[3]>=0x09 && RecvData[4]!=0x00) {
						if (memcmp(RecvData+5, tNo, 8) != 0) {
							char *tStr = NULL;

							if ((tStr=malloc(RecvData[3]*2+3))) {
								Hex2Str(tStr, RecvData+3, RecvData[3]+1, 0);
							}
							Log("LN", LOG_LV_DEBUG, "[%hhu]Card Info:%s", RecvData[1], tStr);
							if (tStr) {
								free(tStr);
								tStr = NULL;
							}
						}
						for (j=0; j<pLn->PermsNum; j++) {
							if((pLn->Perms[j].Head&0x10) && memcmp(RecvData+5, &(pLn->Perms[j].PermID), 8)==0) {
								time_t nTime = time(NULL);
								if (nTime>=pLn->Perms[j].StartT && nTime<=pLn->Perms[j].EndT) {
									if (DrOpenT+1 > BooTime()) {
										// 1秒内算重复刷卡，不需要再开门
										break;
									}
									// 开门，并产生开门记录
									uint8_t tLcStat = nLcStat % 16;
									GpioSet(GPIO_LOCK_CTL, 0);
									usleep(1000);
									GpioSet(GPIO_LOCK_CTL, 1);
									if ((tLcStat&0x02) == 0) {
										nLcStat		|= 0x02;		// 锁打开
										LcStatChgFl	|= LC_SM_STAT;
									}
									DrOpenT = BooTime();

									// 产生开门记录
									int tRecdLen = -1;
									time_t nTime = time(NULL);

									if (pthread_rwlock_rdlock(&pLn->LockMutex)) {
										break;
									}
									if (pLn->LockNum > 0) {
										tRecdLen = pLn->Locks[0].DevCode;
									}
									pthread_rwlock_unlock(&pLn->LockMutex);
									if (tRecdLen > 0) {
										tRecdLen = MkOpenRecd(tData, tRecdLen, (RecvData[1]==1?0x10:0x80)|(pLn->Perms[j].Head%16==2?0x02:0x01), 0, nTime, (nLcStat%16)*16 | tLcStat,
															  (nLcStat>>8)%16, RecvData[4]==0x01?0x02:0x01, pLn->Perms[j].PermID);
										MgtLockRecord(NORMAL_RECORD, nTime, tData, tRecdLen);
									}
								}
								break;
							}
						}
					} else if (RecvData[3]==0x09 && RecvData[4]==0x00) {
						RdTasks[RecvData[1]-1].VerTime = BooTime();
						if (RecvData[1] == 1) {
							if (SoftVer != RecvData[9]*256+RecvData[10]) {
								ConvertMemcpy(&SoftVer, RecvData+9, 2);
								LcStatChgFl |= LC_SM_SOFTV;
							}
							if (HardVer != RecvData[11]*256+RecvData[12]) {
								ConvertMemcpy(&HardVer, RecvData+11, 2);
								LcStatChgFl |= LC_SM_HARDV;
							}
						}
					}
					break;
				}
				case 0x86:
					if (RecvData[1]==RdTasks[i].Addr && RecvData[3]==3 &&
							RecvData[4]==RdTasks[i].FlSize/256 && RecvData[5]==RdTasks[i].FlSize%256 &&
							RecvData[6]==1 && RdTasks[i].Type==3 && RdTasks[i].OffSet<0) {
						RdTasks[i].ToCnt	= 0;
						st = -1;
					}
					break;
				case 0x87:
					if (RecvData[1]==RdTasks[i].Addr && RdTasks[i].Type==3 && RecvData[6]==1 &&
							RdTasks[i].OffSet>=0 && RdTasks[i].OffSet<RdTasks[i].FlSize &&
							RecvData[4]==RdTasks[i].OffSet/256 && RecvData[5]==RdTasks[i].OffSet%256
					   ) {
						RdTasks[i].ToCnt	 = 0;
						st = -1;
					}
					break;
				case 0x88:
					if (RecvData[1]==RdTasks[i].Addr && RdTasks[i].Type==3 &&
							RecvData[4]==RdTasks[i].FlSize/256 && RecvData[5]==RdTasks[i].FlSize%256 &&
							RecvData[6]==1 && RdTasks[i].OffSet>=RdTasks[i].FlSize) {
						RdTasks[i].ToCnt	= 0;
						st = -1;
					}
					break;
				default:
					break;
				}
			}

		}
		// TODO切换 任务？st为-1则当前485任务完成，否则超时后处理
		if (st+RS485_TIMEOUT < BooTime()) {
			if (st < 0) {
				// TODO 任务完成，通知还是怎么样？
				if (RdTasks[i].Type == 3) {
					if (RdTasks[i].OffSet < 0) {
						RdTasks[i].OffSet = 0;
					} else if (RdTasks[i].OffSet >= RdTasks[i].FlSize) {
						RdTasks[i].Type = 0;
					} else {
						RdTasks[i].OffSet +=16;
					}
				} else {
					RdTasks[i].Type = 0;
				}
				CalcPct(RdTasks[i].SigVal, 1);
			} else {
				// TODO 任务超时，通知还是怎么样？
				CalcPct(RdTasks[i].SigVal, -1);
				if (RdTasks[i].ToCnt >= RS485_MAX_RETRY) {
					RdTasks[i].Type = 0;
				}
			}
			if (CalcSignal(RdTasks[0].SigVal, RdTasks[1].SigVal) != nSgnl) {
				nSgnl		= CalcSignal(RdTasks[0].SigVal, RdTasks[1].SigVal);
				LcStatChgFl|= LC_SM_SGNL;
			}

			if (RdTasks[i].Type!=1 && RdTasks[i].Type!=3 && RdTasks[i].VerTime+RS485_RDVER_INTVL < BooTime()) {
				// 需要去读取一下版本
				RdTasks[i].Type = 2;
			}
			i++;
		}

		// TODO CMD操作
		if (pCmd) {
			switch (pCmd->Type) {
			case MCU_CMD_FORWARD:
				Log("LN", LOG_LV_DEBUG, "MCU_CMD_FORWARD KeyWord=%hhu.", pCmd->Arg.RdFwd.Data[2]);
				LcStatChgFl = LnCmdParse(pCmd->Arg.RdFwd.Data, pCmd->Arg.RdFwd.Len, pLn);
				if ((LcStatChgFl&LC_SM_STAT)) {
					uint8_t tLcStat = nLcStat % 16;
					GpioSet(GPIO_LOCK_CTL, 0);
					usleep(1000);
					GpioSet(GPIO_LOCK_CTL, 1);
					nLcStat |= 0x02;		// 锁打开
					if ((tLcStat&0x02)) {
						LcStatChgFl	&= ~LC_SM_STAT;
					}
					DrOpenT = BooTime();

					// 产生开门记录
					int tRecdLen = -1;
					time_t nTime = time(NULL);

					if (pthread_rwlock_rdlock(&pLn->LockMutex)) {
						break;
					}
					if (pLn->LockNum > 0) {
						tRecdLen = pLn->Locks[0].DevCode;
					}
					pthread_rwlock_unlock(&pLn->LockMutex);
					if (tRecdLen > 0) {
						tRecdLen = MkOpenRecd(tData, tRecdLen, 0x59, 0, nTime, (nLcStat%16)*16 | tLcStat, (nLcStat>>8)%16, 0, 0);
						MgtLockRecord(NORMAL_RECORD, nTime, tData, tRecdLen);
					}
				}
				free(pCmd->Arg.RdFwd.Data);
				break;
			case MCU_CMD_READ:
				Log("LN", LOG_LV_DEBUG, "MCU_CMD_READ");
				// TODO 先不支持
//				LnCmdParse(pCmd->Arg.RdFwd.Data, pCmd->Arg.RdFwd.Len, pLn);
				free(pCmd->Arg.RdFwd.Data);
				break;
			case MCU_CMD_RESET:
				Log("LN", LOG_LV_DEBUG, "MCU_CMD_RESET");
				// 重启的优先级最高，直接替换当前任务
				RdTasks[0].Type	= 1;
				RdTasks[1].Type	= 1;
				RdTasks[0].ToCnt= 0;
				RdTasks[1].ToCnt= 0;
				st = -1;
				break;
			case MCU_CMD_UPGRADE: {
				uint32_t FileSize=0;
				uint8_t tSendBuf[100]= {0};

				// TODO 暂时不判断成功与否了
				UpgradeFileRead(FLAG_UPGRADE_LOCK, 0, tSendBuf, CONTENT_OFFSET);
//				Flash_EmulateE2prom_ReadByte(tSendBuf, PWLMS_UPDATE_START_ADD, 64);

				memcpy(&FileSize, tSendBuf+FILESIZE_OFFSET, 4);
				if (FileSize <= 0) {
					break;
				} else {
					FileSize+=CONTENT_OFFSET;
					// 优先级更高的只有重启，但是2者都是上位机发起的，撞到的可能性很小，即使撞到也可以认为时操作失误，再来就好了
					RdTasks[0].Type		= 3;
					RdTasks[1].Type		= 3;
					RdTasks[0].ToCnt	= 0;
					RdTasks[1].ToCnt	= 0;
					RdTasks[0].OffSet	= -1;
					RdTasks[1].OffSet	= -1;
					RdTasks[0].FlSize	= FileSize;
					RdTasks[1].FlSize	= FileSize;
					st = -1;
				}
				break;
			}
			case LN_CMD_IO: {
				Log("LN", LOG_LV_DEBUG, "LN_CMD_IO=%d.", pCmd->Arg.Std.DevCode);
				if (pCmd->Arg.Std.DevCode==GPIO_LOCK_BTN || pCmd->Arg.Std.DevCode==GPIO_LOCK_RCTL) {
					uint8_t tLcStat = nLcStat % 16;
					GpioSet(GPIO_LOCK_CTL, 0);
					usleep(1000);
					GpioSet(GPIO_LOCK_CTL, 1);
					if ((tLcStat&0x02) == 0) {
						nLcStat		|= 0x02;		// 锁打开
						LcStatChgFl	|= LC_SM_STAT;
					}
					DrOpenT = BooTime();

					// 产生开门记录
					int tRecdLen = -1;
					time_t nTime = time(NULL);

					if (pthread_rwlock_rdlock(&pLn->LockMutex)) {
						break;
					}
					if (pLn->LockNum > 0) {
						tRecdLen = pLn->Locks[0].DevCode;
					}
					pthread_rwlock_unlock(&pLn->LockMutex);
					if (tRecdLen > 0) {
						tRecdLen = MkOpenRecd(tData, tRecdLen, 0x09|(pCmd->Arg.Std.DevCode==GPIO_LOCK_BTN?0x20:0x40),
											  0, nTime,(nLcStat%16)*16 | tLcStat, (nLcStat>>8)%16, 0, 0);
						MgtLockRecord(NORMAL_RECORD, nTime, tData, tRecdLen);
					}
				} else if (pCmd->Arg.Std.DevCode == GPIO_LOCK_DTC1) {
					if ((nLcStat&0x0001)) {
						// 门闭合了
						nLcStat		&= 0xFFFE;
						LcStatChgFl	|= LC_SM_STAT;
						Log("LN", LOG_LV_DEBUG, "Door is closed.");
					}
					if ((nLcStat&0x0100)) {
						nLcStat &= 0xFEFF;		// 取消强闯报警
						LcStatChgFl |= LC_SM_STAT;
						Log("LN", LOG_LV_DEBUG, "Break-In alarm Canceled.");
					}
					if (DrOpenT < 0) {
						nLcStat &= 0xFFFD;		// 指示门已上锁
						Log("LN", LOG_LV_DEBUG, "Door is locked.");
					}
				} else {
					if ((nLcStat&0x0001) == 0) {
						// 门打开了
						nLcStat		|= 0x0001;
						LcStatChgFl	|= LC_SM_STAT;
						Log("LN", LOG_LV_DEBUG, "Door is opened.");
					}
					if (DrOpenT<0 && (nLcStat&0x0100)==0) {
						int tRecdLen = -1;
						time_t nTime = time(NULL);

						nLcStat |= 0x0100;		// 产生强闯报警
						LcStatChgFl |= LC_SM_STAT;
						Log("LN", LOG_LV_DEBUG, "Break-In alarm!");

						if (pthread_rwlock_rdlock(&pLn->LockMutex)) {
							break;
						}
						if (pLn->LockNum > 0) {
							tRecdLen = pLn->Locks[0].DevCode;
						}
						pthread_rwlock_unlock(&pLn->LockMutex);
						if (tRecdLen > 0) {
							tRecdLen = MkAlmRecd(tData, tRecdLen, ALERT_TYPE_FORCED_ENTER, nTime);
							MgtLockRecord(ALARM_RECORD, nTime, tData, tRecdLen);
						}
					}
				}
				break;
			}
			default:
				Log("LN", LOG_LV_DEBUG, "Unkown CMD=%d.", pCmd->Type);
				break;
			}
			free(pCmd);
		}
		// 时间到关门
		if (DrOpenT>0 && DrOpenT+pLn->CloseDelay<BooTime()) {
			Log("LN", LOG_LV_DEBUG, "CloseDelay=%.13G", pLn->CloseDelay);
			GpioSet(GPIO_LOCK_CTL, 0);
			if ((nLcStat&0x0001)) {
				LcStatChgFl	|= LC_SM_STAT;
				nLcStat		&= 0xFFFD;
				Log("LN", LOG_LV_DEBUG, "Door is locked.");
			}
			DrOpenT		= -1;
		}
		//
		if (LcStatChgFl) {
			int DevCode = 0;
			uint16_t nLcStatChgFl=0, SM=0;

			if (pthread_rwlock_wrlock(&pLn->LockMutex) == 0) {
				if (pLn->LockNum > 0) {
					DevCode = pLn->Locks[0].DevCode;
					if ((LcStatChgFl&LC_SM_PCAP)) {
						if ((pLn->Locks[0].StatMask&LC_SM_PCAP) == 0) {
							pLn->Locks[0].StatMask |= LC_SM_PCAP;
						}
						nLcStatChgFl |= LC_SM_PCAP;
					}
					if ((LcStatChgFl&LC_SM_PCNT)) {
						if ((pLn->Locks[0].StatMask&LC_SM_PCNT)==0 || pLn->Locks[0].PermsCnt!=pLn->PermsNum) {
							pLn->Locks[0].PermsCnt	 = pLn->PermsNum;
							pLn->Locks[0].StatMask	|= LC_SM_PCNT;
							nLcStatChgFl			|= LC_SM_PCNT;
						}
					}
					if ((LcStatChgFl&LC_SM_STAT)) {
						if ((pLn->Locks[0].StatMask&LC_SM_STAT)==0 || pLn->Locks[0].Status!=nLcStat) {
							pLn->Locks[0].Status	 = nLcStat;
							pLn->Locks[0].StatMask	|= LC_SM_STAT;
							nLcStatChgFl			|= LC_SM_STAT;
						}
					}
					if ((LcStatChgFl&LC_SM_SGNL)) {
						if ((pLn->Locks[0].StatMask&LC_SM_SGNL)==0 || pLn->Locks[0].Signal!=(100-nSgnl)) {
							pLn->Locks[0].Signal	 = (100-nSgnl);
							// 信号强度变化太快了，不再主动上传
							if ((pLn->Locks[0].StatMask&LC_SM_SGNL) == 0) {
								nLcStatChgFl		|= LC_SM_SGNL;
							}
							pLn->Locks[0].StatMask	|= LC_SM_SGNL;
						}
					}
					if ((LcStatChgFl&LC_SM_SOFTV)) {
						if ((pLn->Locks[0].StatMask&LC_SM_SOFTV)==0 || pLn->Locks[0].SoftVer!= SoftVer) {
							pLn->Locks[0].SoftVer	 = SoftVer;
							pLn->Locks[0].StatMask	|= LC_SM_SOFTV;
							nLcStatChgFl			|= LC_SM_SOFTV;
						}
					}
					if ((LcStatChgFl&LC_SM_HARDV)) {
						if ((pLn->Locks[0].StatMask&LC_SM_HARDV)==0 || pLn->Locks[0].HardVer!= HardVer) {
							pLn->Locks[0].HardVer	 = HardVer;
							pLn->Locks[0].StatMask	|= LC_SM_HARDV;
							nLcStatChgFl			|= LC_SM_HARDV;
						}
					}
					SM=pLn->Locks[0].StatMask;
					pLn->Locks[0].CollTime = time(NULL);
				}
				pthread_rwlock_unlock(&pLn->LockMutex);
			}
			// 状态改变上传
			if (DevCode>0 && nLcStatChgFl!=0) {
				LcSendStatus(DevCode, 0, nLcStatChgFl);
			}
			Log("LN", LOG_LV_DEBUG, "ChgFlg=0x%hX, SM=0x%hX, Pcnt=%u, Stat=0x%hX, Sgnl=%hhu, SoftV=0x%hX, HardV=0x%hX",
				nLcStatChgFl, SM, pLn->PermsNum, nLcStat, nSgnl, SoftVer, HardVer);
			LcStatChgFl = 0;
		}
	}

	return 0;
}
