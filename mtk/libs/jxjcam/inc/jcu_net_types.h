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
//ע�⣺������ʹ�á�֪ͨ�ص�ʹ�� j_sdk.h�е�PARAMxxx, 
//�ǲ�������parm.id��Ч(��Чֵ�ݶ�Ϊ-1���� ����Login��JCU_NOTIFY_STREAM_OPENʱ
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

   , JCU_EVENT_PARAM         = 0x0002       //��ʱ��ʹ��
   , JCU_EVENT_ALARM         = 0x1000       //��ʱ��ʹ��
   
   //ipc use, rongp add
   , JCU_EVENT_UPGRADE		 = 0x2000		//��ʱ��ʹ��
   , JCU_EVENT_RECORD		 = 0x2100		//��ʱ��ʹ��
   , JCU_EVENT_REGISTER      = 0x2101
   , JCU_EVENT_CONTROL_DEV   = 0x2102		//��ʱ��ʹ��
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

//�����Խ�����
typedef struct
{
	long		lAudioSample;				//����Ƶ��(8K, 16K)
	long		lEncodeFmt;					//�����������ʽ(0-PCM, 1-G711A, 2-G711U, 3-G726)
	long		lAudioChannel;				//��˫ͨ��(1-��, 2-˫ ע:ֻ֧�ֵ�ͨ��)
	long		lSampleBit;					//����ռλ��(16bit)
}ST_TALK_SAMPLE_PARAM, *PST_TALK_SAMPLE_PARAM;

//�������ص�(���ݸ�ʽ:j_frame_t)
typedef long (*FUNC_VOICE_STREAM_CB)(void* pVoiceData, long lSize, void* pParam);

#define MB_NAME             "mb_lib"
#define MB_VERSION          "v0.0.6"
#define GROUP_IP            "224.0.0.99"


#define GROUP_PORT          40086
#define general_dst         "general_dst"
    
/*
 * �豸���Ͷ���
 */
