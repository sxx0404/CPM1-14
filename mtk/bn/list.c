#include <stdio.h>
#include <string.h>
#include "stdafx.h"
#include "list.h"
//#include "Rfnet.h"
//#include "extern_variable.h"
#include "NormalTool.h"
#include "../include/common.h"
#include "../include/log.h"
#include "lc/lc.h"
#include "main_linuxapp.h"



/* local variables */
static int g_ManageBzTail, g_ManageBzHead;
static int g_LockBzTail, g_LockBzHead;
static int g_GprsSendTail, g_GprsSendHead;
static int g_GprsRecvTail, g_GprsRecvHead;
static int g_SmsSendTail, g_SmsSendHead;
static uchar ManageBzBuf[MAX_MANAGEBZ_BUFFER_LEN];      // 发送给服务器的业务层报文队列(弹出后还需要做传输层封装)
static uchar LockBzBuf[MAX_MANAGEBZ_BUFFER_LEN];        // 转发给门锁的业务层报文队列
static uchar GprsSendBuf[MAX_GPRS_LEN];                 // MangeBzBuf弹出做传输层封装后，弹入该发送队列
static uchar GprsRecvBuf[MAX_GPRS_LEN];                 // 无线网络端收到的数据队列(物理层的数据，需要解析)
static uchar SmsSendBuf[500];

//typNET_QUE g_QueNetRx;

// head指向第一个有效数据，tail指向第一个无效数据，push到tail所指的字节中，另外tail不可等于head，该情况用于标识buf中为空
// 压入1个字节
int PushBuffer( uchar *buf, int MaxLen, uchar c, int *tail, int *head)
{
	int next_tail;

	next_tail = *tail+ 1;
	if ( next_tail >= MaxLen)
	{
		next_tail = 0;
	}

	if ( *head != next_tail )
	{
		buf[*tail] = c;
		*tail = next_tail;

		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_ERROR;
	}
}

/***********************************************************************************************************
* Function: 取出1个字节
* Description:
* Input:
* Output:
************************************************************************************************************/
int PopBuffer( uchar *buf, int MaxLen, uchar *c, int *tail, int *head)
{
	if ( *head != *tail )
	{
		*c = buf[*head];
		(*head) ++;
		if ( *head >= MaxLen)
		{
			*head = 0;
		}

		return STATUS_SUCCESS;
	}
	else
	{
		 return STATUS_ERROR;
	}
}

/***********************************************************************************************************
* Function:清空buf
* Description:
* Input:
* Output:
************************************************************************************************************/
void ClrBuffer( int *tail, int *head )
{
	*head = 0;
	*tail = 0;
}

//读1个字节，不取出
int GetBuffer( uchar *buf, uchar *c, int *tail, int *head)
{
	if ( *head != *tail )
	{
		*c = buf[*head];
		return STATUS_SUCCESS;
	}
	else
	{
		 return STATUS_ERROR;
	}
}

//buffer 不能全满，留最后一个byte以区分满了还是全空
int GetBufferLeftLen(int MaxLen, int *tail, int *head)
{
    if (*tail > *head)
        return (MaxLen -(*tail - *head)-1);
    else if (*tail < *head)
        return *head-*tail-1;
    else
        return (MaxLen -1);
}

int GetBufferExistDataLen(int MaxLen, int *tail, int *head)
{
    if (*tail > *head)
        return (*tail - *head);
    else if (*tail < *head)
        return MaxLen -(*head-*tail);
    else
        return (0);
}


int BusinessDataPush(uchar *sData, int slen, uchar *sHeadData, uchar *queBuf, int queMaxLen, int *tail, int *head)
{
    int len, i;

    len = GetBufferLeftLen(queMaxLen, tail, head);
	if (len < slen +  BUSINESS_HEAD_LEN)
		return 0;
	memcpy(sHeadData, &slen, 2);
	for (i = 0; i < BUSINESS_HEAD_LEN; i++)
	{
        PushBuffer( queBuf, queMaxLen, sHeadData[i], tail, head);
    }

    for (i = 0; i < slen; i++)
    {
        PushBuffer(queBuf, queMaxLen, sData[i], tail, head);
    }

	return (slen +BUSINESS_HEAD_LEN);
}

