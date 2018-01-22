#ifndef GPIO_H_INCLUDED
#define GPIO_H_INCLUDED

#include <stdint.h>

#define GPIO_3G_PWR		45
#define GPIO_LOCK_MODE	67
#define GPIO_LOCK_BTN	69		// "��ť�ӿ�"-���Ű�ť
#define GPIO_LOCK_DTC1	30
#define GPIO_LOCK_DTC2	31
#define GPIO_LOCK_CTL	34
#define GPIO_LOCK_RCTL	35	// "¥��Խ��ӿ�"-Զ�̿���
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
#define GPIO_BI_DTC		28		// "����ӿ�"-BreakIn����
#define GPIO_RS_DTC1	33		// "Ԥ���ӿ�"-״̬���1
#define GPIO_RS_DTC2	29		// "Ԥ���ӿ�"-״̬���2
#define GPIO_RS_CTRL	32		// "Ԥ���ӿ�"-�̵�������
#define GPIO_PD_DTC		68		// "��Դ����/���/�ϵ�澯"-�ϵ�澯�ӿ�
#define GPIO_RST_BTN	70		// RESET按钮

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Type:0下降沿，1上升沿，Arg为注册时的参数
typedef void (*GpioIrqCb)(int Type, int Pin, void *Arg);

int GpioInit(void);
// 将对应IO设置为输入模式并读取其值
int GpioGet(int IoNum);
// 将对应IO设置为输出模式并设置其值
int GpioSet(int IoNum, int Val);
// 读取对应IO的值，不改变其输入输出模式
int GpioStat(int IoNum);
// 读取对应IO的输入输出模式，0输入，1输出
int GpioDir(int IoNum);
// 失败返回全FF
uint32_t GpioModeGet(void);
// 注意，请谨慎使用
int GpioModeSet(uint32_t Mode);
void GpioPrintReg(void);

int GpioRegIrq(int Pin, double dt, GpioIrqCb Cb, void *Arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GPIO_H_INCLUDED
