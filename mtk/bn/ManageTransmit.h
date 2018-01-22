#ifndef TRANSMIT_H_INCLUDE
#define TRANSMIT_H_INCLUDE

#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern int g_RECVWELCOME, g_initialized;
extern typTransmitControl g_TransmitControl;
extern typDEV_PARA_CTL g_BootPara;

extern void SendOnLineNotify(int type);
extern void* TaskManageComm(void *);
extern void StoreSystemPara(void);
extern void TimeSync(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // TRANSMIT_H_INCLUDE
