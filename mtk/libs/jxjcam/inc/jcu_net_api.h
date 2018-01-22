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
//ע�⣺�ú�������ǰ��ͬ�������Ĳ����������˸ı䣬���õĳɹ�������Ƿ���ֵ������
//���Ƿ���ֵ�ͻص�����Ĳ�������ֵ����
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
//ע�⣺�����ͬ��ģʽ���ڻص����治Ҫ�в�������ؼ��Ķ���������
//�ᵼ�½��濨������   ��bug���޸�20130619�����ٻᵼ�¿���
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
//ע�⣺�ú�������ǰ��ͬ�������Ĳ����������˸ı䣬���õĳɹ�������Ƿ���ֵ������
//���Ƿ���ֵ�ͻص�����Ĳ�������ֵ����
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
//////////////////////////////////////¼�����ģ��/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//¼���ѯ
JCUSTREAM_API int 
	jcu_net_rec_query(jcu_user_handle_t *handle 
		, int ch_no 
		, int st_type 
		, JStoreLog *pstSorPkg
		, jcu_notify_cb_t *complete_cb
		);

//¼��ط�
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

//¼������
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

//¼��ر�
JCUSTREAM_API int 
	jcu_net_rec_close(jcu_rec_handle_t *handle);






///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////���ԶԽ�ģ��/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
JCUSTREAM_API jcu_talk_cli_t* 
	jcu_talk_open(  jcu_user_handle_t *handle
			  , const PST_TALK_SAMPLE_PARAM pstTalkFmt  //pstTalkFmt ��ʱ��ΪNULL����
			  , FUNC_VOICE_STREAM_CB fun
			  , void* pParam);

JCUSTREAM_API int 
	jcu_talk_close(jcu_talk_cli_t *handle);

JCUSTREAM_API int 
	jcu_talk_send(jcu_talk_cli_t *handle, void* pVoiceData, long lSize);


///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////�豸����ģ��/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//�豸����
JCUSTREAM_API int 
	jcu_net_upg(jcu_user_handle_t *hdl, char *filename);



///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////�豸����������ģ��/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
/*
 *  cu�˵ĳ�ʼ���ͷ���ʼ��
 *  addr��pu��ʼ���ĵ�ַ
 *  port ��pu��ʼ���Ķ˿�
 *  local_port ��дcu �Լ��趨�ļ����˿�
 */
JCUSTREAM_API int 
	jcu_mb_init(char *addr, unsigned int port, unsigned int local_port);
JCUSTREAM_API int 
	jcu_mb_uninit(void);

/*  jcu_mb_query
  *  �൱�ڸ����еľ������ڵ�pu �豸����mb_cfg_get, �Դ�����ȡ
  *  �����豸��ĳһ����Ϣ, ������pu �豸��dst_id ��
  *  mb_query ��mb_release ��ɶ�ʹ��
  */
JCUSTREAM_API jcu_mb_query_t* 
	jcu_mb_query(int msg_id, mb_cu_notify_t *item_cb);

JCUSTREAM_API int 
	jcu_mb_release(jcu_mb_query_t *hdl);

/*jcu_mb_cfg_get
 * ���һ̨pu��ȡ����
 * msg_id:��Ϣid;
 * dst_id:Ŀ��pu ��ַ, ������mb_query һ��, �õ������������л����ĵ�ַ
 * timeout:��ʱʱ��
 * complete_cb:��ɻص�
 */
JCUSTREAM_API int 
	jcu_mb_cfg_get(int msg_id
        , char *dst_id
        , int  timeout
        , mb_cu_notify_t *complete_cb
        , int is_sync);

/* jcu_mb_cfg_set
 * ���һ̨pu���ò���
 * msg_id:��Ϣid;
 * dst_id:Ŀ��pu ��ַ, ������mb_query һ��, �õ������������л����ĵ�ַ
 * user: �û���
 * pass: ����
 * timeout:��ʱʱ��
 * struct_size: �ṹ���С
 * struct_buf: Ҫ�����Ľṹ��λ��
 * complete_cb:��ɻص�
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
//���½ӿڲ�����ʹ�ã���ʹ�����Ӧ���½ӿ�
//===================================================================================================

/*
//jcu_net_rec_query�� �ýӿڷ���,���������ṩ
int jcu_net_rec_query(
  jcu_user_handle_t	*handle,
  int			ch_no,
  int			st_type,
  char			*begin_time,
  char			*end_time,
  int			rec_type,
  jcu_notify_cb_t		*complete_cb
)

//jcu_stream_handle_t�� �ýӿڷ���,���������ṩ
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

//jcu_net_upg_open�� �ýӿڼ�����������ʹ���µĽӿڣ�
//�µ�����ӿ���ο���jcu_net_upg
JCUSTREAM_API int jcu_net_upg_open(jcu_user_handle_t	*handle
	  , char				*buf
	  , int				size
	  , jcu_notify_cb_t	*complete_cb
	);

//jcu_net_upg_close�� �ýӿڼ�����������ʹ���µĽӿڣ�
//�µ�����ӿ���ο���jcu_net_upg
JCUSTREAM_API int jcu_net_upg_close(
		jcu_user_handle_t	*handle
	);

//jcu_net_cfg_get2���ýӿڼ�����������ʹ���µĽӿڣ�
//�µ�����ӿ���ο���jcu_net_cfg_get
JCUSTREAM_API int jcu_net_cfg_get2(
	  jcu_user_handle_t	*handle,
	  int			ch_no,
	  int			parm_id,
	  char			*parm_buf,
	  int			parm_size,
	  jcu_notify_cb_t		*complete_cb
	);

//jcu_net_rec_open4time���ýӿڼ�����������ʹ���µĽӿڣ��ο���
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

//OpenTalk���ýӿڼ�����������ʹ���µĽӿڣ�
//�µ�����ӿ���ο���jcu_talk_open
JCUSTREAM_API HVOICE __stdcall OpenTalk(
	  void*							pDevHandle,
	  const PST_TALK_SAMPLE_PARAM	pTalkFmt,
	  FUNC_VOICE_STREAM_CB			fun,
	  void*							pParam,
	  DWORD							dwPort
	);

//CloseTalk���ýӿڼ�����������ʹ���µĽӿڣ�
//�µ�����ӿ���ο���jcu_talk_close
JCUSTREAM_API void __stdcall CloseTalk(HVOICE hVoice);

//SendTalkData���ýӿڼ�����������ʹ���µĽӿڣ�
//�µ�����ӿ���ο���jcu_talk_send
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
