#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "up.h"
#include "../stdafx.h"
#include "../NormalTool.h"

// 对升级文件进行检查，通过后将文件大小修改
int UpgradeFileCheck(int Type, int Size, uint32_t Crc)
{
	if (Size <= 0) {
		return -1;
	}
	int Ret = -1;
	int Fd = -1;
	char Path[256] = {0};

	// 准备文件路径
	switch (Type) {
	case FLAG_UPGRADE_CTRL:
		strncpy(Path, BUFFERED_CTRL_UPGRADE_FILE, 255);
		break;
	case FLAG_UPGRADE_LOCK:
		strncpy(Path, BUFFERED_LOCK_UPGRADE_FILE, 255);
		break;
	default:
		return -1;
	}

	// 打开文件
	Fd = open(Path, O_RDWR);
	if (Fd > 0) {
		int FlLen = lseek(Fd, 0, SEEK_END);
		if (FlLen >= Size) {
			// 映射到内存
			void *Data = mmap(NULL, Size, PROT_READ, MAP_PRIVATE, Fd, 0);

			if (Data != MAP_FAILED) {
				// 计算CRC并比较
				if (CRC32(Data, Size) == Crc) {
					// TODO 升级包格式检查否？
					Ret = 0;
				}
				munmap(Data, Size);
			}
		}

		// 校验成功，将文件大小修改为需要的大小
		if (Ret == 0) {
			ftruncate(Fd, Size);
		}
		close(Fd);
	}

	return Ret;
}

int UpgradeFileRead(int Type, int Offset, void* Data, int Len)
{
	if (Offset<0 || Data==NULL || Len<=0) {
		return -1;
	}
	int fd = -1;
	int Ret = -1;
	char Path[256] = {0};

	// 准备文件路径
	switch (Type) {
	case FLAG_UPGRADE_CTRL:
		strncpy(Path, BUFFERED_CTRL_UPGRADE_FILE, 255);
		break;
	case FLAG_UPGRADE_LOCK:
		strncpy(Path, BUFFERED_LOCK_UPGRADE_FILE, 255);
		break;
	default:
		return -1;
	}

	// 打开文件
	fd = open(Path, O_RDONLY);
	if (fd >= 0) {
		// 找到位置并写入
		if (lseek(fd, Offset, SEEK_SET) == Offset) {
			int tRet = read(fd, Data, Len);

			if (tRet == Len) {
				Ret = 0;
			} else if(tRet>0 && (Offset+Len > lseek(fd, 0, SEEK_END))) {
				// 此时说明读取超出了文件长度
				Ret = 1;
			}
		}
		close(fd);
	}

	return Ret;
}

int UpgradeFileWrite(int Type, int Offset, const void* Data, int Len)
{
	if (Offset<0 || Data==NULL || Len<=0) {
		return -1;
	}
	int fd = -1;
	int Ret = -1;
	char Path[256] = {0};

	// 准备文件路径
	switch (Type) {
	case FLAG_UPGRADE_CTRL:
		strncpy(Path, BUFFERED_CTRL_UPGRADE_FILE, 255);
		break;
	case FLAG_UPGRADE_LOCK:
		strncpy(Path, BUFFERED_LOCK_UPGRADE_FILE, 255);
		break;
	default:
		return -1;
	}

	// 打开文件
	fd = open(Path, O_RDWR|O_CREAT, 0666);
	if (fd >= 0) {
		// 找到位置并写入
		if (lseek(fd, Offset, SEEK_SET)==Offset && write(fd, Data, Len)==Len) {
			Ret = 0;
		}
		close(fd);
	}

	return Ret;
}
