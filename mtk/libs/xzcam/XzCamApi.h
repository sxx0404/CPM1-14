#ifndef CAMAPI_H_INCLUDED
#define CAMAPI_H_INCLUDED

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐

typedef struct XzCamStat {
    uint8_t CardIn      :1;
    uint8_t CardInitErr :1;
    uint8_t FsInitErr   :1;
    uint8_t FsErr       :1;
    uint8_t I2cErr      :1;
    uint8_t IsRecord    :1;
    uint8_t IsReadFile  :1;
    uint8_t IsVideo     :1;
} sXzCamStat;                   // 1B

#pragma pack(pop)               // 恢复原对齐方式

typedef struct XzCamFileInfo {
    uint32_t    FileIndex;      // 文件索引，读取时用到
    uint8_t     RecodType;      // 录像时传递的类型
    uint32_t    Len;            // 该录像的长度，单位512B
    time_t      Time;           // 录像时传递的日期参数
} sXzCamFileInfo;

typedef struct XzCamInfo {
    struct sockaddr_in  Addr;
    uint8_t             Mac[6];
    uint16_t            CmdIndex;
    union {
        sXzCamStat        sStat;
        uint8_t         bStat;
    }                   Stat;
    uint32_t            RdFileNum;  // 读到的文件数
    uint32_t            MinFileId;
    uint32_t            MaxFileId;
    uint32_t            MinDate;
    uint32_t            MaxDate;
    uint32_t            RlFileNum;  // 以下结构体中的文件数
    sXzCamFileInfo*       FileList;
} sXzCamInfo;

typedef struct XzCamHost {
    uint16_t            HostPort;
    int                 HostSock;
    int                 CamNum;
    sXzCamInfo            *pCams;
} sXzCamHost;


/** 函数说明：  初始化摄像头主机信息结构体（结构体清空，并初始化UDP套接字）
 *  参数：
 *  @pHost      主机结构体的指针
 *  @Port       用于和摄像头通讯的端口
 *  返回值：    成功返回0
 */
int XzCamInitHost(sXzCamHost *pHost, uint16_t Port);

/** 函数说明：  查找指定网卡上的所有摄像头，并将这些摄像头的信息保存(添加)在主机结构体中
 *  参数：
 *  @pHost      主机结构体的指针
 *	@IfName		网卡名
 *	@Type		0找到的结果替换原pHost中的信息，1添加到原pHost中的信息之后
 *  返回值：    返回找到的摄像头数量
 */
int XzCamSearchCams(sXzCamHost *pHost, const char* IfName, int Type);

/** 函数说明：  获取指定摄像头的状态，并更新到其信息结构体的变量中（更新状态、录像文件数，最大最小文件ID，最大最小时间）
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  返回值：    成功返回0
 */
int XzCamGetStat(sXzCamHost *pHost, int CamIndex);

/** 函数说明：  设置摄像头的IP
 *  参数：
 *  @pHost      主机结构体的指针
 *	@IfName		网卡名
 *  @Ip         摄像头设置后的IP
 *  @Mac        摄像头设置后的MAC
 *  返回值：    成功返回0
 *  备注：     仅适用只有1个摄像头的情况，其他情况请勿适用
 */
int XzCamSetAddr(sXzCamHost *pHost, const char* IfName, const char *Ip, const uint8_t *Mac);

/** 函数说明：  获取指定摄像头的文件信息列表，并更新到其信息结构体的变量中
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  返回值：    成功返回0
 */
int XzCamGetFileList(sXzCamHost *pHost, int CamIndex);

/** 函数说明：  获取指定摄像头中指定文件，并保存在给定路径的文件中
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  @pFile      文件信息的指针（从0开始）
 *  @Path       保存录像文件的路径
 *  返回值：    成功返回0
 */
int XzCamGetFile(sXzCamHost *pHost, int CamIndex, const sXzCamFileInfo *pFile, const char *Path);

/** 函数说明：  控制指定摄像头开始录像
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  @RecType    给定的录像类型
 *  @RecTime    给定的录像时间标签
 *  @RecLen     给定的录像长度，单位秒
 *  返回值：    成功返回0（摄像头开始录像了，还未结束）
 */
int XzCamStartRec(sXzCamHost *pHost, int CamIndex, uint8_t RecType, time_t RecTime, uint16_t RecLen);


/** 函数说明：  控制指定摄像头停止录像
 *  参数：
 *  @pHost      主机结构体的指针
 *  @CamIndex   摄像头在主机结构体的摄像头列表的序号（从0开始）
 *  返回值：    成功返回0
 */
int XzCamStopRec(sXzCamHost *pHost, int CamIndex);

/** 函数说明：  手动添加摄像头信息
 *  参数：
 *  @pHost      主机结构体的指针
 *  @Ip         摄像头的IP
 *  @Mac        摄像头的MAC
 *  返回值：    成功返回新添加摄像头的索引号
 */
int XzCamAddCam(sXzCamHost *pHost, const char *Ip, const uint8_t *Mac);

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
int XzCamSearchFile(sXzCamHost *pHost, int CamIndex, int RecType, time_t StarTime, time_t EndTime, sXzCamFileInfo **ppFiles);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAMAPI_H_INCLUDED
