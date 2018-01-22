#ifndef UP_H_INCLUDED
#define UP_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 对升级文件进行检查，通过后将文件大小修改
int UpgradeFileCheck(int Type, int Size, uint32_t Crc);
int UpgradeFileRead(int Type, int Offset, void* Data, int Len);
int UpgradeFileWrite(int Type, int Offset, const void* Data, int Len);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // UP_H_INCLUDED
