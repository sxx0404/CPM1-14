#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../common/common.h"
#include "../../Logd/logd.h"
#include "log.h"

#define MAX_TAGLEN  100
#define MAX_WR_TO   -1


static int SendLog(sLogInfo *pLog, double Timeout)
{
    int Ret=-1, FifoFd=-1;

    FifoFd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (FifoFd < 0) {
		if (errno!=EACCES && errno!=ENXIO && errno!=ENOENT) {
			fprintf(stderr, "Failed to open FIFO_PATH[%s], errno=%d\n", FIFO_PATH, errno);
		}
        return -1;
    }
    if (Timeout < 0) {
        //设置成非阻塞
        fcntl(FifoFd, F_SETFL, fcntl(FifoFd, F_GETFL, 0)&(~O_NONBLOCK));
        Ret = write(FifoFd, pLog, sizeof(sLogInfoHead)+pLog->Head.Len);
    } else {
        double st = BooTime();

        //超时前重试
        do {
            // 因为LogLen小于PIPE_BUF，所以是原子写入
            Ret = write(FifoFd, pLog, sizeof(sLogInfoHead)+pLog->Head.Len);
            if (Ret == sizeof(sLogInfoHead)+pLog->Head.Len) {
                break;
            } else if (Ret >= 0) {
                fprintf(stderr, "Failed to write, Log len=%d, ret=%d, errno=%d\n", sizeof(sLogInfoHead)+pLog->Head.Len, Ret, errno);
                break;
            }
        } while (st+MAX_WR_TO > BooTime());
    }
    close(FifoFd);
    if (Ret != sizeof(sLogInfoHead)+pLog->Head.Len) {
        fprintf(stderr, "Log len=%d, ret=%d, errno=%d\n", sizeof(sLogInfoHead)+pLog->Head.Len, Ret, errno);
        Ret = -1;
    } else {
        Ret = 0;
    }

    return Ret;
}

int Log(const char *Tag, int LogLv, const char *fmt, ...)
{
    if (Tag==NULL || fmt==NULL) {
        return -1;
    }
    va_list ap;
    sLogInfo t;

    //构造日志完整信息
    t.Head.Tm    = ClockTime(CLOCK_REALTIME);
    t.Head.Lv    = LogLv;
    t.Head.Pid   = getpid();
    t.Head.Tid   = gettid();
    t.Head.TagLen= snprintf(t.StrBuf, MAX_TAGLEN, "%s", Tag)+1;
    if (t.Head.TagLen <= 0) {
        t.Head.TagLen = 1;
    } else if (t.Head.TagLen >= MAX_TAGLEN) {
        t.Head.TagLen = MAX_TAGLEN;
    }
    t.StrBuf[t.Head.TagLen-1] = '\0';
    va_start(ap, fmt);
    t.Head.Len   = vsnprintf(t.StrBuf+t.Head.TagLen, MAX_BUFLEN-t.Head.TagLen, fmt, ap)+t.Head.TagLen+1;
    va_end(ap);
    if (t.Head.Len <= t.Head.TagLen) {
        t.Head.Len = t.Head.TagLen+1;
    } else if (t.Head.Len >= MAX_BUFLEN) {
        t.StrBuf[MAX_BUFLEN-4]  = '.';
        t.StrBuf[MAX_BUFLEN-3]  = '.';
        t.StrBuf[MAX_BUFLEN-2]  = '.';
        t.Head.Len              = MAX_BUFLEN;
    }
    t.StrBuf[t.Head.Len-1]  = '\0';
    t.Head.Cs               = 0;
    t.Head.Cs               = 0xFF - (CheckSum(&t, sizeof(sLogInfoHead))&0xFF);

    // 使用FIFO，open，write
    return SendLog(&t, MAX_WR_TO);
}
