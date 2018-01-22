#include <list>

//#include	<execinfo.h>		// for dbg
#include <stdlib.h>

//#include "NormalTool.h"
#include <stdint.h>
#include <string.h>
//#include "date.h"
//#include <ucos_ii.h>
#include <stdio.h>
#include <stdarg.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include "stdafx.h"
#include "main_linuxapp.h"
#include "TCPCommApp.h"
#include "Dapp.h"
#include "NormalTool.h"
#include "ManageTransmit.h"
//#include "date.h"



int bufcmp(unsigned char  *buf1, unsigned char *buf2, int len);
int bufstr(unsigned char *dbuf, const char *str, int iLen);
 uchar AscToHex(uchar asc);
void delay_us(ulong x);
unsigned char CountZero(unsigned long c, unsigned char len);
unsigned long GetSysTime(void);
 void PrintfTime(void);
//void ConvertMemcpy(uchar *dest, const uchar *src, int len);
void ConvertMemcpy(void *dest, const void *src, int len);
ulong GetRtcTime(void);
unsigned long GetSysTime(void);
//unsigned long CheckSum(void *buf, int iLens);
void PrintfStr(void);

// int debug_printf(const char *format, ...);
// int echo_printf(const char *format, ...);

// int ser_printf(const char *format, ...);
int defence_printf(const char *format, ...);
//int rf_printf(const char *format, ...);
 int sp_printf(const char *format, ...);
 int CAN_printf(const char *format, ...);
 int main_printf(const char *format, ...);
 int tcp_printf(const char *format, ...);
 int  GetLocalMac(unsigned char *retbuf)   ;


unsigned short CRC16(unsigned char const *buffer, unsigned short len);
//unsigned long CRC32(const unsigned char *InStr, unsigned int len);
void SetRtcTime(uint time);

void SetDog(uchar TaskDog);
void ClearDog(uchar TaskDog);
void ClearAllDog(void);
int IsAllDogTrue(void);

 void WatchDogFeed(void);
// void StartBeepByMode(uchar times, uchar worktime, uchar resttime);
// int min(int val1, int val2);
 void reboot(void);
// void RFIntEnable(void);
// void RFIntDisable(void);
// void sysfree(uchar *ptr);
void ShiftMemcpy(uchar *s, uint len, uint shift, uchar mode);

 unsigned long CRC32_recursion(const unsigned char *InStr, unsigned int len, unsigned int Crc);

extern void RTC_WaitForLastTask(void);
extern void BKP_WriteBackupRegister(uint16_t BKP_DR, uint16_t Data);

//extern "C" void LedsCtrl( int num, int oper );

/* extern variables */
extern int g_debug;

/* local variables */
volatile ULONG g_LastTaskDogCheckTime = 0;
volatile UINT lSystemHms = 0;

/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
unsigned static short const crc16_table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

const static unsigned int  crc32tab[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

void reboot(void)
{
	Log("RB", LOG_LV_DEBUG, "Rebooting.");
   char cmd[128]={0};
   printf("####%s####",__func__);
   strncpy(cmd,"reboot",sizeof(cmd));
   system(cmd);
	do {
	} while (1);
}

 void USART1_SendBuf( uchar *pt, ushort n )
{
	while(n--)
		printf("%c", (char)(*pt++) );
 }

void sysfree(uchar *ptr)
{
    free(ptr);
}


int bufcmp(unsigned char *buf1, unsigned char *buf2, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        if (buf1[i] != buf2[i])
            return 0;
    }
    return 1;
}

/*
返回值: -1 - 未找到, >=0 - 所要找的字段在目标字段中的位置
*/
int bufstr(unsigned char *dbuf, const char *str, int iLen)
{
	int i;
	int sl = strlen(str);

	for (i = 0; i <= iLen-sl; i++)
	{
		if (bufcmp(&dbuf[i], (unsigned char *)str, sl))
		{
			return (i);
		}
	}

	return -1;
}

uchar AscToHex(uchar asc)
{
    if ((asc>='0') & (asc<='9')) return(asc-'0');
    else if ((asc>='A') & (asc<='F')) return(asc-'A'+10);
    else if ((asc>='a') & (asc<='f')) return(asc-'a'+10);
    else return(0xff);
}

void delay_us(ulong x)
{
	for ( ; x>0; x-- );
}

unsigned char CountZero(unsigned long c, unsigned char len)
{
    unsigned char i, count = 0;
	for (i = 0; i < len; i++)
	{
        if (!(c & (1<<i)))
        {
            count++;
        }
	}

	return count;
}

#if 1
//get current b file time start from run first,unit: 100ms
unsigned long GetSysTime(void)
{
	uint time;

//	OS_CPU_SR cpu_sr;
	time = lSystemHms;

	return (ulong)(time);
}
#endif

