#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "stdafx.h"
#include "md5.h"
#include "BusinessProtocol.h"
#include "ProtocolDefine.h"
#include "list.h"
#include "NormalTool.h"
#include "TCPCommApp.h"
#include "DES.h"
#include "mgt/mgt.h"
#include "LanMiddle.h"
#include "main_linuxapp.h"
#include "ManageTransmit.h"
#include "ManageSecurity.h"
#include "../include/common.h"
#include "../include/log.h"
#include "lc/lc.h"
#include "nm/nm.h"
#include "frsdb/frsdb.h"
#include "up/up.h"

#define INFRAED_ALERT_INTAL_TIME                  18000 //UNIT:0.1S

int g_BusinessDataDiagnose = 0;

/* Global variables ***********************************************************/
/*
 *  软件版本号
 *  // 奇数:V2.1, 偶数:V2.2
 *  // V2.5, Big Endien //2.6, 13-04-03	// 2.7, 13-04-17 // 2.9, 13-05-10 // 2.13 13-08-01
 */
uchar g_WMCVersion[2] = {SOFT_MINOR_VERSION, SOFT_MAJOR_VERSION};
/* Global variables ***********************************************************/
/*
 *  硬件版本号
 */
uchar g_WMCFirmware[2] = {0, 68};	// 0x1002, Big Endien. The hardware id of pcb v2.1 is "10.1"

UCHAR g_bResetWMC = 0;
int g_bDoNewConnectTest = 0;

/*for some wmc submit send data*/
typDeviceSetup g_DeviceSetup;
typTransmitKey g_TransmitKey;
typDeviceParameter g_tempServerInfo;
typUpgradeNotify g_UpgradeNotify[2];
typSystermInfo g_SystermInfo = {0};
/* Global Variables end *******************************************************/

/* Local Variables **********************************************************/
//static UCHAR sData[100] = {};
//static UCHAR headData[4] = {};
static int g_flagdownloading = FLAG_UPGRADE_INIT;
//static uint8_t g_Voltage;	// 主控器备用电池
//static int slen=1, num=0;
// 各升级包的存储起始地址
//const static ulong g_uptempadd[2] = {FLASH_UPGRADE_ADD, PWLMS_UPDATE_START_ADD};
// 各升级包的存储结束地址
//const static ulong g_uptempadd_end[2] = {FLASH_BOOT_PARA_ADD, PWLMS_UPDATE_END_ADD};
//ulong PackSequence = 0;

/* private function ***********************************************************/
static int RemoteSendDownloadFile(const char *FileName, const ulong offset, const ulong size);
//static void ClearRecordModeSet(int mode);
static int LcMakeStatusInfo(int DevCode, uint16_t StatMask, uint8_t *Buf);
static int F_check_ctrl_upgrade_file_whether_copy(void);

//static void LockLinkLedModeSet(int mode);

/*找到,返回i.没找到,返回0xFF*/
//static int GetInfoByDeviceCode(ushort DeveiceCode)
//{
//	int i;
//
//	for (i = 0; i < MAX_SUBDEVICE/*g_DeviceSetup.count*/; i++) {
//		if (g_DeviceSetup.Device[i].Status && g_DeviceSetup.Device[i].LogicCode == DeveiceCode) {
//			return i;
//		}
//	}
//
//	return (0xff);
//}

static int QueryDevieceStatusLock(uchar *data, uint32_t DeviceCode)
{
	int ltp = 0;

	ltp = LcMakeStatusInfo(DeviceCode, ~0, data+2);
	if (ltp > 0) {
		data[0] = TAG_D_statusInfo;
		data[1] = ltp;
		return ltp+2;
	} else {
		return 0;
	}
}

static int QueryDevieceStatusWMC(uchar *data, uchar DeveiceCode)
{
	int ltp = 0;
	unsigned long t = time(NULL);
	uint32_t tDCode = DeveiceCode;

	ltp = 2;
	ltp += TLVEncode(TAG_D_code, DevCodeSize, (uchar *)&tDCode, 1, &data[ltp]);
	ltp += TLVEncode(TAG_M_time, 4,(uchar *)&t, 1, &data[ltp]);
	t = 0;
	ltp += TLVEncode(TAG_D_workStatus, 1,(uchar *)&t, 0, &data[ltp]);
	ltp += TLVEncode(TAG_M_version, 2, (uchar *)g_WMCVersion, 0, &data[ltp]);
	ltp += TLVEncode(TAG_D_firmware, 2, (uchar *)g_WMCFirmware, 0, &data[ltp]);

	// TODO这样不安全吧？
	TLVEncode(TAG_D_statusInfo, ltp-2, &data[2], 0, &data[0]);     //big tag
	return (ltp);
}

/*
static int QueryDevieceStatusOther(uchar *data, uchar DeveiceCode)
{
	int ltp = 0;
	int pos = 0xff;

	ltp = 2;
	ltp += TLVEncode(TAG_D_code, DCODE_LEN, (uchar *)&DeveiceCode, 1, &data[ltp]);

	pos = GetInfoByDeviceCode(DeveiceCode);
	if (pos >= 0xff) {
		return 0;
	}
	ltp += TLVEncode(TAG_D_address, strlen(g_DeviceSetup.Device[pos].address), (uchar *)g_DeviceSetup.Device[pos].address, 0, &data[ltp]);

	// TODO这样不安全吧？
	TLVEncode(TAG_D_statusInfo, ltp-2, &data[2], 0, &data[0]);     //big tag
	return (ltp);
}
*/
/*
 * D_code: 0 - WMC, 100 - lock
 * type: 0-add, 1-delete, 3-clear
 */
//TODO 本函数中有很多可以精简优化的
static int ModifyDeviceList(uint32_t D_code, typTLV *ptlv, int *pLen, ushort taglen, int type, int *pos)
{
	int i, num = -1, HaveProcessLen3;
	int flag = 0;
	int bIPChanged = 0, bPortChanged = 0;
	typTLV tlv3;

	if (type == 3) {
		for (i=0; i<MAX_SUBDEVICE; i++) {
			g_DeviceSetup.Device[i].Status = 0;
		}
		g_DeviceSetup.count = 0;
		LcSetDevice(3, 0, 0);
	} else if (0 == D_code) {	// 配置主控器参数
		if (0 != type) {
			return -1;
		}

		while (*pLen < taglen) {
			TLVDecode(ptlv, pLen);

			if (ptlv->tag == TAG_D_parameter) {
				if (g_BusinessDataDiagnose) {
					echo_printf(", D_parameter");
				}
				tlv3.NextTag = ptlv->vp;
				HaveProcessLen3 = 0;
				while (HaveProcessLen3 < ptlv->vlen) {	//the second layer tag
					TLVDecode(&tlv3, &HaveProcessLen3);
					sp_printf("HaveProcessLen3=%d, vlen=%d\r\n", HaveProcessLen3, ptlv->vlen);
					if (tlv3.tag == TAG_D_matchCode) {  // 4
						sp_printf("ModifyDeviceList_MatchCode=%d\r\n", *((ulong*)tlv3.vp));
						//ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.MatchCode), tlv3.vp, 4);

						if (g_BusinessDataDiagnose) {
							echo_printf(", D_matchCode:%ld", *((ulong*)tlv3.vp));
						}
					} else if (tlv3.tag == TAG_D_refreshInterval) { // 4
						if (g_BusinessDataDiagnose) {
							echo_printf(", D_refreshInterval:%ld", *((ulong*)tlv3.vp));
						}
					} else if (tlv3.tag == TAG_M_serverIP) { // 4
						ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerIP), tlv3.vp, 4);
						if (g_BootPara.NetInfo.ip != g_tempServerInfo.ServerIP) {
							bIPChanged = 1;
						}

						if (g_BusinessDataDiagnose) {
							echo_printf(", M_serverIP:%ld.%ld.%ld.%ld (0x%08lX)",
										(g_tempServerInfo.ServerIP>>24)&0xff, (g_tempServerInfo.ServerIP>>16)&0xff,
										(g_tempServerInfo.ServerIP>>8)&0xff, g_tempServerInfo.ServerIP&0xff,
										g_tempServerInfo.ServerIP);
						} else {
							sp_printf("in server ip is:0x%08lX\r\n", g_tempServerInfo.ServerIP);
						}

						/*
						ConvertMemcpy((uchar *)&(g_BootPara.NetInfo.ip), tlv3.vp, 4);
						//ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.ServerIP), tlv3.vp, 4);
						*/
					} else if (tlv3.tag == TAG_M_serverPort) { // 2
						ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerPort), tlv3.vp, 2);
						if (g_BootPara.NetInfo.port  != g_tempServerInfo.ServerPort) {
							bPortChanged = 1;
						}

						if (g_BusinessDataDiagnose) {
							echo_printf(", M_serverPort:%d", g_tempServerInfo.ServerPort);
						} else {
							sp_printf("in server port is:%d\r\n", g_tempServerInfo.ServerPort);
						}

						/*
						ConvertMemcpy((uchar *)&(g_BootPara.NetInfo.port), tlv3.vp, 2);
						//ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.ServerPort), tlv3.vp, 2);
						*/
					} else if (tlv3.tag == TAG_D_alertMode) {	// 告警模式（主控器现场告警） 2
						ushort alertMode;
						ConvertMemcpy((uchar *)&alertMode, tlv3.vp, 2);

						if (g_BusinessDataDiagnose) {
							echo_printf(", D_alertMode:%x", alertMode);
						}

						switch ((alertMode & 0xE) >> 1) {// Bit1-Bit3：持续报警时长
						case 0x0:	// 000：默认（厂商自定义）
						case 0x1:	// 001：30秒
						case 0x2:	// 010：1分钟
						case 0x3:	// 011：3分钟
						case 0x4:	// 100：1小时
						case 0x5:	// 101：12小时
						case 0x6:	// 110：3天
						case 0x7:	// 111：永久
						default:
							break;
						}

						switch ((alertMode & 0xF0) >> 4) {// Bit4-Bit7：告警方式（目前只用0，默认方式，间歇峰鸣，厂商可以扩展定义）
						case 0x0:
						default:
							break;
						}

						switch ((alertMode & 0xFF00) >> 8) {// 高字节代表在什么场景下告警
						case 0x1:	// Bit8：强行开门
						case 0x2:	// Bit9：门未关好
						case 0x4:	// Bit10：门锁防拆
						default:
							break;
						}
					}
				}
			}
		}

		if (bIPChanged | bPortChanged) { // 设置IP和端口, 则先测试目标IP和端口
			g_bDoNewConnectTest = 1;
		}
		//StoreBootPara();

		return 0;
	} else { // 对锁或其他子设备进行设置
		/* 遍历配置门锁及其他子设备参数 */
		for (i=0; i<MAX_SUBDEVICE; i++) {
			//		sp_printf("i=%2d, LogicCode=%d, D_code=%d\n", i, g_DeviceSetup.Device[i].LogicCode, D_code);
			if (g_DeviceSetup.Device[i].LogicCode == D_code) {
				sp_printf("\n\rgot it!\r\n");
				if (1 == type) {	// delete this device
					if (g_DeviceSetup.Device[i].Status) { // 门锁联机
						g_DeviceSetup.Device[i].Status = 0;
						g_DeviceSetup.count--;
						if (g_DeviceSetup.Device[i].type == DEVICE_TYPE_LOCK) { //deleer door in rftransmit
							LcSetDevice(1, D_code, 0);
						}
					}
				} else if (0 == type) {	// add or modify
					if (0 == g_DeviceSetup.Device[i].Status) {
						g_DeviceSetup.Device[i].Status = 1;	// change the device status to ENABLE
						g_DeviceSetup.count++;
					}
					while (*pLen < taglen) {
						TLVDecode(ptlv, pLen);

						if (ptlv->tag == TAG_D_type) {  // len:1
							g_DeviceSetup.Device[i].type = *(ptlv->vp);

							if (g_BusinessDataDiagnose) {
								echo_printf(", D_type:%d", g_DeviceSetup.Device[i].type);
							} else {
								sp_printf("type=%d, ", g_DeviceSetup.Device[i].type);
							}
						} else if (ptlv->tag == TAG_D_address) { // str
							strncpy((char *)(g_DeviceSetup.Device[i].address), (char *)ptlv->vp, ptlv->vlen<19 ? ptlv->vlen : 19);
							g_DeviceSetup.Device[i].address[ptlv->vlen] = '\0';

							if (g_BusinessDataDiagnose) {
								echo_printf(", D_address:%s", g_DeviceSetup.Device[i].address);
							} else {
								sp_printf("address=%s\n", g_DeviceSetup.Device[i].address);

								for (int j=0; j<g_DeviceSetup.Device[i].type; j++) {
									sp_printf("%02X", g_DeviceSetup.Device[i].address[j]);
								}
							}

							if (g_DeviceSetup.Device[i].type == DEVICE_TYPE_LOCK) { //add or change door in transmit
								LcSetDevice(0, D_code, strtoll(g_DeviceSetup.Device[i].address, NULL, 10));
								*pos = i;
							}
						} else if (ptlv->tag == TAG_D_parameter) {
							if (g_DeviceSetup.Device[i].type != 1) {
								if (g_BusinessDataDiagnose) {
									echo_printf(", D_parameter");
								}

								tlv3.NextTag = ptlv->vp;
								HaveProcessLen3 = 0;
								while (HaveProcessLen3 < ptlv->vlen) {	//the second layer tag
									TLVDecode(&tlv3, &HaveProcessLen3);
									sp_printf("HaveProcessLen3=%d, vlen=%d\r\n", HaveProcessLen3, ptlv->vlen);
									if (tlv3.tag == TAG_D_matchCode) {  // 4
										ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.MatchCode), tlv3.vp, 4);

										if (g_BusinessDataDiagnose) {
											echo_printf(", D_matchCode:%ld", g_DeviceSetup.Device[i].DeviceParameter.MatchCode);
										} else {
											sp_printf("tlv3.vp=");
											for (int k=0; k<4; k++) {
												sp_printf("%0X", *tlv3.vp);
											}
											sp_printf("\r\n");

											sp_printf("MatchCode=%08lX\n", g_DeviceSetup.Device[i].DeviceParameter.MatchCode);
										}
									} else if (tlv3.tag == TAG_M_serverIP) { // 4
										if (g_BusinessDataDiagnose) {
											echo_printf(", M_serverIP:%ld.%ld.%ld.%ld", (*((ulong*)tlv3.vp)>>24)&0xff, (*((ulong*)tlv3.vp)>>16)&0xff,(*((ulong*)tlv3.vp)>>8)&0xff, *((ulong*)tlv3.vp)&0xff);
										}

										if ( 0 == D_code ) {  // 设置主控器的服务器IP
											ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerIP), tlv3.vp, 4);
											if (g_BootPara.NetInfo.ip != g_tempServerInfo.ServerIP) {
												bIPChanged = 1;
											}
											sp_printf("in server ip is:0x%08lX\r\n", g_tempServerInfo.ServerIP);

											/*
											ConvertMemcpy((uchar *)&(g_BootPara.NetInfo.ip), tlv3.vp, 4);
											sp_printf("ServerIP=%04X\n", g_BootPara.NetInfo.ip);
											*/
										} else { // 设置其他子设备
											ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.ServerIP), tlv3.vp, 4);
											sp_printf("ServerIP=%08lX\n", g_DeviceSetup.Device[i].DeviceParameter.ServerIP);
										}

										//ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.ServerIP), tlv3.vp, 4);
									} else if (tlv3.tag == TAG_M_serverPort) {
										if (g_BusinessDataDiagnose) {
											echo_printf(", M_serverIP:%d", *((USHORT*)tlv3.vp));
										}

										if ( 0 == D_code ) {  // 设置主控器的服务器端口
											ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerPort), tlv3.vp, 2);
											if (g_BootPara.NetInfo.port  != g_tempServerInfo.ServerPort) {
												bPortChanged = 1;
											}
											sp_printf("in server port is:%d\r\n", g_tempServerInfo.ServerPort);

											/*
											ConvertMemcpy((uchar *)&(g_BootPara.NetInfo.ip), tlv3.vp, 4);
											sp_printf("ServerIP=%04X\n", g_BootPara.NetInfo.ip);
											*/
										} else { // 设置其他子设备
											ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.ServerPort), tlv3.vp, 4);
											sp_printf("ServerPort=%04X\n", g_DeviceSetup.Device[i].DeviceParameter.ServerPort);
										}

										//ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[i].DeviceParameter.ServerPort), tlv3.vp, 2);
									}
								}
							}
						}
					}
				}
				flag = 1;
				break;
			} else if (0 == g_DeviceSetup.Device[i].Status) {
				if (-1 == num) {
					num = i;
				}
			}
		}

		if (0 == flag) {	// not fonud the offered device, it's a new device
			if (0 == type) {	// add the device
				if (num != -1) {	// num is the new place, 表示可设置的空位
					if (0 == g_DeviceSetup.Device[num].Status) {
						g_DeviceSetup.Device[num].Status = 1;
					}

					sp_printf("flag=0...  num=%d, pLen=%d, taglen=%d\r\n", num, *pLen, taglen);
					g_DeviceSetup.Device[num].LogicCode = D_code;			// D_code赋值
					while (*pLen < taglen) {
						TLVDecode(ptlv, pLen);

						if (ptlv->tag == TAG_D_type) {  // 1
							g_DeviceSetup.Device[num].type = *(ptlv->vp);

							if (g_BusinessDataDiagnose) {
								echo_printf(", D_type:%d", *(ptlv->vp));
							} else {
								sp_printf("type=%d, ", g_DeviceSetup.Device[i].type);
							}
						} else if (ptlv->tag == TAG_D_address) { // str
							strncpy( (char *)(g_DeviceSetup.Device[num].address), (char *)ptlv->vp, min(ptlv->vlen, 20-1) );
							g_DeviceSetup.Device[num].address[min(ptlv->vlen, 20-1)] = '\0';

							if (g_BusinessDataDiagnose) {
								echo_printf(", D_address:%s", g_DeviceSetup.Device[num].address);
							} else {
								sp_printf("address=%s\r\n", g_DeviceSetup.Device[num].address);
								for (int j=0; j < min(ptlv->vlen, 20-1); j++) {
									sp_printf("%02X", g_DeviceSetup.Device[num].address[j]);
								}
							}
						} else if (ptlv->tag == TAG_D_parameter) {
							if (g_DeviceSetup.Device[num].type != 1) {
								if (g_BusinessDataDiagnose) {
									echo_printf(", D_parameter");
								}

								tlv3.NextTag = ptlv->vp;
								HaveProcessLen3 = 0;
								while (HaveProcessLen3 < ptlv->vlen) {
									TLVDecode(&tlv3, &HaveProcessLen3);
									sp_printf("HaveProcessLen3=%d, vlen=%d\r\n", HaveProcessLen3, ptlv->vlen);
									if (tlv3.tag == TAG_D_matchCode) {
										ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[num].DeviceParameter.MatchCode), tlv3.vp, 4);

										if (g_BusinessDataDiagnose) {
											echo_printf(", D_matchCode:%08lX", g_DeviceSetup.Device[num].DeviceParameter.MatchCode);
										} else {
											sp_printf("tlv3.vp=");
											for (int k=0; k<4; k++) {
												sp_printf("%0X", *tlv3.vp);
											}
											sp_printf("\r\n");

											sp_printf("MatchCode=%08lX\r\n", g_DeviceSetup.Device[num].DeviceParameter.MatchCode);
										}
									} else if (tlv3.tag == TAG_M_serverIP) { // 4
										if (g_BusinessDataDiagnose) {
											echo_printf(", M_serverIP:%ld.%ld.%ld.%ld", (*((ULONG*)tlv3.vp)>>24)&0xff, (*((ULONG*)tlv3.vp)>>16)&0xff,(*((ULONG*)tlv3.vp)>>8)&0xff, *((ULONG*)tlv3.vp)&0xff);
										}

										if ( 0 == D_code ) {  // 设置主控器的服务器IP
											ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerIP), tlv3.vp, 4);
											if (g_BootPara.NetInfo.ip != g_tempServerInfo.ServerIP) {
												bIPChanged = 1;
											}
											sp_printf("in server ip is:0x%08lX\r\n", g_tempServerInfo.ServerIP);

											/*
											ConvertMemcpy((uchar *)&(g_BootPara.NetInfo.ip), tlv3.vp, 4);
											sp_printf("ServerIP=%04X\n", g_BootPara.NetInfo.ip);
											*/
										} else { // 设置其他子设备
											ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[num].DeviceParameter.ServerIP), tlv3.vp, 4);
											sp_printf("ServerIP=%08lX\n", g_DeviceSetup.Device[num].DeviceParameter.ServerIP);
										}
									} else if (tlv3.tag == TAG_M_serverPort) {
										if (g_BusinessDataDiagnose) {
											echo_printf(", M_serverPort:%04X", *((USHORT *)tlv3.vp));
										}

										if ( 0 == D_code ) {  // 设置主控器的服务器端口
											ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerPort), tlv3.vp, 2);
											if (g_BootPara.NetInfo.port  != g_tempServerInfo.ServerPort) {
												bPortChanged = 1;
											}
											sp_printf("in server port is:%d\r\n", g_tempServerInfo.ServerPort);

											/*
											ConvertMemcpy((uchar *)&(g_BootPara.NetInfo.port), tlv3.vp, 2);
											sp_printf("ServerPort=%d\n", g_BootPara.NetInfo.port);
											*/
										} else { // 设置其他子设备
											ConvertMemcpy((uchar *)&(g_DeviceSetup.Device[num].DeviceParameter.ServerPort), tlv3.vp, 2);
											sp_printf("ServerPort=%d\n", g_DeviceSetup.Device[num].DeviceParameter.ServerPort);
										}
									}
								}
							}
						}
					}

					if (g_DeviceSetup.Device[num].type == DEVICE_TYPE_LOCK) { //add or change door in transmit
						LcSetDevice(0, D_code, strtoll(g_DeviceSetup.Device[num].address, NULL, 10));;
						*pos = num;
					}
					g_DeviceSetup.count++;
				}
			}
		}
	}

	sp_printf("\r\n*************************************\r\n");
	sp_printf("Deveice config list\r\n");
	sp_printf("count=%d, D_code=%d\r\n", g_DeviceSetup.count, D_code);

	for (i=0; i<MAX_SUBDEVICE; i++) {
		sp_printf("i=%2d, LogicCode=%d, address=%s, MatchCode=%04X, ServerIP=%04X, ServerPort=%02X, type=%d, status=%d\r\n", i, g_DeviceSetup.Device[i].LogicCode, \
				  g_DeviceSetup.Device[i].address, g_DeviceSetup.Device[i].DeviceParameter.MatchCode, g_DeviceSetup.Device[i].DeviceParameter.ServerIP, \
				  g_DeviceSetup.Device[i].DeviceParameter.ServerPort, g_DeviceSetup.Device[i].type, g_DeviceSetup.Device[i].Status);
	}

	sp_printf("*************************************\r\n");

	if (bIPChanged || bPortChanged) { // 设置IP和端口, 则先测试目标IP和端口
		g_bDoNewConnectTest = 1;
	}
	//StoreSystemPara();

	return 0;
}

