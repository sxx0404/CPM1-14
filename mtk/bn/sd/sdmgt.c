#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include "../NormalTool.h"
#include "../../include/common.h"
#include "../../include/log.h"
#include "../main_linuxapp.h"
#include "../../include/gpio.h"
#include "check_dir.h"
#include "sdmgt.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEF_S_UPDATE_INTVL		(60*3)
#define RCD_FL_SD_DIR					"/mnt/rcd/"
#define SD_MNT_PATH					"/mnt/"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct SdTask				sSdTask;
typedef struct SdTaskList		sSdTaskList;
typedef struct SdInfo				sSdInfo;

struct SdTask {
	sSdTask			*Next;
	int					Type;				// 任务类型(存储或者读取)
	int					CamId;			// 摄像机ID
	int					RecType;		// 录像的类型
	time_t				Time;				// 获取文件时或录像时的时间标签
	SdOprCb			Cb;					// 回调函数
	void					*CbArg;
};

struct SdTaskList {
	pthread_mutex_t	lock;
	pthread_cond_t		cond;
	sSdTask					*Head;
};

typedef struct SdCbFlg {
	pthread_mutex_t	Lock;
	pthread_cond_t		Cond;
	int 							Ret;
} sSdCbFlg;

struct SdInfo {
	pthread_mutex_t	Lock;	// SD卡信息访问修改保护锁
	sSdTask		*pTask;			// 当前的任务
	int				Stat;				// SD卡的当前状态
	uint64_t		TotlCap;			// unit:KB
	uint64_t		FreeCap;
};

struct SdMod {
	sSdTaskList			TaskLst;
	sSdInfo				SdInfo;
	SdStatCb				StatCb;	// 外部指示灯
};

typedef struct SdLedMgt {
	pthread_rwlock_t	Mutex;
	int							bChg;
	int							Stat;
}sSdLedMgt;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static struct SdMod gSdMod;
static sSdLedMgt SdLed;

static void *SdLedThread(void *arg)
{
	int nSdStat = SD_STAT_INIT;
	struct timeval otv =  {0}, ntv = {0};
	int LedV = 0;

	while (1) {
		usleep(1000*50);
		if (pthread_rwlock_rdlock(&SdLed.Mutex)) {
			continue;
		}
		if (SdLed.bChg) {
			SdLed.bChg = 0;
			nSdStat = SdLed.Stat;
			Log("Sd", LOG_LV_DEBUG, "Sd status change, nStatus=%d.", nSdStat);
		}
		pthread_rwlock_unlock(&SdLed.Mutex);
		
		// SD卡状态LED8指示灯
		switch (nSdStat) {
			default:
			case SD_STAT_INIT:
			case SD_STAT_NOT_DETC:
			case SD_STAT_ERRUSED: {
				gettimeofday(&ntv, NULL);
				if (otv.tv_usec/100000ul != ntv.tv_usec/100000ul) {
					// Interval(间隔):100ms
					//Log("Sd", LOG_LV_DEBUG, "Sd Led change, o=%ld, n=%ld.", otv.tv_usec, ntv.tv_usec);
					otv = ntv;
					LedsCtrl(LED_LOCK_MODE, LedV = !LedV);
				}
			}
			break;
			
			case SD_STAT_DETCED:
			case SD_STAT_STANDBY:
			case SD_STAT_WRING:
			case SD_STAT_RDING: {
				if (GpioGet(GPIO_LOCK_MODE) == 0) {
					LedsCtrl(LED_LOCK_MODE, LED_CLOSE_OPR);
				} else {
					LedsCtrl(LED_LOCK_MODE, LED_OPEN_OPR);
				}
			}
			break;
		}
	}

	return NULL;
}

static void SdLedProc(void)
{
	int ret;
	pthread_t tid;
	ret = pthread_create(&tid, NULL, SdLedThread, NULL);
	if (ret) {
		perror("Failed to create SdLedThread\r\n");
		exit(EXIT_FAILURE);
	}
}

