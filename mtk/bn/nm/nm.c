/**************************************************************************************************************************
*	Copyright(C)		Hangzhou Engin Electronic Co.,Ltd		All Rights Reserved
*
*	Filename:ppp_api.c
*
*	Description:
*
*	Author:Sunzheng			Created at:2014.09.15		Ver:V1.01.01
***************************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <alloca.h>
#include <errno.h>
#include <termios.h>

#include "nm.h"
#include "../NormalTool.h"
#include "../main_linuxapp.h"
#include "../../include/gpio.h"
#include "../BusinessProtocol.h"			//SMSDecode RemoteSendImsiChangedNotify g_SystermInfo
#include "../ManageSecurity.h"			//SecurityLayerDecode
#include "../cfgFile.h"
#include "../../include/common.h"
#include "../../include/log.h"

#define PING_INTVL	600				// ping的间隔
#define IMSI_INTVL	60				// ping的间隔
#define RSSI_INTVL	10				// ping的间隔
#define SMS_INTVL	1				// ping的间隔
#define PING_TIMEO	30				// ping操作的超时时间
#define PING_MAX	5				// 最多ping几次，超过需要特殊干预


// TODO 网络模式判断
uint8_t g_Rssi = 0;


static char *AtcDevPath = NULL;
/*
static char alert_content[][50] = {
    {""},
    {"门锁防拆报警"},   // ALERT_TYPE_DESTORY
    {"强行闯入报警"},   // ALERT_TYPE_FORCED_ENTER
    {"主控器断电报警"}, // ALERT_TYPE_WMC_EXTRA_POWER_OFF
    {"控制器备用电池低电量报警"},   // ALERT_TYPE_BAKUP_POWER_LOW
    {"窗磁报警"},   // ALERT_TYPE_WINDOW
    {"红外移动检测报警"},   // ALERT_TYPE_INFRAED
    {"烟感报警"},   // ALERT_TYPE_SMOKE
    {"温度传感器报警"},   // ALERT_TYPE_WMC_TEMPERATURE
    {"手动报警"},   // ALERT_TYPE_MANUAL
    {"门虚掩报警"},   // ALERT_TYPE_LOCK_FAIL
    {"（门锁）电池低电量"},   // ALERT_TYPE_BATTERY_POWER_LOW
    {"需要指纹验证，但是未刷卡、未响应"},   // ALERT_TYPE_FINGER_NOT_CHECKED
    {"PSAM卡即将过期报警（3天）"},
    {"PSAM卡已过期。"},
    {"PSAM卡错误，需要更换"},   // ALERT_TYPE_PSAM_INVALID
    {"门长开报警"}, // ALERT_TYPE_DOOR_LONGOPEN
    {"机械钥匙开门报警"}, // ALERT_TYPE_KEY_OPEN
};


static void ppp_status_proc();
static int32_t F_DialUp(int mode);
static int F_HungUp(void);
static int F_3g_module_is_access();
static int F_3g_module_device_open();
static void PushDelete(uint8_t id);
static int PopDelete(void);
static void PduDecode(unsigned char *density, unsigned char *source, uint8_t pdubits);
static void ProtExplainMessage(uint8_t *ptr, uint8_t *sender, uint8_t encodemode);
static void PduNumToNum(uint8_t *density,uint8_t *source);
static int DoRecvGprsData(void);
static int DoGprsPowerOn(int mode);
static int QueryModuleState(void);
static int SearchNumber(uint8_t *StartPtr, int maxlen, int *octnumber);
static int DeleteAllMessages(int total, int type);
static int ModuleResetOpr(void);
static int ModulePoweronOpr(void);
static int ModulePoweroffOpr(void);
static void init_module_status(void);
static int SendSMS(uint8_t alert_type);
static int module_at_check_proc(void);
static int module_sim_check_proc(void);
static int module_cops_opr_proc(void);
static int module_para_config(void);
static int module_csq_query_proc(void);

static int SendATCommand(char *data)
{
    char	DataBuff[4096+1];

    //     ser_printf("s%\n", data);
    //WriteComPort((unsigned char *)data, strlen(data));
    return	SendSocketData(GPRS_fd, (char *)data, DataBuff, strlen(data), DataBuff, 5);
}

static void PushDelete(uint8_t id)
{
    if (DeleteMessageCount < 70)
    {
        DeleteMessageId[DeleteMessageCount] = id;
        DeleteMessageCount++;
    }
}

// 无数据则返回-1
static int PopDelete(void)
{
    if (DeleteMessageCount>0)
    {
        DeleteMessageCount--;
        return(DeleteMessageId[DeleteMessageCount]);
    }
    else
    {
        return(-1);
    }
}

static int IsNumber(uint8_t data)
{
      if ((data >= '0') && (data <= '9'))return 1;
      else return 0;
}

static int GetImsi(void)
{
    int i,k,judge,RecivedNum, result;
    uint8_t d;
    uint8_t *ptr;

    RecivedNum = 0; k = 0; judge = 0; result = 0;
    ptr = SerialData.read_buf;
    for (i = 0; i < SerialData.len; i++)
    {
        d =*ptr++;
        if (IsNumber(d))
        {
					judge = 1;
          g_SystermInfo.NewImsi[k++] = d;
          if ( k > 15 ) break;
					if ( k == 15 ) RecivedNum = 1;
        }
        else if (RecivedNum)
        {
          //sp_printf("g_SystermInfo.NewImsi=%s, g_SystermInfo.StoreImsi=%s\r\n", g_SystermInfo.NewImsi, g_SystermInfo.StoreImsi);
#if 0
          if (0 == strcmp(g_SystermInfo.NewImsi, g_SystermInfo.StoreImsi))
              ;
          else
          {
              g_SystermInfo.bImsiChangedRequestSend = 1;
          }
#else
          g_SystermInfo.bImsiChangedRequestSend = 1;
#endif
          //sp_printf("g_SystermInfo.NewImsi=%s, g_SystermInfo.StoreImsi=%s\r\n", g_SystermInfo.NewImsi, g_SystermInfo.StoreImsi);
          g_SystermInfo.NewImsi[15] = 0;
          sp_printf("g_SystermInfo.NewImsi=%s, g_SystermInfo.StoreImsi=%s\r\n", g_SystermInfo.NewImsi, g_SystermInfo.StoreImsi);
          result = 1;
          break;
        }
				else if (judge) break;
    }
    return (result);
}

static int SearchNumber( uint8_t *StartPtr, int maxlen, int *octnumber )
{
    int i, k, RecivedNum;
    int result;
    uint8_t *ptr = NULL;
    uint8_t tempIndex[5];
    uint8_t d;

    memset(tempIndex, 0, 5);
    ptr = StartPtr;
    RecivedNum = 0;
    result = 0;
    k = 0;

    for (i=0; i<maxlen; i++)
    {
        d = *ptr++;
        if (IsNumber(d))
        {
            tempIndex[k++] = d;
            if (k > 4)
            {
                break;
            }
            RecivedNum = 1;
        }
        else if (RecivedNum)    // 搜到数字
        {
            *octnumber = atoi((char *)tempIndex);
            result = 1;
            break;
        }
    }

    return (result);
}

static int GetSmsIndex(int start, int maxlen, int  *index)
{
    return SearchNumber(&SerialData.read_buf[start], maxlen, index);
}

static void net_mode_judege(uint8_t *buf, int len)
{
//  uint8_t *p=buf;
  int pos;
  char mode;

  if ( (pos = bufstr(buf, "CHN-UNICOM", len)) >= 0 )
  {
    pos += 12;
    mode = buf[pos];
    //printf("mode:%c\n", mode);
    switch(mode)
    {
      case '0':
        // 熄灭
        LedsCtrl(LED_3G_NET, LED_CLOSE_OPR);
        break;

      case '2':
        // 点亮
        LedsCtrl(LED_3G_NET, LED_OPEN_OPR);
        break;

      default:
        // 熄灭
        LedsCtrl(LED_3G_NET, LED_CLOSE_OPR);
        break;
    }
  }
}

static int QueryModuleState(void)
{
	int opr_suc = 0;
	int i;
//	int offset;
	uint32_t t_cur_sec;
	static uint32_t t_last_sec = 0;
//	long left_time;
	static int ResetCount = 0;

	t_cur_sec = GetRtcTime();
	if ( t_cur_sec >= t_last_sec && t_cur_sec-t_last_sec < _3G_MODULE_QUERY_INTERVAL ) return 0;

	//每隔5s进行一次模块运行状态查询和处理
	t_last_sec = t_cur_sec;
  sp_printf("\r\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
  sp_printf("%s,time:", __func__);
	PrintfTimeDec(t_cur_sec);
	// 运行状态判断
	ser_printf("g_GprsState.SIMCardState=%d, g_ppp_status=%d\r\n",\
		g_GprsState.SIMCardState, g_ppp_status);
  sp_printf("\r\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
#ifdef _FLOW_STATISTICS
  sp_printf("g_SmsPara.g_total_flow=%dB(%fKB)\r\n", g_SmsPara.g_total_flow, g_SmsPara.g_total_flow/1024.0);
#endif
  SetDog(TASK_DOG_GPRS);
  WatchDogFeed();

	// 2.模块参数若没有设置，则进行参数配置
	opr_suc = module_para_config();
	if ( opr_suc!=0 )
	{	// 操作失败，则设置标志位，模块需要进行复位操作
		g_GprsState.SIMCardState = GRPS_MODULE_STATE_AT_ERROR;
		goto ERR1;
	}

	// 3.1 AT指令测试
	opr_suc = module_at_check_proc();
	if ( opr_suc!=0 )
	{	// 操作失败，则设置标志位，模块需要进行复位操作
		g_GprsState.SIMCardState = GRPS_MODULE_STATE_AT_ERROR;
		goto ERR1;
	}

	// 3.3 运营商查询操作
	if ( 0 == SendATCmdEx("AT+COPS?\r", (char *)SerialData.read_buf, 5, "OK") )
	{
		SerialData.len = strlen((char *)SerialData.read_buf);

		if (bufstr(SerialData.read_buf, "UNICOM", SerialData.len) >= 0)
		{
			//memcpy(&g_BootPara.cApn[0], "UNINET", 6); 					 //apn
			//g_BootPara.cApn[6] = '\0';
			g_GprsState.NetType = 1;
			ser_printf("net type is CH-UNICOM\r\n");
		}
		else	if (bufstr(SerialData.read_buf, "MOBILE", SerialData.len) >= 0)
		{
			//memcpy(&g_BootPara.cApn[0], "CMNET", 5);						//apn
			//g_BootPara.cApn[5] = '\0';
			g_GprsState.NetType = 2;
			ser_printf("net type is CHINA MOBILE\r\n");
		}
		else
		{
			//return 0;
			//g_GprsState.NetType = 3;
			g_GprsState.NetType = 0;
			ser_printf("net type UNKNOWN\r\n");
		}
	  // 加入网络制式的判断 0:GSM 2:UTRAN，指示3G还是2G网络
	  net_mode_judege(SerialData.read_buf, SerialData.len);
	}
	else
	{
		g_GprsState.NetType = 0;
	}

	if (g_GprsState.NetType) // 已获取到运营商, LED灯亮
	{
		ser_printf("CopsState: GPRS_MODULE_STATE_COPS_OK...\r\n");
		g_GprsState.CopsState = GPRS_MODULE_STATE_COPS_OK;
	}
	else
	{
		ser_printf("CopsState: GPRS_MODULE_STATE_COPS_ERROR...,ResetCount:%d\r\n", ResetCount);
		ResetCount++;
		if (ResetCount >= 6)
		{
			ResetCount = 0;
			g_GprsState.CopsState = GPRS_MODULE_STATE_COPS_ERROR;
			goto ERR1;
		}
	}

	// 3.4 CSQ查询操作
	module_csq_query_proc();

	// 3.5 CREG网络注册状况查询
	if ( 0 == SendATCmdEx("AT+CREG?\r\n", (char *)SerialData.read_buf, 5, "OK") )
	{
	  ;
	}

	// 3.6 查询当前网络连接状况
	if ( 0 == SendATCmdEx("at^syscfg?\r\n", (char *)SerialData.read_buf, 5, "OK") )
	{
	  ;
	}

	if ( 0 == SendATCmdEx("AT^SYSINFO\r\n", (char *)SerialData.read_buf, 5, "OK") )
	{
	  ;
	}

	// 3.7 读取IMSI号(SIM卡操作指令测试(AT+CIMI))
	if (!g_GprsState.bReadIMSI)
	{
		if (0 == SendATCmdEx("AT+CIMI\r", (char *)SerialData.read_buf, 5, "OK"))
		{
			SerialData.len = strlen((char *)SerialData.read_buf);
			if (GetImsi())
			{
				ser_printf("sim card IMSI:");
				for (i = 0; i < 15; i++)
				ser_printf("%c", g_SystermInfo.NewImsi[i]);
				ser_printf("\r\n");
				g_GprsState.bReadIMSI = 1;
				DeleteAllMessages(20, 4);
			}
		}
	}

	//操作都OK，则返回0
//	g_GprsState.qos = 1;
	g_GprsState.SIMCardState = GRPS_MODULE_STATE_NORMAL;
	return 0;

ERR1:
	// 重新复位3G模块，清标志位
	DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
	return -1;
}

#ifdef _SEND_SMS
static int DoSMSSendProcess(void)
{
    uint8_t HeadData[BUSINESS_HEAD_LEN];
    uint8_t sendbuf[20];
    int len, iOnceSend= 0;

    len = 0;
    for (iOnceSend = 0; iOnceSend < 30; iOnceSend++)
    {
        memset(HeadData, 0, sizeof(HeadData));
        len = SmsSendDataPop(&sendbuf[0], HeadData);
        if (len > 0)    // 有未发短信
        {
            ser_printf("pop sms send len:%d\r\n", len);
            ser_printf("g_doSendSms1=%d\r\n", g_doSendSms);
            if (1 == SendSMS(sendbuf[0])) // 成功
            {
                OSTimeDly(300);
                //g_doSendSms = 0;
            }
            else    // 失败
            {
                g_doSendSms = 1;
            }
            ser_printf("g_doSendSms2=%d\r\n", g_doSendSms);
        }
        else
        {
            g_doSendSms = 0;
            break;
        }
    }

    return 1;
}
#endif
*/
/* 7位 PDU 格式解码 */
static void PduDecode(uint8_t *density, const uint8_t *source, uint8_t pdubits)
{
	unsigned int shift=0,i;
	unsigned int restbits=0;
	unsigned int temp;
	uint8_t sourcelen,densitylen=0;
	uint8_t buffer;
	uint8_t masktab[] = {0x00, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

	sourcelen = *source;
	*density = sourcelen;
	density++;
	for (i=0; i<sourcelen; i++) {
		source++;
		temp = *source;
		shift = shift|(temp << restbits);
		restbits += 8;
		for ( ; restbits>=pdubits; restbits-=pdubits ) {
			buffer = shift & masktab[pdubits];
			*density = buffer;
			shift = shift >> pdubits;
			density++;
			densitylen++;
			if (densitylen >= sourcelen) {
				return;
			}
		}   /* restbits loop */
	}   /* i loop */

	return ;
}

static void ProtExplainMessage(uint8_t *Data, uint16_t Len)
{
	if (Data==NULL || (Len < Data[0]+14)) {
		return;
	}
	uint8_t *ptr = NULL;
	uint8_t encodemode = 0;
	uint8_t buffer[256];

	ptr = Data+Data[0]+2;				// 之前为SCA和TPU的第一个字节，现在指在TPDU中的OA
	if (Len < Data[0]+14+(ptr[0]+1)/2) {
		return;
	} else {
		// 减去实际数据之前的长度，此时为UD的长度
		Len -= (Data[0]+14+(ptr[0]+1)/2);
	}
	ptr = ptr+(*ptr+1)/2+3;				// 跳过OA和PID，指在DCS
	encodemode = *ptr;					// DCS 数据的编码方式
	ptr = ptr+8;						// 跳过DCS和SCTS指向UDL其后为UD

	if (ptr[0] > Len) {
		return;
	}

	if (encodemode == 0) {
		// 7位编码方式
		PduDecode(buffer, ptr, 7);
		ptr = buffer;
	}

	int SeLen = *ptr++;					// 现在指向UD
	uint8_t sedataout[256];

	if (SecurityLayerDecode(ptr, sedataout, &SeLen) == 0) {
		SMSDecode(sedataout, SeLen);
	}
}

/* PDU格式的号码转换成字符串号码 */
/*
static void PduNumToNum(uint8_t * density,uint8_t * source)
{
	uint8_t i;
	char j;

	i = *(source++);
	if (i>15) {
		j = 15;
	} else {
		j = i;
	}
	if (*(source++) == 0x91) {
		*(density++) = '+';
	}
	while (j>0) {
		i=*(source++);
		if ((i & 0xf) < 0xa) {
			j--;
			*(density++) = '0'+(i & 0xf);
		}
		if ((i & 0xf0) < 0xa0) {
			j--;
			*(density++) = '0'+(i & 0xf0)/16;
		}
	}
	*density='\0';

	return ;
}

static int DoRecvGprsData(void)
{
    unsigned char *m_readbuf = NULL;
    uint8_t *ptr = NULL;
    int cmplen, k;
    int len;
    uint8_t buffer[200];
    char atbuf[64];
    uint8_t chr, encodemode;
    uint8_t recvhigh = 1;
    uint8_t count = 0;
    int i;

    //ser_printf("func:%s...\n\r", __func__);
    //GetATCommand("OK", GPRS_READ_WAITTIME);    //the data if have will be processed in GetATcommad function
    i = PopDelete();
    printf("PopDelete, i=%d\r\n", i);
    if (i >= 0)   //have sms
    {
        //goto dfdf;
        sprintf(atbuf, "AT+CMGR=%d\r\n", i);

#if 0
        SendATCommand(atbuf);
        ser_printf("%s\r\n", atbuf);
        if (GetATCommand("+CMGR:", DEFAULT_AT_WAITTIME) > 0)
#else
        memset(SerialData.read_buf, 0, sizeof(SerialData.read_buf));
        if (0 == SendATCmdEx(atbuf, (char *)(SerialData.read_buf), 10, "+CMGR:"))
#endif
        {
            len = strlen((const char *)(SerialData.read_buf));
            m_readbuf = sysmalloc(MAX_SERIAL_DATALEN);
            if (NULL == m_readbuf)   // malloc fail, quit
            {
                ser_printf("func:%s, line:%d, malloc <m_readbuf> fail...\n\r", __func__, __LINE__);
                return 0;
            }

            //goto dfdf;
            memcpy(m_readbuf, &SerialData.read_buf[0], len);
            ptr = m_readbuf;
            printf("len=%d, ptr1=%s\r\n", len, ptr);
            if ((k = bufstr(ptr, "+CMGR:", len)) >=0)
            {
                printf("k1=%d\r\n", k);
                ptr += k;
                cmplen = len -k;

                printf("len=%d, cmplen=%d\r\n", len, cmplen);
                printf("ptr2=%s\r\n", ptr);

                //k = bufstr(ptr, "\r\n", cmplen);
                k = bufstr(ptr, "\n\n", cmplen);

                printf("k2=%d\r\n", k);
                if (k < 0)
                {
                    goto dfdf;
                }
                ptr =ptr+ k+2;

                while (1)
                {  //chage the sms context format
                    chr = *ptr++;
                    if (chr < 0x10)
                    {
                        break;
                    }
                    ser_printf("%c", chr);
                    // 短消息体接收
                    if (count<200)
                    {  // 长度限制
                        if (recvhigh)
                        {
                            recvhigh=0;
                            buffer[count]=AscToHex(chr)*16;
                        }
                        else
                        {
                            recvhigh=1;
                            buffer[count]=AscToHex(chr)+buffer[count];
                            count++;
                        }
                    }
                }
                ser_printf("\r\n");
                //   goto dfdf;

                //decode sms
                ptr = buffer;         	// 短消息中心号码部分
                ptr = buffer+*buffer+1; // 协议部分
                ptr = ptr+1;          	// 发送者号码部分
                PduNumToNum(sender, ptr);
                ptr = ptr+(*ptr+1)/2+2; // 协议时间部分
                encodemode = *(ptr+1);
                ptr = ptr+9;          	// 短消息内容部分

                if (encodemode == 4)
                {// 8位PDU编码方式
                    ;
                }
                else if (encodemode==8)
                {
                    // UNICODE编码方式
                    ;
                }
                else
                {
                    // 7位编码方式
                    PduDecode(buffer, ptr, 7);
                    ptr=buffer;
                }

                ProtExplainMessage(ptr, sender, encodemode);
            }

            printf("k=%d\r\n", k);
        }
dfdf:
        if (m_readbuf)
        {
            sysfree(m_readbuf);
        }
#if 1
        sp_printf("time delay1:500ms\r\n"); // 使用AT+CMGR指令后延时一段时间以使AT+CMGR回复接收完全
        OSTimeDly(500);
        sprintf(atbuf, "AT+CMGD=%d\r\n", i);

#if 0
        SendATCommand(atbuf);
        ser_printf("%s\r\n", atbuf);
        GetATCommand("OK",GPRS_NORMAL_WAITTIME);
#else
        SendATCmdEx(atbuf, (char *)SerialData.read_buf, 10, "OK");
#endif

#endif
        //g_smsid = i;
    }

#if 0
    if (g_cmgl)
    {
#if 1
        g_cmgl = 0;
        sp_printf("send AT+CMGL=4,1\r\n");
        SendATCommand("AT+CMGL=4,1\r\n");
        if (GetATCommand("+CMGL:", DEFAULT_AT_WAITTIME) > 0)
        {
            sp_printf("time delay2:300ms\r\n"); // 使用AT+CMGL指令后延时一段时间以使AT+CMGL回复接收完全
            OSTimeDly(300);
        }
#endif
    }
#endif

    return 1;
}
*/
/*
type - 删除短信类型
    0 - 未读短信
    1 - 已读短信
    2 - 未发送短信
    3 - 已发送短信
    4 - 所有短信
*//*
static int DeleteAllMessages(int total, int type)
{
#if 0
    int i;
    char    cmd[20];

    if (total<=0)
    {
        return -1;
    }

    for (i=1; i<=total; i++)
    {
        sprintf(cmd, "AT+CMGD=%d\r\n", i);
        SendATCommand(cmd);
        ser_printf(cmd);
        GetATCommand("OK", GPRS_NORMAL_WAITTIME);
    }
#else
    int i, pos = 0;
    int index;
    int TotolOffset = 0;
//    int waittime = 5000*1000;
    char cmd[20];
    uint8_t deletecount;
    //uint8_t j;

    if (type > 4)
    {
        return 0;
    }
    PrintfTime();
    sprintf(cmd, "AT+CMGL=%d\r\n", type);
#if 0
    SendATCommand(cmd);
    //SendATCommand("AT+CMGL=4\r\n");
    ser_printf("DeleteAllMessages... %s\r\n", cmd);
#else
    memset(SerialData.read_buf, 0, sizeof(SerialData.read_buf));
    SendATCmdEx(cmd, (char *)SerialData.read_buf, 10, "OK");
    SerialData.len = strlen((char *)SerialData.read_buf);
#endif

    for (i = 0; i < 10; i++)
    {
#if 0
         ret = GetATCommand("OK", waittime);
         if (ret == -6) //buffer full
            waittime = 100*1000;
         else if (ret >= 0)
           i  = 10;
         else
            waittime = 2000*1000;
         if (SerialData.len  == 0 && waittime == 2000*1000)i = 10;
#endif

        TotolOffset = 0;
        while (1)
        {
           SetDog(TASK_DOG_GPRS);
           WatchDogFeed();
           pos = bufstr(&SerialData.read_buf[TotolOffset], "+CMGL:", SerialData.len - pos);
           if (pos < 0)
             break;
           else
            {
                TotolOffset += pos;
                index = 0;
                if ( GetSmsIndex(TotolOffset, SerialData.len -TotolOffset, &index))
                    PushDelete((uint8_t)index);

                 TotolOffset+=6;
                 if (TotolOffset >=  SerialData.len)break;
            }
        }
    }
    PrintfTime();
    while (1)
    {
         deletecount = PopDelete();
         if (DeleteMessageCount == 0)
            break;
         SetDog(TASK_DOG_GPRS);
         WatchDogFeed();
          sprintf(cmd, "AT+CMGD=%d\r\n", deletecount);
#if 0
         SendATCommand(cmd);
         ser_printf("%s", cmd);
         GetATCommand("OK", GPRS_NORMAL_WAITTIME);
#else
        SendATCmdEx(cmd, (char *)SerialData.read_buf, 10, "OK");
#endif
    }
#endif

    return 0;
}

*/
///////////////////////////////////////////////////////////////////////////////////////////////
/*
*	返回值:
*					0:AT指令测试没有问题	-1:指令测试出错
*/
///////////////////////////////////////////////////////////////////////////////////////////////
/*
static int module_at_check_proc(void)
{
	int count;

	count = 8;
	while (count--)
	{
		ser_printf("send at, count=%d\r\n", count);
		if ( 0 == SendATCmd("AT\r", 10, "OK") )
		{
			ser_printf("has receive atok cmd, count=%d\r\n", count);
			g_module_status_ctl.at_chk_opr.opr_ret = 1;
			return 0;
		}
		else
		{
			ser_printf("not receive atok cmd...\r\n");
			sleep(1);
		}
	}
	g_module_status_ctl.at_chk_opr.opr_ret = -1;
	return -1;
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////
/*
*	返回值:
*					0:sim卡指令测试没有问题	-1:指令测试出错
*/
///////////////////////////////////////////////////////////////////////////////////////////////
/*
static int module_sim_check_proc(void)
{
//	int count;

// TODO:代码待完成
#if 1
	g_module_status_ctl.sim_chk_opr.opr_ret = 1;
	return 0;
#endif
#if 0
	count = 8;
	while (count--)
	{
		ser_printf("send at, count=%d\r\n", count);
		SendATCommand("AT\r\n");
		if (GetATCommand("OK", GPRS_FAST_WAITTIME) > 0)
		{
			ser_printf("has receive atok cmd, count=%d\r\n", count);
			return 0;
		}
		else
		{
			ser_printf("not receive atok cmd...\r\n");
		}
	}
	ser_printf("can not receive atok cmd to reboot gprs module,count=%d\r\n", count);
	return -1;
#endif
}

static int module_cops_opr_proc(void)
{
//	int count;

// TODO:代码待完成
#if 1
	g_module_status_ctl.cops_opr.opr_ret = 1;
	return 0;
#endif
#if 0
	count = 8;
	while (count--)
	{
		ser_printf("send at, count=%d\r\n", count);
		SendATCommand("AT\r\n");
		if (GetATCommand("OK", GPRS_FAST_WAITTIME) > 0)
		{
			ser_printf("has receive atok cmd, count=%d\r\n", count);
			return 0;
		}
		else
		{
			ser_printf("not receive atok cmd...\r\n");
		}
	}
	ser_printf("can not receive atok cmd to reboot gprs module,count=%d\r\n", count);
	return -1;
#endif
}

static int module_creg_opr_proc(void)
{
//	int count;

// TODO:代码待完成
#if 1
	g_module_status_ctl.creg_opr.opr_ret = 1;
	return 0;
#endif
#if 0
	count = 8;
	while (count--)
	{
		ser_printf("send at, count=%d\r\n", count);
		SendATCommand("AT\r\n");
		if (GetATCommand("OK", GPRS_FAST_WAITTIME) > 0)
		{
			ser_printf("has receive atok cmd, count=%d\r\n", count);
			return 0;
		}
		else
		{
			ser_printf("not receive atok cmd...\r\n");
		}
	}
	ser_printf("can not receive atok cmd to reboot gprs module,count=%d\r\n", count);
	return -1;
#endif
}

static int module_para_config(void)
{
	int count;

	// 模块参数设置
	if (0 == g_GprsState.ConfigureReady) //模块未配置
	{
#if 1
		if (0 == g_GprsState.iprReady)
		{
			// Set local baud rate temporarily
			for (count=8; count>0; count--)
			{
#if 0
				SendATCommand("AT+IPR=115200\r\n");
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						g_GprsState.iprReady = 1;
						break;
				}
#else
				if (0 == SendATCmd("AT+IPR=115200\r\n", 10, "OK"))
				{
						g_GprsState.iprReady = 1;
						break;
				}
#endif
			}
			if (0 == count)  // 8次未响应，重启模块
			{
				sp_printf("AT+IPREX error...\r\n");
				g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
				return -1;
			}
		}
#endif

		// 关闭模块回显模式
		for (count=8; count>0; count--)
		{
#if 0
			SendATCommand("ATE0\r\n");
			if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
			{
					break;
			}
#else
			if (0 == SendATCmd("ATE0\r\n", 10, "OK"))
			{
					break;
			}
#endif
		}

		if (0 == count)  // 8次未响应，重启模块
		{
			sp_printf("ATE0 error...\r\n");
			g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
			return -2;
		}

#if 0
		//Set CSD or GPRS for Connection Mode 设置无线连接模式
		for (count=8; count>0; count--)
		{
				SendATCommand("AT+CIPCSGP=1\r\n");	// 0-CSD, 1-GPRS
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						break;
				}
		}
		if (0 == count)  // 8次未响应，重启模块
		{
				sp_printf("AT+CIPCSGP error...\r\n");
				DoGprsPowerOn(GPRS_POWER_ON);
				//DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
				return 0;
		}
#endif

#if 0
		// 设置GPRS连接的CLASS等级
		for (count=8; count>0; count--)
		{
				SendATCommand("AT+CGMSCLASS=10\r\n"); 	// (2,4,8,9,10)
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						break;
				}
		}
		if (0 == count)  // 8次未响应，重启模块
		{
				sp_printf("AT+CGMSCLASS error...\r\n");
				DoGprsPowerOn(GPRS_POWER_ON); //DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
				return 0;
		}
#endif

#if 0
		// 注销PDP上下文, 使模块进入IP INITIAL状态
		if (0 == DeactivePDPContext())
		{
				return 0;
		}

		// 设置单连接/多连接模式
		for (count=8; count>0; count--)
		{
#if (!IP_MUX)   // 单连接
				SendATCommand("AT+CIPMUX=0\r\n"); 	// 0-单连接, 1-多连接
#else   // 多连接
				SendATCommand("AT+CIPMUX=1\r\n"); 	// 0-单连接, 1-多连接
#endif
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						break;
				}
		}
		if (0 == count)  // 8次未响应，重启模块
		{
				sp_printf("AT+CIPMUX error...\r\n");
				DoGprsPowerOn(GPRS_POWER_ON); //DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
				return 0;
		}

		//add IP header when receiving data, the format is "+IPD,(data length)"
		for (count=8; count>0; count--)
		{
				SendATCommand("AT+CIPHEAD=1\r\n");
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						break;
				}
		}
		if (0 == count)  // 8次未响应，重启模块
		{
				sp_printf("AT+CIPHEAD error...\r\n");
				DoGprsPowerOn(GPRS_POWER_ON); //DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
				return 0;
		}

		// not Show Remote IP  Address and Port When Received Data
		for (count=8; count>0; count--)
		{
				SendATCommand("AT+CIPSRIP=0\r\n");
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						break;
				}
		}
		if (0 == count)  // 8次未响应，重启模块
		{
				sp_printf("AT+CIPSRIP error...\r\n");
				DoGprsPowerOn(GPRS_POWER_ON); //DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
				return 0;
		}
#endif

		// set new sms message indication
		for (count=8; count>0; count--)
		{
#if 0
				SendATCommand("AT+CNMI=2,1\r\n");
				if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
				{
						break;
				}
#else
				printf("AT+CNMI=2,1,0,1,0\r\n");
				if (0 == SendATCmd("AT+CNMI=2,1,0,1,0\r\n", 10, "OK"))
				{
					break;
				}
#endif
		}
		if (0 == count)  // 8次未响应，重启模块
		{
				sp_printf("AT+CNMI error...\r\n");
				g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
				return -3;
		}

    for (count=8; count>0; count--)
    {
			// 模块复位后，sim卡可能还未ready，需要等待
      sleep(5);
			if ( 0 == SendATCmdEx("AT+CREG=0\r\n", (char *)SerialData.read_buf, 5, "OK") )
			{
				break;
			}
    }
    if (0 == count)  // 8次未响应，重启模块
		{
			sp_printf("AT+CREG error...\r\n");
			g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
			return -4;
		}

		g_GprsState.ConfigureReady = 1;
	}
	// 参数配置成功，则返回0
	return 0;
}

//extern int Leds_fd;
// oper值与GPRS模块实际输入电平相反
static void GPRSResetPinSet(int oper)
{
#ifdef _use_new_kernel
	if (oper) ioctl(Leds_fd, GPIO_LED_CLOSE_OPR, _ioctl_3G_RESET_CTL);
	else ioctl(Leds_fd, GPIO_LED_OPEN_OPR, _ioctl_3G_RESET_CTL);
#else
	if (oper) ioctl(Leds_fd, 1, _ioctl_3G_RESET_CTL);
	else ioctl(Leds_fd, 0, _ioctl_3G_RESET_CTL);
#endif
}
*/

/*
static void GPRSPowerEnable(int oper)
{
#ifdef _use_new_kernel
	if (oper) ioctl(Leds_fd, GPIO_LED_CLOSE_OPR, _ioctl_3g_power_en);
	else ioctl(Leds_fd, GPIO_LED_OPEN_OPR, _ioctl_3g_power_en);
#else
	if (oper) ioctl(Leds_fd, 1, _ioctl_3g_power_en);
	else ioctl(Leds_fd, 0, _ioctl_3g_power_en);
#endif
	sleep(2);
}
*/

//////////////////////////////////////////////////////////////////////////////
/*
*	Reset pluse: _____			_____ 低电平保持50~100ms
*										|___|
*	返回值:	0:复位成功		-1:复位失败
*/
//////////////////////////////////////////////////////////////////////////////
/*
static int ModuleResetOpr(void)
{
//	uint8_t spin = 0;
//	int ret = -1;
//	int dly, count;
//	uint32_t before, after;

	ser_printf("####%s(%d)####...\r\n", __func__,__LINE__);

	// 3G模块电源使能
	GPRSPowerEnable(1);
	// 关闭tty/USB2,避免再次映射时候无法开启USB2
	CloseGPRS();

	// 1.reset管脚操作
#if 0
    if (0 == SendATCmd("AT+CFUN=1,1\r\n", 10, "OK"))
    {
        ;
    }
    else
    {
        return -1;
    }
#else
	GPRSResetPinSet(0);		// pull up Reset Pin
	SetDog(TASK_DOG_GPRS);
	WatchDogFeed();
	OSTimeDly(80);
	GPRSResetPinSet(1);		// pull down Reset Pin
	sleep(5);
	ser_printf("####%s(%d)####:sleep(5) is over\r\n",__func__,__LINE__);
#endif
#if 0
	// 2.模块状态管脚检测，确定模块已经启动
	for ( count=0; count<30; count++ )
	{
		sleep(1);
		SetDog(TASK_DOG_GPRS);
		WatchDogFeed();
		printf("module reset opr processing,count:%d", count);
		// TODO:状态指示管脚检测
		ret = 0;
		if (ret==0) return 0;
	}
	return -1;
#else
	return 0;
#endif
}



static int module_csq_query_proc(void)
{
//	int count;

//	g_csq = GetCsq();

//	printf("g_csq:%d\r\n",g_csq);
	g_GprsState.CopsState = GPRS_MODULE_STATE_COPS_OK;
//	g_GprsState.qos = 1;
	return 0;

#if 0
	count = 8;
	while (count--)
	{
		ser_printf("send at, count=%d\r\n", count);
		SendATCommand("AT\r\n");
		if (GetATCommand("OK", GPRS_FAST_WAITTIME) > 0)
		{
			ser_printf("has receive atok cmd, count=%d\r\n", count);
			return 0;
		}
		else
		{
			ser_printf("not receive atok cmd...\r\n");
		}
	}
	ser_printf("can not receive atok cmd to reboot gprs module,count=%d\r\n", count);
	return -1;
#endif
}
*/

//1 MU609模块代码如上

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
* Description:
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int DoGprsPowerOn(int mode)
{
//	uint8_t spin;
	int count = 0;
//	int i;
	int b_opr_suc = 0;
//	int bGprsPowOn = 0;
//	int ret = 0;
	uint8_t tmp, tmp1;

	ser_printf("####%s(%d)####:do gprs power on,",__func__,__LINE__);
	PrintfTime();

	// 1.软件喂狗
	SetDog(TASK_DOG_GPRS);
	WatchDogFeed();
	// 2.熄灭联网LED灯
  ClrLedFlashMode(LED_ONLINE, 0);
	// 登入成功标志位清除
  g_TransmitControl.bHaveLogin = 0;
	// 3.保留主控器上一个错误状态、复位出错次数和清除GPRS状态标志位
	tmp = g_GprsState.LastErrorModuleState;
	tmp1 = g_GprsState.query_state_err_cnt;
	ser_printf("LastErrorModuleState:%d,query_state_err_cnt:%d\r\n",\
		g_GprsState.LastErrorModuleState, g_GprsState.query_state_err_cnt);
	// 清除状态标志位
	init_module_status();
	g_GprsState.LastErrorModuleState = tmp;
	g_GprsState.query_state_err_cnt = tmp1;
	// 4.模块状态设为缺省值，开始开机复位操作
	g_GprsState.SIMCardState = GPRS_MODULE_STATE_DAFAULT;
	// 5.1模块复位/开机操作
	ser_printf("DoGprsPowerOn to RESET,start to reset...\r\n");
	b_opr_suc = ModuleResetOpr();
	if (b_opr_suc!=0)
	{	// 复位操作失败
		ser_printf("reset opr is fail...\r\n");
		g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
		return -1;
	}
	// 延时等待/tty/USB2和/tty/USB0设备可操作
	count = _3G_MODULE_RESET_WAIT_SECONDS;
	do {
		if ( F_3g_module_is_access() == 0 )
		{
			ser_printf("3g module /tty/USB can operate and to do open /tty/USB2\r\n");
			// 关闭tty/USB2
			CloseGPRS();
			F_3g_module_device_open();
			break;
		}
		sleep(1);
		count--;
		if (count<=0)
		{
			ser_printf("3g module /tty/USB can't operate,err cnt:%d...\r\n", g_GprsState.query_state_err_cnt);
			if (g_GprsState.query_state_err_cnt>=3)
			{	// 重启系统
				ser_printf("3g module reset opr more than 3 times and reboot system\r\n");
				reboot();
			}
			// 关闭tty/USB2,避免再次映射时候无法开启USB2
			CloseGPRS();
			g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
			return -2;
		}
	}while(1);

	// 5.2AT操作确认
	b_opr_suc = module_at_check_proc();
	if (b_opr_suc!=0)
	{ // AT操作无反馈
		ser_printf("SIMCardState: GPRS_MODULE_STATE_MOD_INVALID...\r\n");
		g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
		return -3;
	}

	// 5.3 sim卡操作确认
	b_opr_suc = module_sim_check_proc();
	if (b_opr_suc!=0)
	{ // AT操作无反馈
		ser_printf("SIMCardState: GPRS_MODULE_STATE_MOD_INVALID...\r\n");
		g_GprsState.SIMCardState = GPRS_MODULE_STATE_MOD_INVALID;
		return -4;
	}

	// 操作均成功，则清除模块复位出错变量，返回0
	g_GprsState.query_state_err_cnt = 0;
	return 0;
}
*/

//////////////////////////////////////////////////////////////////////////////////////////
/* Power on:    T_PullDown>1s; T_status>3.2s
 * 开机成功后spin状态处于1
 */
//////////////////////////////////////////////////////////////////////////////////////////
/*
static int ModulePoweronOpr(void)
{
#if 0
    int ret;
    uint8_t dly, count;
    uint8_t spin = 0;

    ser_printf("%s...\r\n", __func__);
    GPRSResetPinSet(0); // pull up Reset Pin
    GPRSPowPinSet(0);   // pull up PWRKEY
    SetDog(TASK_DOG_GPRS);
    WatchDogFeed();
    OSTimeDly(200);     // delay 200ms
    GPRSPowPinSet(1);   // pull down PWRKEY
    ser_printf("pull down finished, delay 2s...\r\n");
    SetDog(TASK_DOG_GPRS);
    WatchDogFeed();
    sleep(2);   // T_PullDown>1s
    GPRSPowPinSet(0);   // pull up PWRKEY
    ser_printf("pull up finished, delay 3s...\r\n");
    SetDog(TASK_DOG_GPRS);
    WatchDogFeed();
    sleep(3);   // delay time > (3.2-1)s=2.2s

    // delay 10s and detect status pin
    for ( dly=10; dly>0; dly-- )
    {
        // first:detect recv "START"
        ret = GetATCommand("RDY", 1000*1000);
        ser_printf( "getting \"RDY\"2... dly=%d\r\n", dly );
        if (ret > 0) // has got "RDY"
        {
            ser_printf( "has got \"RDY\"... dly1=%d\r\n", dly );
            for ( count=20; count>0; count-- )
            {
                spin = ReadGprsStatus();
                if (spin)
                {   // reset and power on is over
                    ser_printf("Module power on success, Get \"RDY\" string and status pin is high...\r\n");
                    break;
                }
                SetDog(TASK_DOG_GPRS);
                WatchDogFeed();
                OSTimeDly(500);
            }
            break;
        }
        // second:status pin detect
#ifdef _use_module_status_pin_detc
        if (dly <= 5) // 检测5s "RDY"后检测status pin
        {
            spin = ReadGprsStatus();
            if (spin)
            {   // power on is over and success
                ser_printf("Module power on success, and status pin is high...\r\n");
                break;
            }
        }
#endif
    }
    if (!spin)
    {
        ser_printf("\n\rModule power on fail, and status pin is low...\r\n");
    }
    // send cmd use uart check again
    for ( count=0; count<3; count++ )
    {
        SendATCommand("AT\r\n");
        ser_printf("send AT...\r\n");
        if ( GetATCommand("OK", GPRS_FAST_WAITTIME) )
        {
            ser_printf("get OK...\r\n");
            if (0 == g_GprsState.iprReady)
            {
                // Set local baud rate temporarily 重启成功后设置波特率
                SendATCommand("AT+IPR=115200\r\n");
                if (GetATCommand("OK", GPRS_NORMAL_WAITTIME) > 0)
                {
                    g_GprsState.iprReady = 1;
                    ser_printf("set band 115200 ok...\r\n");
                }
            }

            return 1;
        }
    }
#endif
    return 0;
}
*/
//////////////////////////////////////////////////////////////////////////////////////////
/* Power off:   1s<T_PullDown<5s;   T_Delay>1.7s;
 * 关机完成后spin状态应处于0状态
 */
//////////////////////////////////////////////////////////////////////////////////////////
/*
static int ModulePoweroffOpr(void)
{
#if 0
    int ret;
    int poweroff_result = 0;    // 用于返回关闭模块的结果, 0-失败, 1-成功
    uint8_t dly, count;
    uint8_t spin = 1;

    ser_printf("%s...\r\n", __func__);
    GPRSResetPinSet(0); // pull up Reset Pin
    GPRSPowPinSet(0);   // pull up PWRKEY
    SetDog(TASK_DOG_GPRS);
    WatchDogFeed();
    OSTimeDly(200);     // delay 200ms
    GPRSPowPinSet(1);   // pull down PWRKEY
    ser_printf("pull down PWRKEY finished, delay 3s...\r\n");
    SetDog(TASK_DOG_GPRS);
    WatchDogFeed();
    sleep(3);   // 1s<T_PullDown<5s
    GPRSPowPinSet(0);   // pull up PWRKEY
    ser_printf("pull up PWRKEY finished, delay 2s...\r\n\r\n");
    SetDog(TASK_DOG_GPRS);
    WatchDogFeed();
    sleep(2);

    // delay 10s and detect status pin
    for ( dly=10; dly>0; dly-- )
    {
        // first:detect recv "NORMAL POWER DOWN"
        ser_printf( "getting NORMAL POWER DOWN... dly=%d\r\n", dly );
        ret = GetATCommand("NORMAL POWER DOWN", 1000*1000);
        ser_printf( "has got NORMAL POWER DOWN... dly=%d\r\n", dly );
        if ( ret>0 ) // has got "NORMAL POWER DOWN"
        {
            for ( count=20; count>0; count-- )
            {
                spin = ReadGprsStatus();
                if (!spin)
                {   // reset and power off is over
                    ser_printf("Module power off succeed, Get \"GetATCommand NORMAL POWER DOWN\", Status pin is low...\r\n");
                    poweroff_result = 1;
                    break;
                }
                SetDog(TASK_DOG_GPRS);
                WatchDogFeed();
                OSTimeDly(500);
            }
            break;
        }
        // second:status pin detect
#ifdef _use_module_status_pin_detc
        if (dly <= 5) // 5s内未收到"NORMAL POWER DOWN", 开始检测status pin
        {
            spin = ReadGprsStatus();
            if (!spin)   // power off is over and success
            {
                ser_printf("Module power off succeed, Status pin is low...\r\n");
                poweroff_result = 1;
                break;
            }
        }
#endif
    }
    if (spin)   // power off is over and fail
    {
        ser_printf("Module power off fail, Status pin is high...\r\n");
        poweroff_result = 0;
    }

    return poweroff_result;
#endif
    return 0;
}

#ifdef _SEND_SMS

static int SendSMS(uint8_t alert_type)
{
    char cmd[20];
    char pdu[512];		// PDU串
	int ret = 0;
    int nPduLength;
    int len;
    int i;
    SM_PARAM sm_para;

#if 0   // 默认门点名称为"仓库"
    if (strlen(g_SmsPara.PhoneNumber) == 0 || strlen(g_SmsPara.DoorName) == 0)
    {
        return 0;
    }
#else
    if (strlen(g_SmsPara.PhoneNumber) == 0) // 手机号码未设置, 退出
    {
        return 0;
    }
#endif

    sm_para.index = 0;
    sm_para.SCA[0] = '\0';  // 不指定SCA, 使用AT+CSCA里的值
    strcpy(sm_para.TPA, g_SmsPara.PhoneNumber); // 目标号码
    //strcpy(sm_para.TPA, "13588805114"); // 目标号码
    sm_para.TP_DCS = GSM_UCS2;  // 中文USC2编码
    sm_para.TP_PID = 0x00;
    //sm_para.TP_SCTS = 0x00;

    ser_printf("%s, alert_type=%d\r\n", __func__, alert_type);
    if (alert_type > ALERT_TYPE_MAX_NO)
    {
        return 0;
    }

    if (0 == alert_type)    // 开门记录
    {
        if (strlen(g_SmsPara.DoorName) == 0)    // 门点名称未设置, 只发送'仓库'
        {
            sprintf(pdu, "您好，仓库门已打开，请关注。");
        }
        else
        {
            sprintf(pdu, "您好，%s号仓库门已打开，请关注。", g_SmsPara.DoorName);
        }
    }
    else    // 告警记录
    {
        if (strlen(g_SmsPara.DoorName) == 0)    // 门点名称未设置, 只发送'仓库'
        {
            sprintf(pdu, "您好，仓库发生%s，请关注。", alert_content[alert_type]);
        }
        else
        {
            sprintf(pdu, "您好，%s号仓库发生%s，请关注。", g_SmsPara.DoorName, alert_content[alert_type]);
        }
    }
    strcpy(sm_para.TP_UD, pdu);    // 短信内容

    ser_printf("PhoneNum:%s, Content:%s\r\n", sm_para.TPA, sm_para.TP_UD);

    nPduLength = gprsEncodePdu(&sm_para, pdu);	// 根据PDU参数，编码PDU串

    len = strlen(pdu);
    ser_printf("nPduLength:%d\r\npdu:%s\r\n", nPduLength, pdu);

    //while (1)
    {
        //if ( QueryModuleState() != 1 )
        {
            //OSTimeDly(500);
            //continue;
        }

        sprintf(cmd, "AT+CMGS=%d\r\n", nPduLength);
        ser_printf("%s", cmd);
        SendATCommand(cmd);

        if (GetATCommand(">", DEFAULT_AT_WAITTIME) <= 0)
        {
            ser_printf("wait \">\" time out.\r\n");
        }
        else
        {
            ser_printf("have got > \r\n");
            //sleep(1);
            pdu[len++] = 0x1A;
            pdu[len] = '\0';
            //strcat(pdu, "\x1A");

            ser_printf("len:%d\r\n", len);
            ser_printf("buf:");
            for (i=0; i<len; i++)
            {
                ser_printf("%02X ", pdu[i]);
            }
            ser_printf("\r\n");

            //g_testsend = 1;
            WriteComPort((uint8_t *)pdu, len);   //send data
            //g_testsend= 0;

            ser_printf("send cmd finished.\r\n");

            if (GetATCommand("OK", DEFAULT_AT_WAITTIME) <= 0)
            {
                ser_printf("send sms fail.\r\n");

                ret = 0;
            }
            else
            {
                ser_printf("send sms success.\r\n");

                ret = 1;
            }
        }

        //break;
    }

    return ret;
}
#endif

static int F_3g_module_is_access()
{
	// 斜杠/
	if ( (0 == access("/dev/ttyUSB0", F_OK)) && (0 == access("/dev/ttyUSB2", F_OK)) ) return 0;
	else return -1;
}
*/
/* 初始化GPRS模块状态 */
/*
static void init_module_status(void)
{
  memset(&g_GprsState, 0, sizeof(g_GprsState));
	g_ppp_status = _ppp_default;
//  ClrLedFlashMode(LED_GPRS, 0);		// 开机LED灯默认灭
  //DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
  //DoGprsPowerOn(GPRS_POWER_RESET_FORCED);
  //g_GprsState.bGprsNetOpen = 0;
}
*/
/* 移植GPRSDrv.c部分代码，如上*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	功能:	判断GPRS连接是否建立
*	返回:	TRUE:是; FALSE:否
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
static char *PppLinkExist(void)
{
	FILE	*fp;
	char	buf[256];

	fp = popen("ifconfig ppp0 |grep \"inet addr\"", "r");
	fgets(buf, sizeof(buf), fp);
	pclose(fp);

	return	strstr(buf, "inet");
}


// TODO:3G模块硬件管脚复位操作
static int F_3g_module_reset_opr(void)
{
	return 0;
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* extern function prototype,定义 开始*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	Function:
*	Description:
*	Input:
*	Output:
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int32_t F_DialUp(int mode)
{
	int32_t	counter;
//	char tty[32], cmd[128]={0};
	int ret_hungup, ret_dialup;

	printf("F_DialUp, g_ppp_status=%d\r\n", g_ppp_status);

	// 若已经拨号了，则先挂断拨号
	switch (mode)
	{
		case _PPP_DIAL_NORMAL:
			break;

		case _PPP_DIAL_FORCE:
			{
				printf("F_DialUp, F_HungUp\r\n");
				// 先PPP挂断
				ret_hungup = F_HungUp();
			}
			break;

		default:
			break;
	}
	printf("####%s(%d)####:to doing ppp dialup...,start time:\r\n",__func__,__LINE__);
	PrintfTime();

	// 1.拨号操作
	system("pppd call wcdma &");
	g_ppp_status = _ppp_processing;

	// 2.检查ppp拨号是否成功
	ret_dialup = -1;
	counter = _PPP_DIAL_WAIT_SECONDS;
	do
	{
		sleep(1);
		if (0 == access("/var/run/ppp0.pid", F_OK))
		{
			if ( PppLinkExist() )
			{	// ppp拨号成功
				printf("####%s(%d)####:ppp dialup is over...,counter=%d,end time:\r\n",\
					__func__,__LINE__,counter);
				PrintfTime();
				printf("dial up success\n");
				g_ppp_status = _ppp_connect;
				// ppp0设备关闭和重新设备gw
//				strncpy(cmd, "ifconfig ppp0 down", sizeof(cmd));
//				system(cmd);
//				strncpy(cmd, "ifconfig ppp0 up", sizeof(cmd));
//				system(cmd);
//				strncpy(cmd, "route add default gw 10.64.64.64", sizeof(cmd));
//				system(cmd);
				return 0;
			}
		}
	}
	while (--counter != 0);
	// 3.若拨号失败，则置位标志位后，挂断该拨号
	printf("####%s(%d)####:ppp dialup is over...,counter=%d,end time:\r\n",\
		__func__,__LINE__,counter);
	PrintfTime();
	printf("dial up failed\n");
	g_ppp_status = _ppp_err_1;
	F_HungUp();
	return ret_dialup;
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*	功能:	ppp挂断, 直接杀进程
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int F_HungUp(void)
{
	int32_t ret=-1, ctr;

	if ( 0 == access("/var/run/ppp0.pid", F_OK) )
	{
		printf("F_HungUp, kill `cat /var/run/ppp0.pid`\r\n");
		system("kill `cat /var/run/ppp0.pid`");
		printf("kill ppp0.pid and sleep 30s\r\n");

		sleep(_PPP_DIAL_WAIT_SECONDS);

		ctr = _PPP_DIAL_WAIT_SECONDS;
		do	// TestGprsWork()耗时1S,总共(2+1)*30=90S,确保退出数据模式,进入命令模式
		{
			sleep(2);
			ret = TestGprsWork();
		}
		while (ret != 0 && --ctr != 0);
	}
	else return -1;

	// 挂断该拨号成功
	if ( ret == 0 )
	{
		// TODO:GPRS数据流量统计
		//GprsDatatota();
	}
	remove("/var/run/pppd2.tdb");	// ppp拨号时此文件会增大,最大14M,满时可能拨号失败
	return ret;
}

static void ppp_status_proc()
{
	uint32_t t_cur_sec;
	static uint32_t t_last_sec = 0;

	// 模块状态异常，则退出
	if (g_GprsState.SIMCardState != GRPS_MODULE_STATE_NORMAL) return;

	t_cur_sec = GetRtcTime();
	if ( t_cur_sec >= t_last_sec && t_cur_sec-t_last_sec < 3 ) return;

	// 每隔3s进行一次PPP拨号运行状态查询和处理
	t_last_sec = t_cur_sec;
	printf("\r\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
  printf("####%s####,time:", __func__);
	PrintfTimeDec(t_cur_sec);
	printf("\r\n");
	ser_printf("g_GprsState.SIMCardState=%d, g_TransmitControl.bHaveLogin=%d,g_g_ppp_status=%d\r\n",\
		g_GprsState.SIMCardState, \
		 g_TransmitControl.bHaveLogin,g_ppp_status);
	printf("\r\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
	switch(g_ppp_status)
	{
		case _ppp_default:
			F_DialUp(_PPP_DIAL_FORCE);
			break;

		case _ppp_idle:
			break;

		case _ppp_err_1:
			F_DialUp(_PPP_DIAL_FORCE);
			break;

		default:
			break;
	}
}

static int F_3g_module_device_open()
{
	if ( OpenGPRS(115200, CTSRTS_NO) < 0 )
	{
		perror("Open 3g module ttyUSB2 error and reboot\n\r");
		reboot();
	}
	return 0;
}

int CheckSMS(char *buf)
{
	char	DataBuff[SOCKET_READ_SIZE+1];
//	char	*pch, *pch2;
	uint8_t    *ptr;
//	int32_t	ret;
	int32_t	index = -1;

	int cmplen, readlen, k;

	printf("start to CheckSMS...\r\n");
	if (NULL == buf)
	{
		memset(DataBuff, '\0', sizeof(DataBuff));
		//usleep(60000);
		readlen = RecvFromGprs(DataBuff, sizeof(DataBuff));
		printf("readlen=%d, RingInt=%s\n", readlen, DataBuff);
		ptr = (uint8_t *)&DataBuff[0];
	}
	else
	{
		readlen = strlen(buf);
		ptr = (uint8_t *)buf;
	}

	if (readlen)
	{
		//ptr = (uint8_t *)&DataBuff[0];
		cmplen = readlen;

		k = bufstr(ptr, "+CMTI:", cmplen);
		if (k >= 0)
		{
			printf("k1=%d, ", k);
			ptr += k;
			cmplen -= k;

			k = bufstr(ptr, ",", cmplen);
			if (k >= 0)
			{
				printf("k2=%d, ", k);

				index = 0; ptr+=k+1;
				while (1)
				{
					SetDog(TASK_DOG_GPRS);
					WatchDogFeed();

					if (*ptr < 0x10)  // not number as 0x0d, 0x0a
					{
						break;
					}
					index = index*10+AscToHex(*ptr++);
				}
				PushDelete(index);
				ser_printf("PushDelete, index=%d\r\n", index);
				ser_printf( "receive message id:%d\r\n", index);
			}
		}
	}
	return 0;
}

static void F_sms_check()
{
	uint32_t t_cur_sec;
	static uint32_t t_last_sec = 0;

	// 模块状态异常，则退出
	if (g_GprsState.SIMCardState != GRPS_MODULE_STATE_NORMAL) return;

	t_cur_sec = GetRtcTime();
	if ( t_cur_sec >= t_last_sec && t_cur_sec-t_last_sec < 3 ) return;

	// 3s检测一次串口
	//RingPinIntFunc();
	CheckSMS(NULL);
	DoRecvGprsData();
	//g_csq = GetCsq();
	//printf("g_csq=%d\r\n", g_csq);

	//(void)SendATCmd("AT+CNMI?\r", TIMEOUT_1S, STR_OK);

}
*/
//返回值请注意清理
static char* GetDns(void)
{
	FILE *pf = fopen("/tmp/resolv.conf.auto", "r");
	char *dns = NULL;

	if (pf) {
		char tBuf[300] = {0};

		while (fgets(tBuf, 299, pf)) {
			if (tBuf[0] == '#') {
				continue;
			}
			int tLen = strlen(tBuf) - 1;

			// 从末尾开始检查，是0-9或.则跳出，否则去除
			while (tLen >= 0) {
				if ((tBuf[tLen]>='0' && tBuf[tLen]<='9') || tBuf[tLen]=='.') {
					break;
				} else {
					tBuf[tLen] = '\0';
				}
				tLen--;
			}

			if (memcmp(tBuf, "nameserver ", 11) == 0) {
				if (strlen(tBuf+11)>6 && memcmp(tBuf+11, "127.0.0.1", 9)!=0) {
					dns = strdup(tBuf+11);
					break;
				}
			}

		}

		fclose(pf);
	}

	return dns;
}

typedef struct SmsInfo {
	struct SmsInfo *Next;
	uint16_t	Index;
	uint8_t		Stat;
	uint16_t	TpduLen;
	uint16_t	PduLen;
	uint8_t		Pdu[256];
} sSmsInfo;

// 记得释放
static sSmsInfo *GetSms(const char *buf)
{
	if(buf == NULL) {
		return NULL;
	}
	int tLen=0, StrLen=strlen(buf);
	const char *tStr = buf;
	sSmsInfo *RetSms=NULL, **pnSms=&RetSms;

	while ((tStr=strstr(tStr, "+CMGL:"))) {
		int Index=-1, Stat=-1, TpduLen=-1, PduLen=-1;

		tLen = 6;
		while (tStr[tLen]) {
			char *pe = NULL;

			if (Index < 0) {
				if (isdigit(tStr[tLen])) {
					Index = strtol(tStr+tLen, &pe, 10);
					if (*pe != ',') {
						Index = -1;
					} else {
						tLen = pe-tStr+1;
						continue;
					}
				}
			} else if (Stat < 0) {
				if (isdigit(tStr[tLen])) {
					Stat = strtol(tStr+tLen, &pe, 10);
					if (*pe != ',') {
						Stat = -1;
					} else {
						do {
							pe++;
						} while (*pe && *pe != ',');

						tLen = pe-tStr;
						continue;
					}
				}
			} else if (TpduLen < 0) {
				if (isdigit(tStr[tLen])) {
					TpduLen = strtol(tStr+tLen, &pe, 10);
					tLen = pe-tStr+1;
					continue;
				}
			} else {
				if (isxdigit(tStr[tLen])) {
					uint8_t *tPdu=alloca(StrLen);

					PduLen = Str2Hex(tStr+tLen, tPdu);
					if (PduLen>0 && PduLen<=256) {
						*pnSms = calloc(1, sizeof(sSmsInfo));
						if (*pnSms) {
							pnSms[0]->Index		= Index;
							pnSms[0]->Stat		= Stat;
							pnSms[0]->TpduLen	= TpduLen;
							pnSms[0]->PduLen	= PduLen;
							memcpy(pnSms[0]->Pdu, tPdu, PduLen);
							pnSms = &(pnSms[0]->Next);
						}
						// 失败也跳过了该短信

						tLen += PduLen*2;
						//
						break;
					}
				}
			}
			tLen++;
		}
		tStr += tLen;
	}

	return RetSms;
}

static int64_t FindImsi(const char *buf)
{
	if (buf==NULL) {
		return -1;
	}
	int i=0, NumLen=0, StrLen=strlen(buf);

	while (i<StrLen) {
		NumLen=0; //数字的长度

		while((i+NumLen < StrLen) && isdigit(buf[i+NumLen])) {
			NumLen++;
		}

		if (NumLen>10) {
			break;
		} else {
			i+=NumLen+1;
		}
	}
	if (NumLen>5) {
		return strtoll(buf+i, NULL, 10);
	} else {
		return -1;
	}
}

static int FindRssi(const char *buf)
{
	if (buf == NULL) {
		return -1;
	}
	char *p = strstr(buf, "+CSQ:");
	int Ret = -1;

	if (p) {
		char *pe = NULL;

		Ret = 5;
		while (isblank(p[Ret])) {
			Ret++;
		}

		Ret = strtol(p+Ret, &pe, 10);
		if (*pe != ',') {
			Ret = -1;
		}
	}

	return Ret;
}


int TestDns(const char *dns, double to)
{
	if (dns == NULL) {
		return -1;
	}
	int s = -1;
	int flag = 1;
	struct sockaddr_in a = {0};
	uint8_t tBuf[1000] = {0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
						  0x00, 0x00, 0x00, 0x00, 0x07, 0x65, 0x6e, 0x67,
						  0x69, 0x6e, 0x68, 0x7a, 0x03, 0x63, 0x6f, 0x6d,
						  0x00, 0x00, 0x01, 0x00, 0x01
						 };


	s = socket(AF_INET, SOCK_DGRAM, 0);

	if (s < 0) {
//		printf("Failed[%d] to socket, errno=%d\n", s, errno);
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	a.sin_family		= AF_INET;
	a.sin_addr.s_addr	= INADDR_ANY;
	a.sin_port			= htons(11111);

	if (bind(s, (struct sockaddr*)&a, sizeof(a)) != 0) {
//		printf("Failed to bind, errno=%d\n", errno);
		close(s);
		return -1;
	}

	a.sin_addr.s_addr	= inet_addr(dns);
	a.sin_port			= htons(53);
	if (sendto(s, tBuf, 29, 0, (struct sockaddr*)&a, sizeof(a)) == 29) {
		double st = BooTime();

		do {
			int Ret = recvfrom(s, tBuf, 1000, MSG_DONTWAIT, NULL, NULL);

			if (Ret > 0) {
				flag = 0;
				break;
			} else if (errno != EWOULDBLOCK) {
				break;
			}
			usleep(100000);
		} while (to<0 || (st+to > BooTime()));
	}

	close(s);

	return flag==0?0:-1;
}

// 向fd中发送SendLen的SendBuf（如果有），然后等待回应保存在最大长度为BufLen的RecvBuf中，出错、超时、满了或者检测到eAck则返回
static int WrtieThenRead(int fd, const void *SendBuf, int SendLen, double TimeOut, uint8_t *RecvBuf, int BufLen, char *eAck)
{
	if (fd<0 || ((SendBuf==NULL || SendLen<=0) && (RecvBuf==NULL || BufLen<=0))) {
		return -1;
	}
	int Ret=0, RecvLen=0;
	double st=BooTime(), dt=TimeOut;

	if (SendBuf && SendLen>0) {
		if (RecvBuf && BufLen>0) {
			tcflush(fd, TCIOFLUSH);
		}
		Ret = write(fd, SendBuf, SendLen);
		if (Ret != SendLen) {
			Log("NM", LOG_LV_DEBUG, "Send nLen=%d, rLen=%d, errno=%d, Data:%s.", SendLen, Ret, errno, (char*)SendBuf);
			return -1;
		}
	}
	if (RecvBuf && BufLen>0) {
		Ret = 1;
		do {
			if (Ret <= 0) {
				usleep(100000);
			}
			struct pollfd fds = {0};
			int to = 0;

			fds.fd		= fd;
			fds.events	= POLLIN;
			if (dt > 0) {
				to = dt>=1?1000:(int)(dt*1000);
			} else {
				to = 0;
			}

			poll(&fds, 1, to);
			if ((fds.revents & POLLIN)) {
				Ret = read(fd, RecvBuf+RecvLen, BufLen-RecvLen);
				if (Ret > 0) {
					RecvLen += Ret;
					if (eAck && RecvLen<BufLen) {
						RecvBuf[RecvLen] = 0;
						if (strstr((char *)(RecvBuf+RecvLen-Ret), eAck)) {
							break;
						}
					}
				}
			} else {
				Ret = 0;
			}
		} while (BufLen>RecvLen && (dt = st+TimeOut-BooTime())>=0);
	}
//	Log("NM", LOG_LV_DEBUG, "Send=%.*s, Recv=%.*s", SendLen, SendBuf, RecvLen, RecvBuf);

	return RecvLen;
}

void GsmModlPwrRst(int t)
{
	if (t < 3) t = 3;
	Log("NM", LOG_LV_DEBUG, "Gsm Module Pwr Reset, Dly=%d.", t);
	GpioSet(GPIO_3G_PWR, 0);
	sleep(t);
	GpioSet(GPIO_3G_PWR, 1);
}

void *NetManageThread(void *Arg)
{
	int Ret = -1;
	double LastPingT=0-PING_INTVL, LastImsiT=0-IMSI_INTVL, LastRssiT=0-RSSI_INTVL, LastSmsT=0-SMS_INTVL;
//	char *DefDns = "114.114.114.114";
	int PingCnt=0, FPing=0;
	char AtStr[100] = {0};
	uint8_t RecvBuf[10000]= {0};
	int64_t Imsi=-1;

	AtcDevPath = GetCfgStr(CFG_FTYPE_SYS, "Sys", "AtcDevPath", "/dev/ttyUSB2");
	if (AtcDevPath == NULL) {
		Log("NM", LOG_LV_DEBUG, "Failed to GetCfgStr");
		AtcDevPath = strdup("/dev/ttyUSB2");
	}
	Log("NM", LOG_LV_DEBUG, "AtcDevPath=%s", AtcDevPath);
	//TODO 一些初始化
	char *p = GetCfgStr(CFG_FTYPE_CFG, "Sys", "PoweronDelay", "3");
	int Dly = strtol(p, NULL, 10);
	if (p) {
		free(p);
		p = NULL;
	}
	// GSM模块上电复位
	GsmModlPwrRst(Dly);
	while (1) {
		// 网络连通性检测
		if (PingCnt>=PING_MAX || (LastPingT+PING_INTVL < BooTime())) {
			// TODO 流量统计与其接合
			char * dns = GetDns();

//			Ret = Ping4(dns?dns:"114.114.114.114", 12, PING_TIMEO);
			Ret = TestDns(dns?dns:"114.114.114.114", PING_TIMEO);
			Log("NM", LOG_LV_DEBUG, "TestDns dest=%s, ret=%d, cnt=%d.", dns, Ret, PingCnt);
			if (dns) {
				free(dns);
			}
			if (Ret) {
				PingCnt++;
			} else {
				LedsCtrl(LED_3G_NET, LED_OPEN_OPR);
				PingCnt = 0;
			}

			if (PingCnt >= PING_MAX) {
				if (LastPingT+PING_INTVL < BooTime()) {
					// 在超时的清空下需要干预，重新上电3G模块
					LedsCtrl(LED_3G_NET, LED_CLOSE_OPR);
					GsmModlPwrRst(Dly);
					Ret = 0;
					FPing++;
					if (FPing >= 10) {
						FPing = 10;
					}
					// ÖØÆô¶¯ÏµÍ³(ÍøÂçÒì³£ÈÝ´í´¦Àí£¬³ýÁËÉÏµçµÚÒ»´Î)		
					if (FPing > 1) {
						Log("NM", LOG_LV_WARN, "NetStat detc cnt is more than PING_MAX, reboot ctrl.");
						reboot();
					}
				}
				PingCnt = PING_MAX;
			}
			if (Ret == 0) {
				LastPingT = BooTime();
			}
		}
		// 打开AT设备
		int fd = open(AtcDevPath, O_RDWR);

		if (fd > 0) {
			// 打开AT设备，并设置为非回显
			snprintf(AtStr, 99, "ATE0\n");
			Ret = WrtieThenRead(fd, AtStr, strlen(AtStr), 0.5, RecvBuf, 9999, NULL);
		} else {
			LastImsiT 	= 0-IMSI_INTVL;
			LastRssiT	= 0-RSSI_INTVL;
			LastSmsT	= 0-SMS_INTVL;
			sleep(1);
			continue;
		}

		if (LastImsiT+IMSI_INTVL < BooTime()) {
			// 需要查询IMSI
			snprintf(AtStr, 99, "AT+CIMI\n");
			Ret = WrtieThenRead(fd, AtStr, strlen(AtStr), 1, RecvBuf, 9999, "OK");
			Imsi = -1;
			if (Ret >= 0) {
				RecvBuf[Ret] = '\0';
//				fprintf(stderr, "CIMI, %s\n", RecvBuf);
				Imsi = FindImsi((const char*)RecvBuf);
			}
			Log("NM", LOG_LV_DEBUG, "Imsi=%lld.", Imsi);
			if (Imsi > 0) {
				// TODO 不安全，有可能真实的ISMI未上传
				Log("NM", LOG_LV_DEBUG, "StoreImsi=%lld.", strtoll(g_SystermInfo.StoreImsi, NULL, 10));
				if (strtoll(g_SystermInfo.StoreImsi, NULL, 10) != Imsi) {
					RemoteSendImsiChangedNotify(Imsi);
				}
				LastImsiT = BooTime();
			}
		}

		if (LastRssiT+RSSI_INTVL < BooTime()) {
			// 需要查询RSSI
			snprintf(AtStr, 99, "AT+CSQ\n");
			Ret = WrtieThenRead(fd, AtStr, strlen(AtStr), 1, RecvBuf, 9999, "OK");
			if (Ret >= 0) {
				RecvBuf[Ret] = '\0';
				int tRssi = FindRssi((const char*)RecvBuf);
//				fprintf(stderr, "CSQ, %s\n", RecvBuf);
				if (tRssi > 31) {
					tRssi = 0;
				}
				if (tRssi >= 0) {
					g_Rssi = tRssi;
					LastRssiT = BooTime();
				}
			}
			Log("NM", LOG_LV_DEBUG, "Ret=%d, Rssi=%d.", Ret, g_Rssi);
		}

		if (LastSmsT+SMS_INTVL < BooTime()) {
			// 需要读取短信
			snprintf(AtStr, 99, "AT+CMGL=4\n");
			Ret = WrtieThenRead(fd, AtStr, strlen(AtStr), 1, RecvBuf, 9999, "OK");
			sSmsInfo *pSms = NULL;
			if (Ret >= 0) {
				RecvBuf[Ret] = '\0';
//				fprintf(stderr, "CMGL, %s\n", RecvBuf);
				pSms = GetSms((const char*)RecvBuf);
				LastSmsT = BooTime();
			}
			while (pSms) {
				sSmsInfo *p = pSms->Next;
				char tStr[pSms->PduLen*3+1];

				Hex2Str(tStr, pSms->Pdu, pSms->PduLen, 0);
				Log("NM", LOG_LV_DEBUG, "SMS info:i=%hu, s=%hhu, tl=%hu, l=%hu, d=%s.", pSms->Index, pSms->Stat, pSms->TpduLen, pSms->PduLen, tStr);

				// 解析短信
				ProtExplainMessage(pSms->Pdu, pSms->PduLen);
				//该短信已解析， 可以删除
				snprintf(AtStr, 99, "AT+CMGD=%d\n", pSms->Index);
				Ret = WrtieThenRead(fd, AtStr, strlen(AtStr), 0.5, RecvBuf, 9999, "OK");
				free(pSms);
				pSms = p;
				SrvOnline();
			}
		}
		close(fd);
	}
}
