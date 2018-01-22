#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../../include/cam.h"
#include "XzCamApi.h"
#include "../common/common.h"
#include "../log/log.h"

#define DEFAULT_BIND_PORT   10001
#define MAX_RETRY           5
#define COMM_INTVL          10
#define RETRY_INTVL         3
#define IN_CAM_IP           "169.254.254.3"
#define OUT_CAM_IP          "169.254.254.4"
#define RCD_FL_DIR			"/tmp/rcd/"

enum {
	CAM_CMD_NONE     = -1,
	CAM_CMD_RECORD,
	CAM_CMD_GETFILE,
	CAM_CMD_TYPES
};

typedef struct CamHostInfo  sCamHostInfo;
typedef struct CamInfo      sCamInfo;
typedef struct CamTask      sCamTask;
typedef struct CamTaskList  sCamTaskList;

struct CamTask {
	sCamTask    *Next;
	int         Type;               // ��������
	int         CamId;              // �����ID
	int         RecType;            // ¼�������
	int         RecLen;             // ¼��ĳ��ȣ���λ�룬��¼��ָ���õ�
	time_t      Time;               // ��ȡ�ļ�ʱ��¼��ʱ��ʱ���ǩ
	CamRcdCb    Cb;					// �ص�����
	void*       CbArg;
	double		AddT;				// ��������ʱ��
};

struct CamTaskList {
	pthread_mutex_t     lock;
	pthread_cond_t      cond;
	sCamTask            *Head;
};

struct CamInfo {
	int         Stat;                   //����ͷ�ĵ�ǰ״̬
	sCamTask    *pTask;                 //��ǰ������
	double      DeadLine;               //����������Ҫ��עһ�µ�ʱ��
	int         ErrCnt;
	int         CamIndex;               //ָ������ͷ��Ϣ
};

struct CamHostInfo {
	sXzCamHost      Host;
	sCamTaskList    TaskLst;
	sCamInfo        Cams[CAM_ID_NUM];
	CamStatCb		StatCb;
	uint8_t			ConnCamNum;
};

typedef struct CamGetFlFlg {
	pthread_mutex_t	Lock;
	pthread_cond_t	Cond;
	int 			Ret;
} sCamGetFlFlg;

const static char *CamIps[CAM_ID_NUM] = {
	IN_CAM_IP,
	OUT_CAM_IP
};

static sCamHostInfo gCamHost;