// 实际程序关闭
//#define DBG_PRT_SCAN_T
static int GetStorageInfo(const char *MountPoint,	//SD卡随便一个分区
													uint64_t *pTotlCap,
													uint64_t *pFreeCap
													)
{
	//系统stat的结构体
	struct statfs statFS;
	uint64_t FreePect = 0;
	uint64_t freeBytes = 0;
	uint64_t totalBytes = 0;
// 扫描时间的调试打印
#ifdef DBG_PRT_SCAN_T
	double st = 0;
#endif
	if (pTotlCap == NULL || pFreeCap == NULL) {
		return -1;
	}
#ifdef DBG_PRT_SCAN_T
	st = BooTime();
#endif
	if (statfs(MountPoint, &statFS)) {
		//获取分区的状态失败
		Log("Sd", LOG_LV_DEBUG, "sd detect(statfs) is failed for path[%s].");
		return (-1);
	}
	totalBytes	= ((uint64_t)statFS.f_blocks * (uint64_t)statFS.f_bsize);	//详细的分区总容量， 以字节为单位
	freeBytes		= ((uint64_t)statFS.f_bfree * (uint64_t)statFS.f_bsize);		//详细的剩余空间容量，以字节为单位
	FreePect		= freeBytes*100ULL/totalBytes;
	*pTotlCap	= totalBytes/1024ULL;			//以KB为单位的总容量
	*pFreeCap	= freeBytes/1024ULL;			//以KB为单位的剩余空间
	Log("Sd", LOG_LV_DEBUG, "sd total capacity=%lldKB, free capacity=%lldKB, free cap percent=%lld%%.", \
		*pTotlCap, *pFreeCap, FreePect);
#ifdef DBG_PRT_SCAN_T
	Log("Sd", LOG_LV_DEBUG, "sd card scan cost:%.13G seconds.", BooTime()-st);
#endif
	return 0;
}

// 容量或者目录更新
static int SdStatUpdt(void)
{
	int nStat = SD_STAT_INIT, oStat = SD_STAT_INIT;
	char cmd[100]	= {0};
	sSdInfo *pSd = &gSdMod.SdInfo;
	int DoFree = -1;
	oStat = pSd->Stat;
	if (pthread_mutex_lock(&pSd->Lock)) {
		// 概率很低
		return nStat;
	}
	// 检测SD卡是否存在
	if (access("/dev/mmcblk0p1", F_OK) && access("/dev/mmcblk0", F_OK)) {
		nStat = SD_STAT_NOT_DETC;
		if (oStat != nStat) {
			if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
				SdLed.bChg = 1;
				SdLed.Stat = nStat;
				pthread_rwlock_unlock(&SdLed.Mutex);
			}
		}
		pSd->Stat = nStat;
		pthread_mutex_unlock(&pSd->Lock);
		return nStat;
	}
	// 容量和格式信息统计
	if (GetStorageInfo(SD_MNT_PATH, &pSd->TotlCap, &pSd->FreeCap)) {
		nStat = SD_STAT_ERRUSED;
		if (oStat != nStat) {
			if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
				SdLed.bChg = 1;
				SdLed.Stat = nStat;
				pthread_rwlock_unlock(&SdLed.Mutex);
			}
		}
		pSd->Stat = nStat;
		pthread_mutex_unlock(&pSd->Lock);
		return nStat;
	}
	// 视频存储目录/mnt/rcd的检查并确认SD文件格式是否支持
	if (access(RCD_FL_SD_DIR, F_OK)) {
		// 未检测到目录创建目录
		snprintf(cmd, 100, "mkdir -p /mnt/rcd/");
		if (system(cmd)) {
			// 创建目录失败，STAT > SD_STAT_ERRUSED
			nStat = SD_STAT_ERRUSED;
			if (oStat != nStat) {
				if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
					SdLed.bChg = 1;
					SdLed.Stat = nStat;
					pthread_rwlock_unlock(&SdLed.Mutex);
				}
			}
			pSd->Stat = nStat;
			pthread_mutex_unlock(&pSd->Lock);
			return nStat;
		}
	}
	// 目录扫描
	if (DirScanDelPathCb(RCD_FL_SD_DIR, 1, PATH_ONLY_SCAN)) {
		nStat = SD_STAT_ERRUSED;
		if (oStat != nStat) {
			if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
				SdLed.bChg = 1;
				SdLed.Stat = nStat;
				pthread_rwlock_unlock(&SdLed.Mutex);
			}
		}
		pSd->Stat = nStat;
		pthread_mutex_unlock(&pSd->Lock);
		return nStat;
	}
	
	// 剩余容量不足处理
	for (;;) {
		if ((pSd->FreeCap * 100ULL)/pSd->TotlCap >= 5ULL) {
			// 容量大于95%
			break;
		}
		DoFree = 0;
		// 删除最旧的目录
		if (DirScanDelPathCb(RCD_FL_SD_DIR, 1, PATH_DEL_FORCE)) {
			Log("Sd", LOG_LV_WARN, "sd capacity isn't enough, free space is fail1.");
			nStat = SD_STAT_ERRUSED;
			if (oStat != nStat) {
				if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
					SdLed.bChg = 1;
					SdLed.Stat = nStat;
					pthread_rwlock_unlock(&SdLed.Mutex);
				}
			}
			pSd->Stat = nStat;
			pthread_mutex_unlock(&pSd->Lock);
			return nStat;
		}
		// 容量再次统计
		if (GetStorageInfo(SD_MNT_PATH, &pSd->TotlCap, &pSd->FreeCap)) {
			Log("Sd", LOG_LV_WARN, "sd capacity isn't enough, free space is fail2.");
			nStat = SD_STAT_NOT_DETC;
			if (oStat != nStat) {
				if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
					SdLed.bChg = 1;
					SdLed.Stat = nStat;
					pthread_rwlock_unlock(&SdLed.Mutex);
				}
			}
			pSd->Stat = nStat;
			pthread_mutex_unlock(&pSd->Lock);
			return nStat;
		}
	}
	if (DoFree == 0) {
		Log("Sd", LOG_LV_WARN, "sd capacity isn't enough, free space is success.Now sd total capacity=%lldKB, free capacity=%lldKB.", \
			pSd->TotlCap, pSd->FreeCap);
	}
	
	// 状态检测正常	
	nStat = SD_STAT_STANDBY;
	if (oStat != nStat) {
		if (!pthread_rwlock_wrlock(&SdLed.Mutex)) {
			SdLed.bChg = 1;
			SdLed.Stat = nStat;
			pthread_rwlock_unlock(&SdLed.Mutex);
		}
	}
	pSd->Stat = nStat;
	pthread_mutex_unlock(&pSd->Lock);
	return nStat;
}

