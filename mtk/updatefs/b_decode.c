#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXSIZE		1024
#define B_PATH		"b"
#define NEW_PATH	"a"
unsigned char SetTable[] = "8893005288930052";
int TableLen = 0;
struct text
{
	int len;
	unsigned char b_buf[1024];
};

void PackDeCodeAndSum(struct text *buf, unsigned long *sum);

int main(int argc, char **argv)
{
#if 1
	if(argc != 2)
	{
		printf("Usage: ./main<b>!\n");
		exit(-1);
	}
#endif
	int new_fd = -1, old_fd = -1;
	struct text bin_file;
	unsigned char headinfo[64] = {'\0'};
	unsigned char new_file[64] = {'\0'};
	unsigned long checksum = 0;
	unsigned long checkinfo = 0;
	struct stat buf;
	TableLen = strlen(SetTable);
	bzero(&bin_file, sizeof(struct text));
	if((old_fd = open(argv[1], O_RDONLY)) == -1)
	{
		perror("Fail to open the encode bin!");
		exit(-1);
	}
	if(fstat(old_fd, &buf) == -1)
	{
		perror("Fail to fstat!");
		exit(-1);
	}
	if((new_fd = open(NEW_PATH, O_RDWR | O_CREAT | O_TRUNC, 0777)) == -1)
	{
		perror("Fail to create the new bin file!");
		exit(-1);
	}
	if(ftruncate(new_fd, buf.st_size - 64) == -1)
	{
		perror("Fail to ftruncate !");
		exit(-1);
	}
	if(read(old_fd, headinfo, 64) != 64)
	{
		perror("Fail to read the headinfo!");
		exit(-1);
	}
	memcpy(&checkinfo, &headinfo[32], 4);
	while(1)
	{
		int len = read(old_fd, bin_file.b_buf, MAXSIZE);
		if(len == -1)
		{
			perror("Fail to read the body of bin file!");
			exit(-1);
		}
		bin_file.len = len;
		PackDeCodeAndSum(&bin_file, &checksum);
		if(write(new_fd, bin_file.b_buf, bin_file.len) != len)
		{
			perror("Fail to write date to new bin file!");
			exit(-1);
		}
		if(len != MAXSIZE)
		{
			if(checksum != checkinfo)
			{
				printf("Updata error!\n");
			}
			break;
		}
	}
	return 0;
}

void PackDeCodeAndSum(struct text *buf, unsigned long *sum)
{
	int i;
	for(i=0; i<buf->len; i++)
	{
		buf->b_buf[i] = buf->b_buf[i] ^ SetTable[i % TableLen];
		*sum += buf->b_buf[i];
	}
}
