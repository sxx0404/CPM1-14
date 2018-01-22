#include <unistd.h>
//#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../../include/common.h"
#include "../../include/log.h"
#include "check_dir.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct my_struct
{
    char f_name[FN_SIZE];
    time_t ctime;
}f_struct, *p_f_struct;

static p_f_struct neo_p_head = NULL;
static p_f_struct neo_p_end = NULL;

// ��ʼ���ļ��������
static int neo_init_check_size()
{
    if (neo_p_head != NULL) {
		memset(neo_p_head, 0, sizeof(f_struct)*CHECK_DIR_SIZE);
    	neo_p_end = neo_p_head;
		return 0;
    }
    neo_p_head = (p_f_struct)malloc(sizeof(f_struct)*CHECK_DIR_SIZE);
    if (neo_p_head == NULL) {
		Log("Sd", LOG_LV_ERROR, "neo_p_head init fail.");
		return -2;
    }
    memset(neo_p_head, 0, sizeof(f_struct)*CHECK_DIR_SIZE);
    neo_p_end = neo_p_head;
    return 0;
}

#if 0
// �ͷų�ʼ�����ڴ�
static int neo_close_check_size()
{
    //p_f_struct p = neo_p_head;
    if (neo_p_head != NULL)
    {
        free(neo_p_head);
        neo_p_head = NULL;
        neo_p_end = NULL;
        return 0;
    }
    return -1;
}

// ��ȡ��һ�ļ�����ַ
static p_f_struct neo_get_p_next(p_f_struct p)
{
    p++;
    if(p->f_name[0] == 0)
        return NULL;
    return p;
}
#endif

// ��ȡ��������׵�ַ
static p_f_struct neo_get_p_head()
{
    if (neo_p_head->f_name[0] == 0)
        return NULL;
    return neo_p_head;
}

// ��ȡ�������β��ַ
static p_f_struct neo_get_p_end()
{
    if (neo_p_head->f_name[0] == 0)
        return NULL;
    neo_p_end = neo_p_head;
    while ((neo_p_end + 1)->f_name[0] != 0)
        neo_p_end++;
    return neo_p_end;
}

// ��ʾĿ¼����������
static int neo_print_f_name()
{
	int i = 0;
	if (neo_p_head == NULL) {
		Log("Sd", LOG_LV_DEBUG, "directory name printf is error.");
		return 0;
	}
	p_f_struct p = neo_p_head;
	while (p->f_name[0] != 0) {
		struct tm Localt = {0};
		char Rtm[100] = {0};
		localtime_r(&p->ctime, &Localt);
		snprintf(Rtm, 100, "%04d-%02d-%02d %02d:%02d:%02d",\
			Localt.tm_year+1900, Localt.tm_mon+1, Localt.tm_mday,\
			Localt.tm_hour, Localt.tm_min, Localt.tm_sec);
		Log("Sd", LOG_LV_DEBUG, "directory name[%s], ctime:%ld[%s].", p->f_name, p->ctime, Rtm);
		p++;
		i++;
	}
	Log("Sd", LOG_LV_DEBUG, "directory num=%d.",i);
	return i;
}

// ��Ŀ¼�������ļ����������
/* ����ֵ:
*		-3:��Ŀ¼���࣬��Ҫ��������
*/
static int neo_check_dir(const char *dir)
{
	int ret = -1;
    DIR * dp;
    struct dirent *dent;
    struct stat st;
    char fn[1024] = {0};
    //char fn1[1024];
	// Ŀ¼��Ϣ��ʼ��
	if (neo_init_check_size()) {
		// ����-1
		ret = -1;
		return ret;
	}
    p_f_struct p = neo_p_head;

    dp = opendir(dir);
    if (!dp) {
		// ����-2
		Log("Sd", LOG_LV_DEBUG, "can't open path[%s].", dir);
		ret = -2;
		return ret;
    }
	while ((dent = readdir(dp)) != NULL) {
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
            continue;
        }
        sprintf(fn, "%s%s", dir, dent->d_name);
        if (stat(fn, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
					// ����ĳ��Ŀ¼����/mnt/rcd/20160323
					snprintf(p->f_name, FN_SIZE, "%s", fn);
                p->ctime = st.st_ctime;
                //neo_p_end = p;
                p++;
                if (p - neo_p_head >= (CHECK_DIR_SIZE-1)) {
						// ����-3
						ret = -3;
						closedir(dp);
						Log("Sd", LOG_LV_DEBUG, "check dir is fail. dir num is than (CHECK_DIR_SIZE-2).");
						return ret;
                }
            }
        } else {
        		// ����-4
        		Log("Sd", LOG_LV_DEBUG, "can't stat path[%s].", fn);
				ret = -4;
				closedir(dp);
				return ret;
        }
	}
	closedir(dp);
	ret = 0;
	Log("Sd", LOG_LV_DEBUG, "check dir is ok.");
	return 0;
}

//����key�������Ϊ������
static p_f_struct partion(p_f_struct pstHead, p_f_struct pstEnd)
{
    f_struct temp_struct;
    memcpy(&temp_struct, pstHead, sizeof(f_struct));
    while(pstHead != pstEnd)
    {
        while((pstHead < pstEnd) && (pstEnd->ctime >= temp_struct.ctime))
            pstEnd --;
        if(pstHead < pstEnd){
    //        printf("%s,%ld\n",pstEnd->f_name,pstEnd->ctime);
            memcpy(pstHead, pstEnd, sizeof(f_struct));
            pstHead ++;
        }
        while((pstHead < pstEnd) && (pstHead->ctime <= temp_struct.ctime))
            pstHead ++;
        if(pstHead < pstEnd){
  //          printf("%s,%ld\n",pstHead->f_name,pstHead->ctime);
            memcpy(pstEnd, pstHead, sizeof(f_struct));
            pstEnd --;
        }
    }
//            printf("%s,%ld\n",temp_struct.f_name,temp_struct.ctime);
    memcpy(pstHead, &temp_struct, sizeof(f_struct));
    return pstHead;
}

