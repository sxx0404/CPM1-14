#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#include "XzCamApi.h"
#include "../common/common.h"

#define MAX_FILENUM         131072

// 指令方向
enum {
    CLI_TO_HST = 0x80,
    HST_TO_CLI = 0x00,
};

// 指令字
enum {
    CMD_TEST_DEVICE         = 0x00,
    CMD_READ_VIDEO          = 0x01,
    CMD_START_RECORD        = 0x02,
    CMD_STOP_RECORD         = 0x03,
    CMD_DEVICE_SEARCH       = 0x04,
    CMD_DEVICE_CFG          = 0x05,
    CMD_EXIT_TOKEN          = 0x06,
    CMD_FORMAT              = 0x07,
    CMD_READ_FILE           = 0x08,
    CMD_GET_IP              = 0x09,
    CMD_ACK_READFILE        = 0x0A,
    CMD_STOP_READ           = 0x0B,
    CMD_READ_LST            = 0x0C,
    CMD_ACK_READLST         = 0x0D,
    CMD_NOW_VIDEO           = 0x0F,
    CMD_IPCONFIG            = 0x7A,
    CMD_ACK_IPCONFIG        = 0x7B,
    CMD_SET_SENSOR_PARAM    = 0x7F,
};

//Read返回状态
enum {
    ACK_ASME_IDX            = 0,
    ACK_READ_OK             = 1,
    ACK_FILE_NOT_EXIST      = 2,
    ACK_FILE_OPEN_ERR       = 3,
};

#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐

typedef struct XzCamFileI {
    uint8_t     RecodType;      // 录像时传递的类型
    uint8_t     Lens[3];        // 该录像的长度，单位512B
    uint32_t    Time;           // 录像时传递的日期参数
} sXzCamFileI;                    // 8B

// 公共头部
typedef struct XzCamFrHd {
    uint8_t     Cmd;            // 指令字
    uint16_t    Len;            // 数据帧总长度，含指令字和校验
    uint16_t    Index;          // 指令序号
} sXzCamFrHd;

// 摄像头IP MAC 主机地址和端口的设置包
typedef struct XzCamFrIpcfg {
    sXzCamFrHd  Head;
    uint8_t     HostMac[6];     // 主机的MAC，如果全FF,摄像机会解析指令包的源MAC作为主机MAC
    uint32_t    HostIp;         // 摄像头应答时发送的目标IP，格式同inet_addr的返回值
    uint16_t    HostPort;       // 摄像头应答时发送的目标端口，格式同htons的返回值
    uint8_t     CamMac[6];      // 设置的摄像头MAC
    uint32_t    CamIp;          // 设置的摄像头IP，如果全FF，摄像头的IP和MAC不修改，格式同inet_addr的返回值
    uint16_t    Sum;
} sXzCamFrIpcfg;

//
typedef struct XzCamFrDevtest {
    sXzCamFrHd  Head;
    sXzCamStat  Stat;
    uint8_t     SensorType;
    uint32_t    TotalSectors;
    uint32_t    FileNum;
    uint32_t    MinFileId;
    uint32_t    MaxFileId;
    uint32_t    MinDate;
    uint32_t    MaxDate;
    uint8_t     Mac[6];
    uint16_t    Sum;
} sXzCamFrDevtest;

typedef struct XzCamFrStartRec {
    sXzCamFrHd  Head;
    uint8_t     RecType;            // 录像类型
    uint32_t    RecTime;               // 录像的时间标签
    uint16_t    RecLen;             // 录像的长度，单位秒
    struct {
        uint8_t     op;             // 未知，取0
        uint8_t     lv;             // 清晰度？取0x24
        uint8_t     fps;            // 未知，取0
        uint8_t     gop;            // 未知，取24
        uint16_t    w;              // 宽度？取640
        uint16_t    h;              // 高度？取480
    }           Param;              // 录像的一些参数
    uint16_t    Sum;
} sXzCamFrStartRec;

//支持停止录像，格式化
typedef struct XzCamFrSimple {
    sXzCamFrHd  Head;
    uint16_t    Sum;
} sXzCamFrSimple;

typedef struct XzCamFrRdAck {
    sXzCamFrHd  Head;
    uint32_t    rmlens;         // 未知作用
    uint16_t    Tid;            // 另一个指令序号，
    uint8_t     RdStat;         // 读指令的返回状态
    uint16_t    Sum;
} sXzCamFrRdAck;

typedef struct XzCamFrRdList {
    sXzCamFrHd  Head;
    uint8_t     All;            // 是否读取所有
    uint16_t    Tid;            // 另一个指令序号
    uint32_t    StartFid;       // 读取列表的起始文件ID
    uint32_t    Num;            // 读取的数量，读取所有时为MAX_FILENUM，读一次为128
    uint16_t    Sum;
} sXzCamFrRdList;

typedef struct XzCamFrListHd {
    sXzCamFrHd  Head;
    uint8_t     Num;            // 该帧含有的节点数
    uint32_t    StartFid;       // 该帧的起始文件ID
    uint8_t     sts;            // 未知作用
    uint16_t    Sum;
} sXzCamFrListHd;