int BusinessDataPop(uchar *sData, uchar *sHeadData, uchar *queBuf, int queMaxLen, int *tail, int *head)
{
    int len, i;

    len = GetBufferExistDataLen(queMaxLen, tail, head);
    if (len <  BUSINESS_HEAD_LEN)
        return 0;
    for (i = 0; i < BUSINESS_HEAD_LEN; i++)
    {
        PopBuffer(queBuf, queMaxLen, &sHeadData[i], tail, head);
    }

    len = 0;
    memcpy(&len, &sHeadData[0], 2);
    if (len > GetBufferExistDataLen(queMaxLen, tail, head) || len > MAX_GPRS_LEN)
    {
        ClrBuffer(tail, head);
        return 0;
    }
    for (i = 0; i < len; i++)
    {
        PopBuffer(queBuf, queMaxLen, &sData[i], tail, head);
    }

    return (len);
}

void InitMangeBusinessBuffer(void)
{
#ifdef USE_LINUX
	pthread_mutex_lock(&lockListMB);
#else
	OSMutexPend(lockListMB, 0, &err);
#endif
	ClrBuffer(&g_ManageBzTail, &g_ManageBzHead);
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListMB);
#else
	OSMutexPost(lockListMB);
#endif

}

int ManageBusinessDataPush(uchar *sData, int slen, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
	ulong seq;
	int i;
#endif
	int len;
//	OS_CPU_SR cpu_sr;
//	uchar err;
	//uchar keyword;
	char *tStr=NULL;
	if (slen>0 && (tStr=(char *)malloc(slen*3+1))!=NULL) {
        Hex2Str(tStr, sData, slen, ' ');
	}
	Log("LIST", LOG_LV_DEBUG, "ManageBusinessDataPush, l=%d, Data:%s.", slen, tStr);
	if (tStr) {
        free(tStr);
        tStr = NULL;
	}

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListMB);
#else
	OSMutexPend(lockListMB, 0, &err);
#endif

#ifdef _DEBUG_LIST
	sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	sp_printf("manage business data to push %d:\r\n", slen);
	for (i = 0; i < (slen>100?100:slen); i++)
	{
		sp_printf("%02x ", sData[i]);
	}
	sp_printf("\r\n");
	ConvertMemcpy((uchar *)&seq, &sData[4], 4);
	sp_printf("want to sen business data key:%02X, business seq:0x%X\r\n", sData[2] , seq);
	PrintfTime();
	sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
#endif

	len = (BusinessDataPush(sData, slen, sHeadData, ManageBzBuf, MAX_MANAGEBZ_BUFFER_LEN, &g_ManageBzTail, &g_ManageBzHead));
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListMB);
#else
	OSMutexPost(lockListMB);
#endif
	return (len);
}

int ManageBusinessDataPop(uchar *sData, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
	int i;
#endif
	int len;
//	uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListMB);
#else
	OSMutexPend(lockListMB, 0, &err);
#endif
	len = (BusinessDataPop(sData, sHeadData, ManageBzBuf, MAX_MANAGEBZ_BUFFER_LEN, &g_ManageBzTail, &g_ManageBzHead));
#ifdef _DEBUG_LIST
	if (len > 0)
	{
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	  sp_printf("manage business data pop %d:\r\n", len);
	  for (i = 0; i < (len>100?100:len); i++)
	  {
	    sp_printf("%02x ", sData[i]);
	  }
		sp_printf("\r\n");
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	}
#endif
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListMB);
#else
	OSMutexPost(lockListMB);
#endif

    return (len);
}


void InitLockBusinessBuffer(void)
{
    ClrBuffer(&g_LockBzTail, &g_LockBzHead);
}

