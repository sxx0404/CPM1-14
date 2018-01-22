#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#include "Jtype.h"
#include "j_sdk.h"
#include "jcu_net_api.h"

#define ASSERT(x) assert(x)

#define SAVE_STR "[Current device ip:%s] "

int upgrade_flag = 0;

FILE *vfp = NULL;

jcu_talk_cli_t *thdl = NULL;
unsigned long talk_frame_num = 0;

char priv_save_str[1024] = {0};
char *priv = NULL;
jcu_user_handle_t *uhdl = NULL;
int login_success_flag = 1;

char *my_fgets(char *buf, int len);
void get_path_name(char *pathname, int size);
void get_user_login_parm(char *ip, /*ip_size*/int *timeout, char *usern, /*user size*/char *userp/*password size*/);
void get_user_play_parm(char *pathname, int len);

int set_notify_cb(jcu_notify_cb_t *nhdl, jcu_cb_parm_t *parm)
{
    ASSERT(parm);
    if (parm->args == JCU_NOTIFY_ERR_0) {
        if (0 == *(int *)parm->data) {
            printf("set notify ok\n");
            return 0;
        }
    }

    printf("set notify failed!\n");
    return -1;
}

int notify_cb(jcu_notify_cb_t *nhdl, jcu_cb_parm_t *parm)
{
    ASSERT(parm);

    if (parm->args == JCU_NOTIFY_ERR) {
        printf("login failed! uasrname or password error!\n");
        return -1;
    } else if (parm->args == JCU_NOTIFY_ERR_TIMEOUT) {
        printf("notify_cb parm->id:%d timeout!\n", parm->id);
        if (parm->id == PARAM_LOGIN) {
            login_success_flag = 0;
        }
        return -1;
    }

    switch(parm->id) {
        case PARAM_SYNC_TIME:         
            printf("PARAM_SYNC_TIME response\n");
            break;
        case PARAM_CHANGE_DISPATCH:  
            printf("PARAM_CHANGE_DISPATCH response\n");
            break;
        case PARAM_DEVICE_INFO:      
            printf("PARAM_DEVICE_INFO response\n");
            break;
        case PARAM_DEVICE_NTP_INFO:   
            printf("PARAM_DEVICE_NTP_INFO response\n");
            break;
        case PARAM_DEVICE_TIME:      
            printf("PARAM_DEVICE_TIME response\n");
            break;
        case PARAM_PLATFORM_INFO:    
            printf("PARAM_PLATFORM_INFO response\n");
            break;
        case PARAM_NETWORK_INFO:     
            printf("PARAM_NETWORK_INFO response\n");
            break;
        case PARAM_PPPOE_INFOE:      
            printf("PARAM_PPPOE_INFOE response\n");
            break;
        case PARAM_ENCODE_INFO:      
            printf("PARAM_ENCODE_INFO response\n");
            {
                JEncodeParameter *enc = (JEncodeParameter*)parm->data;
                printf( "enc->level             %d\n"
                        "enc->frame_rate        %d\n"
                        "enc->i_frame_interval  %d\n"
                        "enc->video_type        %d\n"
                        "enc->audio_type        %d\n"
                        "enc->audio_enble       %d\n"
                        "enc->resolution        %d\n"
                        "enc->qp_value          %d\n"
                        "enc->code_rate         %d\n"
                        "enc->frame_priority    %d\n"
                        "enc->format            %d\n"
                        "enc->bit_rate          %d\n"
                        "enc->encode_level      %d\n"
                        , enc->level
                        , enc->frame_rate
                        , enc->i_frame_interval
                        , enc->video_type
                        , enc->audio_type
                        , enc->audio_enble
                        , enc->resolution
                        , enc->qp_value
                        , enc->code_rate
                        , enc->frame_priority
                        , enc->format
                        , enc->bit_rate
                        , enc->encode_level);
            }
            break;
        case PARAM_DISPLAY_INFO:      
            printf("PARAM_DISPLAY_INFO response\n");
            break;
        case PARAM_RECORD_INFO:      
            printf("PARAM_RECORD_INFO response\n");
            {
                JRecordParameter *pstRecParm = (JRecordParameter *)parm->data;
                printf("pstRecParm->level:%d pstRecParm->pre_record:%d\n", pstRecParm->level, pstRecParm->pre_record);
            }
            break;
        case PARAM_HIDE_INFO:        
            printf("PARAM_HIDE_INFO response\n");
            break;
        case PARAM_SERIAL_INFO:       
            printf("PARAM_SERIAL_INFO response\n");
            break;
        case PARAM_OSD_INFO:         
            printf("PARAM_OSD_INFO response\n");
            break;
        case PARAM_PTZ_INFO:         
            printf("PARAM_PTZ_INFO response\n");
            break;
        case PARAM_FTP_INFO:         
            printf("PARAM_FTP_INFO response\n");
            break;
        case PARAM_SMTP_INFO:         
            printf("PARAM_SMTP_INFO response\n");
            break;
        case PARAM_UPNP_INFO:        
            printf("PARAM_UPNP_INFO response\n");
            break;
        case PARAM_DISK_INFO:        
            printf("PARAM_DISK_INFO response\n");
            break;
        case PARAM_FORMAT_DISK:       
            printf("PARAM_FORMAT_DISK response\n");
            break;
        case PARAM_MOVE_ALARM:       
            printf("PARAM_MOVE_ALARM response\n");
            break;
        case PARAM_LOST_ALARM:       
            printf("PARAM_LOST_ALARM response\n");
            break;
        case PARAM_HIDE_ALARM:       
            printf("PARAM_HIDE_ALARM response\n");
            break;
        case PARAM_IO_ALARM:         
            printf("PARAM_IO_ALARM response\n");
            break;
        case PARAM_JOINT_INFO:       
            printf("PARAM_JOINT_INFO response\n");
            break;
        case PARAM_CONTROL_PTZ:       
            printf("PARAM_CONTROL_PTZ response\n");
            break;
        case PARAM_SUBMINT_ALARM:     
            printf("PARAM_SUBMINT_ALARM response\n");
            break;
        case PARAM_MEDIAL_URL:       
            printf("PARAM_MEDIAL_URL response\n");
            break;
        case PARAM_STORE_LOG:        
            printf("PARAM_STORE_LOG response\n");
            break;
        case PARAM_USER_LOGIN:        
            printf("PARAM_USER_LOGIN response\n");
            break;
        case PARAM_FIRMWARE_UPGRADE: 
            printf("PARAM_FIRMWARE_UPGRADE response\n");
            break;
        case PARAM_DEVICE_CAP:       
            printf("PARAM_DEVICE_CAP response\n");
            break;
        case PARAM_SEARCH_DEV:       
            printf("PARAM_SEARCH_DEV response\n");
            break;
        case PARAM_CHANNEL_INFO:      
            printf("ARAM_CHANNEL_INFO response\n");
            break;
        case PARAM_PIC_INFO:         
            printf("PARAM_PIC_INFO response\n");
            break;
        case PARAM_WIFI_INFO:         
            printf("PARAM_WIFI_INFO response\n");
            break;
        case PARAM_WIFI_SEARCH:       
            printf("PARAM_WIFI_SEARCH response\n");
            break;
        case PARAM_USER_CONFIG:      
            printf("PARAM_USER_CONFIG response\n");
            JUserConfig *juc = (JUserConfig*)parm->data;
            printf("current user name:%s password:%s\n", 
                    juc->username, juc->password);

            break;
        case PARAM_CONTROL_DEV:      
            printf("PARAM_CONTROL_DEV response\n");
            break;
        case PARAM_NETWORK_STATUS:    
            printf("PARAM_NETWORK_STATUS response\n");
            break;
        case PARAM_INTEREST_CODING:   
            printf("PARAM_INTEREST_CODING response\n");
            break;
        case PARAM_DDNS_CODING:      
            printf("PARAM_DDNS_CODING response\n");
            break;
        case PARAM_DEF_DISPLAY_INFO: 
            printf("PARAM_DEF_DISPLAY_INFO response\n");
            break;
        case PARAM_DEF_PICTURE_INFO:  
            printf("PARAM_DEF_PICTURE_INFO response\n");
            break;
        case PARAM_HXHT_CONFIG:      
            printf("PARAM_HXHT_CONFIG response\n");
            break;
        case PARAM_MODIFY_USER_CFG:  
            printf("PARAM_MODIFY_USER_CFG response\n");
            break;
        case PARAM_AVD_CFG:          
            printf("PARAM_AVD_CFG response\n");
            break;
        case PARAM_GB28181_PROTO_CFG:
            printf("PARAM_GB28181_PROTO_CFG response\n");
            break;
        case PARAM_NETWORK_DISK_CFG: 
            printf("PARAM_NETWORK_DISK_CFG response\n");
            break;
        case PARAM_DEV_WORK_STATE:   
            printf("PARAM_DEV_WORK_STATE response\n");
            break;
        case PARAM_OPERATION_LOG:    
            printf("PARAM_OPERATION_LOG response\n");
            break;
        case PARAM_ALARM_UPLOAD_CFG: 
            printf("PARAM_ALARM_UPLOAD_CFG response\n");
            break;
        case PARAM_PRESET_POINT_CFG: 
            printf("PARAM_PRESET_POINT_CFG response\n");
            break;
        case PARAM_CRUISE_POINT_CFG: 
            printf("PARAM_CRUISE_POINT_CFG response\n");
            break;
        case PARAM_CRUISE_WAY_CFG:   
            printf("PARAM_CRUISE_WAY_CFG response\n");
            break;
        case PARAM_VENC_AUTO_SWITCH: 
            printf("PARAM_VENC_AUTO_SWITCH response\n");
            break;
        case PARAM_3D_CONTROL:       
            printf("PARAM_3D_CONTROL response\n");
            break;
        case PARAM_SET_DEVICE_TIME:  
            printf("PARAM_SET_DEVICE_TIME response\n");
            break;
        case PARAM_SYNC_CMS_TIME:    
            printf("PARAM_SYNC_CMS_TIME response\n");
            break;
        case PARAM_IR_CUT_CFG:      
            printf("PARAM_IR_CUT_CFG response\n");
            break;
        case PARAM_NETWORKFAULTCHECK:
            printf("PARAM_NETWORKFAULTCHECK response\n");
            break;
        case PARAM_HERDANALYSE:
            printf("PARAM_HERDANALYSE response\n");
            break;
        case PARAM_ALARM_LINK_IO:            
            printf("RAM_ALARM_LINK_IO response\n");
            break;
        case PARAM_ALARM_LINK_PRESET:        
            printf("PARAM_ALARM_LINK_PRESET response\n");
            break;
        case PARAM_RESOLUTION_CFG:           
            printf("PARAM_RESOLUTION_CFG response\n");
            break;
        case PARAM_IRCUT_CONTROL:            
            printf("PARAM_IRCUT_CONTROL response\n");
            break;
        case PARAM_3D_GOBACK:                
            printf("PARAM_3D_GOBACK response\n");
            break;
        default:
            if (parm->id != -1) //-1 maybe means login ok, but use open event instead, so just discard
                printf("notify_cb Unknow id:%d return!\n", parm->id);
    }

    return 0;
}

