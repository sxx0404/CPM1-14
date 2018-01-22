#ifndef _NORMAL_H
#define _NORMAL_H

#include "Dapp.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
extern int bufcmp(unsigned char  *buf1, unsigned char *buf2, int len);
extern int bufstr(unsigned char *dbuf, const char *str, int iLen);
extern uchar AscToHex(uchar asc);
extern void delay_us(ulong x);
extern unsigned char CountZero(unsigned long c, unsigned char len);
extern unsigned long GetSysTime(void);
extern void PrintfTime(void);
extern void PrintfTimeDec(time_t time_val);
//extern void ConvertMemcpy(uchar *dest, const uchar *src, int len);
void ConvertMemcpy(void *dest, const void *src, int len);
extern ulong GetRtcTime(void);
//extern unsigned long GetSysTime(void);
//extern unsigned long CheckSum(void *buf, int iLens);
extern void PrintfStr(void);

//extern int debug_printf(const char *format, ...);
//extern int echo_printf(const char *format, ...);

//extern int ser_printf(const char *format, ...);
extern int defence_printf(const char *format, ...);
//extern int rf_printf(const char *format, ...);
extern int sp_printf(const char *format, ...);
extern int CAN_printf(const char *format, ...);
extern int main_printf(const char *format, ...);
extern int tcp_printf(const char *format, ...);
extern int  GetLocalMac(unsigned char *retbuf)   ;


extern unsigned short CRC16(unsigned char const *buffer, unsigned short len);
extern unsigned long CRC32(const void *InData, unsigned int len);
extern void SetRtcTime(uint time);

extern void SetDog(uchar TaskDog);
extern void ClearDog(uchar TaskDog);
extern void ClearAllDog(void);
extern int IsAllDogTrue(void);

extern int check_soft_watchdog(void);
extern void WatchDogFeed(void);
extern void StartBeepByMode(uchar times, uchar worktime, uchar resttime);
//extern int min(int val1, int val2);
extern void reboot(void);
//extern void RFIntEnable(void);
//extern void RFIntDisable(void);
extern uchar* sysmalloc(int len);
extern void sysfree(uchar *ptr);
extern  void ShiftMemcpy(uchar *s, uint len, uint shift, uchar mode);

extern unsigned long CRC32_recursion(const unsigned char *InStr, unsigned int len, unsigned int Crc);
extern void CameraCtrlPinSet(int oper);
extern void RebootWMC(void);

extern void	perrorEx(const char *func, const char *oper, const char *para);
extern	void	dump(int signo);		// for dump dbg
extern void RTC_SetCounter(uint CounterValue);
extern uint RTC_GetCounter(void);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif
