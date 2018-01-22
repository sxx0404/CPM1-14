#ifndef RF_H_INCLUDED
#define RF_H_INCLUDED

#include <stdint.h>
#include "mcumod.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int RfInit(sMcuMod **ppMod);
// 供别的线程调用
int RfCmd(sMcuMod *pMod, int Type, void *Arg);
int RfRecvCb(sMcuMod *pMod, const uint8_t *Data, int DtLen);
void* RfRun(void *pMod);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RF_H_INCLUDED