int event_cb(struct _jcu_event_cb_s *handle, jcu_cb_parm_t *parm)
{
    ASSERT(parm);

    switch(parm->id) {
        case JCU_EVNET_OPEN:
            //printf("event_cb open\n");
            break;
        case JCU_EVENT_CLOSE:
            //printf("event_cb close\n");
            break;
        case PARAM_FIRMWARE_UPGRADE:
            if (-1 == parm->args) {
                printf("upgrade failed!\n");        
            } else {
                JUpgradeProgress *prs = (JUpgradeProgress*)parm->data;
                printf("upgrade callback percent: %d\n", prs->percent);
                if (prs->percent == 100) 
                    upgrade_flag = 1;
            }
            break;
        default:
            //printf("event_cb UnKnow id:%d return\n", parm->id); 
            break;
    }

    return 0;
}

int stream_notify_cb(jcu_notify_cb_t *nhdl, jcu_cb_parm_t *parm)
{
    ASSERT(parm);

    if (parm->args == JCU_NOTIFY_ERR_TIMEOUT) {
        printf("stream notify_cb parm->id:%d timeout!\n", parm->id);
        return -1;
    }
    return 0;
}

int stream_event_cb(jcu_stream_cb_t *handle, jcu_cb_parm_t *parm)
{
    ASSERT(parm);

    j_frame_t *frame = (j_frame_t*)parm->data;

    if (parm->args == -1) {
        printf("stream_cb parm->id:%d error!\n", parm->id);
        return -1;
    } 

    if (parm->id == JCU_STREAM_EV_OPEN) 
    {
        //printf("stream open\n");
    } else if(parm->id == JCU_STREAM_EV_RECV) 
    {
       // printf("stream => frame_num:%lu, frame_type:%d codec:%d "
        //        "timestamp_sec:%ld timestamp_usec:%ld size:%ld\n"
         //       , frame->frame_num, frame->frame_type, frame->codec
         //       , frame->timestamp_sec, frame->timestamp_usec, frame->size);
        if (NULL != vfp && (frame->frame_type == j_video_i_frame || 
                    frame->frame_type == j_video_p_frame ||
                    frame->frame_type == j_video_b_frame ||
                    frame->frame_type == j_audio_frame) )
            fwrite(frame->data, frame->size, 1, vfp);
            
    } else if (parm->id == JCU_STREAM_EV_CLOSE) 
    {
       
    } else 
    {
    
        return -1;
    }

    return 0;
}

