#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../NormalTool.h"
#include "../main_linuxapp.h"
#include "../cfgFile.h"
#include "../../include/gpio.h"
#include "../../include/common.h"
#include "../../include/log.h"
#include "lc.h"
#include "mcumod.h"
#include "rf.h"
#include "ln.h"

#define MCU_VER_INTVL		60	// 60���һ��MCU�İ汾

typedef void(*McuCmdCb)(void *Arg, int Ret);
typedef struct SendBuf {
	struct SendBuf	*Next;
	uint8_t			*Data;
	uint16_t		Len;
	double			SendT;		// �ϴεķ���ʱ��
	float			To;			// ���͵���ʱ�ļ��
	uint8_t			ToCnt;		// �ѳ�ʱ�Ĵ���
	uint8_t			ToCntMax;	// ��ʱ�Ĵ��������ޣ�ToCnt���ڵ��ڴ˷���
} sSendBuf;

typedef struct SendCache {
	sSendBuf		*BufHead;
	sSendBuf		**pBufTail;
	pthread_mutex_t	Lock;
	sSendBuf		*UnAckedFrm;// �˱��������̷߳��ʣ������̲߳����������ݣ���˶���Ĳ���Ҫ��������
} sSendCache;

typedef struct McuSelfTask {
	struct McuSelfTask	*Next;
	uint8_t				Type;			// �������ͣ�0���汾�����������룬������Ϣ������ɺ��Ϊ���汾����1������2����
	McuCmdCb			Cb;				// ����Ļص�����
	void				*CbArg;			// �ص��������ݵ�ֵ
	int					UpFd;			// ���������ļ�����������TypeΪ1ʱ��Ч
	uint16_t			NewVer;			// �����������汾
} sMcuSelfTask;

typedef struct McuSelf {
	uint8_t				Vers[4];		// ��Ӳ���汾����0��3����ΪSoftV���ֽڡ�SoftV���ֽڡ�HardV���ֽڡ�HardV���ֽ�
	sMcuSelfTask		*pHead;			// ��δ������������ͷ
	sMcuSelfTask		**ppTail;		// ��δ������������β
	pthread_rwlock_t	Lock;			// ����Vers,pHead,ppTail����
	double				VerNextT;		// ��Ӳ���汾�ϴζ�ȡ����ʱ��
	sMcuSelfTask		*pNowTask;		// ��ǰ���ڴ��������
	double				TaskNexT;		// ��ǰ�����´���Ҫ��Ԥ��ʱ��
	uint8_t				TaskToCnt;		// ��ǰ����ʱ�Ĵ���
} sMcuSelf;

// ��������ģʽ������0��������1
static int LcMode = 0;
// PIPE�����ڴ���
static int PipeFds[2];

// ���ͻ���
static sSendCache McuSdCache;

static sMcuSelf McuInfo = {0};
// ������Ŀ�����й����в�����䶯
static uint8_t ModNum		= 0;
static sMcuMod **pModules	= NULL;
// ������Ϣ����������
//static pthread_rwlock_t LockMutex;
//static uint8_t LockNum 	= 0;
//static sLockInfo *Locks	= NULL;

// ����DevCode�ҵ�������Mod���
static int FindModI(int32_t DevCode)
{
	int i=0, j=0, ModIndex=-1;

	for (i=0; (i<ModNum && ModIndex<0); i++) {
		if (pthread_rwlock_rdlock(&pModules[i]->LockMutex)) {
			continue;
		}
		for (j=0; j<pModules[i]->LockNum; j++) {
			if (pModules[i]->Locks[j].DevCode == DevCode) {
				ModIndex = i;
				break;
			}
		}

		pthread_rwlock_unlock(&(pModules[i]->LockMutex));
	}

	return ModIndex;
}

// Type 0���1ɾ��3ɾ������
int LcSetDevice(int Type, int32_t DevCode, uint64_t LockAddr)
{
	int i=0, ModIndex = -1;
	sMcuCmdSetDev SetDev = {0};

	SetDev.Type		= Type;
	SetDev.DevCode	= DevCode;
	SetDev.Addr		= LockAddr;

	switch (Type) {
	case 0:
		// TODO ���ĸ�ģ�����أ�Ŀǰ����ҵ����޸ģ����Ǹ�ģ�飬�������û��������ģ��
		if (DevCode == 0) {
			break;
		}
		// ���Ҹ�ģ��
		ModIndex = FindModI(DevCode);
		if (ModIndex >= 0) {
			break;
		}
		// ���û�ҵ������ҿ�ģ��
		for (i=0; (i<ModNum && ModIndex<0); i++) {
			if (pthread_rwlock_rdlock(&pModules[i]->LockMutex)) {
				continue;
			}
			if (pModules[i]->LockNum == 0) {
				ModIndex = i;
			}

			pthread_rwlock_unlock(&(pModules[i]->LockMutex));
		}
		break;
	case 1:
		ModIndex = FindModI(DevCode);
		break;
	case 3:
		for (i=0; i<ModNum; i++) {
			pModules[i]->Cmd(pModules[i], MCU_CMD_SETDEV, &SetDev);
		}
		break;
	default:
		return -1;
		break;
	}
	//�洢���ļ���
	char LockDevCodes[1000]= {0}, DevCodeStr[30]= {0}, DevAddrName[50]= {0}, DevAddr[50]= {0};
	char *OldCodes = GetCfgStr(CFG_FTYPE_CFG, "Lock", "DevCodes","");

	snprintf(DevAddr, 50, "%llu", LockAddr);
	snprintf(DevCodeStr, 30, "%d", DevCode);
	switch (Type) {
	case 0:
		// ������DevCodes
		if (OldCodes && OldCodes[0]!='\0') {
			if (!strstr(OldCodes, DevCodeStr)) {
				snprintf(LockDevCodes, 1000, "%s,%s", OldCodes, DevCodeStr);
			}
		} else {
			snprintf(LockDevCodes, 1000, "%s", DevCodeStr);
		}
		if (LockDevCodes[0]) {
			SetCfgStr(CFG_FTYPE_CFG, "Lock", "DevCodes", LockDevCodes);
		}
		// ������Addr
		snprintf(DevAddrName, 1000, "Addr%s", DevCodeStr);
		SetCfgStr(CFG_FTYPE_CFG, "Lock", DevAddrName, DevAddr);
		break;
	case 1:
		if (OldCodes) {
			char *p = strstr(OldCodes, DevCodeStr);
			if (p) {
				int tLen = strlen(DevCodeStr);
				if (tLen>=0 && p[tLen] == '\0') {
					// ���������
					if (p != OldCodes) {
						// ǰ�������ݣ�ȥ��֮ǰ�Ķ���
						*(--p) = '\0';
					} else {
						// ǰ�������ݣ����Ҳ�ǿ�
						*p = '\0';
					}
				} else {
					// ��ߵ�һ��ʱ���ţ��Ѷ���֮���Ų����
					memmove(p, p+tLen+1, strlen(p+tLen+1));
				}
				SetCfgStr(CFG_FTYPE_CFG, "Lock", "DevCodes", OldCodes);
			}
		}
		break;
	case 3:
		if (OldCodes && OldCodes[0]!='\0') {
			SetCfgStr(CFG_FTYPE_CFG, "Lock", "DevCodes", "");
		}
		break;
	default:
		break;
	}
	free(OldCodes);

	// ��ModIndex�������ӿڣ������Ѿ����ù���
	if (ModIndex < 0) {
		return 0;
	} else {
		return pModules[ModIndex]->Cmd(pModules[ModIndex], MCU_CMD_SETDEV, &SetDev);
	}
}