#ifdef USE_LINUX
void SetRtcTime(uint time)
{
	timeval tv;
	tv.tv_sec = time;
	tv.tv_usec = 0;

//	int iErrCode = 0;
	settimeofday (&tv, NULL);
	FILE *stream;
       stream = popen("hwclock -w", "w");
       pclose(stream);
}

void RebootWMC(void)
{
	system("ifconfig can0 down");
	usleep(500000);
	system("reboot");
}

ULONG GetRtcTime(void)
{
	unsigned long t_curr = time((time_t *)NULL);
	return (t_curr);
}

#else
#define BKP_DR1                           ((ushort)0x0004)
void SetRtcTime(uint time)
{
    RTC_Configuration();
    BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);

    RTC_WaitForLastTask();
    RTC_SetCounter(time);
}


ULONG GetRtcTime(void)
{
	uint time = 0;
	OS_CPU_SR cpu_sr;

	RTC_WaitForLastTask();
	time = RTC_GetCounter();
	return (ulong)(time);
}
#endif

void PrintfTime(void)
{
    time_t currtime;
    struct tm rtc;

    currtime = GetRtcTime();
    localtime_r(&currtime, &rtc); // 8 hours beijin time
    ser_printf("time:%04d-%02d-%02d %02d:%02d:%02d\r\n", rtc.tm_year+1900, rtc.tm_mon+1, rtc.tm_mday, rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
}

void PrintfTimeDec(time_t time_val)
{
    struct tm rtc;

    localtime_r(&time_val, &rtc); // 8 hours beijin time
    ser_printf("%04d-%02d-%02d %02d:%02d:%02d", rtc.tm_year+1900, rtc.tm_mon+1, rtc.tm_mday, rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
    //echo_printf("%04d-%02d-%02d %02d:%02d:%02d", rtc.tm_year, rtc.tm_mon, rtc.tm_mday, rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
}

void PrintfStr(void)
{
    ;
}

//for type little to big byte mode or big to little byte mode convert
//void ConvertMemcpy(uchar *dest, const uchar *src, int len)
void ConvertMemcpy(void *dest, const void *src, int len)
{
    int i;
    uint8_t *Dest=(uint8_t*)dest;
    const uint8_t *Src=(const uint8_t*)src;

    for (i = len-1; i >= 0; i--) {
        Dest[len-1-i] = Src[i];
//        *dest++ = *(src+i);
    }
}


/**
 * crc16 - compute the CRC-16 for the data buffer
 * @crc:	previous CRC value
 * @buffer:	data pointer
 * @len:	number of bytes in the buffer
 *
 * Returns the updated CRC value.
 */
unsigned short CRC16(unsigned char const *buffer, unsigned short len)
{
	unsigned short crc = 0x0000;

	while (len--)
	{
		crc = (crc >> 8) ^ crc16_table[(crc ^ (*buffer++)) & 0xff];
	}

	return crc;
}


unsigned long CRC32(const void *InData, unsigned int len)
{
	unsigned int i;
    unsigned int Crc;
    const unsigned char *InStr = (const unsigned char *)InData;

    Crc = 0xFFFFFFFF;
    for ( i=0; i<len; i++)
    {
        Crc = (Crc >> 8) ^ crc32tab[(Crc & 0xFF) ^ InStr[i]];
    }
    Crc ^= 0xFFFFFFFF;

    return Crc;
}

unsigned long CRC32_recursion(const unsigned char *InStr, unsigned int len, unsigned int Crc)
{
	unsigned int i;

    for (i=0; i<len; i++)
    {
        Crc = (Crc >> 8) ^ crc32tab[(Crc & 0xFF) ^ InStr[i]];
    }

    return Crc;
}

//int min(int val1, int val2)
//{
//    if (val1 >= val2)
//    {
//        return (val2);
//    }
//    else
//    {
//        return (val1);
//    }
//}


#ifndef echo_printf
/* 在诊断模式下打印输出, 普通模式下无效 */
int echo_printf(const char *format, ...)
{
#if 0
    char buff[512] = "";
    int chars;
    va_list ap;
    uchar err;

    if (0 == g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif
#endif
    return 0;
}
#endif

#ifndef ser_printf
int ser_printf(const char *format, ...)
{
#if 0
#ifdef _DEBUG_GPRS
    char buff[512] = "";
    int chars;
    va_list ap;
    uchar err;
    //return 0;

    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }

#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif

#endif
#endif
    return 0;
}
#endif

int defence_printf(const char *format, ...)
{
    char buff[512] = "";
    int chars;
    va_list ap;
//    uchar err;

    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif

    return 0;
}

#ifndef rf_printf
int rf_printf(const char *format, ...)
{
#if 0
    char buff[512] = "";
    int chars;
    va_list ap;
    uchar err;

    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif
#endif
    return 0;
}

#endif

//sp: server protocol
int sp_printf(const char *format, ...)
{
#if 1
    char buff[512] = "";
    int chars;
    va_list ap;
//    uchar err;

//	return 0;
    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif
#endif
    return 0;
}

//sp: server protocol
int CAN_printf(const char *format, ...)
{
    char buff[512] = "";
    int chars;
    va_list ap;
//    uchar err;

//	return 0;
    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif

    return 0;
}

int main_printf(const char *format, ...)
{
    char buff[512] = "";
    int chars;
    va_list ap;
//    uchar err;

//	return 0;
    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif

    return 0;
}


int tcp_printf(const char *format, ...)
{
    char buff[512] = "";
    int chars;
    va_list ap;
//    uchar err;

//	return 0;
    if (g_debug)
    {
        return 0;
    }

#ifdef  USE_LINUX
       pthread_mutex_lock(&UPRINT);
#else
    OSMutexPend(UPRINT, 0, &err);
#endif
    va_start(ap, format);
    chars = vsprintf(buff, format, ap);
    va_end(ap);

    if (-1 != chars)
    {
        USART1_SendBuf((uchar *)buff, strlen(buff));
    }
#ifdef  USE_LINUX
       pthread_mutex_unlock(&UPRINT);
#else
      OSMutexPost(UPRINT);
#endif

    return 0;
}


void SetDog(uchar TaskDog)
{
//	OS_CPU_SR cpu_sr;
	g_TransmitControl.dog |= (uchar)(0x1)<<TaskDog;
}

void ClearDog(uchar TaskDog)
{
//	OS_CPU_SR cpu_sr;
	g_TransmitControl.dog &= ~((uchar)(0x1)<<TaskDog);
}

void ClearAllDog(void)
{
//	OS_CPU_SR cpu_sr;
	g_TransmitControl.dog = 0;
}

int IsAllDogTrue(void)
{
    int ret = 0;

//    OS_CPU_SR cpu_sr;
	if ((g_TransmitControl.dog & 0xF) == 0xF)   // check manage, rfnet, gprs, defence tasks - 2013-09-04   //only check manage and rfnet task
	{
	     ret = 1;
	}

	//printf("IsAllDogTrue... g_TransmitControl.dog=0x%02X, ret=%d\r\n", g_TransmitControl.dog, ret);

	return (ret);
}

int check_soft_watchdog()
{
    int ret = 0;
    ULONG currtime;

    if (0 == g_LastTaskDogCheckTime)
    {
        g_LastTaskDogCheckTime = GetSysTime();
#ifdef _debug_softwatchdog_prt_i
        ser_printf("g_LastTaskDogCheckTime=%ld\r\n", g_LastTaskDogCheckTime);
        PrintfTime();
#endif
        return 0;
    }

	currtime = GetSysTime();
#if 1
	if (currtime > g_LastTaskDogCheckTime + 1200)   // 1200/10 = 120s = 2min
	{
#ifdef _debug_softwatchdog_prt_i
    ser_printf("currtime=%ld, g_LastTaskDogCheckTime=%ld, diff=%ld\r\n", currtime, g_LastTaskDogCheckTime, currtime - g_LastTaskDogCheckTime);
    ser_printf("type=%d\r\n", type);
    PrintfTime();
#endif
		if (IsAllDogTrue())
		{
			//printf("IsAllDogTrue = 1 ...\r\n");
			g_LastTaskDogCheckTime = currtime;
			ClearAllDog();
		}
		else
		{
#ifdef _USE_DOG_SOFTWATCHDOG
			printf("IsAllDogTrue = 0 ...\r\n");
//            ret = 1;
			//reboot();
#else
			g_LastTaskDogCheckTime = currtime;
#endif
		}
	}
#endif

    return ret;
}

uchar* sysmalloc(int len)
{
//    uchar err;
    uchar *ptr;

#ifdef  USE_LINUX
       pthread_mutex_lock(&lockmaloc);
#else
    OSMutexPend(lockmaloc, 0, &err);
#endif

    ptr = (uchar *)malloc(len);
#ifdef  USE_LINUX
       pthread_mutex_unlock(&lockmaloc);
#else
    OSMutexPost(lockmaloc);
#endif

    if (ptr == NULL)
    {
        rf_printf("malloc len :%d fail\r\n", len);
        reboot();
    }

    return (ptr);
}

#if 0
void sysfree(uchar *ptr)
{
    uchar err;

#ifdef  USE_LINUX
       pthread_mutex_lock(&lockmaloc);
#else
    OSMutexPend(lockmaloc, 0, &err);
#endif
    free(ptr);
#ifdef  USE_LINUX
       pthread_mutex_unlock(&lockmaloc);
#else
    OSMutexPost(lockmaloc);
#endif
}
#endif

void ShiftMemcpy(uchar *s, uint len, uint shift, uchar mode)
{
	uint i;
	if(len<shift) return;

    if (mode == LEFT_SHIFT)
    {
		for(i=0;i<(len-shift);i++) s[i]=s[i+shift];
    }
    else if (mode == RIGHT_SHIFT)
    {
		for(i=(len-shift);i>0;i--) s[i-1+shift]=s[i-1];
    }
}

int  GetLocalMac(unsigned char *retbuf)
{
    int sock_mac;

    struct ifreq ifr_mac;
    char mac_addr[30];

    sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_mac == -1)
    {
        printf("create mac socket false\n");
        return 0;
    }

    memset(&ifr_mac,0,sizeof(ifr_mac));
    strncpy(ifr_mac.ifr_name, "eth0", sizeof(ifr_mac.ifr_name)-1);

    if( (ioctl( sock_mac, SIOCGIFHWADDR, &ifr_mac)) < 0)
    {
        printf("mac ioctl error\n");
        return 0;
    }

    sprintf(mac_addr,"%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[5]);

    printf("local mac:%s \n",mac_addr);

    close( sock_mac );
    memcpy(retbuf, (unsigned char *)(ifr_mac.ifr_hwaddr.sa_data), 6);
    return ( 1);
}

/*
// LINK:系统联机指示灯
#define RUN_LED			  1     // RUN:运行指示灯
#define W1_LED				2     // D1:门锁联机指示灯
#define W2_LED				3     // D2:摄像机指示灯
#define W3_LED				4     // D3:3G网络指示灯
#define W4_LED				5     // D4:
#define WIFI_LED			6     // WIFI:


#define LED_DOOR_OPEN   5   // 门开关指示灯
#define LED_RSV1        5   // 保留指示灯
#define LED_GPRS        5   // GPRS模块指示灯

////////////////////////////////////////////////////////////////////////////////////////
#define LED_DOOR_LINK   2   // 锁联机指示灯
#define LED_RUN         1   // 主控器运行指示灯
#define LED_SERVER_LINK 0   // 联网指示灯
#define LED_3G_NET      4
#define LED_SEND_FILE		6		// 录像文件发送指示灯

#define LED_3G_NET      4   // 3G网络指示灯

#define LED_SERVER_LINK 5   // 联网指示灯
*/

void CameraCtrlPinSet(int oper)
{
}

 void WatchDogFeed(void)
 {
 }

/********************************************************************/
void	perrorEx(const char *func, const char *oper, const char *para)
{
#ifndef		NDEBUG
	char	ErrBuff[128];
	snprintf(ErrBuff, sizeof(ErrBuff), "%s,%s %s", func, oper, NULL == para ? "" : para);
	perror((const char *)ErrBuff);
#endif
}

#ifdef		NDEBUG
#define	dprintf(x...)
#define	dperror(x...)
#else
#define	dprintf(x...)		printf(x)
#define	dperror(x...)		perror(x)
#endif

/**********************************************************/
// for in system dbg
/*
void	dump(int signo)
{
	FILE	*fp;
	time_t	timep;
	void	*array[20];
	size_t	size, i;
	char	**strings;
	char	temp[40];

	//system("ps");
	system("free");

	signo = signo;
	size = backtrace (array, 20);
	strings = backtrace_symbols (array, size);

	fp	= fopen(ERROR_LOG, "a+");
	if (fp == NULL)
	{
		perrorEx(__FUNCTION__, "fopen", ERROR_LOG);
		exit(1);
	}

	time(&timep);
	fputs(ctime(&timep), fp);
	sprintf(temp, "Obtained %zd stack frames.\n", size);
	fputs(temp, fp);
	printf("%s", temp);
	for (i = 0; i < size; i++)
	{
		dprintf("%s\n", strings[i]);
		fputs(strings[i], fp);
		fputc('\n', fp);
	}
	fputs("\n\n", fp);

	free (strings);
	fclose(fp);
	exit(0);
}
*/
/* Obtain a backtrace and print it to @code{stdout}. */
/*
void print_trace (int signo)
{
    void *array[10];
    int size;
    char **strings;
    int i;

    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);
    if (NULL == strings)
    {
        perror("backtrace_synbols");
        exit(EXIT_FAILURE);
    }

    printf ("Obtained %zd stack frames.\n", size);

    for (i = 0; i < size; i++)
    printf ("%s\n", strings[i]);

    free (strings);
    strings = NULL;

    exit(0);
}
*/
