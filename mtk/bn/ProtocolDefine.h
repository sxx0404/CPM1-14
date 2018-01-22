#if !defined(AFX_PROTOCOLDEFINE_H__9986E32E_B3BA_4D09_9CA9_4BA51435C0D3__INCLUDED_)
#define AFX_PROTOCOLDEFINE_H__9986E32E_B3BA_4D09_9CA9_4BA51435C0D3__INCLUDED_

//transmit layer define  start
#define     CMD_LOGIN_REQUEST    0x1
#define     CMD_LOGIN_RESPONSE 0x80000001
#define     CMD_SUBMIT_RESQUEST  0x2
#define     CMD_SUBMIT_RESPONSE  0x80000002
#define     CMD_DELIVER_REQUEST   0x3
#define     CMD_DELIVER_RESPONSE 0x80000003
#define     CMD_LOGOUT_REQUEST     0x4
#define     CMD_LOGOUT_RESPONSE     0x80000004
#define     CMD_GERENIC_NACK        0x80000000

#define     TRSMT_STATE_OK                                     0x0
#define     TRSMT_STATE_ERR_LENGTH                     0x1
#define     TRSMT_STATE_ERR_CMD                           0x2
#define     TRSMT_STATE_CONN_STATUS_ERR           0x3
#define     TRSMT_STATE_ERR_MAC                           0x4
#define     TRSMT_STATE_ERR_ADDR_SOURCE           0x5
#define     TRSMT_STATE_ERR_ADDR_DISTINATION  0x6
#define     TRSMT_STATE_ERR_PASSWORD                 0x7
#define     TRSMT_STATE_ERR_THOUGHOUT               0x8
#define     TRSMT_STATE_ERR_LockStatus                0x09
#define     TRSMT_STATE_ERR_SEQUENCE                 0x0a

#define     TRSMT_STATE_ERR_PROTOCOL                 0x0b
#define     TRSMT_STATE_ERR_DECODE_PDU            0x0c
#define     TRSMT_STATE_ERR_SECURED_PACKET      0x0d
#define     TRSMT_STATE_ERR_DECODE_MSG             0x0e

#define     TRSMT_STATE_ERR_UNKNOWN                  0xff

//transmit layer define  end

//security layer define  start
#define  ENCRYPT_MODE_SD        0x0	// 双方私定
#define  ENCRYPT_MODE_DES       0x1
#define  ENCRYPT_MODE_AES       0x2
#define  ENCRYPT_MODE_SM1       0x3

#define  DES_MODE_CBC           0x0
#define  THDES_MODE_CBC_2KEY    0x1
#define  THDES_MODE_CBC_3KEY    0x2

#define  DES_MODE_CBCV3			0x1 // 用DES-CBC加密
#define  THDES_MODE_CBC_2KEYV3	0x2 // 用3DES-CBC-2key加密
#define  SM1_MODE_CBC			0x3 // 用SM1-CBC加密
#define  SM1_LV_MODE_ECB		0x4 // 用SM1-LV分组加密，分组长度为192字节（ECB）
#define  AES_MODE_CBC			0x5 // 用AES-CBC加密

#define  RECORD_TYPE_OPEN_DOOR  0x1
#define  RECORD_TYPE_XG         0x2
#define  RECORD_TYPE_ALERT      0x3

#if 0
#define ALERT_TYPE_WINDOW       1
#define ALERT_TYPE_SMOKE        2
#define ALERT_TYPE_INFRAED      3
#define ALERT_TYPE_MANUAL       4
case 1:
     return '门锁防拆报警';
     case 2:
     return '门锁强行闯入报警';
     case 3:
     return '主控器断电报警';
     case 4:
     return '主控器备用电池低电量报警';
     case 5:
     return '主控器窗磁报警';
     case 6:
     return '主控器红外移动检测报警';
     case 7:
     return '主控器烟感报警';
     case 8:
     return '主控器温度传感器报警';
     CASE 9:
	 	手动报警
