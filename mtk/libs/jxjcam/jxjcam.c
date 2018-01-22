#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>

#include "../../include/cam.h"
#include "./inc/j_sdk.h"
#include "./inc/jcu_net_types.h"
#include "./inc/jcu_net_api.h"
#include "../common/common.h"
#include "../log/log.h"

#define DEFAULT_BIND_PORT   10001
#define MAX_RETRY           5
#define COMM_INTVL          60
#define RETRY_INTVL         3
#define CAM_NETMASK			"255.255.255.0"
#define IN_CAM_IP           "169.254.254.3"
#define OUT_CAM_IP          "169.254.254.4"
#define RCD_FL_DIR			"/tmp/rcd/"

enum {
	CAM_CMD_NONE     = -1,
	CAM_CMD_RECORD,
	CAM_CMD_TYPES
};

typedef struct CamHostInfo  sCamHostInfo;
typedef struct CamInfo      sCamInfo;
typedef struct CamTask      sCamTask;
typedef struct CamTaskList  sCamTaskList;

struct CamTask {
	sCamTask			*Next;
	double				AddT;				// 任务添加时间
	jcu_stream_handle_t	*pJxjRcd;			// 录像的对象
	int					Type;				// 任务类型
	int					CamId;				// 摄像机ID
	int					RecType;			// 录像的类型
	int					RecLen;				// 录像的长度，单位秒，仅录像指令用到
	time_t				Time;				// 获取文件时或录像时的时间标签
	CamRcdCb			Cb;					// 回调函数
	void*				CbArg;
};

struct CamTaskList {
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
	sCamTask		*Head;
};

struct CamInfo {
	pthread_mutex_t		Lock;					// 该摄像头信息访问修改的保护锁，保护以下各项
	sCamTask			*pTask;					// 当前的任务，仅CamThread中修改，别处只读取
	int					Stat;					// 摄像头的当前状态
	double				DeadLine;				// 以上任务需要关注一下的时间
	jcu_user_handle_t	*pJxjCam;				// 佳信捷摄像头对象，仅CamThread中置，别处只能清
};

struct CamHostInfo {
	sCamTaskList	TaskLst;
	sCamInfo		Cams[CAM_ID_NUM];
	CamStatCb		StatCb;
	uint8_t			ConnCamNum;
};

typedef struct QueryInfo {
	JNetworkInfo	NetworkInfo;
	char			dst_id[MB_SERIAL_LEN];
} sQueryInfo;

typedef struct CamSearchInfo {
	int				Num;
	sQueryInfo		*pInfos;
} sCamSearchInfo;

const static char *CamIps[CAM_ID_NUM+1] = {
	IN_CAM_IP,
	OUT_CAM_IP,
	"192.168.0.140"			//此为无效的IP，搜索到与以上IP冲突的摄像头IP改为此
};


static sCamHostInfo gCamHost;

static void VoidCb(void)
{
	return;
}

static int CamMkRecFilename(char *Buf, int CamId, int RecType, time_t Time)
{
	return sprintf(Buf, "%d-%d-%ld", CamId, RecType, Time);
}

static void LogoutThread(jcu_user_handle_t *pJxjCam)
{
	if (pJxjCam) {
		jcu_net_logout(pJxjCam);
	}
}

static int LoginEvCb(jcu_event_cb_t *handle, jcu_cb_parm_t *parm)
{
	int i = (int)(handle->user_arg);
	int oCamStat = -1;

	if (i>=0 && i<gCamHost.ConnCamNum && parm->id == JCU_EVENT_CLOSE) {
		pthread_t t;
		pthread_attr_t ta;
		sCamInfo *pCam = gCamHost.Cams+i;
		jcu_user_handle_t *pJxjCam = NULL;

		pthread_mutex_lock(&pCam->Lock);
		oCamStat		= pCam->Stat;
		pCam->Stat		= CAM_STAT_ERRCOMM;
		pJxjCam			= pCam->pJxjCam;
		pCam->pJxjCam	= NULL;
		pCam->DeadLine	= -1;		// 设为-1，如果有任务就会尽早处理
		pthread_mutex_unlock(&pCam->Lock);


		// 没有人去回收该线程资源了，直接设置为不需要回收
		pthread_attr_init(&ta);
		pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
		// 不判断失败了，失败了，我也不知道怎么处理了
		pthread_create(&t, &ta, (void*(*)(void*))LogoutThread, pJxjCam);
		pthread_attr_destroy(&ta);
		if (oCamStat != CAM_STAT_ERRCOMM) {
			gCamHost.StatCb(i, CAM_STAT_ERRCOMM);
		}
	}
	return 0;
}

