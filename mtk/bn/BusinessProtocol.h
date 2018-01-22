#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include "stdafx.h"
#include "Dapp.h"

#ifdef __cplusplus
extern "C" {
#endif
//int CheckIndata(uchar *data, int chklen);
//void SetRCmdRsponseState(uchar cmd);
//void ClrRCmdRsponseState(uchar cmd);
extern int BusinessDecode(uchar *data, int len);
extern typTLV TLVDecode1(uchar *indata, int *HaveProcessLen);
extern void TLVDecode(typTLV *tlv, int *HaveProcessLen);
extern int TLVEncode(uchar tag, ushort len, const void *sdata, int bConvert, uchar * valdata);
//int RemoteSendTimeCheck(void);
//int RemoteSendWMCLogin(void);
//int RemoteSendFileUpdateResult(ulong updataSessionID,uchar operSuccess);
//int RemoteSendControlOffLine(void);
//typDoorInfo GetWDoorInfo(ulong add);

//int RemoteSendHeart(void);
extern int BusinessPackage(uchar cmd, uchar Status, ulong sequence, const uchar *bodydata, ushort len, const uchar *ReserveData, uchar rhl, const char *SourceAdd, uchar type);
extern int RemoteSendOnlineNotify(int type);
extern int RemoteSendTimeSynchronize(void);
//extern int LockDeviceQuery(ulong *DevAdd, ushort *LogicCode);

void SrvOnline(void);
extern int SMSDecode(uchar *data, int len);
extern int RemoteSendImsiChangedNotify(int64_t Imsi);
//extern void ClearDeviceQuery(void);
//extern void ClearRecordSet(void);
//extern void ClearRecordReSet(void);
//extern int UpdateNormalAlertRecord(ushort LogicCode, uchar alerttype, uint32_t *time_stamp);
//int UploadAlertRecord(ulong DevAdd);
/*
服务器对门锁做状态查询时根据门锁返回状态将数据打包上传给服务器

seq - 门锁状态数据的业务层流水号
	seq < 0x80000000 - UploadStatus，此时会根据StatChgMask上传相关信息
	seq >= 0x80000000 - QueryStatusResponse

*/
int LcSendStatus(int DevCode, uint32_t Seq, uint16_t StatChgMask);

//extern int PicsRecordStore ( uchar *picbuf, int len, ushort LogicCode );
extern int head_of_manage_busi_msg_fill( typBUSINESS_LAYER_HEAD *head, uchar *data, int len );
//extern int F_check_ctrl_upgrade_file_whether_copy(void);
extern typSystermInfo g_SystermInfo;
//extern ulong g_LastTime;
//extern ulong g_LastWMCStatusTime;

#ifdef __cplusplus
}
#endif
#endif

