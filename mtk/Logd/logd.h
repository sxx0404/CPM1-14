#ifndef LOGD_H_INCLUDED
#define LOGD_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define FIFO_PATH   "/tmp/mlog"
#define MAX_BUFLEN  4000        // 不能超过PIPE_BUF-sizeof(sLogInfoHead)

#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐

typedef struct LogInfoHead {
    double          Tm;         // 产生时间
    uint32_t        Lv;         // 日志等级
    int32_t         Tid;        // 调用者的TID
    int32_t         Pid;        // 调用者的PID
    uint16_t        Len;        // Buf字段的总长度包含最后一个'\0'
    uint8_t         TagLen;     // Buf中Tag字段的长度，包括'\0'
    uint8_t         Cs;         // 以上各字段的校验和取反，从Tm开始到TagLen结束按字节相加，结果取反
} sLogInfoHead;

typedef struct LogInfo {
    sLogInfoHead    Head;
    char            StrBuf[MAX_BUFLEN];// 日志的标签、内容，格式示例'T''a''g''\0''m''s''g''\0'
} sLogInfo;

#pragma pack(pop)               // 恢复原对齐方式

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // LOGD_H_INCLUDED