static int LoginNtCb(jcu_notify_cb_t *handle, jcu_cb_parm_t *parm)
{
	int i = (int)(handle->user_arg);
	int oCamStat = -1;

	if (i>=0 && i<gCamHost.ConnCamNum) {
		sCamInfo *pCam = gCamHost.Cams+i;

		if (parm->args != JCU_NOTIFY_ERR_0) {
			pthread_t t;
			pthread_attr_t ta;
			jcu_user_handle_t *pJxjCam = NULL;

			pthread_mutex_lock(&pCam->Lock);
			oCamStat		= pCam->Stat;
			pCam->Stat		= CAM_STAT_ERRCOMM;
			pJxjCam			= pCam->pJxjCam;
			pCam->pJxjCam	= NULL;
			pCam->DeadLine	= -1;		// 设为-1，如果有任务就会尽早处理
			pthread_mutex_unlock(&pCam->Lock);

			// 没有人去回收该线程资源了，直接设置为不需要回收
			pthread_attr_init(&ta);
			pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

			// 不判断失败了，失败了，我也不知道怎么处理了
			pthread_create(&t, &ta, (void*(*)(void*))LogoutThread, pJxjCam);
			pthread_attr_destroy(&ta);
			if (oCamStat != CAM_STAT_ERRCOMM) {
				gCamHost.StatCb(i, CAM_STAT_ERRCOMM);
			}
		} else {
			pthread_mutex_lock(&pCam->Lock);
			oCamStat	= pCam->Stat;
			if (oCamStat != CAM_STAT_RECORDING) {
				pCam->Stat	= CAM_STAT_STANDBY;
			} else if(pCam->pTask) {
				// 此时为录像状态，该状态下一定会有ptask
				pCam->DeadLine = -1;
			}
			pthread_mutex_unlock(&pCam->Lock);

			if (oCamStat!=CAM_STAT_STANDBY && oCamStat!=CAM_STAT_RECORDING) {
				gCamHost.StatCb(i, CAM_STAT_STANDBY);
			}
		}
	}

	return 0;
}

static int StreamNtCb(jcu_notify_cb_t *handle, jcu_cb_parm_t *parm)
{
	int i = (int)(handle->user_arg);

	if (i>=0 && i<gCamHost.ConnCamNum) {
		int oCamStat = -1;
		sCamInfo *pCam = gCamHost.Cams+i;

		if (parm->args != JCU_NOTIFY_ERR_0) {
			pthread_t t;
			pthread_attr_t ta;
			jcu_user_handle_t *pJxjCam = NULL;

			pthread_mutex_lock(&pCam->Lock);
			oCamStat		= pCam->Stat;
			pCam->Stat		= CAM_STAT_ERRCOMM;
			pJxjCam			= pCam->pJxjCam;
			pCam->pJxjCam	= NULL;
			pCam->DeadLine	= -1;		// 设为-1，如果有任务就会尽早处理
			pthread_mutex_unlock(&pCam->Lock);

			// 没有人去回收该线程资源了，直接设置为不需要回收
			pthread_attr_init(&ta);
			pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
			// 不判断失败了，失败了，我也不知道怎么处理了
			pthread_create(&t, &ta, (void*(*)(void*))LogoutThread, pJxjCam);
			pthread_attr_destroy(&ta);
			if (oCamStat != CAM_STAT_ERRCOMM) {
				gCamHost.StatCb(i, CAM_STAT_ERRCOMM);
			}
		} else {
			pthread_mutex_lock(&pCam->Lock);
			if(pCam->pTask) {
				oCamStat		= pCam->Stat;
				pCam->Stat		= CAM_STAT_RECORDING;
				// 此时为录像状态，该状态下一定会有ptask
				pCam->DeadLine = BooTime()+pCam->pTask->RecLen+0.1;
			}
			pthread_mutex_unlock(&pCam->Lock);
			if (oCamStat != CAM_STAT_RECORDING) {
				gCamHost.StatCb(i, CAM_STAT_RECORDING);
			}
		}
	}

	return 0;
}

