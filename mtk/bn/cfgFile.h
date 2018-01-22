#ifndef CFGFILE_H_INCLUDED
#define CFGFILE_H_INCLUDED

enum {
	CFG_FTYPE_CFG=0,
	CFG_FTYPE_SYS,
	CFG_FTYPE_NUM,
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


//返回值请注意清理
char * GetCfgStr(int Type, const char *KeyName, const char *ValName, const char * DefStr);
//返回值请注意清理
char * GetPathCfgStr(const char *Path, const char *KeyName, const char *ValName, const char * DefStr);
int SetCfgStr(int Type, const char *KeyName, const char *ValName, const char *ValStr);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CFGFILE_H_INCLUDED