//暂未调用
//void StoreUpgradeInfo(void)
//{
//	UCHAR tempbuf[1000];
//	USHORT len;
//	USHORT crc;
//
//	WatchDogFeed();
//	len = sizeof(g_UpgradeNotify);
//	memcpy(&tempbuf[4], (uchar *)&g_UpgradeNotify, len);
//	len = 4+len;
//	memcpy(tempbuf, (uchar *)&len, 2);
//	crc = CRC16(&tempbuf[4], len-4);
//	memcpy(&tempbuf[2], (uchar *)&crc, 2);
//	sp_printf("store flash len:%d\r\n", len);
//	Flash_EmulateE2prom_WriteByte(tempbuf, 0xC00, len);
//}

void UpgradeFile(void)
{
	static ulong previoustime = GetRtcTime(), presenttime;
	static uchar i = 0;

	presenttime = GetRtcTime();

	if (FLAG_UPGRADE_CTRL == g_flagdownloading || FLAG_UPGRADE_LOCK == g_flagdownloading) {	// downloading ctrl or lock file
		sp_printf("it's downloading... g_flagdownloading=%d, g_UpgradeNotify[%d]: offset=%d, blocktimes=%d, active=%d, uptimes=%d.\r\n",
				  g_flagdownloading, g_flagdownloading, g_UpgradeNotify[g_flagdownloading].offset, g_UpgradeNotify[g_flagdownloading].blocktimes, g_UpgradeNotify[g_flagdownloading].active, g_UpgradeNotify[g_flagdownloading].uptimes);

		if (g_UpgradeNotify[g_flagdownloading].blocktimes < MAX_REDOWNLOAD_BLOCK_TIMES) {
			if (g_UpgradeNotify[g_flagdownloading].active) {
				// has received data, go on downloading...
				/* 单包接收成功标志位清除*/
				g_UpgradeNotify[g_flagdownloading].active = 0;
				sp_printf("UpgradeFile1... g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset=%d\r\n", g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset);

				RemoteSendDownloadFile(g_UpgradeNotify[g_flagdownloading].filename, g_UpgradeNotify[g_flagdownloading].offset,
									   ((g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset)<FRAGMENT_LEN?
										(g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset):FRAGMENT_LEN));
				//BusinessPackage(RCMD_DownloadFile, BUSINESS_STATE_OK,  g_SystermInfo.BusinessSeq++, ltagBuf, ltp,  NULL, 0,  "", SOURCE_TYPE_ZKQ);
				previoustime = presenttime;
			} else {
				// 没有收到分包，超过20秒则重新发送请求包
				if (presenttime > previoustime+20) {	//not received data in more than 20 seconds, resend data...
					sp_printf("UpgradeFile2... g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset=%d\r\n", g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset);

					RemoteSendDownloadFile(g_UpgradeNotify[g_flagdownloading].filename, g_UpgradeNotify[g_flagdownloading].offset, (g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset) < FRAGMENT_LEN ? (g_UpgradeNotify[g_flagdownloading].size - g_UpgradeNotify[g_flagdownloading].offset):FRAGMENT_LEN/*g_UpgradeNotify.size*/);
					//BusinessPackage(RCMD_DownloadFile, BUSINESS_STATE_OK,  g_SystermInfo.BusinessSeq++, ltagBuf, ltp,  NULL, 0,  "", SOURCE_TYPE_ZKQ);
					previoustime = presenttime;
				} else {	//continue wait to receive data
					//break;
				}
			}
		}
	} else {	// not downloading upgrade file
		for (i=FLAG_UPGRADE_CTRL; i<=FLAG_UPGRADE_LOCK; i++) {
			if (g_UpgradeNotify[i].bUpgradeNotify) {
				// whether need to upgrade or not, 对g_flagdownloading变量进行赋值(主控器、门锁)
				g_flagdownloading = i;
				sp_printf("start downloading... g_flagdownloading=%d\r\n", g_flagdownloading);
				break;
			}
		}
	}

	//previoustime = presenttime;
}

static void UpgradeFileReset(void)
{
	static uchar i;

	if (0 == g_UpgradeNotify[FLAG_UPGRADE_CTRL].bUpgradeNotify && 0 == g_UpgradeNotify[FLAG_UPGRADE_LOCK].bUpgradeNotify) {
		g_flagdownloading = FLAG_UPGRADE_INIT;
	} else if (1 == g_UpgradeNotify[FLAG_UPGRADE_CTRL].bUpgradeNotify && 0 == g_UpgradeNotify[FLAG_UPGRADE_LOCK].bUpgradeNotify) {
		g_flagdownloading = FLAG_UPGRADE_CTRL;
	} else if (0 == g_UpgradeNotify[FLAG_UPGRADE_CTRL].bUpgradeNotify && 1 == g_UpgradeNotify[FLAG_UPGRADE_LOCK].bUpgradeNotify) {
		g_flagdownloading = FLAG_UPGRADE_LOCK;
	}

	for (i = FLAG_UPGRADE_CTRL; i <= FLAG_UPGRADE_LOCK; i++) {
		sp_printf("UpgradeFileReset... i=%d, g_flagdownloading=%d, filename=%s, downloadsuccess=%d, uptimes=%d, bUpgradeNotify=%d, blocktimes=%d, active=%d\r\n",\
				  i, g_flagdownloading, g_UpgradeNotify[i].filename, g_UpgradeNotify[i].downloadsuccess, g_UpgradeNotify[i].uptimes, g_UpgradeNotify[i].bUpgradeNotify, g_UpgradeNotify[i].blocktimes, g_UpgradeNotify[i].active);
	}
}

// 检查长度并计算整包CRC32
//static ulong CheckUpgradeFile(ulong size, int flagdownloading)
//{
//	ulong crc32, offset;
//	ulong check_add;
//	//uchar *addr;
//	uint len=500;
//	//int j;
//	uchar buf[500];
//
//	check_add = g_uptempadd[flagdownloading];
//	//TODO 这个校验实际无用
//	if (FLASH_UPGRADE_ADD != check_add && PWLMS_UPDATE_START_ADD != check_add) {
//		sp_printf("%s error, wrong check_add. check_add=0x%08X\r\n", __func__, check_add);
//		return 0;
//	}
//
//	// 检查有无超限
//	if (check_add+size > g_uptempadd_end[flagdownloading]) {
//		sp_printf("%s error, size to long. size=0x%08X\r\n", __func__, size);
//		return 0;
//	}
//
//	// 计算整包CRC
//	crc32 = 0xFFFFFFFF;
//	/* len = 500 */
//	for (offset=0; offset<size; offset+=len) {
//		len = len<(size-offset)?len:(size-offset);
//		Flash_EmulateE2prom_ReadByte(buf, check_add+offset, len);
////		if (offset == 0) {
//			// 计算时 标记恢复为5555AAAA进行计算
////			buf[40] = 0x55;
////			buf[41] = 0x55;
////			buf[42] = 0xAA;
////			buf[43] = 0xAA;
////		}
//		crc32 = CRC32_recursion(buf, len, crc32);
//	}
//	crc32 ^= 0xFFFFFFFF;
//
//	return crc32;
//}

void decode_lock_forward_cmd(UCHAR *data, int len, int seq, int type)
{
	char para_str[36];
	UCHAR para_ch;
	UCHAR Keyword = 0;
	UCHAR flag_device = 0;
	//UCHAR flag_remotectrl = 0;
	//UCHAR flag_param = 0;
	USHORT para_sh;
	USHORT D_code = 0xFFFF;
	UINT para_int;
	int HaveProcessLen1, HaveProcessLen2, HaveProcessLen3;
	int flag_apdu = 0;
	typTLV tlv1, tlv2, tlv3;

	if (0 == g_BusinessDataDiagnose) {
		return ;
	}

	if (0 == type) { // from ctrl
		echo_printf("[DEBUG] Server >> Forward - Request,  Sequence:%d", seq);
	} else if (1 == type) { // from lock
		echo_printf("[DEBUG] Lock   >> Forward Response, LCSeq=0x%08X", seq);
	}

	tlv1.NextTag = data;
	HaveProcessLen1 = 0;
	while (HaveProcessLen1 - len < 0) {            //the first layer tag
		TLVDecode(&tlv1, &HaveProcessLen1);
		if (tlv1.tag == TAG_D_code) {
			ConvertMemcpy((uchar *)&D_code, tlv1.vp, 2);
			//sp_printf("vp=%d, D_code=%d\r\n", *(ushort*)(tlv1.vp), D_code);
			echo_printf(". D_code:%d", D_code);

			if (0 == D_code) {	// 发给主控器
				flag_device = 1;
			} else if (100 == D_code) { // 发给门锁
				flag_device = 2;
			} else {
				;
			}
		} else if (tlv1.tag == TAG_D_keyword) {
			Keyword = *tlv1.vp;

#if 0
			if (g_BusinessDataDiagnose) {
				echo_printf("Keyword=0x%x\r\n", Keyword);
			}
#endif
		} else if (TAG_D_resultCode == tlv1.tag) { // 回复指令
			echo_printf(". D_resultCode:%d", *tlv1.vp);
		} else if (TAG_D_encryptInfo == tlv1.tag) {
			;
		} else if (tlv1.tag == TAG_D_data) {
			tlv2.NextTag = tlv1.vp;
			HaveProcessLen2 = 0;
			while (HaveProcessLen2 < tlv1.vlen) {	//the second layer tag
				TLVDecode(&tlv2, &HaveProcessLen2);

				if (tlv2.tag == TAG_D_parameter && Keyword == RCMD_SetupParameter) { // 设置参数
					echo_printf(". Setup Parameter");

					tlv3.NextTag = tlv2.vp;
					HaveProcessLen3 = 0;
					if (2 == flag_device) { // 对门锁发Setup Parameter指令
						while (HaveProcessLen3 < tlv2.vlen) {	//the second layer tag
							TLVDecode(&tlv3, &HaveProcessLen3);

							if (tlv3.tag == TAG_L_areaID) { // len: 4
								ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);
								echo_printf(", areaID:%d", para_int);
							} else if (tlv3.tag == TAG_L_openDelay) { // 1
								ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
								echo_printf(", openDelay:%d", para_ch);
							} else if (tlv3.tag == TAG_L_PIN) { // str
								ConvertMemcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<30 ? tlv3.vlen : 30);
								para_str[tlv3.vlen<30 ? tlv3.vlen : 30] = '\0';
								echo_printf(", PIN:%s", para_str);
							} else if (tlv3.tag == TAG_L_alertMode) { // 1
								ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
								echo_printf(", alertMode:%d", para_ch);
							} else if (tlv3.tag == TAG_L_fireLink) {  // 1
								ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
								echo_printf(", fireLink:%d", para_ch);
							} else if (tlv3.tag == TAG_L_defenceMode) { // 1
								ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
								echo_printf(", defenceMode:%d", para_ch);
							} else if (tlv3.tag == TAG_L_fingerCheckMode) { // 1
								ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
								echo_printf(", fingerCheckMod:%d", para_ch);
							} else if (tlv3.tag == TAG_L_fingerCheckParam) { // str
								ConvertMemcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<30 ? tlv3.vlen : 30);
								para_str[tlv3.vlen<30 ? tlv3.vlen : 30] = '\0';
								echo_printf(", fingerCheckParam:%s", para_str);
							} else if (tlv3.tag == TAG_L_allowedOpenMode) { // str
								ConvertMemcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<30 ? tlv3.vlen : 30);
								para_str[tlv3.vlen<30 ? tlv3.vlen : 30] = '\0';
								echo_printf(", allowedOpenMode:%s", para_str);
							} else if (tlv3.tag == TAG_M_LOCATION_ID) { // str
								memcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<36 ? tlv3.vlen : 36);
								para_str[tlv3.vlen<36 ? tlv3.vlen : 36] = '\0';
								echo_printf(", LOCATION_ID:%s", para_str);
							}
						}
					} else if (1 == flag_device) { // 对主控器发Setup Parameter指令
						while (HaveProcessLen3 < tlv2.vlen) {	//the second layer tag
							TLVDecode(&tlv3, &HaveProcessLen3);

							if (tlv3.tag == TAG_D_matchCode) { // len: 4
								ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);
								echo_printf(", matchCode:%d", para_int);
							} else if (tlv3.tag == TAG_D_refreshInterval) { // 4
								ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);
								echo_printf(", refreshInterval:%d", para_int);
							} else if (tlv3.tag == TAG_M_serverIP) { // 4
								ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);

								echo_printf(", M_serverIP:%d.%d.%d.%d", (para_int>>24)&0xff, (para_int>>16)&0xff,(para_int>>8)&0xff, para_int&0xff);
								//echo_printf(", serverIP:%d", para_int);
							} else if (tlv3.tag == TAG_M_serverPort) { // 2
								ConvertMemcpy((uchar *)&para_sh, tlv3.vp, 2);
								echo_printf(", serverPort:%d", para_sh);
							} else if (tlv3.tag == TAG_M_serverURL) { // str
								memcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<36 ? tlv3.vlen : 30);
								para_str[tlv3.vlen<36 ? tlv3.vlen : 36] = '\0';
								echo_printf(", serverURL:%s", para_str);
							} else if (tlv3.tag == TAG_D_alertMode) { // 2
								ConvertMemcpy((uchar *)&para_sh, tlv3.vp, 2);
								echo_printf(", alertMode:%d", para_sh);
							} else if (tlv3.tag == TAG_M_LOCATION_ID) { // str
								memcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<36 ? tlv3.vlen : 36);
								para_str[tlv3.vlen<36 ? tlv3.vlen : 36] = '\0';
								echo_printf(", LOCATION_ID:%s", para_str);
							}
						}
					}


					//echo_printf("[DEBUG] Server << Forward - Response, Sequence=%d. status=%d, BUSINESS_STATE_OK\r\n", seq, BUSINESS_STATE_OK);
					//BusinessPackage(RCMD_Forward_Response, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
				} else if (tlv2.tag == TAG_L_popedom && Keyword == RCMD_PutPopedom) { // 下载权限
					echo_printf(". Put Popedom");

					tlv3.NextTag = tlv2.vp;
					HaveProcessLen3 = 0;
					while (HaveProcessLen3 < tlv2.vlen) {	//the second layer tag
//						           tlv3 = TLVDecode1(tlv2.NextTag, &HaveProcessLen3);
						TLVDecode(&tlv3, &HaveProcessLen3);
						if (tlv3.tag == TAG_M_operate) { // 1
							ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
							echo_printf(", operate:%d", para_ch);
						} else if (tlv3.tag == TAG_L_popedomType) { // 1
							ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
							echo_printf(", popedomType:%d", para_ch);
						} else if (tlv3.tag == TAG_L_cardNo) { // 8
							echo_printf(", cardNo:");
							for (int i=0; i<8; i++) {
								echo_printf("%02X", *(tlv3.vp+i));
							}

							//ConvertMemcpy((uchar *)&para_str, tlv3.vp, tlv3.vlen<30 ? tlv3.vlen : 30);
							//para_str[tlv3.vlen<30 ? tlv3.vlen : 30] = '\0';
							//echo_printf(", cardNo:%s", para_str);
						} else if (tlv3.tag == TAG_M_fingerID) { // 4
							ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);
							echo_printf(", fingerID:%d", para_int);
						} else if (tlv3.tag == TAG_M_startTime) { // 4
							ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);

							echo_printf(", startTime:");
							PrintfTimeDec(para_int);
							echo_printf(" (0x%X)", para_int);
						} else if (tlv3.tag == TAG_M_endTime) { // 4
							ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);

							echo_printf(", endTime:");
							PrintfTimeDec(para_int);
							echo_printf(" (0x%X)", para_int);
						} else if (tlv3.tag == TAG_M_fingerCode) { // 4
							ConvertMemcpy((uchar *)&para_int, tlv3.vp, 4);
							echo_printf(", fingerCode:");
						}
					}


					//echo_printf("[DEBUG] Server << Forward - Response, Sequence=%d. status=%d, BUSINESS_STATE_OK\r\n", seq, BUSINESS_STATE_OK);
					//BusinessPackage(RCMD_Forward_Response, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
				} else if (Keyword == RCMD_RemoteControl) { // 远程控制
					echo_printf(". RemoteControl");

					tlv3.NextTag = tlv2.vp;
					HaveProcessLen3 = 0;
					while (HaveProcessLen3 < tlv2.vlen) {	//the second layer tag
						TLVDecode(&tlv3, &HaveProcessLen3);
						if (tlv3.tag == TAG_L_controlType) { // 1
							ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
							echo_printf(", controlType:%d", para_ch);
						} else if (tlv3.tag == TAG_L_remoteOpenMode) { // 1
							ConvertMemcpy((uchar *)&para_ch, tlv3.vp, 1);
							echo_printf(", remoteOpenMode:%d", para_ch);
						}
					}

					//echo_printf("[DEBUG] Server << Forward - Response, Sequence=%d. status=%d, BUSINESS_STATE_OK\r\n", seq, BUSINESS_STATE_OK);
					//BusinessPackage(RCMD_Forward_Response, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
				} else if (tlv2.tag == TAG_M_APDU && Keyword == RCMD_UpdatePSAM) { // 更新PSAM密钥
					//echo_printf("\r\nHaveProcessLen2=%d, tlv1.vlen=%d\r\n", HaveProcessLen3, tlv2.vlen);

					if (0 == flag_apdu) { // 第一次解析apdu指令
						flag_apdu = 1;
						echo_printf(". Update PSAM");
					}
					echo_printf(", \r\nM_APDU(%d):", tlv2.vlen);
					int i;
					for (i = 0; i < tlv2.vlen; i++) {
						echo_printf("%02X", *(tlv2.vp+i));
					}
					//echo_printf("[DEBUG] Server << Forward - Response, Sequence=%d. status=%d, BUSINESS_STATE_OK\r\n", seq, BUSINESS_STATE_OK);
					//BusinessPackage(RCMD_Forward_Response, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
				} else if (tlv2.tag == TAG_M_POR) { // Update PSAM 回复数据
					echo_printf(". Update PSAM - Res");

					echo_printf(", \r\nM_POR(%d):", tlv2.vlen);
					int i;
					for (i = 0; i < tlv2.vlen; i++) {
						echo_printf("%02X", *(tlv2.vp+i));
					}
				}
			}
		}
	}

	echo_printf(".\r\n");

	return ;
}

