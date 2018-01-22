#ifndef LC_H_INCLUDED
#define LC_H_INCLUDED

#include <stdint.h>
#include <time.h>

#define LC_SM_STAT		0x0001
#define LC_SM_BAT		0x0002
#define LC_SM_TEMP		0x0004
#define LC_SM_SGNL		0x0008
#define LC_SM_SOFTV		0x0010
#define LC_SM_HARDV		0x0020
#define LC_SM_PCAP		0x0040
#define LC_SM_PCNT		0x0080
#define LC_SM_FGPCAP	0x0100
#define LC_SM_FGPCNT	0x0200


typedef struct LockInfo sLockInfo;
struct LockInfo {
	int32_t		DevCode;		// �豸����
	uint64_t	Addr;			// ������ַ
	uint8_t		Online;			// ��������ʶ,1����
	time_t		CollTime;		// ״̬�Ĳɼ�ʱ��, ��δ�ɼ�����0
	uint16_t	StatMask;		// ״̬��Ч��ı�ǣ������λ��ʼ���ζ�Ӧһ�¸���
	uint16_t	Status;			// ����״̬
	uint8_t		BatStat;		// ������ص���0x61����6.1V
	uint16_t	Temp;			// �����¶�
	uint8_t		Signal;			// �ź�ǿ��0~100,0��ã�100���
	uint16_t	SoftVer;		// ����汾�����汾��*256+�ΰ汾��
	uint16_t	HardVer;		// Ӳ���汾�����汾��*256+�ΰ汾��
	uint16_t	PermsCap;		// Ȩ����������
	uint16_t	PermsCnt;		// ��ʹ��Ȩ����
	uint16_t	FgPermCap;		// ָ��Ȩ����������
	uint16_t	FgPermCnt;		// ��ʹ��ָ��Ȩ����

	//����Ϊ��������ͨ�ò���

};


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// ����DevCode��ȡ�����������-1û�ҵ�������0�ѻ���1����
int LcGetOnline(int32_t DevCode);
// ����DevCode��StatMask���������״̬��Ϣ
int LcGetStatus(int32_t DevCode, uint16_t StatMask, sLockInfo *rLcInfo);
// ��������������ʱ��
int LcCmdSetTime(void);
// ��ɾ��Dev������Ժ������̸���������Ϣ
int LcSetDevice(int Type, int32_t DevCode, uint64_t LockAddr);
// ����DevCode����������DevCodeΪ-1ʱ������������
int LcCmdReset(int32_t DevCode);
// ����DevCode���Ͷ�ȡ��������ָ�����Seq���Ϊ0ʹ�������Լ�����ţ�����ʹ�ô��ݵ����
int LcCmdReadData(int32_t DevCode, uint32_t Seq, const uint8_t *Buf, int Len);
// ���������DevCodeת������
int LcCmdForward(int32_t DevCode, uint32_t Seq, const uint8_t *Data, uint16_t Len);
// ����������
int LcCmdUpdateNotify(void);

void LcInit(void);

void* LcThread(void *Arg);

typedef void(*McuCmdCb)(void *Arg, int Ret);
int McuSoftReset(McuCmdCb Cb, void *CbArg);
int McuUpgrade(const char *FlPath, int NewVer, McuCmdCb Cb, void *CbArg);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // LC_H_INCLUDED