typedef enum _J_DevType {
    /* --------- IPC ---------- */
      J_DEV_IPC_6001        = 0x0001   //3516 ��ͨǹ��
    , J_DEV_IPC_6002        = 0x0002   //3516 ���⻤��
    , J_DEV_IPC_6003        = 0x0003   //3516 �������
    , J_DEV_IPC_6004        = 0x0004   //3516 ��������    
    , J_DEV_IPC_6005        = 0x0005   //3516 ����(��ircut��������ƿ�ѡ)
    , J_DEV_IPC_6006        = 0x0006   //3516 ��ҹǹ��(�޺���Ƶ���ircut )   
    , J_DEV_IPC_6007        = 0x0007   //3516 ��ͨ����(�޺���ƺ�ircut ����) 


	, J_DEV_IPC_8001        = 0x0101   //3518A ��ͨǹ��
	, J_DEV_IPC_8002        = 0x0102   //3518A ���⻤��
	, J_DEV_IPC_8003        = 0x0103   //3518A �������
	, J_DEV_IPC_8004        = 0x0104   //3518A ��������    
	, J_DEV_IPC_8005        = 0x0105   //3518A ����(��ircut��������ƿ�ѡ)
	, J_DEV_IPC_8006        = 0x0106   //3518A ��ҹǹ��(�޺���Ƶ���ircut )  
	, J_DEV_IPC_8007        = 0x0107   //3518A ��ͨ����(�޺���ƺ�ircut ����)  

	, J_DEV_IPC_8101        = 0x0201   //3518C ��ͨǹ��
	, J_DEV_IPC_8102        = 0x0202   //3518C ���⻤��
	, J_DEV_IPC_8103        = 0x0203   //3518C �������
	, J_DEV_IPC_8104        = 0x0204   //3518C ��������    
	, J_DEV_IPC_8105        = 0x0205   //3518C ����(��ircut��������ƿ�ѡ)
	, J_DEV_IPC_8106        = 0x0206   //3518C ��ҹǹ��(�޺���Ƶ���ircut )  
	, J_DEV_IPC_8107        = 0x0207   //3518C ��ͨ����(�޺���ƺ�ircut ����)  

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
 * �鲥���ò���ID����
 */
typedef enum _J_MB_ParmId {
      J_MB_Device_Id     = 0x0001       /*  �豸 ����&����  J_Device_T    */
    , J_MB_SysCfg_Id                    /*  �豸ϵͳ����    J_SysCfg_T    */
    , J_MB_NetCfg_Id                    /*  �豸��������    JNetworkInfo  */
    , J_MB_DvrCommCfg_Id                /*  DVRͨ������     J_DvrCommCfg_T*/
    , J_MB_ManufCfg_Id                  /*  �豸��������    J_ManufCfg_T  */
    , J_MB_DHCPCtl_Id                   /*  �豸DHCP����    J_DhcpCtl_T   */
    , J_MB_SysStatus_Id                 /*  �豸����״̬     J_SysStatus_T */
    , J_MB_Decoder_Id    = 0x0008       /*  �������豸��Ϣ	J_Decoder_Info */
    , J_MB_Dec_Test_Id                  /*  ������ �������   J_Decoder_Test */

    , J_MB_Jpf_Search_Id = 0x0020       /*  ƽ̨����Jpf_Search*/
    , J_MB_Jpf_Redirect_Id              /*  �ض����豸��½ƽ̨ Jpf_Redirect*/
}J_MB_ParmId_E;


#define J_VER_STR_LEN       32          //�汾��Ϣ����
#define J_SERIAL_NO_LEN     32          //���кų���
#define J_DVR_NAME_LEN      32          //�豸���Ƴ���


typedef struct J_SysCfg_S {

    unsigned char serial_no[J_SERIAL_NO_LEN];       //���к�
    unsigned char device_type[J_VER_STR_LEN];       //�豸�ͺ�
    unsigned char software_ver[J_VER_STR_LEN];      //����汾��
    unsigned char software_date[J_VER_STR_LEN];     //�����������
    unsigned char panel_ver[J_VER_STR_LEN];         //ǰ���汾
    unsigned char hardware_ver[J_VER_STR_LEN];      //Ӳ���汾

    unsigned int  dev_type;                         //�豸����  J_DevType_E
    
    unsigned char ana_chan_num;                     //ģ��ͨ������
    unsigned char ip_chan_num;                      //����ͨ����
    unsigned char dec_chan_num;                     //����·��
    unsigned char stream_num;                       //ͨ��֧�ֵ���������
 
    unsigned char audio_in_num;                     //��������ڵĸ���
    unsigned char audio_out_num;                    //��������ڵĸ���
    unsigned char alarm_in_num;                     //�����������
    unsigned char alarm_out_num;                    //�����������

    unsigned char net_port_num;                     //����ڸ���
    unsigned char rs232_num;                        //232���ڸ���
    unsigned char rs485_num;                        //485���ڸ���
    unsigned char usb_num;                          //USB�ڵĸ���
    
    unsigned char hdmi_num;                         //HDMI�ڵĸ���
    unsigned char vga_num;                          //VGA�ڵĸ���
    unsigned char cvbs_num;                         //cvbs�ڵĸ���
    unsigned char aux_out_num;                      //���ڵĸ���
    
    unsigned char disk_ctrl_num;                    //Ӳ�̿���������
    unsigned char disk_num;                         //Ӳ�̸���
    unsigned char res2[2];                           //����

    unsigned int max_encode_res;                   //������ֱ���
    unsigned int max_display_res;                  //�����ʾ�ֱ���
    
    unsigned int   dvr_bit;                         //DVR����
    unsigned char  dvr_byte[4];                     //DVR����
    
    unsigned int   ipc_ircut:1;                     //IPC �Ƿ�֧��IR_CUT
    unsigned int   ipc_bit:31;                      //IPC ����
    unsigned char  ipc_sensor_type;                 //IPC ͼ�񴫸�������
    unsigned char  ipc_byte[3];                     //IPC ����
}J_SysCfg_T;

/*
 * DVR�������ò���
 */
typedef struct J_DvrCommCfg_S
{
    unsigned char 	dvr_name[J_DVR_NAME_LEN];   //DVR ����
    unsigned int 	dvr_id;                     //DVR ID,(ң����)
    unsigned int 	recycle_record;             //�Ƿ�ѭ��¼��,0:����; 1:��
    unsigned char 	language;                   //����0: ����  1 :Ӣ��
    unsigned char  	video_mode;                 //��Ƶ��ʽ0:PAL  1 NTSC
    unsigned char  	out_device;                 //����豸  0: VGA; 1: HDMI; 2:CVBS
    unsigned char  	resolution;                 //�ֱ���0 :1024 * 768    1 : 1280 * 720   2: 1280*1024  3: 1920*1080
    unsigned char 	standby_time;               //�˵�����ʱ�� 1~60����  0: ��ʾ������
    unsigned char  	boot_guide;                 //������ 0 : ��������  1 : ������
    unsigned char  	resolution2; 				//(����)�ֱ���0 :1024 * 768    1 : 1280 * 720   2: 1280*1024  3: 1920*1080
    unsigned char  	reserve[1];
} J_DvrCommCfg_T;


#define J_MAC_ADDR_LEN        20      //MAC��ַ����

/*
 * ������������
 */
typedef struct J_ManufCfg_S
{
	unsigned char serial_no[J_SERIAL_NO_LEN];       //���к�
	unsigned char device_type[J_VER_STR_LEN];       //�豸�ͺ�
	unsigned int  dev_type;                         //�豸����  J_DevType_E

	unsigned char ana_chan_num;                     //ģ��ͨ������
	unsigned char ip_chan_num;                      //����ͨ����
	unsigned char dec_chan_num;                     //����·��
	unsigned char stream_num;                       //ͨ��֧�ֵ���������

	unsigned char mac[J_MAC_ADDR_LEN];              //MAC��ַ(�ַ���,00:0c:29:00:37:6b)
	unsigned int  platform_type;                    //ƽ̨���ͣ��μ� j_sdk.h => JPlatformType    
	unsigned int  ivs_type;                    		//3516IPC�豸֧�ֵ����ܷ������ͣ����豸��ʹ��
	unsigned int  net_protocol;                    //����Э�飬��j_sdk.h => JNetProtocolType
	unsigned char mac2[J_MAC_ADDR_LEN];            // �豸֧��˫����ʱ,����MAC��ַ2
	unsigned char netport_num;						// ����ڸ���
    unsigned char res2[3+128];
}J_ManufCfg_T;

/* 
 * �豸DHCP����(��������Ч)
 */
typedef struct J_DhcpCtl_S
{
    unsigned short if_type; /* JNetworkType */
    unsigned short dhcp_en; /* ����&�ر�DHCP���� 0���رգ� 1������ */
}J_DhcpCtl_T;


/*
 * �豸����״̬
 */
typedef struct J_SysStatus_S 
{
    unsigned long run_time; //��λs
    unsigned char res[60];
}J_SysStatus_T;
/*
 * �豸��Ϣ����
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
	/* ------- Ӳ���������� -------- */ 
	JNetwork			NetCfg;		/*��������*/
	unsigned int		dec_no;	/*�������������*/
}J_Decoder_Info;

