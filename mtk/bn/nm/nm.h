#ifndef NM_H_INCLUDED
#define NM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 本线程中负责网络连通性检测和管理，同时负责3G模块的短信、IMSI以及信号的读取
void* NetManageThread(void *);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NM_H_INCLUDED
