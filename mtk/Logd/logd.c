#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../libs/common/common.h"
#include "../libs/log/log.h"
#include "logd.h"

#define MAX_ULOGS_NUM 2000

typedef struct LogNode sLogNode;
struct LogNode {
    sLogNode *Next;
    sLogInfo Info;
};

typedef struct Logs {
    uint32_t        Num;
    sLogNode        *Head;
    sLogNode        **pTail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} sLogs;

static const char *Version = "1.0.0";   //版本号
const char *LogFiles[LOG_LV_NUM] = {
    NULL,
    "/tmp/log1",
    "/tmp/log1",
    "/home/log3",
    "/home/log3",
};
const uint32_t MaxLogLen[LOG_LV_NUM] = {
    0,
    102400,
    102400,
    1024000,
    1024000,
};
static sLogs uLogs;                     //未处理的日志信息

static void printLog(sLogInfo *pLog)
{
    time_t SecT = pLog->Head.Tm;
    struct tm LocalT = {0};

    localtime_r(&SecT, &LocalT);

    printf("(%u %d/%d)%04d-%02d-%02d %02d:%02d:%02d.%03d %s::%s\n",
           pLog->Head.Lv, pLog->Head.Tid, pLog->Head.Pid, LocalT.tm_year+1900, LocalT.tm_mon+1, LocalT.tm_mday,
           LocalT.tm_hour, LocalT.tm_min, LocalT.tm_sec, (int)((pLog->Head.Tm-SecT)*1000), pLog->StrBuf, pLog->StrBuf+pLog->Head.TagLen);
}

static void RecLog(sLogInfo *pLog)
{
    const char *LogFilePath=NULL;
    char FileLockPath[256] = {0}, LogBuf[5000] = {0};
    int LockfFd=-1, LogFd = -1, CrossFlag = 0;
    uint32_t LogLen=0, OffSet[2] = {0};       //分别记录文件的起始和结尾
    time_t SecT = pLog->Head.Tm;
    struct tm LocalT = {0};

    //获取路径
    LogFilePath = LogFiles[pLog->Head.Lv];
    if (LogFilePath == NULL) {
        return;
    }
    //准备记录的日志内容
    localtime_r(&SecT, &LocalT);
    LogLen = snprintf(LogBuf, 5000, "(%u)%04d-%02d-%02d %02d:%02d:%02d %s::%s\n",
                      pLog->Head.Lv, LocalT.tm_year+1900, LocalT.tm_mon+1, LocalT.tm_mday,
                      LocalT.tm_hour, LocalT.tm_min, LocalT.tm_sec, pLog->StrBuf, pLog->StrBuf+pLog->Head.TagLen);
    if (LogLen >= 5000) {
        LogLen = 5000;
        LogBuf[LogLen-1] = '\n';
    } else if (LogLen <= 0) {
        return;
    }
    // 锁路径
    snprintf(FileLockPath, 256, "%s.lock", LogFilePath);
    //打开文件
    LockfFd = open(FileLockPath, O_RDWR|O_CREAT, 0666);
    if (LockfFd < 0) {
        return;
    }
    if (flock(LockfFd, LOCK_EX) == 0) {
        //读取或写入偏移
        if (read(LockfFd, OffSet, 8) != 8) {
            OffSet[0] = 0;
            OffSet[1] = 0;
            lseek(LockfFd, 0, SEEK_SET);
            write(LockfFd, OffSet, 8);
        }
        LogFd = open(LogFilePath, O_RDWR|O_CREAT, 0666);
        if (LogFd < 0) {
            // 删掉再试一次
            unlink(LogFilePath);
            LogFd = open(LogFilePath, O_RDWR|O_CREAT, 0666);
        }
        // 再失败就放弃了
        if (LogFd >= 0) {
            off_t FileLen = lseek(LogFd, 0, SEEK_END);

            // 判断偏移是否合理
            if(FileLen<OffSet[1] || FileLen<OffSet[0]) {
                OffSet[1] = OffSet[0] = 0;
            }

            lseek(LogFd, OffSet[1], SEEK_SET);      // 移动到对应位置
            write(LogFd, LogBuf, LogLen);           // 先将日志写入
            if (OffSet[0]>OffSet[1] && OffSet[0]<=OffSet[1]+LogLen) {//判断是否覆盖
                CrossFlag = 1;
            }
            OffSet[1] += LogLen;
            if (OffSet[1] > MaxLogLen[pLog->Head.Lv]) { //如果超限则回到文件开头
                ftruncate(LogFd, OffSet[1]);
                OffSet[1] = 0;
            }
            lseek(LogFd, OffSet[1], SEEK_SET);
            //写入结束标识
            LogBuf[0] = '\0';
            write(LogFd, "=", 1);
            while (read(LogFd, LogBuf, 1)==1 && LogBuf[0]!='\n') {
                lseek(LogFd, -1, SEEK_CUR);
                write(LogFd, "=", 1);
            }
            if (OffSet[0]>=OffSet[1] && OffSet[0]<lseek(LogFd, 0, SEEK_CUR)) {  //结束标识写入时判断是否覆盖
                CrossFlag = 1;
            }
            if (LogBuf[0] != '\n') {
                write(LogFd, "\n", 1);
                lseek(LogFd, 0, SEEK_SET);
            }
            //如果有覆盖，重新设置起始位置
            if (CrossFlag) {
                OffSet[0] = lseek(LogFd, 0, SEEK_CUR);
            }
            //写入日志的起始和结束偏移
            lseek(LockfFd, 0, SEEK_SET);
            write(LockfFd, OffSet, 8);
            close(LogFd);
        }
        ftruncate(LockfFd, 8);
        flock(LockfFd, LOCK_UN);
    }

    close(LockfFd);
}

