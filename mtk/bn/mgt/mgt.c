#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include "../tcp.h"
#include "../FileSend.h"
#include "../BusinessProtocol.h"
#include "../ProtocolDefine.h"
#include "mgt.h"
#include "../../include/cam.h"
#include "../../include/common.h"
#include "../../include/log.h"
#include "../main_linuxapp.h"
#include "../cfgFile.h"
#include "../sd/sdmgt.h"

//#include "../tcp.h"

typedef struct MgtRecordTask sMgtRecordTask;
struct MgtRecordTask {
	sMgtRecordTask*	Next;	// ָ����һ������
	double			AddT;	// ������ӵ�ʱ��

	uint8_t*		Data;
	uint32_t 		DataL;
	uint8_t			Flag;	// ����ִ��״̬�ı�ʶ
};

typedef struct RecordTaskList {
	sMgtRecordTask*	Head;
	pthread_mutex_t	Lock;
	pthread_cond_t	Cond;
} sRecordTaskList;

typedef struct CamDl {
	void*		handle;
	int (*CamInit)(int ConnCamNum, CamStatCb StatCb);
	int (*CamCmdRecordNb)(int CamId, int RecType, int RecLen, int Time, CamRcdCb Cb, void *Arg);
	// ���ػ�õ�¼���ļ�����ʹ����ǵ�����(��������ֵ���ļ�)
	char* (*CamCmdGetFile)(int CamId, int RecType, int Time);
	void* (*CamThread)(void *Arg);
} sCamDl;

/* ¼���ʵʱ��������IP��PORT��Ϣ */
typRecdServerInfo g_RecdServerInfo;
static sCamDl CamFuncs = {0};
static sRecordTaskList RcdTskLst;
static pthread_mutex_t SendLock;
static uint8_t ConnCamNum = 0;

// ���������
char	CamType = 'J';

static void CamStat(int CamId, int nStat)
{
	if (CamId>=0 && CamId<CAM_ID_NUM) {
		switch (nStat) {
		case CAM_STAT_INITING:
		case CAM_STAT_INITED:
		case CAM_STAT_ERRCOMM:
		case CAM_STAT_ERRCOND:
			LedsCtrl(LED_CAM1_ONLINE+CamId, LED_CLOSE_OPR);
			break;
		case CAM_STAT_STANDBY:
		case CAM_STAT_RECORDING:
		case CAM_STAT_LIVING:
			LedsCtrl(LED_CAM1_ONLINE+CamId, LED_OPEN_OPR);
			break;
		default:
		case CAM_STAT_GETFILE:
			break;
		}
	}
	Log("Cam", LOG_LV_DEBUG, "Cam[%d]'s stat is changed to %d.", CamId, nStat);
}

int MgtInit(void)
{
	char *p = GetCfgStr(CFG_FTYPE_CFG, "Sys", "CamType", "J");
	if (p) {
		CamType = p[0];
		free(p);
		p = NULL;
	}
	
	if (CamType == 'X' || CamType == 'x') {
		CamFuncs.handle = dlopen("libxzcam.so", RTLD_NOW);
	} else {
		CamFuncs.handle = dlopen("libjxjcam.so", RTLD_NOW);
	}
	if (CamFuncs.handle == NULL) {
		Log("MGT", LOG_LV_DEBUG, "Failed to open libjxjcam.so, errmsg:%s.", dlerror());
		return -1;
	}
	if ((CamFuncs.CamInit = dlsym(CamFuncs.handle, "CamInit"))==NULL ||
			(CamFuncs.CamCmdRecordNb =dlsym(CamFuncs.handle, "CamCmdRecordNb"))==NULL ||
			(CamFuncs.CamCmdGetFile = dlsym(CamFuncs.handle, "CamCmdGetFile"))==NULL ||
			(CamFuncs.CamThread = dlsym(CamFuncs.handle, "CamThread"))==NULL) {
		Log("MGT", LOG_LV_DEBUG, "Failed to dlsym, errmsg:%s.", dlerror());
		return -1;
	}
	p = GetCfgStr(CFG_FTYPE_CFG, "Sys", "CamNum", "1");
	if (p) {
		ConnCamNum = strtol(p, NULL, 10);
		if (ConnCamNum<0 || ConnCamNum>2) {
			ConnCamNum = 1;
		}
		free(p);
		p=NULL;
	} else {
		ConnCamNum = 1;
	}
	Log("MGT", LOG_LV_DEBUG, "CamNum=%d.", ConnCamNum);

	memset(&RcdTskLst, 0, sizeof(sRecordTaskList));
	pthread_mutex_init(&SendLock, NULL);
	pthread_mutex_init(&(RcdTskLst.Lock), NULL);
	pthread_cond_init(&(RcdTskLst.Cond), NULL);
	CamFuncs.CamInit(ConnCamNum, CamStat);

	return 0;
}