static int CamMkRecFilename(char *Buf, int CamId, int RecType, time_t Time)
{
	struct tm Localt;

	localtime_r(&Time, &Localt);
	return sprintf(Buf, "%c-%04d%02d%02d-%02d%02d%02d-%d.h264",
				   RecType, Localt.tm_year+1900, Localt.tm_mon+1, Localt.tm_mday,
				   Localt.tm_hour, Localt.tm_min, Localt.tm_sec, CamId);
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
/*
int CamCmdRecord(int CamId, int RecType, int RecLen, int Time)
{
    Log("Cam", LOG_LV_INFO, "CamCmdRecord, CamId=%d, T=%d, L=%d, Time=%d", CamId, RecType, RecLen, Time);
    if (CamId<0 || CamId>=CAM_ID_NUM || RecType<0 || RecType>0xFF || RecLen<=0) {
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
    pTask->Next     = NULL;

    if (CamAddTask(pTask) == 0) {
        return 0;
    } else {
        free(pTask);
        return -1;
    }
}
*/
int CamCmdRecordNb(int CamId, int RecType, int RecLen, int Time, CamRcdCb Cb, void *Arg)
{
	// �����������
	Log("Cam", LOG_LV_INFO, "CamCmdRecordNb, CamId=%d, T=%d, L=%d, Time=%d, Cb=%d, Arg=%d",
		CamId, RecType, RecLen, Time, (int)Cb, (int)Arg);
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

static void CamGetFlCb(sCamGetFlFlg* pFlg, int CamId, int RecType, int Time, int Ret)
{
	if (pFlg == NULL) {
		return;
	}
	pthread_mutex_lock(&pFlg->Lock);
	pFlg->Ret = Ret;
	pthread_cond_broadcast(&pFlg->Cond);
	pthread_mutex_unlock(&pFlg->Lock);
}

char* CamCmdGetFile(int CamId, int RecType, int Time)
{
	Log("Cam", LOG_LV_INFO, "CamCmdGetFile, CamId=%d, T=%d, Time=%d", CamId, RecType, Time);
	if (CamId<0 || CamId>=gCamHost.ConnCamNum || RecType>0xFF || Time<0) {
		return NULL;
	}
	sCamTask *pTask = calloc(1, sizeof(sCamTask));
	sCamGetFlFlg *pFlg= calloc(1, sizeof(sCamGetFlFlg));

	if (pTask==NULL || pFlg==NULL) {
		if (pTask) {
			free(pTask);
		}
		if (pFlg) {
			free(pFlg);
		}
		return NULL;
	}
	pthread_mutex_init(&pFlg->Lock, NULL);
	pthread_cond_init(&pFlg->Cond, NULL);
	pthread_mutex_lock(&pFlg->Lock);
	pFlg->Ret		= -1;

	pTask->CamId    = CamId;
	pTask->Type     = CAM_CMD_GETFILE;
	pTask->RecType  = RecType;
	pTask->Time     = Time;
	pTask->Next     = NULL;
	pTask->Cb		= (CamRcdCb)CamGetFlCb;
	pTask->CbArg	= pFlg;

	if (CamAddTask(pTask) == 0) {
		pthread_cond_wait(&pFlg->Cond, &pFlg->Lock);
		char *FlPath=NULL, tFlPath[100]=RCD_FL_DIR;

		CamMkRecFilename(tFlPath+strlen(tFlPath), CamId, RecType, Time);
		if (pFlg->Ret==0 && access(tFlPath, F_OK)==0) {
			FlPath = strdup(tFlPath);
		}
		pthread_mutex_unlock(&pFlg->Lock);
		pthread_mutex_destroy(&pFlg->Lock);
		pthread_cond_destroy(&pFlg->Cond);
		free(pFlg);
		return FlPath;
	} else {
		pthread_mutex_unlock(&pFlg->Lock);
		pthread_mutex_destroy(&pFlg->Lock);
		pthread_cond_destroy(&pFlg->Cond);
		free(pFlg);
		free(pTask);
		return NULL;
	}

}
/*

static int GenFileMd5(const char *FilePath)
{
    char cmd[256] = {0};
    char md5value[33] = {0};
    FILE *fp_in=NULL, *fp_out=NULL;

    sprintf(cmd, "md5sum %s > %s.tmp", FilePath, FilePath);
    system(cmd);

    //strcat(filename, ".md5");
    sprintf(cmd, "%s.tmp", FilePath);
    fp_in = fopen(cmd, "r");
    if (NULL == fp_in) {
        return -1;
    }

    sprintf(cmd, "%s.md5", FilePath);
    fp_out = fopen(cmd, "w");

    fread(md5value, 1, 32, fp_in);

    fwrite(md5value, 1, 32, fp_out);
    fclose(fp_in);
    fclose(fp_out);

    sprintf(cmd, "rm %s.tmp", FilePath);
    system(cmd);
    return 0;
}
static int CamUploadRecFile(sCamTask *pTask)
{
    int Ret = 0;
    char RecFileName[128] = {0};
    char RecFilePath[128] = {0};
    char RecFileMd5Path[128] = {0};		// ¼���ļ�����·��
    double st = BooTime();

    LedsCtrl(LED_SEND_FILE, LED_OPEN_OPR);
    //·��׼��
    CamMkRecFilename(RecFileName, pTask->CamId, pTask->RecType, pTask->Time);
    snprintf(RecFilePath, 128, RCD_FL_DIR"%s", RecFileName);
    // 1.����¼���ļ�������
    Recd_InitTCP(g_RecdServerInfo.cRecd_ip, g_RecdServerInfo.iRecd_port);
    Ret = RecordFileUpload(RecFilePath);
    Recd_CloseTCP();
//    Log("Cam", LOG_LV_DEBUG, "RecordFileUpload f ret=%d", Ret);
    // ����MD5�ļ�
    GenFileMd5(RecFilePath);
    snprintf(RecFileMd5Path, 128, "%s.md5", RecFilePath);
    // 2.����¼���ļ������ݵ�MD5
    Recd_InitTCP(g_RecdServerInfo.cRecd_ip, g_RecdServerInfo.iRecd_port);
    Ret = RecordFileUpload(RecFileMd5Path);
    Recd_CloseTCP();
//    Log("Cam", LOG_LV_DEBUG, "RecordFileUpload m ret=%d", Ret);
    Log("Cam", LOG_LV_INFO, "CamUploadRecFile is finished, dt:%.13G", BooTime()-st);

    LedsCtrl(LED_SEND_FILE, LED_CLOSE_OPR);
    // ɾ���ļ�
    unlink(RecFilePath);
    unlink(RecFileMd5Path);
//    sprintf(cmd, "rm %s", file_path);
//    printf("%s", cmd);
//    system(cmd);
//    sprintf(cmd, "rm %s", recd_md5_path);
//    printf("%s", cmd);
//    system(cmd);

    return Ret;
}
*/

int CamInit(int ConnCamNum, CamStatCb StatCb)
{
	if (ConnCamNum<0 || ConnCamNum>CAM_ID_NUM || StatCb == NULL) {
		return -1;
	}
	int Ret=-1, i=0;

	memset(&gCamHost, 0, sizeof(sCamHostInfo));
	gCamHost.StatCb = StatCb;
	gCamHost.ConnCamNum = ConnCamNum;
	for (i=0; i<gCamHost.ConnCamNum; i++) {
		gCamHost.Cams[i].CamIndex = -1;
	}

	if (pthread_mutex_init(&(gCamHost.TaskLst.lock), NULL) != 0) {
		return -1;
	}

	if (pthread_cond_init(&(gCamHost.TaskLst.cond), NULL) != 0) {
		return -1;
	}

	if (XzCamInitHost(&(gCamHost.Host), DEFAULT_BIND_PORT) < 0) {
		return -1;
	}
	XzCamSearchCams(&(gCamHost.Host), "br-lan", 1);

	//����Ӧ������ͷ�ҵ�û
	for (i=0; i<gCamHost.Host.CamNum; i++) {
		int j=0;

		for (j=0; j<gCamHost.ConnCamNum; j++) {
			if (gCamHost.Host.pCams[i].Addr.sin_addr.s_addr == inet_addr(CamIps[j])) {
				gCamHost.Cams[j].CamIndex = i;
				break;
			}
		}
	}
	for (i=0; i<gCamHost.ConnCamNum; i++) {
		if (gCamHost.Cams[i].CamIndex < 0) {
			Ret = XzCamAddCam(&(gCamHost.Host), CamIps[i], NULL);
			Log("Cam", LOG_LV_DEBUG, "r=%d, gCamHost.Host.CamNum=%d.", Ret, gCamHost.Host.CamNum);
			if (Ret >= 0) {
				gCamHost.Cams[i].CamIndex = Ret;
			} else {
				return -1;
			}
		}
		gCamHost.Cams[i].Stat       = CAM_STAT_INITED;
		gCamHost.StatCb(i, CAM_STAT_INITED);
	}

	return 0;
}

//����״̬�������´�ʱ��
double CamUpdateStat(void)
{
	int Ret=-1, i=0;
	double st=BooTime(), dt=COMM_INTVL;

	for (i=0; i<gCamHost.ConnCamNum; i++) {
		sCamInfo *pCam = gCamHost.Cams+i;
		double tdt = pCam->DeadLine - st;

		if (tdt <= 0) {
			//��ȡ״̬
			Ret = XzCamGetStat(&(gCamHost.Host), pCam->CamIndex);
			Log("Cam", LOG_LV_DEBUG, "XzCamGetStat[%d] ret=%d, s=%02X.", i, Ret, gCamHost.Host.pCams[pCam->CamIndex].Stat.bStat);
			if (Ret != 0) {
				pCam->ErrCnt++;
				pCam->DeadLine = st+RETRY_INTVL;
				if (pCam->ErrCnt > MAX_RETRY) {     //�����ж�
					pCam->ErrCnt = 0;

					//�����������ɾ���ýڵ�
					if (pCam->pTask) {
						sCamTask *pTask = pCam->pTask;

						pCam->pTask = pTask->Next;
						// �����¼�������лص�
						if (pTask->Type == CAM_CMD_RECORD) {
							if (pTask->Cb) {
								pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, -1);
							}
						}
						free(pTask);
						pTask=NULL;
						pCam->DeadLine = -1;
					}
					if (pCam->Stat != CAM_STAT_ERRCOMM) {
						Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_ERRCOMM);
						pCam->Stat = CAM_STAT_ERRCOMM;
						gCamHost.StatCb(i, CAM_STAT_ERRCOMM);
					}
					if (gCamHost.ConnCamNum == 1) {
						// ֻ��1������ͷʱ��ֱ����������ͷIP
						uint8_t Mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
						Log("Cam", LOG_LV_INFO, "Set Cam[%d] Ip to %s.", i, CamIps[0]);
						XzCamSetAddr(&(gCamHost.Host), "br-lan", CamIps[0], Mac);
					}
				}
			} else {
				sXzCamInfo *pXzCam = gCamHost.Host.pCams + pCam->CamIndex;

				if (pXzCam->Stat.bStat&0x1E || pXzCam->Stat.sStat.CardIn==0 ||
					pXzCam->Stat.sStat.IsRecord || pXzCam->Stat.sStat.IsReadFile ||
					pXzCam->Stat.sStat.IsVideo) {
					//״̬����
					if (pXzCam->Stat.sStat.IsRecord) {
						//�ֶ�ֹͣһ��
						XzCamStopRec(&gCamHost.Host, pCam->CamIndex);
					}
					pCam->ErrCnt++;
					pCam->DeadLine = st+RETRY_INTVL;
					if (pCam->ErrCnt > MAX_RETRY) {     //�����ж�
						pCam->ErrCnt = 0;

						//�����������ɾ���ýڵ�
						if (pCam->pTask) {
							sCamTask *pTask = pCam->pTask;

							pCam->pTask = pTask->Next;
							// �����¼�������лص�
							if (pTask->Type == CAM_CMD_RECORD) {
								if (pTask->Cb) {
									pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, -1);
								}
							}
							free(pTask);
							pTask=NULL;
							pCam->DeadLine = -1;
						}
						if (pCam->Stat != CAM_STAT_ERRCOND) {
							Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_ERRCOND);
							pCam->Stat = CAM_STAT_ERRCOND;
							gCamHost.StatCb(i, CAM_STAT_ERRCOND);
						}
					}
				} else {        //���������е�״̬
					// ���ָ���������״̬
					pCam->ErrCnt = 0;
					pCam->DeadLine = st+COMM_INTVL;
					if (pCam->Stat != CAM_STAT_STANDBY) {
						if (pCam->Stat==CAM_STAT_RECORDING && pCam->pTask) {
							sCamTask *pTask = pCam->pTask;
//							int ErrCnt = 0;
//
//                            // ����ļ��������
//							do {
//								// �Ȼ�ȡ�б�
//								Ret = XzCamGetFileList(&(gCamHost.Host), pCam->CamIndex);
//								Log("Cam", LOG_LV_DEBUG, "Cam[%d]:XzCamGetFileList 2, ret=%d", i, Ret);
//								if (Ret != 0) {
//									continue;
//								}
//								// ���Ҷ�Ӧ�ļ���ID
//								Ret = XzCamSearchFile(&(gCamHost.Host), pCam->CamIndex, pTask->RecType, pTask->Time-1, pTask->Time+1, NULL);
//								Log("Cam", LOG_LV_DEBUG, "Cam[%d] XzCamSearchFile 2, ret=%d", i, Ret);
//								if (Ret > 0) {
//									// �ҵ���
//									Ret = 0;
//								} else if (Ret == 0) {
//									// ��Ϊ�ļ��б��ѳɹ�����ȡ�������Ҳ������Ի����Ҳ���
//									ErrCnt = MAX_RETRY;
//								} else {
//									continue;
//								}
//								break;
//							} while (ErrCnt++ < MAX_RETRY);
//
//							Ret = ErrCnt<MAX_RETRY?0:-1;
							// �����¼�����񣬲鿴�Ƿ��лص�
							if (pTask->Type == CAM_CMD_RECORD) {
								if (pTask->Cb) {
									pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, Ret);
								}
							}
							// ��ɾ���ýڵ�
							pCam->pTask = pTask->Next;
							free(pTask);
							pTask=NULL;
							pCam->DeadLine = -1;
						}
						Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_STANDBY);
						pCam->Stat = CAM_STAT_STANDBY;
						gCamHost.StatCb(i, CAM_STAT_STANDBY);
					}
				}
			}
			tdt = pCam->DeadLine - st;
		}
		if (tdt < dt) {
			dt = tdt;
		}
	}

	return st+dt;
}