void* func(void *Arg)
{
    sLogNode *pNode = NULL;

    while (1) {
        if (pthread_mutex_lock(&uLogs.lock) == 0) {
            while (1) {
                if (uLogs.Head) {
                    pNode = uLogs.Head;
                    uLogs.Head = pNode->Next;
                    pNode->Next = NULL;
                    if (uLogs.Head == NULL) {
                        uLogs.pTail = &uLogs.Head;
                    }
                    uLogs.Num--;
                    break;
                } else {
                    uLogs.Num = 0;
                    pthread_cond_wait(&uLogs.cond, &uLogs.lock);
                }
            }
            pthread_mutex_unlock(&uLogs.lock);
        }
        if (pNode) {
            RecLog(&(pNode->Info));
            printLog(&(pNode->Info));
            free(pNode);
            pNode = NULL;
        }
    }
    return NULL;
}

int main(int argc, const char * argv[])
{
	if (argc==2 && strcmp(argv[1], "-v")==0) {
		printf("Soft Ver:%s\nCompile Time:%s %s\n",
				Version, __DATE__, __TIME__);
		exit(EXIT_SUCCESS);
	}
    int Ret=-1, FifoFd=-1, DataLen=0;
    char tBuf[4096];
    pthread_t thread;
    sLogInfo *pLog = (sLogInfo*)tBuf;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    memset(&uLogs, 0, sizeof(sLogs));
    uLogs.pTail = &(uLogs.Head);
    pthread_mutex_init(&(uLogs.lock), NULL);
    pthread_cond_init(&(uLogs.cond), NULL);

    Ret = pthread_create(&thread, NULL, func, NULL);

    mkfifo(FIFO_PATH, 0666);

    FifoFd = open(FIFO_PATH, O_RDONLY);
    if (FifoFd < 0) {
        printf("Failed to open FIFO_PATH[%s], errno=%d\n", FIFO_PATH, errno);
        return -1;
    }
    open(FIFO_PATH, O_WRONLY);  //使上者读时不会返回0
    while (1) {
        Ret = read(FifoFd, tBuf+DataLen, 4096-DataLen);
        if (Ret <= 0) {
            fprintf(stderr, "Failed to Read, errno=%d\n", errno);
            usleep(1000);
            continue;
        }
        DataLen += Ret;
        while (DataLen>sizeof(sLogInfoHead)) {
            if ((pLog->Head.TagLen <= 0) ||
                (pLog->Head.Len > MAX_BUFLEN) ||
                (pLog->Head.Len < pLog->Head.TagLen+1) ||
                (CheckSum(tBuf, sizeof(sLogInfoHead))&0xFF) != 0xFF) {   //长度或校验和有问题
                memmove(tBuf, tBuf+1, --DataLen);
                continue;
            }
            if (DataLen < pLog->Head.Len+sizeof(sLogInfoHead)) {    //长度不够，需要再接收
                break;
            }
            pLog->StrBuf[pLog->Head.TagLen-1] = '\0';
            pLog->StrBuf[pLog->Head.Len-1] = '\0';
            pLog->Head.Lv = pLog->Head.Lv%LOG_LV_NUM;

            //取出并插入ulogs
            sLogNode *pNode = calloc(1, sizeof(sLogNode));
            if (pNode != NULL) {    // 空间申请成功--
                memcpy(&(pNode->Info), tBuf, pLog->Head.Len + sizeof(sLogInfoHead));
                int bFlag = 1;
                while (bFlag) {
                    if (pthread_mutex_lock(&uLogs.lock) == 0) { // 加锁成功
                        if (uLogs.Num < MAX_ULOGS_NUM) {        // 未超限度
                            *uLogs.pTail= pNode;
                            uLogs.pTail = &(pNode->Next);
                            uLogs.Num++;
                            pthread_cond_signal(&uLogs.cond);
                            bFlag = 0;
                        } else {
//                            fprintf(stderr, "Too many ulogs, Num=%d, Limit=%d\n", uLogs.Num, MAX_ULOGS_NUM);
                        }
                        pthread_mutex_unlock(&uLogs.lock);
                    } else {
                        fprintf(stderr, "Failed to pthread_mutex_lock, errno=%d\n", errno);
                        free(pNode);
                        bFlag = 0;
                    }
                    if (bFlag) {
                        usleep(10000);
                    }
                }
            } else {
                fprintf(stderr, "Failed to calloc, errno=%d\n", errno);
            }
            DataLen -= (pLog->Head.Len + sizeof(sLogInfoHead));
            memmove(tBuf, tBuf+(pLog->Head.Len + sizeof(sLogInfoHead)), DataLen);
        }
    }


    return 0;
}