#else
#define ALERT_TYPE_DESTORY				1	// 防拆报警
#define ALERT_TYPE_FORCED_ENTER			2	// 强行闯入
#define ALERT_TYPE_WMC_EXTRA_POWER_OFF	3	// 主控器断电报警
#define ALERT_TYPE_BAKUP_POWER_LOW		4   // 控制器备用电池低电量报警
#define ALERT_TYPE_WINDOW				5   // 窗磁报警
#define ALERT_TYPE_INFRAED				6   // 红外移动检测报警
#define ALERT_TYPE_SMOKE				7   // 烟感报警
#define ALERT_TYPE_WMC_TEMPERATURE		8   // 温度传感器报警
#define ALERT_TYPE_MANUAL				9   // 手动报警
#define	ALERT_TYPE_LOCK_FAIL			10	// 门未关好报警
#define	ALERT_TYPE_BATTERY_POWER_LOW	11	// （门锁）电池低电量
#define	ALERT_TYPE_FINGER_NOT_CHECKED	12	// 需要指纹验证，但是未刷卡、未响应
#define	ALERT_TYPE_PSAM_EXPIRING		13	// PSAM卡即将过期报警（3天）
#define	ALERT_TYPE_PSAM_EXPIRED			14	// PSAM卡已过期。
#define	ALERT_TYPE_PSAM_INVALID			15	// PSAM卡错误，需要更换
#define	ALERT_TYPE_DOOR_LONGOPEN		16	// 门长开报警
#define	ALERT_TYPE_KEY_OPEN 			17	// 钥匙开门报警

#define	ALERT_TYPE_WMC_DEFAULT 			18	// 主控器脱机事件, 缺省状态, 模块刚开启或重启时
#define	ALERT_TYPE_WMC_MOD_INVALID		19	// 主控器脱机事件, 模块失效(重启失败)
#define	ALERT_TYPE_WMC_SIM_ERROR		20	// 主控器脱机事件, SIM卡ERROR(掉卡)
#define	ALERT_TYPE_WMC_SIM_NOTINSERT	21	// 主控器脱机事件, SIM卡没插
#define	ALERT_TYPE_WMC_COPS_ERROR		22	// 主控器脱机事件, 无法获取运营商
#define	ALERT_TYPE_WMC_PDP_ERROR		23	// 主控器脱机事件, 建立PDP上下文连接ERROR

#define	ALERT_TYPE_CAPTURE_SD_OVERFLOW	32	// 拍照器存储空间满（无法自动回收）。

#define ALERT_TYPE_MAX_NO               ALERT_TYPE_KEY_OPEN // 最大报警标号
#endif

//security layer define end

#if 0
//Business layer define
/*
TAG 定义
*/
// 控制器定期发出的时钟校验
#define   RCMD_TimeCheck                     0x01
// 控制器登陆命令
#define   RCMD_Login                              0x02
//  查询门锁状态
#define   RCMD_QueryLockStatus            0x03
// 重启控制器或者门锁
#define   RCMD_ResetCtrlLock                 0x04
// 门锁重启的结果
#define   RCMD_ResetLockResult             0x05
// 远程开门
#define   RCMD_RemoteOpen                   0x06
// 开门记录，由主控器上传
#define   RCMD_OpenRecord                    0x07
// 程序升级的准备
#define   RCMD_ProgramUpdatePrepare    0x08
// 升级文件的分包发送
#define   RCMD_ProgramUpdatePack          0x09
// 升级完毕
#define   RCMD_ProgramUpdateFinish        0x0A
// 控制器门锁设置，最多5个门锁
#define   RCMD_ConfigCtrlLocks                0x0B
// 开门权限设置
#define   RCMD_ConfigOpenRights            0x0C
// 控制器自身信息的配置
#define   RCMD_ConfigCtrlInfo                  0x0D
// GPRS传输密钥设置  待定（DES－CBC）
#define   RCMD_SetGPRSTransPass          0x0E
// 设置门锁提醒
#define   RCMD_SetLockNotify                  0x0F
// 通知主控器临时下线
#define   RCMD_NotifyCtrlOffline              0x10
// 主控器主动退出
#define   RCMD_CtrlOffline                       0x11
// 心跳包
#define   RCMD_HeartThrob                    0x20



