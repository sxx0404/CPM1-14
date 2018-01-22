/*
*********************************************************************************************************
*                                                                 (C) Copyright 2009
*                                                    Copyright hangzhou engin Inc , 2002
 *                                                                 All rights reserved.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*
*
* Filename      :
* Version       : V1.00
* Programmer(s) :
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/




/* ----------------- APPLICATION GLOBALS ------------------ */






/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/



/*
the security layer code, des encode and decode is in des.cpp
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <ucos_ii.h>
#include "aes.h"
#include "stdafx.h"
#include "ProtocolDefine.h"
#include "ManageSecurity.h"
#include "ManageTransmit.h"
#include "NormalTool.h"
#include "DES.h"
#include "BusinessProtocol.h"
#include "main_linuxapp.h"
#include "../include/log.h"

char cveci[10] = {0};

void SecurityCountAdd(void)
{
	ulong count;
//	 OS_CPU_SR cpu_sr;
		ConvertMemcpy(&count , &g_TransmitControl.SecurityCounter[1], 4);

	count++;
		ConvertMemcpy(&g_TransmitControl.SecurityCounter[1], &count , 4);

	if (count == 0) {
//	 	OS_CPU_SR cpu_sr;
				g_TransmitControl.SecurityCounter[0]++;
			}

}


int SecurityLayerEncode(uint8_t *PaddingData, int len, int EncryptMode, uint8_t *out, int *outlen)
{
	if (ProtVer/16 == 2) {
		typSecurityLayer se;
		int i, cc_len, cntr_len;
		uchar key[16];
		uchar * crcin = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
		uchar pcntr;

		//se.spi = 0x09;
		se.spi = 0x1d;          //000:reserve: 11: count over 1:encrypt  01: RC

		if (COUNTER_TYPE_NONE == ((se.spi>>3) & 0x3)) {	// 00 COUNTER_TYPE_NONE
			cntr_len = 0;
		} else {
			cntr_len = 5;
		}

		if ((se.spi>>2) & 0x1) {	// EncryptMode
			switch (EncryptMode) {
			case ENCRYPT_MODE_DES: {
				se.kic = (DES_MODE_CBC/*THDES_MODE_CBC_2KEY*/<<2) | ENCRYPT_MODE_DES;
			}
			break;

			case ENCRYPT_MODE_SM1: {
				se.kic = ENCRYPT_MODE_SM1;
			}
			break;

			default:
				se.kic = 0x0;
				break;
			}
		} else {
			se.kic = 0x0;
		}
		se.kid = 0x5;	// 00000101 CRC32 Check
		if (0x0 == se.kid) {	// checksum check
			cc_len = 1;
		} else if (0x5 == se.kid) {	// CRC32 Check
			cc_len = 4;
		} else if (0x1 == se.kid) {	// CRC16 Check
			cc_len = 2;
		}
		SecurityCountAdd();

		if (0 == ((se.spi>>2) & 0x1) || 0 == (5+1+cc_len+len)%8) {
			pcntr = 0;
		} else {
			pcntr = 8-(5+1+cc_len+len)%8;
		}

		if (cntr_len) {
			memset(&se.cntr[0], 0x0, cntr_len);
		}
		se.cntr[4] = 1;
		se.pcntr = pcntr;

		if (0x0 == se.kid) {	// checksum check
			*se.cc = se.spi + se.kic + se.kid;
			for (i = 0; i < 5; i++) {
				*se.cc += se.cntr[i];
			}
			*se.cc += se.pcntr;
			for (i = 0; i < len ; i++) {
				*se.cc += PaddingData[i];
			}
		} else if (0x5 == se.kid) {	// CRC32 Check
			memcpy(crcin, (uchar*)&(se.spi), 1+1+1+cntr_len);
			crcin[1+1+1+cntr_len] = se.pcntr;
			memcpy(crcin+(1+1+1+cntr_len+1), PaddingData, len);
			if (pcntr > 0) {
				memset(crcin+(1+1+1+cntr_len+1)+len, 0x0, pcntr);
			}

			*((unsigned long*)se.cc) = CRC32(crcin, len+(1+1+1+cntr_len+1)+pcntr);
		} else if (0x1 == se.kid) {	// CRC16 Check
			memcpy(crcin, (uchar*)&(se.spi), 1+1+1+cntr_len);
			crcin[1+1+1+cntr_len] = se.pcntr;
			memcpy(crcin+(1+1+1+cntr_len+1), PaddingData, len);
			if (pcntr > 0) {
				memset(crcin+(1+1+1+cntr_len+1)+len, 0x0, pcntr);
			}
			*((unsigned long*)se.cc) = CRC16(crcin, len+(1+1+1+cntr_len+1)+pcntr);
		}

		if (cntr_len) {
			memcpy(&out[6], &se.cntr[0], cntr_len);
		}
		out[6+cntr_len] = se.pcntr;

		ConvertMemcpy(&out[6+cntr_len+1], se.cc, cc_len);
		memcpy(&out[6+cntr_len+1+cc_len], &PaddingData[0], len);
		if (pcntr > 0) {
			memset(&out[6+cntr_len+1+cc_len+len], 0x0, pcntr);
		}
		len = cntr_len+1+cc_len+len;
		if (((se.spi>>2) & 0x1) && len%8) {	// encrpe
			len =len/8*8+8;
		}

		if ((se.spi>>2) & 0x1) {	// EncryptMode
			uchar num=0;
			int p_total;
			printf("key=");
			for (p_total=0; p_total<16; p_total++) {
				if (p_total<8) {
					num+=0x11;
				} else if (p_total > 8) {
					num-=0x11;
				}
				key[p_total] = num;
				printf("%02X", num);
			}
			printf("\r\n");

			if (ENCRYPT_MODE_DES == EncryptMode) {
				if (DES_MODE_CBC == ((se.kic>>2) & 0x3)) {
					printf("DES_MODE_CBC...\r\n");
					DesCbc2keyEncode(&out[6], len, key, cveci);
				} else if (THDES_MODE_CBC_2KEY == ((se.kic>>2) & 0x3)) {
					printf("THDES_MODE_CBC_2KEY...\r\n");
					ThDesCbc2keyEncode(&out[6], len, key, cveci);
				}
			}
		}

		se.chl = 1+1+1+cntr_len+1+cc_len;  // not contarn itself
		se.cpl = 1+1+1+1+len;
		ConvertMemcpy(out,(uchar *) &se.cpl, 2);
		out[2] = se.chl;
		out[3] = se.spi;
		out[4] = se.kic;
		out[5] = se.kid;
		*outlen = len+6;
		sysfree(crcin);
		return (TRSMT_STATE_OK);

	} else {
		uchar key[16];
		uchar spi = 0;
		uchar pcntr;
		uchar head_len;
		ushort packet_len;
		ushort crc16;
		int cc_len;
		uchar * crcin = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
		int i;
		//int cntr_len;

		ser_printf("len=%d\r\n", len);

		//spi = 0x21; // 00100001: 存在CRC16字段, 默认密钥, DES-CBC加密
		//spi = 0x22; // 00100010: 存在CRC16字段, 默认密钥, 3DES-CBC加密
		//spi = 0x25; // 00100101: 存在CRC16字段, 默认密钥, AES-CBC加密
		// spi = (0x20 | (AES_MODE_CBC & 0x7)); // 00100 000+EncryptMode(b0-b2), 存在CRC16字段, 默认密钥, 加密类型由EncryptMode指定

		cc_len = 2;

		head_len = 1;
		out[2+head_len] = spi;
		head_len++;

		SecurityCountAdd();

		if (0 == (spi & 0x7)) { // 不加密或者用户数据长度是8的倍数, 不需要补齐
			pcntr = 0;
		} else if (AES_MODE_CBC == (spi & 0x7)) { // AES, 16字节对齐
			if (0 == (len%16)) {
				pcntr = 0;
			} else {
				pcntr = 16-len%16;
			}
		} else { // 其他加密方式, 8字节对齐
			if (0 == (len%8)) {
				pcntr = 0;
			} else {
				pcntr = 8-len%8;
			}
		}

		if (spi & 0x7) {   // 存在加密, 有PADDING字段
			out[2+head_len] = pcntr;
			head_len += 1;
		} else { // 不加密, 无PADDING字段
			;
		}

		if ((spi>>5) & 0x1) { // 存在CRC16校验字段, 计算CRC16值并存入报文头中
			crc16 = CRC16(PaddingData, len);
			cc_len = 2;
			ConvertMemcpy(&out[2+head_len], (uchar *)&crc16, cc_len);
			head_len += cc_len;
		} else {     // 不存在CRC16校验字段, 无CRC16值
			;
		}

		/* head extension */

		/* 复制User Data */
		memcpy(&out[2+head_len], PaddingData, len);
		/* 填充字节0 */
		if (pcntr > 0) {
			memset(&out[2+head_len+len], 0x0, pcntr);
			//len += pcntr;
		}
		len += pcntr;   // 补齐之前的数据总长度

		if (spi & 0x7) {	// EncryptMode
			if (0x0 == ((spi>>3)&0x3)) {	// the default key
				uchar num=0;
				int p_total;
				printf("key=");
				for (p_total=0; p_total<16; p_total++) {
					if (p_total<8) {
						num+=0x11;
					} else if (p_total > 8) {
						num-=0x11;
					}
					key[p_total] = num;
					printf("%02X", num);
				}
				printf("\r\n");
			}

			printf("encrypt in (%d)=", len);
			for (i=0; i<len; i++) {
				printf("%02X", out[2+head_len+i]);
			}
			printf("\r\n");

			if (DES_MODE_CBC == (spi&0x7)) {	// DES-CBC
				if (len%8) { // 8字节对齐
					//len =len/8*8+8; // len是待加密数据总长度
				}

				printf("DES_MODE_CBC...\r\n");
				DesCbc2keyEncode(&out[2+head_len], len, key, cveci);
			} else if (THDES_MODE_CBC_2KEY == (spi&0x7)) {	//3DES-CBC
				if (len%8) { // 8字节对齐
					//len =len/8*8+8; // len是待加密数据总长度
				}

				printf("THDES_MODE_CBC_2KEY...\r\n");
				ThDesCbc2keyEncode(&out[2+head_len], len, key, cveci);
			} else if (AES_MODE_CBC == (spi & 0x7)) { // AES
				if (len%16) { // 16字节对齐
					//len =len/16*16+16; // len是待加密数据总长度
				}

				printf("AES_MODE_CBC...\r\n");
				AES_CBC_Encode(&out[2+head_len], len, key);
			} else { // 其他加密方式, 暂未实现
				printf("Unknown_Encrypt_type(0x%X)...\r\n", (spi & 0x7));
			}

			printf("encrypt out (%d)=", len);
			for (i=0; i<len; i++) {
				printf("%02X", out[2+head_len+i]);
			}
			printf("\r\n");
		} else { // 不加密
			printf("No_Encrypt...\r\n");
		}

		packet_len = 2+head_len+len;  // contarn itself
		ConvertMemcpy(out, (uchar *)&packet_len, 2);
		out[2] = head_len;
		out[3] = spi;
		*outlen = packet_len;
		sysfree(crcin);
		return TRSMT_STATE_OK;
	}
}