int LockBusinessDataPush(uchar *sData, int slen, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
	int i;
#endif
	int len;
//	uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListLB);
#else
	OSMutexPend(lockListLB, 0, &err);
#endif

	// TODO:目前程序，该报文实际上是业务层加密后传输层的发送报文
	/* 门锁通信中业务层发送报文的原始数据打印*/
#ifdef _debug_rf_busi_layer_prt_i
	sp_printf("\r\n|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
	sp_printf("[lock busi layer send]send len=%d,hex:\r\n", slen);
	for (i = 0; i < (slen>100?100:slen); i++)
	{
	    sp_printf("%02x ", sData[i]);
	}
	sp_printf("\r\n|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
	PrintfTime();
#endif
	len = (BusinessDataPush(sData, slen, sHeadData, LockBzBuf, MAX_MANAGEBZ_BUFFER_LEN, &g_LockBzTail, &g_LockBzHead));
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListLB);
#else
	OSMutexPost(lockListLB);
 #endif
    return (len);
}

int LockBusinessDataPop(uchar *sData, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
    int i;
#endif
    int len;
//    uchar err;
#ifdef USE_LINUX
	pthread_mutex_lock(&lockListLB);
#else
    OSMutexPend(lockListLB, 0, &err);
#endif
	len = (BusinessDataPop(sData, sHeadData, LockBzBuf, MAX_MANAGEBZ_BUFFER_LEN, &g_LockBzTail, &g_LockBzHead));
#ifdef _DEBUG_LIST
	if (len > 0)
	{
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
		sp_printf("lock business data pop %d:\r\n", len);
		for (i = 0; i < (len>100?100:len); i++)
		{
			sp_printf("%02x ", sData[i]);
		}
		sp_printf("\r\n");
		PrintfTime();
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	}
#endif
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListLB);
#else
	OSMutexPost(lockListLB);
 #endif

	return (len);
}

void InitGprsSendBuffer(void)
{
//	uchar err;
#ifdef USE_LINUX
	pthread_mutex_lock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif
	ClrBuffer(&g_GprsSendTail, &g_GprsSendHead);
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif
}

int IsGprsSendDataNull(void)
{
	int ret;
//	uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif
	if (g_GprsSendTail != g_GprsSendHead)
		ret = 0;
	else
		ret = 1;
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif

	return (ret);
}

int GprsSendDataPush(uchar *sData, int slen, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
    int i;
#endif
    int len;
//    uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif
	len = BusinessDataPush(sData, slen, sHeadData, GprsSendBuf, MAX_GPRS_LEN, &g_GprsSendTail, &g_GprsSendHead) ;
#ifdef _DEBUG_LIST
	sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	sp_printf("gprs send push %d bytes, tail:%d, head:%d, data:\r\n", slen, g_GprsSendTail, g_GprsSendHead);
	for (i = 0; i < (slen>100?100:slen); i++)
	{
		sp_printf("%02x ", sData[i]);
	}
	sp_printf("\r\n");
	sp_printf("gprs send  push head data:\r\n");
	for (i = 0; i < BUSINESS_HEAD_LEN; i++)
	{
		sp_printf("%02x ", sHeadData[i]);
	}
	sp_printf("\r\n");
	sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
#endif
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif

	return(len);
}

int GprsSendDataPop(uchar *sData, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
	int i;
#endif
	int len;
//	uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif
	len = BusinessDataPop(sData, sHeadData, GprsSendBuf, MAX_GPRS_LEN, &g_GprsSendTail, &g_GprsSendHead);
#ifdef _DEBUG_LIST
	if (len > 0)
	{
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
		sp_printf("gprs send pop %d bytes, tail:%d, head:%d, data:\r\n", len, g_GprsSendTail, g_GprsSendHead);
		for (i = 0; i <(len>100?100:len); i++)
		{
			sp_printf("%02x ", sData[i]);
		}
		sp_printf("\r\n");
		sp_printf("gprs send pop head data:\r\n");
		for (i = 0; i < BUSINESS_HEAD_LEN; i++)
		{
			sp_printf("%02x ", sHeadData[i]);
		}
		sp_printf("\r\n");
		PrintfTime();
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	}
#endif
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListGPSend);
#else
	OSMutexPend(lockListGPSend, 0, &err);
#endif
	return(len);
}

void InitGprsRecvBuffer(void)
{
    ClrBuffer(&g_GprsRecvTail, &g_GprsRecvHead);
}

int GprsRecvDataPush(uchar *sData, int slen, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
    int i;
#endif
    int len;
//    uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListGPRecv);
#else
	OSMutexPend(lockListGPRecv, 0, &err);
#endif

#ifdef _DEBUG_LIST
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
    sp_printf("gprs receive push %d data:\r\n", slen);
    for (i = 0; i < (slen>400?400:slen); i++)
    {
        sp_printf("%02x ", sData[i]);
    }
    sp_printf("\r\n");
    PrintfTime();
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
#endif

    len = (BusinessDataPush(sData, slen, sHeadData, GprsRecvBuf, MAX_GPRS_LEN, &g_GprsRecvTail, &g_GprsRecvHead));
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListGPRecv);
#else
	OSMutexPend(lockListGPRecv, 0, &err);