static void SdFileOprCb(sSdCbFlg* pFlg, int CamId, int RecType, int Time, int Ret)
{
	if (pFlg == NULL) {
		return;
	}
	pthread_mutex_lock(&pFlg->Lock);
	// 操作结果返回
	pFlg->Ret = Ret;
	pthread_cond_broadcast(&pFlg->Cond);
	pthread_mutex_unlock(&pFlg->Lock);
}

static int ChkOpr(int T)
{
	if (T != SD_CMD_WR && T != SD_CMD_RD) return -1;
	else return 0;
}

static int SdAddTask(sSdTask *pTask)
{
	sSdTask **ppTaskList = &(gSdMod.TaskLst.Head);

	//pTask->AddT = BooTime();
	//加锁
	if (pthread_mutex_lock(&(gSdMod.TaskLst.lock)) != 0) {
		return -1;
	}
	//插入任务
	while (*ppTaskList != NULL) {
		ppTaskList = &(ppTaskList[0]->Next);
	}
	*ppTaskList = pTask;
	//通知并解锁
	pthread_cond_signal(&(gSdMod.TaskLst.cond));
	pthread_mutex_unlock(&(gSdMod.TaskLst.lock));

	return 0;
}

static int SdMkRecFilePath(char *Buf, int CamId, int RecType, struct tm Time)
{
	return sprintf(Buf, "%04d%02d%02d/%c-%04d%02d%02d-%02d%02d%02d-%d.h264",\
				   Time.tm_year+1900, Time.tm_mon+1, Time.tm_mday,\
				   RecType, Time.tm_year+1900, Time.tm_mon+1, Time.tm_mday,\
				   Time.tm_hour, Time.tm_min, Time.tm_sec, CamId);
}