/*tag*/
#define Tag_MC_time                             0x01            // 时间，4B，与1970年时间差，以秒为单位
#define Tag_C_mac                                0x02             // C_mac	0x02	主控器Mac地址，17B
#define Tag_C_protocol                         0x03             // C_protocol	0x03	主控器协议版本号，2B
#define Tag_C_hsType                           0x04             // C_hsType	0x04	主控器软件和硬件类型，2B，第一个字节硬件，第二个软件
#define Tag_L_addr                               0x05           // L_addr	0x05	门锁地址，4B
#define Tag_MC_Record                         0x06           // MC_Record	0x06	表示一条记录，可以是权限、门锁状态、开门记录等
#define Tag_L_online                             0x07          // L_online	 0x07	门锁联机，1B，
#define Tag_L_status                            0x08          // L_status	0x08	门锁状态，1B
#define Tag_L_battery                          0x09              // L_battery	0x09	门锁电池电量，1B
#define Tag_L_temperature                 0x0A            //L_temperature	0x0A	门锁温度，2B
#define Tag_L_signal                            0x0B            // L_signal	0x0B	门锁信号强度，1B
#define Tag_L_version                         0x0C              // L_version	0x0C	门锁版本号，2B
#define Tag_CL_operSucc                    0x0D             // CL_operSucc	0x0D	主控器或门锁操作是否成功，1B
#define Tag_L_remoteOpenMode         0x0E             // L_remoteOpenMode	0x0E	远程开门模式，1B
#define Tag_C_openRecID                    0x0F               //C_openRecID	0x0F	开门记录的ID号，由主控器生成，上发，4B
#define Tag_C_openMode                     0x10             // C_openMode	 0x10	开门记录中的开门类型，1B
#define Tag_C_cardNo                          0x11            //C_cardNo	0x11	卡号，4B
#define Tag_M_fingerID                       0x12           //M_fingerID	0x12	指纹ID号，4B，由管理软件生成
#define Tag_C_openStatus                   0x13          // C_openStatus	0x13	开门的权限状态：1B
#define Tag_M_updateSessionID         0x14         // M_updateSessionID	0x14	程序升级的会话ID，4B
#define Tag_M_updateType                 0x15            // M_updateType	0x15	升级类型，1B
#define Tag_M_fileTotalLen                  0x16        // M_fileTotalLen	0x16	升级文件的总长度，4B
#define Tag_M_filePackNo                    0x17        // M_filePackNo	0x17	升级文件分包的包号，2B
#define Tag_M_fileOver                        0x18         // M_fileOver 	0x18	升级是否结束，即是否是最后一个包了，1B
#define Tag_M_filePackCnt                    0x19       //M_filePackCnt	0x19	升级文件每包的内容，
#define Tag_L_type                              0x1A           // L_type	0x1A	门锁类型，1B，
#define Tag_L_areaID                          0x1B           // L_areaID	0x1B	门锁所属的区域ID，4B
#define Tag_L_openDelay                    0x1C       //L_openDelay	0x1C	门锁的开门延时，1B
#define Tag_L_openPass                     0x1D        // L_openPass	0x1D	开门密码，4B
#define Tag_L_operate                        0x1E           // L_operate	0x1E	对门锁的操作，1B
#define Tag_L_fireLink                        0x1F            // L_fireLink	0x1F	门锁是否消防联动，1B
#define Tag_M_startTime                    0x20         // M_startTime	0x20	开始时间，4B
#define Tag_M_endTime                      0x21        // M_endTime	0x21	截止时间，4B
#define Tag_M_fingerCode                  0x22        // M_fingerCode	0x22	指纹码，512B
#define Tag_M_userPass                     0x23         // M_userPass	0x23	用户密码，4B，
#define Tag_M_heartInterval              0x24       // M_heartInterval	0x24	心跳间隔，1B
#define Tag_L_PSAMPass                    0x25        // L_PSAMPass	0x25	门锁PSAM密钥，16B
#define Tag_M_secuKeyNo                  0x26        // M_secuKeyNo	0x26	GPRS通讯密钥的编号，1B
#define Tag_M_secuKey                      0x27          // M_secuKey	0x27	GPRS通讯密钥，采用DES－CBC加密，8B
#define Tag_L_alertOper                    0x28        // L_alertOper	0x28	设置门锁提醒，1B
#define Tag_M_fileContent                 0x29        //M_fileContent   0x29 文件传输内容


