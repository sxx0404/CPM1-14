#ifndef __SD_MGT_H__
#define __SD_MGT_H__

// SD��״̬
enum {
	SD_STAT_INIT = 0,
	SD_STAT_DETCED,					// 1-��⵽��(����Ŀ¼�б���ʧ�����ʽ��)
	SD_STAT_STANDBY,				// 2-������Ŀ¼����OK(ֻ��2��������)
	SD_STAT_NOT_DETC,				// 3-δ��⵽SD��
	SD_STAT_ERRUSED,					// 4-SD���ļ���ʽ����ʹ�õ�ԭ��
	SD_STAT_WRING,           		// 5-SD��д�ļ���
	SD_STAT_RDING,         			// 6-SD�����ļ���
	SD_STAT_NUM,
};

// SD������ָ��
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

// �ⲿ����
int SdFileOprB(int OprType, int CamId, int RecType, int Time);
int SdInit(void);
void* SdThread(void *Arg);

#ifdef __cplusplus
}
#endif
#endif // __SD_MGT_H__
