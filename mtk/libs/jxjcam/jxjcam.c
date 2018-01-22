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
	double				AddT;				// �������ʱ��
	jcu_stream_handle_t	*pJxjRcd;			// ¼��Ķ���
	int					Type;				// ��������
	int					CamId;				// �����ID
	int					RecType;			// ¼�������
	int					RecLen;				// ¼��ĳ��ȣ���λ�룬��¼��ָ���õ�
	time_t				Time;				// ��ȡ�ļ�ʱ��¼��ʱ��ʱ���ǩ
	CamRcdCb			Cb;					// �ص�����
	void*				CbArg;
};

struct CamTaskList {
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
	sCamTask		*Head;
};

struct CamInfo {
	pthread_mutex_t		Lock;					// ������ͷ��Ϣ�����޸ĵı��������������¸���
	sCamTask			*pTask;					// ��ǰ�����񣬽�CamThread���޸ģ���ֻ��ȡ
	int					Stat;					// ����ͷ�ĵ�ǰ״̬
	double				DeadLine;				// ����������Ҫ��עһ�µ�ʱ��
	jcu_user_handle_t	*pJxjCam;				// ���Ž�����ͷ���󣬽�CamThread���ã���ֻ����
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
	"192.168.0.140"			//��Ϊ��Ч��IP��������������IP��ͻ������ͷIP��Ϊ��
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
		pCam->DeadLine	= -1;		// ��Ϊ-1�����������ͻᾡ�紦��
		pthread_mutex_unlock(&pCam->Lock);


		// û����ȥ���ո��߳���Դ�ˣ�ֱ������Ϊ����Ҫ����
		pthread_attr_init(&ta);
		pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
		// ���ж�ʧ���ˣ�ʧ���ˣ���Ҳ��֪����ô������
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
			pCam->DeadLine	= -1;		// ��Ϊ-1�����������ͻᾡ�紦��
			pthread_mutex_unlock(&pCam->Lock);

			// û����ȥ���ո��߳���Դ�ˣ�ֱ������Ϊ����Ҫ����
			pthread_attr_init(&ta);
			pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

			// ���ж�ʧ���ˣ�ʧ���ˣ���Ҳ��֪����ô������
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
				// ��ʱΪ¼��״̬����״̬��һ������ptask
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
			pCam->DeadLine	= -1;		// ��Ϊ-1�����������ͻᾡ�紦��
			pthread_mutex_unlock(&pCam->Lock);

			// û����ȥ���ո��߳���Դ�ˣ�ֱ������Ϊ����Ҫ����
			pthread_attr_init(&ta);
			pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
			// ���ж�ʧ���ˣ�ʧ���ˣ���Ҳ��֪����ô������
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
				// ��ʱΪ¼��״̬����״̬��һ������ptask
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
		pCam->DeadLine	= -1;		// ��Ϊ-1�����������ͻᾡ�紦��
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
	//����
	if (pthread_mutex_lock(&(gCamHost.TaskLst.lock)) != 0) {
		return -1;
	}
	//��������
	while (*ppTaskList != NULL) {
		ppTaskList = &(ppTaskList[0]->Next);
	}
	*ppTaskList = pTask;
	//֪ͨ������
	pthread_cond_signal(&(gCamHost.TaskLst.cond));
	pthread_mutex_unlock(&(gCamHost.TaskLst.lock));

	return 0;
}