/* 将日期转化成从公元1970-01-01年算起至传入时间的UTC时间所经过的秒数
 * fname: 20140828-095705
 * 返回: 从公元1970-01-01年算起至传入时间的UTC时间所经过的秒数
 */
static ulong Convert_time(char *fname)
{
	int Ret = 0;
	struct tm time_cha;

	Ret = sscanf(fname, "%04d%02d%02d-%02d%02d%02d", &(time_cha.tm_year), &(time_cha.tm_mon), &(time_cha.tm_mday),
				 &(time_cha.tm_hour), &(time_cha.tm_min), &(time_cha.tm_sec));

	Log("BIZ", LOG_LV_DEBUG, "Ret=%d, %s, %04d%02d%02d-%02d%02d%02d.", Ret, fname, time_cha.tm_year, time_cha.tm_mon, time_cha.tm_mday,
		time_cha.tm_hour, time_cha.tm_min, time_cha.tm_sec);

	time_cha.tm_year-= 1900;
	time_cha.tm_mon -= 1;

	return (mktime(&time_cha));
}

int head_of_manage_busi_msg_fill( typBUSINESS_LAYER_HEAD *head, uchar *data, int len )
{
	head->type = 0xff;
	ConvertMemcpy((uchar *)&(head->length), &data[0], 2);
	head->cmd = data[2];
	head->cmd_status = data[3];
	ConvertMemcpy((uchar *)&(head->seq), &data[4], 4);
	head->rhl = data[8];
	return 0;
}

static void Randombytes(uint8_t *buf, int len)
{
	int rnd, n;
	unsigned int time =  GetRtcTime();

	srand(time);

	for (int i = 0; i < len; ) {
		for (rnd = rand(), n = (len - i < 4?len - i:4); n-- > 0; rnd >>= 8) {
			buf[i++] = (uchar)rnd;
		}
	}
}


static int xor_random(unsigned char random1[8], unsigned char random2[8], unsigned char result[8])
{
	int i;

	for (i=0; i<8; i++) {
		result[i] = random1[i] ^ random2[i];
	}

	return 0;
}

static int CheckServerAuthData(uchar *data)
{
	uchar TempSecurityData[32];
	uchar DUID[9];  // 设备唯一识别码
	int total_len = 0;
	//int macadd_len = 0;

	// 生成客户端认证数据
	/* 第一步, 进行3DES_CBC加密 */
	memcpy(TempSecurityData, g_SystermInfo.Random2, 8);
	TempSecurityData[8] = 0x14;
	TempSecurityData[8+1] = 0x30;
	memset(&TempSecurityData[8+2], 0, 6);

	/* 生成设备唯一识别码 */
	strcpy((char *)DUID, "12345678");

	memcpy(&TempSecurityData[16], DUID, 8);
	//strcpy((char *)&TempSecurityData[16], g_SystermInfo.macadd);
	//macadd_len = strlen(g_SystermInfo.macadd);

	total_len = 16+8;
	if (total_len % 8 != 0) {	// 不是8的倍数，后面补0
		memset((char *)&TempSecurityData[total_len], 0,8 - total_len%8);
		total_len = total_len/8*8+8;
	}

	ser_printf("total_len=%d\r\n", total_len);

	ser_printf("TempSecurityData before=");
	for (int i=0; i<total_len; i++) {
		ser_printf("%02x,", TempSecurityData[i]);
	}
	ser_printf("\r\n");

	ThDesCbc2keyEncode(TempSecurityData, total_len, g_SystermInfo.SessionKey, cveci);    // 使用临时会话密钥SessionKey, 进行3DES_CBC加密

	ser_printf("TempSecurityData after=");
	for (int i=0; i<total_len; i++) {
		ser_printf("%02x,", TempSecurityData[i]);
	}
	ser_printf("\r\n");

	/* 第二步, 做MD5哈希计算 */
	uint8_t mp[16] = {0};
	Md5Data(TempSecurityData, total_len, mp);

	ser_printf("M_authData2=");
	for (int i=0; i<16; i++) {
		ser_printf("%02x,", mp[i]);
	}
	ser_printf("\r\n");

	if (memcmp(mp, data, 16) == 0) { // 如果比较相同, 则通过认证, 否则返回失败
		return 1;
	} else {
		return 0;
	}
}

static int RemoteSendInitializeSession(ulong seq)
{
	uchar TempSecurityData[32];
	uchar sessionbuf[32];
	uchar ltagBuf[100];
	uchar tempBuf[200];
	uchar DUID[9];  // 设备唯一识别码
	uchar protocol_version;
	uchar random[8];

	//uchar temp;
	//ushort wmc_code;
	//int tp, tempdcode, t;
	//int macadd_len = 0;
	int ltp;
	int total_len = 0;

	ltp = 0;
	protocol_version = 0x30;

// 生成客户端随机数
	Randombytes(g_SystermInfo.Random2, 8);

	ser_printf("has got g_SystermInfo.Random2:\r\n");
	for (int i=0; i<8; i++) {
		ser_printf("%02x,", g_SystermInfo.Random2[i]);
	}
	ser_printf("\r\n");

// 生成临时会话密钥
	//long long random;

	//random = (long long)g_SystermInfo.Random1[0] ^ (long long)g_SystermInfo.Random2[0];
	//memcpy(sessionbuf, (uchar*)&random, 8);

	xor_random(g_SystermInfo.Random1, g_SystermInfo.Random2, random);
	memcpy(sessionbuf, random, 8);

	sessionbuf[8] = 0x12;
	sessionbuf[9] = 0x28;
	memset(&sessionbuf[10], 0, 6);

	ser_printf("sessionbuf=");
	for (int i=0; i<16; i++) {
		ser_printf("%02x,", sessionbuf[i]);
	}
	ser_printf("\r\n");

	ThDesCbc2keyEncode(sessionbuf, 16, g_SystermInfo.ManagePassword, cveci);   // 进行3DES_CBC加密, 生成临时会话密钥

	memcpy(g_SystermInfo.SessionKey, sessionbuf, 16);

	ser_printf("g_SystermInfo.SessionKey=");
	for (int i=0; i<16; i++) {
		ser_printf("%02x,", g_SystermInfo.SessionKey[i]);
	}
	ser_printf("\r\n");

	// 生成客户端认证数据
	/* 第一步, 进行3DES_CBC加密 */
	memcpy(TempSecurityData, g_SystermInfo.Random1, 8);
	TempSecurityData[8] = 0x13;
	TempSecurityData[8+1] = 0x29;
	memset(&TempSecurityData[8+2], 0, 6);

	/* 生成设备唯一识别码 */
	strcpy((char *)DUID, "12345678");

	memcpy(&TempSecurityData[16], DUID, 8);
	//strcpy((char *)&TempSecurityData[16], g_SystermInfo.macadd);
	//macadd_len = strlen(g_SystermInfo.macadd);

	total_len = 16+8;
	if (total_len % 8 != 0) {	// 不是8的倍数，后面补0
		memset((char *)&TempSecurityData[total_len], 0,8 - total_len%8);
		total_len = total_len/8*8+8;
	}

	ser_printf("total_len=%d\r\n", total_len);

	ser_printf("TempSecurityData before=");
	for (int i=0; i<total_len; i++) {
		ser_printf("%02x,", TempSecurityData[i]);
	}
	ser_printf("\r\n");

	ThDesCbc2keyEncode(TempSecurityData, total_len, g_SystermInfo.SessionKey, cveci);    // 使用临时会话密钥SessionKey, 进行3DES_CBC加密

	ser_printf("TempSecurityData after=");
	for (int i=0; i<total_len; i++) {
		ser_printf("%02x,", TempSecurityData[i]);
	}
	ser_printf("\r\n");

	/* 第二步, 做MD5哈希计算 */
	uint8_t mp[16] = {0};

	Md5Data(TempSecurityData, total_len, mp);

	ser_printf("M_authData=");
	for (int i=0; i<16; i++) {
		ser_printf("%02x,", mp[i]);
	}
	ser_printf("\r\n");

	ltp += TLVEncode(TAG_M_version, 1, (uchar *)&protocol_version, 1, &ltagBuf[ltp]);	// 支持协议版本号
	ltp += TLVEncode(TAG_M_random, 8, g_SystermInfo.Random2, 0, &ltagBuf[ltp]);	// 客户端生成的随机数
	ltp += TLVEncode(TAG_D_address, 18, (uchar *)g_SystermInfo.macadd, 0, &ltagBuf[ltp]);	// 客户端地址
	ltp += TLVEncode(TAG_D_DUID, 8, DUID, 0, &ltagBuf[ltp]);	// 设备唯一识别码, 大端方式
	ltp += TLVEncode(TAG_M_IMSI, 15, g_SystermInfo.StoreImsi, 0, ltagBuf+ltp);
	ltp += TLVEncode(TAG_M_authData, 16, mp, 0, &ltagBuf[ltp]);	// 客户端认证数据

	memcpy(tempBuf, ltagBuf, ltp);

	if (seq < 0x80000000) {
		BusinessPackage(RCMD_InitializeSession, BUSINESS_STATE_OK, seq/*g_Sequence++*/, tempBuf, ltp, NULL, 0, NULL, BUSINESS_TYPE_CM);
	} else {
		//BusinessPackage(RCMD_QueryStatus_Response, BUSINESS_STATE_OK, seq-0x80000000, tempBuf, ltp, NULL, 0, "", BUSINESS_TYPE_CM);
	}

	return 0;
}