/*-----------------------------------*/


/*
 * MB_NOTIFY_ERR_E
 * ������
 * cu����Ҫ���errno��ȷ���Ƿ��pu�˷��ص��Ƿ���  ȷ
 * pu�˻ص�pu_process_cb_t.callback  �ķ���ֵ��ΪMB_NOTIFY_ERR_0 ����MB_NOTIFY_ERR_FAILED
 *
 */


typedef enum _MB_NOTIFY_ERR_E {
      MB_NOTIFY_ERR_0         = 0x0000     //none error;
    , MB_NOTIFY_ERR_TIMEOUT                //timeout;
    , MB_NOTIFY_ERR_FAILED                 //cfg set/get failed;
}MB_NOTIFY_ERR_E;




#define MB_SERIAL_LEN (32)                 //�鲥���кų���
#define MB_USER_LEN   (32)                 //�鲥�û�������
#define MB_PASS_LEN   (32)                 //�鲥���볤��


/*-------------- cu -------------------*/
/*
 * mb_cu_parm_t
 * cu �˻ص�����
 *
 * id:��ϢID
 * error:�豸���ص���Ϣ�����룬MB_NOTIFY_ERR_0����ɹ�
 * size:�豸���صĽṹ���С
 * data:���صĽṹ���ַ
 * dst_id:���ص��鲥����ID��
 */
typedef struct _mb_cu_parm_s {
    int  id;                                //id;
    int  error;                              //error;
    int  size;                              //data size;
    char *data;                             //data;
    char dst_id[MB_SERIAL_LEN];                           //dst_id  ���ַ������ִ��ĸ�pu  ���ص���Ϣ
}mb_cu_parm_t;

/* 
  * mb_cu_notify_t 
  * cu �˻ص��ṹ��
  *
  */
typedef struct _mb_cu_notify_s {
    void *user_arg;
    int(*callback)(struct _mb_cu_notify_s *handle
            , mb_cu_parm_t *parm);        
}mb_cu_notify_t;


//HVOICE�������ͼ�������
typedef void *				HVOICE;
//DWORD�������ͼ�������
typedef unsigned long       DWORD;

#ifdef __cplusplus
}
#endif
#endif //__jcu_net_api__