//
int CamCmdRecordNb(int CamId, int RecType, int RecLen, int Time, CamRcdCb Cb, void *Arg)
{
	// �����������
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

// ���ػ�õ�¼���ļ�����ʹ����ǵ�����(����free����ֵ��unlink�ļ�)
char* CamCmdGetFile(int CamId, int RecType, int Time)
{
	char FlName[100] = RCD_FL_DIR;

	// TODO:��ȡ¼���ļ�
	
	// ���¼��Ŀ¼���Ƿ���ڣ������򷵻��ļ��������򷵻ش���
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

	// �޸�·�ɱ������鲥��ַ
	while (system("route del "GROUP_IP " > /dev/null 2>&1") == 0);
	system("route add "GROUP_IP" br-lan");

	// �ṹ�������Ϣ��ʼ��
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

	// ���Ž�����ͷ��س�ʼ��
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
		// ��������ͷ¼��ֹͣ����
		for (i=0; i<gCamHost.ConnCamNum; i++) {
			sCamInfo *pCam = gCamHost.Cams+i;
			if (pthread_mutex_lock(&pCam->Lock)) {
				continue;
			}
			// Ŀǰ����¼������,ֹͣ¼��ͬʱ�ص�
			if (pCam->pTask && pCam->DeadLine<BooTime()) {
				int nCamStat=-1, oCamStat=-1;
				sCamTask *pTask = pCam->pTask;
				jcu_user_handle_t *pJxjCam = NULL;

				// ��ȡ������
				pCam->pTask = NULL;
				pthread_mutex_unlock(&pCam->Lock);
				// ֹͣ¼��
				if (jcu_net_stream_close(pTask->pJxjRcd)) {
					nCamStat = CAM_STAT_ERRCOMM;
				} else {
					nCamStat = CAM_STAT_STANDBY;
				}
				// ����״̬
				pthread_mutex_lock(&pCam->Lock);
				oCamStat = pCam->Stat;
				pCam->Stat = nCamStat = (oCamStat==CAM_STAT_ERRCOMM?oCamStat:nCamStat);
				if (nCamStat != CAM_STAT_STANDBY) {
					pJxjCam = pCam->pJxjCam;
					pCam->pJxjCam = NULL;
				}
				pthread_mutex_unlock(&pCam->Lock);
				// ����ʱ�رն���
				if (oCamStat != nCamStat) {
					gCamHost.StatCb(i, nCamStat);
				}
				if (pJxjCam) {
					jcu_net_logout(pJxjCam);
				}
				char tBuf[100] = RCD_FL_DIR;
				struct stat FlStat = {0};
				// �ж��ļ��Ƿ����
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
		// ����ͷ����Ƚ�һ����������������������ͷ
		Ret = 0;
		uint8_t CamIdleFlgs[CAM_ID_NUM] = {0};	// 0��ʶæµ��������ʶ����
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
			// ������ͷδ���ӣ���Ҫ����������
			sCamSearchInfo ScInfo = {0};
			mb_cu_notify_t mbQueryInfo = {.user_arg=&ScInfo, .callback=QueryCb};

			jcu_mb_query_t *pQuery= jcu_mb_query(J_MB_NetCfg_Id, &mbQueryInfo);
			if (pQuery) {
				sleep(1);
				jcu_mb_release(pQuery);
			}
			usleep(200000);
			if (ScInfo.Num > 0) {
				uint8_t	CamFlgs1[CAM_ID_NUM] = {0};		// ��¼��Ӧ����ͷ��ScInfo�е���ţ�0xFF��Ǹ�����ͷδ�ҵ�ƥ����Ϣ
				uint8_t	*CamFlgs2 = calloc(1, ScInfo.Num*sizeof(uint8_t));
				// ��¼ScInfo������ͷ��ƥ�䣬�Լ���Ӧ��ID��0xFF��Ǹ�����ͷδ�ҵ�ƥ����Ϣ��0xFE������Ǹ�����ͷ��Ϣ�Ƿ���Ҫ�޸�

				memset(CamFlgs1, 0xFF, CAM_ID_NUM);
				memset(CamFlgs2, 0xFF, ScInfo.Num);
				// ���ҵ����ʵ�
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
				// ����Ƿ���û���úõ�
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
				// ��������һ��
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
			// ��¼
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
		// ȡ������
		sCamTask *pTask=NULL, **ppTask=NULL;
		//δ��ʱ�ȴ���ȡ������
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
							// ̫���ˣ�����¼�ˣ�ɾ��������
							// ����лص������������
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
					//ʱ�䵽�Ͳ�����--
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
		// ��������ʼ����
		if (pTask) {
			sCamInfo *pCam = gCamHost.Cams+pTask->CamId;
			char tBuf[100] = RCD_FL_DIR;
			int oCamStat = -1;

			Log("Cam", LOG_LV_DEBUG, "Task CamId=%d", pTask->CamId);
			// ɾ��ԭ�ļ�
			Ret = -1;
			CamMkRecFilename(tBuf+strlen(tBuf), pTask->CamId, pTask->RecType, pTask->Time);
			unlink(tBuf);
			if (pthread_mutex_lock(&(pCam->Lock)) == 0) {
				if (pCam->pJxjCam) {
					jcu_stream_cb_t sc = {pCam, StreamCb};
					jcu_notify_cb_t nc = {JCU_OP_ASYNC, (void *)pTask->CamId, StreamNtCb};

					oCamStat		= pCam->Stat;
					pCam->DeadLine	= BooTime()+5;		// 5����û��ʼ¼�͹ص�
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
				// �������
				if (pTask->Cb) {
					pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, Ret);
				}

				jcu_user_handle_t *pJxjCam = NULL;

				pthread_mutex_lock(&pCam->Lock);
				oCamStat		= pCam->Stat;
				pCam->Stat		= CAM_STAT_ERRCOMM;
				pJxjCam			= pCam->pJxjCam;
				pCam->pJxjCam	= NULL;
				pCam->DeadLine	= -1;		// ��Ϊ-1�����������ͻᾡ�紦��
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