int check_item_num(char *item_str, int max)
{
    while(*item_str && isdigit(*item_str))
        item_str++;

    if (*item_str)
        return 0;

    int item_num = atoi(item_str);

    return item_num<=max?item_num:0;
}

char start_time[32];
char end_time[32];
unsigned int rec_type = 0;
/*#include "j_xmlpacket.h"*/
int record_query_notify_cb(struct _jcu_notify_cb_s *handle, jcu_cb_parm_t *parm)
{
    ASSERT(parm);

    if (parm->args == JCU_NOTIFY_ERR_TIMEOUT) {
        printf("record query notify_cb parm->id:%d timeout!\n", parm->id);
        return -1;
    }

    JStoreLog *stpSorPkg = (JStoreLog *)parm->data;

    int i;
    for (i = 0; i < stpSorPkg->total_count; ++i) 
        printf("item num[%d] ssee_id:%d %d%02d%02d-%d:%d:%d  start_time:%s end_time:%s\n", 
                i, stpSorPkg->sess_id, 
                stpSorPkg->store[0].beg_time.year+1900, stpSorPkg->store[0].beg_time.month, stpSorPkg->store[0].beg_time.date,
                stpSorPkg->store[0].beg_time.hour, stpSorPkg->store[0].beg_time.minute, stpSorPkg->store[0].beg_time.second,
                start_time, end_time);

    if (stpSorPkg->total_count) {
        char item_str[16];
        int item_num;
        while(1) {
            printf("Enter item num: ");
            fflush(stdout);
            my_fgets(item_str, sizeof(item_str));
            if (!(item_num = check_item_num(item_str, stpSorPkg->total_count-1))) {
                printf("item num:%s invalid!\n", item_str);
            } else {
                break;
            }
        }
        sprintf(start_time, "%04d%02d%02d%02d%02d%02d", 
                stpSorPkg->store[item_num].beg_time.year+1900, stpSorPkg->store[item_num].beg_time.month, stpSorPkg->store[item_num].beg_time.date,
                stpSorPkg->store[item_num].beg_time.hour, stpSorPkg->store[item_num].beg_time.minute, stpSorPkg->store[item_num].beg_time.second);

        //sprintf(start_time, "20121118230601");
        sprintf(end_time, "%04d%02d%02d%02d%02d%02d", 
                stpSorPkg->store[item_num].end_time.year+1900, stpSorPkg->store[item_num].end_time.month, stpSorPkg->store[item_num].end_time.date,
                stpSorPkg->store[item_num].end_time.hour, stpSorPkg->store[item_num].end_time.minute, stpSorPkg->store[item_num].end_time.second);

        //sprintf(end_time, "20121118231421");
        rec_type = stpSorPkg->store[item_num].rec_type;
    } else {
        printf("record not found!\n");
        rec_type = -1;
    }

    return 0;
}

int record_stream_notify_cb(struct _jcu_notify_cb_s *handle, jcu_cb_parm_t *parm)
{
    if (parm->args == JCU_NOTIFY_ERR_0)
        printf("record_play_nfy_cb: open stream ok=====================\n");
    else 
        printf("record_play_nfy_cb: open stream failed=====================\n");

}

int time_convert(char *str, JTime *time)
{
    int year, mon, day, hour, min, sec;
    if (sscanf(str, "%4d%2d%2d%2d%2d%2d", 
                &year, &mon, &day, &hour, &min, &sec) != 6)
        return -1;

    time->year  = year-1900;
    time->month = mon;
    time->date  = day;
    time->hour  = hour;
    time->minute= min;
    time->second= sec;
    return 0;
}