static int StreamCb(jcu_stream_cb_t *handle, jcu_cb_parm_t *parm)
{
	sCamInfo *pCam = handle->user_arg;
	j_frame_t *frame = (j_frame_t*)parm->data;
	int CamId=-1, RecType=-1, Time=-1;

	if (parm->id==JCU_STREAM_EV_RECV &&
			frame->frame_type>j_unknown_frame &&
			frame->frame_type<j_pic_frame) {
		pthread_mutex_lock(&pCam->Lock);
		if (pCam->pTask) {
			CamId	= pCam->pTask->CamId;
			RecType	= pCam->pTask->RecType;
			Time	= pCam->pTask->Time;
		}
		pthread_mutex_unlock(&pCam->Lock);
		char tBuf[100] = RCD_FL_DIR;
		if (CamId >= 0) {
			CamMkRecFilename(tBuf+strlen(tBuf), CamId, RecType, Time);
			int fd = open(tBuf, O_WRONLY|O_CREAT|O_APPEND, 0777);

			if (fd >= 0) {
				write(fd, frame->data, frame->size);
				close(fd);
			}
		}
	} else if (parm->id == JCU_STREAM_EV_CLOSE) {
		pthread_mutex_lock(&pCam->Lock);
		pCam->DeadLine	= -1;		// 设为-1，如果有任务就会尽早处理
		pthread_mutex_unlock(&pCam->Lock);
	}

	return 0;
}

static int QueryCb(mb_cu_notify_t *handle, mb_cu_parm_t *parm)
{
	sCamSearchInfo *pInfo = handle->user_arg;

	if (parm->id==J_MB_NetCfg_Id && parm->error==0 && parm->size>=sizeof(JNetworkInfo)) {
		int i=0;

		for (i=0; i<pInfo->Num; i++) {
			if (memcmp(pInfo->pInfos[i].dst_id, parm->dst_id, MB_SERIAL_LEN)==0) {
				break;
			}
		}
		if (i >= pInfo->Num) {
			pInfo->pInfos = realloc(pInfo->pInfos, (pInfo->Num+1)*sizeof(sQueryInfo));
			memcpy(pInfo->pInfos[pInfo->Num].dst_id, parm->dst_id, MB_SERIAL_LEN);
			memcpy(&(pInfo->pInfos[pInfo->Num].NetworkInfo), parm->data, sizeof(JNetworkInfo));
			pInfo->Num++;
		}
	}

	return 0;
}

static int CamAddTask(sCamTask *pTask)
{
	sCamTask **ppTaskList = &(gCamHost.TaskLst.Head);

	pTask->AddT = BooTime();
	//加锁
	if (pthread_mutex_lock(&(gCamHost.TaskLst.lock)) != 0) {
		return -1;
	}
	//插入任务
	while (*ppTaskList != NULL) {
		ppTaskList = &(ppTaskList[0]->Next);
	}
	*ppTaskList = pTask;
	//通知并解锁
	pthread_cond_signal(&(gCamHost.TaskLst.cond));
	pthread_mutex_unlock(&(gCamHost.TaskLst.lock));

	return 0;
}

