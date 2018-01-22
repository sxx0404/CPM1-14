#define _GNU_SOURCE
#include <ctype.h>
#include <ifaddrs.h>
#include <netdb.h>            	// struct addrinfo
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>           	// strcpy, memset(), and memcpy()
#include <time.h>
#include <unistd.h>           	// close()
#include <arpa/inet.h>        	// inet_pton() and inet_ntop()
#include <bits/socket.h>
#include <linux/if_ether.h>   	// ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  	// struct sockaddr_ll (see man 7 packet)
#include <linux/if_ether.h>   	// ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <net/ethernet.h>
#include <net/if.h>           	// struct ifreq
#include <netinet/in.h>       	// IPPROTO_UDP, INET_ADDRSTRLEN struct
#include <netinet/ip.h>       	// ip and IP_MAXPACKET (which is 65535)
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>      	// struct udphdr
#include <sys/ioctl.h>			// macro ioctl is defined
#include <sys/socket.h>       	// needed for socket()
#include <sys/syscall.h>		// For SYS_xxx definitions
#include <sys/time.h>
#include <sys/types.h>        	// needed for socket(), uint8_t, uint16_t, uint32_t

#include "common.h"

#define PACKET_SIZE		4096
#define DEF_TIMEOUT		30

// Checksum function
static uint16_t checksum(const uint16_t *addr, int len)
{
    int nleft = len;
    int sum = 0;
    const uint16_t *w = addr;
    uint16_t answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= sizeof (uint16_t);
    }

    if (nleft == 1) {
        *(uint8_t *) (&answer) = *(uint8_t *) w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

// Build IPv4 UDP pseudo-header and call checksum function.
static uint16_t udp4_checksum(struct ip iphdr, struct udphdr udphdr, const void *payload, int payloadlen)
{
    char buf[IP_MAXPACKET];
    char *ptr;
    int chksumlen = 0;
    int i;

    ptr = &buf[0];  // ptr points to beginning of buffer buf

    // Copy source IP address into buf (32 bits)
    memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
    ptr += sizeof (iphdr.ip_src.s_addr);
    chksumlen += sizeof (iphdr.ip_src.s_addr);

    // Copy destination IP address into buf (32 bits)
    memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
    ptr += sizeof (iphdr.ip_dst.s_addr);
    chksumlen += sizeof (iphdr.ip_dst.s_addr);

    // Copy zero field to buf (8 bits)
    *ptr = 0;
    ptr++;
    chksumlen += 1;

    // Copy transport layer protocol to buf (8 bits)
    memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
    ptr += sizeof (iphdr.ip_p);
    chksumlen += sizeof (iphdr.ip_p);

    // Copy UDP length to buf (16 bits)
    memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
    ptr += sizeof (udphdr.len);
    chksumlen += sizeof (udphdr.len);

    // Copy UDP source port to buf (16 bits)
    memcpy (ptr, &udphdr.source, sizeof (udphdr.source));
    ptr += sizeof (udphdr.source);
    chksumlen += sizeof (udphdr.source);

    // Copy UDP destination port to buf (16 bits)
    memcpy (ptr, &udphdr.dest, sizeof (udphdr.dest));
    ptr += sizeof (udphdr.dest);
    chksumlen += sizeof (udphdr.dest);

    // Copy UDP length again to buf (16 bits)
    memcpy (ptr, &udphdr.len, sizeof (udphdr.len));
    ptr += sizeof (udphdr.len);
    chksumlen += sizeof (udphdr.len);

    // Copy UDP checksum to buf (16 bits)
    // Zero, since we don't know it yet
    *ptr = 0;
    ptr++;
    *ptr = 0;
    ptr++;
    chksumlen += 2;

    // Copy payload to buf
    memcpy (ptr, payload, payloadlen);
    ptr += payloadlen;
    chksumlen += payloadlen;

    // Pad to the next 16-bit boundary
    for (i=0; i<payloadlen%2; i++, ptr++) {
        *ptr = 0;
        ptr++;
        chksumlen++;
    }

    return checksum ((uint16_t *) buf, chksumlen);
}

/** 函数说明：  获得tid
 *  返回值：    当前线程的tid
 */
pid_t gettid(void)
{
	return syscall(SYS_gettid);
}

/** 函数说明：  计算校验和，按字节相加
 *  参数：
 *  @pData      数据指针
 *  @DataLen    数据长度
 *  返回值：    校验和
 */
uint32_t CheckSum(const void *pData, uint32_t DataLen)
{
    if (pData == NULL) {
        return 0;
    }
    const uint8_t *pV = (const uint8_t *)pData;
    uint32_t i=0, Sum=0;

    for (i=0; i<DataLen; i++) {
        Sum += pV[i];
    }

    return Sum;
}

/**	函数说明：	计算校验和取反：按字节相加，结果取反
 *	参数：
 *	@pData		数据指针
 *	@DataLen	数据长度
 *	返回值：	校验和取反
 */
uint32_t RvCheckSum(const void *pData, uint32_t DataLen)
{
	return ~CheckSum(pData, DataLen);
}

/** 函数说明：  获得对应时钟ID的时间
 *  返回值：    对应时钟的秒数（小数格式），失败返回-1
 */
double ClockTime(int ClockId)
{
    struct timespec t;

    if (clock_gettime(ClockId, &t) == 0) {
        return t.tv_sec+t.tv_nsec/1000000000.0;
    } else {
        return -1;
    }
}

/** 函数说明：  获得系统启动时间(CLOCK_BOOTTIME，否则CLOCK_MONOTONIC)
 *  返回值：    系统启动到现在的秒数（小数格式）
 */
double BooTime(void)
{
    double t = ClockTime(7);

    if (t < 0) {
        t = ClockTime(CLOCK_MONOTONIC);
    }

    return t;
}

/** 函数说明：  获得网络接口的MAC
 *  参数：
 *  @IfName     网络接口的名称
 *  @MAC        用于返回对应的MAC，请保证6个字节
 *  返回值：    成功返回0
 */
int GetMac(const char *IfName, uint8_t *MAC)
{
    if (IfName==NULL || MAC==NULL) {
        return -1;
    }
    int s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    struct ifreq ifr;

    if (s < 0) {
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", IfName);
    if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0) {
        return -1;
    }
    close(s);

    memcpy(MAC, ifr.ifr_hwaddr.sa_data, 6);

    return 0;
}

/** 函数说明：  获得网络接口的第一个IP
 *  参数：
 *  @IfName     网络接口的名称
 *  @pAddr      用于返回对应地址，其中pAddr->sa_family为地址类型，调用前请先设置，如IPV4为AF_INET
 *  返回值：    成功返回0
 */
int GetIP(const char *IfName, struct sockaddr *pAddr)
{
    int Ret = -1;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) != 0) {
        return -1;
    }

    for (ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next) {
        if (strcmp(ifa->ifa_name, IfName) != 0) {
            continue;
        }
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == pAddr->sa_family) {
            *pAddr = *(ifa->ifa_addr);
            Ret = 0;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return Ret;
}

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP4_HDRLEN 20  // IPv4 header length
#define UDP_HDRLEN  8  // UDP header length, excludes data

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
int DIYSendUdp(const char *interface, const void *data, int datalen, const char *SrcIP, int SrcPort, const char *DstIP, int DstPort)
{
    int status, frame_length, sd, bytes, ip_flags[4]= {0};
    struct ip iphdr;
    struct udphdr udphdr;
    uint8_t src_mac[6]= {0}, dst_mac[6]= {0}, *ether_frame;
    struct sockaddr_ll device;

    // Allocate memory for various arrays.
    ether_frame = alloca(IP_MAXPACKET);
    if (ether_frame == NULL) {
        return -1;
    }

    // Get MAC
    GetMac(interface, src_mac);

    // Find interface index from interface name and store index in struct sockaddr_ll device, which will be used as an argument of sendto().
    memset (&device, 0, sizeof (device));
    if ((device.sll_ifindex = if_nametoindex(interface)) == 0) {
        return -1;
    }

    // Set destination MAC address: you need to fill these out
    memset(dst_mac, 0xFF, 6);

    // Fill out sockaddr_ll.
    device.sll_family = AF_PACKET;
    memcpy (device.sll_addr, src_mac, 6 * sizeof (uint8_t));
    device.sll_halen = htons (6);
    device.sll_protocol = htons(ETH_P_IP);


    // IPv4 header

    // IPv4 header length (4 bits): Number of 32-bit words in header = 5
    iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);

    // Internet Protocol version (4 bits): IPv4
    iphdr.ip_v = 4;

    // Type of service (8 bits)
    iphdr.ip_tos = 0;

    // Total length of datagram (16 bits): IP header + UDP header + datalen
    iphdr.ip_len = htons (IP4_HDRLEN + UDP_HDRLEN + datalen);

    // ID sequence number (16 bits): unused, since single datagram
    //////iphdr.ip_id = htons (0x2e20+temp_i);
    iphdr.ip_id = htons(0);   //the ip_id can be constant or variable

    // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

    // Zero (1 bit)
    ip_flags[0] = 0;

    // Do not fragment flag (1 bit)
    ip_flags[1] = 1;

    // More fragments following flag (1 bit)
    ip_flags[2] = 0;

    // Fragmentation offset (13 bits)
    ip_flags[3] = 0;

    iphdr.ip_off = htons ((ip_flags[0] << 15)
                          + (ip_flags[1] << 14)
                          + (ip_flags[2] << 13)
                          +  ip_flags[3]);

    // Time-to-Live (8 bits): default to maximum value
    iphdr.ip_ttl = 64;

    // Transport layer protocol (8 bits): 17 for UDP
    iphdr.ip_p = IPPROTO_UDP;

    // Source IPv4 address (32 bits)
    if ((status = inet_pton (AF_INET, SrcIP, &(iphdr.ip_src))) != 1) {
        return -1;
    }

    // Destination IPv4 address (32 bits)
    if ((status = inet_pton (AF_INET, DstIP, &(iphdr.ip_dst))) != 1) {
        return -1;
    }

    // IPv4 header checksum (16 bits): set to 0 when calculating checksum
    iphdr.ip_sum = 0;
    iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

    // UDP header

    // Source port number (16 bits): pick a number
    udphdr.source = htons (SrcPort);

    // Destination port number (16 bits): pick a number
    udphdr.dest = htons (DstPort);

    // Length of UDP datagram (16 bits): UDP header + UDP data
    udphdr.len = htons (UDP_HDRLEN + datalen);

    // UDP checksum (16 bits)
    udphdr.check = udp4_checksum (iphdr, udphdr, data, datalen);

    // Fill out ethernet frame header.

    // Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + UDP header + UDP data)
    frame_length = 6 + 6 + 2 + IP4_HDRLEN + UDP_HDRLEN + datalen;

    // Destination and Source MAC addresses
    memcpy (ether_frame, dst_mac, 6 * sizeof (uint8_t));
    memcpy (ether_frame + 6, src_mac, 6 * sizeof (uint8_t));

    // Next is ethernet type code (ETH_P_IP for IPv4).
    // http://www.iana.org/assignments/ethernet-numbers
    ether_frame[12] = ETH_P_IP / 256;
    ether_frame[13] = ETH_P_IP % 256;

    // Next is ethernet frame data (IPv4 header + UDP header + UDP data).

    // IPv4 header
    memcpy (ether_frame + ETH_HDRLEN, &iphdr, IP4_HDRLEN * sizeof (uint8_t));

    // UDP header
    memcpy (ether_frame + ETH_HDRLEN + IP4_HDRLEN, &udphdr, UDP_HDRLEN * sizeof (uint8_t));

    // UDP data
    memcpy (ether_frame + ETH_HDRLEN + IP4_HDRLEN + UDP_HDRLEN, data, datalen * sizeof (uint8_t));

    // Submit request for a raw socket descriptor.
    if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
        return -1;
    }

    // Send ethernet frame to socket.
    bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device));

    // Close socket descriptor.
    close (sd);

    return bytes;
}

