#ifndef MCUMOD_H_INCLUDED
#define MCUMOD_H_INCLUDED

#include "lc.h"

#define MCU_CMD_SETTIME		1
#define MCU_CMD_SETDEV		2
#define MCU_CMD_RESET		3
#define MCU_CMD_READ		4
#define MCU_CMD_FORWARD		5
#define MCU_CMD_UPGRADE		6

#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐

typedef struct McuFrmHd {
	uint8_t Func;
	uint8_t SrcA;
	uint8_t DstA;
	uint8_t Tag;
	uint16_t DtLen;
} sMcuFrmHd;

#pragma pack(pop)               // 恢复原对齐方式
typedef struct McuCmdSetDev {
	uint8_t		Type;
	int32_t		DevCode;
	uint64_t	Addr;
} sMcuCmdSetDev;

typedef struct McuCmdStd {
	int32_t		DevCode;
} sMcuCmdStd;

typedef struct McuCmdRdFwd {
	int32_t		DevCode;
	uint32_t	Seq;
	uint8_t		*Data;
	int			Len;
} sMcuCmdRdFwd;

typedef struct McuMod sMcuMod;
typedef int (*ModInit)(sMcuMod **);
typedef void* (*ModRun)(void *);
typedef int (*ModRecvCb)(sMcuMod *, const uint8_t *, int);
typedef int (*ModCmd)(sMcuMod *, int, void*);

struct McuMod {
	uint8_t				DevAddr;		//该模块的地址
	pthread_rwlock_t	LockMutex;		// 该mutex用来保护LockNum和Locks
	uint8_t				LockNum;
	sLockInfo			*Locks;

	ModInit				Init;
	ModRun				Run;
	ModRecvCb			RecvCb;
	ModCmd				Cmd;
	// 之前为各模块通用的定义
};
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 组包并放入缓冲
int McuSendFr(uint8_t Func, uint8_t SrcAddr, uint8_t DstAddr, uint8_t Tag, const void *Data, uint16_t DtLen);
// 组包并放入缓冲
int McuSendFrRetry(uint8_t Func, uint8_t SrcAddr, uint8_t DstAddr, uint8_t Tag, const void *Data, uint16_t DtLen, float TimeOut, uint8_t MaxRetry);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // MCUMOD_H_INCLUDED