//
int CamCmdRecordNb(int CamId, int RecType, int RecLen, int Time, CamRcdCb Cb, void *Arg)
{
	// 添加任务并设置
	Log("Cam", LOG_LV_INFO, "CamCmdRecordNb, CamId=%d, T=%d, L=%d, Time=%d, Cb=%d, Arg=%d", CamId, RecType, RecLen, Time, (int)Cb, (int)Arg);
	if (CamId<0 || CamId>=gCamHost.ConnCamNum || RecType<0 || RecType>0xFF || RecLen<=0) {
		return -1;
	}
	sCamTask *pTask = calloc(1, sizeof(sCamTask));

	if (pTask == NULL) {
		return -1;
	}
	pTask->CamId    = CamId;
	pTask->Type     = CAM_CMD_RECORD;
	pTask->RecType  = RecType;
	pTask->RecLen   = RecLen;
	pTask->Time     = Time;
	pTask->Cb       = Cb;
	pTask->CbArg    = Arg;
	pTask->Next     = NULL;

	if (CamAddTask(pTask) == 0) {
		return 0;
	} else {
		free(pTask);
		return -1;
	}
	return 0;
}

// 返回获得的录像文件名，使用完记得清理(包括free返回值和unlink文件)
char* CamCmdGetFile(int CamId, int RecType, int Time)
{
	char FlName[100] = RCD_FL_DIR;

	// TODO:读取录像文件
	
	// 检测录像目录中是否存在，存在则返回文件名，否则返回错误
	CamMkRecFilename(FlName+strlen(FlName), CamId, RecType, Time);
	if (access(FlName, F_OK) == 0) {
		return strdup(FlName);
	} else {
		return NULL;
	}
}

int CamInit(int ConnCamNum, CamStatCb StatCb)
{
	if (ConnCamNum<0 || ConnCamNum>CAM_ID_NUM || StatCb == NULL) {
		return -1;
	}
	int i=0;

	// 修改路由表，加入组播地址
	while (system("route del "GROUP_IP " > /dev/null 2>&1") == 0);
	system("route add "GROUP_IP" br-lan");

	// 结构体基本信息初始化
	memset(&gCamHost, 0, sizeof(sCamHostInfo));

	if (pthread_mutex_init(&(gCamHost.TaskLst.lock), NULL) ||
			pthread_cond_init(&(gCamHost.TaskLst.cond), NULL)) {
		return -1;
	}
	gCamHost.ConnCamNum = ConnCamNum;
	gCamHost.StatCb = StatCb;

	for (i=0; i<gCamHost.ConnCamNum; i++) {
		if (pthread_mutex_init(&(gCamHost.Cams[i].Lock), NULL)) {
			return -1;
		}
		gCamHost.Cams[i].Stat		= CAM_STAT_INITED;
		gCamHost.StatCb(i, CAM_STAT_INITED);
		gCamHost.Cams[i].DeadLine	= -1;
	}

	// 佳信捷摄像头相关初始化
	if (jcu_net_init()) {
		Log("Cam", LOG_LV_DEBUG, "Failed[jcu_net_init] to init Cam, errno:%d.", errno);
		return -1;
	} else if (jcu_mb_init(GROUP_IP, GROUP_PORT, DEFAULT_BIND_PORT)) {
		Log("Cam", LOG_LV_DEBUG, "Failed[jcu_mb_init] to init Cam, errno:%d.", errno);
		return -1;
	}

	return 0;
}

