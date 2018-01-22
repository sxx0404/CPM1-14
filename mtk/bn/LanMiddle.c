#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <pthread.h>

#include "stdafx.h"
#include "LanMiddle.h"

#define RECORD_FILE  "/home/record.bin"

static int record_fd;
static pthread_rwlock_t fl;

/**
* 关于flash
*
*/

void InitRecordFile(void)
{
	pthread_rwlock_init(&fl, NULL);
	record_fd = open(RECORD_FILE, O_RDWR);
	if(record_fd < 0) {
		//没有这文件
		if((record_fd = open(RECORD_FILE, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1) {
			printf("Record file can not creat\r\n");
		} else {
			ftruncate(record_fd, 2048);
		}
//		//初始化6兆空间
//		uchar buff[10240];
//		memset(buff, 0xff, 10240);
//		int i = 0;
//		for(i = 0; i < 600; i++) {
//			write(record_fd, buff, 10240);
//		}
	} else {
		//
		printf("Record file is exist\r\n");
	}
}


uint16_t Flash_EmulateE2prom_WriteByte(const void* pBuffer, uint32_t WriteAddrOffset, uint32_t NumByteToWrite)
{
	pthread_rwlock_wrlock(&fl);
	lseek(record_fd, WriteAddrOffset, SEEK_SET);
	write(record_fd, pBuffer, NumByteToWrite);
	pthread_rwlock_unlock(&fl);
	return (NumByteToWrite);
}

void Flash_EmulateE2prom_ReadByte(void* pBuffer, uint32_t ReadAddrOffset, uint32_t NumByteToRead)
{
	pthread_rwlock_rdlock(&fl);
	lseek(record_fd, ReadAddrOffset, SEEK_SET);
	read(record_fd, pBuffer, NumByteToRead);
	pthread_rwlock_unlock(&fl);
}