typedef struct XzCamFrRdFile {
    sXzCamFrHd  Head;
    uint32_t    AddrPos;        // 本包数据在本文件中的偏移地址，单位512B
    uint32_t    FileId;         // 文件的ID
    uint16_t    Len;            // 读取的长度，单位512B
    uint16_t    Tid;            // 另一个指令序号
    uint16_t    Sum;
} sXzCamFrRdFile;

typedef struct XzCamFrFileHd {
    sXzCamFrHd  Head;
    uint32_t    AddrPos;        // 本包数据在本文件中的偏移地址，单位512B
    uint8_t     sts;            // 未知作用
    uint16_t    Sum;
} sXzCamFrFileHd;

#pragma pack(pop)               //恢复原对齐方式

static uint32_t ConvLen(const uint8_t *pLen)
{
    return pLen[0]*65536 + pLen[1]*256 + pLen[2];
}

static int FindHole(uint8_t *pTable, int Len)
{
    int i;

    for (i=0; i<Len; i++) {
        if (pTable[i] == 0) {
            return i;
        }
    }

    return -1;
}

static int CheckFrame(void *pFrame, int ExpCmd, int RetLen)
{
    if (pFrame==NULL || RetLen<=0) {
        return -1;
    }
    uint8_t *pData  = pFrame;
    sXzCamFrHd *pHead = pFrame;
    uint16_t FrameLen = ntohs(pHead->Len);

    if (pHead->Cmd != ExpCmd) {
        return -1;
    }
    if (FrameLen > RetLen) {
        return -1;
    }
    if ((uint16_t)CheckSum(pFrame, FrameLen-2) != (pData[FrameLen - 1]*256 + pData[FrameLen - 2])) {
        return -1;
    }

    return 0;
}

static void PackFrame(void *pFrame, int Cmd, int Len, int Index)
{
    if (pFrame==NULL || Len<5) {
        return;
    }
    uint8_t *pData = (uint8_t *)pFrame;
    uint16_t Sum;
    sXzCamFrHd *pHead = pFrame;

    pHead->Cmd      = Cmd;
    pHead->Len      = htons(Len);
    pHead->Index    = htons(Index);

    Sum = CheckSum(pFrame, Len-2);
    pData[Len-2]    = Sum%256;
    pData[Len-1]    = Sum/256;
}

static int ClrRcv(int Sock, double TimeOut)
{
    uint8_t tBuf[2048];
    double St = BooTime();

    while(recvfrom(Sock, tBuf, 2048, MSG_DONTWAIT, NULL, 0) > 0) {
        if (TimeOut>=0 && St+TimeOut > BooTime()) {
            return -1;
        }
    }

    return 0;
}

/** 函数说明：  初始化摄像头主机信息结构体（结构体清空，并初始化UDP套接字）
 *  参数：
 *  @pHost      主机结构体的指针
 *  @Port       用于和摄像头通讯的端口
 *  返回值：    成功返回0
 */
int XzCamInitHost(sXzCamHost *pHost, uint16_t Port)
{
    if (pHost == NULL) {
        return -1;
    }
    int HostSock=-1, opt=1;
    struct timeval timeout = {1, 0};
    struct sockaddr_in LocalA = {
        .sin_family        = AF_INET,
        .sin_port          = htons(Port),
        .sin_addr.s_addr   = INADDR_ANY
    };

    //建socket
    HostSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (HostSock < 0) {
        return -1;
    }

    //设置一些属性
    if(setsockopt(HostSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==0 &&
       setsockopt(HostSock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))==0) {
        //绑定到对应端口
        if (bind(HostSock, (struct sockaddr*)&LocalA, sizeof(struct sockaddr_in)) == 0) {
            memset(pHost, 0, sizeof(sXzCamHost));
            pHost->HostPort = Port;
            pHost->HostSock = HostSock;

            return 0;
        }
    }

    close(HostSock);
    return -1;
}

/** 函数说明：  查找指定网卡上的所有摄像头，并将这些摄像头的信息保存(添加)在主机结构体中
 *  参数：
 *  @pHost      主机结构体的指针
 *	@IfName		网卡名
 *	@Type		0找到的结果替换原pHost中的信息，1添加到原pHost中的信息之后
 *  返回值：    返回找到的摄像头数量
 */
