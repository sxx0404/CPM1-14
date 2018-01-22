/*
 * Clinet Network SDK Interface.
 * Copyright (c) Shenzhen JXJ Electronic Co. Ltd, 2012
 */

#ifndef __jcu_net_api__
#define __jcu_net_api__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#ifdef EXPORT_API
#define JCUSTREAM_API __declspec(dllexport)
#else
#define JCUSTREAM_API __declspec(dllimport)
#endif
#else

#define JCUSTREAM_API
#define __stdcall

#endif
#include "jcu_net_types.h"
/*
 * init && deinit
 */
JCUSTREAM_API int jcu_net_init(void);
JCUSTREAM_API int jcu_net_uninit(void);

JCUSTREAM_API int jcu_net_init_ex(char *ip, int port, jcu_event_cb_t cb);
JCUSTREAM_API int jcu_net_uninit_ex(void);
/*
 * login && logout
 */
JCUSTREAM_API jcu_user_handle_t*
    jcu_net_login(char *ip                          //device ip;
            , unsigned short port                   //device cmd port;
            , char *user                            //device user name;
            , char *pass                            //device password;
            , int timeout                           //user_handle operate timeout( s);
            , jcu_event_cb_t *event_cb              //event process;
            , jcu_notify_cb_t *complete_cb);

JCUSTREAM_API int jcu_net_logout(jcu_user_handle_t *handle);


JCUSTREAM_API int jcu_get_req_ttl(jcu_user_handle_t *handle); // in seconds 


JCUSTREAM_API int jcu_set_req_ttl(jcu_user_handle_t *handle, int ttl); // in seconds

/*
 * stream open && close
 */
JCUSTREAM_API jcu_stream_handle_t*
    jcu_net_stream_open(jcu_user_handle_t *handle   //user connect handle;
            , int ch_no                             //channel number(0 - 255);
            , int st_type                           //stream type;
            , int protocol                          //transform protocol;
            , jcu_stream_cb_t *stream_cb            //stream av process callback;
            , jcu_notify_cb_t *complete_cb);

JCUSTREAM_API int 
	jcu_net_stream_close(jcu_stream_handle_t *handle);


JCUSTREAM_API char *
	jcu_net_stream_get_url(jcu_stream_handle_t *handle, char *url);


/*
 * config set && get
 */
//注意：该函数和以前的同名函数的操作方法有了改变，设置的成功与否不再是返回值决定，
//而是返回值和回调里面的参数返回值决定
JCUSTREAM_API int 
	jcu_net_cfg_set(jcu_user_handle_t *handle
        , int  ch_no                                //channel number(0 - 255);
        , int  st_type        
        , int  parm_id                              //JCU_PARM_ID_E from j_sdk.h;
        , char *parm_buf                            //parm buffer;
        , int  parm_size                            //parm buffer size;
        , jcu_notify_cb_t *complete_cb);

/*
 * complete_cb return result;
 */
//注意：如果是同步模式，在回调里面不要有操作界面控件的动作，否则
//会导致界面卡死现象   该bug已修复20130619，不再会导致卡死
JCUSTREAM_API int 
	jcu_net_cfg_get(jcu_user_handle_t *handle
        , int  ch_no
        , int  st_type
        , int  parm_id
		, char *parm_buf
		, int  parm_size
        , jcu_notify_cb_t *complete_cb);
	
	
/*
 * ptz control
 */
//注意：该函数和以前的同名函数的操作方法有了改变，设置的成功与否不再是返回值决定，
//而是返回值和回调里面的参数返回值决定
JCUSTREAM_API int 
	jcu_net_ptz_ctl(jcu_user_handle_t *handle
        , int ch_no                                 //channel number(0 - 255);
        , int ptz_cmd                               //JCU_PTZ_CTL_ID_E from j_sdk.h;
        , int ptz_value                             //ptz value;
        , jcu_notify_cb_t *complete_cb);
/*
 * device control
 */
JCUSTREAM_API int 
	jcu_net_dev_ctl(jcu_user_handle_t *handle
        , int ch_no                                 //channel number(0 - 255);
        , int ctl_cmd                               //JCU_DEV_CTL_ID_E from j_sdk.h;
        , int ctl_value                             //control value;
        , jcu_notify_cb_t *complete_cb);



///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////录像相关模块/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//录像查询
JCUSTREAM_API int 
	jcu_net_rec_query(jcu_user_handle_t *handle 
		, int ch_no 
		, int st_type 
		, JStoreLog *pstSorPkg
		, jcu_notify_cb_t *complete_cb
		);

//录像回放
JCUSTREAM_API jcu_rec_handle_t* 
	jcu_net_rec_playback(jcu_user_handle_t *handle
		, int ch_no
		, int st_type
		, int rec_type
		, int protocol
		, char *begin_time
		, char *end_time
        , jcu_stream_cb_t *stream_cb
		, jcu_notify_cb_t *complete_cb
        );

//录像下载
JCUSTREAM_API jcu_rec_handle_t* 
	jcu_net_rec_download(jcu_user_handle_t *handle
		, int ch_no
		, int st_type
		, int rec_type
		, int protocol
		, char *begin_time
		, char *end_time
		, jcu_stream_cb_t *stream_cb
		, jcu_notify_cb_t *complete_cb
        );

//录像关闭
JCUSTREAM_API int 
	jcu_net_rec_close(jcu_rec_handle_t *handle);






///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////语言对讲模块/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
JCUSTREAM_API jcu_talk_cli_t* 
	jcu_talk_open(  jcu_user_handle_t *handle
			  , const PST_TALK_SAMPLE_PARAM pstTalkFmt  //pstTalkFmt 暂时置为NULL即可
			  , FUNC_VOICE_STREAM_CB fun
			  , void* pParam);