void* CamThread(void *Arg)
{
	int i=0, Ret=-1;

	while (1) {
//		Log("Cam", LOG_LV_DEBUG, "Loop start");
		// 处理摄像头录像停止任务
		for (i=0; i<gCamHost.ConnCamNum; i++) {
			sCamInfo *pCam = gCamHost.Cams+i;
			if (pthread_mutex_lock(&pCam->Lock)) {
				continue;
			}
			// 目前都是录像任务,停止录像，同时回调
			if (pCam->pTask && pCam->DeadLine<BooTime()) {
				int nCamStat=-1, oCamStat=-1;
				sCamTask *pTask = pCam->pTask;
				jcu_user_handle_t *pJxjCam = NULL;

				// 先取出任务
				pCam->pTask = NULL;
				pthread_mutex_unlock(&pCam->Lock);
				// 停止录像
				if (jcu_net_stream_close(pTask->pJxjRcd)) {
					nCamStat = CAM_STAT_ERRCOMM;
				} else {
					nCamStat = CAM_STAT_STANDBY;
				}
				// 更改状态
				pthread_mutex_lock(&pCam->Lock);
				oCamStat = pCam->Stat;
				pCam->Stat = nCamStat = (oCamStat==CAM_STAT_ERRCOMM?oCamStat:nCamStat);
				if (nCamStat != CAM_STAT_STANDBY) {
					pJxjCam = pCam->pJxjCam;
					pCam->pJxjCam = NULL;
				}
				pthread_mutex_unlock(&pCam->Lock);
				// 出错时关闭动作
				if (oCamStat != nCamStat) {
					gCamHost.StatCb(i, nCamStat);
				}
				if (pJxjCam) {
					jcu_net_logout(pJxjCam);
				}
				char tBuf[100] = RCD_FL_DIR;
				struct stat FlStat = {0};
				// 判断文件是否存在
				CamMkRecFilename(tBuf+strlen(tBuf), pTask->CamId, pTask->RecType, pTask->Time);
				Ret = -1;
				if (stat(tBuf, &FlStat)==0 && FlStat.st_size>0) {
					Ret = 0;
				}
				if (pTask->Cb) {
					pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, Ret);
				}
				free(pTask);
			} else {
				pthread_mutex_unlock(&pCam->Lock);
			}
		}
//		Log("Cam", LOG_LV_DEBUG, "Stop task over");
		// 摄像头出错等进一步处理，包括搜索设置摄像头
		Ret = 0;
		uint8_t CamIdleFlgs[CAM_ID_NUM] = {0};	// 0标识忙碌，其他标识空闲
		for (i=0; i<gCamHost.ConnCamNum; i++) {
			sCamInfo *pCam = gCamHost.Cams+i;

			if (pthread_mutex_lock(&pCam->Lock)) {
				continue;
			}
			if (pCam->pJxjCam == NULL) {
				Ret = 1;
			}
			if (pCam->pTask == NULL) {
				CamIdleFlgs[i] = 1;
			}
			pthread_mutex_unlock(&pCam->Lock);
		}
		if (Ret) {
			// 有摄像头未连接，需要搜索来建立
			sCamSearchInfo ScInfo = {0};
			mb_cu_notify_t mbQueryInfo = {.user_arg=&ScInfo, .callback=QueryCb};

			jcu_mb_query_t *pQuery= jcu_mb_query(J_MB_NetCfg_Id, &mbQueryInfo);
			if (pQuery) {
				sleep(1);
				jcu_mb_release(pQuery);
			}
			usleep(200000);
			if (ScInfo.Num > 0) {
				uint8_t	CamFlgs1[CAM_ID_NUM] = {0};		// 记录对应摄像头在ScInfo中的序号，0xFF标记该摄像头未找到匹配信息
				uint8_t	*CamFlgs2 = calloc(1, ScInfo.Num*sizeof(uint8_t));
				// 记录ScInfo中摄像头是匹配，以及对应的ID，0xFF标记该摄像头未找到匹配信息，0xFE用来标记该摄像头信息是否需要修改

				memset(CamFlgs1, 0xFF, CAM_ID_NUM);
				memset(CamFlgs2, 0xFF, ScInfo.Num);
				// 先找到合适的
				for (i=0; i<gCamHost.ConnCamNum; i++) {
					int j = 0;

					for (j=0; j<ScInfo.Num; j++) {
						if (strcmp(CamIps[i], (char *)ScInfo.pInfos[j].NetworkInfo.network[J_SDK_ETH0].ip) == 0) {
							if (CamFlgs1[i] == 0xFF) {
								CamFlgs1[i] = j;
								CamFlgs2[j] = i;
							} else {
								CamFlgs2[j] = 0xFE;
								strcpy((char *)ScInfo.pInfos[j].NetworkInfo.network[J_SDK_ETH0].ip, CamIps[CAM_ID_NUM]);
							}
						}
					}
				}
				// 检查是否还有没设置好的
				for (i=0; i<gCamHost.ConnCamNum; i++) {
					if (CamFlgs1[i] == 0xFF) {
						int j = 0;

						for (j=0; j<ScInfo.Num; j++) {
							if (CamFlgs2[j]==0xFF || strcmp(CamIps[CAM_ID_NUM], (char *)ScInfo.pInfos[j].NetworkInfo.network[J_SDK_ETH0].ip) == 0) {
								strcpy((char *)ScInfo.pInfos[j].NetworkInfo.network[J_SDK_ETH0].ip, CamIps[i]);
								strcpy((char *)ScInfo.pInfos[j].NetworkInfo.network[J_SDK_ETH0].netmask, CAM_NETMASK);
								CamFlgs2[j] = 0xFE;
								CamFlgs1[i] = j;
								break;
							}
						}
					}
				}
				for (i=0; i<gCamHost.ConnCamNum; i++) {
					Log("Cam", LOG_LV_DEBUG, "CamFlgs1[%d]=%d", i, CamFlgs1[i]);
				}
				// 进行设置一下
				for (i=0; i<ScInfo.Num; i++) {
					Log("Cam", LOG_LV_DEBUG, "CamFlgs2[%d]=%d", i, CamFlgs2[i]);
					if (CamFlgs2[i] == 0xFE) {
						mb_cu_notify_t mbNotify = {.callback = (int(*)(mb_cu_notify_t*, mb_cu_parm_t*))VoidCb};
						jcu_mb_cfg_set(J_MB_NetCfg_Id, ScInfo.pInfos[i].dst_id,
									   "admin", "admin", 3, sizeof(JNetworkInfo),
									   &ScInfo.pInfos[i].NetworkInfo, &mbNotify, JCU_OP_ASYNC);
					}
				}
				free(CamFlgs2);
				free(ScInfo.pInfos);
			}
			// 登录
			for (i=0; i<gCamHost.ConnCamNum; i++) {
				sCamInfo *pCam = gCamHost.Cams+i;

				if (pthread_mutex_lock(&pCam->Lock)) {
					continue;
				}
				if (pCam->pJxjCam == NULL) {
					jcu_event_cb_t ec = {(void *)i, LoginEvCb};
					jcu_notify_cb_t nc = {JCU_OP_ASYNC, (void *)i, LoginNtCb};

					pCam->pJxjCam = jcu_net_login((char *)CamIps[i], 3321, "admin", "admin", 5, &ec, &nc);
				}
				pthread_mutex_unlock(&pCam->Lock);
			}
		}
//		Log("Cam", LOG_LV_DEBUG, "Find Cam over");
		// 取出任务
		sCamTask *pTask=NULL, **ppTask=NULL;
		//未超时等待并取出任务
		double et = BooTime()+0.5;
		if (pthread_mutex_lock(&gCamHost.TaskLst.lock) == 0) {
			while (1) {
				ppTask = &(gCamHost.TaskLst.Head);
				while (*ppTask) {
					if (CamIdleFlgs[(*ppTask)->CamId]) {
						pTask		= *ppTask;
						*ppTask		= pTask->Next;
						pTask->Next	= NULL;

						if (pTask->AddT+300 < BooTime()) {
							// 太早了，不用录了，删除该任务
							// 如果有回调函数，则调用
							if (pTask->Cb) {
								pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, -1);
							}
							free(pTask);
							pTask = NULL;
							continue;
						}
						break;
					} else {
						ppTask = &((*ppTask)->Next);
					}
				}
				if (pTask) {
					break;
				}
				double Intvl = et - BooTime();
				if (Intvl <= 0) {
					//时间到就不等了--
					break;
				}
				Intvl = Intvl+ClockTime(CLOCK_REALTIME)+0.1;
				struct timespec to;

				to.tv_sec	= Intvl;
				to.tv_nsec	= (Intvl-to.tv_sec)*1000000000LL;

				pthread_cond_timedwait(&gCamHost.TaskLst.cond, &gCamHost.TaskLst.lock, &to);
			}
			pthread_mutex_unlock(&gCamHost.TaskLst.lock);
		}