//Business layer  end
#endif

#define LCCMD_TimeSynchronize		0x01
#define LCCMD_QueryLockStatus		0x02      /*0x50*/
#define LCCMD_LockStatusChange		0x03      /*0x51*/
#define LCCMD_Reset_Notify			0x04
#define LCCMD_Upgrade_Notify		0x05
#define LCCMD_Download_File			0x06
#define LCCMD_Read_Data 			0x07    // 读取门锁数据
#define RCMD_FILE_TRANSMIT			0x08

#define TAG_L_DefenceState			0x30        //门锁的设防状态
#define TAG_L_TerminalSeqNo			0x31         //终端序列号

//keyword   redefine 2012/5/24
#define RCMD_TimeSynchronize		0x01
#define RCMD_OnlineNotify			0x02    // 设备上线/下线线通知
#define RCMD_UploadStatus			0x03    // 上传状态数据
#define RCMD_UploadRecord			0x04    // 上传记录数据
#define RCMD_DownloadFile			0x05    // 下载文件
#define RCMD_Data_Changed_Notify	0x06    // IMSI变更通知

#define RCMD_DownloadFile_Response	(RCMD_DownloadFile+0x80)
#define RCMD_QueryStatus			0x11
#define RCMD_QueryStatus_Response	(RCMD_QueryStatus+0x80)
#define RCMD_ResetNotify			0x12
#define RCMD_ResetNotify_Response	(RCMD_ResetNotify+0x80)
#define RCMD_UpgradeNotify			0x13
#define RCMD_UpgradeNotify_Response	(RCMD_UpgradeNotify+0x80)
#define RCMD_SetupDevice			0x14
#define RCMD_SetupDevice_Response	(RCMD_SetupDevice+0x80)	// SetupDevice鐨凴esponse鎸囦护
#define RCMD_SetupKey				0x15
#define RCMD_SetupKey_Response		(RCMD_SetupKey+0x80)	// SetupKey鐨凴esponse鎸囦护
#define RCMD_Forward				0x16
#define RCMD_Forward_Response		(RCMD_Forward+0x80)
#define RCMD_SetupParameter			0x17
#define RCMD_PutPopedom				0x18
#define RCMD_GetPopedom				0x19
#define RCMD_RemoteControl			0x1A
#define RCMD_UpdatePSAM				0x1B

#define RCMD_SMSNotify				0x1c
#define RCMD_READIMSI				0x1D

#define RCMD_ReadFile				0x1E
#define RCMD_ListFile				0x1F

#define RCMD_ReadData				0x1C


#define RCMD_Welcome			    0x10    // TCP建立时特殊报文welcome
#define RCMD_InitializeSession		0x07    // 建立安全信道session

#define RCMD_TouchSession		    0x20    // UDP专用
#define RCMD_InvokeOnline		    0x21    // 唤醒TCP链路通知（UDP/短信通道专用）

// TAG redefine 2012/5/24
#define TAG_D_code				0x01
#define TAG_D_online			0x02
#define TAG_D_type				0x03
#define TAG_D_address			0x04
#define TAG_M_version			0x05
#define TAG_M_time				0x06
#define TAG_D_recordType		0x07
#define TAG_D_encryptInfo		0x08
#define TAG_D_matchCode			0x09
#define TAG_D_keyword			0x0A
#define TAG_D_resultCode		0x0B
#define TAG_M_serverIP			0x0C
#define TAG_M_serverPort		0x0D
#define TAG_M_file				0x0E
#define TAG_M_offset			0x0F
#define TAG_M_size				0x10
#define TAG_M_content			0x11
#define TAG_M_crc32				0x12
#define TAG_M_crc16				0x13
#define TAG_M_token				0x14
#define TAG_M_keyNo				0x15
#define TAG_M_keyIndex			0x16
#define TAG_M_keyData			0x17
#define TAG_M_KCV				0x18
#define TAG_L_status			0x19
#define TAG_L_battery			0x1A
#define TAG_L_temperature		0x1B
#define TAG_L_signal			0x1C
#define TAG_L_areaID			0x1D
#define TAG_L_openDelay			0x1E
#define TAG_L_PIN				0x1F
#define TAG_L_alertMode			0x20    // protocol: V2.1
#define TAG_L_cardMode			0x21    // protocol: V2.1
#define TAG_L_PINMode			0x22    // protocol: V2.1

