#ifndef _LIST
#define _LIST

//#include "Rfnet.h"
#include "lc/lc.h"

/* definetion */
#define MAX_GPRS_LEN 10000

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//headdata 格式 : len(2B)+type(1B)+addsource[16]+reserve[1B]

void InitMangeBusinessBuffer(void);
int ManageBusinessDataPush(uchar *sData, int slen, uchar *sHeadData);
int ManageBusinessDataPop(uchar *sData, uchar *sHeadData);
void InitLockBusinessBuffer(void);
int LockBusinessDataPush(uchar *sData, int slen, uchar *sHeadData);
int LockBusinessDataPop(uchar *sData, uchar *sHeadData);
void InitGprsSendBuffer(void);
int GprsSendDataPush(uchar *sData, int slen, uchar *sHeadData);
int GprsSendDataPop(uchar *sData, uchar *sHeadData);
void InitGprsRecvBuffer(void);
int GprsRecvDataPush(uchar *sData, int slen, uchar *sHeadData);
int GprsRecvDataPop(uchar *sData, uchar *sHeadData);
void InitSmsSendBuffer(void);
int SmsSendDataPush(uchar *sData, int slen, uchar *sHeadData);
int SmsSendDataPop(uchar *sData, uchar *sHeadData);
//void ClrWirelessReceiveQue(void);
//int PushWirelessReceiveData(typNET_DATA RfData);
//int PopWirelessReceiveData(typNET_DATA *tempRfData);
int IsGprsSendDataNull(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif
