/***********************************************************************************************************
*	CopyRight(C)		Hangzhou Engin Electronic Co.,Ltd		All Right Reserved
*
*	Filename:
*
*	Description:
*
*	Author:Sunzheng			Created at:2012.05.02		Ver:V1.0.1
************************************************************************************************************/

#ifndef __USER_DEF_H__
#define __USER_DEF_H__

#include <stdint.h>
#include "Dapp.h"
//#include "stdafx.h"
/* Private typedef --------------------------------------------------------------------------------------------*/


/* Private macro ---------------------------------------------------------------------------------------------*/
#define SetBit(VAR,Place)				( VAR |= (1<<Place) )
#define ClrBit(VAR,Place)				( VAR &= ((1<<Place)^255) )

#define BIT0							0X0001
#define BIT1							0X0002
#define BIT2							0X0004
#define BIT3							0X0008
#define BIT4							0X0010
#define BIT5							0X0020
#define BIT6							0X0040
#define BIT7							0X0080
#define BIT8							0X0100
#define BIT9							0X0200
#define BIT10							0X0400
#define BIT11							0X0800
#define BIT12							0X1000
#define BIT13							0X2000
#define BIT14							0X4000
#define BIT15							0X8000

/* CN3052A battery charge detect pin */
#define BAT_CHARGE_DETC_GPIO_PIN				GPIO_Pin_0
#define BAT_CHARGE_DETC_GPIO_PORT			GPIOC
#define BAT_CHARGE_DETC_GPIO_RCC				RCC_APB2Periph_GPIOC
#define BAT_CHARGE_DETC_EXTI_IRQ				EXTI0_IRQn
#define BAT_CHARGE_DETC_EXTI_LINE				EXTI_Line8
#define BAT_CHARGE_DETC_EXTI_PORT_SOURCE		GPIO_PortSourceGPIOC
#define BAT_CHARGE_DETC_EXTI_PIN_SOURCE		GPIO_PinSource0

/* LEDs GPIO definition --------*/
#if defined(_PCB_2_0) || defined(_PCB_3_1)
#define LED12_GPIO_PIN		GPIO_Pin_3
#define LED12_GPIO_PORT	GPIOB
#define LED12_GPIO_RCC_CLK	RCC_APB2Periph_GPIOB

#define LED10_GPIO_PIN		GPIO_Pin_2
#define LED10_GPIO_PORT	GPIOD
#define LED10_GPIO_RCC_CLK	RCC_APB2Periph_GPIOD
#else
#define LED12_GPIO_PIN		GPIO_Pin_2
#define LED12_GPIO_PORT	GPIOD
#define LED12_GPIO_RCC_CLK	RCC_APB2Periph_GPIOD

#define LED10_GPIO_PIN		GPIO_Pin_3
#define LED10_GPIO_PORT	GPIOB
#define LED10_GPIO_RCC_CLK	RCC_APB2Periph_GPIOB
#endif

#define LED11_GPIO_PIN		GPIO_Pin_4
#define LED11_GPIO_PORT	GPIOB
#define LED11_GPIO_RCC_CLK	RCC_APB2Periph_GPIOB

#define LED13_GPIO_PIN		GPIO_Pin_5
#define LED13_GPIO_PORT	GPIOB
#define LED13_GPIO_RCC_CLK	RCC_APB2Periph_GPIOB

#define  SERVER_LINK_GPIO_PIN		GPIO_Pin_8
#define  SERVER_LINK_GPIO_PORT	GPIOB
#define  SERVER_LINK_GPIO_RCC_CLK	RCC_APB2Periph_GPIOB

#define DOOR_OPEN_GPIO_PIN		GPIO_Pin_9
#define DOOR_OPEN_GPIO_PORT	GPIOB
#define DOOR_OPEN_GPIO_RCC_CLK	RCC_APB2Periph_GPIOB



#define GPRS_CTRL_POW_PIN  GPIO_Pin_13
#define GPRS_CTRL_POW_PORT	GPIOC
#define GPRS_CTRL_POW_RCC_CLK	RCC_APB2Periph_GPIOC

#define GPRS_CTRL_RESET_PIN  GPIO_Pin_12
#define GPRS_CTRL_RESET_PORT	GPIOC
#define GPRS_CTRL_RESET_RCC_CLK	RCC_APB2Periph_GPIOC

#define GPRS_STAS_GPIO_PIN		GPIO_Pin_9
#define GPRS_STAS_GPIO_PORT	GPIOC
#define GPRS_STAS_GPIO_RCC	RCC_APB2Periph_GPIOC

/* Beep GPIO */
#define BEEP_GPIO_PIN				GPIO_Pin_6
#define BEEP_GPIO_PORT	GPIOC
#define BEEP_GPIO_RCC		RCC_APB2Periph_GPIOC

#define USE_BAT_GPIO_PIN				GPIO_Pin_4
#define USE_BAT_GPIO_PORT	GPIOC
#define USE_BAT_GPIO_RCC		RCC_APB2Periph_GPIOC

/* Alm in and Relay out */
#define AlmIn_GPIO_PIN			GPIO_Pin_2
#define AlmIn_GPIO_PORT		GPIOC
#define AlmIn_GPIO_RCC			RCC_APB2Periph_GPIOC 
#define Relayout_GPIO_PIN		GPIO_Pin_11
#define Relayout_GPIO_PORT	GPIOA
#define Relayout_GPIO_RCC	RCC_APB2Periph_GPIOA

/* Private variables -------------------------------------------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------------------------------------------*/
extern void RCC_Configuration_Bat_Charge_Detc(void);
extern void NVIC_Configuration_Bat_Charge_Detc(void);
//extern void Bat_Charge_Detc_EXTI_Config( uint32_t cLineSel, EXTITrigger_TypeDef cMode, FunctionalState cState );


#endif

