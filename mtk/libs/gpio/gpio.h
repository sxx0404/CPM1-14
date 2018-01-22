#ifndef GPIO_H_INCLUDED
#define GPIO_H_INCLUDED

#include <stdint.h>

#define GPIO_3G_PWR		45
#define GPIO_LOCK_MODE	67
#define GPIO_LOCK_BTN	69		// "°´Å¥½Ó¿Ú"-³öÃÅ°´Å¥
#define GPIO_LOCK_DTC1	30
#define GPIO_LOCK_DTC2	31
#define GPIO_LOCK_CTL	34
#define GPIO_LOCK_RCTL	35	// "Â¥Óî¶Ô½²½Ó¿Ú"-Ô¶³Ì¿ªÃÅ
#define GPIO_LED1		62
#define GPIO_LED2		61
#define GPIO_LED3		60
#define GPIO_LED4		64
#define GPIO_LED5		65
#define GPIO_LED6		27
#define GPIO_LED7		26
#define GPIO_LED8		25
#define GPIO_BUZZ		24
#define GPIO_MCU_RST	12
#define GPIO_BI_DTC		28		// "·À²ğ½Ó¿Ú"-BreakIn¹¦ÄÜ
#define GPIO_RS_DTC1	33		// "Ô¤Áô½Ó¿Ú"-×´Ì¬¼ì²â1
#define GPIO_RS_DTC2	29		// "Ô¤Áô½Ó¿Ú"-×´Ì¬¼ì²â2
#define GPIO_RS_CTRL	32		// "Ô¤Áô½Ó¿Ú"-¼ÌµçÆ÷¿ØÖÆ
#define GPIO_PD_DTC		68		// "µçÔ´ÊäÈë/Êä³ö/¶Ïµç¸æ¾¯"-¶Ïµç¸æ¾¯½Ó¿Ú
#define GPIO_RST_BTN	70		// RESETæŒ‰é’®

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Type:0ä¸‹é™æ²¿ï¼Œ1ä¸Šå‡æ²¿ï¼ŒArgä¸ºæ³¨å†Œæ—¶çš„å‚æ•°
typedef void (*GpioIrqCb)(int Type, int Pin, void *Arg);

int GpioInit(void);
// å°†å¯¹åº”IOè®¾ç½®ä¸ºè¾“å…¥æ¨¡å¼å¹¶è¯»å–å…¶å€¼
int GpioGet(int IoNum);
// å°†å¯¹åº”IOè®¾ç½®ä¸ºè¾“å‡ºæ¨¡å¼å¹¶è®¾ç½®å…¶å€¼
int GpioSet(int IoNum, int Val);
// è¯»å–å¯¹åº”IOçš„å€¼ï¼Œä¸æ”¹å˜å…¶è¾“å…¥è¾“å‡ºæ¨¡å¼
int GpioStat(int IoNum);
// è¯»å–å¯¹åº”IOçš„è¾“å…¥è¾“å‡ºæ¨¡å¼ï¼Œ0è¾“å…¥ï¼Œ1è¾“å‡º
int GpioDir(int IoNum);
// å¤±è´¥è¿”å›å…¨FF
uint32_t GpioModeGet(void);
// æ³¨æ„ï¼Œè¯·è°¨æ…ä½¿ç”¨
int GpioModeSet(uint32_t Mode);
void GpioPrintReg(void);

int GpioRegIrq(int Pin, double dt, GpioIrqCb Cb, void *Arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GPIO_H_INCLUDED
