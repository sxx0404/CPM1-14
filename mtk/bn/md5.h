#ifndef ___MD5_H___
#define ___MD5_H___

#ifdef __cplusplus
extern "C" {
#endif
// Md5Buf 保证16个字节
void Md5Data(const void *inData, unsigned int iLength, uint8_t *Md5Buf);
#ifdef __cplusplus
}
#endif

#endif /* ___MD5_H___ included */
