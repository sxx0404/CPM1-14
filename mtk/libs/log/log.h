#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum {
    LOG_LV_VERBOSE = 0,
    LOG_LV_DEBUG,
    LOG_LV_INFO,
    LOG_LV_WARN,
    LOG_LV_ERROR,
    LOG_LV_NUM,
};

extern int Log(const char *Tag, int LogLv, const char *fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LOG_H_INCLUDED
