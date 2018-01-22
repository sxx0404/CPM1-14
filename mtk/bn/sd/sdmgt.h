#ifndef __SD_MGT_H__
#define __SD_MGT_H__

// SD卡状态
enum {
	SD_STAT_INIT = 0,
	SD_STAT_DETCED,					// 1-检测到卡(建立目录列表，若失败则格式化)
	SD_STAT_STANDBY,				// 2-容量和目录检测均OK(只有2是正常的)
	SD_STAT_NOT_DETC,				// 3-未检测到SD卡
	SD_STAT_ERRUSED,					// 4-SD卡文件格式不能使用等原因
	SD_STAT_WRING,           		// 5-SD卡写文件中
	SD_STAT_RDING,         			// 6-SD卡读文件中
	SD_STAT_NUM,
};

// SD卡操作指令
enum {
	SD_CMD_NONE		= -1,
	SD_CMD_WR,
	SD_CMD_RD,
	SD_CMD_TYPES
};

#ifdef __cplusplus
extern "C" {
#endif		// __cplusplus

typedef void (*SdStatCb)(int nStat);
typedef void (*SdOprCb)(void* Arg, int CamId, int RecType, int Time, int Ret);
typedef void (*SdSUpdt)(void* Arg);

// 外部函数
int SdFileOprB(int OprType, int CamId, int RecType, int Time);
int SdInit(void);
void* SdThread(void *Arg);

#ifdef __cplusplus
}
#endif
#endif // __SD_MGT_H__
