#ifndef __jcu_net_types__
#define __jcu_net_types__

#ifdef __cplusplus
extern "C" {
#endif
 
#define  WITH_SERVER_MODULE		0x00000001

#include "j_sdk.h"
#include "Jtype.h"
#include "j_pea_osc.h"

/*
 * handle define
 */
typedef void jcu_user_handle_t;
typedef void jcu_stream_handle_t;
typedef void jcu_upg_handle_t;
typedef void jcu_rec_handle_t;
typedef void jcu_talk_cli_t;
typedef void jcu_mb_query_t;
/*
 * callback parm define 
 */
typedef struct _jcu_cb_parm_s {
    int  id;                                //id;
    int  args;                              //args;
    int  size;                              //data size;
    char *data;                             //data;
}jcu_cb_parm_t;


typedef enum _JCU_ERR_CODE_ID_E {
	  SUCCESS = 0
	, JCU_ERR_NET_CORE_INIT = -1
	, JCU_ERR_STREAM_MODULE_INIT = -2
	, JCU_ERR_TALK_MODULE_INIT = -3
	, JCU_ERR_SERVER_MODULE_INIT = -4
	, JCU_ERR_INVALID_ARGV = -5
	, JCU_ERR_SEND = -6
}JCU_ERR_CODE_ID_E;
/*
 * notify callback define
 */
//注意：不建议使用。通知回调使用 j_sdk.h中的PARAMxxx, 
//非参数操作parm.id无效(无效值暂定为-1）， 比如Login、JCU_NOTIFY_STREAM_OPEN时
typedef enum _JCU_NOTIFY_ID_E {
     JCU_NOTIFY_LOGIN           = 0x0000    //login
   , JCU_NOTIFY_LOGOUT                      //logout;
   , JCU_NOTIFY_STREAM_OPEN                 //stream open;
   , JCU_NOTIFY_STREAM_CLOSE                //stream close;
   , JCU_NOTIFY_CFG_SET                     //config set;
   , JCU_NOTIFY_CFG_GET                     //config get;
   , JCU_NOTIFY_PTZ_CTL                     //ptz control;
   , JCU_NOTIFY_DEV_CTL                     //device control;
   , JCU_NOTIFY_RESERVE        = 0x1000     //reserve;
}JCU_NOTIFY_ID_E;

typedef enum _JCU_NOTIFY_ERR_E {
	  JCU_NOTIFY_ERR		   = -1
    , JCU_NOTIFY_ERR_0         = 0x0000     //none error;
    , JCU_NOTIFY_ERR_TIMEOUT                //timeout;
    , JCU_NOTIFY_ERR_CLOSE                  //conn close;
}JCU_NOTIFY_ERR_E;


typedef enum _JCU_OP_E {
      JCU_OP_ASYNC              = 0x0000   //async op; 
    , JCU_OP_SYNC               = 0x0001   //sync op;
}JCU_OP_E;

typedef struct _jcu_notify_cb_s {
    int is_sync;                            //JCU_OP_E(sync:wait && call complete_cb, async:return && callback complete_cb);
    void *user_arg;
    int(*callback)(struct _jcu_notify_cb_s *handle
            , jcu_cb_parm_t *parm);         //parm.id:JCU_NOTIFY_ID_E, parm.args:JCU_NOTIFY_ERR_E;
}jcu_notify_cb_t;

/*
 * event callback define
 */
typedef enum _JCU_EVENT_ID_E {
     JCU_EVNET_OPEN          = 0x0000       //conn open;
   , JCU_EVENT_CLOSE         = 0x0001       //conn close;

   , JCU_EVENT_PARAM         = 0x0002       //暂时不使用
   , JCU_EVENT_ALARM         = 0x1000       //暂时不使用
   
   //ipc use, rongp add
   , JCU_EVENT_UPGRADE		 = 0x2000		//暂时不使用
   , JCU_EVENT_RECORD		 = 0x2100		//暂时不使用
   , JCU_EVENT_REGISTER      = 0x2101
   , JCU_EVENT_CONTROL_DEV   = 0x2102		//暂时不使用
   , JCU_EVENT_UNREGISTER    = 0x2103
}JCU_EVENT_ID_E;

typedef struct _jcu_event_cb_s {
    void *user_arg;
    int (*callback)(struct _jcu_event_cb_s *handle
            , jcu_cb_parm_t *parm);         //parm.id:JCU_EVENT_ID_E, parm.args:JCU_ALARM_ID_E(from j_sdk.h);
}jcu_event_cb_t;

/*
 * stream callback define
 */
typedef enum _JCU_STREAM_EV_E {
      JCU_STREAM_EV_OPEN                    //stream open;
    , JCU_STREAM_EV_RECV                    //recv one frame data;
    , JCU_STREAM_EV_CLOSE                   //stream close;
}JCU_STREAM_EV_E;

typedef struct _jcu_stream_cb_s {
    void *user_arg;
    int(*callback)(struct _jcu_stream_cb_s *handle
            , jcu_cb_parm_t *parm);         //parm.id:JCU_STREAM_EV_E
                                            //parm.args:j_frame_type_t(from Jtype.h)
                                            //parm.data:j_frame_t(from Jtype.h)
}jcu_stream_cb_t;

//语音对讲参数
typedef struct
{
	long		lAudioSample;				//采样频率(8K, 16K)
	long		lEncodeFmt;					//语音包编码格式(0-PCM, 1-G711A, 2-G711U, 3-G726)
	long		lAudioChannel;				//单双通道(1-单, 2-双 注:只支持单通道)
	long		lSampleBit;					//样本占位数(16bit)
}ST_TALK_SAMPLE_PARAM, *PST_TALK_SAMPLE_PARAM;

//语音流回调(数据格式:j_frame_t)
typedef long (*FUNC_VOICE_STREAM_CB)(void* pVoiceData, long lSize, void* pParam);

#define MB_NAME             "mb_lib"
#define MB_VERSION          "v0.0.6"
#define GROUP_IP            "224.0.0.99"


#define GROUP_PORT          40086
#define general_dst         "general_dst"
    
/*
 * 设备类型定义
 */
typedef enum _J_DevType {
    /* --------- IPC ---------- */
      J_DEV_IPC_6001        = 0x0001   //3516 普通枪机
    , J_DEV_IPC_6002        = 0x0002   //3516 红外护罩
    , J_DEV_IPC_6003        = 0x0003   //3516 红外大炮
    , J_DEV_IPC_6004        = 0x0004   //3516 红外中炮    
    , J_DEV_IPC_6005        = 0x0005   //3516 半球(带ircut，带红外灯可选)
    , J_DEV_IPC_6006        = 0x0006   //3516 日夜枪机(无红外灯但带ircut )   
    , J_DEV_IPC_6007        = 0x0007   //3516 普通半球(无红外灯和ircut 半球) 


	, J_DEV_IPC_8001        = 0x0101   //3518A 普通枪机
	, J_DEV_IPC_8002        = 0x0102   //3518A 红外护罩
	, J_DEV_IPC_8003        = 0x0103   //3518A 红外大炮
	, J_DEV_IPC_8004        = 0x0104   //3518A 红外中炮    
	, J_DEV_IPC_8005        = 0x0105   //3518A 半球(带ircut，带红外灯可选)
	, J_DEV_IPC_8006        = 0x0106   //3518A 日夜枪机(无红外灯但带ircut )  
	, J_DEV_IPC_8007        = 0x0107   //3518A 普通半球(无红外灯和ircut 半球)  

	, J_DEV_IPC_8101        = 0x0201   //3518C 普通枪机
	, J_DEV_IPC_8102        = 0x0202   //3518C 红外护罩
	, J_DEV_IPC_8103        = 0x0203   //3518C 红外大炮
	, J_DEV_IPC_8104        = 0x0204   //3518C 红外中炮    
	, J_DEV_IPC_8105        = 0x0205   //3518C 半球(带ircut，带红外灯可选)
	, J_DEV_IPC_8106        = 0x0206   //3518C 日夜枪机(无红外灯但带ircut )  
	, J_DEV_IPC_8107        = 0x0207   //3518C 普通半球(无红外灯和ircut 半球)  

    /* --------- IPNC ---------- */
    , J_Dev_IPNC_           = 0x2000   
    /* --------- DVR ---------- */
    , J_Dev_DVR_2_3520      = 0x4000    //DVR 3520 16-D1
    , J_Dev_DVR_16_3531     = 0x4001    //DVR 3531 16-D1(960)
    , J_Dev_DVR_4_3531      = 0x4002    //DVR 3531 04-HD-SDI
    , J_Dev_DVR_8_3531      = 0x4003    //DVR 3531 08-HD-SDI
    /* --------- NVR 1 ethx ---------- */
    , J_Dev_NVR_1_3520      = 0x5000    //NVR 3520 08-720P
    , J_Dev_NVR_16_3531     = 0x5001    //NVR 3531 16-720P
    , J_Dev_NVR_9_3531      = 0x5002    //NVR 3531 09-1080P
    , J_Dev_NVR_25_3531     = 0x5003    //NVR 3531 25-720P
    , J_Dev_NVR_32_3531     = 0x5004    //NVR 3531 32-720P
    
    /* --------- NVR 2 ethx ---------- */
    , J_Dev_NVR_16_3531_e2  = 0x5011    //NVR 3531 16-720P  2 ethx
    , J_Dev_NVR_9_3531_e2   = 0x5012    //NVR 3531 09-1080P 2 ethx
    , J_Dev_NVR_25_3531_e2  = 0x5013    //NVR 3531 25-720P  2 ethx
    , J_Dev_NVR_32_3531_e2  = 0x5014    //NVR 3531 32-720P  2 ethx
    
    /* --------- NVR for 3521 ---------- */
    , J_Dev_NVR_4_3521      = 0x5020    //NVR 3521 04-1080P
    , J_Dev_NVR_9_3521      = 0x5021    //NVR 3521 09-720P 
    , J_Dev_NVR_16_3521     = 0x5022    //NVR 3521 16-720P 
    
    /* --------- DEC ---------- */    
    , J_Dev_DECC_10_3531    = 0x6000    //DEC CARD 720P 10in, 2out;
    , J_Dev_DECB_13_3531    = 0x6001    //DEC BOX  720P 13in, 2out;
    
}J_DevType_E;
/*-----------------------------------*/

/*
 * 组播配置参数ID定义
 */
typedef enum _J_MB_ParmId {
      J_MB_Device_Id     = 0x0001       /*  设备 搜索&配置  J_Device_T    */
    , J_MB_SysCfg_Id                    /*  设备系统配置    J_SysCfg_T    */
    , J_MB_NetCfg_Id                    /*  设备网络配置    JNetworkInfo  */
    , J_MB_DvrCommCfg_Id                /*  DVR通用配置     J_DvrCommCfg_T*/
    , J_MB_ManufCfg_Id                  /*  设备生产配置    J_ManufCfg_T  */
    , J_MB_DHCPCtl_Id                   /*  设备DHCP控制    J_DhcpCtl_T   */
    , J_MB_SysStatus_Id                 /*  设备运行状态     J_SysStatus_T */
    , J_MB_Decoder_Id    = 0x0008       /*  解码器设备信息	J_Decoder_Info */
    , J_MB_Dec_Test_Id                  /*  解码器 测试输出   J_Decoder_Test */

    , J_MB_Jpf_Search_Id = 0x0020       /*  平台搜索Jpf_Search*/
    , J_MB_Jpf_Redirect_Id              /*  重定向设备登陆平台 Jpf_Redirect*/
}J_MB_ParmId_E;


#define J_VER_STR_LEN       32          //版本信息长度
#define J_SERIAL_NO_LEN     32          //序列号长度
#define J_DVR_NAME_LEN      32          //设备名称长度


typedef struct J_SysCfg_S {

    unsigned char serial_no[J_SERIAL_NO_LEN];       //序列号
    unsigned char device_type[J_VER_STR_LEN];       //设备型号
    unsigned char software_ver[J_VER_STR_LEN];      //软件版本号
    unsigned char software_date[J_VER_STR_LEN];     //软件生成日期
    unsigned char panel_ver[J_VER_STR_LEN];         //前面板版本
    unsigned char hardware_ver[J_VER_STR_LEN];      //硬件版本

    unsigned int  dev_type;                         //设备类型  J_DevType_E
    
    unsigned char ana_chan_num;                     //模拟通道个数
    unsigned char ip_chan_num;                      //数字通道数
    unsigned char dec_chan_num;                     //解码路数
    unsigned char stream_num;                       //通道支持的码流个数
 
    unsigned char audio_in_num;                     //语音输入口的个数
    unsigned char audio_out_num;                    //语音输出口的个数
    unsigned char alarm_in_num;                     //报警输入个数
    unsigned char alarm_out_num;                    //报警输出个数

    unsigned char net_port_num;                     //网络口个数
    unsigned char rs232_num;                        //232串口个数
    unsigned char rs485_num;                        //485串口个数
    unsigned char usb_num;                          //USB口的个数
    
    unsigned char hdmi_num;                         //HDMI口的个数
    unsigned char vga_num;                          //VGA口的个数
    unsigned char cvbs_num;                         //cvbs口的个数
    unsigned char aux_out_num;                      //辅口的个数
    
    unsigned char disk_ctrl_num;                    //硬盘控制器个数
    unsigned char disk_num;                         //硬盘个数
    unsigned char res2[2];                           //保留

    unsigned int max_encode_res;                   //最大编码分辨率
    unsigned int max_display_res;                  //最大显示分辨率
    
    unsigned int   dvr_bit;                         //DVR保留
    unsigned char  dvr_byte[4];                     //DVR保留
    
    unsigned int   ipc_ircut:1;                     //IPC 是否支持IR_CUT
    unsigned int   ipc_bit:31;                      //IPC 保留
    unsigned char  ipc_sensor_type;                 //IPC 图像传感器类型
    unsigned char  ipc_byte[3];                     //IPC 保留
}J_SysCfg_T;

/*
 * DVR常规配置参数
 */
typedef struct J_DvrCommCfg_S
{
    unsigned char 	dvr_name[J_DVR_NAME_LEN];   //DVR 名字
    unsigned int 	dvr_id;                     //DVR ID,(遥控器)
    unsigned int 	recycle_record;             //是否循环录像,0:不是; 1:是
    unsigned char 	language;                   //语言0: 中文  1 :英语
    unsigned char  	video_mode;                 //视频制式0:PAL  1 NTSC
    unsigned char  	out_device;                 //输出设备  0: VGA; 1: HDMI; 2:CVBS
    unsigned char  	resolution;                 //分辨率0 :1024 * 768    1 : 1280 * 720   2: 1280*1024  3: 1920*1080
    unsigned char 	standby_time;               //菜单待机时间 1~60分钟  0: 表示不待机
    unsigned char  	boot_guide;                 //开机向导 0 : 不启动向导  1 : 启动向导
    unsigned char  	resolution2; 				//(副屏)分辨率0 :1024 * 768    1 : 1280 * 720   2: 1280*1024  3: 1920*1080
    unsigned char  	reserve[1];
} J_DvrCommCfg_T;


#define J_MAC_ADDR_LEN        20      //MAC地址长度

/*
 * 用于生产配置
 */
typedef struct J_ManufCfg_S
{
	unsigned char serial_no[J_SERIAL_NO_LEN];       //序列号
	unsigned char device_type[J_VER_STR_LEN];       //设备型号
	unsigned int  dev_type;                         //设备类型  J_DevType_E

	unsigned char ana_chan_num;                     //模拟通道个数
	unsigned char ip_chan_num;                      //数字通道数
	unsigned char dec_chan_num;                     //解码路数
	unsigned char stream_num;                       //通道支持的码流个数

	unsigned char mac[J_MAC_ADDR_LEN];              //MAC地址(字符串,00:0c:29:00:37:6b)
	unsigned int  platform_type;                    //平台类型，参见 j_sdk.h => JPlatformType    
	unsigned int  ivs_type;                    		//3516IPC设备支持的智能分析类型，仅设备端使用
	unsigned int  net_protocol;                    //网络协议，见j_sdk.h => JNetProtocolType
	unsigned char mac2[J_MAC_ADDR_LEN];            // 设备支持双网口时,配置MAC地址2
	unsigned char netport_num;						// 网络口个数
    unsigned char res2[3+128];
}J_ManufCfg_T;

/* 
 * 设备DHCP控制(重启后无效)
 */
typedef struct J_DhcpCtl_S
{
    unsigned short if_type; /* JNetworkType */
    unsigned short dhcp_en; /* 开启&关闭DHCP服务， 0：关闭， 1：开启 */
}J_DhcpCtl_T;


/*
 * 设备运行状态
 */
typedef struct J_SysStatus_S 
{
    unsigned long run_time; //单位s
    unsigned char res[60];
}J_SysStatus_T;
/*
 * 设备信息定义
 */
typedef struct J_Device_S {
    unsigned int    SysSize;
    J_SysCfg_T      SysCfg;
    unsigned int    NetSize;
    JNetworkInfo    NetworkInfo;
}J_Device_T;
typedef J_Device_T j_Device_T;

typedef struct J_Decoder_Info
{
	unsigned long		dwDecoderId;
	unsigned int		nType;
	unsigned char		szName[32];
	unsigned long		lChannelCount;
	unsigned char		szRegPlatformId[32];
	unsigned long		lCmsPort;
	unsigned char		szCmsIp[64];
	/* ------- 硬解器码配置 -------- */ 
	JNetwork			NetCfg;		/*网络配置*/
	unsigned int		dec_no;	/*解码器编号配置*/
}J_Decoder_Info;

/*-----------------------------------*/


/*
 * MB_NOTIFY_ERR_E
 * 错误码
 * cu端需要检查errno以确定是否从pu端返回的是否正  确
 * pu端回调pu_process_cb_t.callback  的返回值需为MB_NOTIFY_ERR_0 或者MB_NOTIFY_ERR_FAILED
 *
 */


typedef enum _MB_NOTIFY_ERR_E {
      MB_NOTIFY_ERR_0         = 0x0000     //none error;
    , MB_NOTIFY_ERR_TIMEOUT                //timeout;
    , MB_NOTIFY_ERR_FAILED                 //cfg set/get failed;
}MB_NOTIFY_ERR_E;




#define MB_SERIAL_LEN (32)                 //组播序列号长度
#define MB_USER_LEN   (32)                 //组播用户名长度
#define MB_PASS_LEN   (32)                 //组播密码长度


/*-------------- cu -------------------*/
/*
 * mb_cu_parm_t
 * cu 端回调参数
 *
 * id:消息ID
 * error:设备返回的消息错误码，MB_NOTIFY_ERR_0代表成功
 * size:设备返回的结构体大小
 * data:返回的结构体地址
 * dst_id:返回的组播序列ID，
 */
typedef struct _mb_cu_parm_s {
    int  id;                                //id;
    int  error;                              //error;
    int  size;                              //data size;
    char *data;                             //data;
    char dst_id[MB_SERIAL_LEN];                           //dst_id  此字符串区分从哪个pu  返回的消息
}mb_cu_parm_t;

/* 
  * mb_cu_notify_t 
  * cu 端回调结构体
  *
  */
typedef struct _mb_cu_notify_s {
    void *user_arg;
    int(*callback)(struct _mb_cu_notify_s *handle
            , mb_cu_parm_t *parm);        
}mb_cu_notify_t;


//HVOICE：该类型即将废弃
typedef void *				HVOICE;
//DWORD：该类型即将废弃
typedef unsigned long       DWORD;

#ifdef __cplusplus
}
#endif
#endif //__jcu_net_api__