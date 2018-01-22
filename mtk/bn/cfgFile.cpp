#include <string.h>
#include "iniFile.h"
#include "cfgFile.h"

static const char *CfgFlPath[CFG_FTYPE_NUM] = {
	"/home/acs09.ini",
	"/home/syscfg.ini",
};

static CIniFile CfgFls[CFG_FTYPE_NUM];

//返回值请注意清理，缓冲，不安全
char* GetCacheCfgStr(int Type, const char *KeyName, const char *ValName, const char * DefStr)
{
	if (Type<0 || Type>=CFG_FTYPE_NUM || KeyName==NULL || ValName==NULL || DefStr==NULL) {
		return NULL;
	}

	if (CfgFls[Type].Path() == "") {
		CfgFls[Type].SetPath(CfgFlPath[Type]);
		CfgFls[Type].CaseSensitive();
		CfgFls[Type].ReadFile();
	}

	string ValStr = CfgFls[Type].GetValue(string(KeyName), string(ValName), string(DefStr));

	return strdup(ValStr.c_str());
}

//返回值请注意清理
char * GetPathCfgStr(const char *Path, const char *KeyName, const char *ValName, const char * DefStr)
{
	if (Path==NULL || KeyName==NULL || ValName==NULL || DefStr==NULL) {
		return NULL;
	}
	CIniFile tFl(Path);

	tFl.ReadFile();
	string ValStr = tFl.GetValue(string(KeyName), string(ValName), string(DefStr));

	return strdup(ValStr.c_str());
}
//返回值请注意清理
char * GetCfgStr(int Type, const char *KeyName, const char *ValName, const char * DefStr)
{
	if (Type<0 || Type>=CFG_FTYPE_NUM) {
		return NULL;
	}

	return GetPathCfgStr(CfgFlPath[Type], KeyName, ValName, DefStr);
}

int SetCfgStr(int Type, const char *KeyName, const char *ValName, const char *ValStr)
{
	if (Type<0 || Type>=CFG_FTYPE_NUM || KeyName==NULL || KeyName[0]=='\0' || ValName==NULL || ValName[0]=='\0' || ValStr==NULL) {
		return -1;
	}

	CIniFile tFl(CfgFlPath[Type]);

	if (!tFl.ReadFile()) {
		return -1;
	}
	if (!tFl.SetValue(string(KeyName), string(ValName), string(ValStr), true)) {
		return -1;
	}
	if (!tFl.WriteFile()) {
		return -1;
	}

	return 0;
}