/** 函数说明：  16进制数据转字符串
 *  参数：
 *  @StrBuf     用来保存转换后字符串的指针，空间要能装下转换后的字符串（含'\0'）
 *  @Data       16进制数据
 *  @Len		16进制数据的长度（字节为单位）
 *  @Seg        字符串中显示字节间的间隔符，如果不可打印，则无间隔符
 *  返回值：    成功返回转换后字符串的长度，不包括'\0'
 */
int Hex2Str(char *StrBuf, const void *Data, uint32_t Len, char Seg)
{
    if (StrBuf==NULL) {
        return -1;
    } else if (Data==NULL || Len == 0) {
		StrBuf[0] = '\0';
		return 0;
	}
    uint32_t i=0, StrLen=0;
    const uint8_t *p=Data;

    for (i=0; i<Len-1; i++) {
        sprintf(StrBuf+StrLen, "%02X", p[i]);
        StrLen += 2;
        if (isprint(Seg)) {
            StrBuf[StrLen++] = Seg;
        }
    }
    sprintf(StrBuf+StrLen, "%02X", p[i]);
    StrLen += 2;

    return StrLen;
}

/** 函数说明：	字符串转16进制数据
 *  参数：
 *  @Str		需要转换的字符串，以'\0'结尾，可以识别的16进制数据0-9、a-f、A-F
 *  @HexBuf		用来保存16进制数据的buf，请保证长度
 *  返回值：	返回成功转换的16进制数据的字节数（不足一个字节的部分不转换）,出错返回-1
 */
