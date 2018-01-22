// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef AFX_STDAFX_H__9986E32E_B3BA_4D09_9CA9_4BA51435C0D3__INCLUDED_
#define AFX_STDAFX_H__9986E32E_B3BA_4D09_9CA9_4BA51435C0D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Dapp.h"

//typedef enum {FALSE = 0, TRUE = !FALSE} bool;   //????

/* the definition whether the wmc device is new pcb or old pcb
 * 新旧PCB决定硬件版本号
 */
//#define _PCB_3_2	// PCB3.2 (22.1)
#define _PCB_3_1	// PCB3.1 (16.2)

/* linux主控器*/
#define  USE_LINUX

/* 采用新的内核版本,rf射频驱动部分采用魔数*/
#define _use_new_kernel

//#define _PCB_2_0	// PCB2.0 (16.1)
//#define _PCB_1_2	// PCB1.2

//#if 1
//#define _NEW_PCB	// PCB2.0
//#else
//#define _OLD_PCB	// PCB1.2
//#endif

/*
 *  基础软件版本号, 决定软件版本号起始数
 *  // 奇数:V2.1, 偶数:V2.2
 *  // V2.5, Big Endien //2.6, 13-04-03	// 2.7, 13-04-17 // 2.9, 13-05-10 // 2.13 13-08-01
 */
#define SOFT_MAJOR_VERSION  (10)   // 主版本号
#define SOFT_MINOR_VERSION  (0)   // 次版本号,

/*
 *  支持协议版本号, 决定软件版本是奇数或偶数, 决定是否具有"门常开报警"
 */

#define _USE_WIFI_TASK

#define _WIFI_HF_LPB    // 汉枫WIFI模块
//#define _WIFI_MX_EMW    // 庆科WIFI模块

//#if defined (_PROTOCOL_V21) // 出租房项目标配"门常开报警"
#define _OPEN_ALARM	// 门常开报警, 1-打开, 0-关闭
//#endif

//#define _SEND_SMS   // 发送短信功能

//#define _FLOW_STATISTICS    // 流量统计

/****define for update ****************************/
//#define _DEBUF_USE_TEST_PARA
//#define _DEBUF_USE_TEST_BOOT_PARA
//#define TEST2
#define _USE_UPDATE_MODE        // use bootloader

/* 使用看门狗设备 */
#define USE_WATCHDOG
/* 打印TCP Recv信息 */
//#define _debug_tcp_comm_recv_prt_i

#define _USE_DOG_SOFTWATCHDOG	// 软件看门狗

/* 软件看门狗调试打印的宏定义*/
//#define _debug_softwatchdog_prt_i

/* 摄像机继电器联动打印的宏定义*/
//#define _debug_relay_link_camera_prt_i

/* 多接口主控器用于清除设备列表 */
//#define _debug_clear_setupdevice

/* 常在线连接 */
//#define _debug_alway_connect_server   1

/* rf2.4G大数据底层发送测试*/
//#define _debug_rfdata_test1	1

/**************************************************/

//#define DCODE_LEN   4   // d_code长度

//#define ERROR     0
//#define SUCCESS   1
#define STATUS_ERROR    0
#define STATUS_SUCCESS  1

#define	MAX_SERIAL_DATALEN 						6000//3000

//wangcq add door open mode  at 2010.8.13
#define OPEN_TYPE_CARD 					0
#define OPEN_TYPE_CARD_PASSWORD           11
#define OPEN_TYPE_PASSWORD           10
#define OPEN_TYPE_KQ                         12
#define OPEN_TYPE_DOUBLE_CARD				13

#define	SERIAL_TIMEOUT								40 * 1000	//串口超时时间 10ms

#define DEFAULT_SER_WAIT_TIME  (40*1000)

/**********for RF define********************/
#define MAX_MODU            1
#define MAX_RFADD           6

#define COM_BASE_ADD     0x20

#define DEFAULT_CMD_WAIT_FAST_TIME     10
#define DEFAULT_CMD_WAIT_NORMAL_TIME     30
#define DEFAULT_CMD_WAIT_REQUEST_TIME  50
#define W_MODULE_OFF_TME               100  //10 seconds
#define W_CHANGE_CHANNEL_INTERVAL  20
#define MAX_CHANEL_ID  12
#define RECEIVE_MAX_PCOUNT  2
#define DOOR_OFF_LINE_TIME  300
#define DOOR_FAST_OFF_TIME 10

#define DOOR_ONLINE_DEEP_OFF_TIME  1200
#define DOOR_ONLINE_ON_TIME    20

#define CHECK_RETRY_FAST_INTERVAL  2
#define CHECK_RETRY_NORMAL_INTERVAL  10
#define STAT_INTERVAL   10
#define CHECK_STAT_INTERVAL   300
#define SEND_ONLINE_ALL_INTERVAL 600
#define SYN_CHK_INTERVAL 3000

#define LED_FLASH_INTERVAL         1
#define MS_SET_SYSTERM_TIME_INTERVAL 6000
#define CHECK_KEY_DESROY_INTERVAL   50
#define MODULE_OFF_TIME   15
#define MAX_PROGRAM_INFO_SEND_TIME  270

#define FAST_MODE 1
#define NORMAL_MODE 0

#define ON   1
#define OFF 0

#define DHCP_MODE 0

//param init, tqk set
#define TEST1
#define ENABLE_FLUSH
#define LADD 0xfe
#define DPVAL 0x3f
#define ENABLE_PACKID
#define USE_LEN 21
#define USE_WHOLE_LEN 23
#define PGM_USE_LEN (USE_LEN -1)

#define DESDECRY 2
#define DESENCRY 1
//#define SUCCESS  1
#define TEADECRY  0
#define TEAENCRY  1

#define DEFAULT_AT_WAITTIME		(5000*1000)
#define GPRS_READ_WAITTIME		(30*1000)
#define GPRS_FAST_WAITTIME		(200*1000)
#define GPRS_NORMAL_WAITTIME	(1000*1000)
#define GPRS_CONNECT_WAITTIME	(20000*1000)	// 如连接失败需超时20s
#define GPRS_CLOSE_WAITTIME	    (27000*1000)	// 关闭连接延时27s

#define GPRS_IP_STATE_DEFAULT		0   // 缺省状态
#define GPRS_IP_STATE_PDP_DEACT		1
#define GPRS_IP_STATE_IP_INITIAL	2
#define GPRS_IP_STATE_IP_START	    3
#define	GPRS_IP_STATE_IP_CONFIG	    4   // 介于IP START和IP GPRSACT状态之间
#define	GPRS_IP_STATE_IP_GPRSACT	5
#define	GPRS_IP_STATE_IP_STATUS	    6
#define	GPRS_IP_STATE_CONNECTING	7   // TCP/UDP正在连接
#define GPRS_IP_STATE_CONNECT_OK	8   // 已连接(单连接状态)
#define GPRS_IP_STATE_IP_PROCESSING	8   // 已连接(多连接状态)
#define GPRS_IP_STATE_IP_CLOSED		9   // 已关闭

//#define	GPRS_IP_STATE_IP_GPRSACT	5
enum {
    GPRS_MODULE_STATE_DAFAULT       = 0,    // 缺省状态
    GPRS_MODULE_STATE_MOD_INVALID,          // 模块失效(AT操作失败)
    GPRS_MODULE_STATE_SIM_ERROR	,           // SIM卡ERROR(掉卡)
    GPRS_MODULE_STATE_SIM_NOTINS,           // SIM卡没插
    GPRS_MODULE_STATE_COPS_ERROR,           // 无法获取运营商
    GPRS_MODULE_STATE_PDP_ERROR,            // 建立PDP上下文连接ERROR
    GPRS_MODULE_STATE_SIM_READY,            // SIM卡正常(AT+CIMI;AT+CCID)
    GPRS_MODULE_STATE_COPS_OK,              // 成功获取运营商
    GPRS_MODULE_STATE_PDP_OK,               // 建立PDP上下文连接成功
    GPRS_MODULE_STATE_AT_OK,                // 模块AT指令集操作OK
    GPRS_MODULE_STATE_CREG_ERROR,           // 注册入网出错
    GPRS_MODULE_STATE_CREG_OK,              // 注册入网
    GPRS_MODULE_STATE_CSQ_ERROR,            // 未检测到基站信号
    GPRS_MODULE_STATE_CSQ_OK,               // 基站信号OK
    GRPS_MODULE_STATE_NORMAL,               // 模块各个状态都OK, 14
    GRPS_MODULE_STATE_AT_ERROR,             // 模块AT指令集操作不成功
};


#define   STATE_ACK                            0x1       //bit0
#define   STATE_RECEIVE_VALID         0x2       //bit1
#define   STATE_NOT_ALLOW_SEND    0x4       //bit2
#define   STATE_USE_SM1                   0x8       //bit3

#define    PACK_PER_LEN                     24
#define    MAX_RESEND_TIMES           2

#define    WIN_STAE_KZ                     0x80      //bit7
#define    WIN_STATE_START             0x40     //bit 6
#define    WIN_STATE_END                0x20      //bit5

#define WIRELESS_PACKID_MAX   100

#define STATUS_RSP     1
#define STATUS_REQ     0
#define OPENID_TYPE_CARD       1
#define OPENID_TYPE_FINGER   0

/*以下是设备类型*/
#define DEVICE_TYPE_WMC					0	//主控器自身（独立设备）
#define DEVICE_TYPE_WIRELESS_LOCK		1	//门锁（独立设备）
#define DEVICE_TYPE_DOOR_MAGNETIC		2	//门磁（附属设备）
#define DEVICE_TYPE_WINDOW_MAGNETIC		3	//窗磁（附属设备）
#define DEVICE_TYPE_SMOKE				4	//烟感（附属设备）
#define DEVICE_TYPE_INFRARED			5	//红外（附属设备）
#define DEVICE_TYPE_YK_SET_DEFENCE		6	//设防遥控器（窗磁）
#define DEVICE_TYPE_YK_CLEAR_DEFENCE	7	//撤防遥控器（窗磁）
#define DEVICE_TYPE_MANUAL_ALERT		8	//手动按键遥控报警设备
#define DEVICE_TYPE_PSAM_CARD			10	//PSAM卡（门锁附属）
#define DEVICE_TYPE_CAPTURE				11	//拍照器
#define DEVICE_TYPE_CAMERA				12	//摄像头（包括云台）
#define DEVICE_TYPE_KZ_MODULE           16
#define DEVICE_TYPE_WIRE_LOCK           17
#define DEVICE_TYPE_reserve				255	//（保留）

#define DEVICE_TYPE_LOCK				1	//门锁（独立设备）

#define  MAX_BUSINESS_SINGLE_PACK_LEN  1100//600
#define BUSINESS_HEAD_LEN  20
#define MAX_MANAGEBZ_BUFFER_LEN        3000 //8000

#define WLMS_CMD_REQUEST_ADD          0x2c
#define  WLMS_CMD_SET_ADD                 0x2d
#define WLMS_UP_CMD_REQUEST_ADD          0xac
#define  WLMS_UP_CMD_SET_ADD                 0xad

#define M_KEY_LEN	16

#define	MAX_SUBDEVICE     15

#define	FRAGMENT_LEN	500 // 900		// size of each upgrade data fragment
#define	FILENAME_LEN	100		// length of upgrade file name

#define GPRS_POWER_RESET_CHECK_AT	0
#define GPRS_POWER_ON_FIRST				1		// 开机
#define GPRS_POWER_RESET_FORCED		2		// 运行过程中开机

#define BUFFERED_CTRL_UPGRADE_FILE			"/home/ctrl_upgrade.bin"
#define BUFFERED_LOCK_UPGRADE_FILE			"/home/lock_upgrade.bin"
#define MAX_CTRL_UPGRADE_FILE_SIZE			10000000
#define MAX_LOCK_UPGRADE_FILE_SIZE			1000000
#if defined(_PCB_2_0) || defined(_PCB_3_1)
///////////////////////////////////////////////////////////////////////////////////////////////////
/* linux主控器，主控器升级*/
/*
*	0x0~0xA000:40K (用于boot程序，其实已经没有boot空间)
*	0x100000~0X500000:4M (用于b应用程序的升级存放)
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
/* 脱机记录456K(1024B=1K)存储空间*/
#define	FLASH_APP_BOOT_ADD          0x0A000	// flash start address to boot application program

#ifdef USE_LINUX
//#define FLASH_UPGRADE_ADD           0x100000	// flash start address to store upgrade data
//#define FLASH_BOOT_PARA_ADD         0X500000	// flash start address to store boot parameter
#else
//#define FLASH_UPGRADE_ADD           0x3C000	// flash start address to store upgrade data
//#define FLASH_BOOT_PARA_ADD         0x6E000	// flash start address to store boot parameter
#endif

