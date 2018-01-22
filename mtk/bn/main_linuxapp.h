#ifndef _MAIN_LINUX_APP_H
#define _MAIN_LINUX_APP_H

#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern pthread_mutex_t  UPRINT;
extern pthread_mutex_t  lockListGPSend;
extern pthread_mutex_t  lockListGPRecv;
extern pthread_mutex_t  lockListMB;
extern pthread_mutex_t  lockListLB;
extern pthread_mutex_t  lockmaloc;
extern pthread_mutex_t  LockCameraCtrl;
extern pthread_mutex_t  SoftWatchdogMutex;
extern pthread_mutex_t  lockListSmsSend;
extern pthread_mutex_t  lock_take_rec_list;
extern pthread_mutex_t  lock_read_rec_list;
extern pthread_mutex_t  lock_realstream_opr;

extern void LedsCtrl(int num, int oper);

extern typServerInfo g_ServerInfo;
extern int ProtVer;
extern int DevCodeSize;

#ifdef __cplusplus
}
#endif // __cplusplus


#endif