// ������ȡָ��������Ϣ
int LcGetStatus(int32_t DevCode, uint16_t StatMask, sLockInfo *rLcInfo)
{
	if (rLcInfo == NULL) {
		return -1;
	}
	int i=0, j=0;
	sLockInfo *pLock=NULL;

	for (i=0; i<ModNum; i++) {
		if (pthread_rwlock_rdlock(&pModules[i]->LockMutex)) {
			continue;
		}
		for (j=0; j<pModules[i]->LockNum; j++) {
			if (DevCode == pModules[i]->Locks[j].DevCode) {
				pLock = pModules[i]->Locks+j;
				break;
			}
		}

		if (pLock == NULL) {
			// û���ҵ���
			pthread_rwlock_unlock(&pModules[i]->LockMutex);
			continue;
		}

		memcpy(rLcInfo, pLock, sizeof(sLockInfo));
		rLcInfo->StatMask = StatMask&pLock->StatMask;
		pthread_rwlock_unlock(&pModules[i]->LockMutex);

		break;
	}

	if (pLock) {
		return 0;
	} else {
		return -1;
	}
}


int LcGetOnline(int32_t DevCode)
{
	int Ret = -1;
	sLockInfo tInfo = {0};

	Ret = LcGetStatus(DevCode, ~0, &tInfo);

	if (Ret == 0) {
		return tInfo.Online;
	} else {
		return -1;
	}

}

static int McuHardReset()
{
	if (GpioSet(GPIO_MCU_RST, 0) == 0) {
		usleep(1000);
		return GpioSet(GPIO_MCU_RST, 1);
	} else {
		return -1;
	}
}

static int OpenSetUart(const char *Path, int Baud, int DataBit, char Parity, int StopBit, int FlowCtrl)
{
	int fd = -1;
	struct termios OldAttr= {0}, NewAttr= {0};
	double st = 0;

//	cfmakeraw(&NewAttr);

	NewAttr.c_iflag  |= IGNBRK;
	NewAttr.c_cflag  |= CREAD | CLOCAL;		// CREAD�����޷���������
	// ����������
	if (cfsetospeed(&NewAttr, Baud)<0 || cfsetispeed(&NewAttr, Baud)<0) {
		return -1;
	}

	// ����λ����
	switch (DataBit) {
	case 5:
		NewAttr.c_cflag |= CS5;
		break;
	case 6:
		NewAttr.c_cflag |= CS6;
		break;
	case 7:
		NewAttr.c_cflag |= CS7;
		break;
	default:
		// Ĭ��8λ
		NewAttr.c_cflag |= CS8;
		break;
	}

	// ��żУ������
	switch(Parity) {
	case 'o':
	case 'O':
		// ��У��
		NewAttr.c_cflag |= PARODD;
	case 'e':
	case 'E':
		// żУ��
		NewAttr.c_iflag |= INPCK;
		NewAttr.c_cflag |= PARENB;
		break;
	default:
		// Ĭ����У��
		NewAttr.c_iflag |= IGNPAR;
		break;
	}

	// ����ֹͣλ
	switch (StopBit) {
	case 2:
		NewAttr.c_cflag |= CSTOPB;
		break;
	default:
		NewAttr.c_cflag &= ~CSTOPB;
		break;
	}

	// ��������
	switch (FlowCtrl) {
	case 1:
		// �������
		NewAttr.c_iflag |= IXON | IXOFF | IXANY;
		break;
	case 2:
		// Ӳ������
		NewAttr.c_cflag |= CRTSCTS;
		break;
	default:
		// ������
		NewAttr.c_cflag &= ~CRTSCTS;
		NewAttr.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
	}

	fd = open(Path, O_RDWR|O_NOCTTY|O_NDELAY|O_NONBLOCK);
	if (fd < 0) {
		return -1;
	}

	st = BooTime();
	while (1) {
		tcsetattr(fd, TCSANOW, &NewAttr);
		if (tcgetattr(fd, &OldAttr)==0 && memcmp(&OldAttr, &NewAttr, sizeof(struct termios))==0) {
			break;
		} else if(st+1 < BooTime()) {
			close(fd);
			fd = -1;
			break;
		}
	}
	tcflush(fd, TCIOFLUSH);

	return fd;
}

