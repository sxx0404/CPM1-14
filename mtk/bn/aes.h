#ifndef _AES_H
#define _AES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct
{
	uint8_t *pData;
	uint32_t len;
}strAES_PROC_BODY;

typedef struct
{
    uint32_t erk[64];     /* encryption round keys */
    uint32_t drk[64];     /* decryption round keys */
    int nr;             /* number of rounds */
}
aes_context;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define o_debug1_aes			// test code
//#define o_aes_crc_prt1

/* rf security lay encode type (BIT5 BIT4)*/
#define ENCODETYPE_BIT_MASK 0X30
#define NO_ENCODE	0X00
#define TEA_ENCODE	0X10
#define DES_ENCODE	0X20
#define AES_ENCODE	0X30

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int  aes_set_key(aes_context *ctx, uint8_t *key, int nbits);
void aes_encrypt(aes_context *ctx, uint8_t input[16], uint8_t output[16]);
void aes_decrypt(aes_context *ctx, uint8_t input[16], uint8_t output[16]);
extern int PackAesEmulateDecode(strAES_PROC_BODY *decodebody);
extern int PackAesEmulateEncode(strAES_PROC_BODY *encodebody);

extern int AES_CBC_Encode(uint8_t *pinbuf, int len, uint8_t *key);
extern int AES_CBC_Decode(uint8_t *pinbuf, int len, uint8_t *key);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif /* aes.h */
