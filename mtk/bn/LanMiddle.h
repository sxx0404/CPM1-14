#ifndef _LAN_MIDDLE_H
#define _LAN_MIDDLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void InitRecordFile(void);
uint16_t Flash_EmulateE2prom_WriteByte(const void* pBuffer, uint32_t WriteAddrOffset, uint32_t NumByteToWrite);
void Flash_EmulateE2prom_ReadByte(void* pBuffer, uint32_t ReadAddrOffset, uint32_t NumByteToRead);

#ifdef __cplusplus
}
#endif
#endif


