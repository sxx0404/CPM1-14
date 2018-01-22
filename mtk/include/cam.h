#ifndef CAM_H_INCLUDED
#define CAM_H_INCLUDED

// CamID
enum {
    CAM_ID_IN = 0,
    CAM_ID_OUT,
    CAM_ID_NUM
};

// Cam状态
enum {
    CAM_STAT_INITING = 0,
    CAM_STAT_INITED,
    CAM_STAT_STANDBY,           // 等待指令中
    CAM_STAT_ERRCOMM,           // 无法通信
    CAM_STAT_ERRCOND,           // 摄像头状态有问题
    CAM_STAT_GETFILE,           // 获取文件中
    CAM_STAT_RECORDING,         // 录像中
    CAM_STAT_LIVING,            // 直播
    CAM_STAT_NUM,
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (*CamStatCb)(int CamId, int nStat);
typedef void (*CamRcdCb)(void* Arg, int CamId, int RecType, int Time, int Ret);

int CamInit(int ConnCamNum, CamStatCb StatCb);
//int CamCmdRecord(int CamId, int RecType, int RecLen, int Time);
int CamCmdRecordNb(int CamId, int RecType, int RecLen, int Time, CamRcdCb Cb, void *Arg);
// 返回获得的录像文件名，使用完记得清理(包括返回值和文件)
char* CamCmdGetFile(int CamId, int RecType, int Time);
void* CamThread(void *Arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CAM_H_INCLUDED