#define FLASH_CAM_PARA_ADD          0x6E080 // flash for store camera para, from 0x6E200 to 0x6E080 @2013-10-15 by x.j
#define FLASH_CAM_PARA_MAX_LEN			128u

#define FLASH_DCODE_PARA_ADD        0x6E100 // 设备编码起始地址
#define FLASH_DCODE_PARA_MAX_LEN		64u     // 设备编码数据长度

#define FLASH_SMS_PARA_ADD          0x6E300 // flash for store camera para
#define FLASH_SMS_PARA_MAX_LEN			128u

#define FLASH_SYSTERM_PARA_ADD      0x0	// flash start address to store system parameter

#define RECORD_START_ADDRESS        0x6EC00	// flash start address to store offline data
#define RECORD_END_ADDRESS          0xE0C00 //456*1024 SPACE

//#define PWLMS_UPDATE_START_ADD      0xE0C00	// flash start address to store lock programme
//#define PWLMS_UPDATE_END_ADD        0xF9C00

#define RF_SECURITY_ENCODE_TYPE_ADD	0x6E3FF // AES para store add
///////////////////////////////////////////////////////////////////////////////////////////////////
/* 脱机记录200K(1024B=1K)存储空间*/
#elif defined(_PCB_1_2)
#define	FLASH_APP_BOOT_ADD	0x08000	// flash start address to boot application program

//#define FLASH_UPGRADE_ADD	0x21000	// flash start address to store upgrade data
//#define FLASH_BOOT_PARA_ADD           0x3a000	// flash start address to store boot parameter

#define FLASH_CAM_PARA_ADD	0x3a080		// flash for store camera para, from 0x3A200 to 0x3A080 @2013-10-15 by x.j
#define FLASH_CAM_PARA_MAX_LEN		128u

#define FLASH_WIFI_PARA_ADD	0x3A100		// flash for store wifi para
#define FLASH_WIFI_PARA_MAX_LEN		512u

#define FLASH_SMS_PARA_ADD	0x3A300		// flash for store sms para
#define FLASH_SMS_PARA_MAX_LEN		128u

#define FLASH_SYSTERM_PARA_ADD     0x3a400	// flash start address to store system parameter

#define RECORD_START_ADDRESS      0x3AC00	// flash start address to store offline data
#define RECORD_END_ADDRESS          0x65400        //263*1024 SPACE	has reduce to 170K 20130105 xuji

//#define PWLMS_UPDATE_START_ADD  0x65400	// flash start address to store lock programme 	has increase to 100K 20130105 xuji
//#define PWLMS_UPDATE_END_ADD      0x7E400

#define RF_SECURITY_ENCODE_TYPE_ADD			0x3a3FF		// AES para store add
#endif

#define  GPRS_SEND_OVER_TIME                300
#define  GPRS_BUSINESS_WAIT_TIME            36000	//30 //3000//700 // 无数据传输断开连接时间(单位100ms)，负数表示不主动断开
#define  GPRS_BUSINESS_DELIVER_WAIT_TIME    300	//1800

#define RTC_MIN_TIME         0x4E267801             //2011-07-19

#define RTC_WMC_REQUEST_INTERVAL       43200 //unit: Second,  12Hours
#define RTC_LOCK_REQUEST_INTERVAL       43200 //unit: Second,  12Hours
#define RTC_WMC_RESEND_INTERVAL          800   //unit 0.1s
#define RTC_LOCK_RESEND_INTERVAL         600   //unit 0.1s

#define TASK_DOG_DEFENCE    0   // TaskDefence
#define TASK_DOG_RFNET      1   // TaskRfnet
#define TASK_DOG_MANAGE     2   // taskManage
#define TASK_DOG_GPRS       3   // TaskGprs

#define LOCK_LINK_MODE_OFF     0
#define LOCK_LINK_MODE_ON       1
#define LOCK_LINK_MODE_FLASH  2

//#define _DEBUG_USE_WIRELESS
#define _DEBUG_GPRS
#define _DEBUG_RFNET
#define _DEBUG_PROTOCOL

/* 容器list调试打印宏定义*/
//#define _DEBUG_LIST

/* 服务器通信中传输层的调试打印宏定义*/
#define _debug_manage_transmit_layer_prt_i

/* 服务器通信中安全层的调试打印宏定义*/
//#define _debug_manage_security_layer_prt_i
/* 服务器通信中业务层的调试打印宏定义*/
//#define _debug_manage_busi_layer_prt_i

/* 主控器升级程序路径*/
#define UPDATE_FILE_PATH		"/home/upgrade.tar.bz2"
#define TMP_UPDATE_FILE_PATH	UPDATE_FILE_PATH".tmp"
// 文件头添加信息
#define SOFT_VER_OFFSET				(6)
#define HARD_VER_OFFSET				(13)
#define CHKSUM_OFFSET				(0X20)
#define FILESIZE_OFFSET					(0X24)
#define VALIDFLAG_OFFSET				(0X28)					// 55 55 AA AA
#define ENCODETYPE_OFFSET		(0X2C)					// 0:表示不加密	1:表示异或算法加密
#define DEVTYPE_OFFSET				(0X2D)					// 0:表示主控器	1:表示门锁
#define CONTENT_OFFSET				(0X40)					// 实际代码内容起始地址

#define FLAG_UPGRADE_INIT 0xff	// the init flag
#define FLAG_UPGRADE_CTRL 0	// the flag to download ctrl file
#define	FLAG_UPGRADE_LOCK 1	// the flag to download lock file

#define MAX_UPFILE_RESEND_TIMES     2	// the max times to download upgrade file // 最多允许下载2次
#define MAX_REDOWNLOAD_BLOCK_TIMES	15	// the max times redownload the block // 最多允许下载15次
#define MAX_RF_DOOR             4

#define REQUEST_MODE   2
#define RESPONSE_MODE  1

/*define end*/

/**********for RF define end****************/

/*
*/

#define VERCHAR "Z3086 T1.001"

/*****************************************************************************
History:

******************************************************************************/

typedef struct
{
    unsigned char read_buf[MAX_SERIAL_DATALEN];
    int len;
}typSerialData;


