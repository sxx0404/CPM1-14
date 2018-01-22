//#
#ifndef _SECURITY_H
#define _SECURITY_H

#ifdef __cplusplus
extern "C" {
#endif

//外部只读，不要修改（初始化除外）
extern char cveci[10];

int SecurityLayerDecode(uint8_t *SecurityData, uint8_t *out, int *len);
int SecurityLayerEncode(uint8_t *PaddingData, int len, int EncryptMode, uint8_t *out, int *outlen);

#ifdef __cplusplus
}
#endif
#endif