int Str2Hex(const char *Str, uint8_t *HexBuf)
{
	if (Str==NULL || HexBuf==NULL) {
		return -1;
	}
	int HexNum=0;
	uint8_t h=0, l=0;

	while ((h=Str[HexNum*2]) && (l=Str[HexNum*2+1])) {
		if (h>='0' && h<='9') {
			h -= '0';
		} else if (h>='a' && h<='f') {
			h -= ('a'-10);
		} else if (h>='A' && h<='F') {
			h -= ('A'-10);
		} else {
			break;
		}
		if (l>='0' && l<='9') {
			l -= '0';
		} else if (l>='a' && l<='f') {
			l -= ('a'-10);
		} else if (l>='A' && l<='F') {
			l -= ('A'-10);
		} else {
			break;
		}
		HexBuf[HexNum++] = h*16+l;
	}

	return HexNum;
}

/** 函数说明：  IPV4的ping功能
 *  参数：
 *  @Node     	ping的地址，可以是网址
*	@Size		承载数据长度，同ping -s的参数
 *  @TimeOut	超时，单位秒
 *  返回值：	成功返回0，其他为失败
 */
int Ping4(char *Node, uint8_t Size, double TimeOut)
{
	if (Node == NULL) {
		return -1;
	}
	if (TimeOut <= 0) {
		TimeOut = DEF_TIMEOUT;
	}
	double st = BooTime();
	int sockfd=-1, Ret=-1;
	char sendpacket[PACKET_SIZE] = {0};
	struct ip *iph = NULL;
	struct icmp *icmp = (struct icmp*)sendpacket;

	// 准备发送数据
	icmp->icmp_type	=ICMP_ECHO;
	icmp->icmp_code	=0;
	icmp->icmp_cksum=0;
	icmp->icmp_id	=getpid(); // 用PID作为Ping的Sequence ID
	icmp->icmp_seq	=1;
	time((time_t *)icmp->icmp_dun.id_data);
	icmp->icmp_cksum=checksum((uint16_t *)icmp, ICMP_MINLEN+Size);

	// 建立socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd > 0) {
		// 建立成功
		struct timeval timeo = {0};
		struct addrinfo hints = {0}, *result=NULL, *rp=NULL;

		// 设定TimeOut时间
		timeo.tv_sec	= TimeOut;
		timeo.tv_usec	= (uint32_t)(TimeOut*1000) % 1000;
		//不管成功与否，失败也就算了
		setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));

		// 设定Ip过滤条件
		hints.ai_family		= AF_INET;		// IPv4
		hints.ai_socktype	= SOCK_RAW;		// 过滤一下，否则会有多个

		// 获取地址信息
		if (getaddrinfo(Node, NULL, &hints, &result) == 0) {
			// 获取成功
			for (rp=result; (rp!=NULL && Ret!=0); rp=rp->ai_next) {
//				printf("N=%s, IP=%s\n", Node, inet_ntoa(((struct sockaddr_in*)rp->ai_addr)->sin_addr));
				// 发送
				if (sendto(sockfd, &sendpacket, ICMP_MINLEN+Size, 0, rp->ai_addr, rp->ai_addrlen) < 1) {
					continue;
				}
				double dt;
				while((dt = st+TimeOut-BooTime()) >= 0) {
					int maxfds = 0;
					fd_set readfds;
					uint8_t recvpacket[PACKET_SIZE] = {0};

					FD_ZERO(&readfds);
					FD_SET(sockfd, &readfds);

					maxfds 			= sockfd + 1;
					timeo.tv_sec	= dt;
					timeo.tv_usec	= (uint32_t)(dt*1000) % 1000;
					if (select(maxfds, &readfds, NULL, NULL, &timeo) <= 0) {
						break;
					}

					// 接收
					socklen_t fromlen = sizeof(struct sockaddr);
					struct sockaddr from = {0};

					if (recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, &from, &fromlen)>1 &&
					    memcmp(&(((struct sockaddr_in*)&from)->sin_addr),&(((struct sockaddr_in*)rp->ai_addr)->sin_addr), sizeof(struct in_addr))==0) {

						iph = (struct ip *)recvpacket;

						icmp=(struct icmp *)(recvpacket + (iph->ip_hl<<2));
						// 判断Ping回复包的状态
						if (icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == getpid()) {
							// 正常就退出循环
							Ret = 0;
							break;
						}
					}
				}
			}
			//清理
			freeaddrinfo(result);
		}
		close(sockfd);
	}

	return Ret;
}