/**for wireless  type***************************************************/
enum SEND_STATE{idle  = 0, busy};

typedef struct
{
    unsigned char cModuID;
    unsigned char cPipeID;
    unsigned char data[32];
    unsigned char len;
    int BufID;
}typRfPack;

typedef struct
{
    unsigned char bSendValid;
    unsigned long LastProcessTime;
    unsigned char CurrState;
    unsigned short PackID;
}typSendState;

typedef struct
{
    unsigned char buf[32];
    unsigned char len;
    int PipeID;
}typRFData;

typedef struct
{
    int i;
    //   typRFData down[2];
#if 0
    unsigned char Head[32];
    unsigned char HeadLen;
    unsigned char Buf[1024];
    unsigned char BufLen;
    unsigned char Tail[32];
    unsigned char TailLen;
#endif
}typDownData;

typedef struct
{
    uchar temp;
#if 0
    typSendState RemoteCardRight;
    unsigned long LastSendCardID;
    typSendState  LocalDoorRight;

    unsigned char Event;
    unsigned char CurrExcEvent;
    unsigned char CurrExcEventState;
    //add for local right send >1k at 2011-07-10
    int CurrStartBigPackID;
    int  TotalPackQua;
    int  BigPackCount;
    //end
    int bValid;
    unsigned char PackOk[20];
    unsigned char PackStart[20];
    unsigned char PackPutin[20];
    unsigned char PackQua;
    typDownData  DownData;
    unsigned long  LastSendTime;
    int  ExcCount;
    int  FullCount;
    unsigned short  Crc[3];   //0 speccrc, 1.localcrc, 2 timetable crc
    unsigned char  CrcValid;
    unsigned char  CrcNull;
    unsigned char  CrcSame;
    unsigned char  CrcSameValid;
    typRFData para[1];  // 0 ,para and time
    int BigSendCancel;
    int bUpdateStart;
    int bUpdateInfo;
    int bUpdateCurrInfoPackage;
#endif
}typDownloadControl;

#pragma pack(1)
typedef struct
{
    ushort PackID;
    uchar Ack;
    uchar Win;
}typWirelessHead;
#pragma pack()

typedef struct
{
    unsigned char cmd;
    unsigned char pid;
    unsigned char pcount;
    unsigned long  LastOperTime;  //unit: ms
    typRfPack        LastPack;
    unsigned char LastSMode;
    unsigned long LastCheckTXNullTime;
    enum SEND_STATE state;  //busy  ,idle
    int  WaitTime;         //unit: ms

    typWirelessHead packhead;
    int bPackReceived;
    ushort  LastPackid;
    ushort  StartPackID;
    ushort  TotalPackCount;
}typWSendControl;

typedef struct
{
    unsigned char LostValSum;
    unsigned char BadCount;  //lost not less then 60%
    unsigned char CompleteLostCount;
    unsigned long CompleteLostDoorID[MAX_RFADD];
    unsigned char len;
    bool  bJumped;
}typChannleState;

typedef struct
{
    unsigned long LastChangeTime;  //change channel
    unsigned char LastChID;
    bool   bChannleValid;
    bool   bModuleValid;
    bool   bModuleOnline;
    bool   bChangeRequest;
    typChannleState ChannleState[MAX_CHANEL_ID];
}typChangeChannel;

typedef struct
{
    bool  state;
    bool  ShouldOn;
    unsigned long LastFlashTime;
}typLedControl;

typedef struct
{
    uchar temp;
#if 0
    unsigned char OrignalVal[16];
    unsigned char CrtVal[16];
    unsigned char bKeyNew;
    unsigned char bKeyValid;
    unsigned  long LastCreateTime;
    unsigned long  LastValidTime;
#endif
}typKey;

typedef struct
{
    unsigned long  ProductID;
    unsigned long ComID;
    unsigned char PipeID;
    bool bMalloc;
    bool bAddEnable;
    bool bDhcp;
}typOrignID;

typedef struct
{
    char Ver[12];
    char SwType[5];
    char HwType[5];
}typPgmVersion;

typedef struct
{
    unsigned long lMsb;
    unsigned char cLsb[MAX_RFADD];
    unsigned char  qua;
    unsigned char  DoorQua;
    unsigned long LastStatTime;
    typWSendControl ChipSend;
    typWSendControl send[MAX_RFADD];
    typLedControl led;
    typKey  key[MAX_RFADD];
    typDownloadControl download[MAX_RFADD];
    unsigned char mode;
    typOrignID OrignID[MAX_RFADD] ;
    unsigned char bValid[MAX_RFADD];
    unsigned char bBroadcast;
    unsigned char bBroadcastSendAdd;
    unsigned long lLastBroadcastAckTime;
    unsigned char cBaseComID;
    typPgmVersion VerInfo[MAX_RFADD];
    unsigned long CancelPipe0AckTime;
}typPipes;

#define	MASK_STATUS				1	// whether has received status
#define	MASK_BATTERY			(MASK_STATUS << 1)
#define	MASK_TEMPERATURE		(MASK_BATTERY << 1)
#define	MASK_SIGNAL				(MASK_TEMPERATURE << 1)
#define	MASK_VERSION			(MASK_SIGNAL << 1)
#define	MASK_HARDWARE			(MASK_VERSION << 1)

#define	MASK_POPEDOMCAPACITY	(MASK_HARDWARE << 1)
#define	MASK_POPEDOMCOUNT		(MASK_POPEDOMCAPACITY << 1)
#define	MASK_FINGERCAPACITY		(MASK_POPEDOMCOUNT << 1)
#define	MASK_FINGERCOUNT		(MASK_FINGERCAPACITY << 1)

#define	MASK_WMCBATTERY		    (MASK_FINGERCOUNT << 1)     // 主控器备用电池电量
#define	MASK_WMCGSMSIGNAL	    (MASK_WMCBATTERY << 1)      // 主控器GSM信号
#define	MASK_WMCNETWORKMODE	    (MASK_WMCGSMSIGNAL << 1)    // 主控器网络制式

#define	MASK_WMCWORKSTATUS	    (MASK_WMCNETWORKMODE << 1)    // 主控器网络制式
#define	MASK_WMCVERSION 	    (MASK_WMCWORKSTATUS << 1)    // 主控器网络制式
#define	MASK_WMCFIRMWARE	    (MASK_WMCVERSION << 1)    // 主控器网络制式