//		Log("Cam", LOG_LV_DEBUG, "Pick task over");
		// 有任务则开始处理
		if (pTask) {
			sCamInfo *pCam = gCamHost.Cams+pTask->CamId;
			char tBuf[100] = RCD_FL_DIR;
			int oCamStat = -1;

			Log("Cam", LOG_LV_DEBUG, "Task CamId=%d", pTask->CamId);
			// 删除原文件
			Ret = -1;
			CamMkRecFilename(tBuf+strlen(tBuf), pTask->CamId, pTask->RecType, pTask->Time);
			unlink(tBuf);
			if (pthread_mutex_lock(&(pCam->Lock)) == 0) {
				if (pCam->pJxjCam) {
					jcu_stream_cb_t sc = {pCam, StreamCb};
					jcu_notify_cb_t nc = {JCU_OP_ASYNC, (void *)pTask->CamId, StreamNtCb};

					oCamStat		= pCam->Stat;
					pCam->DeadLine	= BooTime()+5;		// 5秒内没开始录就关掉
					pCam->pTask		= pTask;
					pTask->pJxjRcd	= jcu_net_stream_open(pCam->pJxjCam, 0, j_primary_stream, j_rtp_over_tcp, &sc, &nc);
					Ret = 0;
				}
				pthread_mutex_unlock(&(pCam->Lock));
			}
			if (Ret != 0) {
				if (pTask->pJxjRcd) {
					jcu_net_stream_close(pTask->pJxjRcd);
				}
				// 任务出错
				if (pTask->Cb) {
					pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, Ret);
				}

				jcu_user_handle_t *pJxjCam = NULL;

				pthread_mutex_lock(&pCam->Lock);
				oCamStat		= pCam->Stat;
				pCam->Stat		= CAM_STAT_ERRCOMM;
				pJxjCam			= pCam->pJxjCam;
				pCam->pJxjCam	= NULL;
				pCam->DeadLine	= -1;		// 设为-1，如果有任务就会尽早处理
				pthread_mutex_unlock(&pCam->Lock);

				if (oCamStat != CAM_STAT_ERRCOMM) {
					gCamHost.StatCb(pTask->CamId, CAM_STAT_ERRCOMM);
				}
				if (pJxjCam) {
					jcu_net_logout(pJxjCam);
				}
				free(pTask);
				pTask = NULL;
			}
		}