// 从SD卡文件操作(阻塞式/外部调用)
int SdFileOprB(int OprType, int CamId, int RecType, int Time)
{
	Log("Sd", LOG_LV_DEBUG, "SdOprCbB, OprT=%d, CamId=%d, T=%d, Time=%d.", OprType, CamId, RecType, Time);
	if (RecType>0xFF || Time<0 || ChkOpr(OprType)) {
		return -1;
	}
	sSdTask *pTask		= calloc(1, sizeof(sSdTask));
	sSdCbFlg *pFlg		= calloc(1, sizeof(sSdCbFlg));

	if (pTask==NULL || pFlg==NULL) {
		if (pTask) {
			free(pTask);
		}
		if (pFlg) {
			free(pFlg);
		}
		return -1;
	}
	pthread_mutex_init(&pFlg->Lock, NULL);
	pthread_cond_init(&pFlg->Cond, NULL);
	pthread_mutex_lock(&pFlg->Lock);
	pFlg->Ret				= -1;
	pTask->Type			= OprType;
	pTask->CamId		= CamId;
	pTask->RecType	= RecType;
	pTask->Time			= Time;
	pTask->Next			= NULL;
	pTask->Cb				= (SdOprCb)SdFileOprCb;
	pTask->CbArg			= pFlg;

	int Ret = -1;
	if (SdAddTask(pTask) == 0) {
		// 等待条件变量，结果返回
		pthread_cond_wait(&pFlg->Cond, &pFlg->Lock);
		Ret = pFlg->Ret;
		pthread_mutex_unlock(&pFlg->Lock);
		pthread_mutex_destroy(&pFlg->Lock);
		pthread_cond_destroy(&pFlg->Cond);
		free(pFlg);
		return Ret;
	} else {
		pthread_mutex_unlock(&pFlg->Lock);
		pthread_mutex_destroy(&pFlg->Lock);
		pthread_cond_destroy(&pFlg->Cond);
		free(pFlg);
		free(pTask);
		return Ret;
	}

}

int SdInit(void)
{
	// 结构体基本信息初始化
	memset(&gSdMod, 0, sizeof(struct SdMod));

	if (pthread_mutex_init(&(gSdMod.TaskLst.lock), NULL) ||
			pthread_cond_init(&(gSdMod.TaskLst.cond), NULL)) {
		return -2;
	}
	gSdMod.StatCb	= NULL;

	if (pthread_mutex_init(&(gSdMod.SdInfo.Lock), NULL)) {
		return -3;
	}
	gSdMod.SdInfo.Stat = SD_STAT_INIT;

	pthread_rwlock_init(&SdLed.Mutex, NULL);
	SdLed.bChg	= 0;
	SdLed.Stat		= SD_STAT_INIT;
	SdLedProc();
	
	// 挂载SD > /mnt/
	char cmd[100] = {0};
	int cnt = 0;
	if (access("/dev/mmcblk0p1", F_OK) == 0) {
		snprintf(cmd, 100, "mount /dev/mmcblk0p1 "SD_MNT_PATH " > /dev/null 2>&1");
	} else if (access("/dev/mmcblk0", F_OK) == 0) {
		snprintf(cmd, 100, "mount /dev/mmcblk0 "SD_MNT_PATH " > /dev/null 2>&1");
	} else {
		snprintf(cmd, 100, "mount /dev/mmcblk0p1 "SD_MNT_PATH " > /dev/null 2>&1");
	}
	do {
		if (system(cmd) == 0) {
			break;
		}
		cnt++;
		sleep(1);
		if (cnt >= 5) {
			break;
		}
	} while (1);
	return 0;
}