typedef struct
{
	unsigned long lDoorLinkTime;
    ULONG DoorLastOfflineTime;
	bool               bOnline;
	unsigned char      bFirstOnlie;
	bool       bDeepOff;
	bool       bFastMode;			// 表示该门锁处于一种快速通信模式，优先向其发送数据，否则为低速模式
	unsigned long LostStat;
	bool      din;
	bool      bNotInstalled;
	int bUserKeyError;
	unsigned char ValidLen;
	unsigned char StatVal;
	unsigned char LastStatVal;
	unsigned long AllStatLost;
	unsigned long AllStatRecv;
	unsigned char JumpCount;

	ulong FlagStatus;           // 标识状态数据的有效性
	ulong add;
	ushort status;
	uchar battery;
	ushort temperatrue;
	uchar ver[2];
	uchar DefenceState;
	uchar  TerminalSeqNo[8];
	uchar firmware[2];

	//uchar PopedomCapacityFlag;
	uchar PopedomCapacity[2];
	//uchar PopedomCountFlag;
	uchar PopedomCount[2];
	//uchar FingerCapacityFlag;
	uchar FingerCapacity[2];
	//uchar FingerCountFlag;
	uchar FingerCount[2];
}typDoorOnline;

#pragma pack(1)
typedef struct
{
    unsigned char RetryTimes;
    unsigned char RetryInterval;
    unsigned char NormComInterval;
    unsigned char SpecComInterval;
    unsigned long sTime;
    unsigned long eTime;
    unsigned char LockDelay;
    unsigned short  Area;
    char 	  charDoorPass[9];
    char	  charDoorHiJackPass[9];
    unsigned char cDoorType;
    unsigned char cOpenMode;
    unsigned char iPassAvalidTime;
}typWDoor;
#pragma pack()

// MAX_MODU个模块，每个模块MAX_RFADD个（6个）通道
typedef struct
{
    unsigned char cModuID[MAX_MODU];				// 未知，暂未使用
    unsigned char cModuQua;							// 模块数
    bool  bMOnline[MAX_MODU];						// 各模块连接与否
    unsigned long LastMReceiveTime[MAX_MODU];		// 各模块最后接收数据的时间
    unsigned long lLastSetSystermTime[MAX_MODU][6];	// 暂未使用
    typChangeChannel ChangeChannel[MAX_MODU];
    typPipes pipes[MAX_MODU];
    typDoorOnline DoorOnline[MAX_MODU][6];
    typWDoor init[MAX_MODU][6];
    unsigned char StatCount;
    unsigned long LastStatTime;
    unsigned long LastCheckStatTime;
    unsigned long LastSendOnlieInfoTime;
    unsigned long ModuleLastActiveTime;
    unsigned char AddMode;
    unsigned long lUpdateDoorID;
}typRfControl;

typedef struct
{
    unsigned long lDoorExcID;
    unsigned long lComID;
    bool bDhcp;
    typWDoor      init;
    unsigned char 	iOnline;				// online stats, -1: init value, 0: offline, 1: online
    unsigned long	lLastTime;		//鏈€鍚庡贰妫€鏃堕棿
    unsigned int	iErrorCount;			//Error Count value
    unsigned char	byLastStatus;	//Last status,
}typWDoorControl;

typedef struct
{
    unsigned char 	iChip;			//芯片
    unsigned char 	iChannelID;	//通
    unsigned char	iRegNo;			//拇
    unsigned char	iLen;				//盏效莩
    unsigned char 	buf[32];		//莼
}typnRF2401_Data;

/**for old rf type  end****************************************************/
typedef struct
{
    uchar ManagePassword[M_KEY_LEN];    // 默认约定会话密钥
    uchar DEKKey[M_KEY_LEN];
//    ulong UserPassword;
//    ulong heartInterval;    // 心跳包传输时间间隔
//    uchar AlertAllow;
    char StoreImsi[16];			// 上传通讯处的最新的IMSI，上传完设置，其他地方只读不改
    char macadd[18];			// 设备以MAC作为唯一的标识，格式为AA:BB:CC:DD:EE:FF
//    char DevId[12];

    char NewImsi[16];				// 仅上传时设置，上传完清除，其他地方不读不改

//    uchar bImsiChangedRequestSend;  // 是否发送IMSI号
//    uchar bDoWMCTempOff;
    ulong BusinessSeq;  // 业务层流水号
//    uchar ProtocolVer[2];
//    uchar hsType[2];

//    uchar Version[2];

    uchar Random1[8];   // 服务器下发随机数-add on 13-03-06
    uchar Random2[8];   // 主控器生成随机数-add on 13-03-06
    uchar SessionKey[M_KEY_LEN];    // 临时会话密钥sessionKey
}typSystermInfo;


/***********************************************************************
systerm write to flash:
1.ulong lServerIP
2.ushort ServerPort
3.uchar  ManagePassword[16]
4.ulong UserPassword
5.ulong  heartInterval
6.uchar AlertAllow;
7. char  APN[30]
************************************************************************/
//add for gprs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
* 1.SIMCardState确保处于GRPS_MODULE_STATE_NORMAL，确保模块状态正常
* 2.如果存在业务数据，则置位bDoGprsServerConnect标志位，用于进行PPP拨号
* 3.检测g_ppp_status状态，判断是否已经建立PPP拨号
* 4.如果PPP拨号成功，则进行协议服务器TCP连接，连接成功后置位bGprsConnectOK
*	，并点亮系统联机指示灯
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
//	uchar bDoPppDialup;						// 1:请求建立PPP拨号连接操作
//	uchar bDoGprsServerConnect;		        // 1:有业务报文数据，则请求建立socket连接
//	uchar bGprsConnectOK;					// 状态表示，1:协议服务器建立socket连接	0:未建立socket 注：请只在管理连接的线程中修改，其他地方别修改
//	int gprs_send_fail_cnt;				    // 若发送次数大于一定数值，则需要断开socket连接并且PPP拨号重新拨号
//	uchar bGprsSendFail;					// 1:socket层发送失败(重新进行PPP拨号)
	uchar NetType;
//	uchar bAttached;
//	uchar bDoDisconnect;					// 1:进行断开服务器socket连接操作
//	uchar qos;
//	uchar IPState;							// g_ppp_status用于检测是否建立PPP拨号连接
	uchar bReadIMSI;