#define CALL_CU_API_RETURN_IF_FAILED(func, args...) \
    do {\
        int ret;\
        if (SUCCESS != (ret = func(args))) { \
            fprintf(stderr, "call func:"#func " failed! return value %d\n", ret);\
            return ret;\
        }\
    }while(0) 

int map[] = {
    [PARAM_SYNC_TIME]        = sizeof(JTime)               //= 3,			//同步服务器时�?        ,[PARAM_CHANGE_DISPATCH]  = -1                          //= 4,			//更改流媒体服务器
        ,[PARAM_DEVICE_INFO]      = sizeof(JDeviceInfo)         //= 5,			//设备出厂信息
        ,[PARAM_DEVICE_NTP_INFO]  = sizeof(JDeviceNTPInfo)      //= 6,			//NTP 信息
        ,[PARAM_DEVICE_TIME]      = sizeof(JDeviceTime)         //= 7,			//设置时间信息(保留)-->扩展: 60,61
        ,[PARAM_PLATFORM_INFO]    = sizeof(JPlatformInfo)       //= 8,			//平台信息
        ,[PARAM_NETWORK_INFO]     = sizeof(JNetworkInfo)        //= 9,			//网络信息
        ,[PARAM_PPPOE_INFOE]      = sizeof(JPPPOEInfo)          //= 10,			//PPPOE拨号信息
        ,[PARAM_ENCODE_INFO]      = sizeof(JEncodeParameter)    //= 11,			//编码参数
        ,[PARAM_DISPLAY_INFO]     = sizeof(JDisplayParameter)   //= 12,			//显示参数
        ,[PARAM_RECORD_INFO]      = sizeof(JRecordParameter)    //= 13,			//录像参数
        ,[PARAM_HIDE_INFO]        = sizeof(JHideParameter)      //= 14,			//遮挡参数
        ,[PARAM_SERIAL_INFO]      = sizeof(JSerialParameter)    //= 15,			//串口参数
        ,[PARAM_OSD_INFO]         = sizeof(JOSDParameter)       //= 16,			//OSD参数
        ,[PARAM_PTZ_INFO]         = sizeof(JPTZParameter)       //= 17,			//云台信息
        ,[PARAM_FTP_INFO]         = sizeof(JFTPParameter)       //= 18,			//ftp 参数
        ,[PARAM_SMTP_INFO]        = sizeof(JSMTPParameter)      //= 19,			//smtp 参数
        ,[PARAM_UPNP_INFO]        = sizeof(JUPNPParameter)      //= 20,			//upnp 参数
        ,[PARAM_DISK_INFO]        = sizeof(JDeviceDiskInfo)     //= 21,			//磁盘信息
        ,[PARAM_FORMAT_DISK]      = sizeof(JFormatDisk)         //= 22,			//格式磁盘
        ,[PARAM_MOVE_ALARM]       = sizeof(JMoveAlarm)          //= 23,			//移动告警
        ,[PARAM_LOST_ALARM]       = sizeof(JLostAlarm)          //= 24,			//丢失告警
        ,[PARAM_HIDE_ALARM]       = sizeof(JHideAlarm)          //= 25,			//遮挡告警
        ,[PARAM_IO_ALARM]         = sizeof(JIoAlarm)            //= 26,			//IO 告警
        ,[PARAM_JOINT_INFO]       = sizeof(JJointAction)        //= 27,			//联动操作
        ,[PARAM_CONTROL_PTZ]      = sizeof(JPTZControl)         //= 28,			//云镜控制
        ,[PARAM_SUBMINT_ALARM]    = -1                          //= 29,			//上报告警
        ,[PARAM_MEDIAL_URL]       = sizeof(JMediaUrl)           //= 30,			//流媒�?URL
        ,[PARAM_STORE_LOG]        = sizeof(JStoreLog)           //= 31,			//视频检�?        ,[PARAM_USER_LOGIN]       = sizeof(JUserInfo)           //= 32,			//用户登录请求
        ,[PARAM_FIRMWARE_UPGRADE] = sizeof(JEncodeParameter)    //= 33,			//固件升级
        ,[PARAM_DEVICE_CAP]       = sizeof(JDevCap)             //= 34,			//设备能力�?        ,[PARAM_SEARCH_DEV]       = -1                          //= 35,			//局域网设备搜索
        ,[PARAM_CHANNEL_INFO]     = sizeof(JChannelInfo)        //= 36,			//设备通道配置
        ,[PARAM_PIC_INFO]         = sizeof(JPictureInfo)        //= 37,			//图像参数
        ,[PARAM_WIFI_INFO]        = sizeof(JWifiConfig)         //= 38,			//wifi 配置
        ,[PARAM_WIFI_SEARCH]      = sizeof(JWifiSearchRes)      //= 39,			//wifi 搜索
        ,[PARAM_USER_CONFIG]      = sizeof(JUserConfig)         //= 40,			//用户信息配置JUserConfig
        ,[PARAM_CONTROL_DEV]      = sizeof(JControlDevice)      //= 41,			//控制设备
        ,[PARAM_NETWORK_STATUS]   = sizeof(JNetworkStatus)      //= 42,			//网络连接状�?        ,[PARAM_INTEREST_CODING]  = sizeof(JRegionConfig)       //= 43,			//感兴趣区域编�?        ,[PARAM_DDNS_CODING]      = sizeof(JDdnsConfig)         //= 44,			//动态域名服�?        ,[PARAM_DEF_DISPLAY_INFO] = sizeof(JDisplayParameter)   //= 45,			//默认显示参数
        ,[PARAM_DEF_PICTURE_INFO] = sizeof(JPictureInfo)        //= 46,			//默认图像参数
        ,[PARAM_HXHT_CONFIG]      = sizeof(JHxhtPfConfig)       //= 47,			//互信互通平台JHxhxPfConfig
        ,[PARAM_MODIFY_USER_CFG]  = sizeof(JUserModConfig)      //= 48,			//修改用户信息JUserModConfig
        ,[PARAM_AVD_CFG]          = sizeof(JAvdConfig)          //= 49,			//AVD 配置
        ,[PARAM_GB28181_PROTO_CFG]= sizeof(JGB28181Proto)       //= 50,			//
        ,[PARAM_NETWORK_DISK_CFG] = sizeof(JNetworkDisk)        //= 51,			//网络磁盘管理
        ,[PARAM_DEV_WORK_STATE]   = sizeof(JDevWorkState)       //= 52,			//设备状态信�?        ,[PARAM_OPERATION_LOG]    = sizeof(JOperationLog)       //= 53,			//获取用户操作记录
        ,[PARAM_ALARM_UPLOAD_CFG] = sizeof(JAlarmUploadCfg)     //= 54,			//报警上传配置
        ,[PARAM_PRESET_POINT_CFG] = sizeof(JPPConfig)           //= 55,			//预置点配�?        ,[PARAM_CRUISE_POINT_CFG] = sizeof(JCruiseConfig)       //= 56,			//巡航路径(查询巡航路径号集合，启动、停止、删除某一巡航�?
        ,[PARAM_CRUISE_WAY_CFG]   = sizeof(JCruiseWaySet)       //= 57,			//巡航路径(查询、添加、修改某一巡航路径信息)
        ,[PARAM_VENC_AUTO_SWITCH] = sizeof(JVencBitAutoSwitch)  //= 58,			//通道码率自动切换设置
        ,[PARAM_3D_CONTROL]       = sizeof(J3DControl)          //= 59,			//3D 控制
        ,[PARAM_SET_DEVICE_TIME]  = sizeof(JDeviceTimeExt)      //= 60,			//手动设置时间 JDeviceTimeExt
        ,[PARAM_SYNC_CMS_TIME]    = sizeof(int)                 //= 61,			//设置同步cms 时间标记
        ,[PARAM_IR_CUT_CFG]       = sizeof(JIrcutCtrl)          //= 62,			//IR-CUT切换模式参数设置�?JIrcutCtrl(海思IPC 私有)
        ,[PARAM_NETWORKFAULTCHECK]= sizeof(JNetWorkFaultCheck)  //= 63,
    ,[PARAM_HERDANALYSE]      = sizeof(JHerdAnalyse)        //= 64,
    ,[PARAM_ALARM_LINK_IO]    = -1                          //= 65,			//联动设备IO 告警
        ,[PARAM_ALARM_LINK_PRESET]= -1                          //= 66,          //联动预置�?        ,[PARAM_RESOLUTION_CFG]   = -1                          //= 67,			//解码器分辨率信息
        ,[PARAM_IRCUT_CONTROL]    = sizeof(JIrcutCtrl)          //= 68,			//红外控制:JIrcutCtrl
        ,[PARAM_3D_GOBACK]        = -1                          //= 69,          //3D 控制返回原样
}; 


long talk_cb(void* data, long size, void* priv)
{
    j_frame_t *frame_info = (j_frame_t *)data;

    printf("frameNum:%lu dn:%d size:%ld\n", frame_info->frame_num, frame_info->frame_num-talk_frame_num, size);
    //printf("%ld, %ld, ts %lu, uts %lu\n", pstFrame->frame_num, pstFrame->size, pstFrame->timestamp_sec, pstFrame->timestamp_usec);
    if (talk_frame_num != 0 && frame_info->frame_num-talk_frame_num != 1) {
        printf("------talk_cb--error=====lost talk data\n");
    }

    talk_frame_num = frame_info->frame_num;
    jcu_talk_send(thdl, frame_info->data, frame_info->size-sizeof(j_frame_t));
    return 0;
}

int mb_search_cb(struct _mb_cu_notify_s *handle, mb_cu_parm_t *parm)
{
    ASSERT(NULL != parm);
    if (parm->error == MB_NOTIFY_ERR_0) {
    
        j_Device_T *dev_info = (j_Device_T *)(parm->data);
        if (dev_info->SysCfg.dev_type < J_Dev_IPNC_)
            printf("device:%-20s type:%s online\n", dev_info->NetworkInfo.network[J_SDK_ETH0].ip
                    , dev_info->SysCfg.dev_type <= J_DEV_IPC_6005? "3516": "3518");
         if (dev_info->SysCfg.dev_type ==J_Dev_IPNC_)
         {
         	printf("device:%-20s type:%s online\n", dev_info->NetworkInfo.network[J_SDK_ETH0].ip
                    ,  "ipnc");
         }
    }
}

int dev_search(void)
{
    static int ds_init = 0;

    if (!ds_init) {
        int ret = jcu_mb_init(GROUP_IP, GROUP_PORT, 6001);
        if (ret != 0) {
            printf("jcu_mb_init call failed\n");
            return -1;
        }
        ds_init = 1;
    }

    mb_cu_notify_t  mb_notify_info = {NULL, mb_search_cb};
    jcu_mb_query_t *query_hdl = jcu_mb_query(J_MB_Device_Id, &mb_notify_info);
    sleep(5); //wait 5 seconds for recving response packet return by any device 
    jcu_mb_release(query_hdl);

    //jcu_mb_uninit();
    return 0;
}

int conf_get_test()
{
    int parm_id;
    char parm_buf[1024];
    jcu_notify_cb_t nc = {JCU_OP_SYNC, NULL, notify_cb};

    jcu_net_cfg_get(uhdl, 0, 0, PARAM_SYNC_TIME, parm_buf, map[PARAM_SYNC_TIME], &nc);

    return 0;
}

int conf_get_set(jcu_user_handle_t *uhdl)
{
    int parm_id;
    char parm_buf[1024];
    jcu_notify_cb_t nc = {JCU_OP_SYNC, NULL, notify_cb};

    for (parm_id = PARAM_SYNC_TIME; parm_id < PARAM_3D_GOBACK/*PARAM_ALARM_LINK_IO*/; ++parm_id) {
        if (map[parm_id] >= 0)
            CALL_CU_API_RETURN_IF_FAILED(jcu_net_cfg_get, uhdl, 0, 0, 
                    parm_id, parm_buf, map[parm_id], &nc);
        usleep(100000);
    }

    JUserModConfig *jmc = (JUserModConfig *)parm_buf;
    strncpy(jmc->old_usrname,  "admin", J_SDK_MAX_NAME_LEN);
    strncpy(jmc->old_password, "admin", J_SDK_MAX_NAME_LEN);
    strncpy(jmc->new_username, "admin", J_SDK_MAX_NAME_LEN);
    strncpy(jmc->new_password, "12345", J_SDK_MAX_NAME_LEN);
    /*jmc->local_right*/
    /*jmc->remote_right*/
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_cfg_set, uhdl, 0, 0, 
            PARAM_MODIFY_USER_CFG, parm_buf, map[PARAM_MODIFY_USER_CFG], &nc);
    memset(parm_buf, 0, 1024);
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_cfg_get, uhdl, 0, 0, 
            PARAM_USER_CONFIG, parm_buf, map[PARAM_USER_CONFIG], &nc);

    printf("@@@@@@@@@@@@@get set finished, reset@@@@@@@@@@@@@@@@@\n");
    memset(parm_buf, 0, 1024);
    strncpy(jmc->old_usrname,  "admin", J_SDK_MAX_NAME_LEN);
    strncpy(jmc->old_password, "12345", J_SDK_MAX_NAME_LEN);
    strncpy(jmc->new_username, "admin", J_SDK_MAX_NAME_LEN);
    strncpy(jmc->new_password, "admin", J_SDK_MAX_NAME_LEN);
    /*jmc->local_right*/
    /*jmc->remote_right*/
    jcu_notify_cb_t set_nc = {JCU_OP_SYNC, NULL, set_notify_cb};
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_cfg_set, uhdl, 0, 0, 
            PARAM_MODIFY_USER_CFG, parm_buf, map[PARAM_MODIFY_USER_CFG], &set_nc);
    memset(parm_buf, 0, 1024);
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_cfg_get, uhdl, 0, 0, 
            PARAM_USER_CONFIG, parm_buf, map[PARAM_USER_CONFIG], &nc);
    return 0;
}

