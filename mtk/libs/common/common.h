#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define min(X,Y) (__extension__ ({typeof(X) x=(X), y=(Y);(x<y)?x:y;}))
#define max(X,Y) (__extension__ ({typeof(X) x=(X), y=(Y);(x>y)?x:y;}))

/** 函数说明：  获得对应时钟ID的时间
 *  返回值：    对应时钟的秒数（小数格式），失败返回-1
 */
double ClockTime(int ClockId);

/** 函数说明：  获得系统启动时间(CLOCK_BOOTTIME，否则CLOCK_MONOTONIC)
 *  返回值：    系统启动到现在的秒数（小数格式）
 */
double BooTime(void);

/** 函数说明：  计算校验和，按字节相加
 *  参数：
 *  @pData      数据指针
 *  @DataLen    数据长度
 *  返回值：    校验和
 */
uint32_t CheckSum(const void *pData, uint32_t DataLen);

/**	函数说明：	计算校验和取反：按字节相加，结果取反
 *	参数：
 *	@pData		数据指针
 *	@DataLen	数据长度
 *	返回值：	校验和取反
 */
uint32_t RvCheckSum(const void *pData, uint32_t DataLen);

/** 函数说明：  网络接口的MAC
 *  参数：
 *  @IfName     网络接口的名称
 *  @MAC        用于返回对应的MAC，请保证6个字节
 *  返回值：    成功返回0
 */
int GetMac(const char *IfName, uint8_t *MAC);

/** 函数说明：  获得网络接口的第一个IP
 *  参数：
 *  @IfName     网络接口的名称
 *  @pAddr      用于返回对应地址，其中pAddr->sa_family为地址类型，调用前请先设置，如IPV4为AF_INET
 *  返回值：    成功返回0
 */
int GetIP(const char *IfName, struct sockaddr *pAddr);

/** 函数说明：  从指定网口手动组一包UDP数据包发出去
 *  参数：
 *  @interface	网络接口的名称
 *  @data      	UDP承载数据中的内容
 *	@datalen	UDP承载数据中的长度
 *	@SrcIP		源IP地址
 *	@SrcPort	源端口
 *	@DstIP		目标IP
 *	@DstPort	目标DstPort
 *  返回值：	成功返回发送的数据包长度
 */
int DIYSendUdp(const char *interface, const void *data, int datalen, const char *SrcIP, int SrcPort, const char *DstIP, int DstPort);

/** 函数说明：  获得tid
 *  返回值：    当前线程的tid
 */
pid_t gettid(void);

/** 函数说明：  2进制数据转字符串
 *  参数：
 *  @StrBuf     用来保存转换后字符串的指针，空间要能装下转换后的字符串（含'\0'）
 *  @Data       2进制数据
 *  @Len        2进制数据的长度（字节为单位）
 *  @Seg        字符串中显示字节间的间隔符，如果不可打印，则无间隔符
 *  返回值：    成功返回转换后字符串的长度，不包括'\0'
 */
int Hex2Str(char *StrBuf, const void *Data, uint32_t Len, char Seg);

/** 函数说明：	字符串转16进制数据
 *  参数：
 *  @Str		需要转换的字符串，以'\0'结尾，可以识别的16进制数据0-9、a-f、A-F
 *  @HexBuf		用来保存16进制数据的buf，请保证长度
 *  返回值：	返回成功转换的16进制数据的字节数（不足一个字节的部分不转换）,出错返回-1
 */
int Str2Hex(const char *Str, uint8_t *HexBuf);

/** 函数说明：  IPV4的ping功能
 *  参数：
 *  @Node		ping的地址，可以是网址
*	@Size		承载数据长度，同ping -s的参数
 *  @TimeOut	超时，单位秒
 *  返回值：	成功返回0，其他为失败
 */
int Ping4(char *Node, uint8_t Size, double TimeOut);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // COMMON_H_INCLUDED