//	uchar bDoDisconnecWorking;              //1标识正在做断开操作，等变为0则断开操作完成
//	uchar ConnecttingCount;
//	uchar ModuleReady;						// 1-模块初始化完成, 0-模块未初始化
	uchar ConfigureReady;					//1-配置成功,0-未配置
	uchar iprReady;							// 1-配置过IPR,0-未配置IPR
//	ulong LastResetTime;
//	ulong LastErrorTime;    			    // SIM卡掉卡时间
	//uchar bGprsNetOpen;	// NET是否OPEN
	uchar ModuleState;                      // 模块状态, 使用bit位表示
	uchar LastErrorModuleState;		        // 最后一次检测到错误的模块状态, 使用bit位表示
	uchar SIMCardState;						// 用于表示模块状态
	uchar query_state_err_cnt;		        // QueryModuleState函数中操作错误次数累计，达到上限则reset模块
	uchar CopsState;                        // 运营商状态
	uchar NetState;                         // 联网状态
}typGprsState;

typedef struct {
	int opr_ret;
	int opr_err_cnt;
	ulong opr_start_time;
}typOPR_PARA;

typedef struct {
	typOPR_PARA at_chk_opr;
	typOPR_PARA sim_chk_opr;
	typOPR_PARA cops_opr;
	typOPR_PARA creg_opr;
}typMODULE_STATUS_CTL;

/*end*/

/**for repeater end**************************************************/

/*new protocol*/
typedef struct
{
    ushort Length;
    uchar  Keyword;
    uchar  Status;
    ulong  Sequence;
    uchar  KeyID;
    uchar  Sum;
}typProtocol;

typedef struct
{
	uchar *vp;
	uchar *NextTag;
	ushort vlen;
	uchar tag;
}typTLV;


typedef struct
{
    uchar operate;
    ulong IdentifyID;
    ulong addr;                     //door add or areaID
    ulong StartTime;
    ulong EndTime;
    uchar FingerCode[512];
}typOpenRights;

typedef struct
{
    uchar tail;
    uchar head;
    uchar buf[ WIRELESS_PACKID_MAX];
}typWirelessPackIDQue;

#define TEMP_RECEIVE_MAX        1000
typedef struct
{
    int tail;
    uchar buf[TEMP_RECEIVE_MAX];
}typTempReceive;

typedef struct
{
    ulong len;
    ulong cmd;
    ulong cmdstate;
    ulong sequence;
}typTransmitLayerHead;

typedef struct {
	uchar type;
	ushort length;
	uchar cmd;
	uchar cmd_status;
	ulong seq;
	uchar rhl;
}typBUSINESS_LAYER_HEAD;

typedef struct
{
    ushort cpl;
    uchar chl;
    uchar spi;
    uchar kic;
    uchar kid;
    uchar cntr[5];
    uchar pcntr;
    uchar cc[4];
    uchar padding;
}typSecurityLayer;

typedef struct
{
    char SourceAdd[21];
    uchar type;

}typSubmitRequest;

typedef struct
{
   uchar bWMCTimeReceived;
   uchar bLockReceived;
   uchar bLcokWaitReceive;          // 该标识置位时，会根据根据上次同步的时间去同步锁的时间
   double LastWMCSyncTime;
   ulong LastWMCSyncRequestTime;
   ulong LastLockSyncTime;
   ulong LastLockSyncRequestTime;
}typTimeSynControl;

typedef struct
{
    int bLoginReceived;             // 标识Login指令的应答包已收到，发送前置0，收到后置1
    int bSubmitResponseReceived;    // 标识Submit指令的应答包已收到，收到后置1
    int bLogoutReceived;            // 标识Logout指令的应答包已收到，发送前置0，收到后置1
    int bGNACKReceived;             // 标识Nack指令的应答包已收到，收到后置1
    //int bOnline;
    int bRequestConnect;            // 联网请求标记, 表示是否需要联网 (联网同时会发送login指令)， 外部请求ManageTransmit模块
//    int bHaveLogin;                 // 连接成功和断开连接后清空，认证后置位

    ulong LastCommTime;             // 上次收到数据的时间

    uchar SecurityCounter[5];       // 计数器，用于传输时的CNTR字段
//    uchar pcount;

    int bSendOnlineNotify;          // 上线标记, 表示是否发送过上线数据. 1-上线过, 0-未上线过
//    int bSendOnlineTimeAdjust;
    ulong  LastBeatSendTime;//

    //int bUpgradeNotify;	// whether should upgrade or not
    typTimeSynControl TimeSync;

    uchar dog;                      // 软狗
    int bDoClearDoorAdd;
    uchar LockLinkLedMode;          // 门锁指示灯显示方式
    int bDoClearRecord;
    uchar RecordClearLedMode;
}typTransmitControl;

typedef struct
{
    uchar buf[1024];
    int len;
    char SourceAdd[16];
    uchar type;
    uchar bUseSm1;
}typBusinessData;

typedef struct
{
    ulong MatchCode;
    ulong ServerIP;
    ushort ServerPort;
}typDeviceParameter;

typedef struct
{
    uint32_t LogicCode;
    uchar type;
    char address[20];
   int ComPort;
    typDeviceParameter  DeviceParameter;
    uchar Status;
}typDevice;

typedef struct
{
    typDevice Device[MAX_SUBDEVICE];
    uchar count;
}typDeviceSetup;

typedef struct
{
    uint32_t LogicCode;	// 升级的目标ID
	uchar ver[2];		// 升级后的版本
	ulong size;			// 升级包的总大小
	ulong crc32;		// the crc32 check value of the whole file
//	ulong upfile_crc32;	// the crc32 check value of a single file-block
//	uchar token[8];
	ulong offset;		// 当前下载的偏移
	uchar active;	// the flag whether wmc device has received a block data，为1则准备接收下一包，为0则本包还没接收到
	char filename[FILENAME_LEN];	// 升级包的定位文件名
	uchar bUpgradeNotify;	// whether should upgrade or not
	uchar uptimes;	// times to download the whole upgrade file
	uchar downloadsuccess;	// the flag whether the wmc device upgrade packet has totally download successfully
	uchar blocktimes;	// times to download one upgrade file block
	//uchar flagDownload;	// the flag whether the upgradefile need to download or not. 1-yes, 0-no
}typUpgradeNotify;

typedef struct
{
    ulong offset;   // 回复分包的偏移量
	ulong crc32;			// crc32 check code
	ushort len;				// the length of FILE data fragment
	uchar content[FRAGMENT_LEN];	// FILE data fragment
}typUpgradeData;