int real_stream(jcu_user_handle_t *uhdl)
{
    jcu_stream_cb_t real_stream_cb_info = {NULL, stream_event_cb};
    jcu_notify_cb_t real_notify_cb_info = {JCU_OP_SYNC, NULL, stream_notify_cb};

    char pathname[256] = "1.h264";
#if 1    
   // get_user_play_parm(pathname, 256);
    vfp = fopen(pathname, "wb"); 
    if (NULL == vfp) {
        printf("create file:%s failed, test failed\n", pathname);
        return -1;
    }
#endif
    jcu_stream_handle_t *shdl = jcu_net_stream_open(uhdl, 0, j_primary_stream, j_rtp_over_tcp, 
            &real_stream_cb_info, &real_notify_cb_info);
    if (NULL != shdl) {
        sleep(10);
#if 1        
        fclose(vfp);
        vfp = NULL;
 #endif       
        char url_buf[256];
        printf("stream url:%s test finished\n", jcu_net_stream_get_url(shdl, url_buf));
    } else {
        printf("jcu_net_stream_open failed\n");
    }

    CALL_CU_API_RETURN_IF_FAILED(jcu_net_stream_close, shdl);

    return 0;
}

int record_stream(jcu_user_handle_t *uhdl)
{
    jcu_notify_cb_t rec_query_notify_cb_info = {JCU_OP_SYNC, NULL, record_query_notify_cb};
    jcu_notify_cb_t rec_stream_notify_cb_info = {JCU_OP_SYNC, NULL, record_stream_notify_cb};
    jcu_stream_cb_t rec_stream_event_cb_info = {NULL, stream_event_cb};

    JStoreLog sl;
    sl.rec_type = ALL_RECODE;
    sl.beg_node = 1;
    sl.end_node = 20;
    sl.sess_id  = 0;
    time_convert("20101101000000", &sl.beg_time);
    time_convert("20131201230000", &sl.end_time);

    CALL_CU_API_RETURN_IF_FAILED(jcu_net_rec_query, uhdl, 0, 
            j_primary_stream, &sl, &rec_query_notify_cb_info);

    if (rec_type != -1) {
        char pathname[256];
        get_user_play_parm(pathname, 256);
        vfp = fopen(pathname, "wb"); 
        if (NULL == vfp) {
            printf("create file:%s failed, test failed\n", pathname);
            return -1;
        }

        jcu_rec_handle_t* rhdl = jcu_net_rec_playback(uhdl, 0, j_primary_stream, rec_type, 
                j_rtp_over_tcp, start_time, end_time, &rec_stream_event_cb_info, &rec_stream_notify_cb_info);
        if (NULL != rhdl) {
            sleep(5);
            fclose(vfp);
            vfp = NULL;
            CALL_CU_API_RETURN_IF_FAILED(jcu_net_rec_close, rhdl);
            printf("jcu_net_rec_playback ok!------------------------->call close\n");
        }

        get_user_play_parm(pathname, 256);
        vfp = fopen(pathname, "wb"); 
        if (NULL == vfp) {
            printf("create file:%s failed, test failed\n", pathname);
            return -1;
        }
        rhdl = jcu_net_rec_download(uhdl, 0, j_primary_stream, rec_type, 
                j_rtp_over_tcp, start_time, end_time, &rec_stream_event_cb_info, &rec_stream_notify_cb_info); 
        if (NULL != rhdl) {
            sleep(5);
            fclose(vfp);
            vfp = NULL;
            CALL_CU_API_RETURN_IF_FAILED(jcu_net_rec_close, rhdl);
            printf("jcu_net_rec_download ok!------------------------->call close\n");
        }
    }
    return 0;
}

