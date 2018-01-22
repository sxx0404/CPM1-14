#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>

#include "../../include/common.h"
#include "../../include/log.h"
#include "frsdb.h"

#define FRAME_DB_FILE	"/home/frames.db"
#define TRIM_THR		102400	// 整理阈值，空闲空间大于此值进行一次整理

#pragma pack(push, 1)           // 结构体定义时设置为按字节对齐
typedef struct SavedFr {
	uint16_t	StLen;			// 本包数据的总长度
	uint16_t	FrId;			// 本结构体的唯一ID，0标识该包已确认，可以删除
	uint32_t	Seq;			// 数据包的指令序号，用于删除的标记
	uint8_t		HdRvCs;			// 本结构体头部校验码，从StLen一直到DtRvCs结束，注：生成时先计算DtRvCs再计算HdRvCs
	uint8_t		DtRvCs;			// 数据域（Data）校验码
	uint8_t		Data[0];		// 原始数据
} sSavedFr;
#pragma pack(pop)               //恢复原对齐方式

typedef struct CachedFr {
	sSavedFr	*pFr;
	off_t		Offset;			// 本条记录在文件中的位置, (off_t)-1则标识位置不可信
	double		SendT;
} sCachedFr;

// TODO可以用链表
static sCachedFr *pCcFrs = NULL;
static uint16_t	SavedFrNum = 0;
static pthread_mutex_t Mut;


// 初始化，成功后返回0，否则-1
int FrDbInit(void)
{
	// 锁初始化
	pthread_mutex_init(&Mut, NULL);
	// 读取文件
	int Fd = -1;
	char *tData = NULL;
	int tDataLen = 0;
	int Ret = 0;

	Fd = open(FRAME_DB_FILE, O_RDONLY);
	if (Fd < 0) {
		if (errno == ENOENT) {
			return 0;
		} else {
			return -1;
		}
	}
	while (1) {
		int tRdLen = 0;
		off_t CurOff = 0;
		void *p=realloc(tData, tDataLen+1000);

		if (p) {
			tData = p;
		} else {
			Ret = -1;
			break;
		}
		tRdLen = read(Fd, tData+tDataLen, 1000);
		if (tRdLen <= 0) {
			Ret = tRdLen;
			break;
		}
		CurOff	 = lseek(Fd, 0, SEEK_CUR);
		tDataLen+= tRdLen;

		for (tRdLen=1; tDataLen>=sizeof(sSavedFr); memmove(tData, tData+tRdLen, (tDataLen-=tRdLen))) {
			tRdLen = 1;
			sSavedFr *ptFr = (sSavedFr*)tData;

			if (ptFr->StLen <= sizeof(sSavedFr)) {
				Log("FRDB", LOG_LV_DEBUG, "Wrong FR, StLen=%hu", ptFr->StLen);
				continue;
			}
			if ((uint8_t)RvCheckSum(tData, sizeof(sSavedFr)) != 0) {
				Log("FRDB", LOG_LV_DEBUG, "Wrong HdRvCs");
				continue;
			}
			if (ptFr->StLen > tDataLen) {
				// 长度不够，再去读些
				break;
			}
			if ((uint8_t)RvCheckSum(ptFr->Data, ptFr->StLen-sizeof(sSavedFr)) != ptFr->DtRvCs) {
				Log("FRDB", LOG_LV_DEBUG, "Wrong DtRvCs=%hhu", ptFr->DtRvCs);
				continue;
			}
			tRdLen = ptFr->StLen;
			if (ptFr->FrId == 0) {
				// 该包已确认
				Log("FRDB", LOG_LV_DEBUG, "Confirmed Fr");
				continue;
			}
			int i=0;
			for (i=0; i<SavedFrNum; i++) {
				if (pCcFrs[i].pFr->FrId == ptFr->FrId) {
					// 该ID已被使用
					Log("FRDB", LOG_LV_DEBUG, "dupped Fr");
					break;
				}
			}
			if (i >= SavedFrNum) {
				pCcFrs = realloc(pCcFrs, (SavedFrNum+1)*sizeof(sCachedFr));
				pCcFrs[SavedFrNum].SendT	= -1;
				pCcFrs[SavedFrNum].pFr		= malloc(ptFr->StLen);
				memcpy(pCcFrs[SavedFrNum].pFr, ptFr, ptFr->StLen);
				// tDataLen是当前起始位置到读取的数据结尾的长度，CurOff指在下一包要读的置位，CurOff-tDataLen，刚好指在本包数据的开始
				pCcFrs[SavedFrNum].Offset	= CurOff-tDataLen;
				SavedFrNum++;
				if (SavedFrNum >= UINT16_MAX) {
					break;
				}
			}
		}
		if (SavedFrNum >= UINT16_MAX) {
			Ret = -1;
			break;
		}
	}
	if (tData) {
		free(tData);
	}
	// 按ID排序
	int i=0, j=0;

	for (i=0; i+1<SavedFrNum; i++) {
		for (j=i+1; j<SavedFrNum; j++) {
			if (pCcFrs[j].pFr->FrId < pCcFrs[i].pFr->FrId) {
				sCachedFr tFr;

				tFr = pCcFrs[j];
				pCcFrs[j] = pCcFrs[i];
				pCcFrs[i] = tFr;
			}
		}
	}

	close(Fd);

	return Ret;
}

