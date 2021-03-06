#ifndef _CHECK_DIR_
#define _CHECK_DIR_
#include <time.h>

#ifdef	__cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 最多放365天的录像
#define CHECK_DIR_SIZE		(365+2)
//#define CHECK_DIR_SIZE		(10+2)

#define FN_SIZE						(30)		// 例如:/mnt/rcd/20160324

#define PATH_ONLY_SCAN			0		// 扫描目录，仅对目录最大数目做维护
#define PATH_SCAN_DEL			1		// 删除目录的同时，对目录最大数目做维护
#define PATH_DEL_FORCE			2		// 扫描一次后强制删除

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int DirScanDelPathCb(const char *path, // 绝对路径
									int num,
									int mode
									);

#ifdef	__cplusplus
}
#endif
#endif