// �����Եط��ͣ����ط��ͳ�ȥ�����ݣ�һ��ʼ�ͷ������󷵻�-1
static int UartWrite(int fd, const uint8_t *Data, int Len)
{
	if (Data==NULL || Len<=0) {
		return -1;
	}
	int Ret=-1, SendLen=0;
	int Cnt=0;

	while (Len > SendLen) {
		Ret = write(fd, Data+SendLen, Len-SendLen);
		if (Ret > 0) {
			Cnt		= 0;
			SendLen+= Ret;
		} else {
			Log("LOCK", LOG_LV_INFO, "Failed to UartWrite, cnt=%d, ret=%d, errno=%d.", Cnt, Ret, errno);
			if (++Cnt >= 5) {
				Log("LOCK", errno==EAGAIN?LOG_LV_INFO:LOG_LV_WARN, "Failed to UartWrite, give up, errno=%d.", errno);
				break;
			}
		}
	}

	return SendLen;
}

int LcCmdReset(int32_t DevCode)
{
	int ModIndex = -1;
	sMcuCmdStd Std = {0};

	Std.DevCode = DevCode;

	ModIndex = FindModI(DevCode);

	if (ModIndex < 0) {
		return -1;
	} else {
		return pModules[ModIndex]->Cmd(pModules[ModIndex], MCU_CMD_RESET, &Std);
	}
}

int LcCmdSetTime(void)
{
	int i=0, j=0;
	sMcuCmdStd Std = {0};

	for (i=0; i<ModNum; i++) {
		if (pthread_rwlock_rdlock(&pModules[i]->LockMutex)) {
			continue;
		}
		int32_t DevCodes[pModules[i]->LockNum];
		int DevNum = pModules[i]->LockNum;
		for (j=0; j<pModules[i]->LockNum; j++) {
			DevCodes[j] = pModules[i]->Locks[j].DevCode;
		}

		pthread_rwlock_unlock(&(pModules[i]->LockMutex));
		for (j=0; j<DevNum; j++) {
			Std.DevCode = DevCodes[j];

			pModules[i]->Cmd(pModules[i], MCU_CMD_SETTIME, &Std);
		}
	}

	return 0;
}

int LcCmdReadData(int32_t DevCode, uint32_t Seq, const uint8_t *Buf, int Len)
{
	if (Buf==NULL || Len<=0) {
		return -1;
	}
	int ModIndex = FindModI(DevCode);

	if (ModIndex < 0) {
		return -1;
	} else {
		sMcuCmdRdFwd Rd = {0};

		Rd.DevCode	= DevCode;
		Rd.Seq		= Seq;
		Rd.Len		= Len;
		Rd.Data		= malloc(Len);
		if (Rd.Data == NULL) {
			return -1;
		}
		memcpy(Rd.Data, Buf, Len);

		return pModules[ModIndex]->Cmd(pModules[ModIndex], MCU_CMD_READ, &Rd);
	}
}
int LcCmdForward(int32_t DevCode, uint32_t Seq, const uint8_t *Data, uint16_t Len)
{
	if (Data==NULL || Len<=0) {
		return -1;
	}
	int ModIndex = FindModI(DevCode);

	if (ModIndex < 0) {
		return -1;
	} else {
		sMcuCmdRdFwd Fwd = {0};

		Fwd.DevCode	= DevCode;
		Fwd.Seq		= Seq;
		Fwd.Len		= Len;
		Fwd.Data	= malloc(Len);
		if (Fwd.Data == NULL) {
			return -1;
		}
		memcpy(Fwd.Data, Data, Len);

		return pModules[ModIndex]->Cmd(pModules[ModIndex], MCU_CMD_FORWARD, &Fwd);
	}
	return 0;
}
int LcCmdUpdateNotify(void)
{
	int i=0, j=0;
	sMcuCmdStd Std = {0};

	for (i=0; i<ModNum; i++) {
		if (pthread_rwlock_rdlock(&pModules[i]->LockMutex)) {
			continue;
		}
		int32_t DevCodes[pModules[i]->LockNum];
		int DevNum = pModules[i]->LockNum;
		for (j=0; j<pModules[i]->LockNum; j++) {
			DevCodes[j] = pModules[i]->Locks[j].DevCode;
		}

		pthread_rwlock_unlock(&(pModules[i]->LockMutex));
		for (j=0; j<DevNum; j++) {
			Std.DevCode = DevCodes[j];

			pModules[i]->Cmd(pModules[i], MCU_CMD_UPGRADE, &Std);
		}
	}

	return 0;
}
// ��������뻺��
int McuSendFrRetry(uint8_t Func, uint8_t SrcAddr, uint8_t DstAddr, uint8_t Tag, const void *Data, uint16_t DtLen, float TimeOut, uint8_t MaxRetry)
{
	if (Data==NULL && DtLen!=0) {
		return -1;
	}
	uint8_t *FrData = NULL;
	sSendBuf *pFr = NULL;

	FrData	= malloc(DtLen+9);
	if (FrData == NULL) {
		return -1;
	}
	pFr		= calloc(1, sizeof(sSendBuf));

	if (pFr == NULL) {
		free(FrData);
		return -1;
	}
	FrData[0]	= 0x58;
	FrData[1]	= Func;
	FrData[2]	= SrcAddr;
	FrData[3]	= DstAddr;
	FrData[4]	= Tag;
	FrData[5]	= DtLen%256;
	FrData[6]	= DtLen/256;
	if (DtLen) {
		memcpy(FrData+7, Data, DtLen);
	}
	FrData[7+DtLen]	= RvCheckSum(FrData+1, DtLen+6);
	FrData[8+DtLen]	= 0x96;

	pFr->Data		= FrData;
	pFr->Len		= 9+DtLen;
	pFr->Next		= NULL;
	pFr->ToCnt		= 0;
	pFr->SendT		= -1;
	//
	pFr->To			= TimeOut;
	pFr->ToCntMax	= (Func&0x80)?0:MaxRetry;

	char *tStr = NULL;
	if (pFr->Len>0 && (tStr=malloc(pFr->Len*3+1))) {
		Hex2Str(tStr, pFr->Data, pFr->Len, ' ');
	}
	Log("LOCK", LOG_LV_INFO, "McuSendFr[%huB]:%s.", pFr->Len, tStr);
	if (tStr) {
		free(tStr);
		tStr = NULL;
	}

	if (pthread_mutex_lock(&(McuSdCache.Lock))) {
		free(FrData);
		free(pFr);
		return -1;
	}
	*(McuSdCache.pBufTail)	= pFr;
	McuSdCache.pBufTail		= &(pFr->Next);
	// ��㷢һ���ֽ���������֪ͨ
	write(PipeFds[1], pFr, 1);
	pthread_mutex_unlock(&(McuSdCache.Lock));

	return 0;
}

