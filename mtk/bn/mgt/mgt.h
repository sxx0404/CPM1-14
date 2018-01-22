#ifndef MGT_H_INCLUDED
#define MGT_H_INCLUDED
#include <stdint.h>

#include "../stdafx.h"

#define DEF_RCD_SRV_IP				"123.157.158.94"
#define DEF_RCD_SRV_PORT			"20000"
#define DEF_LIV_SRV_IP				"123.157.158.94"
#define DEF_LIV_SRV_PORT			"20000"
#define CAM_DCODE_BASE				1100
#define DEF_NORMAL_RECORD_TIME		"5"
#define DED_ALARM_RECORD_TIME		"10"

enum {
    NORMAL_RECORD   = 'A',
    ALARM_RECORD    = 'W',
    REAL_RECORD     = 'R',
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
/* 录像和实时流服务器IP和PORT信息 */
extern typRecdServerInfo g_RecdServerInfo;

int MgtInit(void);
void* MgtThread(void* Arg);
int MgtLockRecord(int RecType, int Time, const uint8_t *Data, int DataLen);
int CamUpFlNb(int CamId, int RecType, int Time);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MGT_H_INCLUDED