typedef struct
{
    uchar No;
    uchar Index;
    uchar Data[16];
    uchar KCV[3];
}typTKey;

typedef struct
{
    int count;
    typTKey  key[16];
}typTransmitKey;

#pragma pack(1)
typedef struct {
	ulong ip;
	ushort port;
	uchar cMac[6];
}typNETINFO;

/*typedef struct {
	uchar cYear;
	uchar cMon;
	ushort SerialNo;
}typDEVID;*/
typedef struct {
	uchar cFactory;
	uchar cDevType;
	uchar cProdSeries[2];
	uchar cSerialNo[8];
}strDEVID;

#define KEYNUM	(16)
#define APNNUM	(30)
typedef struct {
//	ushort quantity;			// 2
//	ushort crc;					// 2
//	uchar cComKey[KEYNUM];	// 16
//	uchar cApn[APNNUM];					// 30
	typNETINFO NetInfo;	// 12
	//typDEVID DevId;			// 4
//	strDEVID DevId;	// 12
}typDEV_PARA_CTL;			// 66

/* 设备编码 */
typedef struct
{
    ushort quantity;			// 2
    ushort crc;					// 2
    char IDcode[25];    // 识别码 24
    char AddrCode[33];  // 地址编码 32
}typDEV_PARA_DEVICECODE;    // total: 60


typedef struct
{
	ushort Identify;	// b2
	ulong  LastLink;	// b4, 当前脱机记录的节点的位置
	ulong  NextLink;	// b4, 下一条脱机记录的节点位置
	ulong  RecordID;	// b4
	ushort len;				// b2
	ushort crc;     	// b2 crc16 value
}typRecordList;			// b18

typedef struct
{
	ushort RecordNum;		// 脱机记录数目
	ulong HeadAddress;  // 头节点地址
	ulong TailAddress;  // 尾节点地址(当前历史脱机记录节点所在的地址)
	ulong NextAddress;  // 下一节点地址()
	ulong RecordID;
	ushort crc;                  //head crc16 with identify, headaddress, tailaddress;
}typRecordInfo;		// b20-b2(crc) = b18

typedef struct
{
    typRecordInfo Info;
    ulong rest;
}typRecordControl;

#pragma pack()

#define MAX_LOG_BUF_LEN         500
#define LOG_BUF_UNVALID         0
#define LOG_BUF_VALID           1
#define LOG_BUF_SEND_FAIL       2
#define LOG_BUF_SEND_OK         3

#define LOG_STATE_IDLE			0		// 未连接，仅TaskTCPComm置
#define LOG_STATE_DOING_LONGIN	1		// 已连接尚未登录，仅TaskTCPComm置
#define LOG_STATE_HAVE_LONGIN	2		// 连接且已登录，仅业务处置
#define LOG_STATE_DOING_LOGOUT	3		//
#define LOG_STATE_HAVE_LOGOUT	4		//
typedef struct
{
    uchar buf[MAX_LOG_BUF_LEN];
    uchar len;
    uchar BufStatus;		// LOG_BUF_UNVALID初始化后、LOG_BUF_VALID连接认证、LOG_BUF_SEND_OK认证数据包已发送完成、LOG_BUF_SEND_FAIL发送失败
    uchar LogSate;			// 标记连接状态
}typLogControl;

/*new protocol end*/


//for lan controler, add by wangcq

/*for device manage*/

#define	INIFILE					"/home/acs09.ini"
#define CAN_LINK_MAX_WAITTIME    50   //unit: ms
#define DEVICE_OPERATE_ADD_ALERT    0
#define DEVICE_OPERATE_DEL                 1
#define DEVICE_OPERATE_ALERT            2
#define DEVICE_OPERATE_DEL_ALL        3

#define LEFT_SHIFT		0
#define RIGHT_SHIFT		1


struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
};

struct typDeviceBase
{
     uchar type;
     ulong dcode;
     ulong phyadd;
     char   strAddress[20];
     uchar status;
};


struct typKzBase
{
        uchar  comport;
        struct typDeviceBase      main;
	 struct typDeviceBase      sub[6];           //for  wireless devices under kz module
	 int subcount;
};

struct typWireDoorBase
{
       uchar comport;
	struct typDeviceBase     mdoor;
};


struct typDevList
{
      struct typKzBase KzList[100];
      int kzcount;

      struct typWireDoorBase   WireDoorList[100];
      int WireDoorCount;

      struct typDeviceBase   WirelessDoorList[6];
      int WirelessDoorCount;

};

typedef struct
{
         uchar LeaderType;
	  ulong SrcDcode;
	  ulong DestDcode;
	  ushort DataSN;
}typComNet;

struct sfile{
    char fname[27];
    uchar *fptr;
	int flen;
	typComNet route;
    struct sfile *next;
    struct sfile *prior;
} ;


typedef struct
{
     struct  sfile  *head;
     struct  sfile  *tail;
      uchar state;
}typSendFile;


typedef struct
{
    ulong add;
    uchar online;
    ushort status;
    uchar  WorkStatus;
    uchar battery;
    ushort temperatrue;
    uchar signal;
    uchar ver[2];
    uchar DefenceState;
    uchar  TerminalSeqNo[8];

     ulong FlagStatus;
    uchar firmware[2];
	//uchar PopedomCapacityFlag;
	uchar PopedomCapacity[2];
	//uchar PopedomCountFlag;
	uchar PopedomCount[2];
	//uchar FingerCapacityFlag;
	uchar FingerCapacity[2];
	//uchar FingerCountFlag;
	uchar FingerCount[2];
    typSendFile  SendFile;
}typDoorInfo;

typedef struct
{
    ulong addr;
    uchar type;
    ulong areaID;
    uchar openDelay;
    uchar OpenPass[4];
    uchar Oper;
    uchar  PSAMPass[16];
}typDoorSettings;


struct  typKzInfo
{
    struct typDeviceBase   kzbase;
    uchar  online;
    uchar  WorkStatus;
    uchar  SwVersion[2];
    uchar  HwVersion[2];
    uchar  TerminalSeqNo[8];
    typSendFile  SendFile;
    ulong  LastRecvTime;
    ulong	FlagStatus;
};

struct typKzDoorInfo
{
     struct typDeviceBase   KzdoorBase;
     typDoorInfo       KzdoorInfo;
     uchar bValid;
};

