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
	int32_t		DevCode;		// 设备编码
	uint64_t	Addr;			// 门锁地址
	uint8_t		Online;			// 在线与否标识,1在线
	time_t		CollTime;		// 状态的采集时间, 从未采集过是0
	uint16_t	StatMask;		// 状态有效项的标记，从最低位开始依次对应一下各项
	uint16_t	Status;			// 门锁状态
	uint8_t		BatStat;		// 门锁电池电量0x61代表6.1V
	uint16_t	Temp;			// 门锁温度
	uint8_t		Signal;			// 信号强度0~100,0最好，100最差
	uint16_t	SoftVer;		// 软件版本，主版本号*256+次版本号
	uint16_t	HardVer;		// 硬件版本，主版本号*256+次版本号
	uint16_t	PermsCap;		// 权限容量上限
	uint16_t	PermsCnt;		// 已使用权限数
	uint16_t	FgPermCap;		// 指纹权限容量上限
	uint16_t	FgPermCnt;		// 已使用指纹权限数

	//以上为各种锁的通用部分

};


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 根据DevCode获取门锁在线与否，-1没找到门锁，0脱机，1在线
int LcGetOnline(int32_t DevCode);
// 根据DevCode和StatMask获得门锁的状态信息
int LcGetStatus(int32_t DevCode, uint16_t StatMask, sLockInfo *rLcInfo);
// 更新所有门锁的时间
int LcCmdSetTime(void);
// 添删改Dev，添改以后请立刻更新锁的信息
int LcSetDevice(int Type, int32_t DevCode, uint64_t LockAddr);
// 根据DevCode重启门锁，DevCode为-1时，重启所有锁
int LcCmdReset(int32_t DevCode);
// 根据DevCode发送读取门锁数据指令，其中Seq如果为0使用门锁自己的序号，否则使用传递的序号
int LcCmdReadData(int32_t DevCode, uint32_t Seq, const uint8_t *Buf, int Len);
// 向给定门锁DevCode转发数据
int LcCmdForward(int32_t DevCode, uint32_t Seq, const uint8_t *Data, uint16_t Len);
// 更新所有锁
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