/*
     Business layer decode
*/
int BusinessDecode(uchar *data, int len)
{
	uchar ltagBuf[256];
	uchar bfa;					// 后续TLV数据的起始偏移
	uint32_t LogicCode = 0;		// V3中门锁地址为4字节
	int TagTotalLen = 0;		// user data中包含TLV格式
	uchar cmd;
	uint8_t CmdStat = 0;
	int i;
	int CommFlag = 0;
	int HaveProcessLen = 0;
	ulong seq;
	ulong stime;
	typTLV tlv = {0};

	if (ProtVer/16 == 2) {
		if (len < 9) {
			return -1;
		}

		cmd = data[2]&0x7F;
		ConvertMemcpy(&seq, data+4, 4);	// 服务器下发数据中的业务层流水号

		bfa = 9;	//body first add
	} else {
		if (len < 3) {
			return -1;
		}

		tlv.NextTag = data;
		TLVDecode(&tlv, &HaveProcessLen);
		cmd = tlv.vp[0]&0x7F;

		if (TAG_M_request == tlv.tag) {	// requeset, 对于request需要回复response指令
			if (RCMD_Welcome == cmd) {	// 如果是welcome指令, 无流水号, 不需要回复. 直接发送initialize指令
				;
				//seq = g_Sequence++;
			} else if (tlv.vlen>=5 && len>=HaveProcessLen) {
				ConvertMemcpy(&seq, tlv.vp+1, 4);  // 大端模式转小端模式
			} else {	// 非welcome指令, 且数据长度小于request包长度, 过滤
				return -1;
			}
		} else if (TAG_M_response == tlv.tag) {	// response, 对于response, 解析但不需要回复
			g_TransmitControl.bSubmitResponseReceived = 1;
			if (tlv.vlen>=6 && len>=HaveProcessLen) { // TAG 1+len 1+val 6
				ConvertMemcpy(&seq, tlv.vp+1, 4);
			}
			CmdStat = tlv.vp[5];
//			// there is error that accord, 有错误, 直接返回. 无错误, 继续解析
//			if (tlv.vp[5]) {
//				return -1;
//			}
		} else {
			return -1;
		}

		bfa = HaveProcessLen;		//body first add
	}

	TagTotalLen = len-bfa;
	if (TagTotalLen < 0) {
		return -1;
	}

	if (ProtVer/16 == 2) {
		switch (cmd) {
		case RCMD_ResetNotify: {
			len = 0;
			tlv.NextTag = &data[bfa];
			tlv = TLVDecode1(tlv.NextTag, &HaveProcessLen);
			if (tlv.tag==TAG_D_code && tlv.vlen>=2 && tlv.vlen<=4) {
				ConvertMemcpy(&LogicCode, tlv.vp, tlv.vlen);
				Log("BIZ", LOG_LV_DEBUG, "RCMD_ResetNotify, seq=%lu, c=%hu", seq, LogicCode);

				if (LogicCode == 0xffff) { //query all deveice
					LcCmdReset(-1);
					ser_printf("g_bResetWMC1...\r\n");
					g_bResetWMC = 1;
				} else {
					if (0 == LogicCode) { // reboot the WMC
						ser_printf("g_bResetWMC2...\r\n");
						sp_printf("reboot the WMC\r\n");
						g_bResetWMC = 1;
					} else {
						/* 门锁不联机, 则直接返回错误. 门锁已联机，则先转发给门锁，待门锁回复后再回复 */
						if (LcGetOnline(LogicCode) != 1) {
							if (g_BusinessDataDiagnose) {
								echo_printf("[DEBUG] Server << ResetNotify - Response, Sequence=%ld. state:0x%02X (BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE).\r\n",\
											seq, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE);
//                                debug_printf("");
							}

							BusinessPackage(RCMD_ResetNotify_Response, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE, seq, NULL, len, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
							break;
						}

						LcCmdReset(LogicCode);
					}
				}

				//LCSetTime();

				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << ResetNotify - Response, Sequence=%ld. Status:%d, BUSINESS_STATE_OK .\r\n", seq, BUSINESS_STATE_OK);
//                debug_printf("");
				}

				BusinessPackage(RCMD_ResetNotify_Response, BUSINESS_STATE_OK, seq, NULL, len, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
//				sleep(3);
				/*if(bResetWMC)
				{
					sleep(3);
					reboot();
				}*/
			}
		}
		break;

		case RCMD_READIMSI: {
			int ltp;
			//uchar ltagBuf[40];

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> Read IMSI - Request,  Sequence=%ld.\r\n", seq);
//            debug_printf("");
			}

			ltp = TLVEncode(TAG_M_IMSI, 15, (uchar *)&g_SystermInfo.StoreImsi[0], 0, &ltagBuf[0]);

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server << Read IMSI - Response, Sequence=%ld. IMSI:%s.\r\n", seq, g_SystermInfo.StoreImsi);
//            debug_printf("");
			}

			BusinessPackage(RCMD_READIMSI+0x80, BUSINESS_STATE_OK, seq, ltagBuf, ltp, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
		}
		break;

		case RCMD_ReadFile: {
//        ulong crc32;
			uchar flag;
			int offset, rdlen;
			int HaveProcessLen = 0;
			ushort D_code;
			char fname[50];
			uchar TransmitMode;	// 传输模式
			ulong TransmitHandle;	// 传输句柄(用于上传照片)

			tlv.NextTag = &data[bfa];
			flag = 0;

			while(HaveProcessLen < TagTotalLen) {
//				tlv = TLVDecode1(tlv.NextTag, &HaveProcessLen);
				TLVDecode(&tlv, &HaveProcessLen);
				sp_printf("tlv.vlen=%d, tlv.tag=%d\r\n", tlv.vlen, tlv.tag);
				if(tlv.tag == TAG_D_code) {
					ConvertMemcpy((uchar *)&D_code, tlv.vp, 2);
					sp_printf("D_code=0x%04X, %d\r\n", D_code, D_code);

					if(1100 == D_code || 1101 == D_code) {	// 发给拍照器
						flag |= 1;
					} else {
						flag = 0;
						break;
					}
				} else if(tlv.tag == TAG_M_file) {	// 文件定位字符串
					memcpy(fname, tlv.vp, tlv.vlen);
					fname[tlv.vlen] = '\0';
					sp_printf("fname=%s\r\n", fname);

					flag |= 1<<1;
				} else if(tlv.tag == TAG_M_offset) {	// 下载偏移量
					ConvertMemcpy((uchar *)&offset, tlv.vp, tlv.vlen);
					sp_printf("offse=%d\r\n", offset);

					flag |= 1<<2;
				} else if(tlv.tag == TAG_M_size) {	// 下载分片大小
					ConvertMemcpy((uchar *)&rdlen, tlv.vp, tlv.vlen);
					sp_printf("rdlen=%d\r\n", rdlen);

					flag |= 1<<3;
				} else if(tlv.tag == TAG_M_transmitHandle) {	// 传输句柄
					ConvertMemcpy((uchar *)&TransmitHandle, tlv.vp, tlv.vlen);
					sp_printf("g_TransmitHandle=%d\r\n", TransmitHandle);

					flag |= 1<<4;
				} else if(tlv.tag == TAG_M_transmitMode) {	// 传输模式
					ConvertMemcpy((uchar *)&TransmitMode, tlv.vp, tlv.vlen);
					sp_printf("TransmitMode=%d\r\n", TransmitMode);

					flag |= 1<<5;
				}
			}

			//TransmitHandle = 0;
			//TransmitMode = 2;	// UPD方式上传照片

			sp_printf("flag=%X\r\n", flag);
			if(0x33 == flag) {	// 第一次ReadFile, D_code + M_file=0x33. 返回M_content + M_crc32 + M_size.
				offset = 0;
				rdlen = 800;	//第一次ReadFile默认读取长度为500字节
			}

			//if(1)
			if(0x03 == (flag & 0x03) || 0x3F == flag) {	//第一次(0x33)或第二次ReadFile: D_code + M_file + M_offset + M_size+M_transmitHandle+M_transmitMode=0x3F
				//这里调用上传录像文件接口
				// ***
//                REC_EVENT UploadEvent;  // 获取视频文件参数
//
//                UploadEvent.dcode = D_code;
//                UploadEvent.cType = fname[0];
//                UploadEvent.lStart_time = Convert_time(fname+2);
//                UploadEvent.lRec_time = 0;
//
//                printf("UploadEvent.lStart_time=%lu\r\n", UploadEvent.lStart_time);
//
//                read_rec_event_push(&UploadEvent);
//fprintf(stderr, "s=%s, r=%ld\n", fname+2, Convert_time(fname+2));
				CamUpFlNb(D_code-CAM_DCODE_BASE, fname[0], Convert_time(fname+2));
				BusinessPackage(RCMD_ReadFile+0x80, BUSINESS_STATE_OK, seq, NULL, 0,  NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			} else {
				BusinessPackage(RCMD_ReadFile+0x80, BUSINESS_STATE_ERROR_READFILE_ARGERROR, seq, NULL, 0,  NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			}
		}
		break;

		default:
			CommFlag = 1;
			break;
		}

	} else {
		switch (cmd) {
		case RCMD_Welcome: {	// 收到TCP连接welcome报文
			uchar pro_version;   // 服务器支持的协议版本号

			tlv.NextTag = &data[bfa];
			HaveProcessLen = 0;

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> Welcome - Request,  Sequence=%ld.\r\n", seq);
			} else {
				sp_printf("start to process RCMD_Welcome...\r\n");
			}

			while (HaveProcessLen < TagTotalLen) {            //the first layer tag
				TLVDecode(&tlv, &HaveProcessLen);
				if (tlv.tag == TAG_M_version) {	// 服务器支持的最高协议版本号
					pro_version = *tlv.vp;
					ser_printf("pro_version=%d.%d\r\n", pro_version>>4, pro_version&0xF);
				} else if (tlv.tag == TAG_M_random) {	// random1, 服务器生成的随机数
					memcpy(g_SystermInfo.Random1, tlv.vp, 8);   // 接收数据采用大端方式
					//ConvertMemcpy(g_SystermInfo.Random1, tlv.vp, 8);

					ser_printf("has got g_SystermInfo.Random1:\r\n");
					for (int i=0; i<8; i++) {
						ser_printf("%02x,", g_SystermInfo.Random1[i]);
					}
					ser_printf("\r\n");
				}
			}


			RemoteSendInitializeSession(g_SystermInfo.BusinessSeq++);	// seq >= 0x80000000
			//BusinessPackage(RCMD_INITIALIZE, BUSINESS_STATE_OK, seq, ltagBuf, len, NULL, 0, "", SOURCE_TYPE_ZKQ);

			g_RECVWELCOME = 1;
		}
		break;

		case RCMD_InitializeSession: {	//收到初始化命令回复
			uchar authData[16];

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> InitializeSession - Response, Sequence=%ld.\r\n", seq);
			} else {
				ser_printf("has got RCMD_InitializeSession...\r\n");
			}
			tlv.NextTag = data+bfa;
			HaveProcessLen = 0;
			while (HaveProcessLen < TagTotalLen) {            //the first layer tag
				TLVDecode(&tlv, &HaveProcessLen);
				if (tlv.tag == TAG_M_authData) { // 服务器端认证数据
					memcpy(authData, tlv.vp, 16);   // 接收数据采用大端方式
					//ConvertMemcpy(authData, tlv.vp, 16);
				} else if (tlv.tag == TAG_M_sessionId) {	// 会话ID，如果不成功，此字段不存在
					ser_printf("has got sessionId...\r\n");
				}
			}

			/* 进行服务器端的认证 */
			g_initialized = 0;

			ser_printf("authData server=");
			for (int i=0; i<16; i++) {
				ser_printf("%02x,", authData[i]);
			}
			ser_printf("\r\n");

			if (CheckServerAuthData(authData)) { // 通过认证
				ser_printf("CheckServerAuthData OK...\r\n");
				g_initialized = 1;

				/* 重新发送online指令 */
				g_TransmitControl.bSendOnlineNotify = 0;

				//RfnetSoftInit();
			}
		}
		break;

		case RCMD_RemoteControl: {  // 远程控制
			uchar controlAction = 0;
			ulong D_code;
			int HaveProcessLen1;
			typTLV tlv1;

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> RCMD_RemoteControl - Request, Sequence=%ld. ", seq);
			} else {
				ser_printf("has got RCMD_RemoteControl...\r\n");
			}

			tlv1.NextTag = &data[bfa];
			HaveProcessLen1 = 0;
			while (HaveProcessLen1 < TagTotalLen) {            //the first layer tag
				TLVDecode(&tlv1, &HaveProcessLen1);
				if (tlv1.tag == TAG_D_code) {
					ConvertMemcpy((uchar *)&D_code, tlv1.vp, 4);
				} else if (tlv1.tag == TAG_D_controlAction) {
					controlAction = *tlv1.vp;

					if (g_BusinessDataDiagnose) {
						echo_printf(", D_controlAction=%d", controlAction);
					}
				} else if (tlv1.tag == TAG_D_controlParam) {
					if (g_BusinessDataDiagnose) {
						echo_printf(", D_controlParam=%d", *tlv1.vp);
					}
				}
			}

			if (0 == D_code) {	// 发给主控器
				sp_printf("1111111111111111111111111111111111111111111111111111111111111\r\n");

				if (controlAction == 2) { // 对主控器做复位操作
					if (g_BusinessDataDiagnose) {
						echo_printf(" (reset the WMC)");
					} else {
						ser_printf("g_bResetWMC2...\r\n");
						sp_printf("reboot the WMC\r\n");
					}

					if (g_BusinessDataDiagnose) {
						echo_printf(".\r\n");

						echo_printf("[DEBUG] Server << RCMD_RemoteControl - Response, Sequence=%ld. state:%d, BUSINESS_STATE_OK.\r\n", seq, BUSINESS_STATE_OK);
					}

					BusinessPackage(RCMD_RemoteControl+0x80, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);

					g_bResetWMC = 1;
				} else { // 做其他操作, 返回未知参数错误
					if (g_BusinessDataDiagnose) {
						echo_printf(".\r\n");

						echo_printf("[DEBUG] Server << RCMD_RemoteControl - Response, Sequence=%ld. state:%d, BUSINESS_STATE_ERROR_UNKNOWN_PARAM_NAME.\r\n", seq, BUSINESS_STATE_ERROR_UNKNOWN_PARAM_NAME);
					}

					BusinessPackage(RCMD_RemoteControl+0x80, BUSINESS_STATE_ERROR_UNKNOWN_PARAM_NAME, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
				}
			} else {
				// 发给门锁, 直接打包成forward指令发到门锁
				if (g_BusinessDataDiagnose) {
					echo_printf(".\r\n");
				}
				if (LcGetOnline(D_code) == 0) {
					if (g_BusinessDataDiagnose) {
						echo_printf("[DEBUG] Server << RCMD_RemoteControl - Response, Sequence=%ld. state:%d, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE.\r\n", seq, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE);
					}

					BusinessPackage(RCMD_RemoteControl+0x80, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
					break;
				}

				// TODO 组包转发给门锁
				uchar *BusinessDataBuf = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
				/* 服务器指令打包到request tag中 */
				uchar tempdata[255];		// 内部数据
				uchar chl = 4;
				uchar request_data[5];
				ushort total_len;
				ushort sub_total_len;
				ushort crc16;
				int ltp;

				/* 内层数据 */
				total_len = 2;  // CPL_2
				chl = 2;
				tempdata[2] = chl;  // CHL_1+spi_1+crc16_2=4
				tempdata[3] = 0;    // spi=0x20;

				total_len += chl;
				request_data[0] = RCMD_RemoteControl;   // 指令码
				ltp = TLVEncode(TAG_M_request, 1, request_data, 0, &tempdata[total_len]);    // request TAG
				total_len += ltp;
				ser_printf("len=%d, bfa=%d, diff=%d\r\n", len, bfa, len-bfa);
				memcpy(tempdata+total_len, data+bfa, len-bfa);    // 后续的remotectrl指令
				total_len += len-bfa;
				ConvertMemcpy(tempdata, (uchar *)&total_len, 2);   // CPL
//			crc16 = CRC16(&tempdata[2+chl], total_len-chl-2);
//			ConvertMemcpy(tempdata+4, &crc16, 2); // crc16

				ser_printf("total_len=%d\r\n", total_len);

				sub_total_len = total_len;

				/* 外层数据 */
				total_len = 2;  // CPL_2
				chl = 4;

				BusinessDataBuf[2] = chl;   // 外层HL
				BusinessDataBuf[3] = 0x20;   // 外层SPI=0

				total_len += chl;

				request_data[0] = RCMD_Forward;   // 指令码
				ConvertMemcpy(&request_data[1], (uchar *)&seq, 4);

				total_len += TLVEncode(TAG_M_request, 5, request_data, 0, &BusinessDataBuf[total_len]);    // request TAG
				total_len += TLVEncode(TAG_D_code, DevCodeSize, (uchar *)&D_code, 1, &BusinessDataBuf[total_len]);   // D_code_tag
				request_data[0] = 0;
				total_len += TLVEncode(TAG_M_secureHead, 1, request_data, 0, &BusinessDataBuf[total_len]);   // D_code_tag
				total_len += TLVEncode(TAG_M_secureData, sub_total_len, tempdata, 0, &BusinessDataBuf[total_len]);   // D_code_tag

				crc16 = CRC16(BusinessDataBuf+6, total_len-6);
				ConvertMemcpy(BusinessDataBuf+4, &crc16, 2);
				ConvertMemcpy(BusinessDataBuf, (uchar *)&total_len, 2);    // 外层cpl

				LcCmdForward(D_code, seq, BusinessDataBuf, total_len);
				sysfree(BusinessDataBuf);
				/* 对门锁做操作, 直接返回 */
				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << RCMD_RemoteControl - Response, Sequence=%ld. state:%d, BUSINESS_STATE_OK.\r\n", seq, BUSINESS_STATE_OK);
				}

				BusinessPackage(RCMD_RemoteControl+0x80, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);

				//break;
			}
		}
		break;

		default:
			CommFlag = 1;
			break;
		}
	}

	if (CommFlag) {
		switch (cmd) {
		case RCMD_TimeSynchronize: {
			tlv.NextTag = data+bfa;
			tlv = TLVDecode1(tlv.NextTag, &HaveProcessLen);
			if (tlv.tag == TAG_M_time) {
				g_TransmitControl.TimeSync.LastWMCSyncTime = BooTime();
				ConvertMemcpy((uchar *)&stime, tlv.vp, 4);
				system("hwclock -s");
				unsigned long NowT = time(NULL);

				if (NowT+3<=stime || NowT>=stime+3) {
					// 相差3秒以上校时修改
					Log("BIZ", LOG_LV_DEBUG, "SetTime:%lu.", stime);

					SetRtcTime(stime);
					uint t =GetRtcTime();
					sp_printf("########################################################\r\n");
					sp_printf("t=0x%08lX\r\n", t);
					sp_printf("########################################################\r\n");
	//            OS_CPU_SR cpu_sr;

					g_TransmitControl.TimeSync.bLcokWaitReceive = 1;
					//g_TransmitControl.TimeSync.bWMCTimeReceived = 1;
					g_TransmitControl.TimeSync.LastWMCSyncRequestTime = t;

					//门锁校时
					LcCmdSetTime();
				} else {
					Log("BIZ", LOG_LV_DEBUG, "DevTime is ok, DevT:%lu, SetT:%lu.", (unsigned long)NowT, stime);
				}
			}
		}
		break;

		case RCMD_UploadRecord: // 对UploadRecord的回复指令
			FrDbDel(seq);
		case RCMD_OnlineNotify:
		case RCMD_UploadStatus: // 对UploadStatus的回复指令
			break;

		case RCMD_DownloadFile: {//forward the seq and context to download thread;
			Log("BIZ", LOG_LV_DEBUG, "RCMD_DownloadFile Response, seq=%lu, start to analyse", seq);

			ULONG crc32;
			typUpgradeData g_UpgradeData;

			g_UpgradeData.offset = 0xFFFFFFFF;      // 现将偏移置为全F用于标识是否有offset字段（旧版本无该字段）

			tlv.NextTag = data+bfa;
			HaveProcessLen = 0;
			while (HaveProcessLen < TagTotalLen) {
				TLVDecode(&tlv, &HaveProcessLen);

				//各字段的解析
				if (tlv.tag == TAG_M_offset) {  // 4
					ConvertMemcpy((uchar *)&g_UpgradeData.offset, tlv.vp, 4);
					Log("BIZ", LOG_LV_DEBUG, "Offset=%lu.", g_UpgradeData.offset);
				} else if (tlv.tag == TAG_M_content) {  // x
					g_UpgradeData.len = tlv.vlen;
					memcpy((uchar *)&g_UpgradeData.content[0], (uchar *)tlv.vp, tlv.vlen);
					Log("BIZ", LOG_LV_DEBUG, "vl=%hu.", g_UpgradeData.len);
				} else if (tlv.tag == TAG_M_crc32) {   // 4
					ConvertMemcpy((uchar *)&g_UpgradeData.crc32, tlv.vp, 4);
					Log("BIZ", LOG_LV_DEBUG, "crc32=%u.", g_UpgradeData.crc32);
				} else {
					Log("BIZ", LOG_LV_DEBUG, "RCMD_DownloadFile, unknow tag:%hhu.", tlv.tag);
				}
			}

			// 计算一下CRC32，等下与传递值进行比较
			crc32 = CRC32(g_UpgradeData.content, g_UpgradeData.len);

			Log("BIZ", LOG_LV_DEBUG, "RCMD_DownloadFile Response, seq=%lu, data picked, g_flagdownloading=%d, Notify offset=%lu, cCRC=%lu",
				seq, g_flagdownloading, g_UpgradeNotify[g_flagdownloading].offset, crc32);
			// 下载分包无偏移量(老协议)或者有偏移量且与本地下载偏移量相符(新协议)
			if ((g_UpgradeData.offset == 0xFFFFFFFF || g_UpgradeData.offset == g_UpgradeNotify[g_flagdownloading].offset)) {
				if (crc32 == g_UpgradeData.crc32) { // I.分片内容校验正确block crc32 check ok
					if (g_UpgradeNotify[g_flagdownloading].offset == 0) { // has download the first block
						ulong identify;

						ConvertMemcpy((uchar *)&identify, (uchar *)&g_UpgradeData.content[0x28], 4);
						if (identify != 0x5555AAAA) {// not 0x5555AAAA, the Upgrade flag is incorrect, quit downloading
							g_UpgradeNotify[g_flagdownloading].blocktimes++;
							sp_printf("source file error, blocktimes=%d\r\n", g_UpgradeNotify[g_flagdownloading].blocktimes);
							if (g_UpgradeNotify[g_flagdownloading].blocktimes >= 3) {// 对于第一包数据包头标记不正确的，最多下载3次，如一直校验不对则报错退出
								sp_printf("source file error, blocktimes=%d, redownload has beyond the max times.\r\n", g_UpgradeNotify[g_flagdownloading].blocktimes);
								memset((char*)&g_UpgradeNotify[g_flagdownloading], 0, sizeof(g_UpgradeNotify[g_flagdownloading]));
								//g_TransmitControl.bUpgradeNotify = 0;
								g_UpgradeNotify[g_flagdownloading].bUpgradeNotify = 0;
							}
							// 单包接收成功标志位置位，set the active, start to download the file again
							g_UpgradeNotify[g_flagdownloading].active = 1;
							UpgradeFileReset();

							//BusinessPackage(RCMD_DownloadFile_Response, BUSINESS_STATE_UNKNOWN_ERROR, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
//                        OS_CPU_SR cpu_sr;

							StoreSystemPara();
							//StoreSystemPara();

							break;
						} else { // 0x5555AAAA, it's the correct download file, then erase the loading flag(0x5555AAAA)
							g_UpgradeNotify[g_flagdownloading].blocktimes = 0;
							if ((strncmp(g_UpgradeNotify[g_flagdownloading].filename, "ctrl", 4)==0 &&
									g_UpgradeData.content[14]==g_WMCFirmware[1]) ||
									strncmp(g_UpgradeNotify[g_flagdownloading].filename, "lock", 4)==0) {
								// ctrl*** it's the wmc's upgrade file, and the hardware version fits. -add on 12/11 2012
								// 这里是一个标记位，完整以后，会将此处写回5555AAAA，否则重启等情况会被认为完整包
							} else { // hardware ID is incorrect, quit downloading
								if (g_BusinessDataDiagnose) {
									echo_printf("[DEBUG] Server -  Download File. ERROR, wrong source file or the hardware does not fit.\r\n");
								} else {
									sp_printf("Download File. ERROR, wrong source file or the hardware does not fit\r\n");
								}

								memset((char*)&g_UpgradeNotify[g_flagdownloading], 0, sizeof(g_UpgradeNotify[g_flagdownloading]));
								//g_TransmitControl.bUpgradeNotify = 0;
								g_UpgradeNotify[g_flagdownloading].bUpgradeNotify = 0;
								// set the active, start to download the file again
								g_UpgradeNotify[g_flagdownloading].active = 1;
								UpgradeFileReset();

								//BusinessPackage(RCMD_DownloadFile_Response, BUSINESS_STATE_UNKNOWN_ERROR, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);

//                            OS_CPU_SR cpu_sr;

								StoreSystemPara();
								//StoreSystemPara();
								break;
							}
						}
					}

					//sp_printf("UpdataFileWrite, add:0x%08X, offset:%d, len:%d.\r\n", g_uptempadd[g_flagdownloading]+g_UpgradeNotify[g_flagdownloading].offset, g_UpgradeNotify[g_flagdownloading].offset, g_UpgradeData.len);
					// 2.分包内容存储
					UpgradeFileWrite(g_flagdownloading, g_UpgradeNotify[g_flagdownloading].offset, g_UpgradeData.content, g_UpgradeData.len);
					/* 偏移量改变*/
					g_UpgradeNotify[g_flagdownloading].offset += g_UpgradeData.len;

					Log("BIZ", LOG_LV_DEBUG, "Download size=%lu, total size=%lu.", g_UpgradeNotify[g_flagdownloading].offset, g_UpgradeNotify[g_flagdownloading].size);

					// 3.download has finished
					if (g_UpgradeNotify[g_flagdownloading].offset >= g_UpgradeNotify[g_flagdownloading].size) {
						// download has finished
						if (UpgradeFileCheck(g_flagdownloading, g_UpgradeNotify[g_flagdownloading].size, g_UpgradeNotify[g_flagdownloading].crc32) == 0) {
							// file crc32 check ok, stop downloading and reboot the WMC device or download to lock.
							// a.file crc32 check ok, stop downloading and reboot the WMC device or download to lock.

							if (g_BusinessDataDiagnose) {
								echo_printf("[DEBUG] Server -  Download File. file CRC32 check OK. receive file name:%s\r\n", g_UpgradeNotify[g_flagdownloading].filename);
//                            debug_printf("");
							} else {
								sp_printf("Download File. file CRC32 check OK. receive file name:%s\r\n", g_UpgradeNotify[g_flagdownloading].filename);
								sp_printf("download file finished...\r\n");
								sp_printf("11111111111111111111\r\n");
								//sp_printf("receive file name:%s\r\n", g_UpgradeNotify[g_flagdownloading].filename);
							}

							g_UpgradeNotify[g_flagdownloading].downloadsuccess = 0;
							if (bufstr((uchar *)g_UpgradeNotify[g_flagdownloading].filename, "lock", FILENAME_LEN)>=0 && g_flagdownloading==FLAG_UPGRADE_LOCK) {
								// a.1 copy lock update file
								if (g_UpgradeNotify[g_flagdownloading].size < MAX_LOCK_UPGRADE_FILE_SIZE) {// size < 0x11800(old pcb) or 0x19000(new pcb)
									sp_printf("lock file size is correct and ready to deliver update notify to lock\r\n");
									//send to let lock upgrade
									g_UpgradeNotify[g_flagdownloading].downloadsuccess = 1;
									LcCmdUpdateNotify();
								} else {// size > 0x11800(old pcb) or 0x19000(new pcb),
									sp_printf("lock file size is larger\r\n");
								}
							} else if (bufstr((uchar *)g_UpgradeNotify[g_flagdownloading].filename, "ctrl", FILENAME_LEN)>=0 && g_flagdownloading==FLAG_UPGRADE_CTRL) {
								// a.2 copy ctrl update file
								ser_printf("g_bResetWMC4...\r\n");
								g_bResetWMC = 1;  //wmc update file to reboot
								g_UpgradeNotify[g_flagdownloading].downloadsuccess = 1;
								/* b_update.bin文件生成*/
								F_check_ctrl_upgrade_file_whether_copy();
							} else {
								g_UpgradeNotify[g_flagdownloading].filename[FILENAME_LEN-1] ='\0';
								Log("BIZ", LOG_LV_DEBUG, "Wrong filename:%s.", g_UpgradeNotify[g_flagdownloading].filename);
							}
							g_UpgradeNotify[g_flagdownloading].bUpgradeNotify = 0;
							//UpgradeFileReset();
							//OS_CPU_SR cpu_sr;
							//OS_ENTER_CRITICAL();
							//g_TransmitControl.bUpgradeNotify = 0;
							//OS_EXIT_CRITICAL();
							//while(1);
						} else {	// b.check failed, redownload the upgrade file(整个升级文件)
							/* 重新启动下载，从第一包分包开始下载*/
							g_UpgradeNotify[g_flagdownloading].offset = 0;			// reset the downlod file offset
							g_UpgradeNotify[g_flagdownloading].downloadsuccess = 0;	// clear download success flag
							g_UpgradeNotify[g_flagdownloading].blocktimes = 0;	// clear blocktimes flag
							g_UpgradeNotify[g_flagdownloading].uptimes++;
							// first time to download the file, redownlode the file
							if (g_UpgradeNotify[g_flagdownloading].uptimes < MAX_UPFILE_RESEND_TIMES) {
								g_UpgradeNotify[g_flagdownloading].bUpgradeNotify = 1;
							} else {	// has download file more than 1 time, quit downloading
								g_UpgradeNotify[g_flagdownloading].bUpgradeNotify = 0;
								//UpgradeFileReset();
							}

							sp_printf("22222222222222222222\r\n");
							//while(1);
							//OS_CPU_SR cpu_sr;
							//OS_ENTER_CRITICAL();
							//g_TransmitControl.bUpgradeNotify = 1;
							//OS_EXIT_CRITICAL();
						}
					}
				} else { // block crc32 check failed, redownload the file block
					// II.分片内容校验不正确，重新下载该分包
					g_UpgradeNotify[g_flagdownloading].blocktimes++;

					Log("BIZ", LOG_LV_DEBUG, "CRC check failed, Cnt=%d", g_UpgradeNotify[g_flagdownloading].blocktimes);
					//BusinessPackage(RCMD_DownloadFile_Response, BUSINESS_STATE_ERROR_DOWNLOADFILE_CRCERROR, seq, NULL, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
					if (g_UpgradeNotify[g_flagdownloading].blocktimes >= MAX_REDOWNLOAD_BLOCK_TIMES) {// redownload has past the max times, quit downloading
						g_UpgradeNotify[g_flagdownloading].bUpgradeNotify = 0;
					}
				}
				/* 只有收到符合偏移量的分包数据, 才认为收到了有效分包 */
				g_UpgradeNotify[g_flagdownloading].active = 1;	// set the active, start to download the file again
			} else {	// offset值校验失败
				// TODO 重发？
			}

			UpgradeFileReset();

//        OS_CPU_SR cpu_sr;

			StoreSystemPara();
		}
		break;

		case RCMD_Data_Changed_Notify: {
			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> IMSI_Changed_Notify - Response, Sequence=%ld.\r\n", seq);
			}

			if (g_SystermInfo.NewImsi[0]) {
//			g_SystermInfo.bImsiChangedRequestSend = 0;
				strcpy((char *)g_SystermInfo.StoreImsi, (char *)g_SystermInfo.NewImsi);
				memset(g_SystermInfo.NewImsi, 0, 16);
//            OS_CPU_SR cpu_sr;

				StoreSystemPara();
				//StoreSystemPara();
				sp_printf("change ims notify success[%d]\r\n", (int)CmdStat);
			}
		}
		break;

		case RCMD_QueryStatus: {
			len = 0;

			tlv.NextTag = data+bfa;
			TLVDecode(&tlv, &HaveProcessLen);
			if (tlv.tag==TAG_D_code && tlv.vlen>=2 && tlv.vlen<=4) {
				ConvertMemcpy(&LogicCode, tlv.vp, tlv.vlen);
				Log("BIZ", LOG_LV_DEBUG, "RCMD_QueryStatus, seq=%lu, c=%hu", seq, LogicCode);

				uint32_t CodeAll = ~0;
				if (memcmp(&CodeAll, tlv.vp, tlv.vlen) == 0) { //query all deveice
					for ( i = 0; i < MAX_SUBDEVICE/*g_DeviceSetup.count*/; i++) {
						if (g_DeviceSetup.Device[i].Status) {
							if (g_DeviceSetup.Device[i].type == DEVICE_TYPE_LOCK) {
								len += QueryDevieceStatusLock(&ltagBuf[len], g_DeviceSetup.Device[i].LogicCode);
							} else if (g_DeviceSetup.Device[i].type == DEVICE_TYPE_WMC) {
								len += QueryDevieceStatusWMC(&ltagBuf[len], g_DeviceSetup.Device[i].LogicCode);
//						} else {
//							len += QueryDevieceStatusOther(&ltagBuf[len], g_DeviceSetup.Device[i].LogicCode);
							}
						}
					}
				} else if(0 == LogicCode) {	// query the wmc status
					//g_WMCFlagStatus |= (MASK_WMCBATTERY | MASK_WMCGSMSIGNAL);
//				g_WMCFlagStatus |= (ulong)(MASK_WMCWORKSTATUS | MASK_WMCVERSION | MASK_WMCFIRMWARE | MASK_WMCBATTERY | MASK_WMCGSMSIGNAL);

					len += QueryDevieceStatusWMC(&ltagBuf[len], 0);
//			} else if(2100 == LogicCode || 2101 == LogicCode) {
//				// TODO:调用实时视频接口
//				// realstream_start_or_stop_opr(_REALSTREAM_START_OPR, LogicCode);
				} else {
					// 不需要直接去与门锁通讯
					len = QueryDevieceStatusLock(ltagBuf, LogicCode);

					if (len <= 0) {
						BusinessPackage(RCMD_QueryStatus+0x80, BUSINESS_STATE_ERROR_UNKNOWN_DEVICE, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
						break;
					}
				}
				BusinessPackage(RCMD_QueryStatus+0x80, BUSINESS_STATE_OK, seq, ltagBuf, len, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			} else {
				// TODO 回复错误
			}
		}
		break;

		case RCMD_UpgradeNotify: {
			ulong filelen = 0;
			ulong flagupcmd = 0;
			typUpgradeNotify tmp_UpgradeNotify;

			sp_printf("RCMD_UpgradeNotify...\r\n");

			memset((char *)&tmp_UpgradeNotify, 0, sizeof(tmp_UpgradeNotify));

			tlv.NextTag = &data[bfa];
			HaveProcessLen = 0;
			while (HaveProcessLen < TagTotalLen) {
				//				tlv = TLVDecode1(tlv.NextTag, &HaveProcessLen);
				TLVDecode(&tlv, &HaveProcessLen);
				if (tlv.tag==TAG_D_code && tlv.vlen>=2 && tlv.vlen<=4) {
					ConvertMemcpy((uchar *)&tmp_UpgradeNotify.LogicCode, tlv.vp, tlv.vlen);
					sp_printf("tmp_UpgradeNotify.LogicCode=%d\r\n", tmp_UpgradeNotify.LogicCode);

					flagupcmd |= 1<<0;
				} else if (tlv.tag == TAG_M_file) {
					sp_printf("RCMD_UpgradeNotify, filename: before=%s, len=%d, now=%s, len=%d, vlen=%d\r\n", tmp_UpgradeNotify.filename, strlen(tmp_UpgradeNotify.filename), (char *)tlv.vp, strlen((char *)tlv.vp), tlv.vlen);
					filelen = tlv.vlen<FILENAME_LEN?tlv.vlen:(FILENAME_LEN-1);
					memcpy(tmp_UpgradeNotify.filename, (char *)tlv.vp, filelen);

					flagupcmd |= 1<<1;
				} else if (tlv.tag == TAG_M_version) {
					memcpy(tmp_UpgradeNotify.ver, tlv.vp, 2);
					flagupcmd |= 1<<2;
				} else if (tlv.tag == TAG_M_size) {
					ConvertMemcpy((uchar *)&(tmp_UpgradeNotify.size), tlv.vp, 4);
					sp_printf("tmp_UpgradeNotify.size=%d, 0x%08X\r\n", tmp_UpgradeNotify.size, tmp_UpgradeNotify.size);
					flagupcmd |= 1<<3;
				} else if (tlv.tag == TAG_M_crc32) {
					ConvertMemcpy((uchar *)&(tmp_UpgradeNotify.crc32), tlv.vp, 4);
					sp_printf("tmp_UpgradeNotify.crc32=0x%08X\r\n", tmp_UpgradeNotify.crc32);
					flagupcmd |= 1<<4;
				}
			}

			// 调试输出
			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> UpgradeNotify - Request, Sequence=%ld", seq);
				if (flagupcmd & (1<<0)) { // LogicCode
					echo_printf(". LogicCode:%u", tmp_UpgradeNotify.LogicCode);
				} else {
					echo_printf(". LogicCode:NULL");
				}
				if (flagupcmd & (1<<1)) { // filename
					echo_printf(", filename:%s", tmp_UpgradeNotify.filename);
				} else {
					echo_printf(", filename:NULL");
				}
				if (flagupcmd & (1<<2)) { // ver
					echo_printf(", ver:%d.%d", tmp_UpgradeNotify.ver[0], tmp_UpgradeNotify.ver[1]);
				} else {
					echo_printf(", ver:NULL");
				}
				if (flagupcmd & (1<<3)) { // size
					echo_printf(", size:%ldB", tmp_UpgradeNotify.size);
				} else {
					echo_printf(", size:NULL");
				}
				if (flagupcmd & (1<<4)) { // crc32
					echo_printf(", crc32:0x%08lX", tmp_UpgradeNotify.crc32);
				} else {
					echo_printf(", crc32:NULL");
				}
				echo_printf(".\r\n");
			}
			if ((flagupcmd&0xF) != 0xF) {	// 软件升级指令参数接收不完全，返回错误
				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << UpgradeNotify - Response, Sequence=%ld. status:%d, BUSINESS_STATE_ERROR_UPNOTIFY_ARGMISSING .\r\n", seq, BUSINESS_STATE_ERROR_UPNOTIFY_ARGMISSING);
				}
				BusinessPackage(RCMD_UpgradeNotify+0x80, BUSINESS_STATE_ERROR_UPNOTIFY_ARGMISSING, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
				break;
			}

			if (filelen) {
				uchar flagUpgrade = FLAG_UPGRADE_INIT;	// the flag whether the upgrade file is wmc device or lock, 0-init, 1-ctrl, 2-lock

				if (strncmp(tmp_UpgradeNotify.filename, "ctrl", 4) == 0) {
					sp_printf("RCMD_UpgradeNotify... it's ctrl's upgrade file\r\n");
					flagUpgrade = FLAG_UPGRADE_CTRL;
				} else if (strncmp(tmp_UpgradeNotify.filename, "lock", 4) == 0) {
					sp_printf("RCMD_UpgradeNotify... it's lock's upgrade file\r\n");
					flagUpgrade = FLAG_UPGRADE_LOCK;
				} else {
					flagUpgrade = FLAG_UPGRADE_INIT;
					if (g_BusinessDataDiagnose) {
						echo_printf("[DEBUG] Server << UpgradeNotify - Response, Sequence=%ld. status:%d, BUSINESS_STATE_ERROR_UPNOTIFY_INCORRECTNAME .\r\n", seq, BUSINESS_STATE_ERROR_UPNOTIFY_INCORRECTNAME);
					} else {
						sp_printf("RCMD_UpgradeNotify... BUSINESS_STATE_ERROR_UPNOTIFY_INCORRECTNAME\r\n");
					}
					BusinessPackage(RCMD_UpgradeNotify+0x80, BUSINESS_STATE_ERROR_UPNOTIFY_INCORRECTNAME, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
					break;
				}
				sp_printf("flagUpgrade=%d\r\n", flagUpgrade);

				//文件大小合理判断
				if ((FLAG_UPGRADE_LOCK==flagUpgrade && tmp_UpgradeNotify.size>MAX_LOCK_UPGRADE_FILE_SIZE) ||
						(FLAG_UPGRADE_CTRL==flagUpgrade && tmp_UpgradeNotify.size>MAX_CTRL_UPGRADE_FILE_SIZE)) {
					// 长度不合理
					if (g_BusinessDataDiagnose) {
						echo_printf("[DEBUG] Server << UpgradeNotify - Response, Sequence=%ld. status:%d, BUSINESS_STATE_ERROR_DOWNLOAD_SIZELONG .\r\n", seq, BUSINESS_STATE_ERROR_DOWNLOAD_SIZELONG);
					} else {
						sp_printf("RCMD_UpgradeNotify... BUSINESS_STATE_ERROR_DOWNLOAD_SIZELONG\r\n");
					}
					BusinessPackage(RCMD_UpgradeNotify+0x80, BUSINESS_STATE_ERROR_DOWNLOAD_SIZELONG, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
					break;
				}
				// 已下载成功否检测
				if (strncmp(g_UpgradeNotify[flagUpgrade].filename, tmp_UpgradeNotify.filename, filelen)==0
						&& memcmp(g_UpgradeNotify[flagUpgrade].ver, tmp_UpgradeNotify.ver, 2)==0
						&& g_UpgradeNotify[flagUpgrade].size==tmp_UpgradeNotify.size
						&& g_UpgradeNotify[flagUpgrade].crc32==tmp_UpgradeNotify.crc32
						&& UpgradeFileCheck(flagUpgrade, tmp_UpgradeNotify.size, tmp_UpgradeNotify.crc32)==0) {
					// 远程升级指令中文件信息与主控器中信息相同，避免重复下载，尤其同时升级多个门锁时
					// TODO 升级包对象id的更新？
					sp_printf("RCMD_UpgradeNotify0, filename is the same. g_flagdownloading=%d, flagUpgrade=%d, \r\ng_UpgradeNotify[flagUpgrade].filename=%s, tmp_UpgradeNotify.filename=%s\r\n", g_flagdownloading, flagUpgrade, g_UpgradeNotify[flagUpgrade].filename, tmp_UpgradeNotify.filename);
					g_UpgradeNotify[flagUpgrade].downloadsuccess = 1;
					if (g_UpgradeNotify[flagUpgrade].bUpgradeNotify) {
						g_UpgradeNotify[flagUpgrade].bUpgradeNotify = 0;	// 如原本已成功，则将升级标志清零
					}
					UpgradeFileReset();
					if (FLAG_UPGRADE_LOCK == flagUpgrade) {	// 如果是锁的升级通知则重新给门锁传一遍升级文件
						LcCmdUpdateNotify();	 //send to let lock upgrade
					} else if (FLAG_UPGRADE_CTRL == flagUpgrade) {
						g_bResetWMC = 1;  //wmc update file to reboot
						// 再次检查，准备最终的系统升级
						F_check_ctrl_upgrade_file_whether_copy();
					}
				} else {
					// 远程升级通知中的文件名与原有文件名不同，则重新开始下载
					sp_printf("RCMD_UpgradeNotify7, filename is different. g_flagdownloading=%d, flagUpgrade=%d, g_UpgradeNotify[flagUpgrade].filename=%s, tmp_UpgradeNotify.filename=%s\r\n", g_flagdownloading, flagUpgrade, g_UpgradeNotify[flagUpgrade].filename, tmp_UpgradeNotify.filename);
					memset((uchar *)&g_UpgradeNotify[flagUpgrade], 0, sizeof(g_UpgradeNotify[flagUpgrade]));
					g_UpgradeNotify[flagUpgrade].offset = 0;
					g_UpgradeNotify[flagUpgrade].downloadsuccess = 0;
					g_UpgradeNotify[flagUpgrade].uptimes = 0;
					g_UpgradeNotify[flagUpgrade].blocktimes = 0;
					g_UpgradeNotify[flagUpgrade].bUpgradeNotify = 1;
//				g_UpgradeNotify[flagUpgrade].upfile_crc32 = 0;
					memcpy((char *)&g_UpgradeNotify[flagUpgrade].filename[0], tmp_UpgradeNotify.filename, filelen);
					g_UpgradeNotify[flagUpgrade].filename[filelen] = '\0';
					memcpy(g_UpgradeNotify[flagUpgrade].ver, tmp_UpgradeNotify.ver, 2);
					g_UpgradeNotify[flagUpgrade].LogicCode = tmp_UpgradeNotify.LogicCode;
					g_UpgradeNotify[flagUpgrade].size = tmp_UpgradeNotify.size;
					g_UpgradeNotify[flagUpgrade].crc32 = tmp_UpgradeNotify.crc32;
//				if (g_flagdownloading == FLAG_UPGRADE_INIT || g_flagdownloading == flagUpgrade) {	// 没有下载或正在下载升级通知中的文件，立即开始下载
//					sp_printf("RCMD_UpgradeNotify8... g_flagdownloading=%d, flagUpgrade=%d\r\n", g_flagdownloading, flagUpgrade);
					g_UpgradeNotify[flagUpgrade].active = 1;
//				}
					UpgradeFileReset();
				}
				StoreSystemPara();
			}
			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server << UpgradeNotify - Response, sequence=%ld. status:%d, BUSINESS_STATE_OK .\r\n", seq, BUSINESS_STATE_OK);
			}
			BusinessPackage(RCMD_UpgradeNotify+0x80, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
		}
		break;

		case RCMD_SetupDevice: {	// TODO 修改服务器IP和端口，暂不支持
			//uchar ltagBuf[10];
			UCHAR operate = 0;
			int posi=-1;
			int HaveProcessLen1, HaveProcessLen2;
			//int HaveProcessLen3;
			typTLV tlv1, tlv2;

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> Setup Device - Request, Sequence=%ld", seq);
			} else {
				sp_printf("RCMD_SetupDevice...\r\n");
				//			sp_printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
			}

			tlv1.NextTag = &data[bfa];
			HaveProcessLen1 = 0;
			sp_printf("start to process setup device decode\r\n");
			while (HaveProcessLen1 < TagTotalLen) {		//the first layer tag
				TLVDecode(&tlv1, &HaveProcessLen1);
				if (tlv1.tag == TAG_D_setup) {
					//大包标签
					if (g_BusinessDataDiagnose) {
						echo_printf(". D_setup");
					}

					tlv2.NextTag = tlv1.vp;
					HaveProcessLen2 = 0;
					while (HaveProcessLen2 < tlv1.vlen) {	//the second layer tag
						TLVDecode(&tlv2, &HaveProcessLen2);
						sp_printf("HaveProcessLen2=%d, vlen=%d\r\n", HaveProcessLen2, tlv1.vlen);

						if (tlv2.tag==TAG_D_code && tlv2.vlen>=2 && tlv2.vlen<=4) {
							uint32_t D_code = 0;   // V3中D_code扩展成4位

							ConvertMemcpy(&D_code, tlv2.vp, tlv2.vlen);

							if (g_BusinessDataDiagnose) {
								echo_printf(", D_code:%d", D_code);
							}

							TLVDecode(&tlv2, &HaveProcessLen2);
							if (tlv2.tag == TAG_M_operate) {
								operate = *(tlv2.vp);
								if (g_BusinessDataDiagnose) {
									echo_printf(", M_operate:%d", operate);
								} else {
									sp_printf("M_operate=%d\r\n", operate);
								}

								if (operate==0 || operate==1 || operate==3) {
									ModifyDeviceList(D_code, &tlv2, &HaveProcessLen2, tlv1.vlen, operate, &posi);
								}
							}
						}

						sp_printf("End...  HaveProcessLen2=%d, vlen=%d\r\n", HaveProcessLen2, tlv1.vlen);
					}
				}
			}

			if (g_BusinessDataDiagnose) {
				echo_printf(".\r\n");
			}

			StoreSystemPara();
			//StoreSystemPara();

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server << Setup Device - Response, Sequence=%ld. state:%d, BUSINESS_STATE_OK.\r\n", seq, BUSINESS_STATE_OK);
			}

			BusinessPackage(RCMD_SetupDevice+0x80, BUSINESS_STATE_OK, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			if (0 == operate) { // 纯添加设备, 只发送主控器上线信息
				SendOnLineNotify(2);
			} else { // 删除或清空, 额外发送门锁脱机信息
				SendOnLineNotify(0);
			}

			/*OSTimeDly(2000);
			sp_printf("posi=%d\r\n", posi);
			if(posi != -1)
			{
				len =QueryDevieceStatusLock(&ltagBuf[0], g_DeviceSetup.Device[posi].LogicCode);
				BusinessPackage(RCMD_UploadStatus, BUSINESS_STATE_OK, seq, ltagBuf, len, NULL, 0, "", SOURCE_TYPE_ZKQ);
			}*/
		}
		break;

		case RCMD_SetupKey: {
			uchar TransmitKey[M_KEY_LEN], DEKKey[M_KEY_LEN];
			uchar FlagTrans, FlagDEK, CheckTrans, CheckDEK;
			int FlagCheck;
			int HaveProcessLen1, HaveProcessLen2;
			typTLV tlv1, tlv2;

			g_TransmitKey.count = 0;

			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> Setup Key - Request, Sequence=%ld", seq);
			} else {
				sp_printf("RCMD_SetupKey....\n");
			}

			FlagTrans = FlagDEK = FlagCheck = CheckDEK = CheckTrans = 0;
			//KeyLen = -1;
			tlv1.NextTag = &data[bfa];
			HaveProcessLen1 = 0;
			while (HaveProcessLen1 < TagTotalLen) {            //the first layer tag
//				    tlv1 = TLVDecode1(tlv1.NextTag, &HaveProcessLen1);
				TLVDecode(&tlv1, &HaveProcessLen1);
				if (tlv1.tag == TAG_M_key) {

					tlv2.NextTag = tlv1.vp;
					HaveProcessLen2 = 0;
					while (HaveProcessLen2 < tlv1.vlen) {	//the second layer tag
//					           tlv2 = TLVDecode1(tlv2.NextTag, &HaveProcessLen2);
						TLVDecode(&tlv2, &HaveProcessLen2);
						if (tlv2.tag == TAG_M_keyNo) {  // 1
							if (0x01 == *tlv2.vp) {	//Transmit Key
								FlagTrans = 1;
								CheckTrans = 1;
							} else if (0x02 == *tlv2.vp) {	//DEK Key
								FlagDEK = 1;
								CheckDEK = 1;
							} else {
								FlagCheck = -1;
								break;
							}
							g_TransmitKey.key[g_TransmitKey.count].No = *tlv2.vp;

							if (g_BusinessDataDiagnose) {
								echo_printf(", M_keyNo:%d", g_TransmitKey.key[g_TransmitKey.count].No);
							}
						} else if (tlv2.tag == TAG_M_keyIndex) { // 1
							if (0x00 != *tlv2.vp) {
								FlagCheck = -2;
								break;
							}
							g_TransmitKey.key[g_TransmitKey.count].Index = *tlv2.vp;

							if (g_BusinessDataDiagnose) {
								echo_printf(", M_keyIndex:%d", g_TransmitKey.key[g_TransmitKey.count].Index);
							}
						} else if (tlv2.tag == TAG_M_keyData) { // 16
							if (g_BusinessDataDiagnose) {
								echo_printf(", M_keyData:");
								for (int i = 0; i<M_KEY_LEN; i++) {
									echo_printf("%X", *(tlv2.vp+i));
								}
							}

							if (FlagTrans) {
								memcpy(TransmitKey, tlv2.vp, M_KEY_LEN);
								DesCbc2keyDecode(TransmitKey, M_KEY_LEN, g_SystermInfo.DEKKey, cveci);
								//PrintBuff("TransmitKey1=", TransmitKey, M_KEY_LEN);
							} else if (FlagDEK) {
								memcpy(DEKKey, tlv2.vp, M_KEY_LEN);
								DesCbc2keyDecode(DEKKey, M_KEY_LEN, g_SystermInfo.DEKKey, cveci);
								//PrintBuff("DEKKey1=", DEKKey, M_KEY_LEN);
							}
							memcpy((uchar *)&(g_TransmitKey.key[g_TransmitKey.count].Data[0]), tlv2.vp, M_KEY_LEN);
							//memcpy(&g_SystermInfo.ManagePassword[0], tlv2.vp, M_KEY_LEN);
						} else if (tlv2.tag == TAG_M_KCV) { // 3
							if (g_BusinessDataDiagnose) {
								echo_printf(", M_KCV:");
								for (int i = 0; i<3; i++) {
									echo_printf("%X", *(tlv2.vp+i));
								}
							}

							if (FlagTrans) {
								uchar tmp[8];
								memset(tmp, 0x0, sizeof(tmp));
								//PrintBuff("tmp=", tmp, 8);
								//PrintBuff("TransmitKey2=", TransmitKey, M_KEY_LEN);
								ThDesCbc2keyEncode(tmp, 8, TransmitKey, cveci);
								//PrintBuff("FlagTrans...\ntmp=", tmp, 3);
								//PrintBuff("vp=", tlv2.vp, 3);
								if (memcmp(tmp, tlv2.vp, 3)==0) {
									FlagTrans=0;
									//CheckTrans=1;
								} else {
									FlagCheck = -3;
									break;
								}
							} else if (FlagDEK) {
								uchar tmp[8];
								memset(tmp, 0x0, sizeof(tmp));
								//PrintBuff("tmp=", tmp, 8);
								//PrintBuff("DEKKey2=", DEKKey, M_KEY_LEN);
								ThDesCbc2keyEncode(tmp, 8, DEKKey, cveci);
								//PrintBuff("FlagDEK...\ntmp=", tmp, 3);
								//PrintBuff("vp=", tlv2.vp, 3);
								if (memcmp(tmp, tlv2.vp, 3)==0) {
									FlagDEK=0;
									//CheckDEK = 1;
								} else {
									FlagCheck = -1;
									break;
								}
							}
							memcpy((uchar *)&(g_TransmitKey.key[g_TransmitKey.count].KCV[0]), tlv2.vp,3);
						}
					}
					if (0 != FlagCheck) {
						break;
					}
					g_TransmitKey.count++;
				}
			}

			if (g_BusinessDataDiagnose) {
				echo_printf(".\r\n");
			}

			//ModifySystemSet(SYSTEM_SET_FILE, g_SystermInfo.ManagePassword, OFFSET_TRANSKEY, 1);
			//ModifySystemSet(SYSTEM_SET_FILE, g_SystermInfo.DEKKey, OFFSET_DEKKEY, 1);

			if (0 == FlagCheck) {	// Check Success
				if (CheckTrans) {
					memcpy(g_SystermInfo.ManagePassword, TransmitKey, M_KEY_LEN);
					//ModifySystemSet(SYSTEM_SET_FILE, TransmitKey, OFFSET_TRANSKEY, 1);

					//PrintBuff("ManagePassword=", g_SystermInfo.ManagePassword, M_KEY_LEN);
					//BusinessPackage(RCMD_SetupKey_Response, BUSINESS_STATE_OK, seq, ltagBuf, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
				}
				if (CheckDEK) {
					memcpy(g_SystermInfo.DEKKey, DEKKey, M_KEY_LEN);
					//ModifySystemSet(SYSTEM_SET_FILE, DEKKey, OFFSET_DEKKEY, 1);

					//PrintBuff("DEKKey=", g_SystermInfo.DEKKey, M_KEY_LEN);
					//BusinessPackage(RCMD_SetupKey_Response, BUSINESS_STATE_OK, seq, ltagBuf, 0, NULL, 0, "", SOURCE_TYPE_ZKQ);
				}

				StoreSystemPara();
				//StoreSystemPara();

				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << Setup Key - Response, Sequence=%ld. Status:%d, BUSINESS_STATE_OK .\r\n", seq, BUSINESS_STATE_OK);
				}

				BusinessPackage(RCMD_SetupKey_Response, BUSINESS_STATE_OK, seq, ltagBuf, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			} else if (-1 == FlagCheck) {
				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << Setup Key - Response, Sequence=%ld. Status:%d, BUSINESS_STATE_ERROR_SETUPKEY_NO .\r\n", seq, BUSINESS_STATE_ERROR_SETUPKEY_NO);
				}
				BusinessPackage(RCMD_SetupKey_Response, BUSINESS_STATE_ERROR_SETUPKEY_NO, seq, ltagBuf, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			} else if (-2 == FlagCheck) {
				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << Setup Key - Response, Sequence=%ld. Status:%d, BUSINESS_STATE_ERROR_SETUPKEY_INDEX .\r\n", seq, BUSINESS_STATE_ERROR_SETUPKEY_INDEX);
				}
				BusinessPackage(RCMD_SetupKey_Response, BUSINESS_STATE_ERROR_SETUPKEY_INDEX, seq, ltagBuf, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			} else if (-3 == FlagCheck) {
				if (g_BusinessDataDiagnose) {
					echo_printf("[DEBUG] Server << Setup Key - Response, Sequence=%ld. Status:%d, BUSINESS_STATE_ERROR_SETUPKEY_KCV .\r\n", seq, BUSINESS_STATE_ERROR_SETUPKEY_KCV);
				}
				BusinessPackage(RCMD_SetupKey_Response, BUSINESS_STATE_ERROR_SETUPKEY_KCV, seq, ltagBuf, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			}
		}
		break;

		case RCMD_Forward: {
			uint32_t D_code = 0;
			int HaveProcessLen1 = 0;
			typTLV tlv1;

			tlv1.NextTag = data+bfa;
			HaveProcessLen1 = 0;
			while (HaveProcessLen1 < TagTotalLen) {            //the first layer tag
				TLVDecode(&tlv1, &HaveProcessLen1);
				if (tlv1.tag==TAG_D_code && tlv1.vlen>=2 && tlv1.vlen<=4) {
					memset(&D_code, 0, sizeof(D_code));
					ConvertMemcpy((uchar *)&D_code, tlv1.vp, tlv1.vlen);
					if (0 == D_code) {	// 发给主控器
						Log("BIZ", LOG_LV_DEBUG, "Forward to [%d] is not supported.", D_code);
					} else {
						// 发给门锁或其他子设备
						/* 门锁不联机, 则直接返回错误. 门锁已联机，则先转发给门锁，待门锁回复后再回复 */
						if (LcGetOnline(D_code) != 1) {
							if (g_BusinessDataDiagnose) {
								echo_printf("[DEBUG] Server << Forward - Response, Sequence=%ld. state:%d, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE.\r\n", seq, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE);
							}
							BusinessPackage(RCMD_Forward_Response, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
							break;
						}

						if (ProtVer/16 == 2) {
							/* 门锁联机, 则转发给门锁*/
							LcCmdForward(D_code, seq, data, len);

						} else {
							uint16_t tLen = len+6, crc;
							uint8_t *tData = (uint8_t*)malloc(tLen);

							ConvertMemcpy(tData, &tLen, 2);
							tData[2]	= 4;
							tData[3]	= 0x20;
							crc			= CRC16(data, len);
							ConvertMemcpy(tData+4, &crc, 2);
							memcpy(tData+6, data, len);
							LcCmdForward(D_code, seq, tData, tLen);
							free(tData);
						}

						break;
					}
				}
			}
		}
		break;

		case RCMD_ReadData: {
			int ltp = 0;
			uint8_t Tag=0;
			uint32_t DevCode=~0;

			tlv.NextTag = data+bfa;
			HaveProcessLen = 0;
			while (HaveProcessLen < TagTotalLen) {            //the first layer tag
				TLVDecode(&tlv, &HaveProcessLen);
				if (tlv.tag==TAG_D_code && tlv.vlen>=2 && tlv.vlen<=4) {
					memset(&DevCode, 0, sizeof(DevCode));
					ConvertMemcpy(&DevCode, tlv.vp, tlv.vlen);   // D_code扩展成4位
				} else if(tlv.tag == TAG_M_tag) {
					Tag = *(tlv.vp);
				}
			}

			if (0 == DevCode) {  // 读取主控器参数, 直接返回
				switch (Tag) {
				case TAG_M_IMSI:
					ltp = TLVEncode(TAG_M_IMSI, 15, g_SystermInfo.StoreImsi, 0, ltagBuf);
					break;
				case TAG_D_DEVICE_CUID:
				case TAG_M_LOCATION_ID:
				case TAG_M_feature:
				case TAG_D_deviceInfo:
				case TAG_L_allowedOpenMode:
				case TAG_M_vendor:
				case TAG_M_version:
				case TAG_D_firmware:
				default:
					break;
				}

				if (ltp > 0) {  // 参数存在
					BusinessPackage(RCMD_ReadData|0x80, BUSINESS_STATE_OK, seq, ltagBuf, ltp, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
				} else { // 参数不存在, 直接返回错误码
					BusinessPackage(RCMD_ReadData+0x80, BUSINESS_STATE_ERROR_NONE_TAG_DATA, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
				}
			} else { // 读取子设备数据(如门锁等)
				if (LcGetOnline(DevCode) != 1) { // 门锁不在线, 直接返回错误
					BusinessPackage(RCMD_ReadData|0x80, BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
				} else { // 门锁在线, 查询门锁
					ltp = TLVEncode(TAG_M_tag, 1, &Tag, 0, ltagBuf);

					LcCmdReadData(DevCode, seq, ltagBuf, ltp);	// 读取门锁数据
					break;
				}
			}
		}
		break;

		default:
			BusinessPackage(cmd|0x80, BUSINESS_STATE_ERROR_NOT_SUPPORTED_KEYWORD, seq, NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
			break;
		}
	}
	return 0;
}

int SMSDecode(uchar *data, int len)
{
	//uchar ltagBuf[256];
	typTLV tlv1, tlv2;
	int HaveProcessLen1, HaveProcessLen2;
	//ulong tempIP;
	//ushort  tempPort;
	int bIPChanged, bPortChanged;

	tlv1.NextTag = &data[0];
	HaveProcessLen1 = 0;
	bIPChanged = 0;
	bPortChanged = 0;

	sp_printf("start to sms decode\r\n");
	while (HaveProcessLen1 < len) {            //the first layer tag
		TLVDecode(&tlv1, &HaveProcessLen1);
		if (tlv1.tag == TAG_M_notify) {
			if (g_BusinessDataDiagnose) {
				echo_printf("[DEBUG] Server >> SMS - Notify. M_notify");
			}

			tlv2.NextTag = tlv1.vp;
			HaveProcessLen2 = 0;
			while (HaveProcessLen2 < tlv1.vlen) {	//the second layer tag
				TLVDecode(&tlv2, &HaveProcessLen2);
				sp_printf("HaveProcessLen2=%d, vlen=%d\r\n", HaveProcessLen2, tlv1.vlen);
				if (tlv2.tag == TAG_M_serverIP) {
					ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerIP), tlv2.vp, 4);
					if (g_BootPara.NetInfo.ip != g_tempServerInfo.ServerIP) {
						bIPChanged = 1;
					}

					if (g_BusinessDataDiagnose) {
						echo_printf(", M_serverIP:%ld.%ld.%ld.%ld", (g_tempServerInfo.ServerIP>>24)&0xff, (g_tempServerInfo.ServerIP>>16)&0xff,(g_tempServerInfo.ServerIP>>8)&0xff, g_tempServerInfo.ServerIP&0xff);
						//echo_printf(", M_serverIP=0x%08X", g_tempServerInfo.ServerIP);
					} else {
						sp_printf("in server ip is:0x%08lX\r\n", g_tempServerInfo.ServerIP);
					}
				} else if (tlv2.tag == TAG_M_serverPort) {
					ConvertMemcpy((uchar *)&(g_tempServerInfo.ServerPort), tlv2.vp, 2);
					if (g_BootPara.NetInfo.port != g_tempServerInfo.ServerPort) {
						bPortChanged = 1;
					}

					if (g_BusinessDataDiagnose) {
						echo_printf(", M_serverPort:%d", g_tempServerInfo.ServerPort);
					} else {
						sp_printf("in server port is:%d\r\n", g_tempServerInfo.ServerPort);
					}
				}
			}

			if (g_BusinessDataDiagnose) {
				echo_printf(".\r\n");
//                debug_printf("");
			}

			if (bIPChanged || bPortChanged) {
				g_bDoNewConnectTest = 1;
			}

			if (bIPChanged == 0 && bPortChanged == 0) {     // if changed first to do connect test
				sp_printf("sms to do connect request\r\n");
//                OS_CPU_SR cpu_sr;

				g_TransmitControl.bRequestConnect = 1;
			}
		}
	}
	return 0;
}

typTLV TLVDecode1(uchar *indata, int *HaveProcessLen)
{
	ushort  headlen;
	typTLV tlv;

	tlv.tag = indata[0];

	if (indata[1] == 0xff) {
		ConvertMemcpy((uchar *)&tlv.vlen, indata+2, 2);
		tlv.vp = indata+4;
		headlen = 4;
	} else {
		tlv.vlen = indata[1];
		tlv.vp = indata+2;
		headlen = 2;
	}

	tlv.NextTag = tlv.vp+tlv.vlen;
	*HaveProcessLen = *HaveProcessLen + headlen+tlv.vlen;

	return tlv;
}

void TLVDecode(typTLV *tlv, int *HaveProcessLen)
{
	uchar *indata = tlv->NextTag;
	ushort  headlen;
//	typTLV tlv;

	tlv->tag = indata[0];

	if (indata[1] == 0xff) {
		ConvertMemcpy((uchar *)&tlv->vlen, &indata[2], 2);
		tlv->vp = &indata[4];
		headlen = 4;
	} else {
		tlv->vlen = indata[1];
		tlv->vp = &indata[2];
		headlen = 2;
	}
	tlv->NextTag = tlv->vp+tlv->vlen;
	*HaveProcessLen = *HaveProcessLen + headlen + tlv->vlen;

	return;
}

//bConvert =0, not convert; bConvert = 1 want to convert
int TLVEncode(uchar tag, ushort len, const void *sdata, int bConvert, uchar * valdata)
{
	int packlen = 0;
	uchar *pValData = NULL;
	valdata[0] = tag;

	if (len < 254) {
		valdata[1] = len;
		pValData = &valdata[2];
		packlen = 2;
	} else {
		valdata[1] = 0xff;
		ConvertMemcpy(&valdata[2], (uchar *)&len, 2);
		pValData = &valdata[4];
		packlen = 4;
	}
	if (bConvert) {
		ConvertMemcpy(pValData, sdata, len);
	} else {
		memcpy(pValData, sdata, len);
	}

	return (packlen+len);
}

/*
     business layer data package ,and send transmit layer to send out the data
     sequence: 一定<0x80000000
*/
int BusinessPackage(uchar cmd, uchar Status, ulong sequence, const uchar *bodydata, ushort len, const uchar *ReserveData, uchar rhl, const char *SourceAdd, uchar type)
{
	UCHAR *tempBuf = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
	ushort packlen = 0;
	UCHAR HeadData[BUSINESS_HEAD_LEN] = {0};

	if (ProtVer/16 == 2) {
		packlen = len+9+rhl;
		ConvertMemcpy(tempBuf, &packlen, 2);
		tempBuf[2] = cmd/*+0x80*/;
		tempBuf[3] = Status;
		ConvertMemcpy(tempBuf+4, &sequence, 4);
		tempBuf[8] = rhl;
		memcpy(tempBuf+9, ReserveData, rhl);
		if (bodydata) {
			memcpy(tempBuf+9+rhl, bodydata, len);
		}
	} else {
		if (NULL == bodydata) {
			len = 0;
		}

		if (NULL == ReserveData) {
			rhl = 0;
		}

		/* 生成User Data内容 */
		uchar requeset[6];  // M_request(指令1+流水号4+错误码1)
		int ltp = 0;

		// the requeset tag
		requeset[0] = cmd;
		ConvertMemcpy(&requeset[1], (uchar *)&sequence, 4); // 服务器采用大端模式
		if ((cmd & 0x80)) { // 指令码最高位为1, 属于回复指令, 则包含回复指令tag和错误码
			requeset[0] &= 0x7F;
			requeset[5] = Status;   // 错误码
			ltp += TLVEncode(TAG_M_response, 6, (uchar *)requeset, 0, &tempBuf[0]);
		} else {
			ltp += TLVEncode(TAG_M_request, 5, (uchar *)requeset, 0, &tempBuf[0]);
		}

		if (bodydata) {
			memcpy(tempBuf+ltp, bodydata, len);
		}

		packlen = ltp+len; // User Data数据总长度
	}

	memcpy(HeadData, (UCHAR *)&packlen, 2); // 数据长度
	HeadData[2] = type; // 业务指令类型

	char tStr[len*3+1];

	tStr[0] = '\0';
	Hex2Str(tStr, bodydata, len, ' ');
	Log("BIZ", LOG_LV_DEBUG, "BusinessPackage, cmd=%hhu, st=%hhu, seq=%lu, t=%hhu, l=%hu, Data:%s.", cmd, Status, sequence, type, len, tStr);

	if (cmd == RCMD_UploadRecord) {
		Log("BIZ", LOG_LV_DEBUG, "FrDbPush=%d", FrDbPush(tempBuf, packlen));
	} else if (ProtVer/16!=2 && cmd==RCMD_InitializeSession) {
		packlen += 4;
		ConvertMemcpy(g_LogControl.buf, &packlen, 2);
		g_LogControl.buf[2]	= 2;	// Head Len
		g_LogControl.buf[3]	= 0;	// SPI
		memcpy(g_LogControl.buf+4, tempBuf, packlen-4);
		g_LogControl.len = packlen;
		g_LogControl.BufStatus = LOG_BUF_VALID;
	} else {	// 如果是回复指令，则不存入缓冲区
		ManageBusinessDataPush(tempBuf, packlen, HeadData);
	}

	sysfree(tempBuf);
	// 系统联机请求标志位
	g_TransmitControl.bRequestConnect = 1;

	return  0;
}

// 准备上线
void SrvOnline(void)
{
	g_TransmitControl.bRequestConnect = 1;
}
/*
	type:
	0-上传主控器和门锁上线信息,门锁脱机,
	1-上传主控器和门锁上线信息,门锁联机,
	2-只上传主控器上线.
*/
int RemoteSendOnlineNotify(int type)
{
	uchar ltagBuf[512];
	uchar temp;
	uint32_t wmc_code = 0;
	int start=0, ltp=0;

	ltp += 2;		// first two bytes reserver to big tag
	start = ltp;
	wmc_code = 0;
	ltp += TLVEncode(TAG_D_code, DevCodeSize, &wmc_code, 1, ltagBuf+ltp);

	temp = 0;
	ltp += TLVEncode(TAG_D_workStatus, 1, (uchar *)&temp, 0, ltagBuf+ltp);
	ltp += TLVEncode(TAG_M_version, 2, g_WMCVersion, 0, ltagBuf+ltp);
	ltp += TLVEncode(TAG_D_firmware, 2, g_WMCFirmware, 0, ltagBuf+ltp);
	ltagBuf[start-2] = TAG_D_deviceInfo;
	ltagBuf[start-1] = ltp-start;

	/* 处理子设备状态信息开始*/
	switch (type) {
	case 0:
	case 1: {	//上传主控器和特定DoorExcId门锁上线信息
		ulong DoorDcode=100;

		ltp += 2;  // first two bytes reserver to big tag
		start = ltp;
		ltp += TLVEncode(TAG_D_code, DevCodeSize, (uchar *)&DoorDcode, 1, &ltagBuf[ltp]);

		sLockInfo tInfo = {0};
		temp = LcGetStatus(DoorDcode, ~0, &tInfo);

		if (temp==0 && tInfo.Online==1) {
			ltp += TLVEncode(TAG_D_workStatus, 1, &temp, 0, &ltagBuf[ltp]);

			if ((tInfo.StatMask&LC_SM_SOFTV)) {
				ltp += TLVEncode(TAG_M_version, 2, &tInfo.SoftVer, 0, &ltagBuf[ltp]);
			}
			if ((tInfo.StatMask&LC_SM_HARDV)) {
				ltp += TLVEncode(TAG_D_firmware, 2, &tInfo.HardVer, 0, &ltagBuf[ltp]);
			}
		} else {
			temp = 1;
			ltp += TLVEncode(TAG_D_workStatus, 1, &temp, 0, &ltagBuf[ltp]);
		}
		ltagBuf[start-2] = TAG_D_deviceInfo;
		ltagBuf[start-1] = ltp-start;
	}
	break;

	case 2:	// 只上传主控器上线
	default:
		break;
	}
	BusinessPackage(RCMD_OnlineNotify, BUSINESS_STATE_OK, g_SystermInfo.BusinessSeq++, ltagBuf, ltp, NULL, 0, NULL, SOURCE_TYPE_ZKQ);

	return 0;
}

int RemoteSendTimeSynchronize(void)
{
	BusinessPackage(RCMD_TimeSynchronize, BUSINESS_STATE_OK, g_SystermInfo.BusinessSeq++, (UCHAR *)NULL, 0, NULL, 0, NULL, SOURCE_TYPE_ZKQ);

	return (0);
}

static int RemoteSendDownloadFile(const char *FileName, const ulong offset, const ulong size)
{
	int ltp = 0;
	uchar ltagBuf[FILENAME_LEN+14];

	// TODO 是否加上'\0'
	ltp += TLVEncode(TAG_M_file, strlen(FileName), (const uchar*)FileName, 0, &ltagBuf[ltp]);  //not contain end char '\0'
	ltp += TLVEncode(TAG_M_offset, 4, (uchar *)&offset, 1, &ltagBuf[ltp]);
	ltp += TLVEncode(TAG_M_size, 4, (uchar *)&size, 1, &ltagBuf[ltp]);

	Log("BIZ", LOG_LV_DEBUG, "RemoteSendDownloadFile, N=%s, o=%lu, s=%lu.", FileName, offset, size);

	BusinessPackage(RCMD_DownloadFile, BUSINESS_STATE_OK, g_SystermInfo.BusinessSeq++, ltagBuf, ltp, NULL, 0, NULL, SOURCE_TYPE_ZKQ);
	return (0);
}

int RemoteSendImsiChangedNotify(int64_t Imsi)
{
	if (Imsi <= 0) {
		return -1;
	}
	int ltp = 0;
	uchar ltagBuf[50];

	if (g_LogControl.LogSate!=LOG_STATE_HAVE_LONGIN && g_SystermInfo.NewImsi[0]) {
		// 没登陆且已上传过，不理，否则需要试试上传
		return -1;
	}
	snprintf(g_SystermInfo.NewImsi, 16, "%lld", Imsi);

	if (ProtVer/16 != 2) {
		// Imsi 被用来当做临时变量
		Imsi= 0;
		ltp+= TLVEncode(TAG_D_code, DevCodeSize, &Imsi, 1, ltagBuf+ltp);
		Imsi= TAG_M_IMSI;
		ltp+= TLVEncode(TAG_M_tag, 1, &Imsi, 0, ltagBuf+ltp);
	}
	ltp+= TLVEncode(TAG_M_IMSI, 15, g_SystermInfo.NewImsi, 0, ltagBuf+ltp);

	BusinessPackage(RCMD_Data_Changed_Notify, BUSINESS_STATE_OK, g_SystermInfo.BusinessSeq++, ltagBuf, ltp, NULL, 0, NULL, SOURCE_TYPE_ZKQ);

	return (0);
}

/*
服务器对门锁做状态查询时根据门锁返回状态将数据打包上传给服务器

seq - 门锁状态数据的业务层流水号
	seq < 0x80000000 - UploadStatus
	seq >= 0x80000000 - QueryStatusResponse

*/
/*
int RemoteSendUploadCameraStatus(ulong seq, ulong D_code, char workstatus)
{
    int tp, tempdcode, ltp;
//    typDoorOnline  *doorinfo = &(RfControl.DoorOnline[0][0]);
    char add[20];
    ulong addr = g_lDoorID, t;
    uchar ltagBuf[40];
    uchar tempBuf[200];

    tp = 0;
    tempdcode =100;
    t = GetRtcTime();

    if (t >= g_LastTime) {	// 当前获取的是最新状态，更新时间
        g_LastTime = t;
    } else {
        return 0;
    }

    ltp = 0;
    ltp += TLVEncode(TAG_D_code, DCODE_LEN, (uchar *)&D_code, 1, &ltagBuf[ltp]);
    ltp += TLVEncode(TAG_D_workStatus, 1, (uchar *)&workstatus, 0, &ltagBuf[ltp]);
    tp = TLVEncode(TAG_D_statusInfo, ltp, ltagBuf, 0, &tempBuf[5]); 	//big tag, 对于状态数据，从bit5开始复制，预留时间字段和状态标记

    memcpy(&tempBuf[1], (uchar *)&t, 4);	// bit1-bit5存放时间
    tp += 5;

    rf_printf( "cmd to query lock status, door id:%ld\r\n", addr);
    if (seq < 0x80000000) {
        if (g_BusinessDataDiagnose) {
            echo_printf("[DEBUG] Server << UploadStatus - Request,  sequence=%ld. D_code:100.\r\n", seq);
            debug_printf("");
        }

        tempBuf[0] = 0xFF;	// bit0存放状态标记, 0xFF - upload status, 0xFE - Querystatus Response

        BusinessPackage(RCMD_UploadStatus, BUSINESS_STATE_OK, seq, tempBuf, tp, NULL, 0, NULL, BUSINESS_TYPE_LC_LOCAL);
    } else {
        if (g_BusinessDataDiagnose) {
            echo_printf("[DEBUG] Server << QueryStatus - Response, sequence=%ld. D_code:100.\r\n", seq-0x80000000);
            debug_printf("");
        }

        tempBuf[0] = 0xFE;	// bit0存放状态标记, 0xFF - upload status, 0xFE - Querystatus Response

        BusinessPackage(RCMD_QueryStatus_Response, BUSINESS_STATE_OK, seq-0x80000000, tempBuf, tp, NULL, 0, NULL, BUSINESS_TYPE_LC_LOCAL);
    }
    return 0;
}
*/


/*
服务器对门锁做状态查询时根据门锁返回状态将数据打包上传给服务器

seq - 门锁状态数据的业务层流水号
	seq < 0x80000000 - UploadStatus
	seq >= 0x80000000 - QueryStatusResponse

*/
/*
int RemoteSendUploadStatus(ulong seq)
{
    int tp, tempdcode, ltp;
    typDoorOnline  *doorinfo = &(RfControl.DoorOnline[0][0]);
    char add[20];
    ulong addr = g_lDoorID, t;
    uchar ltagBuf[40];
    uchar tempBuf[200];

    tp = 0;
    tempdcode =100;
    t = GetRtcTime();

    if (t >= g_LastTime) {	// 当前获取的是最新状态，更新时间
        g_LastTime = t;
    } else {
        return 0;
    }

    ltp = 0;
    ltp += TLVEncode(TAG_D_code, DCODE_LEN, (uchar *)&tempdcode, 1, &ltagBuf[ltp]);
    sprintf(add, "%ld", doorinfo->add);
    ltp += TLVEncode(TAG_D_address, strlen(add), (uchar *)&add[0], 0, &ltagBuf[ltp]);
    ltp += TLVEncode(TAG_D_online, 1, (uchar *)&doorinfo->bOnline, 0, &ltagBuf[ltp]);
    if (doorinfo->FlagStatus & MASK_STATUS) {
#if defined (_PROTOCOL_V21)
        ltp += TLVEncode(TAG_L_status, 1, (uchar *)&doorinfo->status, 0, &ltagBuf[ltp]);
#elif defined (_PROTOCOL_V22) || defined (_PROTOCOL_V3)
        ltp += TLVEncode(TAG_L_lockerFlags, 2, (uchar *)&doorinfo->status, 1, &ltagBuf[ltp]);
#endif
    }
    if (doorinfo->FlagStatus & MASK_BATTERY) {  // 门锁电池电量
        ltp += TLVEncode(TAG_L_battery, 1, &doorinfo->battery, 0, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_TEMPERATURE) {  // 门锁温度
        ltp += TLVEncode(TAG_L_temperature, 2,(uchar *)&(doorinfo->temperatrue), 1, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_SIGNAL) { // 门锁丢包率
        ltp += TLVEncode(TAG_L_signal, 1, &doorinfo->StatVal, 0, &ltagBuf[ltp]);
    }
    ltp += TLVEncode(TAG_M_time, 4,(uchar *)&t, 1, &ltagBuf[ltp]);
    if (doorinfo->FlagStatus & MASK_VERSION) {  // 门锁软件版本
        ltp += TLVEncode(TAG_M_version, 2, (uchar *)&doorinfo->ver, 0, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_HARDWARE) { // 门锁硬件版本
        ltp += TLVEncode(TAG_D_firmware, 2, (uchar *)&doorinfo->firmware, 0, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_POPEDOMCAPACITY)    // 权限容量
        //if(doorinfo->PopedomCapacityFlag)
    {
        ltp += TLVEncode(TAG_L_popedomCapacity, 2, (uchar *)&doorinfo->PopedomCapacity, 0, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_POPEDOMCOUNT)    // 权限数
        //if(doorinfo->PopedomCountFlag)
    {
        ltp += TLVEncode(TAG_L_popedomCount, 2, (uchar *)&doorinfo->PopedomCount, 0, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_FINGERCAPACITY)
        //if(doorinfo->FingerCapacityFlag)
    {
        ltp += TLVEncode(TAG_L_fingerCapacity, 2, (uchar *)&doorinfo->FingerCapacity, 0, &ltagBuf[ltp]);
    }
    if (doorinfo->FlagStatus & MASK_FINGERCOUNT)
        //if(doorinfo->FingerCountFlag)
    {
        ltp += TLVEncode(TAG_L_fingerCount, 2, (uchar *)&doorinfo->FingerCount, 0, &ltagBuf[ltp]);
    }

    tp = TLVEncode(TAG_D_statusInfo, ltp, ltagBuf, 0, &tempBuf[5]); 	//big tag, 对于状态数据，从bit5开始复制，预留时间字段和状态标记

    memcpy(&tempBuf[1], (uchar *)&t, 4);	// bit1-bit5存放时间
    tp += 5;

    rf_printf( "cmd to query lock status, door id:%ld\r\n", addr);
    if (seq < 0x80000000) {
        if (g_BusinessDataDiagnose) {
            echo_printf("[DEBUG] Server << UploadStatus - Request,  sequence=%ld. D_code:100.\r\n", seq);
            debug_printf("");
        }

        tempBuf[0] = 0xFF;	// bit0存放状态标记, 0xFF - upload status, 0xFE - Querystatus Response

        BusinessPackage(RCMD_UploadStatus, BUSINESS_STATE_OK, seq, tempBuf, tp, NULL, 0, NULL, BUSINESS_TYPE_LC_LOCAL);
    } else {
        if (g_BusinessDataDiagnose) {
            echo_printf("[DEBUG] Server << QueryStatus - Response, sequence=%ld. D_code:100.\r\n", seq-0x80000000);
            debug_printf("");
        }

        tempBuf[0] = 0xFE;	// bit0存放状态标记, 0xFF - upload status, 0xFE - Querystatus Response

        BusinessPackage(RCMD_QueryStatus_Response, BUSINESS_STATE_OK, seq-0x80000000, tempBuf, tp, NULL, 0, NULL, BUSINESS_TYPE_LC_LOCAL);
    }
    return 0;
}
*/

//void testdes(void);
//int LockDeviceQuery(ulong *DevAdd, ushort *LogicCode)
//{
//	int i;
//
//	for (i = 0; i < MAX_SUBDEVICE; i++) {
//		if (g_DeviceSetup.Device[i].Status && (g_DeviceSetup.Device[i].type == DEVICE_TYPE_LOCK) ) {
//			if (DevAdd) {
//				*DevAdd = atol(g_DeviceSetup.Device[i].address);
//			}
//			if (LogicCode) {
//				*LogicCode =  g_DeviceSetup.Device[i].LogicCode;
//			}
//			sp_printf("find lock by add:%ld\r\n", *DevAdd);
//			// 作用？
////            LockLinkLedModeSet(LOCK_LINK_MODE_OFF);
//			return 1;
//		}
//	}
//	// 都未找到，则闪烁模式
//	LockLinkLedModeSet(LOCK_LINK_MODE_FLASH);
//	return 0;
//}


int AlertDeviceQuery(ulong DevAdd, uchar *type, ushort *LogicCode)
{
	int i;
	char sdevadd[20];

	sprintf(sdevadd, "%ld", DevAdd);

	for (i = 0; i < MAX_SUBDEVICE; i++) {
		if (g_DeviceSetup.Device[i].Status && (g_DeviceSetup.Device[i].type >= DEVICE_TYPE_WINDOW_MAGNETIC) && \
				(g_DeviceSetup.Device[i].type <= DEVICE_TYPE_MANUAL_ALERT )) {
			//   sp_printf("query add:%s, type:%d, cmp dev add:%s\r\n", g_DeviceSetup.Device[i].address, g_DeviceSetup.Device[i].type, sdevadd);
			if (!strcmp(g_DeviceSetup.Device[i].address, sdevadd)) {
				*type = g_DeviceSetup.Device[i].type;
				*LogicCode =  g_DeviceSetup.Device[i].LogicCode;
				return 1;
			}
		}
	}

	return 0;
}
/*
void ClearDeviceQuery(void)
{
    memset((uchar *)&g_DeviceSetup, 0, sizeof(g_DeviceSetup));
    OS_CPU_SR	cpu_sr;
    OS_ENTER_CRITICAL();
    StoreSystemPara();
    OS_EXIT_CRITICAL();
    //StoreSystemPara();
    LockLinkLedModeSet(LOCK_LINK_MODE_FLASH);
    ModuleAddDoorID(0, 0, 0, 1);

    RfControl.DoorOnline[0][0].bOnline = 0;

    sp_printf("door %d not online 5...\r\n", g_lDoorID);
    RemoteSendOnlineNotify(0);
}

static void LockLinkLedModeSet(int mode)
{
	//return ;
	g_TransmitControl.LockLinkLedMode = mode;
	switch (mode) {
	case LOCK_LINK_MODE_FLASH:
		LedsCtrl(LED_DOOR_LINK, LED_CLOSE_OPR);
		break;
	case LOCK_LINK_MODE_OFF:
		LedsCtrl(LED_DOOR_LINK, LED_CLOSE_OPR);
		break;
	case LOCK_LINK_MODE_ON:
		LedsCtrl(LED_DOOR_LINK, LED_OPEN_OPR);
		break;
	}
}

void LockLinkLedFlash(void)
{
	static int ledstate = 0;

	if (g_TransmitControl.LockLinkLedMode == LOCK_LINK_MODE_FLASH) {
		ledstate = ledstate>0?0:1;
		LedsCtrl(LED_DOOR_LINK, ledstate);
	}
}
#if defined(_PCB_2_0) || defined (_PCB_2_2) || defined(_PCB_3_1) || defined(_PCB_3_2)
void PowerOffLedFlash(void)
{
    int i;
    static int ledstate = 0;

    //LedsCtrl(LED_DOOR_OPEN, 1);
    //OSTimeDly(100);
    SetDog(TASK_DOG_RFNET);
    WatchDogFeed();
    for (i=0; i<4; i++) {
        ledstate = ledstate>0?0:1;
//		LedsCtrl(LED_DOOR_OPEN, ledstate);
        OSTimeDly(100);
    }

#if USE_PROTOCOL_V21
    if (RfControl.DoorOnline[0][0].status & 0x10)	// bit4, open
#elif (USE_PROTOCOL_V22 || USE_PROTOCOL_V3)
    if (RfControl.DoorOnline[0][0].status & 0x1)	// bit0, open
#endif
    {
//		LedsCtrl(LED_DOOR_OPEN, 0);
    } else {	// close
//		LedsCtrl(LED_DOOR_OPEN, 1);
    }
}
#endif
*/
#if 1
/*
static void ClearRecordModeSet(int mode)
{
    g_TransmitControl.RecordClearLedMode = mode;
    switch (mode) {
    case LOCK_LINK_MODE_FLASH:
//			LedsCtrl(LED_RSV1, 1);
        break;
    case LOCK_LINK_MODE_OFF:
//			LedsCtrl(LED_RSV1, 1);
        break;
    case LOCK_LINK_MODE_ON:
//			LedsCtrl(LED_RSV1, 0);
        break;
    default:
        break;
    }
}

// clear record here
void ClearRecordSet(void)
{
    ClearAllRecord();
    ClearRecordModeSet(LOCK_LINK_MODE_FLASH);
}
void ClearRecordReSet(void)
{
    ClearRecordModeSet(LOCK_LINK_MODE_OFF);
}
*/
#endif

void RecordClearFlash(void)
{
	static int ledstate = 0;

	if (g_TransmitControl.RecordClearLedMode == LOCK_LINK_MODE_FLASH) {
		ledstate = ledstate>0?0:1;
//		LedsCtrl(LED_RSV1, ledstate);
	}
}

//send wireless alert
/*
int UploadAlertRecord(ulong DevAdd )
{
	//OS_CPU_SR cpu_sr;
	uchar type, alerttype;
	ushort LogicCode;
	int bShouldUpRecord = 0	;

	int tp, ltp1, ltp2;
	uchar ltagBuf1[60];
	uchar ltagBuf2[60];
	uchar tempBuf[60];
#if 0
	if (DevAdd == 42195152) {
//        StartBeepByMode(1, 10, 10);
		//   testrecordwrite();
#if 0
		memset(sData, 0, sizeof(sData));
		sData[0]=num++;
		GprsSendDataPush(sData, 85, headData);
#endif
		//	g_GprsState.bDoGprsServerConnect = 1;
		//	sp_printf("do connect server\r\n");
		return 1;
	} else if (DevAdd == 42195144) {
//        StartBeepByMode(1, 10, 10);
		//  testfirstread();
		//	GprsSendDataPop(sData, headData);

		//	g_GprsState.bDoDisconnect = 1;
		//sp_printf("do disconnect server\r\n");
		return 1;
	} else if (DevAdd == 42195140) {
		//StartBeepByMode(1, 10, 10);
		OS_CPU_SR cpu_sr;
		OS_ENTER_CRITICAL();
		//	g_TransmitControl.bRequestConnect = 1;
		OS_EXIT_CRITICAL();
		//	testrecorddelete();
		return 1;
	}
	if (DevAdd == 5592112) {
		//   LCUpdateNotify();
		//   testflash();
		return 1;
	}
#endif

	if (AlertDeviceQuery(DevAdd, &type, &LogicCode)) {
		sp_printf("query device success by devadd:%ld, type:%d, logic code:%d\r\n", DevAdd, type, LogicCode);
		sp_printf("g_SystermInfo.AlertAllow=%X\r\n", g_SystermInfo.AlertAllow);
		switch (type) {
		case DEVICE_TYPE_SMOKE: {
			alerttype = ALERT_TYPE_SMOKE;
			sp_printf("device type is smoke\r\n");
			if (g_SystermInfo.AlertAllow & (1<<ALERT_TYPE_SMOKE)) {
				bShouldUpRecord = 1;
			}
		}
		break;

		case DEVICE_TYPE_WINDOW_MAGNETIC: {
			alerttype = ALERT_TYPE_WINDOW;
			sp_printf("device type is window\r\n");
			if (g_SystermInfo.AlertAllow & (1<<ALERT_TYPE_WINDOW)) {
				bShouldUpRecord = 1;
			}
		}
		break;

		case DEVICE_TYPE_INFRARED: {
			alerttype = ALERT_TYPE_INFRAED;
			sp_printf("device type is infraed\r\n");
			if (g_SystermInfo.AlertAllow & (1<<ALERT_TYPE_INFRAED)) {
				bShouldUpRecord = 1;
			}
		}
		break;

		case DEVICE_TYPE_YK_SET_DEFENCE: {
			sp_printf("device type is set defence\r\n");
			g_SystermInfo.AlertAllow |= 1<<ALERT_TYPE_WINDOW;
//            StartBeepByMode(1, 10, 10);
		}
		break;

		case DEVICE_TYPE_YK_CLEAR_DEFENCE: {
			sp_printf("device type is clear defence\r\n");
			g_SystermInfo.AlertAllow &= ~(1<<ALERT_TYPE_WINDOW);
//            StartBeepByMode(1, 20, 1);
		}
		break;

		case DEVICE_TYPE_MANUAL_ALERT: {
			alerttype = ALERT_TYPE_MANUAL;
			sp_printf("device type is manual alert\r\n");
			if (g_SystermInfo.AlertAllow & (1<<ALERT_TYPE_MANUAL)) {
				bShouldUpRecord = 1;
			}
		}
		break;

		default:
			break;
		}
		if (bShouldUpRecord) {
			static ulong LastInfraedSendTime = 0;
			if (alerttype == ALERT_TYPE_INFRAED) {
				ulong currtime = GetSysTime();
				if (currtime < LastInfraedSendTime + INFRAED_ALERT_INTAL_TIME) { // half hour
					return 0;
				} else {
					LastInfraedSendTime = currtime;
				}
			} else if (alerttype == ALERT_TYPE_SMOKE) {
				ulong currtime = GetSysTime();
				if (currtime < LastInfraedSendTime + 600) { // 1 minute
					return 0;
				} else {
					LastInfraedSendTime = currtime;
				}
			}

			ltp1 = 0;
			ltp1 += TLVEncode(TAG_D_alertType, 1, &alerttype, 0, &ltagBuf1[ltp1]);
			ulong t = GetRtcTime();
			ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
			ser_printf("t=%ld\r\n", t);
			ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
			ltp1 += TLVEncode(TAG_M_time, 4,(uchar *)&t, 1, &ltagBuf1[ltp1]);

			ltp2 = TLVEncode(TAG_L_alertRecord, ltp1, ltagBuf1, 0, &ltagBuf2[0]);     //big tag

			tp = 0;
			tp += TLVEncode(TAG_D_code, DCODE_LEN, (uchar *)&LogicCode, 1, &ltagBuf1[tp]);
			uchar typerecord = RECORD_TYPE_ALERT;
			tp += TLVEncode(TAG_D_recordType, 1, (uchar *)&typerecord, 1, &ltagBuf1[tp]);
			tp += TLVEncode(TAG_D_data, ltp2, ltagBuf2, 0, &ltagBuf1[tp]);                  //big tag

			tp = TLVEncode(TAG_D_record, tp, ltagBuf1, 0, &tempBuf[0]);

			BusinessPackage(RCMD_UploadRecord, BUSINESS_STATE_OK, g_SystermInfo.BusinessSeq++, tempBuf, tp, NULL, 0, NULL, BUSINESS_TYPE_CM);
		}
	}
	return 0;
}
*/

//#if USE_PROTOCOL_V21
//int UpdateNormalAlertRecord(ushort LogicCode, uchar alerttype)
//{
//    ulong t;
//    int tp, ltp1, ltp2;
//    uchar ltagBuf1[60];
//    uchar ltagBuf2[60];
//    uchar tempBuf[60];
//    uchar typerecord;
//
//    ser_printf("NormalAlertRecord... alerttype=%d\r\n", alerttype);
//
//    ltp1 = 0;
//    ltp1 += TLVEncode(TAG_D_alertType, 1, &alerttype, 0, &ltagBuf1[ltp1]);
//    t = GetRtcTime();
//    ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
//    ser_printf("t=%d\r\n", t);
//    ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
//    ltp1 += TLVEncode(TAG_M_time, 4,(uchar *)&t, 1, &ltagBuf1[ltp1]);
//
//    ltp2 = TLVEncode(TAG_L_alertRecord, ltp1, ltagBuf1, 0, &ltagBuf2[0]);     //big tag
//
//    tp = 0;
//    tp += TLVEncode(TAG_D_code, DCODE_LEN, (uchar *)&LogicCode, 1, &ltagBuf1[tp]);
//    typerecord = RECORD_TYPE_ALERT;
//    tp += TLVEncode(TAG_D_recordType, 1, (uchar *)&typerecord, 1, &ltagBuf1[tp]);
//    tp += TLVEncode(TAG_D_data, ltp2, ltagBuf2, 0, &ltagBuf1[tp]);                  //big tag
//
//    tp = TLVEncode(TAG_D_record, tp, ltagBuf1, 0, &tempBuf[0]);
//
//    //BusinessPackage(RCMD_UploadRecord, BUSINESS_STATE_OK,  g_SystermInfo.BusinessSeq++,  tempBuf, tp, NULL, 0, "", BUSINESS_TYPE_CM);
//
//#if 1
//    UCHAR HeadData[BUSINESS_HEAD_LEN];
//    UCHAR rhl = 0;
//    UCHAR cmd = RCMD_UploadRecord;
//    UCHAR Status = BUSINESS_STATE_OK;
//    UCHAR type = BUSINESS_TYPE_CM;
//    ushort packlen;
//    ulong seq = g_SystermInfo.BusinessSeq++;
//    UCHAR *data = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
//    UCHAR *ReserveData = NULL;
//    char *SourceAdd = NULL;
//
//    packlen = tp+9+0;
//
//    ConvertMemcpy(&data[0], (uchar *)(&packlen), 2);
//    data[2] = cmd/*+0x80*/;
//    data[3] = Status;
//    ConvertMemcpy(&data[4], (uchar *)&seq, 4);
//    data[8] = rhl;
//    memcpy(&data[9], ReserveData, rhl);
//    memcpy(&data[9+rhl], tempBuf, tp);
//
//    memset(HeadData, 0, sizeof(HeadData));
//    memcpy(HeadData, &packlen, 2);
//    HeadData[2] = type;
//#ifndef _project_cbm_32
//    memcpy(&HeadData[3], SourceAdd, 16);
//#endif
//    //ManageBusinessDataPush(data, packlen, HeadData);
//    int ret = PushRecordBuf(seq, data, packlen);
//    if (ret == 1) { //place at send buf ,not record flash
//        ManageBusinessDataPush(data, packlen, HeadData);
//    }
//    sysfree(data);
//
//    //g_SystermInfo.BusinessSeq++;  // 已用seq替代
//#endif
//
//#ifdef _SEND_SMS
//    ltagBuf1[0] = alerttype;
//    packlen = 1;
//    memset(HeadData, 0, sizeof(HeadData));
//    memcpy(HeadData, &packlen, 2);
//    HeadData[2] = type;
//    SmsSendDataPush(ltagBuf1, packlen, HeadData);
////    g_doSendSms = 1;
//#endif
//
//    return 0;
//}
//#endif
//
//#if USE_PROTOCOL_V22
//int UpdateNormalAlertRecord(ushort LogicCode, uchar alerttype, uint32_t *time_stamp)
//{
//    ulong t;
//    int tp, ltp1, ltp2;
//    uchar ltagBuf1[60];
//    uchar ltagBuf2[60];
//    uchar tempBuf[60];
//
//    ltp1 = 0;
//    /*
//    ltp1 += TLVEncode(TAG_D_alertType, 1, &alerttype, 0, &ltagBuf1[ltp1]);
//    ulong t = GetRtcTime();
//    ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
//    ser_printf("t=%d\r\n", t);
//    ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
//    ltp1 += TLVEncode(TAG_M_time, 4,(uchar *)&t, 1,  &ltagBuf1[ltp1]);
//    */
//
//    ser_printf("NormalAlertRecord... alerttype=%d\r\n", alerttype);
//
//    ltagBuf1[0] = alerttype;
//    t = GetRtcTime();
//    if (time_stamp) {
//        *time_stamp = t;
//    }
//    ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
//    ser_printf("t=%ld\r\n", t);
//    ser_printf("||||||||||||||||||||||||||||||||||||||||||||||||||||\r\n");
//    ConvertMemcpy(&ltagBuf1[1], (uchar *)&t, 4);
//    ltp1 = 5;
//
//    ltp2 = TLVEncode(TAG_L_alertRecord3, ltp1, ltagBuf1, 0, &ltagBuf2[0]);     //big tag
//
//    tp = 0;
//    tp += TLVEncode(TAG_D_code, DCODE_LEN, (uchar *)&LogicCode, 1, &ltagBuf1[tp]);
//    //uchar typerecord = RECORD_TYPE_ALERT;
//    //tp += TLVEncode(TAG_D_recordType, 1, (uchar *)&typerecord, 1, &ltagBuf1[tp]);
//    tp += TLVEncode(TAG_D_data, ltp2, ltagBuf2, 0, &ltagBuf1[tp]);                  //big tag
//
//    tp = TLVEncode(TAG_D_record, tp, ltagBuf1, 0, &tempBuf[0]);
//
//    //BusinessPackage(RCMD_UploadRecord, BUSINESS_STATE_OK,  g_SystermInfo.BusinessSeq++,  tempBuf, tp, NULL, 0, "", BUSINESS_TYPE_CM);
//
//#if 1
//    UCHAR HeadData[BUSINESS_HEAD_LEN];
//    UCHAR rhl = 0;
//    UCHAR cmd = RCMD_UploadRecord;
//    UCHAR Status = BUSINESS_STATE_OK;
//    UCHAR type = BUSINESS_TYPE_CM;
//    ushort packlen;
//    //int j;
//    ulong seq = g_SystermInfo.BusinessSeq++;
//    uchar *ReserveData = NULL;
//    uchar *data = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
//    char *SourceAdd = NULL;
//
//    packlen = tp+9+0;
//
//    ConvertMemcpy(&data[0], (uchar *)(&packlen), 2);
//    data[2] = cmd/*+0x80*/;
//    data[3] = Status;
//    ConvertMemcpy(&data[4], (uchar *)&seq, 4);
//    data[8] = rhl;
//    memcpy(&data[9], ReserveData, rhl);
//    memcpy(&data[9+rhl], tempBuf, tp);
//
//    memset(HeadData, 0, sizeof(HeadData));
//    memcpy(HeadData, &packlen, 2);
//    HeadData[2] = type;
//    //memcpy(&HeadData[3], SourceAdd, 16);
//    //ManageBusinessDataPush(data, packlen, HeadData);
//    int ret = PushRecordBuf(seq, data, packlen);
//    if (ret == 1) { //place at send buf ,not record flash
//        ManageBusinessDataPush(data, packlen, HeadData);
//    }
//    sysfree(data);
//
//    //g_SystermInfo.BusinessSeq++;
//#endif
//
//#ifdef _SEND_SMS
//    ltagBuf1[0] = alerttype;
//    packlen = 1;
//    memset(HeadData, 0, sizeof(HeadData));
//    memcpy(HeadData, &packlen, 2);
//    HeadData[2] = type;
//    SmsSendDataPush(ltagBuf1, packlen, HeadData);
////    g_doSendSms = 1;
//#endif
//
//    return 0;
//}
//#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
* Description:
* Input:
* Ouput:
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BUF_SIZE 10240
static int file_content_sumchk(int fd, uint32_t Cs)
{
	uchar buff[BUF_SIZE] = {0};
	int retrys = 0, chk_ok = 0, len;
	ulong sum_chk=0;

	if ( fd<0 ) {
		printf("fd is error\n\r");
		return -1;
	}

	for ( retrys = 0; retrys < 3; retrys++ ) {
		if (lseek(fd, 0, SEEK_SET) == -1) {
			perror("Fail to lseek!\n\r");
			return -2;
		}
		sum_chk = 0;
		// 分段累加和
		for (;;) {
			len = read(fd, buff, BUF_SIZE);
			if (len == -1) {
				perror("Fail to read b_update.bin file!\n\r");
				return -6;
			}
			sum_chk += CheckSum(buff, len);
			//printf("len:%d, sum:%08lX\n\r", len, sum_chk);
			if (len < BUF_SIZE) {
				// 整个文件读取完毕了，比较校验和
				if ( sum_chk != Cs ) {
					ser_printf("sum_chk:%08lX,sum:%08X\n\r", sum_chk, Cs);
					chk_ok = 0;
					break;
				}
				chk_ok = 1;
				break;
			}
		}
		// 判断结果
		if (chk_ok == 1) {
			printf("file content chk is correct\n\r");
			return 0;
		}
	}
	return -3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int F_check_ctrl_upgrade_file_whether_copy(void)
{
	int b_update_fd = -1, retrys=0, bSuc=-1;
	typUpgradeNotify *ctrl_upgrade = NULL;
	uchar *b_buf = NULL;
	uint32_t Cs=0;

	ctrl_upgrade = &g_UpgradeNotify[FLAG_UPGRADE_CTRL];
	printf("g_UpgradeNotify[FLAG_UPGRADE_CTRL]: bUpgradeNotify=%d,offset=%ld,size=%ld,crc32=%08lX,downloadsuccess=%d\r\n",\
		   ctrl_upgrade->bUpgradeNotify, ctrl_upgrade->offset,\
		   ctrl_upgrade->size, ctrl_upgrade->crc32,\
		   ctrl_upgrade->downloadsuccess);

	/* I.若升级文件下载成功标志位存在，则进行文件拷贝*/
	if (ctrl_upgrade->downloadsuccess==0 || ctrl_upgrade->size<=CONTENT_OFFSET) {
		return -1;
	}
	// 读取升级文件的校验码
	if (UpgradeFileRead(FLAG_UPGRADE_CTRL, CHKSUM_OFFSET, &Cs, 4) != 0) {
		return -1;
	}
	/* II.文件创建和拷贝*/
	if((b_update_fd = open(TMP_UPDATE_FILE_PATH, O_RDWR | O_TRUNC | O_CREAT, 0777)) == -1) {
		perror("Fail to create the new_file!\n\r");
		return -2;
	}
	if (ftruncate(b_update_fd, ctrl_upgrade->size-CONTENT_OFFSET) == -1) {
		perror("Fail to ftruncate the new_binfile!\n\r");
		close(b_update_fd);
		return -2;
	}
	if (lseek(b_update_fd, 0, SEEK_SET) == -1) {
		perror("Fail to lseek!\n\r");
		close(b_update_fd);
		return -2;
	}
	// 缓存分配
	b_buf = (uchar *)malloc(ctrl_upgrade->size-CONTENT_OFFSET);
	if (b_buf==NULL) {
		printf("malloc is fail\n\r");
		close(b_update_fd);
		return -3;
	}
	// 读取升级文件的内容
	if (UpgradeFileRead(FLAG_UPGRADE_CTRL, CONTENT_OFFSET, b_buf, ctrl_upgrade->size-CONTENT_OFFSET) != 0) {
		close(b_update_fd);
		return -4;
	}
	for ( retrys = 0; retrys < 3; retrys++ ) {
		if ( write(b_update_fd, b_buf, ctrl_upgrade->size-CONTENT_OFFSET) == (int)(ctrl_upgrade->size-CONTENT_OFFSET )) {
			break;
		}
		if ( retrys == 2 ) {
			perror("write b_backup is fail and clear downloadsuccess flag\n\r");
			close(b_update_fd);
			free(b_buf);
			/* 清除升级文件包下载成功标志位*/
			memset((char*)&g_UpgradeNotify[FLAG_UPGRADE_CTRL], 0, sizeof(typUpgradeNotify));
			return -5;
		}
		lseek(b_update_fd, 0, SEEK_SET);
	}
	free(b_buf);

	/* III.文件内容校验,若正确则置位重启标志位*/
	if ( file_content_sumchk(b_update_fd, Cs) == 0 ) {
		printf("b_update.bin check sum success and clear downloadsuccess flag\n\r");
		close(b_update_fd);
		rename(TMP_UPDATE_FILE_PATH, UPDATE_FILE_PATH);
		/* 清除升级文件包下载成功标志位*/
		memset((char*)&g_UpgradeNotify[FLAG_UPGRADE_CTRL], 0, sizeof(typUpgradeNotify));
		bSuc = 0;
	}

	if (bSuc==0) {
		return 0;
	} else {
		return -6;
	}
}

// 可以放在BIZ中
static int LcMakeStatusInfo(int DevCode, uint16_t StatMask, uint8_t *Buf)
{
	if (Buf==NULL) {
		return -1;
	}
	int Ret = -1;
	char tAddr[20] = {0};
	sLockInfo tLock = {0};

	Ret = LcGetStatus(DevCode, StatMask, &tLock);
	if (Ret) {
		return -1;
	}
	snprintf(tAddr, 20, "%llu", tLock.Addr);
	Ret = 0;
	Ret+= TLVEncode(TAG_D_code, DevCodeSize, &DevCode, 1, Buf+Ret);
	if (StatMask == (uint16_t)~0) {
		// 查询所有信息时，带上地址
		Ret+= TLVEncode(TAG_D_address, strlen(tAddr)+1, tAddr, 0, Buf+Ret);
	}
	if (ProtVer/16 == 2) {
		Ret+= TLVEncode(TAG_D_online, 1, &(tLock.Online), 0, Buf+Ret);
	} else {
		tLock.Online = !tLock.Online;
		Ret+= TLVEncode(TAG_D_workStatus, 1, &(tLock.Online), 0, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_STAT)) {
		Ret+= TLVEncode(TAG_L_lockerFlags, 2, &(tLock.Status), 1, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_BAT)) {
		Ret+= TLVEncode(TAG_L_battery, 1, &(tLock.BatStat), 1, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_TEMP)) {
		Ret+= TLVEncode(TAG_L_temperature, 2, &(tLock.Temp), 1, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_SGNL)) {
		Ret+= TLVEncode(TAG_L_signal, 1, &(tLock.Signal), 1, Buf+Ret);
	}
	Ret+= TLVEncode(TAG_M_time, 4, &(tLock.CollTime), 1, Buf+Ret);
	if ((tLock.StatMask&LC_SM_SOFTV)) {
		Ret+= TLVEncode(TAG_M_version, 2, &(tLock.SoftVer), 0, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_HARDV)) {
		Ret+= TLVEncode(TAG_D_firmware, 2, &(tLock.HardVer), 0, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_PCAP)) {
		Ret+= TLVEncode(TAG_L_popedomCapacity, 2, &(tLock.PermsCap), 1, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_PCNT)) {
		Ret+= TLVEncode(TAG_L_popedomCount, 2, &(tLock.PermsCnt), 1, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_FGPCAP)) {
		Ret+= TLVEncode(TAG_L_fingerCapacity, 2, &(tLock.FgPermCap), 1, Buf+Ret);
	}
	if ((tLock.StatMask&LC_SM_FGPCNT)) {
		Ret+= TLVEncode(TAG_L_fingerCount, 2, &(tLock.FgPermCnt), 1, Buf+Ret);
	}

	return Ret;
}

/*
服务器对门锁做状态查询时根据门锁返回状态将数据打包上传给服务器

seq - 门锁状态数据的业务层流水号
	seq < 0x80000000 - UploadStatus，此时会根据StatChgMask上传相关信息
	seq >= 0x80000000 - QueryStatusResponse

*/
int LcSendStatus(int DevCode, uint32_t Seq, uint16_t StatChgMask)
{
	int Ret = -1;
	uint8_t FrameBuf[256] = {0};

	Ret = LcMakeStatusInfo(DevCode, Seq<0x80000000?StatChgMask:~0, FrameBuf+2);

	if (Ret > 0) {
		FrameBuf[0]	= TAG_D_statusInfo;
		FrameBuf[1]	= Ret;
		Ret			+=2;
		Ret = BusinessPackage(Seq<0x80000000?RCMD_UploadStatus:RCMD_QueryStatus_Response, BUSINESS_STATE_OK,
							  Seq<0x80000000?g_SystermInfo.BusinessSeq++:Seq-0x80000000, FrameBuf, Ret,
							  NULL, 0, NULL, BUSINESS_TYPE_LC_LOCAL);
	} else {
		Ret = -1;
	}

	return Ret;
}
