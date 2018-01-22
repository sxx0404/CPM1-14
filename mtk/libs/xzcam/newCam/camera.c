#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <net/if.h>
#include <string.h>
#include <netinet/in.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ifaddrs.h>
//#include "../../../include/common.h"

#define BIND_IF		"eth0"
#define BIND_PORT	9800

#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐
struct FindPacket {
	uint32_t cmd;
	uint32_t id;
	uint32_t flag;
	uint32_t ip;
	uint16_t port;
	uint16_t rev1;
	uint8_t rev2[24];
};

#pragma pack(pop)               // 恢复原对齐方式
static int bsock = -1;
static int sock = -1;
static struct FindPacket findpk= {0};
static uint32_t LocalIp = 0;
static uint32_t RemoteIp = 0;
static uint16_t RemotePort = 0;
static uint16_t LocalPort = 0;


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

/************************************************************
通过这个广播可以在局域网中发现摄像机设备
如果设备收到这个广播，会对发送广播的主机回应这个数据包
数据包里面包括了摄像机的IP地址和端口号
recvfrom(sock,(char*)&findpk,sizeof(findpk),0,(SOCKADDR*)&addr,&addlens);
其中ip地址直接取recvfrom 后面的地址，端口号取包里面的port
************************************************************/
void BroadCast()
{
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	addr.sin_port = htons(BIND_PORT);

	struct FindPacket packet;
	packet.cmd = 0x55AA0801;
	packet.ip  = LocalIp;
	packet.port = LocalPort;

	if(sendto(bsock, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Failed to send to INADDR_BROADCAST");
	}
}

int RecvFined()
{
	socklen_t addlens = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	/*这个地方是UDP socket*/
	int lens = recvfrom(bsock,&findpk,sizeof(findpk), MSG_DONTWAIT,(struct sockaddr*)&addr,&addlens);
	if( lens == sizeof(findpk) ) {
		if( findpk.cmd == 0x55AA0801 ) {
			printf("Cam info:%s:%hu\n", inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr), ntohs(findpk.port));
			if (((struct sockaddr_in*)&addr)->sin_addr.s_addr!=LocalIp && RemoteIp == 0) {
				RemoteIp = ((struct sockaddr_in*)&addr)->sin_addr.s_addr;
				RemotePort = findpk.port;
			}
			//摄像机IP和端口号
			//addr.sin_addr.s_addr
			//findpk.port 端口号
		}
		return 0;
	}

	return -1;
}

/*****************************************************/
/*以下是tcp链接,全都是一个连接 sock*/
typedef uint32_t u32;
typedef uint16_t u16;
#pragma pack(push)
#pragma pack(1)
//按照字节对齐
struct COMMAND {
	u32 sync;
	u32 cmd;
	u16 video_w;
	u16 video_h;
	u32 bitrate;
	u32 seconds;
	u32 date;
	u32 rec_type;
	u16 fps;
	u16 gop;
	u16 max_qp;
	u16 min_qp;//6*5 30
	char filename[128];
	u32 rev[10];
};
#pragma pack(pop)
enum {
	CMD_STOP_RECORD = 0x0200,
	CMD_START_RECORD = 0x0201,

	CMD_STOP_REALVIDEO = 0x0300,
	CMD_START_REALVIDEO = 0x0301,
};

/*连接摄像机*/
int ConnectServer(void)
{
	struct sockaddr_in serverAddr;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock < 0 ) {
		return -1;
	}
	printf("RemoteInfo:%X, %hu\n", RemoteIp, RemotePort);
	serverAddr.sin_addr.s_addr = RemoteIp;//inet_addr("127.0.0.1");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = RemotePort;
	return connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

}

/*开启实时视频*/
int StartRealVideo()
{

	struct COMMAND cmd;
	cmd.sync = 0x55AA0801;
	cmd.cmd = CMD_START_REALVIDEO;

	cmd.fps			= 25;
	cmd.bitrate		= 250;
	cmd.gop			= 60;
	cmd.max_qp		= 38;
	cmd.min_qp		= 26;
	cmd.rec_type	= 2;
	cmd.video_w		= 1280;
	cmd.video_h		= 720;//必须是16的整数倍

	return send(sock,(char*)&cmd,sizeof(cmd),0)==sizeof(cmd)?0:-1;
}

/*录像命令，存储为文件*/
int StartRecord()
{
	struct COMMAND cmd;
	//nTSeconds +=  8*
	cmd.sync		= 0x55AA0801;
	cmd.cmd			= CMD_START_RECORD;
	cmd.seconds		= 15;/*录像时间长度*/
	cmd.date		= time(NULL);/*当前时间，总秒数 1970以来*/

	cmd.fps			= 25;
	cmd.bitrate		= 250;
	cmd.gop			= 60;
	cmd.max_qp		= 36;
	cmd.min_qp		= 26;
	cmd.rec_type	= 2;
	cmd.video_w		= 1280;
	cmd.video_h		= 720;//必须是16的整数倍

	snprintf(cmd.filename, 128, "A-%ld-0.h264", time(NULL));

	return send(sock,(char*)&cmd,sizeof(cmd),0)==sizeof(cmd)?0:-1;
}