struct typKzControl
{
      struct typKzInfo kz;
      struct typKzDoorInfo  kzdoor[6];
      int DoorCount;
};


struct typWireDoorInfo
{
    struct typDeviceBase WireDoorBase;
    typDoorInfo DoorInfo;
#if 0
    uchar  online;
    uchar  WorkStatus;
    ushort  SwVersion;
    ushort  HwVersion;
    uchar  TerminalSeqNo[8];

    uchar  state;
    typSendFile  SendFile;
#endif
    ulong  LastRecvTime;
};

#define TEMP_RECV_MAX   1024

typedef struct
{
      ushort len;                      /*底层业务包总长字段*/
      ulong  SendAddress;
      ulong  ReceiveAddress;
      ushort PackID;
      uchar  Ack;
      uchar  Win;
      ushort TotalLen;
      ushort  TotalCrc;
      uchar  KzLen;
      ushort RecvID;
      ushort Crc;
}typCanLinkHead;

typedef struct
{
      int tail;
      ushort TotalLen;
      ushort TotalCrc;
      short LastPackid;
      uchar buf[TEMP_RECEIVE_MAX];
}typDataLinkRecvControl;


typedef struct
{
    typWirelessHead packhead;

    int  WaitTime;         //unit: ms
    int bPackReceived;
    ushort  LastPackid;
    ushort  StartPackID;
    ushort  TotalPackCount;
    ushort    NetLayerPackID;
}typDataLinkSendControl;

typedef struct
{
     uchar type;
     ulong dcode;
     ulong phyadd;
     char   strAddress[20];
     int operate;             // 0, 增加或修改; 1:删除; 2: 修改; 3: 删除所有;
}typDeviceOperate;   //设备添加、修改、删除


#define DATA_CACHE_MAX_LEN    1024
typedef struct
{
     typComNet head;
     uchar data[DATA_CACHE_MAX_LEN];
     int len;
}typDataCache;

struct typComDeviceControl
{
        struct typKzControl   kzm;
	 struct typWireDoorInfo    wiredoor;
	 typDataLinkSendControl  DataSendControl;
        typDataLinkRecvControl   DataRecvControl;
	 uchar validtype;
};

typedef struct
{
    long  date;
    int     time;
    int     week;
}typDate;

typedef struct
{
        char strServerIP[20];
	 int   ServerPort;
//	 int   LocalPort;
//	 int   bServerOnline;
}typServerInfo;

/* 视频录像和实时流IP地址和端口 */
typedef struct
{
  char cRecd_ip[16];  // 123.157.158.094
  int iRecd_port;
  char cReal_ip[16];
  int iReal_port;
	unsigned short normal_recd_time;	// 正常录像时间
	unsigned short alarm_recd_time;		// 报警录像时间
}typRecdServerInfo;


#define MAX_TCP_SEND_LEN		(1024)
struct typTcpData
{
	unsigned int len;
	char DataBuf[MAX_TCP_SEND_LEN];
};

#ifndef TYPCANRECV_CONTROL
#define TYPCANRECV_CONTROL
#pragma pack(1)
#define CAN_TRANS_RECV_MAX	(1100)
typedef struct
{
	ushort TotalLen;
	ushort Crc;
	ushort LastPackId;
	uchar RevBuf[CAN_TRANS_RECV_MAX];
	int Tail;
	ulong Identi;
	uchar Status;
}typCanRecvControl;
#pragma pack()
#endif

typedef struct
{
      int docansendonline1;
}typEvent;

typedef struct
{
    ulong type;
    uchar text[10];
}typEventMsg;
//for os change

typedef struct
{
   int k;
}OS_CPU_SR;

#define ser_printf  printf
#define rf_printf  printf
#define echo_printf   printf
#define debug_printf  printf

/* Z1008M LED管脚定义:online:GPM0, run:GPM1, w1:GPM2, w2:GPM3, w3:GPK6, w4:GPK7, wifi:GPQ2 */
#define LED_ONLINE      0       // LINK:系统联机指示灯
#define LED_RUN         1       // 主控器运行指示灯
#define LED_DOOR_LINK   2       // 锁联机指示灯
#define LED_CAM1_ONLINE 3       // 摄像机1联机指示灯
#define LED_CAM2_ONLINE 4       // 摄像机2联机指示灯
#define LED_3G_NET      5       // D3:3G网络制式指示灯
//#define LED_DOOR_OPEN   5       // 门开关指示灯
//#define LED_RSV1        5       // 保留指示灯
//#define LED_GPRS        5       // GPRS模块指示灯
#define LED_SEND_FILE   6       // 文件传输指示灯（录像文件单包发送指示灯）
#define LED_LOCK_MODE	7
#define LED_NUM         8

#define LED_OPEN_OPR    0       // 打开指示灯
#define LED_CLOSE_OPR   1       // 关闭指示灯

////////////////////////////////////////////////////////////////////////////////////////

#if defined(_use_new_kernel)
#define nRF2401_CE_CTL			12
#define nRF2401_CS_CTL			13
#define _ioctl_3g_power_en				14		// 3G模块电源控制管脚
#define _ioctl_3G_RESET_CTL			15		// GPP0

#define GPIO_LED_IOC_MAGIC          0x92

#define GPIO_LED_OPEN_OPR          _IOW(GPIO_LED_IOC_MAGIC,1, int)
#define GPIO_LED_CLOSE_OPR        _IOW(GPIO_LED_IOC_MAGIC, 2, int)
#define GPIO_INPUT_READ          _IOR(GPIO_LED_IOC_MAGIC, 3, int)
#define nRF2401_READ_IRQ					_IOR(GPIO_LED_IOC_MAGIC, 4, int)
#else
#define _ioctl_3g_power_en				10		// 3G模块电源控制管脚
#define _ioctl_3G_RESET_CTL				11		// GPP0
#endif

//////////////////////////////////////////////////////



#define MSGKEY 1024

#define MSG_TYPE_ONLINE       1    //参数0 offline, 1 online



#define _project_cbm_32

#define _20MS_DIV_1MS 		(20)
#define _100MS_DIV_20MS 	(5)
#define _1S_DIV_100MS		(10)

// debug nrf24l01+
//#define _debug_rf_chip

//end

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9986E32E_B3BA_4D09_9CA9_4BA51435C0D3__INCLUDED_)