int talk(jcu_user_handle_t *uhdl)
{
    thdl = jcu_talk_open(uhdl, NULL, talk_cb, NULL);
    if (NULL != thdl) {
        sleep(5);
        CALL_CU_API_RETURN_IF_FAILED(jcu_talk_close, thdl);
        printf("talk test ok!------------------------->call close\n");
    } else {
        printf("talk test jcu_talk_open failed!-------------------------\n");
    }
    return 0;
}

int ptz(jcu_user_handle_t *uhdl) 
{
    jcu_notify_cb_t ptz_notify_cb_info = {JCU_OP_SYNC, NULL, set_notify_cb};
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_ptz_ctl, uhdl, 0, PTZ_UP, 50, &ptz_notify_cb_info); 
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_ptz_ctl, uhdl, 0, PTZ_DOWN, 50, &ptz_notify_cb_info); 
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_ptz_ctl, uhdl, 0, PTZ_LEFT, 50, &ptz_notify_cb_info); 
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_ptz_ctl, uhdl, 0, PTZ_RIGHT, 50, &ptz_notify_cb_info); 
    return 0;
}

int upgrade(jcu_user_handle_t *uhdl) 
{
    char pathname[256] = {0};
    get_path_name(pathname, 256);

    upgrade_flag = 0;
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_upg, uhdl, pathname);
    while(!upgrade_flag) usleep(1000);
    upgrade_flag = 0;

    printf("upgrade finished, wait a minutes for device reboot\n");

    return 0;
}