// ��������뻺��
int McuSendFr(uint8_t Func, uint8_t SrcAddr, uint8_t DstAddr, uint8_t Tag, const void *Data, uint16_t DtLen)
{
	return McuSendFrRetry(Func, SrcAddr, DstAddr, Tag, Data, DtLen, 0.5, 3);
}

// ������ݰ��Ƿ�������ͬʱ���������ŵ����忪ͷ���޸Ļ���ʣ�೤�ȣ���������������������������ȣ�û������������0��������-1(���ݵĲ��������)
static int McuFrCheck(uint8_t *pData, uint32_t *pLen)
{
	if (pData==NULL || pLen==NULL || *pLen<=0) {
		return -1;
	}

	for (; *pLen>0; memmove(pData, pData+1, (*pLen = *pLen-1))) {
		if (pData[0] != 0x58) {
			// ��ͷ����
			continue;
		}
		if (*pLen < 9) {
			// ���������Ȳ���
			break;
		}

		if ((pData[1]&0x80)) {
			// ��Ӧ��CPU�ģ�SrcAddr������0
			if (pData[2] != 0) {
				continue;
			}
		} else {
			// ������CPU�ģ�DstAddr������0
			if (pData[3] != 0) {
				continue;
			}
		}

		uint32_t DataLen = pData[5]+pData[6]*256;	// �����򳤶�

		if (*pLen < DataLen+9) {
			// ��δ��������
			break;
		}
		if (pData[DataLen+8] != 0x96) {
			// ��β����
			continue;
		}
		if ((uint8_t)RvCheckSum(pData+1, DataLen+7) != 0) {
			// У�����
			continue;
		} else {
			// �ҳ���һ��������
			memmove(pData, pData+1, *pLen-1);	// ȥ����ͷ������ʱû����
			*pLen = *pLen-1;

			return DataLen+8;
		}
	}

	return 0;
}