#define TAG_L_fingerCheckMode	0x20    // protocol: V2.2, V2.3
#define TAG_L_fingerCheckParam	0x21    // protocol: V2.2, V2.3
#define TAG_L_allowedOpenMode	0x22    // protocol: V2.2, V2.3

#define TAG_L_fireLink			0x23
#define TAG_L_fingerMode		0x24

#define TAG_L_gsmSignal			0x24

#define TAG_L_defenceMode		0x25
#define TAG_M_operate			0x26
#define TAG_L_popedomType		0x27
#define TAG_L_cardNo			0x28
#define TAG_M_fingerID			0x29
#define TAG_M_startTime			0x2A
#define TAG_M_endTime			0x2B
#define TAG_M_fingerCode		0x2C
#define TAG_L_controlType		0x2D	// protocol: V2.1
#define TAG_L_remoteOpenMode	0x2E	// protocol: V2.1
#define TAG_D_controlAction		0x2D	// protocol: V2.2, V2.3
#define TAG_D_controlParam		0x2E	// protocol: V2.2, V2.3
#define TAG_M_APDU				0x2F
#define TAG_M_POR				0x30
//#define TAG_M_SP_Address	    0x31
#define TAG_L_lockerFlags		0x31
#define TAG_M_random			0x32
#define TAG_M_IMSI				0x33
#define TAG_L_openType			0x34
#define TAG_L_openStatus		0x35
#define TAG_L_cardDataEx		0x36
#define TAG_L_cardDataEx2		0x37
#define TAG_D_alertType			0x38
#define TAG_D_alertData			0x39

#define TAG_D_refreshInterval	0x41     //unit:s
#define TAG_D_firmware			0x42
#define TAG_L_popedomCapacity	0x43
#define TAG_L_popedomCount		0x44
#define TAG_L_fingerCapacity	0x45
#define TAG_L_fingerCount		0x46
#define TAG_L_openRecord3		0x47
#define TAG_L_alertRecord3		0x48
#define TAG_M_busyTime			0x49

#define TAG_M_sourceMsisdn		0x4A
#define TAG_M_serverURL			0x4B
#define TAG_D_workStatus		0x4C	// 设备工作状态


#define TAG_M_request			0x50
#define TAG_M_response			0x51
#define TAG_M_secureHead		0x52
#define TAG_M_secureData		0x53
#define TAG_M_authData			0x54
#define TAG_M_sessionId			0x55
#define TAG_M_counter			0x56
#define TAG_M_tag				0x57

#define TAG_M_customParam       0x58    // 通用参数字符串
#define TAG_M_paramName         0x59
#define TAG_M_paramValue        0x5A

#define TAG_M_feature           0x5B    // 设备支持特性
#define TAG_M_vendor            0x5C    // 设备厂商
#define TAG_M_ICCID             0x5D    // SIM卡的ICCID数据 (hex string)
#define TAG_D_DUID              0x5E    // （格式化的）设备唯一识别码

#define TAG_D_alertMode			0x5f

//wmc&lock local comunicate
#define TAG_WL_CheckSum         0x60

#define TAG_L_networkMode		0x60
#define TAG_L_SdCapacity		0x61
#define TAG_L_SdFreeSpace		0x62
#define TAG_L_fileRecord3		0x63
#define TAG_M_location			0x64
#define TAG_M_CameraParam		0x65
#define TAG_M_CaptureProfile	0x66
#define TAG_M_pattern			0x67
#define TAG_M_resultValue		0x68
#define TAG_M_transmitHandle	0x69	// 传输句柄
#define TAG_M_transmitMode		0x6A	// 传输模式
#define TAG_D_DEVICE_CUID		0x6B	// 兼容的设备识别码（长地址）, 只上传不设置
#define TAG_M_LOCATION_ID		0x6C	// 兼容的地址位置编码, 只下设