int XzCamSearchCams(sXzCamHost *pHost, const char* IfName, int Type)
{
    if (pHost==NULL || IfName==NULL) {
        return -1;
    }
    int CamNum=0, Ret=0;
    sXzCamFrIpcfg tCfg = {{0}};
    uint8_t tData[1024] = {0};
    struct sockaddr HostAddr;
    struct sockaddr_in CameraA = {0}, *pAddr = (struct sockaddr_in *)&HostAddr;
    socklen_t AddrLen = sizeof(struct sockaddr);
    sXzCamInfo *pCameras = NULL;

    //获取主机IP
    HostAddr.sa_family = AF_INET;
    if (GetIP(IfName, &HostAddr) != 0) {
        return -1;
    }

    //组发送包
    memset(&(tCfg.CamIp), 0xFF, 4);
    memset(&(tCfg.HostMac), 0xFF, 6);
    tCfg.HostIp     = pAddr->sin_addr.s_addr;
    tCfg.HostPort   = htons(pHost->HostPort);
    //打包
    PackFrame(&tCfg, CMD_IPCONFIG, sizeof(sXzCamFrIpcfg), 0);

    //发送一条广播帧
    Ret = DIYSendUdp(IfName, &tCfg, sizeof(sXzCamFrIpcfg), inet_ntoa(pAddr->sin_addr), pHost->HostPort, "255.255.255.255", pHost->HostPort);
    if (Ret <= 0) {
        return -1;
    }
    while (1) {
        Ret = recvfrom(pHost->HostSock, tData, 1024, 0, (struct sockaddr*)&CameraA, &AddrLen);
        //接收失败--
        if (Ret < 5) {
            break;
        }

        sXzCamFrIpcfg *pFrame = (sXzCamFrIpcfg *)tData;
        struct sockaddr_in *pAddr;

        if (CheckFrame(tData, CMD_ACK_IPCONFIG|CLI_TO_HST, Ret) < 0) {  //基本校验
            continue;
        }
        if (pFrame->CamIp != CameraA.sin_addr.s_addr) {                 //接收包的发送IP和数据帧中的IP不符
            continue;
        }

        //准备空间
        void *p = realloc(pCameras, (CamNum + 1)*sizeof(sXzCamInfo));
        if (p == NULL) {
            p = realloc(pCameras, (CamNum + 1)*sizeof(sXzCamInfo));
            if (p == NULL) {
                break;
            }
        }
        pCameras = p;
        memset(pCameras+CamNum, 0, sizeof(sXzCamInfo));
        //复制数据
        pAddr = &(pCameras[CamNum].Addr);
        pAddr->sin_family       = AF_INET;
        pAddr->sin_addr.s_addr  = pFrame->CamIp;
        pAddr->sin_port         = htons(pHost->HostPort);
        memcpy(pCameras[CamNum].Mac, pFrame->CamMac, 6);
        CamNum++;

        //数据准备好，准备recv
        AddrLen = sizeof(struct sockaddr_in);
        memset(&CameraA, 0, AddrLen);
    }
    if (Type) {
		void *p = realloc(pHost->pCams, (pHost->CamNum+CamNum)*sizeof(sXzCamInfo));
		if (p) {
			pHost->pCams = p;
			memcpy(pHost->pCams+pHost->CamNum, pCameras, CamNum*sizeof(sXzCamInfo));
			pHost->CamNum += CamNum;
		} else {
			CamNum = -1;
		}
		free(pCameras);
    } else {
		free(pHost->pCams);
		pHost->pCams    = pCameras;
		pHost->CamNum   = CamNum;
    }

    return CamNum;
}

/** 函数说明：  获取指定摄像头的状态，并更新到其信息结构体的变量中（更新状态、录像文件数，最大最小文件ID，最大最小时间）
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  返回值：    成功返回0
 */
int XzCamGetStat(sXzCamHost *pHost, int CamIndex)
{
    if (pHost==NULL || CamIndex<0 || CamIndex>=pHost->CamNum) {
        return -1;
    }
    int Ret=-1;
    uint8_t tBuf[2048] = {0};
    sXzCamFrDevtest tTest = {{0}}, *pTestAck=(sXzCamFrDevtest *)tBuf;
    sXzCamInfo *pCam = pHost->pCams+CamIndex;

    PackFrame(&tTest, CMD_TEST_DEVICE, sizeof(sXzCamFrDevtest), pCam->CmdIndex++);

    ClrRcv(pHost->HostSock, 1);
    Ret = sendto(pHost->HostSock, &tTest, sizeof(sXzCamFrDevtest), 0, (struct sockaddr *)&(pCam->Addr), sizeof(struct sockaddr_in));
    if (Ret != sizeof(sXzCamFrDevtest)) {
        return -1;
    }
    Ret = recvfrom(pHost->HostSock, tBuf, 2048, 0, NULL, 0);
    if (Ret <= 5) {
        return -1;
    }
    //校验
    if (CheckFrame(tBuf, CMD_TEST_DEVICE|CLI_TO_HST, Ret) < 0) {
        return -1;
    }
    pCam->Stat.sStat= pTestAck->Stat;
    pCam->RdFileNum = ntohl(pTestAck->FileNum);
    pCam->MinFileId = ntohl(pTestAck->MinFileId);
    pCam->MaxFileId = ntohl(pTestAck->MaxFileId);
    pCam->MinDate   = ntohl(pTestAck->MinDate);
    pCam->MaxDate   = ntohl(pTestAck->MaxDate);

    return 0;
}

/** 函数说明：  设置摄像头的IP
 *  参数：
 *  @pHost      主机结构体的指针
 *	@IfName		网卡名
 *  @Ip         摄像头设置后的IP
 *  @Mac        摄像头设置后的MAC
 *  返回值：    成功返回0
 *  备注：     仅适用只有1个摄像头的情况，其他情况请勿适用
 */