int StopRealVideo()
{

	struct COMMAND cmd;
	cmd.sync = 0x55AA0801;
	cmd.cmd = CMD_STOP_REALVIDEO;
	return send(sock,(char*)&cmd,sizeof(cmd),0)==sizeof(cmd)?0:-1;
}

int StopRecord()
{
	struct COMMAND cmd;
	cmd.sync = 0x55AA0801;
	cmd.cmd = CMD_STOP_RECORD;
	return send(sock,(char*)&cmd,sizeof(cmd),0)==sizeof(cmd)?0:-1;
}




/*获取实时流：
发送StartRealVideo，然后调用下面获取流函数
*/

int GetVideoStream()
{
	char buffer[512];
	char stream[1024*1024];
	int sync55_cnt = 0;
	int recvlens = -1, i=0;

	while(1) {
		/*同步数据*/
		recvlens = recv(sock,buffer,512,0);
		if( recvlens <= 0 ) {
			continue;
		}
		for(i=0; i<recvlens; i++) {
			if( buffer[i] != 0x55 ) {
				sync55_cnt = 0;
			} else {
				sync55_cnt++;
				if( sync55_cnt == 128 ) {
					i++;
					goto end;
				}
			}
		}
	}

end:

	if( sync55_cnt == 128 ) {
		if( i < recvlens ) {
			/*这个地方有一部分stream,必须存储*/
			memcpy(stream,buffer+i,recvlens-i);
		}
	}

	double st = BooTime();
	char FlName[100] = {0};

	snprintf(FlName, 100, "/tmp/%ld.h264", time(NULL));
	int fd = open(FlName, O_CREAT|O_RDWR|O_TRUNC, 0777);
	if (fd < 0) {
		perror("Failed open file");
		return -1;
	}
	while(st+10 > BooTime()) {
		/*获取h264流*/
		recvlens = recv(sock,stream,1024*1024,0);
		if (recvlens <= 0) {
			perror("Failed to recv");
			break;
		} else {
			write(fd, stream, recvlens);
		}
	}

	return close(fd);
}

int main(int argc, const char *argv[])
{
	if (argc == 2) {
		switch(argv[1][0]) {
		default:
			break;
		case 's':
		case 'r':
		case 'v': {
			struct sockaddr laddr = {0};

			laddr.sa_family = AF_INET;
			if (GetIP(BIND_IF, &laddr)) {
				perror("Failed to GetIP");
				return -1;
			}
			LocalIp = ((struct sockaddr_in*)&laddr)->sin_addr.s_addr;
			LocalPort = htons(BIND_PORT);
			bsock = socket(AF_INET, SOCK_DGRAM, 0);
			if (bsock < 0) {
				perror("Failed to socket");
				return -1;
			}
			struct ifreq i={{{0}}};

			strncpy(i.ifr_ifrn.ifrn_name, BIND_IF, IFNAMSIZ);
			if (setsockopt(bsock, SOL_SOCKET, SO_BINDTODEVICE,
						   &i, sizeof(struct ifreq))) {
				perror("SO_BINDTODEVICE failed");
			}
			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(BIND_PORT);

			if (bind(bsock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
				perror("bind port");
			}
			int v=1;
			if (setsockopt(bsock, SOL_SOCKET, SO_BROADCAST, &v, sizeof(int))) {
				perror("setsockopt SO_BROADCAST");
			}
			BroadCast();
			printf("BroadCasted\n");
			sleep(3);
			while (!RecvFined()) {
				usleep(1000000);
			}
			if (argv[1][0] == 'r') {
				printf("StartRecording\n");

				if (ConnectServer()) {
					perror("Failed to Connet");
				}
				if (StartRecord()) {
					perror("Failed to StartRecord");
				}
				sleep(10);
				if (StopRecord()) {
					perror("Failed to StopRecord");
				}
			} else if (argv[1][0] == 'v') {
				printf("StartRealVideoing\n");

				if (ConnectServer()) {
					perror("Failed to Connet");
				}
				if (StartRealVideo()) {
					perror("Failed to StartRealVideo");
				}
				if (GetVideoStream()) {
					perror("Failed to GetVideoStream");
				}
				if (StopRealVideo()) {
					perror("Failed to StopRealVideo");
				}

			}
			break;
		}
		}
	} else {
		printf("Usage:%s [s|r|v]\n", argv[0]);
	}

	return 0;
}