// ���ݰ������ͷַ�
static void McuFrAnls(const uint8_t *Data, uint32_t Len)
{
	if (Data==NULL || Len<=0) {
		return;
	}
	int i = 0;
	uint8_t Status = 0xFF;
	sMcuFrmHd *pHead = (sMcuFrmHd *)Data;

	// ����ACK��⣬��Ҫ�Ļ����
	if ((pHead->Func&0x80)) {
		// ��Ӧ��
		sSendBuf **ppFr = &McuSdCache.UnAckedFrm;

		while (*ppFr) {
			sSendBuf *pFr = *ppFr;
			sMcuFrmHd *ptHead = (sMcuFrmHd *)(pFr->Data+1);

			if (pHead->Func==(ptHead->Func|0x80) && pHead->SrcA==ptHead->SrcA
					&& pHead->DstA==ptHead->DstA && pHead->Tag==ptHead->Tag) {
				// �յ��ˣ������
				*ppFr = pFr->Next;
				free(pFr->Data);
				free(pFr);
			} else {
				ppFr = &(pFr->Next);
			}
		}
	}
	// �����Ѿ����������ˣ����ټ���ⲿ�����
	switch (pHead->Func) {	//FUNC
	case 0x01: {
		uint32_t OffSet = 0;
		uint16_t Size = 0;
		uint8_t *tData = NULL;

		//��������CPU��TAG��ֻ֧��TAG 0
		if (pHead->Tag==0 && pHead->DtLen==6 && McuInfo.pNowTask && (McuInfo.pNowTask->Type==0 || McuInfo.pNowTask->Type==1)) {
			memcpy(&OffSet, Data+6, 4);
			memcpy(&Size, Data+10, 2);
			if (Size>0 && Size<UINT16_MAX-7 && lseek(McuInfo.pNowTask->UpFd, OffSet, SEEK_SET)==OffSet) {
				tData = malloc(Size+7);
				if (tData) {
					if (read(McuInfo.pNowTask->UpFd, tData+7, Size) == Size) {
						Status = 0;
					}
				}
			}
		}
		if (Status == 0) {
			uint16_t tCrc = CRC16(tData+7, Size);

			tData[0] = Status;
			memcpy(tData+1, &OffSet, 4);
			memcpy(tData+5, &tCrc, 2);
			McuSendFr(pHead->Func|0x80, pHead->SrcA, pHead->DstA, pHead->Tag, tData, Size+7);
		} else {
			// �ظ���֧��
			McuSendFr(pHead->Func|0x80, pHead->SrcA, pHead->DstA, pHead->Tag, &Status, 1);
		}
		if (tData) {
			free(tData);
			tData = NULL;
		}
	}
	break;
	case 0x02:
		// �ظ���֧��
		McuSendFr(pHead->Func|0x80, pHead->SrcA, pHead->DstA, pHead->Tag, &Status, 1);
		break;
	case 0x03:
		if (pHead->SrcA == 1) {
			// MCUģ��
			if (pHead->Tag == 2) {
				// ������
				Status = 0;
			}
			McuSendFr(pHead->Func|0x80, pHead->SrcA, pHead->DstA, pHead->Tag, &Status, 1);
		} else {
			for (i=0; i<ModNum; i++) {
				if (pModules[i]->DevAddr == pHead->SrcA) {
					// ������Ӧ��ģ�鴦��
					pModules[i]->RecvCb(pModules[i], Data, Len);
					break;
				}
			}
			if (i >= ModNum) {
				// û���ҵ���Ӧģ�飬�ظ���֧��
				McuSendFr(pHead->Func|0x80, pHead->SrcA, pHead->DstA, pHead->Tag, &Status, 1);
			}
		}
		break;
	case 0x81:
	case 0x82:
		if (pHead->DstA == 1) {
			// MCUģ��
			// ����Ĵ�����
			switch(pHead->Tag) {
			case 0:
				if (pHead->Func==0x81 && pHead->DtLen==5 && Data[6]==0) {
					if (McuInfo.pNowTask && McuInfo.pNowTask->Type==0) {
						if (memcmp(Data+7, &(McuInfo.pNowTask->NewVer), 2) == 0) {
							if (McuInfo.pNowTask->Cb) {
								McuInfo.pNowTask->Cb(McuInfo.pNowTask->CbArg, 0);
							}
							free(McuInfo.pNowTask);
							McuInfo.pNowTask = NULL;
						} else {
							McuInfo.TaskNexT = BooTime()+2;
							McuInfo.TaskToCnt++;
						}
					}
					if (pthread_rwlock_wrlock(&(McuInfo.Lock)) == 0) {
						memcpy(McuInfo.Vers, Data+7, 4);
						McuInfo.VerNextT = BooTime() + MCU_VER_INTVL;
						pthread_rwlock_unlock(&(McuInfo.Lock));
					}
					Log("MCU", LOG_LV_DEBUG, "MCU Hard Ver:%hhu.%hhu, Soft Ver:%hhd.%hhd", Data[10], Data[9], Data[8], Data[7]);
				}
				break;
			case 1:
				if (pHead->Func==0x82 && pHead->DtLen==1) {
					if (McuInfo.pNowTask && McuInfo.pNowTask->Type==1) {
						if (Data[6] == 0) {
							if (McuInfo.pNowTask->NewVer == 0) {
								// �ް汾��Ϣ��ֱ�ӷ���
								if (McuInfo.pNowTask->Cb) {
									McuInfo.pNowTask->Cb(McuInfo.pNowTask->CbArg, 0);
								}
								free(McuInfo.pNowTask);
								McuInfo.pNowTask = NULL;
							} else {
								// �л�ģʽ����ʼ��ѯ�汾������Ƿ�ɹ�
								McuInfo.pNowTask->Type	= 0;
								McuInfo.TaskNexT		= BooTime()+5;
								McuInfo.TaskToCnt		= 0;
							}
						} else {
							McuInfo.TaskNexT = BooTime();	// ��������
						}
					}
				}
			case 2:
				if (pHead->Func==0x82 && pHead->DtLen==1 && Data[6]==0) {
					if (McuInfo.pNowTask && McuInfo.pNowTask->Type==2) {
						if (McuInfo.pNowTask->Cb) {
							McuInfo.pNowTask->Cb(McuInfo.pNowTask->CbArg, 0);
						}
						free (McuInfo.pNowTask);
						McuInfo.pNowTask = NULL;
					}
				}
			default:
				break;
			}
		} else {
			for (i=0; i<ModNum; i++) {
				if (pModules[i]->DevAddr == pHead->DstA) {
					// ������Ӧ��ģ�鴦��
					pModules[i]->RecvCb(pModules[i], Data, Len);
					break;
				}
			}
			if (i >= ModNum) {
				// û���ҵ���Ӧģ�飬����һ��LOG
				Log("LOCK", LOG_LV_INFO, "No Mod to handle, f=%hhu, sa=%hhu, da=%hhu, t=%hhu.",
					pHead->Func, pHead->SrcA, pHead->DstA, pHead->Tag);
			}
		}
		break;
	default:
		// ��ʶ��
		Log("LOCK", LOG_LV_INFO, "Unkown FUNC:%hhu", pHead->Func);
		break;
	}

}

