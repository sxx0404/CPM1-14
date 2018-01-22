#ifndef FRSDB_H_INCLUDED
#define FRSDB_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 初始化，成功后返回0，否则-1
int FrDbInit(void);
// 将数据插入，后边可以通过Get拿出来，Push的数据优先于init加载的
int FrDbPush(const uint8_t *Data, int DtLen);
// 数据包的获取规则是：先找从未上传过的，然后再找超时的，To为超时时间，此时间之前，pSeq为希望获得的包的Seq，如果不能满足会自动修改为无重复的，获得成功后ppData指向该数据，记得释放
// 数据包中Seq并没有修改，请自行修改
// 返回-1没有缓冲的包或出错，返回0，没有满足条件的包，返回正数，为返回的数据长度
int FrDbGet(double To, uint32_t *pSeq, uint8_t **ppData);
int FrDbDel(uint32_t Seq);
void FrDbPrint(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FRSDB_H_INCLUDED