// 将数据插入，后边可以通过Get拿出来，Push的数据优先于init加载的
int FrDbPush(const uint8_t *Data, int DtLen)
{
	if (Data==NULL || DtLen<1 || DtLen>(UINT16_MAX-sizeof(sSavedFr))) {
		return -1;
	}
	sSavedFr *ptFr = malloc(DtLen+sizeof(sSavedFr));

	if (ptFr == NULL) {
		return -1;
	}
	memcpy(ptFr->Data, Data, DtLen);
	ptFr->StLen		= DtLen+sizeof(sSavedFr);
	ptFr->DtRvCs	= (uint8_t)RvCheckSum(ptFr->Data, DtLen);
	ptFr->HdRvCs	= 0;

	int i = 0;
	ptFr->FrId = 1;
	if (pthread_mutex_lock(&Mut) == 0) {
		if (SavedFrNum < UINT16_MAX) {
			// 先设置FrId，并插入列表中
			for (i=0; i<SavedFrNum; i++) {
				if (ptFr->FrId != pCcFrs[i].pFr->FrId) {
					break;
				} else {
					ptFr->FrId++;
				}
			}
			pCcFrs = realloc(pCcFrs, (SavedFrNum+1)*sizeof(sCachedFr));
			memmove(pCcFrs+i+1, pCcFrs+i, (SavedFrNum-i)*sizeof(sCachedFr));
			pCcFrs[i].pFr	= ptFr;
			pCcFrs[i].SendT = -2;
			SavedFrNum++;
			pCcFrs[i].pFr->HdRvCs	= RvCheckSum(pCcFrs[i].pFr, sizeof(sSavedFr));

			int Fd = -1;

			// 在末尾将数据写入, 打不开或者写入不了都不再额外处理
			Fd = open(FRAME_DB_FILE, O_WRONLY|O_APPEND|O_CREAT, 0666);
			if (Fd > 0) {
				if (write(Fd, pCcFrs[i].pFr, pCcFrs[i].pFr->StLen) == pCcFrs[i].pFr->StLen) {
					pCcFrs[i].Offset = lseek(Fd, 0, SEEK_END) - pCcFrs[i].pFr->StLen;
				} else {
					pCcFrs[i].Offset = -1;
				}
				close(Fd);
			}
		}

		pthread_mutex_unlock(&Mut);
	} else {
		free(ptFr);
		return -1;
	}
	//
	FrDbPrint();
	return 0;
}

// 数据包的获取规则是：先找从未上传过的，然后再找超时的，To为超时时间，此时间之前，pSeq为希望获得的包的Seq，如果不能满足会自动修改为无重复的，获得成功后ppData指向该数据，记得释放
// 返回-1没有缓冲的包或出错，返回0，没有满足条件的包，返回正数，为返回的数据长度
int FrDbGet(double To, uint32_t *pSeq, uint8_t **ppData)
{
	if (pSeq==NULL || ppData==NULL) {
		return -1;
	}
	int i=0, Ret=-1;

	if (pthread_mutex_lock(&Mut) == 0) {
		int j=0;

		for (j=1; j<SavedFrNum; j++) {
			if (pCcFrs[j].SendT < pCcFrs[i].SendT) {
				i = j;
			}
		}
		if (i < SavedFrNum) {
			// 至少有缓冲的包
			if (pCcFrs[i].SendT+To < BooTime()) {
				// 修改Seq
				while (1) {
					for (j=0; j<SavedFrNum; j++) {
						if (i!=j && pCcFrs[j].SendT>=0 && pCcFrs[j].pFr->Seq==*pSeq) {
							break;
						}
					}
					if (j < SavedFrNum) {
						(*pSeq)++;
					} else {
						break;
					}
				}
				pCcFrs[i].pFr->Seq	= *pSeq;
				pCcFrs[i].SendT		= BooTime();
				Ret		= (pCcFrs[i].pFr->StLen - sizeof(sSavedFr));
				*ppData	= malloc(Ret);
				memcpy(*ppData, pCcFrs[i].pFr->Data, Ret);
			} else {
				// 没有满足条件的
				Ret = 0;
			}
		}

		pthread_mutex_unlock(&Mut);
	}
	//
	return Ret;
}