/** ����ͷ�̣߳���Ҫ����������У�
 *
 */
void* CamThread(void *Arg)
{
	int Ret, i;
	double et = 0;

	Log("Cam", LOG_LV_WARN, "CamThread is started.");
//    CamCmdGetFile(CAM_ID_OUT, 'A', 1430364547);

	// ���ڸ�������ͷ״̬��������Ƿ�������������ʱִ��
	for (et = CamUpdateStat(); 1; et = CamUpdateStat()) {
		sCamTask *pTask=NULL, **ppTask=NULL;
		//δ��ʱ�ȴ���ȡ������
		if (pthread_mutex_lock(&gCamHost.TaskLst.lock) == 0) {
			while (1) {
				if (gCamHost.TaskLst.Head) {
					pTask = gCamHost.TaskLst.Head;
					gCamHost.TaskLst.Head = pTask->Next;
					pTask->Next = NULL;
					break;
				}
				double Intvl = et - BooTime();
				if (Intvl <= 0) {
					//ʱ�䵽�Ͳ�����--
					break;
				}
				struct timespec to;

				clock_gettime(CLOCK_REALTIME, &to);
				to.tv_sec += Intvl+1;   // ������1��

				pthread_cond_timedwait(&gCamHost.TaskLst.cond, &gCamHost.TaskLst.lock, &to);
			}
			pthread_mutex_unlock(&gCamHost.TaskLst.lock);
		}

		// ������Ͳ�������
		if (pTask) {
			Log("Cam", LOG_LV_DEBUG, "CamTask[%X], i=%d,t=%d, l=%d, rt=%d, tm=%ld",(int)pTask , pTask->CamId, pTask->Type, pTask->RecLen, pTask->RecType, pTask->Time);
			sCamInfo *pCam = gCamHost.Cams+pTask->CamId;

			ppTask = &(pCam->pTask);
			while (*ppTask != NULL) {
				ppTask = &((*ppTask)->Next);
			}
			*ppTask = pTask;
			pTask = NULL;

			// ����ǵ�һ��������Ҫ�޸�dealine�Ա�ʶ������δ�����
			if (ppTask == &(pCam->pTask)) {
				pCam->DeadLine = -1;
			}
		}

		// ����������������ζԸ�����ͷ����
		for (i=0; i<gCamHost.ConnCamNum; i++) {
			sCamInfo *pCam = gCamHost.Cams+i;

			if (pCam->DeadLine < 0) {     //�����һ������û�������򣬵�ǰ����δ��ɣ���ִ�и�����ͷ����������
				// ���δ��������
				while (pCam->pTask) {
					int ErrCnt = 0;
					if (pCam->pTask->Type == CAM_CMD_RECORD) {  // ¼������
						pCam->ErrCnt = 0;

						if (pCam->pTask->AddT+300 < BooTime()) {
							// ̫���ˣ�����¼�ˣ�ɾ��������
							pTask = pCam->pTask;
							pCam->pTask = pTask->Next;
							// ����лص������������
							if (pTask->Cb) {
								pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, -1);
							}
							free(pTask);
							pTask = NULL;
							continue;
						}
						//���������
						while ((Ret = XzCamStartRec(&(gCamHost.Host), pCam->CamIndex, pCam->pTask->RecType, pCam->pTask->Time, pCam->pTask->RecLen))!=0 &&
							   ErrCnt++<MAX_RETRY) {
							sleep(1);
						}
						if (ErrCnt >= MAX_RETRY) {      // ʧ����
							Log("Cam", LOG_LV_INFO, "Cam[%d]: Failed to record", i);
							pCam->DeadLine  = BooTime() + RETRY_INTVL;		// ��������û�����⣬��Ϊ��while�У���ʹ������Ϊ-1,Ҳ��ִ����һ������
							if (pCam->Stat!=CAM_STAT_ERRCOMM && pCam->Stat!=CAM_STAT_ERRCOND) {
								Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_ERRCOMM);
								pCam->Stat      = CAM_STAT_ERRCOMM;
								gCamHost.StatCb(i, CAM_STAT_ERRCOMM);
							}
							// ɾ��������
							pTask = pCam->pTask;
							pCam->pTask = pTask->Next;
							// ����лص������������
							if (pTask->Cb) {
								pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, -1);
							}
							free(pTask);
							pTask = NULL;
						} else {    // ָ��ͳɹ�
							Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_RECORDING);
							pCam->Stat      = CAM_STAT_RECORDING;
							gCamHost.StatCb(i, CAM_STAT_RECORDING);
							pCam->DeadLine  = BooTime()+pCam->pTask->RecLen+3;
							break;
						}
					} else if (pCam->pTask->Type == CAM_CMD_GETFILE) {  // ��ȡ�ļ�����
						pCam->ErrCnt    = 0;
						Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_GETFILE);
						pCam->Stat = CAM_STAT_GETFILE;
						gCamHost.StatCb(i, CAM_STAT_GETFILE);
						do {
							// �Ȼ�ȡ�б�
							Ret = XzCamGetFileList(&(gCamHost.Host), pCam->CamIndex);
							Log("Cam", LOG_LV_DEBUG, "Cam[%d]:XzCamGetFileList, ret=%d", i, Ret);
							if (Ret != 0) {
								continue;
							}
							// ���Ҷ�Ӧ�ļ���ID
							sXzCamFileInfo *pFiles = NULL;
							Ret = XzCamSearchFile(&(gCamHost.Host), pCam->CamIndex, pCam->pTask->RecType, pCam->pTask->Time-1, pCam->pTask->Time+1, &pFiles);
							Log("Cam", LOG_LV_DEBUG, "Cam[%d] XzCamSearchFile, ret=%d", i, Ret);
							if (Ret <= 0) {
								// �Ҳ����Ͳ�Ҫ����
								ErrCnt = MAX_RETRY;
								continue;
							}
							char FileName[128] = {0};
							char FilePath[256] = {0};

							// ׼��·��
							CamMkRecFilename(FileName, pCam->pTask->CamId, pCam->pTask->RecType, pCam->pTask->Time);
							snprintf(FilePath, 100, RCD_FL_DIR"%s", FileName);
							//ֻȡ��һ�������������ļ�
							Ret = XzCamGetFile(&(gCamHost.Host), pCam->CamIndex, pFiles, FilePath);
							Log("Cam", LOG_LV_DEBUG, "Cam[%d] XzCamGetFile, ret=%d", i, Ret);
							free(pFiles);
							if (Ret != 0) {
								unlink(FilePath);
								continue;
							}
							// �ϴ��ļ�
//                            CamUploadRecFile(pCam->pTask);
							break;
						} while (ErrCnt++ < MAX_RETRY);

						// �ж��Ƿ�ɹ�
						if (ErrCnt < MAX_RETRY) {   // �ɹ���
							Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_STANDBY);
							pCam->Stat = CAM_STAT_STANDBY;
							gCamHost.StatCb(i, CAM_STAT_STANDBY);
							pCam->DeadLine = BooTime() + COMM_INTVL;
							pTask = pCam->pTask;
							if (pTask->Cb) {
								pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, 0);
							}
							pTask = NULL;
						} else {                    // ʧ����
							Log("Cam", LOG_LV_INFO, "Cam[%d]: Failed to getfile", i);
							pCam->DeadLine = BooTime() + RETRY_INTVL;		// ��������û�����⣬��Ϊ��while�У���ʹ������Ϊ-1,Ҳ��ִ����һ������
							if (pCam->Stat!=CAM_STAT_ERRCOMM && pCam->Stat!=CAM_STAT_ERRCOND) {
								Log("Cam", LOG_LV_INFO, "CamStat[%d], o=%d,n=%d", i, pCam->Stat, CAM_STAT_ERRCOMM);
								pCam->Stat = CAM_STAT_ERRCOMM;
								gCamHost.StatCb(i, CAM_STAT_ERRCOMM);
							}
							pTask = pCam->pTask;
							if (pTask->Cb) {
								pTask->Cb(pTask->CbArg, pTask->CamId, pTask->RecType, pTask->Time, -1);
							}
							pTask = NULL;
						}
						// ɾ��������
						pTask = pCam->pTask;
						pCam->pTask = pTask->Next;
						free(pTask);
						pTask = NULL;
					} else {
						Log("Cam", LOG_LV_DEBUG, "Failed to recognise task[%d]", pCam->pTask->Type);
						// �޷�ʶ��ɾ��������
						pTask = pCam->pTask;
						pCam->pTask = pTask->Next;
						free(pTask);
						pTask = NULL;
					}
				}
			}
		}
	}

	return NULL;
}
