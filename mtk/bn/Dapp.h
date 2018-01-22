#ifndef _DAPP_H
#define _DAPP_H
#include <stdint.h>
#include <stdlib.h>

//#define LED_RECD_PROC		7		// 录像文件发送过程指示灯

//#define  ushort     unsigned  short
//#define  uint         unsigned  int
//#define  ulong      unsigned  long
#ifndef __cplusplus
typedef int					bool;
#endif // __cplusplus
typedef int					INT;
typedef unsigned char		uchar;
typedef unsigned char 		UCHAR;
typedef	unsigned char		BYTE;
typedef unsigned char 		BOOL;
typedef unsigned short		ushort;
typedef unsigned short		USHORT;
typedef unsigned int		uint;
typedef unsigned int		UINT;
typedef unsigned long long	ullong;
typedef unsigned long 		ULONG;
typedef unsigned long 		ulong;

typedef signed long      s32;
typedef signed short     s16;
typedef signed char      s8;

typedef volatile signed long      vs32;
typedef volatile signed short     vs16;
typedef volatile signed char      vs8;

typedef unsigned long       u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

typedef unsigned long  const    uc32;  /* Read Only */
typedef unsigned short const    uc16;  /* Read Only */
typedef unsigned char  const    uc8;   /* Read Only */

typedef volatile unsigned long      vu32;
typedef volatile unsigned short     vu16;
typedef volatile unsigned char      vu8;

typedef volatile unsigned long  const    vuc32;  /* Read Only */
typedef volatile unsigned short const    vuc16;  /* Read Only */
typedef volatile unsigned char  const    vuc8;   /* Read Only */
#endif