// V2.*
int SecurityLayerDecode(uint8_t *SecurityData, uint8_t *out, int *len)
{
	if (SecurityData==NULL || out==NULL || len==NULL || len[0]<4) {
		return -1;
	}

	if (ProtVer/16 == 2) {
		uchar EncryptMode;
		//uchar	shlen;
		//uchar	key[16];
		ushort salen;
		ushort DataLen;
		uchar *SecuredData;
		typSecurityLayer se;

		ConvertMemcpy((uchar *)&salen, &SecurityData[0], 2);
		if (len[0] < salen+2) {
			printf("SecurityLayerDecode error, len diff!\r\n");

			return -1;
		}


		//shlen = SecurityData[2];
		se.chl = SecurityData[2];   // CHL
		se.spi = SecurityData[3];   //should check if the type is support
		se.kic = SecurityData[4];
		se.kid = SecurityData[5];
//	DesCbc2keyDecode(&SecurityData[6], salen-6, key);

		if (se.spi>>2 & 0x1) { // EncryptMode
			EncryptMode = (se.kic & 0x3);
			if (ENCRYPT_MODE_DES == EncryptMode) {
				if (DES_MODE_CBC == (se.kic>>2 & 0x3)) {
					printf("DES_MODE_CBC...\r\n");
					DesCbc2keyDecode(&SecurityData[6], salen-1-1-1-1, g_SystermInfo.ManagePassword, cveci);
				} else if (THDES_MODE_CBC_2KEY == (se.kic>>2 & 0x3)) {
					printf("THDES_MODE_CBC_2KEY...\r\n");
					ThDesCbc2keyDecode(&SecurityData[6], salen-1-1-1-1, g_SystermInfo.ManagePassword, cveci);
				}
			}
		}

		memcpy(&se.cntr[0], &SecurityData[6], 5);
		se.pcntr = SecurityData[6+5];  //
		DataLen = salen-(1+se.chl);
		SecuredData = &SecurityData[2+1+se.chl];

		if (0x0 == se.kid) { // checksum check
			*se.cc = SecurityData[12];

			uchar 	checksum;
			int		i;
			checksum = se.spi + se.kic + se.kid;
			for (i = 0; i < 5; i++) {
				checksum += se.cntr[i];
			}
			checksum += se.pcntr;
			for (i = 0; i < DataLen ; i++) {
				checksum += SecuredData[i];
			}

			if (*se.cc == checksum) { // 校验成功, 开始复制实际数据
				DataLen -= se.pcntr;     // 去除对齐数据

				memcpy(out, SecuredData, DataLen);
				*len = DataLen;
				return (TRSMT_STATE_OK);
			}

			/*if(*se.cc == CheckSum(&SecurityData[3], salen -3))  //from spi
			{
				memcpy(out, &SecurityData[13], salen -13);
				*len = salen -13;
				return (TRSMT_STATE_OK);
			}*/
		} else if (0x5 == se.kid) {// CRC32 Check
			uchar * crcin = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
			memcpy(crcin, (uchar*)&(se.spi), 9);
			memcpy(crcin+9, SecuredData, DataLen);

			ConvertMemcpy(se.cc, &SecurityData[12], se.chl-1-1-1-5-1);

			if (*((unsigned long*)se.cc) == CRC32(crcin, 9+DataLen)) {  // 校验成功, 开始复制实际数据
				DataLen -= se.pcntr;    // 去除对齐数据

				memcpy(out, SecuredData, DataLen);
				*len = DataLen;
				sysfree(crcin);
				return (TRSMT_STATE_OK);
			}
			sysfree(crcin);
		} else if (0x1 == se.kid) {// CRC16 Check
			uchar * crcin = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
			memcpy(crcin, (uchar*)&(se.spi), 9);
			memcpy(crcin+9, SecuredData, DataLen);
//		memcpy(se.cc, CRC16(crcin, len+9), cc_len);
			ConvertMemcpy(se.cc, &SecurityData[12], se.chl-1-1-1-5-1);
			if ( *((unsigned long*)se.cc) == CRC16(crcin, 9+DataLen) ) {  // 校验成功, 开始复制实际数据
				DataLen -= se.pcntr;      // 去除对齐数据

				memcpy(out, SecuredData, DataLen);
				*len = DataLen;
				sysfree(crcin);
				return (TRSMT_STATE_OK);
			}
			sysfree(crcin);
		}
		return TRSMT_STATE_ERR_UNKNOWN;
	} else {
		uchar shlen;
		uchar spi = 0;
		uchar padding_val = 0;
		ushort salen;
		ushort DataLen;
		ushort crc16 = 0;
		int padding = 0;
		uchar *SecuredData;

		printf("use key:");
		for (int i = 0; i < 16; i++) {
			printf("%02x ", g_SystermInfo.ManagePassword[i]);
		}
		printf("\n");
		ConvertMemcpy((uchar *)&salen, &SecurityData[0], 2);    // Packet Length
		if (salen > len[0]) {
			return -1;
		}
		shlen = SecurityData[2];    // CHL
		spi = SecurityData[3];   //should check if the type is support
		if (shlen+2 > salen) {
			return -1;
		}

		if (spi & 0x7) {	// EncryptMode

			if (0 == ((spi>>3) & 0x3)) {	// the default key
				;
			} else if (1 == ((spi>>3) & 0x3)) {	// the specified key 1
				;
			}

			if (DES_MODE_CBCV3 == (spi & 0x7)) {
				printf("DES_MODE_CBC...\r\n");
				DesCbc2keyDecode(&SecurityData[2+shlen], salen-2-shlen, g_SystermInfo.ManagePassword, cveci);
			} else if (THDES_MODE_CBC_2KEYV3 == (spi & 0x7)) {
				printf("THDES_MODE_CBC_2KEY...\r\n");
				ThDesCbc2keyDecode(&SecurityData[2+shlen], salen-2-shlen, g_SystermInfo.ManagePassword, cveci);
			} else if (AES_MODE_CBC == (spi & 0x7)) { // AES
				printf("AES_MODE_CBC...\r\n");
				AES_CBC_Decode(&SecurityData[2+shlen], salen-2-shlen, g_SystermInfo.ManagePassword);
			} else {
				printf("unknown encrype mode...\r\n");
			}

			padding_val = SecurityData[4];  // PADDING值
			padding = 1;
		} else {
			printf("no encrype...\r\n");
		}

		SecuredData = &SecurityData[2+shlen];   // 解密后的User Data数据头
		if (salen < 2+shlen+padding_val) {
			return -1;
		}
		DataLen = salen-(2+shlen)-padding_val;  // 解密后减去PADDING的CRC16待校验的User Data长度

		ser_printf("padding=%d, padding_val=%d\r\n", padding, padding_val);
		ser_printf("DataLen=%d\r\n", DataLen);

		if ((spi>>5) & 0x1) {	// 存在CRC16字段, 校验CRC16值
			uchar * crcin = sysmalloc(MAX_BUSINESS_SINGLE_PACK_LEN);
			memcpy(crcin, SecuredData, DataLen);

//		memcpy(se.cc, CRC16(crcin, len+9), cc_len);
			ConvertMemcpy((uchar *)&crc16, &SecurityData[2+1+1+padding], 2);    // crc16值


			if (crc16 == CRC16(crcin, DataLen)) {
				printf("crc16 check OK.\r\n");

				memcpy(out, SecuredData, DataLen);
				*len = DataLen;
				sysfree(crcin);
				return (TRSMT_STATE_OK);
			} else {
				printf("crc16 check fail.crc16=0x%04x, CRC16()=0x%04x\r\n", crc16, CRC16(crcin, DataLen));
			}
			sysfree(crcin);
		} else { // 不存在CRC16字段, 不校验直接返回原始数据
			memcpy(out, SecuredData, DataLen);
			*len = DataLen;
			return (TRSMT_STATE_OK);
		}
		return (TRSMT_STATE_ERR_UNKNOWN);
	}
}