//		Log("Cam", LOG_LV_DEBUG, "Task start over");
	}

	return NULL;
}
/*
void tCamRcdCb(void* Arg, int CamId, int RecType, int Time, int Ret)
{
	printf("tCamRcdCb, Arg=%d, CamId=%d, RecType=%d, Time=%d, Ret=%d\n", (int)Arg, CamId, RecType, Time, Ret);
	char *t = CamCmdGetFile(CamId, RecType, Time);
	printf("CamCmdGetFile ret=%s\n", t);
}

void CamStatF(int CamId, int Stat)
{
	printf("Cam[%d] stat is changed to %d\n", CamId, Stat);
}

void SigHd(int SigNum)
{
	if (SigNum == SIGUSR1) {
		printf("CamCmdRecordNb ret=%d\n", CamCmdRecordNb(0, 'A', 5, time(NULL), tCamRcdCb, 0));
	}
}

int main(int argc, char *argv[])
{
	int Ret = CamInit(CamStatF);
	pthread_t t;

	printf("CamInit=%d\n", Ret);

	Ret = pthread_create(&t, NULL, CamThread, NULL);
	printf("pthread_create=%d\n", Ret);
	signal(SIGUSR1, SigHd);
	while(1) {
		sleep(1);
	}

	return 0;
}
*/