#endif


    return (len);
}

int GprsRecvDataPop(uchar *sData, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
	int i;
#endif
	int len;
//	uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListGPRecv);
#else
	OSMutexPend(lockListGPRecv, 0, &err);
#endif

	len = (BusinessDataPop(sData, sHeadData, GprsRecvBuf, MAX_GPRS_LEN, &g_GprsRecvTail, &g_GprsRecvHead));
#ifdef _DEBUG_LIST
	if (len > 0)
	{
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
		sp_printf("gprs receive pop %d data:\r\n", len);
		for (i = 0; i <(len>100?100:len); i++)
		{
			sp_printf("%02x ", sData[i]);
		}
		sp_printf("\r\n");
		sp_printf("\r\nLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL\r\n");
	}
#endif
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListGPRecv);
#else
	OSMutexPend(lockListGPRecv, 0, &err);
#endif

	  return (len);
}

void InitSmsSendBuffer(void)
{
    ClrBuffer(&g_SmsSendTail, &g_SmsSendHead);
}

int SmsSendDataPush(uchar *sData, int slen, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
    int i;
#endif
    int len;
//    uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListSmsSend);
#else
	OSMutexPend(lockListSmsSend, 0, &err);
#endif

#ifdef _DEBUG_LIST
    sp_printf("sms receive  push %d data:", slen);
    for (i = 0; i < (slen>400?400:slen); i++)
    {
        sp_printf("%02x ", sData[i]);
    }
    sp_printf("\r\n");
    PrintfTime();
#endif

    ser_printf("push sms send len:%d\r\n", slen);

    len = (BusinessDataPush(sData, slen, sHeadData, SmsSendBuf, 500, &g_SmsSendTail, &g_SmsSendHead));

#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListSmsSend);
#else
	OSMutexPend(lockListSmsSend, 0, &err);
#endif
    return (len);
}

int SmsSendDataPop(uchar *sData, uchar *sHeadData)
{
#ifdef _DEBUG_LIST
    int i;
#endif
    int len;
//    uchar err;

#ifdef USE_LINUX
	pthread_mutex_lock(&lockListSmsSend);
#else
	OSMutexPend(lockListSmsSend, 0, &err);
#endif
    len = (BusinessDataPop(sData, sHeadData, SmsSendBuf, 500, &g_SmsSendTail, &g_SmsSendHead));
#ifdef _DEBUG_LIST
    if (len > 0)
    {
        sp_printf("sms receive pop %d data:", len);
        for (i = 0; i <(len>100?100:len); i++)
        {
            sp_printf("%02x ", sData[i]);
        }
        sp_printf("\r\n");
    }
#endif
#ifdef USE_LINUX
	pthread_mutex_unlock(&lockListSmsSend);
#else
	OSMutexPend(lockListSmsSend, 0, &err);
#endif
    return (len);
}

//int PushWirelessReceiveQu(typNET_QUE* que, typNET_DATA  * nd)
//{
//    ushort next_tail;
//    next_tail =  que->tail+1;
//    if (next_tail >= 30) {
//        next_tail = 0;
//    }
//    if (next_tail != que->head) {    /* if full then return;*/
//        que->buf[que->tail] = *nd;
//        que->tail = next_tail;
//        return 0;
//    } else {
//        return -1;
//    }
//}
//
//int PopWirelessReceiveQu(typNET_QUE *que, typNET_DATA  *nd)
//{
//   if (que->head != que->tail) {
//   	    *nd = que->buf[que->head];
//   	    que->head ++;
//   	    if (que->head >= 30) {
//            que->head = 0;
//   	    }
//   	    return 0;
//   	} else {
//        return -1;
//   	}
//}

//void ClrWirelessReceiveQue(void)
//{
//    typNET_QUE *que= &g_QueNetRx;
//    que->tail = 0;
//    que->head = 0;
//}
////成功0，失败-1
//int PushWirelessReceiveData(typNET_DATA  RfData )
//{
//    return PushWirelessReceiveQu(&g_QueNetRx, &RfData);
//}
////成功0，失败-1
//int PopWirelessReceiveData(typNET_DATA *tempRfData)
//{
//    return PopWirelessReceiveQu(&g_QueNetRx, tempRfData);
//}