int dev_control(jcu_user_handle_t *uhdl)
{
    jcu_notify_cb_t dev_control_notify_cb_info = {JCU_OP_SYNC, NULL, notify_cb};

    printf("warning! reboot device now! wait a minutes\n");
    CALL_CU_API_RETURN_IF_FAILED(jcu_net_dev_ctl, uhdl, 0, RESTART_DEVICE, 0/*ignore value*/, &dev_control_notify_cb_info);
    return 0;
}

int login(void)
{
    char ip[128] = {0};
    char usern[128] = {0};
    char userp[128] = {0};
    int timeout;
    jcu_event_cb_t ec = {NULL, event_cb};
    jcu_notify_cb_t nc = {JCU_OP_SYNC, NULL, notify_cb};

    get_user_login_parm(ip, &timeout, usern, userp);

    printf("do jcu_net_login, wait a second[timeout setting %ds]\n", timeout);

    uhdl = jcu_net_login( 
            ip, 3321, 
            usern, userp, 
            timeout, &ec, &nc);

    if (login_success_flag && NULL != uhdl) {
        sprintf(priv_save_str, SAVE_STR, ip);                         
        priv = priv_save_str;
        printf("====================>jcu_net_login ok\n");
        return 0;
    } else {
        printf("====================>jcu_net_login failed!\n");
        CALL_CU_API_RETURN_IF_FAILED(jcu_net_logout, uhdl);
        uhdl = NULL;
        return -1;
    }
}


int login_(void)
{
    int timeout = 5;
    char ip[128] = "10.1.50.102";
    char usern[128] = "admin";
    char userp[128] = "admin";

    jcu_event_cb_t ec = {NULL, event_cb};
    jcu_notify_cb_t nc = {JCU_OP_SYNC, NULL, notify_cb};
    

    printf("do jcu_net_login, wait a second[timeout setting %ds]\n", timeout);

    uhdl = jcu_net_login( 
            ip, 3321, 
            usern, userp, 
            timeout, &ec, &nc);

    if (login_success_flag && NULL != uhdl) {
        sprintf(priv_save_str, SAVE_STR, ip);                         
        priv = priv_save_str;
        printf("====================>jcu_net_login ok\n");
        return 0;
    } else {
        printf("====================>jcu_net_login failed!\n");
        CALL_CU_API_RETURN_IF_FAILED(jcu_net_logout, uhdl);
        uhdl = NULL;
        return -1;
    }
}

char *produce_prompt(char *buf, int len, char *priv) 
{
    static char *prompt_str = "\n\nenter a, do device search\n"		    
        "enter b, do conf_get_set_test\n"  
        "enter c, do real_stream_test\n"   
        "enter d, do record_test\n"	    
        "enter e, do talk_test\n"		    
        "enter f, do ptz_test\n"           
        "enter g, do upg_test\n"		    
        "enter h, do dev_crl_test\n"       
        "enter i, do login\n"       
        "enter z, do test all\n"
        "enter n, do exit\n%sYour choice: ";       

    snprintf(buf, len, prompt_str, NULL == priv? "":priv, prompt_str);
    return buf;
}

char get_user_choice(const char *prompt)
{
    printf("%s", prompt);
    fflush(stdout);

    int c = getchar();
    getchar();
    if (c == EOF)
        return 'n';
    return (char)c;
}

char *my_fgets(char *buf, int len)
{
    char _buf[len];

    char *p = fgets(_buf, sizeof(_buf), stdin);
    if (NULL == p)
        return NULL;

    if (_buf[0] == '\n')
        return buf;

    _buf[strlen(_buf)-1] = 0;
    strncpy(buf, _buf, len);

    return buf;
}

//ignore argv check 
void get_path_name(char *pathname, int size)
{
    char tmppath[1024] = {0};
    printf("\nInput upgrade file: ");
    fflush(NULL);
    my_fgets(tmppath, 1024);
    strncpy(pathname, tmppath, size-1);
    pathname[size-1] = 0;
}