void LcInit(void)
{
	if (pipe2(PipeFds,O_NONBLOCK|O_CLOEXEC) != 0) {
		Log("LOCK", LOG_LV_ERROR, "Failed to pipe2, errno:%d.", errno);
		exit(1);
	}
	int LnCnt=0, RfCnt=0;

	// ������һ��
	McuHardReset();
	// ������Σ��ж���������ģʽ
	LcMode=0;
	while (LcMode++<100) {
		if (GpioGet(GPIO_LOCK_MODE) == 0) {
			LnCnt++;
		} else {
			RfCnt++;
		}
		usleep(1);
	}
	//TODO �������������жϣ�ͬʱ�б�Ҫ�Ļ�������һ��MCU��������ģʽ�£��ر����������ȣ�
	LcMode = LnCnt<RfCnt;

	Log("LOCK", LOG_LV_WARN, "Lock Mode=%d", LcMode);

	// ��ʼ�����ͻ���
	memset(&McuSdCache, 0, sizeof(sSendCache));
	McuSdCache.pBufTail		= &(McuSdCache.BufHead);
	McuSdCache.UnAckedFrm	= NULL;
	pthread_mutex_init(&McuSdCache.Lock, NULL);

	// ��ʼ��MCU��Ϣ
	memset(McuInfo.Vers, 0, 4);
	McuInfo.VerNextT	= -1;
	McuInfo.pHead		= NULL;
	McuInfo.ppTail		= &(McuInfo.pHead);
	McuInfo.pNowTask	= NULL;
	pthread_rwlock_init(&McuInfo.Lock, NULL);

	// TODO��ʼ��ģ��
	ModNum				= 1;
	pModules			= calloc(ModNum, sizeof(sMcuMod*));
	pModules[0]			= calloc(1, sizeof(sMcuMod));
	pModules[0]->LockNum= 0;
	pthread_rwlock_init(&pModules[0]->LockMutex, NULL);
	if (LcMode) {
		// ����ģʽ

		// TODO���ø�ģ����������Ϣ
		pModules[0]->DevAddr	= 2;
		char *LockDevCodes=GetCfgStr(CFG_FTYPE_CFG, "Lock", "DevCodes","");
		char *p=LockDevCodes;

		// �������ļ��ж�ȡ������Ϣ
		while (p[0]) {
			char *pTail=NULL, tStr[100]= {0};
			int DevCode = -1;
			uint64_t DevAddr = 0;

			DevCode = strtol(p, &pTail, 10);

			if (pTail == p) {
				break;
			}
			if (pTail[0] == '\0') {
				p = pTail;
			} else {
				// ��������
				p = pTail+1;
			}
			if (DevCode <=0 ) {
				continue;
			}
			snprintf(tStr, 100, "Addr%d", DevCode);
			char *AddrStr = GetCfgStr(CFG_FTYPE_CFG, "Lock", tStr,"");

			DevAddr = strtoull(AddrStr, &pTail, 10);
			if (DevAddr<=0 || pTail[0]!='\0') {
				free(AddrStr);
				continue;
			}
			free(AddrStr);
			AddrStr = NULL;
			void *pnLocks = realloc(pModules[0]->Locks,(pModules[0]->LockNum+1)*sizeof(sLockInfo));
			if (!pnLocks) {
				Log("LOCK", LOG_LV_WARN, "Failed to realloc Locks, errno=%d", errno);
				continue;
			}
			pModules[0]->Locks = pnLocks;
			memset(pModules[0]->Locks+pModules[0]->LockNum, 0, sizeof(sLockInfo));
			pModules[0]->Locks[pModules[0]->LockNum].DevCode= DevCode;
			pModules[0]->Locks[pModules[0]->LockNum].Addr	= DevAddr;
			pModules[0]->LockNum++;

			// Ŀǰ֧��ֻ1������
			break;
		}
		free(LockDevCodes);
		pModules[0]->Init		= RfInit;
		pModules[0]->Run		= RfRun;
		pModules[0]->Cmd		= RfCmd;
		pModules[0]->RecvCb		= RfRecvCb;
		pModules[0]->Init(pModules);
	} else {
		// ����ģʽ
		uint64_t LcAddr = 0;

		// �ر�RF���ܣ���ΪĿǰ2�߹��ܲ�ͬʱ����
		// RF����ģʽ��Ϊ��������߲����ˣ����Կ��Զ����Լ���
		McuSendFrRetry(0x02, 0, 2, 4, &LcAddr, 1, 1, 10);
		// Ȼ��ر�������������߲����ˣ����Կ��Զ����Լ���
		McuSendFrRetry(0x02, 0, 2, 2, &LcAddr, 5, 1, 10);
		// ����ģʽ
		pModules[0]->DevAddr	= 3;
		pModules[0]->LockNum	= 1;
		pModules[0]->Locks		= calloc(1, sizeof(sLockInfo));
		//��������
		pModules[0]->Locks[0].DevCode	= 100;
		pModules[0]->Locks[0].Addr		= 1;
		pthread_rwlock_init(&pModules[0]->LockMutex, NULL);
		pModules[0]->Init		= LnInit;
		pModules[0]->Run		= LnRun;
		pModules[0]->Cmd		= LnCmd;
		pModules[0]->RecvCb		= LnRecvCb;
		pModules[0]->Init(pModules);
	}
	Log("LOCK", LOG_LV_DEBUG, "LcInit finished.");
}

int McuSoftReset(McuCmdCb Cb, void *CbArg)
{
	sMcuSelfTask *pTask = NULL;

	// �������񲢲���
	pTask = calloc(1, sizeof(sMcuSelfTask));

	pTask->Cb		= Cb;
	pTask->CbArg	= CbArg;
	pTask->Next		= NULL;
	pTask->Type		= 2;
	pTask->UpFd		= -1;
	pTask->NewVer	= 0;
	if (pthread_rwlock_wrlock(&(McuInfo.Lock)) == 0) {
		*McuInfo.ppTail	= pTask;
		McuInfo.ppTail	= &(pTask->Next);
		pthread_rwlock_unlock(&(McuInfo.Lock));

		return 0;
	} else {
		free(pTask);

		return -1;
	}
}

int McuUpgrade(const char *FlPath, int NewVer, McuCmdCb Cb, void *CbArg)
{
	if (FlPath == NULL) {
		return -1;
	}
	int UpFd = -1;
	sMcuSelfTask *pTask = NULL;

	// ���ļ���unlink��ԭ�ļ�
	UpFd = open(FlPath, O_RDWR);
	if (UpFd < 0) {
		return -1;
	}
	pTask = calloc(1, sizeof(sMcuSelfTask));

	pTask->Cb		= Cb;
	pTask->CbArg	= CbArg;
	pTask->Next		= NULL;
	pTask->Type		= 1;
	pTask->UpFd		= UpFd;
	pTask->NewVer	= NewVer;
	if (pthread_rwlock_wrlock(&(McuInfo.Lock)) == 0) {
		*McuInfo.ppTail	= pTask;
		McuInfo.ppTail	= &(pTask->Next);
		pthread_rwlock_unlock(&(McuInfo.Lock));
		unlink(FlPath);

		return 0;
	} else {
		free(pTask);
		close(UpFd);

		return -1;
	}
}

