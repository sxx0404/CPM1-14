#ifndef LN_H_INCLUDED
#define LN_H_INCLUDED

#include <stdint.h>
#include "mcumod.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int LnInit(sMcuMod **ppMod);
// 供别的线程调用
int LnCmd(sMcuMod *pMod, int Type, void *Arg);
int LnRecvCb(sMcuMod *pMod, const uint8_t *Data, int DtLen);
void* LnRun(void *pMod);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // LN_H_INCLUDED