JCUSTREAM_API int 
	jcu_talk_close(jcu_talk_cli_t *handle);

JCUSTREAM_API int 
	jcu_talk_send(jcu_talk_cli_t *handle, void* pVoiceData, long lSize);


///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////设备升级模块/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//设备升级
JCUSTREAM_API int 
	jcu_net_upg(jcu_user_handle_t *hdl, char *filename);



///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////设备搜索、配置模块/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
/*
 *  cu端的初始化和反初始化
 *  addr填pu初始化的地址
 *  port 填pu初始化的端口
 *  local_port 填写cu 自己设定的监听端口
 */
JCUSTREAM_API int 
	jcu_mb_init(char *addr, unsigned int port, unsigned int local_port);
JCUSTREAM_API int 
	jcu_mb_uninit(void);

/*  jcu_mb_query
  *  相当于给所有的局域网内的pu 设备发送mb_cfg_get, 以此来获取
  *  所有设备的某一类信息, 和所有pu 设备的dst_id ，
  *  mb_query 和mb_release 需成对使用
  */
JCUSTREAM_API jcu_mb_query_t* 
	jcu_mb_query(int msg_id, mb_cu_notify_t *item_cb);

JCUSTREAM_API int 
	jcu_mb_release(jcu_mb_query_t *hdl);

/*jcu_mb_cfg_get
 * 针对一台pu获取参数
 * msg_id:消息id;
 * dst_id:目标pu 地址, 可以用mb_query 一次, 得到局域网内所有机器的地址
 * timeout:超时时间
 * complete_cb:完成回调
 */
JCUSTREAM_API int 
	jcu_mb_cfg_get(int msg_id
        , char *dst_id
        , int  timeout
        , mb_cu_notify_t *complete_cb
        , int is_sync);

/* jcu_mb_cfg_set
 * 针对一台pu设置参数
 * msg_id:消息id;
 * dst_id:目标pu 地址, 可以用mb_query 一次, 得到局域网内所有机器的地址
 * user: 用户名
 * pass: 密码
 * timeout:超时时间
 * struct_size: 结构体大小
 * struct_buf: 要拷贝的结构体位置
 * complete_cb:完成回调
 */
JCUSTREAM_API int 
	jcu_mb_cfg_set(int msg_id
        , char *dst_id
        , char *user
        , char *pass
        , int  timeout
        , int struct_size
        , void *struct_buf
        , mb_cu_notify_t *complete_cb
        , int is_sync);

//===================================================================================================
//===================================================================================================
//以下接口不建议使用，请使用相对应的新接口
//===================================================================================================

/*
//jcu_net_rec_query： 该接口废弃,不再向外提供
int jcu_net_rec_query(
  jcu_user_handle_t	*handle,
  int			ch_no,
  int			st_type,
  char			*begin_time,
  char			*end_time,
  int			rec_type,
  jcu_notify_cb_t		*complete_cb
)

//jcu_stream_handle_t： 该接口废弃,不再向外提供
jcu_stream_handle_t*  jcu_net_stream_open(
  jcu_user_handle_t	*handle,
  int			ch_no,
  int			st_type,
  int			mix_type,
  int			protocol,
  jcu_stream_cb_t		*stream_cb,
  jcu_notify_cb_t		*complete_cb
)

*/   

//jcu_net_upg_open： 该接口即将废弃，请使用新的接口，
//新的替代接口请参考：jcu_net_upg
JCUSTREAM_API int jcu_net_upg_open(jcu_user_handle_t	*handle
	  , char				*buf
	  , int				size
	  , jcu_notify_cb_t	*complete_cb
	);

//jcu_net_upg_close： 该接口即将废弃，请使用新的接口，
//新的替代接口请参考：jcu_net_upg
JCUSTREAM_API int jcu_net_upg_close(
		jcu_user_handle_t	*handle
	);

//jcu_net_cfg_get2：该接口即将废弃，请使用新的接口，
//新的替代接口请参考：jcu_net_cfg_get
JCUSTREAM_API int jcu_net_cfg_get2(
	  jcu_user_handle_t	*handle,
	  int			ch_no,
	  int			parm_id,
	  char			*parm_buf,
	  int			parm_size,
	  jcu_notify_cb_t		*complete_cb
	);

//jcu_net_rec_open4time：该接口即将废弃，请使用新的接口，参考：
//jcu_net_rec_playback   jcu_net_rec_download
JCUSTREAM_API jcu_user_handle_t*  jcu_net_rec_open4time(
	  jcu_user_handle_t	*handle,
	  int			ch_no,
	  int			st_type,
	  char			*begin_time,
	  char			*end_time,
	  int			rec_type,
	  int			is_play,
	  jcu_stream_cb_t		*stream_cb,
	  jcu_notify_cb_t		*complete_cb
	);

//OpenTalk：该接口即将废弃，请使用新的接口，
//新的替代接口请参考：jcu_talk_open
JCUSTREAM_API HVOICE __stdcall OpenTalk(
	  void*							pDevHandle,
	  const PST_TALK_SAMPLE_PARAM	pTalkFmt,
	  FUNC_VOICE_STREAM_CB			fun,
	  void*							pParam,
	  DWORD							dwPort
	);

//CloseTalk：该接口即将废弃，请使用新的接口，
//新的替代接口请参考：jcu_talk_close
JCUSTREAM_API void __stdcall CloseTalk(HVOICE hVoice);

//SendTalkData：该接口即将废弃，请使用新的接口，
//新的替代接口请参考：jcu_talk_send
JCUSTREAM_API long __stdcall SendTalkData(
	  HVOICE		hVoice,
	  void*			pVoiceData,
	  void*			lSize,
	  DWORD			dwUser
	);

#ifdef __cplusplus
}
#endif
#endif //__jcu_net_api__
