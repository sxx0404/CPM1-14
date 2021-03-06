#ifndef GPIO_H_INCLUDED
#define GPIO_H_INCLUDED

#include <stdint.h>

#define GPIO_3G_PWR		45
#define GPIO_LOCK_MODE	67
#define GPIO_LOCK_BTN	69		// "按钮接口"-出门按钮
#define GPIO_LOCK_DTC1	30
#define GPIO_LOCK_DTC2	31
#define GPIO_LOCK_CTL	34
#define GPIO_LOCK_RCTL	35	// "楼宇对讲接口"-远程开门
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
#define GPIO_BI_DTC		28		// "防拆接口"-BreakIn功能
#define GPIO_RS_DTC1	33		// "预留接口"-状态检测1
#define GPIO_RS_DTC2	29		// "预留接口"-状态检测2
#define GPIO_RS_CTRL	32		// "预留接口"-继电器控制
#define GPIO_PD_DTC		68		// "电源输入/输出/断电告警"-断电告警接口
#define GPIO_RST_BTN	70		// RESET鎸夐挳

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Type:0涓嬮檷娌匡紝1涓婂崌娌匡紝Arg涓烘敞鍐屾椂鐨勫弬鏁�
typedef void (*GpioIrqCb)(int Type, int Pin, void *Arg);

int GpioInit(void);
// 灏嗗搴擨O璁剧疆涓鸿緭鍏ユā寮忓苟璇诲彇鍏跺��
int GpioGet(int IoNum);
// 灏嗗搴擨O璁剧疆涓鸿緭鍑烘ā寮忓苟璁剧疆鍏跺��
int GpioSet(int IoNum, int Val);
// 璇诲彇瀵瑰簲IO鐨勫�硷紝涓嶆敼鍙樺叾杈撳叆杈撳嚭妯″紡
int GpioStat(int IoNum);
// 璇诲彇瀵瑰簲IO鐨勮緭鍏ヨ緭鍑烘ā寮忥紝0杈撳叆锛�1杈撳嚭
int GpioDir(int IoNum);
// 澶辫触杩斿洖鍏‵F
uint32_t GpioModeGet(void);
// 娉ㄦ剰锛岃璋ㄦ厧浣跨敤
int GpioModeSet(uint32_t Mode);
void GpioPrintReg(void);

int GpioRegIrq(int Pin, double dt, GpioIrqCb Cb, void *Arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GPIO_H_INCLUDED