void* LcThread(void *Arg)
{
	Log("LOCK", LOG_LV_DEBUG, "LcThread started.");
	int Ret=-1, UartFd=-1;
	uint8_t tBuf[10000]= {0}, *RecvBuf=malloc(65545);	// 65545Ϊ��֡���ݵ���󳤶�
	uint32_t RecvLen = 0;
	struct pollfd fds[2] = {0};
	double LastRecvT = BooTime();

	// ָʾ����ʾ
	LedsCtrl(LED_LOCK_MODE, LcMode==0?LED_CLOSE_OPR:LED_OPEN_OPR);

	if (RecvBuf == NULL) {
		Log("LOCK", LOG_LV_ERROR, "Failed to malloc RecvBuf, errno:%d.", errno);
		exit(1);
	}

	UartFd = OpenSetUart("/dev/ttyS0", B230400, 8, 'N', 1, 0);
	if (UartFd < 0) {
		Log("LOCK", LOG_LV_ERROR, "Failed to OpenSetUart, errno:%d.", errno);
		exit(1);
	}

	// fds[0] ��Ӧ����
	fds[0].fd 		= UartFd;
	fds[0].events 	= POLLIN;
	// fds[1] ��Ӧ���ͻ���
	fds[1].fd		= PipeFds[0];
	fds[1].events 	= POLLIN;

	pthread_t ModThreads[ModNum];

	for (Ret=0; Ret<ModNum; Ret++) {
		if (pthread_create(ModThreads+Ret, NULL, pModules[Ret]->Run, pModules[Ret])) {
			Log("LOCK", LOG_LV_ERROR, "Failed to pthread_create[%d], errno=%d", Ret, errno);
			exit(1);
		}
	}

	while (1) {
		if ((LastRecvT + MCU_VER_INTVL*3) < BooTime()) {
			// �ܾ�ʲô���ݶ�δ������
			Log("MCU", LOG_LV_WARN, "MCU need to be hard reset, n=%G, l=%G", BooTime(), LastRecvT);
			McuHardReset();
			LastRecvT = BooTime();
		}
		// ��ѯ�汾
		if (McuInfo.VerNextT < BooTime()) {
			McuInfo.VerNextT = BooTime()+1;		// ÿ�β�ѯ�����֤��1������
			McuSendFrRetry(0x01, 0, 1, 0, NULL, 0, 0, 0);
		}
		// MCU��������
		while (1) {
			if (McuInfo.pNowTask) {
				if (McuInfo.TaskNexT < BooTime()) {
					if (McuInfo.TaskNexT > 0) {
						McuInfo.TaskToCnt++;
					}
					if (McuInfo.TaskToCnt >= 5) {
						// ʧ�ܴ���
						if (McuInfo.pNowTask->Cb) {
							McuInfo.pNowTask->Cb(McuInfo.pNowTask->CbArg, -1);
						}
						free(McuInfo.pNowTask);
						McuInfo.pNowTask = NULL;
					}
					switch (McuInfo.pNowTask->Type) {
					case 0:
						McuSendFrRetry(0x01, 0, 1, 0, NULL, 0, 0, 0);
						McuInfo.TaskNexT = BooTime()+2;
						break;
					case 1: {
						off_t FlLen = 0;
						uint8_t *tData = NULL;

						FlLen = lseek(McuInfo.pNowTask->UpFd, 0, SEEK_END);
						if (FlLen>0 && FlLen!=(off_t)-1 && (tData=malloc(FlLen))!=NULL) {
							uint16_t tCrc = 0;

							if (lseek(McuInfo.pNowTask->UpFd, 0, SEEK_SET)==0 &&
									read(McuInfo.pNowTask->UpFd, tData, FlLen)==FlLen) {
								tCrc = CRC16(tData, FlLen);
								memcpy(tBuf, &FlLen, 4);
								memcpy(tBuf+4, &tCrc, 2);
								McuSendFrRetry(0x02, 0, 1, 1, tBuf, 6, 1, 3);
								McuInfo.TaskNexT = BooTime()+5;	// 5�����Ϊ��ʱ
							} else {
								McuInfo.TaskNexT = BooTime();	// ��������
							}

							free(tData);
							tData = NULL;
						}

						break;
					}
					case 2:
						tBuf[0] = 2;
						McuSendFrRetry(0x02, 0, 1, 2, tBuf, 1, 0.5, 1);
						McuInfo.TaskNexT = BooTime()+5;	// 5�����Ϊ��ʱ
						break;
					default:
						free(McuInfo.pNowTask);
						McuInfo.pNowTask = NULL;
						break;
					}
				}
			}
			if (McuInfo.pNowTask==NULL && pthread_rwlock_wrlock(&(McuInfo.Lock))==0) {
				// ȡ������
				if (McuInfo.pHead) {
					McuInfo.pNowTask= McuInfo.pHead;
					McuInfo.pHead	= McuInfo.pHead->Next;
					if (McuInfo.pHead == NULL) {
						McuInfo.ppTail = &(McuInfo.pHead);
					}
				}
				pthread_rwlock_unlock(&(McuInfo.Lock));
				if (McuInfo.pNowTask == NULL) {
					break;
				} else {
					McuInfo.pNowTask->Next	= NULL;
					McuInfo.TaskNexT		= -1;
					McuInfo.TaskToCnt		= 0;
				}
			} else {
				break;
			}
		}
		fds[0].revents 	= 0;
		fds[1].revents	= 0;
		// ����0.1��
		Ret = poll(fds, 2, 100);
		if ((fds[0].revents & POLLIN)) {
			// �������лظ����ݣ����ղ��ַ�
			Ret = read(UartFd, RecvBuf+RecvLen, 65545-RecvLen);
			if (Ret > 0) {
				char *tStr=NULL;

				LastRecvT = BooTime();
				if ((tStr = malloc(Ret*3+1))) {
					Hex2Str(tStr, RecvBuf+RecvLen, Ret, ' ');
				}
				Log("LOCK", LOG_LV_VERBOSE, "Mcu Recv[%dB]:%s.", Ret, tStr);
				if (tStr) {
					free(tStr);
					tStr=NULL;
				}
				RecvLen += Ret;
				// �ַ�����
				while ((Ret = McuFrCheck(RecvBuf, &RecvLen)) > 0) {
					// ������
					if ((tStr = malloc(Ret*3+1))) {
						Hex2Str(tStr, RecvBuf, Ret, ' ');
					}
					Log("LOCK", LOG_LV_INFO, "McuRecvFr[%dB]:%s.", Ret, tStr);
					if (tStr) {
						free(tStr);
						tStr=NULL;
					}
					McuFrAnls(RecvBuf, Ret);
					memmove(RecvBuf, RecvBuf+Ret, (RecvLen -= Ret));
				}
			} else {
				Log("LOCK", LOG_LV_WARN, "Failed to read form UartFd, ret=%d, errno:%d.", Ret, errno);
			}
		}
		read(PipeFds[0], tBuf, 9999);		// ��������ֽ�
		// ��Ҫ�ط�������
		sSendBuf **ppFr = &McuSdCache.UnAckedFrm;

		while (*ppFr) {
			sSendBuf *pFr = *ppFr;

			if (pFr->SendT+pFr->To < BooTime()) {
				// ʱ�䵽����Ҫ�ط�
				if (pFr->ToCnt >= pFr->ToCntMax) {
					// �Ѵ����ޣ���Ҫ������
					*ppFr = pFr->Next;
					free(pFr->Data);
					free(pFr);
				} else {
					char *tStr = NULL;
					if (pFr->Len>0 && (tStr=malloc(pFr->Len*3+1))) {
						Hex2Str(tStr, pFr->Data, pFr->Len, ' ');
					}
					Log("LOCK", LOG_LV_INFO, "McuReSendFr[%huB]:%s.", pFr->Len, tStr);
					if (tStr) {
						free(tStr);
						tStr = NULL;
					}
					// �ٴη��ͳ�ȥ
					if (UartWrite(UartFd, pFr->Data, pFr->Len) == pFr->Len) {
						pFr->SendT = BooTime();
					}
					pFr->ToCnt++;
				}
			}
			if (*ppFr == pFr) {
				ppFr = &(pFr->Next);
			}
		}

		// �״η��͵�����
		while (1) {
			sSendBuf *pFr = NULL;
			if (pthread_mutex_lock(&McuSdCache.Lock)) {
				// ����ʧ�ܣ�����
				Log("LOCK", LOG_LV_WARN, "Failed to pthread_mutex_lock McuSdCache.Lock, errno:%d.", errno);
				break;
			} else {
				// ȡ��һ�������UnAckedFrm����ͬTag������������

				ppFr = &(McuSdCache.BufHead);
				while (*ppFr) {
					sSendBuf **pptFr = &(McuSdCache.UnAckedFrm);
					while (*pptFr) {
						if (memcmp((*pptFr)->Data+1, (*ppFr)->Data+1, 4) == 0) {
							// �Ѿ���tagһ�������ݰ��ˣ��ð���ʱ��Ҫ���ͣ������ط����ƻ�ʧЧ
							break;
						}
						pptFr = &((*pptFr)->Next);
					}
					if (*pptFr == NULL) {
						// û���ظ��ģ�ȡ���ð�
						pFr = *ppFr;
						*ppFr = pFr->Next;
						if (pFr->Next == NULL) {
							// ��ʱTailָ����ȡ�ߵİ���Next����Ҫ����Tail
							McuSdCache.pBufTail = ppFr;
						}
						break;
					} else {
						ppFr = &((*ppFr)->Next);
					}
				}

				pthread_mutex_unlock(&McuSdCache.Lock);
			}
			// ���ͳ�ȥ
			if (pFr) {
				Ret = UartWrite(UartFd, pFr->Data, pFr->Len);
				if (pFr->ToCntMax>0 && pFr->To>0 && (pFr->Data[1]&0x80)==0) {
					// �����ط�
					if (Ret == pFr->Len) {
						pFr->SendT = BooTime();
					}
					// �ٲ嵽���Ӷ���
					// TODO �ظ�������ޱ�Ҫ��
					sSendBuf **pptFr = &McuSdCache.UnAckedFrm;

					while (*pptFr) {
						sSendBuf *ptFr = *pptFr;

						if (memcmp(pFr->Data+1, ptFr->Data+1, 4) == 0) {
							// �Ѿ�����ͬ��tag�Ļ��������Ҫ�Ѿɰ��滻��
							*pptFr = ptFr->Next;
							free(ptFr->Data);
							free(ptFr);
						} else {
							pptFr = &(ptFr->Next);
						}
					}
					pFr->Next = McuSdCache.UnAckedFrm;
					McuSdCache.UnAckedFrm = pFr;
				} else {
					free(pFr->Data);
					free(pFr);
				}
			} else {
				// û�о�����
				break;
			}
		}
		// TODO����ȴ���ʱ
	}

	return 0;
}