void* SdThread(void *Arg)
{
	double LastUpdtT = 0-DEF_S_UPDATE_INTVL;	// 记录SD卡状态更新时间
	int nSdStat = SD_STAT_INIT;
	int ErrCnt = 0;

	while (1) {
		// 定期更新SD卡状态
		if (LastUpdtT + DEF_S_UPDATE_INTVL < BooTime()) {
			double tt = LastUpdtT;
			nSdStat = SdStatUpdt();
			if (nSdStat == SD_STAT_STANDBY || nSdStat == SD_STAT_WRING || nSdStat == SD_STAT_RDING) {
				ErrCnt = 0;
				LastUpdtT = BooTime();
			} else {
				LastUpdtT = BooTime() - DEF_S_UPDATE_INTVL + 1;
				ErrCnt++;
				if (ErrCnt >= 60*15) {
				//if (ErrCnt >= 60*1) {
					// 15分钟之后重启动
					ErrCnt = 0;
					// 重启动主控器
					Log("Sd", LOG_LV_WARN, "Sd status is error and maintain 15minutes, reboot ctrl.");
					reboot();
				}
			}
			Log("Sd", LOG_LV_DEBUG, "SdStatUpdate Sd's Stat=%d, l=%G, b=%G, n=%G.", nSdStat, tt, BooTime(), LastUpdtT);
		}
		
		sSdInfo *pSd = NULL;
		// 取出任务
		// 未超时等待并取出任务
		double et = BooTime()+0.5;
		if (pthread_mutex_lock(&gSdMod.TaskLst.lock) == 0) {
			sSdTask *pTask=NULL, **ppTask=NULL;
			while (1) {
				pSd = &gSdMod.SdInfo;
				ppTask = &(gSdMod.TaskLst.Head);
				if (*ppTask) {
					if (pthread_mutex_lock(&pSd->Lock)) {
						break;
					}
					if (pSd->pTask != NULL) {
						pthread_mutex_unlock(&pSd->Lock);
						break;
					}
					pTask				= *ppTask;
					*ppTask			= pTask->Next;
					pTask->Next	= NULL;
					pSd->pTask	= pTask;
					pthread_mutex_unlock(&pSd->Lock);
				}
				if (pTask) {
					break;
				}
				double Intvl = et - BooTime();
				if (Intvl <= 0) {
					//时间到就不等了
					break;
				}
				Intvl = Intvl+ClockTime(CLOCK_REALTIME)+0.1;
				struct timespec to;

				to.tv_sec	= Intvl;
				to.tv_nsec	= (Intvl-to.tv_sec)*1000000000LL;
				pthread_cond_timedwait(&gSdMod.TaskLst.cond, &gSdMod.TaskLst.lock, &to);
			}
			pthread_mutex_unlock(&gSdMod.TaskLst.lock);
		}else {
			// 概率很小
		}

		// 判断是否任务，有则开始处理
		pSd = &gSdMod.SdInfo;
		if (pthread_mutex_lock(&pSd->Lock) == 0) {
			sSdTask *pTask = NULL;
			sSdInfo *pSd = &gSdMod.SdInfo;
			pTask = pSd->pTask;
			if (pTask == 0) {
				pthread_mutex_unlock(&pSd->Lock);
				continue;
			}
			int Tpye, CamId, RecT;
			time_t RTime;
			Tpye	= pTask->Type;
			CamId	= pTask->CamId;
			RecT	= pTask->RecType;
			RTime	= pTask->Time;
			struct tm Localt = {0};
			char Rtm[100] = {0};
			localtime_r(&RTime, &Localt);
			snprintf(Rtm, 100, "%04d-%02d-%02d %02d:%02d:%02d", \
				Localt.tm_year+1900, Localt.tm_mon+1, Localt.tm_mday,\
				Localt.tm_hour, Localt.tm_min, Localt.tm_sec);
			Log("Sd", LOG_LV_DEBUG, "Sd nTask is dealing with:OprT=%d, CamId=%d, T=%d, Time=%d[%s].",\
				Tpye, CamId, RecT, RTime, Rtm);
#if 0
			if (Tpye == SD_CMD_WR) {
				if (pthread_mutex_lock(&pSd->Lock) == 0) {
					gSdMod.SdInfo.Stat = SD_STAT_WRING;
					pthread_mutex_unlock(&pSd->Lock);
				}
			}
			else if (Tpye == SD_CMD_RD) {
				if (pthread_mutex_lock(&pSd->Lock) == 0) {
					gSdMod.SdInfo.Stat = SD_STAT_RDING;
					pthread_mutex_unlock(&pSd->Lock);
				}
			}
			else {
				// 基本不可能
				if (pTask->Cb) {
					pTask->Cb(pTask->CbArg, CamId, RecT, -1);
				}
				free(pTask);
				pSd->pTask = NULL;
				pthread_mutex_unlock(&pSd->Lock);
				continue;
			}
#endif
			int ret = -1;
			int retry = 0, RetryRet = 0;
			switch (Tpye) {
				case SD_CMD_RD: {
					char FPath[100]	= {RCD_FL_SD_DIR};
					char tFPath[100]	= "/tmp/rcd/";
					SdMkRecFilePath(FPath+strlen(FPath), CamId, RecT, Localt);
					// 重试
					for (retry = 0; retry < 3; retry++) {
						RetryRet = access(FPath, F_OK);
						if (RetryRet) {
							sleep(1);
						} else {
							break;
						}
					}
					if (RetryRet == 0) {
						// SD卡内有录像文件
						sprintf(tFPath+strlen(tFPath), "%d-%d-%ld", CamId, RecT, RTime);
						char cmd[100]	= {0};
						sprintf(cmd, "cp -fr %s %s", FPath, tFPath);
						if (system(cmd)) {
							// 失败-1
							LastUpdtT = 0-DEF_S_UPDATE_INTVL;
							ret = -1;
						} else {
							// 成功
							ret = 0;
						}
						Log("Sd", LOG_LV_DEBUG, "Sd RD Task deal with ret=%d.", ret);
						if (pTask->Cb) {
							pTask->Cb(pTask->CbArg, CamId, RecT, RTime, ret);
						}
						free(pTask);
						pSd->pTask = NULL;
					} else {
						// SD卡内没有该文件,失败-2
						LastUpdtT = 0-DEF_S_UPDATE_INTVL;
						ret = -2;
						Log("Sd", LOG_LV_DEBUG, "Sd RD Task deal with ret=%d.", ret);
						if (pTask->Cb) {
							pTask->Cb(pTask->CbArg, CamId, RecT, RTime, ret);
						}
						free(pTask);
						pSd->pTask = NULL;
					}
				}
				break;

				case SD_CMD_WR: {
					char FPath[100]	= {RCD_FL_SD_DIR};
					char tFPath[100]	= "/tmp/rcd/";
					char cmd[100]	= {0};
					int tLen = 0;
					sprintf(FPath+strlen(FPath), "%04d%02d%02d/", Localt.tm_year+1900, Localt.tm_mon+1, Localt.tm_mday);
					sprintf(tFPath+strlen(tFPath), "%d-%d-%ld", CamId, RecT, RTime);
					// 将文件进行存储，/tmp/rcd/*.264 > /mnt/rcd/YYYYMMDD/*.h264
					if (access(tFPath, F_OK) == 0) {
						if (access(FPath, F_OK) == 0) {
							// SD视频目录存在
							tLen = snprintf(FPath, 100, "%s", RCD_FL_SD_DIR);
							if (tLen >= 0) {
								FPath[tLen] = '\0';
								SdMkRecFilePath(FPath+strlen(FPath), CamId, RecT, Localt);
								sprintf(cmd, "cp -fr %s %s", tFPath, FPath);
								if (system(cmd)) {
									// 失败-3
									LastUpdtT = 0-DEF_S_UPDATE_INTVL;
									ret = -3;
								} else {
									// 成功
									ret = 0;
								}
							} else {
								// 失败-6
								ret = -6;
							}
						} else {
							// SD视频目录不存在
							sprintf(cmd, "mkdir -p %s/%04d%02d%02d", RCD_FL_SD_DIR, Localt.tm_year+1900, Localt.tm_mon+1, Localt.tm_mday);
							if (system(cmd) == 0) {
								if (access(FPath, F_OK) == 0) {
									// SD视频目录存在
									tLen = snprintf(FPath, 100, "%s/", RCD_FL_SD_DIR);
									if (tLen >= 0) {
										FPath[tLen] = '\0';
										SdMkRecFilePath(FPath+strlen(FPath), CamId, RecT, Localt);
										sprintf(cmd, "cp -fr %s %s", tFPath, FPath);
										if (system(cmd)) {
											// 失败-4
											LastUpdtT = 0-DEF_S_UPDATE_INTVL;
											ret = -4;
										} else {
											// 成功
											ret = 0;
										}
									} else {
										// 失败-7
										ret = -7;
									}
								}else {
									// 概率很低,失败-8
									ret = -8;
								}
							} else {
								// 失败-5
								LastUpdtT = 0-DEF_S_UPDATE_INTVL;
								ret = -5;
							}
						}
						// 写入操作完成了，判断结果
						if (ret == 0) {
							// 写入成功，则删除/tmp/rcd文件
							unlink(tFPath);
							// 重新扫描目录并按照时间排序
							LastUpdtT = 0-DEF_S_UPDATE_INTVL;
						}
						Log("Sd", LOG_LV_DEBUG, "Sd WR Task deal with ret=%d.", ret);
						if (pTask->Cb) {
							pTask->Cb(pTask->CbArg, CamId, RecT, RTime, ret);
						}
						free(pTask);
						pSd->pTask = NULL;
					} else {
						// 视频/tmp/rcd临时文件不存在,失败-6
						LastUpdtT = 0-DEF_S_UPDATE_INTVL;
						ret = -6;
						Log("Sd", LOG_LV_DEBUG, "Sd WR Task deal with ret=%d.", ret);
						if (pTask->Cb) {
							pTask->Cb(pTask->CbArg, CamId, RecT, RTime, ret);
						}
						free(pTask);
						pSd->pTask = NULL;
					}
				}
				break;

				default:
					break;
			}
			pthread_mutex_unlock(&pSd->Lock);
		}
	}
}