//��������Ƿ�ִ����ɣ���ɵĻ�������ظ�
void* MgtThread(void* Arg)
{
	pthread_t Cam;

	if (pthread_create(&Cam, NULL, CamFuncs.CamThread, NULL) != 0) {
		perror("can not create CamThread!\n");
		exit(EXIT_FAILURE);
	}
	while (pthread_mutex_lock(&(RcdTskLst.Lock))) {
		Log("MGT", LOG_LV_DEBUG, "Failed to lock RcdTskLst.Lock 1, errno:%d", errno);
		sleep(1);
	}

	while (1) {
		sMgtRecordTask **ppTask=NULL, *pTask=NULL;

		ppTask = &(RcdTskLst.Head);
		while (*ppTask) {
			if (((*ppTask)->AddT+300 < BooTime()) ||
					((*ppTask)->Flag == ((1<<ConnCamNum) - 1))) {
				//��ʱ��������ɣ�ȡ��������
				pTask = *ppTask;
				*ppTask = pTask->Next;
				pTask->Next = NULL;
				break;
			}
			ppTask = &(*ppTask)->Next;
		}
		if (pTask) {
			pthread_mutex_unlock(&(RcdTskLst.Lock));

			BusinessPackage(RCMD_UploadRecord, 0, g_SystermInfo.BusinessSeq++, pTask->Data, pTask->DataL, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			free(pTask->Data);
			free(pTask);
			pTask = NULL;

			while (pthread_mutex_lock(&(RcdTskLst.Lock))) {
				Log("MGT", LOG_LV_DEBUG, "Failed to lock RcdTskLst.Lock 2, errno:%d", errno);
				sleep(1);
			}
		} else {
			struct timespec to = {0};

			clock_gettime(CLOCK_REALTIME, &to);
			to.tv_sec += 1;
			pthread_cond_timedwait(&(RcdTskLst.Cond), &(RcdTskLst.Lock), &to);
		}
	}

	return NULL;
}


static int CamMkRecFilename(char *Buf, int CamId, int RecType, time_t Time)
{
	struct tm Localt;

	localtime_r(&Time, &Localt);
	return sprintf(Buf, "%c-%04d%02d%02d-%02d%02d%02d-%d.h264",
				   RecType, Localt.tm_year+1900, Localt.tm_mon+1, Localt.tm_mday,
				   Localt.tm_hour, Localt.tm_min, Localt.tm_sec, CamId);
}

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

static int CamUploadRecFile(char *RecFilePath)
{
	int Ret = 0;
	char RecFileMd5Path[128] = {0};		// ¼���ļ�����·��
	double st = BooTime();

	pthread_mutex_lock(&SendLock);
//    LedsCtrl(LED_SEND_FILE, LED_OPEN_OPR);
	// 1.����¼���ļ�������
	Recd_InitTCP(g_RecdServerInfo.cRecd_ip, g_RecdServerInfo.iRecd_port);
	Ret = RecordFileUpload(RecFilePath);
	Recd_CloseTCP();
	Log("Cam", LOG_LV_DEBUG, "RecordFileUpload f ret=%d", Ret);
	// ����MD5�ļ�
	GenFileMd5(RecFilePath);
	snprintf(RecFileMd5Path, 128, "%s.md5", RecFilePath);
	// 2.����¼���ļ������ݵ�MD5
	Recd_InitTCP(g_RecdServerInfo.cRecd_ip, g_RecdServerInfo.iRecd_port);
	Ret = RecordFileUpload(RecFileMd5Path);
	Recd_CloseTCP();
	Log("Cam", LOG_LV_DEBUG, "RecordFileUpload m ret=%d", Ret);
	pthread_mutex_unlock(&SendLock);
//    Log("Cam", LOG_LV_DEBUG, "RecordFileUpload m ret=%d", Ret);
	Log("Cam", LOG_LV_INFO, "CamUploadRecFile is finished, dt:%.13G", BooTime()-st);

//    LedsCtrl(LED_SEND_FILE, LED_CLOSE_OPR);
	// ɾ���ļ�
#if 1
	unlink(RecFilePath);
	unlink(RecFileMd5Path);
#endif
//    sprintf(cmd, "rm %s", file_path);
//    printf("%s", cmd);
//    system(cmd);
//    sprintf(cmd, "rm %s", recd_md5_path);
//    printf("%s", cmd);
//    system(cmd);

	return Ret;
}

void CamUpFlThread(int *Args)
{
	int CamId=Args[0], RecType=Args[1], Time=Args[2];
	char *FlPath=NULL, nFlPath[100]="/tmp/rcd/";

	free(Args);
	Args = NULL;

	if (CamType == 'J' || CamType == 'j') {
		// ��SD����ȡ¼���ļ���cp��/tmp/rcd/Ŀ¼��
		SdFileOprB(SD_CMD_RD, CamId, RecType, Time);
	}
	CamMkRecFilename(nFlPath+strlen(nFlPath), CamId, RecType, Time);
#if 0
	if (ProtVer/16 == 2 && (CamType == 'J' ||CamType == 'j')) {
		strcat(nFlPath, "n");
	}
#endif
	FlPath = CamFuncs.CamCmdGetFile(CamId, RecType, Time);
	if (FlPath) {
		if (rename(FlPath, nFlPath)) {
			unlink(FlPath);
		} else {
			CamUploadRecFile(nFlPath);
		}
		free(FlPath);
	}
}

int CamUpFlNb(int CamId, int RecType, int Time)
{
	int *Args = calloc(3, sizeof(int));
	int Ret=-1;
	pthread_t t;
	pthread_attr_t ta;

	Args[0] = CamId;
	Args[1] = RecType;
	Args[2] = Time;

	// û����ȥ���ո��߳���Դ�ˣ�ֱ������Ϊ����Ҫ����
	pthread_attr_init(&ta);
	pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

	Ret = pthread_create(&t, &ta, (void*(*)(void*))CamUpFlThread, Args);
	pthread_attr_destroy(&ta);
	if (Ret == 0) {
		return 0;
	} else {
		free(Args);
		return -1;
	}
}

static int MkCamFlRcd3(uint8_t *Buf, int CamId, int RecType, int Time)
{
	int Len1=0, Len2=0;
	uint8_t tBuf1[256], tBuf2[256];
	uint32_t tCode=CamId+CAM_DCODE_BASE;

	// �����L_fileRecord3
	tBuf1[0] = 2;															// ����¼���ļ�
	tBuf1[1] = CamMkRecFilename((char*)(tBuf1+2), CamId, RecType, Time);	// ���ֳ���
	Len2 = TLVEncode(TAG_L_fileRecord3, tBuf1[1]+2, tBuf1, 0, tBuf2);

	// ����D_codeһ����D_record���ݷ���tBuf1��
	Len1 = TLVEncode(TAG_D_code, DevCodeSize, &tCode, 1, tBuf1);
	if (ProtVer/16 == 2) {
		Len1+= TLVEncode(TAG_D_data, Len2, tBuf2, 0, tBuf1+Len1);
	} else {
		tCode= 0;
		Len1+= TLVEncode(TAG_M_secureHead, 1, &tCode, 0, tBuf1+Len1);
		Len1+= TLVEncode(TAG_M_secureData, Len2, tBuf2, 0, tBuf1+Len1);
	}

	// ����D_record��TLV
	return TLVEncode(TAG_D_record, Len1, tBuf1, 0, Buf);
}


// ��������ִ�����ʱ���ã��޸Ķ�Ӧ�������ʧ��ʱֱ�ӷ���
int MgtCamRcdCb(sMgtRecordTask *pTask, int CamId, int RecType, int Time, int Ret)
{
	Log("MGT", LOG_LV_DEBUG, "MgtCamRcdCb[%d], d=%d, t=%d, T=%d, r=%d", (int)pTask, CamId, RecType, Time, Ret);
	int Len = 0;
	uint8_t tBuf[256] = {0};

	if (Ret == 0) {
		// ¼��ɹ��Ļ���׼���ϴ�������
		Len = MkCamFlRcd3(tBuf, CamId, RecType, Time);
		if (ProtVer/16 != 2) {
			// 3.xЭ����Ҫ�����ϴ�
			CamUpFlNb(CamId, RecType, Time);
		} else {
			if (CamType == 'X' || CamType == 'x') {
				// Ѷ�����������
				if (RecType == ALARM_RECORD) {
					// ����¼����Ҫ�����ϴ����������ȥ
					CamUpFlNb(CamId, ALARM_RECORD, Time);
				}
			} else {
				// ���Ž����������
				// �洢¼���ļ�(�ɹ���ɾ��/tmp/rcd/*�ļ���ʧ����Ҫɾ�����ļ�)
				if (SdFileOprB(SD_CMD_WR, CamId, RecType, Time)) {
					CamUpFlNb(CamId, RecType, Time);
				} else {
					//if (RecType == ALARM_RECORD)
					{
						// ����¼����Ҫ�����ϴ����������ȥ
						CamUpFlNb(CamId, RecType, Time);
					}
				}
			}
		}
	}

	if (pTask) {
		// ������ʱ�����������������
		if (pthread_mutex_lock(&(RcdTskLst.Lock)) == 0) {
			sMgtRecordTask **ppTask = NULL;

			ppTask = &(RcdTskLst.Head);
			while (*ppTask) {
				if (*ppTask == pTask) {
					break;
				}
				ppTask = &((*ppTask)->Next);
			}
			if (*ppTask) {
				// �ҵ���ԭ����
				if (Ret == 0) {
					uint8_t *t = realloc(pTask->Data, pTask->DataL+Len);
					if (t) {
						pTask->Data  = t;
						memcpy(pTask->Data+pTask->DataL, tBuf, Len);
						pTask->DataL+= Len;
					} else {
						// ���ָ���Ҳ�ǳ�С
						pTask = NULL;
					}
				}
				(*ppTask)->Flag |= 1<<CamId;
				pthread_cond_broadcast(&(RcdTskLst.Cond));
			} else {
				pTask = NULL;
			}

			pthread_mutex_unlock(&(RcdTskLst.Lock));
		} else {
			// �����ϲ�����
			pTask = NULL;
		}
	}
	if (pTask==NULL && Ret==0) {
		// �ϴ�
		BusinessPackage(RCMD_UploadRecord, BUSINESS_STATE_OK, g_SystermInfo.BusinessSeq++, tBuf, Len, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
	}

	return 0;
}

int MgtLockRecord(int RecType, int Time, const uint8_t *Data, int DataLen)
{
	Log("MGT", LOG_LV_DEBUG, "MgtLockRecord, t=%d, T=%d, l=%d.", RecType, Time, DataLen);
	if (Data==NULL || DataLen<=0) {
		return -1;
	}
	int Ret = 0;
	sMgtRecordTask *pTask = NULL;

	if (RecType >= 0) {
		// ��Ҫ¼�����ｨ�����񲢲���
		Ret = -2;
		if (ProtVer/16 == 2) {
			// 2.xЭ����Ҫ�ϲ��ϴ���3.xЭ�鲻��Ҫ
			pTask = calloc(1, sizeof(sMgtRecordTask));
			if (pTask != NULL) {
				pTask->AddT	= BooTime();
				pTask->DataL= DataLen;
				pTask->Data	= malloc(DataLen);
				if (pTask->Data) {
					memcpy(pTask->Data, Data, DataLen);
					// �������
					if (pthread_mutex_lock(&(RcdTskLst.Lock)) == 0) {
						sMgtRecordTask **ppTask = &(RcdTskLst.Head);

						// �����β
						while (*ppTask) {
							ppTask = &((*ppTask)->Next);
						}
						*ppTask = pTask;
						Ret = 0;		//��ǲ���ɹ�
						pthread_mutex_unlock(&RcdTskLst.Lock);
					}
				}
			}
		}
		if (Ret && pTask) {
			// ʧ��������
			if (pTask->Data) {
				free(pTask->Data);
			}
			free(pTask);
			pTask = NULL;
		}
		// ����¼��
		int RecLen = 0;

		if (RecType == NORMAL_RECORD) {
			RecLen = g_RecdServerInfo.normal_recd_time;
		} else {
			RecLen = g_RecdServerInfo.alarm_recd_time;
		}
		int Flag = 0, i=0;

		for (i=0; i<ConnCamNum; i++) {
			// ����¼��
			if (CamFuncs.CamCmdRecordNb(i, RecType, RecLen, Time, (CamRcdCb)MgtCamRcdCb, pTask)) {
				// ��Ǹ������¼��ʧ��
				Flag |= 1<<i;
			}
		}
		if (Flag==((1 << ConnCamNum) - 1) && pTask) {
			// ȫ��¼��ʧ�ܣ��������ٵȴ������ԭ����
			if (pthread_mutex_lock(&(RcdTskLst.Lock)) == 0) {
				sMgtRecordTask **ppTask = &(RcdTskLst.Head);

				// ���Ÿ����� ��ȡ��
				while (*ppTask) {
					if (*ppTask == pTask) {
						*ppTask = pTask->Next;
						break;
					} else {
						ppTask = &((*ppTask)->Next);
					}
				}
				pthread_mutex_unlock(&RcdTskLst.Lock);
			}
			// ����ռ�
			free(pTask->Data);
			free(pTask);
			pTask = NULL;
			// ���÷���ֵ
			Ret = -3;
		}
	}
	if (!pTask) {
		// ����δ�ܽ����ɹ�����Ҫ��������ֱ�ӷ���
		BusinessPackage(RCMD_UploadRecord, 0, g_SystermInfo.BusinessSeq++, Data, DataLen, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
	}

	return Ret;
}
