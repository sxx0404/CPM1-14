#ifndef _CHECK_DIR_
#define _CHECK_DIR_
#include <time.h>

#ifdef	__cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ����365���¼��
#define CHECK_DIR_SIZE		(365+2)
//#define CHECK_DIR_SIZE		(10+2)

#define FN_SIZE						(30)		// ����:/mnt/rcd/20160324

#define PATH_ONLY_SCAN			0		// ɨ��Ŀ¼������Ŀ¼�����Ŀ��ά��
#define PATH_SCAN_DEL			1		// ɾ��Ŀ¼��ͬʱ����Ŀ¼�����Ŀ��ά��
#define PATH_DEL_FORCE			2		// ɨ��һ�κ�ǿ��ɾ��

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int DirScanDelPathCb(const char *path, // ����·��
									int num,
									int mode
									);

#ifdef	__cplusplus
}
#endif
#endif
