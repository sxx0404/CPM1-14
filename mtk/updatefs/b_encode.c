#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXSIZE		(1024)
// 文件头添加信息
#define SOFT_VER_OFFSET			(6)						// 2个字节
#define HARD_VER_OFFSET			(13)					// 2个字节
#define CHKSUM_OFFSET			(0X20)					// 4个字节
#define FILESIZE_OFFSET			(0X24)					// 4个字节
#define VALIDFLAG_OFFSET		(0X28)					// 55 55 AA AA
#define ENCODETYPE_OFFSET		(0X2C)					// 0:表示不加密	1:表示异或算法加密
#define DEVTYPE_OFFSET			(0X2D)					// 0:表示主控器	1:表示门锁

typedef unsigned char uchar;
typedef unsigned long ulong;


unsigned char SetTable[] = "8893005288930052";
int TableLen = 0;
struct text {
	int len;
	unsigned char b_buf[1024];
};

void PackEnCodeAndSum(struct text *buf, unsigned long *sum, int bEncodetype);
int StringResolve(uchar *s, uchar *msb, uchar *lsb);

int main(int argc, char **argv)
{
	if(argc != 5) {
		printf("Input format: ./b <ProcFlieName> <SoftVer etc 3.0> <HardwareVer etc 14.0> <EncodeType>\n");
		exit(EXIT_FAILURE);
	}
	unsigned char headinfo[64] = {'\0'};
	unsigned char new_file[128] = {'\0'};
	unsigned long checksum = 0;
	struct text	bin_file = {0};
	struct stat buf;
	uchar hwmsb,hwlsb,swmsb,swlsb;
	int bEncodeType=0;
	int old_fd = -1, new_fd = -1;

	TableLen = strlen(SetTable);

	StringResolve(argv[2], &swmsb, &swlsb);			// 软件版本号
	StringResolve(argv[3], &hwmsb, &hwlsb);			// 硬件版本号

	if((old_fd = open(argv[1], O_RDONLY)) == -1) {
		perror("Fail to open the bin file!");
		exit(EXIT_FAILURE);
	}
	if(fstat(old_fd, &buf) == -1) {
		perror("Fail to fstat the bin_file!");
		exit(EXIT_FAILURE);
	}
	printf("File size:%ld\n",buf.st_size);
	switch(argv[4][0]) {
	default:
	case '0':
		bEncodeType = 0;
		sprintf(new_file, "%s%c%s", argv[1], '_', "NoEncode");
		break;
	case '1':
		bEncodeType = 1;
		sprintf(new_file, "%s%c%s", argv[1], '_', "XorEncode");
		break;
	}
	if((new_fd = open(new_file, O_RDWR | O_TRUNC | O_CREAT, 0777)) == -1) {
		perror("Fail to create the new_file!");
		exit(EXIT_FAILURE);
	}
	if(ftruncate(new_fd, buf.st_size + 64) == -1) {
		perror("Fail to ftruncate the new_binfile!");
		exit(EXIT_FAILURE);
	}
	if(lseek(new_fd, 64, SEEK_SET) == -1) {
		perror("Fail to lseek !");
		exit(EXIT_FAILURE);
	}
	while(1) {
		int len = read(old_fd, bin_file.b_buf, MAXSIZE);
		if(len == -1) {
			perror("Fail to read the bin file!");
			exit(EXIT_FAILURE);
		}
		bin_file.len = len;
		PackEnCodeAndSum(&bin_file, &checksum, bEncodeType);
		if(write(new_fd, bin_file.b_buf, bin_file.len) != bin_file.len) {
			perror("Fail to write the bin to new_file!");
			exit(EXIT_FAILURE);
		}
		if(bin_file.len < MAXSIZE) {
			headinfo[SOFT_VER_OFFSET]=swlsb;
			headinfo[SOFT_VER_OFFSET+1]=swmsb;
			headinfo[HARD_VER_OFFSET]=hwlsb;
			headinfo[HARD_VER_OFFSET+1]=hwmsb;
			memcpy(&headinfo[CHKSUM_OFFSET], &checksum, 4);
			memcpy(&headinfo[FILESIZE_OFFSET], (uchar*)&buf.st_size, 4);
			headinfo[VALIDFLAG_OFFSET]=0x55;
			headinfo[VALIDFLAG_OFFSET+1]=0x55;
			headinfo[VALIDFLAG_OFFSET+2]=0xaa;
			headinfo[VALIDFLAG_OFFSET+3]=0xaa;
			headinfo[ENCODETYPE_OFFSET]=bEncodeType;
			headinfo[DEVTYPE_OFFSET]=0;
			if(lseek(new_fd, 0, SEEK_SET) == -1) {
				perror("Fail to lseek!");
				exit(EXIT_FAILURE);
			}
			if(write(new_fd, &headinfo[0], 48) != -1) {
				break;
			} else {
				perror("Fail to write!");
				exit(EXIT_FAILURE);
			}
		}
		bzero(&bin_file, sizeof(struct text));
	}
	return 0;
}

void PackEnCodeAndSum(struct text *buf, unsigned long *sum, int bEncodetype)
{
	int i;
	for(i=0; i<buf->len; i++) {
		*sum += buf->b_buf[i];
		switch(bEncodetype) {
		case 0:
			break;

		case 1:
			buf->b_buf[i] = buf->b_buf[i] ^ SetTable[i % TableLen];
			break;

		default:
			break;
		}
	}
}

int StringResolve(uchar *s, uchar *msb, uchar *lsb)
{
	int i,j=0,len;
	long tmp;
	uchar buf[32]= {'\0'};

	//printf("\n\r ---s:%s",s);
	j = strlen(s);
	if(j==0) return -1;
	//printf("\n\rj:%d",j);
	for(i=0; i<j; i++) {
		if(s[i]=='.') break;
	}
	len = i;
	//printf("\n\rlen1:%d",len);
	memcpy(buf,s,len);
#if 1
	buf[len]='\0';
	tmp = atol(buf);
	*msb=(uchar)tmp;
	len = j-(i+1);
	memcpy(buf,s+i+1,len);
	buf[len]='\0';
	tmp = atol(buf);
	*lsb=(uchar)tmp;
	return 0;
#endif
}