int XzCamSetAddr(sXzCamHost *pHost, const char* IfName, const char *Ip, const uint8_t *Mac)
{
    if (pHost==NULL || Ip==NULL || Mac==NULL) {
        return -1;
    }
    int Ret=0, ErrCnt=0;
    sXzCamFrIpcfg tCfg = {{0}};
    uint8_t tData[1024] = {0};
    struct sockaddr HostAddr;
    struct sockaddr_in CameraA = {0}, *pAddr = (struct sockaddr_in *)&HostAddr;
    socklen_t AddrLen = sizeof(struct sockaddr);

    //获取主机IP
    HostAddr.sa_family = AF_INET;
    if (GetIP(IfName, &HostAddr) != 0) {
        return -1;
    }
    //组发送包
    tCfg.CamIp      = inet_addr(Ip);
    memcpy(tCfg.CamMac, Mac, 6);
    memset(&(tCfg.HostMac), 0xFF, 6);
    tCfg.HostIp     = pAddr->sin_addr.s_addr;
    tCfg.HostPort   = htons(pHost->HostPort);
    //打包
    PackFrame(&tCfg, CMD_IPCONFIG, sizeof(sXzCamFrIpcfg), 0);

    //发送一条广播帧
    Ret = DIYSendUdp(IfName, &tCfg, sizeof(sXzCamFrIpcfg), inet_ntoa(pAddr->sin_addr), pHost->HostPort, "255.255.255.255", pHost->HostPort);
    if (Ret <= 0) {
        return -1;
    }
    while (1) {
        Ret = recvfrom(pHost->HostSock, tData, 1024, 0, (struct sockaddr*)&CameraA, &AddrLen);
        //接收失败--
        if (Ret < 5) {
            if (++ErrCnt > 2) {
                Ret = -1;
                break;
            }
            continue;
        }
        ErrCnt = 0;

        sXzCamFrIpcfg *pFrame = (sXzCamFrIpcfg *)tData;

        if (CheckFrame(tData, CMD_ACK_IPCONFIG|CLI_TO_HST, Ret) < 0) {  //基本校验
            continue;
        }
        if (pFrame->CamIp != CameraA.sin_addr.s_addr) {                 //接收包的发送IP和数据帧中的IP不符
            continue;
        }
        Ret = 0;
        break;
    }

    return Ret;
}

/** 函数说明：  获取指定摄像头的文件信息列表，并更新到其信息结构体的变量中
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  返回值：    成功返回0
 */