int FrDbDel(uint32_t Seq)
{
	int i=0, Ret=-1;

	if (pthread_mutex_lock(&Mut) == 0) {
		for (i=0; i<SavedFrNum; i++) {
			if (pCcFrs[i].SendT>=0 && pCcFrs[i].pFr->Seq==Seq) {
				break;
			}
		}
		if (i < SavedFrNum) {
			// 改文件 从列表删除，判断是否需要清理
			if (pCcFrs[i].Offset!=(off_t)-1 && pCcFrs[i].Offset>=0 &&
			    pCcFrs[i].pFr->DtRvCs==(uint8_t)RvCheckSum(pCcFrs[i].pFr->Data, pCcFrs[i].pFr->StLen-sizeof(sSavedFr))) {
				// 偏移有效，DtRvCs正确，修改文件
				pCcFrs[i].pFr->FrId	= 0;
				pCcFrs[i].pFr->HdRvCs	= 0;
				pCcFrs[i].pFr->HdRvCs	= RvCheckSum(pCcFrs[i].pFr, sizeof(sSavedFr));
				int Fd = open(FRAME_DB_FILE, O_WRONLY);

				if (Fd >= 0) {
					if (lseek(Fd, pCcFrs[i].Offset, SEEK_SET)==pCcFrs[i].Offset &&
					    write(Fd, pCcFrs[i].pFr, sizeof(sSavedFr))==sizeof(sSavedFr)) {
						Ret = 0;
					}
					uint32_t NeedSize = 0;
					off_t FlSize = 0;
					int j = 0;

					for (j=0; j<SavedFrNum; j++) {
						if (pCcFrs[j].pFr->FrId != 0) {
							NeedSize += pCcFrs[j].pFr->StLen;
						}
					}
					FlSize = lseek(Fd, 0, SEEK_END);
					Log("FRDB", LOG_LV_DEBUG, "FlSize=%d, NeedSize=%d, TRIM_THR=%d", (int)FlSize, (int)NeedSize, TRIM_THR);
					if (FlSize>0 && (NeedSize+TRIM_THR)<FlSize) {
						// 缩小
						FlSize = 0;
						for (j=0; j<SavedFrNum; j++) {
							if (pCcFrs[j].pFr->FrId == 0) {
								continue;
							}
							if (lseek(Fd, FlSize, SEEK_SET) != FlSize) {
								break;
							}
							if (write(Fd, pCcFrs[j].pFr, pCcFrs[j].pFr->StLen) != pCcFrs[j].pFr->StLen) {
								break;
							}
							pCcFrs[j].Offset = FlSize;
							FlSize += pCcFrs[j].pFr->StLen;
						}
						if (j >= SavedFrNum) {
							j = ftruncate(Fd, FlSize);
							Log("FRDB", LOG_LV_DEBUG, "ftruncate ret=%d", j);
						} else {
							Log("FRDB", LOG_LV_DEBUG, "Failed to trim, errno=%d", errno);
						}
					}
					close(Fd);
				}
			}
			// 列表中去掉此条记录
			memmove(pCcFrs+i, pCcFrs+i+1, (SavedFrNum-i-1)*sizeof(sCachedFr));
			SavedFrNum--;
			pCcFrs = realloc(pCcFrs, SavedFrNum*sizeof(sCachedFr));
		}
		pthread_mutex_unlock(&Mut);
	}
	FrDbPrint();
	//
	return Ret;
}

void FrDbPrint(void)
{
	int i=0;

	if (pthread_mutex_lock(&Mut) == 0) {
		for (i=0; i<SavedFrNum; i++) {
			Log("FRDB", LOG_LV_DEBUG, "[%d] ID=%hu, off=0x%X, Seq=%u, t=%.13G",i+1, pCcFrs[i].pFr->FrId, (int)pCcFrs[i].Offset, pCcFrs[i].pFr->Seq, pCcFrs[i].SendT);
		}
		pthread_mutex_unlock(&Mut);
	} else {
		Log("FRDB", LOG_LV_DEBUG, "Failed to lock Mut, errno=%d", errno);
	}
}