// ��ɨ�赽���ļ������һ���޸�ʱ���������
static void quick_sort(p_f_struct pstHead, p_f_struct pstEnd)
{
	if (pstHead == NULL || pstEnd == NULL) return;
	if (pstHead < pstEnd) {
		p_f_struct temp_Head = pstHead;
		p_f_struct temp_End = pstEnd;
		p_f_struct pstTemp = partion(temp_Head, temp_End);
		quick_sort(pstHead, pstTemp - 1);
		quick_sort(pstTemp + 1, pstEnd);
	}
}

// ɨ��&& ɾ��Ŀ¼
#define DBG_DEL_PATH_T
int DirScanDelPathCb(const char *path, // ����·��
									int num,
									int mode
									)
{
	int i = 0, ret = -1;
	if (path == NULL || num <= 0) return 0;

	Log("Sd", LOG_LV_DEBUG, "Dir scan&del path[%s], num=%d, mode=%d.", path, num, mode);
#ifdef DBG_DEL_PATH_T
	double st = BooTime();
#endif
	for (i = 0; i < num; i++) {
		int ChkRet = 0;
		if (mode == PATH_SCAN_DEL) {
			// ����Ŀ¼ɨ�裬��Ŀ¼��Ŀ����365���Ƚ��еݹ�ѭ��ɾ��ֱ��Ŀ¼��ĿС��365
			do {
				ChkRet = neo_check_dir(path);
				switch (ChkRet) {
					default:
					case -1:
					case -2:
					case -4: {
						// ʧ��-1
						Log("Sd", LOG_LV_DEBUG, "PATH_SCAN_DEL process:fail1.");
						ret = -1;
						goto END;
					}
					break;

					case -3: {
						// �ݹ����ɾ��Ŀ¼
						if (DirScanDelPathCb(path, 1, PATH_DEL_FORCE)) {
							Log("Sd", LOG_LV_DEBUG, "PATH_SCAN_DEL process:fail2.");
							// ʧ��-2
							ret = -2;
							goto END;
						}
					}
					break;

					case 0:
					break;
				}
				if (ChkRet == 0) {
					// ɨ��ɹ�(����Ŀ¼��ĿС�ڵ���365)
					Log("Sd", LOG_LV_DEBUG, "PATH_SCAN_DEL process:success.");
					ret = 0;
					goto END;
				}
			} while (1);
		}else if (mode == PATH_DEL_FORCE) {
			// ǿ��ɨ��һ��Ŀ¼(���Գ��������Ŀ¼�������)
			ChkRet = neo_check_dir(path);
			switch (ChkRet) {
				default:
				case -1:
				case -2:
				case -4: {
					// ʧ��-3
					Log("Sd", LOG_LV_DEBUG, "PATH_DEL_FORCE process:fail.");
					ret = -3;
					goto END;
				}
				break;

				case -3:
				case 0: {
					Log("Sd", LOG_LV_DEBUG, "PATH_DEL_FORCE process:success.");
				}
				break;
			}
		} else if (mode == PATH_ONLY_SCAN) {
			// ��ɨ��Ŀ¼(ֻ�Գ������Ŀ¼���������ά��)
			ChkRet = neo_check_dir(path);
			switch (ChkRet) {
				default:
				case -1:
				case -2:
				case -4: {
					// ʧ��-4
					Log("Sd", LOG_LV_DEBUG, "PATH_ONLY_SCAN process:fail1.");
					ret = -4;
					goto END;
				}
				break;

				case -3: {
					// �ݹ����ɾ��Ŀ¼
					if (DirScanDelPathCb(path, 1, PATH_SCAN_DEL)) {
						Log("Sd", LOG_LV_DEBUG, "PATH_ONLY_SCAN process:fail2.");
						// ʧ��-5
						ret = -5;
						goto END;
					} else {
						// �ݹ�ɾ��Ŀ¼�ɹ�
						Log("Sd", LOG_LV_DEBUG, "PATH_ONLY_SCAN process:success.");
						ret = 0;
						goto END;
					}
				}
				break;

				case 0:
					// �ɹ�
					Log("Sd", LOG_LV_DEBUG, "PATH_ONLY_SCAN process:success.");
					ret = 0;
					goto END;
				break;
			}
		}

		// ɾ��Ŀ¼����
		int DirNum = 0;
		quick_sort(neo_get_p_head(), neo_get_p_end());
		DirNum = neo_print_f_name();
		if (DirNum == 0) {
			ret = 0;
			goto END;
		}
		
		// ����Ŀ¼��ɾ���������������Ŀ¼
		// TODO:��ʧ�����ݴ���
		char cmd[100] = {0};
		int len = 0;
		len = snprintf(cmd, 100, "rm -r %s", neo_p_head->f_name);
		cmd[len] = '\0';
		if (system(cmd)) {
			// ʧ��-6
			ret = -6;
		} else {
			ret = 0;
		}
	}
	
END:
#ifdef DBG_DEL_PATH_T
	Log("Sd", LOG_LV_DEBUG, "Dir scan&del path ret:%d, cost:%.3G seconds.", ret, BooTime()-st);
#endif
	return ret;
}