int XzCamGetFileList(sXzCamHost *pHost, int CamIndex)
{
    if (pHost==NULL || CamIndex<0 || CamIndex>=pHost->CamNum) {
        return -1;
    }
    int Ret=0, ErrCnt=0, SendFlag=1, RecvFlag=0;
    uint8_t tBuf[2048] = {0}, DataTable[MAX_FILENUM/128] = {0};
    sXzCamInfo *pCam = pHost->pCams+CamIndex;
    uint32_t InfoNum=0;
    sXzCamFrRdList SendFrame = {{0}};
    sXzCamFileInfo *pInfo = NULL;

    //组包
    memset(&SendFrame, 0, sizeof(sXzCamFrRdList));
    SendFrame.All       = 1;
    SendFrame.Tid       = htons(pCam->CmdIndex);
    SendFrame.StartFid  = 0;
    SendFrame.Num       = htonl(MAX_FILENUM);

    //开始读取--
    while (ErrCnt < 5) {  //每次读1024B共128条信息
        //发送
        if (SendFlag) {
            //打包
            PackFrame(&SendFrame, CMD_READ_LST, sizeof(sXzCamFrRdList), pCam->CmdIndex++);
            //发送
            ClrRcv(pHost->HostSock, 0.3);
            Ret = sendto(pHost->HostSock, &SendFrame, sizeof(sXzCamFrRdList), 0, (struct sockaddr *)&(pCam->Addr), sizeof(struct sockaddr_in));
//printf("%0.13G Send ret=%d, Data:", BooTime(), Ret);
//printHex((uint8_t*)&SendFrame, Ret);
            if (Ret != sizeof(sXzCamFrRdList)) {
                return -1;
            }
        }
        SendFlag = 0;

        //接收
        Ret = recvfrom(pHost->HostSock, tBuf, 2048, RecvFlag, NULL, 0);
//printf("%.13G Recv ret=%d, Data:", BooTime(), Ret);
//Ret>0?printHex(tBuf, 12):printf("\n");
        RecvFlag = 0;
        //不够解析
        if (Ret < 5) {
            //判断是否接收完成
            int HoleI = FindHole(DataTable, MAX_FILENUM/128);
//printf("HoleI=%d\n", HoleI);
            if (HoleI < 0) {
                ErrCnt = 0;
                break;
            }
            ErrCnt++;

            //尝试重发
            SendFrame.Tid       = htons(pCam->CmdIndex);
            SendFrame.All       = 0;
            SendFrame.StartFid  = htonl(HoleI*128);
            SendFrame.Num       = htonl(128);
            SendFlag            = 1;
            continue;
        }
        //开始解析
        if (tBuf[0] == (CLI_TO_HST|CMD_READ_LST)) {         //含有文件信息的数据包
            sXzCamFrListHd *pHead = (sXzCamFrListHd *)tBuf;

            //校验
            if (Ret < sizeof(sXzCamFrListHd)+1026) {          // 长度
                continue;
            }
            if (ntohl(pHead->StartFid) >= MAX_FILENUM) {
                continue;
            }
            if ((uint16_t)CheckSum(tBuf, sizeof(sXzCamFrListHd)+1024) !=
                tBuf[sizeof(sXzCamFrListHd)+1025]+tBuf[sizeof(sXzCamFrListHd)+1024]*256) {
                //校验码
                continue;
            }
            ErrCnt = 0;
            // 拷贝数据
            int i=0;
            sXzCamFileI *pFiles = (sXzCamFileI *)(tBuf+sizeof(sXzCamFrListHd));       //新数据指针
            void *p = realloc(pInfo, (InfoNum+128)*sizeof(sXzCamFileInfo));       //准备一下空间

            if (p == NULL) {    //空间申请失败
                free(pInfo);
                return -1;
            }
            pInfo = p;

            for (i=0; i<128; i++) {
                if (ConvLen(pFiles[i].Lens) > 0) {       //有实际长度
                    pInfo[InfoNum].FileIndex= ntohl(pHead->StartFid)+i;
                    pInfo[InfoNum].RecodType= pFiles[i].RecodType;
                    pInfo[InfoNum].Time     = ntohl(pFiles[i].Time);
                    pInfo[InfoNum].Len      = ConvLen(pFiles[i].Lens);
                    InfoNum++;
                }
            }
            //记录该点已读取
            DataTable[ntohl(pHead->StartFid)/128] = 1;
        } else if (CheckFrame(tBuf, CLI_TO_HST|CMD_ACK_READLST, Ret) == 0) {
            sXzCamFrRdAck *pHead = (sXzCamFrRdAck *)tBuf;

            //printf("rdstat=%d\n", tBuf[11]);
            if (pHead->Tid == SendFrame.Tid) {
                switch (pHead->RdStat) {
                case ACK_ASME_IDX:      //指令重复需要重发
                    SendFrame.Tid   = htons(pCam->CmdIndex);
                    SendFlag        = 1;
                    break;
                default:
                    ErrCnt++;
                    RecvFlag        = MSG_DONTWAIT;
                    break;
                }
            }
        }
    }
    if (ErrCnt < 3) {
        void *p=realloc(pCam->FileList, InfoNum * sizeof(sXzCamFileInfo));
        if (p) {
            pCam->FileList = p;
            memcpy(pCam->FileList, pInfo, InfoNum * sizeof(sXzCamFileInfo));
            pCam->RlFileNum = InfoNum;

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}
/*按长度去读取列表，逻辑未完善，暂时废除
int XzCamGetFileList(sXzCamHost *pHost, int CamIndex)
{
    if (pHost==NULL || CamIndex<0 || CamIndex>=pHost->CamNum) {
        return -1;
    }
    int Ret=0, ErrCnt=0, SendFlag=1, RecvFlag=0;
    uint8_t tBuf[2048] = {0};
    sXzCamInfo *pCam = pHost->pCams+CamIndex;
    uint32_t StartPos=0, PacketNum=(pCam->RdFileNum+127)/128;
    sXzCamFrRdList SendFrame = {{0}};
    sXzCamFileInfo tInfo[PacketNum*128];

    // 可变空间先清空
    memset(tInfo, 0, PacketNum*128*sizeof(sXzCamFileInfo));

    //组包
    memset(&SendFrame, 0, sizeof(sXzCamFrRdList));
    SendFrame.All       = 0;
    SendFrame.Tid       = htons(pCam->CmdIndex);
    SendFrame.StartFid  = htonl(StartPos);
    SendFrame.Num       = htonl(128);

    //开始读取--
    while (ErrCnt < 3) {  //每次读1024B共128条信息
        //发送
        if (SendFlag) {
            //打包
            PackFrame(&SendFrame, CMD_READ_LST, sizeof(sXzCamFrRdList), pCam->CmdIndex++);
            //发送
            ClrRcv(pHost->HostSock, 0.3);
            Ret = sendto(pHost->HostSock, &SendFrame, sizeof(sXzCamFrRdList), 0, (struct sockaddr *)&(pCam->Addr), sizeof(struct sockaddr_in));
            if (Ret != sizeof(sXzCamFrRdList)) {
                return -1;
            }
        }
        SendFlag = 0;

        //接收
        Ret = recvfrom(pHost->HostSock, tBuf, 2048, RecvFlag, NULL, 0);
        RecvFlag = 0;
        //不够解析
        if (Ret < 5) {
            //判断是否接收完成
            if (StartPos >= pCam->RdFileNum) {
                ErrCnt = 0;
                break;
            } else {
                ErrCnt++;
                SendFrame.Tid   = htons(pCam->CmdIndex);
                SendFlag        = 1;
            }
            continue;
        }
        //开始解析
        if (tBuf[0] == (CLI_TO_HST|CMD_READ_LST)) {         //含有文件信息的数据包
            sXzCamFrListHd *pHead = (sXzCamFrListHd *)tBuf;

            //校验
            if (Ret < sizeof(sXzCamFrListHd)+1026) {          // 长度
                continue;
            }
            if (pHead->StartFid != SendFrame.StartFid) {    //起始偏移
                continue;
            }
            if ((uint16_t)CheckSum(tBuf, sizeof(sXzCamFrListHd)+1024) !=
                tBuf[sizeof(sXzCamFrListHd)+1025]+tBuf[sizeof(sXzCamFrListHd)+1024]*256) {
                //校验码
                continue;
            }
            ErrCnt = 0;
            // 拷贝数据
            int i=0;
            sXzCamFileI *pFiles = (sXzCamFileI *)(tBuf+sizeof(sXzCamFrListHd));

            for (i=0; i<128; i++) {
                tInfo[StartPos+i].RecodType = pFiles[i].RecodType;
                tInfo[StartPos+i].Time      = ntohl(pFiles[i].Time);
                tInfo[StartPos+i].Len       = ConvLen(pFiles[i].Lens);
            }
            // 偏移设为下一个块
            StartPos += 128;
            if (StartPos < pCam->RdFileNum) {
                SendFrame.Tid       = htons(pCam->CmdIndex);
                SendFrame.StartFid  = htonl(StartPos);
                SendFlag            = 1;
            } else {
                RecvFlag  = MSG_DONTWAIT;
            }
        } else if (CheckFrame(tBuf, CLI_TO_HST|CMD_ACK_READLST, Ret) == 0) {
            sXzCamFrRdAck *pHead = (sXzCamFrRdAck *)tBuf;

            //printf("rdstat=%d\n", tBuf[11]);
            if (pHead->Tid == SendFrame.Tid) {
                switch (pHead->RdStat) {
                case ACK_ASME_IDX:      //指令重复需要重发
                    SendFrame.Tid   = htons(pCam->CmdIndex);
                    SendFlag        = 1;
                    break;
                default:
                    ErrCnt++;
                    RecvFlag        = MSG_DONTWAIT;
                    break;
                }
            }
        }
    }
    if (ErrCnt < 3) {
        void *p=realloc(pCam->FileList, pCam->RdFileNum * sizeof(sXzCamFileInfo));
        if (p) {
            pCam->FileList = p;
            memcpy(pCam->FileList, tInfo, pCam->RdFileNum * sizeof(sXzCamFileInfo));
            pCam->RlFileNum = pCam->RdFileNum;

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}
*/

//返回-2，致命错误，简单重发可能无法解决，返回-1，调用者可以尝试再次调用，返回0，读取成功
static int XzCamGetPartFile(sXzCamHost *pHost, int Fd, int CamIndex, const sXzCamFileInfo *pFile, uint32_t AddrPos, int ReadLen)
{
    int Ret=-1, ErrCnt=0, SendFlag=1, RecvFlag=0;
    uint8_t tBuf[2048]= {0}, DataTable[512] = {0};
    sXzCamInfo *pCam;
    sXzCamFrRdFile SendFrame = {{0}};

    pCam    = pHost->pCams+CamIndex;

    //组包，此时要求读整块
    SendFrame.AddrPos   = htonl(AddrPos);
    SendFrame.FileId    = htonl(pFile->FileIndex);
    SendFrame.Len       = htons(ReadLen);
    SendFrame.Tid       = htons(pCam->CmdIndex);

    // 本循环中尽可能读完要求的数据
    while (1) {
        if (SendFlag) { // 需要发送
            PackFrame(&SendFrame, CMD_READ_FILE, sizeof(sXzCamFrRdFile), pCam->CmdIndex++);
            //发送
            ClrRcv(pHost->HostSock, 0.3);
            Ret = sendto(pHost->HostSock, &SendFrame, sizeof(sXzCamFrRdFile), 0, (struct sockaddr *)&(pCam->Addr), sizeof(struct sockaddr_in));
//            printf("%.13G, Send Ret=%d, Data:", BooTime(), Ret);
//            printHex((uint8_t *)&SendFrame, sizeof(sXzCamFrRdFile));
            if (Ret != sizeof(sXzCamFrRdFile)) {
                return -2;
            }
        }
        SendFlag = 0;
        Ret = recvfrom(pHost->HostSock, tBuf, 2048, RecvFlag, NULL, 0);
        RecvFlag = 0;
//        printf("%.13G, Recv Ret=%d, Data:", BooTime(), Ret);
//        printHex(tBuf, 12);
        if (Ret < 5) {
            Ret = -1;
            if (++ErrCnt > 3) {
                break;
            }
            //检查是否完整
            int i = FindHole(DataTable, (ReadLen+1)/2);

            if (i < 0) {
                Ret = 0;
                break;
            }
            SendFrame.AddrPos   = htonl(AddrPos+i*2);
            SendFrame.Len       = htons(2);
            SendFrame.Tid       = htons(pCam->CmdIndex);
            SendFlag            = 1;

            continue;
        }

        if (tBuf[0] == (CLI_TO_HST|CMD_READ_FILE)) {        //含有文件信息的数据包
            sXzCamFrFileHd *pHead = (sXzCamFrFileHd *)tBuf;

            //校验
            if (Ret < sizeof(sXzCamFrFileHd)+1026) {          // 长度
                continue;
            }
            //偏移是否合理
            if (ntohl(pHead->AddrPos)<AddrPos ||
                ntohl(pHead->AddrPos)>=(AddrPos + ReadLen)) {
                continue;
            }
            ErrCnt = 0;
            // 找到合适位置并写入数据
            Ret = lseek(Fd, ntohl(pHead->AddrPos)*512, SEEK_SET);
            if (Ret < 0) {
//                printf("Failed to lseek,fd=%d , ret=%d, errno=%d\n", Fd, Ret, errno);
                close(Fd);
                return -2;
            }
            Ret = write(Fd, tBuf+sizeof(sXzCamFrFileHd), 1024);
            if (Ret != 1024) {
//                printf("Failed to write, ret=%d, errno=%d\n", Ret, errno);
                close(Fd);
                return -2;
            }

            // 标识一下该点数据已收到并写入
            DataTable[(ntohl(pHead->AddrPos)-AddrPos)/2] = 1;
        } else if (CheckFrame(tBuf, CLI_TO_HST|CMD_ACK_READFILE, Ret) == 0) {
            sXzCamFrRdAck *pHead = (sXzCamFrRdAck *)tBuf;

            if (pHead->Tid == SendFrame.Tid) {
                // 指令重复
                switch (pHead->RdStat) {
                case ACK_ASME_IDX:
                    SendFrame.Tid   = htons(pCam->CmdIndex);
                    SendFlag        = 1;
                    break;
                default:
                    ErrCnt++;
                    RecvFlag = MSG_DONTWAIT;
                    break;
                }
                continue;
            }
        }

        int HoleI = FindHole(DataTable, (ReadLen+1)/2);
//        printf("HoleI=%d\n", HoleI);
        if (HoleI == -1) {
            // 所有数据已成功读取
            RecvFlag = MSG_DONTWAIT;
        }
    }

    return Ret;
}

/** 函数说明：  获取指定摄像头中指定文件，并保存在给定路径的文件中
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  @pFile      文件信息的指针（从0开始）
 *  @Path       保存录像文件的路径
 *  返回值：    成功返回0
 */
int XzCamGetFile(sXzCamHost *pHost, int CamIndex, const sXzCamFileInfo *pFile, const char *Path)
{
    if (pHost==NULL || CamIndex<0 || CamIndex>=pHost->CamNum || pFile==NULL) {
        return -1;
    }
    int Ret=0, ErrCnt=0, Fd=-1;
    uint32_t AddrPos=0;

    // 文件序号错误或保存文件打开失败
    if ((Fd = open(Path, O_RDWR|O_CREAT, 0666)) < 0) {
        return -1;
    }

    while (ErrCnt<3 && AddrPos<pFile->Len) {  //错误计数未超限且未读满
        int ReadLen = (pFile->Len-AddrPos)>1024? 1024:((pFile->Len-AddrPos)+1)/2*2; //计算大包读的长度

        Ret = XzCamGetPartFile(pHost, Fd, CamIndex, pFile, AddrPos, ReadLen);

        //如果上述循环出错，则
        switch (Ret) {
        case 0:
            ErrCnt  = 0;
            AddrPos+= ReadLen;
            break;
        case -2:
            ErrCnt = 3;
            break;
        default:
            ErrCnt++;
            break;
        }
    }
    Ret = ftruncate(Fd, pFile->Len*512);
    close(Fd);

    return ErrCnt<3?0:-1;
}

/** 函数说明：  控制指定摄像头开始录像
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  @RecType    给定的录像类型
 *  @RecTime    给定的录像时间标签
 *  @RecLen     给定的录像长度，单位秒
 *  返回值：    成功返回0（摄像头开始录像了，还未结束）
 */
int XzCamStartRec(sXzCamHost *pHost, int CamIndex, uint8_t RecType, time_t RecTime, uint16_t RecLen)
{
    if (pHost==NULL || CamIndex<0 || CamIndex>=pHost->CamNum) {
        return -1;
    }
    int Ret;
    uint8_t tBuf[2048] = {0};
    sXzCamInfo *pCam = pHost->pCams+CamIndex;
    sXzCamFrStartRec SendFrame = {{0}};

    if (RecTime < 0) {
        RecTime = time(NULL);
    }
    SendFrame.RecType   = RecType;
    SendFrame.RecTime   = htonl(RecTime);
    SendFrame.RecLen    = htons(RecLen);
    SendFrame.Param.op  = 0;
    SendFrame.Param.lv  = 0x24;
    SendFrame.Param.fps = 0;
    SendFrame.Param.gop = 24;
    SendFrame.Param.w   = htons(640);
    SendFrame.Param.h   = htons(480);
    PackFrame(&SendFrame, CMD_START_RECORD, sizeof(sXzCamFrStartRec), pCam->CmdIndex++);

    ClrRcv(pHost->HostSock, 1);
    if (sendto(pHost->HostSock, &SendFrame, sizeof(sXzCamFrStartRec),
               0, (struct sockaddr *)&(pCam->Addr), sizeof(struct sockaddr_in))
        != sizeof(sXzCamFrStartRec)) {
        return -1;
    }

    Ret = recvfrom(pHost->HostSock, tBuf, 2048, 0, NULL, 0);
    if (Ret < 5) {
        return -1;
    }

    return CheckFrame(tBuf, CLI_TO_HST|CMD_START_RECORD, Ret);
}

static int XzCamCmdSimple(sXzCamHost *pHost, sXzCamInfo *pCam, int Cmd)
{
    int Ret;
    uint8_t tBuf[2048] = {0};
    sXzCamFrSimple SendFrame = {{0}};

    PackFrame(&SendFrame, Cmd, sizeof(sXzCamFrSimple), pCam->CmdIndex++);

    ClrRcv(pHost->HostSock, 1);
    if (sendto(pHost->HostSock, &SendFrame, sizeof(sXzCamFrSimple),
               0, (struct sockaddr *)&(pCam->Addr), sizeof(struct sockaddr_in))
        != sizeof(sXzCamFrSimple)) {
        return -1;
    }

    Ret = recvfrom(pHost->HostSock, tBuf, 2048, 0, NULL, 0);
    if (Ret < 5) {
        return -1;
    }

    return CheckFrame(tBuf, CLI_TO_HST|Cmd, Ret);
}

/** 函数说明：  控制指定摄像头停止录像
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  返回值：    成功返回0
 */
int XzCamStopRec(sXzCamHost *pHost, int CamIndex)
{
    if (pHost==NULL || CamIndex<0 || CamIndex>=pHost->CamNum) {
        return -1;
    }

    return XzCamCmdSimple(pHost, pHost->pCams+CamIndex, CMD_STOP_RECORD);
}

/** 函数说明：  手动添加摄像头信息
 *  参数：
 *  @pHost      主机结构体的指针
 *  @Ip         摄像头的IP
 *  @Mac        摄像头的MAC
 *  返回值：    成功返回新添加摄像头的索引号
 */
int XzCamAddCam(sXzCamHost *pHost, const char *Ip, const uint8_t *Mac)
{
    if (pHost==NULL || Ip==NULL) {
        return -1;
    }
    sXzCamInfo *pCam = NULL;

    pCam = realloc(pHost->pCams, (pHost->CamNum + 1)*sizeof(sXzCamInfo));
    if (pCam == NULL) {
        return -1;
    }
    pHost->pCams = pCam;
    pCam = pHost->pCams+pHost->CamNum;
    memset(pCam, 0, sizeof(sXzCamInfo));

    pCam->Addr.sin_family       = AF_INET;
    pCam->Addr.sin_addr.s_addr  = inet_addr(Ip);
    pCam->Addr.sin_port         = htons(pHost->HostPort);
    if (Mac) {
        memcpy(pCam->Mac, Mac, 6);
    }
    pHost->CamNum++;

    return pHost->CamNum-1;
}

/** 函数说明：  根据给定的录像类型和时间标签，得到符合条件的文件列表
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  @RecType    给定的录像类型，非负时有效
 *  @StarTime   给定的录像起始时间标签
 *  @EndTime   给定的录像结束时间标签，负数时不使用时间过滤
 *  @ppFiles    用于返回文件列表指针的地址，调用者使用后需要自己去释放，如果传递的是NULL，则不去修改其值，此时只返回找到的数目
 *  返回值：    成功返回找到的文件数，错误时返回-1
 */
int XzCamSearchFile(sXzCamHost *pHost, int CamIndex, int RecType, time_t StarTime, time_t EndTime, sXzCamFileInfo **ppFiles)
{
    if (pHost==NULL || CamIndex>=pHost->CamNum) {
        return -1;
    }
    int i=0, RetFileNum=0;
    sXzCamInfo *pCam = pHost->pCams+CamIndex;
    sXzCamFileInfo *pFiles = NULL;

    for (i=0; i<pCam->RlFileNum; i++) {
        //类型判断
        if (RecType>=0 && pCam->FileList[i].RecodType!=RecType) {
            continue;
        }
        //时间判断
        if (EndTime>=0 && (pCam->FileList[i].Time<StarTime || pCam->FileList[i].Time>EndTime)) {
            continue;
        }
        //准备空间
        void *p = realloc(pFiles, (RetFileNum+1)*sizeof(sXzCamFileInfo));
        if (p == NULL) {
            // 空间申请失败
            free(pFiles);
            return -1;
        }
        //复制内容
        pFiles = p;
        pFiles[RetFileNum] = pCam->FileList[i];
        RetFileNum++;
    }
	if (ppFiles) {
		*ppFiles = pFiles;
	} else {
		free(pFiles);
	}

    return RetFileNum;
}