#define TAG_D_deviceInfo		0x70    // 设备描述信息
#define TAG_D_statusInfo		0x71
#define TAG_D_record			0x72
#define TAG_D_setup				0x73
#define TAG_D_parameter			0x74
#define TAG_M_key				0x76
#define TAG_L_popedom			0x77
#define TAG_D_data				0x78
#define TAG_M_notify			0x79
#define TAG_M_register			0x7A
#define TAG_L_openRecord		0x7B
#define TAG_L_alertRecord		0x7c

#define BUSINESS_STATE_OK                            0
#define BUSINESS_STATE_UNKNOWN_ERROR  255

#define BUSINESS_STATE_ERROR_NOT_SUPPORTED_KEYWORD	0x20
#define BUSINESS_STATE_ERROR_NOT_AUTHERATED			0x21
#define BUSINESS_STATE_ERROR_UNKNOWN_DEVICE			0x22	// 未知设备
#define BUSINESS_STATE_ERROR_UNKNOWN_TAG			0x23	// 未知TAG
#define BUSINESS_STATE_ERROR_DECODE_TLV				0x24	// TLV解包出错
#define BUSINESS_STATE_ERROR_CRYPTED_DATA			0x25	// CRC16校验出错，可能是SM1密码错误。

#define BUSINESS_STATE_ERROR_SETUPKEY_NO			0x30	// Setup Key - M_keyNo error
#define BUSINESS_STATE_ERROR_SETUPKEY_INDEX			0x31	// Setup Key - M_keyIndex error
#define BUSINESS_STATE_ERROR_SETUPKEY_KCV			0x32	// Setup Key - M_KCV error

#define BUSINESS_STATE_ERROR_FORWARD_LOCKNOTONLINE	0x33	// Forward - lock not online
#define BUSINESS_STATE_ERROR_FORWARD_TRANSMITFAIL	0x34	// Forward - transmit fail
#define BUSINESS_STATE_ERROR_FORWARD_REQTIMEOUT		0x35	// Forward  -online but request time out

#define BUSINESS_STATE_ERROR_DOWNLOAD_CRCERROR		0x36	// Downloadfile - block data crc32 check error
#define BUSINESS_STATE_ERROR_DOWNLOAD_SIZELONG		0x37	// Downloadfile - the upgrade file size too long

#define BUSINESS_STATE_ERROR_UPNOTIFY_ARGMISSING	0x38	// UpgradeNotify - some UpgradeNotify argument missing
#define BUSINESS_STATE_ERROR_UPNOTIFY_INCORRECTNAME	0x39	// UpgradeNotify - the notified upgrade file is incorrect

#define BUSINESS_STATE_ERROR_DOWNLOAD_INCORRECTFILE	0x40	// Downloadfile - the file to be downloaded is incorrect
#define BUSINESS_STATE_ERROR_DOWNLOAD_INCORRECTHWID	0x41	// Downloadfile - the file type is OK but the hardware ID is incorrect

#define BUSINESS_STATE_ERROR_NONE_TAG_DATA	        0x42	// ReadData时，未初始化的TAG数据
#define BUSINESS_STATE_ERROR_NONE_PARAM_DATA	    0x43	// ReadData时，未初始化的param数据
#define BUSINESS_STATE_ERROR_UNKNOWN_PARAM_NAME	    0x44	// ReadData时，未定义的paramName

#define BUSINESS_STATE_ERROR_READFILE_RDDATAERROR	0x45		//ReadFile - Read Data Error
#define BUSINESS_STATE_ERROR_READFILE_ARGERROR		0x46		//ReadFile - argument Error

#define BUSINESS_TYPE_LCM_RECORD                1
#define BUSINESS_TYPE_LOCK_FORWARD              2
#define BUSINESS_TYPE_LC_LOCAL                  3   // 门锁记录
#define BUSINESS_TYPE_CM                        4   // 主控器产生记录

#define SOURCE_TYPE_ZKQ    1	// 主控器产生的记录
#define SOURCE_TYPE_MS      2
#define SOURCE_TYPE_PZQ      3	//拍照器产生的记录(照片文件)
#define PACK_HEAD_LENGTH  9 // 10 最短数据长度为9 @14-01-17

#define	COUNTER_TYPE_NONE 		0x0
#define	COUNTER_TYPE_NO_CHECK	0x1

#endif