int check_ip(char *ip)
{
    struct in_addr addr;
    return inet_pton(AF_INET, ip, &addr)==1;
}

int check_timeout(char *timeout)
{
    while(*timeout && isdigit(*timeout))
        timeout++;

    if (*timeout)
        return 0;
    return 1;
}

int check_pathname(char *pathname, int len)
{
    char _pathname[len];
    strncpy(_pathname, pathname, len);

    char *p = strrchr(_pathname, '/');
    if (NULL != p) {
        *p = 0;
        if (access(_pathname, F_OK) < 0) {
            printf("dir:%s check failed\n", _pathname);
            return 0;
        }
        ++p;
    } else {
        p = _pathname;
    }

    if (access(p, F_OK|W_OK) == 0) {
        char buf[16] = "n";
        printf("file %s exist, override it? Y[y]/N[n]", p+1);
        fflush(stdout);
        my_fgets(buf, sizeof(buf));
        if (buf[0] == 'n' || buf[0] == 'N')
            return 0;
    }
    return 1;
}

void get_user_login_parm(char *ip, /*ip_size*/int *timeout, char *usern, /*user size*/char *userp/*password size*/)
{
    char _ip[128] = "10.1.50.102";
    char _usern[128] = "admin";
    char _userp[128] = "admin";
    char _timeout[16] = "5";

    while(1) {
        printf("Enter device ip[default 192.168.5.205]: ");
        fflush(stdout);
        my_fgets(_ip, sizeof(_ip));
        if (!check_ip(_ip)) {
            printf("IP:%s invalid!\n", _ip);
            strcpy(_ip, "192.168.5.205");
            continue;
        }

        printf("Enter timeout(s)[default 5]: ");
        fflush(stdout);
        my_fgets(_timeout, sizeof(_timeout));
        if (!check_timeout(_timeout)) {
            printf("timeout:%s invalid!\n", _timeout);
            strcpy(_timeout, "5");
            continue;
        }

        printf("Enter username[default admin]: ");
        fflush(stdout);
        my_fgets(_usern, sizeof(_usern));

        printf("Enter password[default admin]: ");
        fflush(stdout);
        my_fgets(_userp, sizeof(_userp));
        break;
    }

    /*    printf("ip:%s username:%s password:%s timeout:%s\n", */
    /*            _ip, _usern, _userp, _timeout);*/

    if (ip)//assume 128 len
        strncpy(ip, _ip, 128);
    if (timeout)
        *timeout = atoi(_timeout);
    if (usern)//assume 128 len
        strncpy(usern, _usern, 128);
    if (userp)//assume 128 len
        strncpy(userp, _userp, 128);
}

void get_user_play_parm(char *pathname, int len)
{
    char _pathname[128] = "./realstream.h264";

    while(1) {
        printf("Enter device pathname[default %s]: ", _pathname);
        fflush(stdout);
        my_fgets(_pathname, sizeof(_pathname));
        if (!check_pathname(_pathname, strlen(_pathname)+1)) {
            printf("pathname:%s invalid!\n", _pathname);
            strcpy(_pathname, "./realstream.h264");
            continue;
        }
        strncpy(pathname, _pathname, len);
        break;
    }
}

#define TEST(func, args...)       do {\
    int ret;\
    printf("\n\n#####################test "#func " start\n");\
    usleep(500000);\
    ret = func(args);\
    if (ret != 0) {\
        printf("[error], test "#func " failed!\n");\
        char ans = get_user_choice("Continue test? Y[y]/N[n]: ");\
        if (ans == 'n' || ans == 'N')\
        exit(-1);\
    }\
    printf("#####################test "#func " finished!\n");\
} while(0)


int main(int argc, char *argv[])
{
    int exit_flag = 0;
    char prompt[4096];

    char round_c[] = "aibcdefghz";
    int  round_pos = 0;
    int round_flag = 0;

    CALL_CU_API_RETURN_IF_FAILED(jcu_net_init);
    printf("====================>jcu_net_init ok\n");
    //login_();
    //usleep(500000);
    //conf_get_test();
    while(!exit_flag) {
        char c;
        if (!round_flag) {
            c = get_user_choice(produce_prompt(prompt, 4096, priv));
        } else {
            c = round_c[round_pos++];
            round_pos %= sizeof(round_c)-1;
        }

        if ((c != 'i' && c != 'a' && c != 'z') && NULL == uhdl) {
            printf("\n======>Please choice 'i' or 'a' first, you should login!\n");
            usleep(500000);
            continue;
        }

        switch(c) {
            case 'a':
                TEST(dev_search);
                break;
            case 'b':
                TEST(conf_get_set, uhdl);
                break;
            case 'c':
                TEST(real_stream, uhdl);
                break;
            case 'd':
                TEST(record_stream, uhdl);
                break;
            case 'e':
                TEST(talk, uhdl);
                break;
            case 'f':
                TEST(ptz, uhdl);
                break;
            case 'g':
                TEST(upgrade, uhdl);
                CALL_CU_API_RETURN_IF_FAILED(jcu_net_logout, uhdl);
                uhdl = NULL;
                priv = NULL;
                break;
            case 'h':
                TEST(dev_control, uhdl);
                break;
            case 'i':
                if (uhdl) {
                    CALL_CU_API_RETURN_IF_FAILED(jcu_net_logout, uhdl);
                    uhdl = NULL;
                }

                TEST(login);
                break;
            case 'z':
                round_flag = !round_flag;
                break;
            case 'n':
                exit_flag = 1;
                break;
            default:
                //fflush(stdin);
                /*                while(!feof(stdin)) getchar();*/
                setbuf(stdin, NULL);
                break;
        }
    }

    CALL_CU_API_RETURN_IF_FAILED(jcu_net_logout, uhdl);

    return 0;
}
