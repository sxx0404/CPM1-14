#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <linux/types.h>

struct erase_info_user {
	__u32 start;
	__u32 length;
};

struct erase_info_user64 {
	__u64 start;
	__u64 length;
};

struct mtd_oob_buf {
	__u32 start;
	__u32 length;
	unsigned char  *ptr;
};

struct mtd_oob_buf64 {
	__u64 start;
	__u32 pad;
	__u32 length;
	__u64 usr_ptr;
};

/**
 * MTD operation modes
 *
 * @MTD_OPS_PLACE_OOB:	OOB data are placed at the given offset (default)
 * @MTD_OPS_AUTO_OOB:	OOB data are automatically placed at the free areas
 *			which are defined by the internal ecclayout
 * @MTD_OPS_RAW:	data are transferred as-is, with no error correction;
 *			this mode implies %MTD_OPS_PLACE_OOB
 *
 * These modes can be passed to ioctl(MEMWRITE) and are also used internally.
 * See notes on "MTD file modes" for discussion on %MTD_OPS_RAW vs.
 * %MTD_FILE_MODE_RAW.
 */
enum {
	MTD_OPS_PLACE_OOB = 0,
	MTD_OPS_AUTO_OOB = 1,
	MTD_OPS_RAW = 2,
};

/**
 * struct mtd_write_req - data structure for requesting a write operation
 *
 * @start:	start address
 * @len:	length of data buffer
 * @ooblen:	length of OOB buffer
 * @usr_data:	user-provided data buffer
 * @usr_oob:	user-provided OOB buffer
 * @mode:	MTD mode (see "MTD operation modes")
 * @padding:	reserved, must be set to 0
 *
 * This structure supports ioctl(MEMWRITE) operations, allowing data and/or OOB
 * writes in various modes. To write to OOB-only, set @usr_data == NULL, and to
 * write data-only, set @usr_oob == NULL. However, setting both @usr_data and
 * @usr_oob to NULL is not allowed.
 */
struct mtd_write_req {
	__u64 start;
	__u64 len;
	__u64 ooblen;
	__u64 usr_data;
	__u64 usr_oob;
	__u8 mode;
	__u8 padding[7];
};

#define MTD_ABSENT		0
#define MTD_RAM			1
#define MTD_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_DATAFLASH		6
#define MTD_UBIVOLUME		7
#define MTD_MLCNANDFLASH	8

#define MTD_WRITEABLE		0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		0x1000	/* No erase necessary */
#define MTD_POWERUP_LOCK	0x2000	/* Always locked after reset */

/* Some common devices / combinations of capabilities */
#define MTD_CAP_ROM		0
#define MTD_CAP_RAM		(MTD_WRITEABLE | MTD_BIT_WRITEABLE | MTD_NO_ERASE)
#define MTD_CAP_NORFLASH	(MTD_WRITEABLE | MTD_BIT_WRITEABLE)
#define MTD_CAP_NANDFLASH	(MTD_WRITEABLE)

/* Obsolete ECC byte placement modes (used with obsolete MEMGETOOBSEL) */
#define MTD_NANDECC_OFF		0	// Switch off ECC (Not recommended)
#define MTD_NANDECC_PLACE	1	// Use the given placement in the structure (YAFFS1 legacy mode)
#define MTD_NANDECC_AUTOPLACE	2	// Use the default placement scheme
#define MTD_NANDECC_PLACEONLY	3	// Use the given placement in the structure (Do not store ecc result on read)
#define MTD_NANDECC_AUTOPL_USR 	4	// Use the given autoplacement scheme rather than using the default

/* OTP mode selection */
#define MTD_OTP_OFF		0
#define MTD_OTP_FACTORY		1
#define MTD_OTP_USER		2

struct mtd_info_user {
	__u8 type;
	__u32 flags;
	__u32 size;	/* Total size of the MTD */
	__u32 erasesize;
	__u32 writesize;
	__u32 oobsize;	/* Amount of OOB data per block (e.g. 16) */
	__u64 padding;	/* Old obsolete field; do not use */
};

struct region_info_user {
	__u32 offset;		/* At which this region starts,
				 * from the beginning of the MTD */
	__u32 erasesize;	/* For this region */
	__u32 numblocks;	/* Number of blocks in this region */
	__u32 regionindex;
};

struct otp_info {
	__u32 start;
	__u32 length;
	__u32 locked;
};

/*
 * Note, the following ioctl existed in the past and was removed:
 * #define MEMSETOOBSEL           _IOW('M', 9, struct nand_oobinfo)
 * Try to avoid adding a new ioctl with the same ioctl number.
 */

/* Get basic MTD characteristics info (better to use sysfs) */
#define MEMGETINFO		_IOR('M', 1, struct mtd_info_user)
/* Erase segment of MTD */
#define MEMERASE		_IOW('M', 2, struct erase_info_user)
/* Write out-of-band data from MTD */
#define MEMWRITEOOB		_IOWR('M', 3, struct mtd_oob_buf)
/* Read out-of-band data from MTD */
#define MEMREADOOB		_IOWR('M', 4, struct mtd_oob_buf)
/* Lock a chip (for MTD that supports it) */
#define MEMLOCK			_IOW('M', 5, struct erase_info_user)
/* Unlock a chip (for MTD that supports it) */
#define MEMUNLOCK		_IOW('M', 6, struct erase_info_user)
/* Get the number of different erase regions */
#define MEMGETREGIONCOUNT	_IOR('M', 7, int)
/* Get information about the erase region for a specific index */
#define MEMGETREGIONINFO	_IOWR('M', 8, struct region_info_user)
/* Get info about OOB modes (e.g., RAW, PLACE, AUTO) - legacy interface */
#define MEMGETOOBSEL		_IOR('M', 10, struct nand_oobinfo)
/* Check if an eraseblock is bad */
#define MEMGETBADBLOCK		_IOW('M', 11, __kernel_loff_t)
/* Mark an eraseblock as bad */
#define MEMSETBADBLOCK		_IOW('M', 12, __kernel_loff_t)
/* Set OTP (One-Time Programmable) mode (factory vs. user) */
#define OTPSELECT		_IOR('M', 13, int)
/* Get number of OTP (One-Time Programmable) regions */
#define OTPGETREGIONCOUNT	_IOW('M', 14, int)
/* Get all OTP (One-Time Programmable) info about MTD */
#define OTPGETREGIONINFO	_IOW('M', 15, struct otp_info)
/* Lock a given range of user data (must be in mode %MTD_FILE_MODE_OTP_USER) */
#define OTPLOCK			_IOR('M', 16, struct otp_info)
/* Get ECC layout (deprecated) */
#define ECCGETLAYOUT		_IOR('M', 17, struct nand_ecclayout_user)
/* Get statistics about corrected/uncorrected errors */
#define ECCGETSTATS		_IOR('M', 18, struct mtd_ecc_stats)
/* Set MTD mode on a per-file-descriptor basis (see "MTD file modes") */
#define MTDFILEMODE		_IO('M', 19)
/* Erase segment of MTD (supports 64-bit address) */
#define MEMERASE64		_IOW('M', 20, struct erase_info_user64)
/* Write data to OOB (64-bit version) */
#define MEMWRITEOOB64		_IOWR('M', 21, struct mtd_oob_buf64)
/* Read data from OOB (64-bit version) */
#define MEMREADOOB64		_IOWR('M', 22, struct mtd_oob_buf64)
/* Check if chip is locked (for MTD that supports it) */
#define MEMISLOCKED		_IOR('M', 23, struct erase_info_user)
/*
 * Most generic write interface; can write in-band and/or out-of-band in various
 * modes (see "struct mtd_write_req"). This ioctl is not supported for flashes
 * without OOB, e.g., NOR flash.
 */
#define MEMWRITE		_IOWR('M', 24, struct mtd_write_req)

/*
 * Obsolete legacy interface. Keep it in order not to break userspace
 * interfaces
 */
struct nand_oobinfo {
	__u32 useecc;
	__u32 eccbytes;
	__u32 oobfree[8][2];
	__u32 eccpos[32];
};

struct nand_oobfree {
	__u32 offset;
	__u32 length;
};

#define MTD_MAX_OOBFREE_ENTRIES	8
#define MTD_MAX_ECCPOS_ENTRIES	64
/*
 * OBSOLETE: ECC layout control structure. Exported to user-space via ioctl
 * ECCGETLAYOUT for backwards compatbility and should not be mistaken as a
 * complete set of ECC information. The ioctl truncates the larger internal
 * structure to retain binary compatibility with the static declaration of the
 * ioctl. Note that the "MTD_MAX_..._ENTRIES" macros represent the max size of
 * the user struct, not the MAX size of the internal struct nand_ecclayout.
 */
struct nand_ecclayout_user {
	__u32 eccbytes;
	__u32 eccpos[MTD_MAX_ECCPOS_ENTRIES];
	__u32 oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};

/**
 * struct mtd_ecc_stats - error correction stats
 *
 * @corrected:	number of corrected bits
 * @failed:	number of uncorrectable errors
 * @badblocks:	number of bad blocks in this partition
 * @bbtblocks:	number of blocks reserved for bad block tables
 */
struct mtd_ecc_stats {
	__u32 corrected;
	__u32 failed;
	__u32 badblocks;
	__u32 bbtblocks;
};

/*
 * MTD file modes - for read/write access to MTD
 *
 * @MTD_FILE_MODE_NORMAL:	OTP disabled, ECC enabled
 * @MTD_FILE_MODE_OTP_FACTORY:	OTP enabled in factory mode
 * @MTD_FILE_MODE_OTP_USER:	OTP enabled in user mode
 * @MTD_FILE_MODE_RAW:		OTP disabled, ECC disabled
 *
 * These modes can be set via ioctl(MTDFILEMODE). The mode mode will be retained
 * separately for each open file descriptor.
 *
 * Note: %MTD_FILE_MODE_RAW provides the same functionality as %MTD_OPS_RAW -
 * raw access to the flash, without error correction or autoplacement schemes.
 * Wherever possible, the MTD_OPS_* mode will override the MTD_FILE_MODE_* mode
 * (e.g., when using ioctl(MEMWRITE)), but in some cases, the MTD_FILE_MODE is
 * used out of necessity (e.g., `write()', ioctl(MEMWRITEOOB64)).
 */
enum mtd_file_modes {
	MTD_FILE_MODE_NORMAL = MTD_OTP_OFF,
	MTD_FILE_MODE_OTP_FACTORY = MTD_OTP_FACTORY,
	MTD_FILE_MODE_OTP_USER = MTD_OTP_USER,
	MTD_FILE_MODE_RAW,
};

/** 函数说明：  16进制数据转字符串
 *  参数：
 *  @StrBuf     用来保存转换后字符串的指针，空间要能装下转换后的字符串（含'\0'）
 *  @Data       16进制数据
 *  @Len		16进制数据的长度（字节为单位）
 *  @Seg        字符串中显示字节间的间隔符，如果不可打印，则无间隔符
 *  返回值：    成功返回转换后字符串的长度，不包括'\0'
 */
static int Hex2Str(char *StrBuf, const void *Data, uint32_t Len, char Seg)
{
    if (StrBuf==NULL) {
        return -1;
    } else if (Data==NULL || Len == 0) {
		StrBuf[0] = '\0';
		return 0;
	}
    uint32_t i=0, StrLen=0;
    const uint8_t *p=Data;

    for (i=0; i<Len-1; i++) {
        sprintf(StrBuf+StrLen, "%02X", p[i]);
        StrLen += 2;
        if (isprint(Seg)) {
            StrBuf[StrLen++] = Seg;
        }
    }
    sprintf(StrBuf+StrLen, "%02X", p[i]);
    StrLen += 2;

    return StrLen;
}

int main(int argc, char *argv[])
{
	int Fd = -1;
	uint8_t Mac[6] = {0};

	if (argc==2 && strcmp(argv[1], "r")==0) {
		Fd = open("/dev/mtd2ro", O_RDONLY);
		if (Fd < 0) {
			printf("Failed to open /dev/mtd2ro, errno=%d\n", errno);
			return -1;
		}
		if (lseek(Fd, 4, SEEK_SET) != 4) {
			printf("Failed to lseek, errno=%d\n", errno);
			return -1;
		}
		if (read(Fd, Mac, 6) != 6) {
			printf("Failed to read, errno=%d\n", errno);
			return -1;
		}
		printf("rMAC=%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);
		return 0;
	} else if (argc==3 && strcmp(argv[1], "w")==0 && strlen(argv[2])==12) {
		// 读传递参数
		if (sscanf(argv[2], "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", Mac, Mac+1, Mac+2, Mac+3, Mac+4, Mac+5) != 6) {
			printf("Wrong Mac:%s, need:AABBCCDDEEFF\n", argv[2]);
			return -1;
		}
		Fd = open("/dev/mtd2", O_RDWR);
		if (Fd < 0) {
			printf("Failed to open /dev/mtd2, errno=%d\n", errno);
			return -1;
		}
		// 读分区信息
		struct mtd_info_user info = {0};
		if (ioctl(Fd, MEMGETINFO, &info) || info.erasesize<=0) {
			printf("Failed to MEMGETINFO, es=%X, errno=%d\n", info.erasesize, errno);
			return -1;
		}

		// 读出原分区数据
		uint8_t *tData = NULL;

		tData = malloc(info.erasesize*sizeof(uint8_t));
		if (tData == NULL) {
			printf("Failed to malloc, errno=%d\n", errno);
			return -1;
		}

		if (lseek(Fd, 0, SEEK_SET) != 0) {
			printf("Failed to lseek, errno=%d\n", errno);
			return -1;
		}
		if (read(Fd, tData, info.erasesize) != info.erasesize) {
			printf("Failed to read, errno=%d\n", errno);
			return -1;
		}

		// 擦除数据
		struct erase_info_user era = {0};

		era.start	= 0;
		era.length	= info.erasesize;
		if (ioctl(Fd, MEMERASE, &era)) {
			printf("Failed to MEMERASE, es=%X, errno=%d\n", info.erasesize, errno);
			return -1;
		}

		// 写入数据
		char tStr[info.erasesize*3+1];

		Hex2Str(tStr, tData, info.erasesize, ' ');
		printf("oMAC=%.300s\n", tStr);

		memcpy(tData+4, Mac, 6);
		if (lseek(Fd, 0, SEEK_SET) != 0) {
			printf("Failed to lseek, errno=%d\n", errno);
			return -1;
		}
		if (write(Fd, tData, info.erasesize) != info.erasesize) {
			printf("Failed to write, errno=%d\n", errno);
			return -1;
		}
		free(tData);
		tData = NULL;
		printf("wMAC=%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);

		// 再读出来看看
		if (lseek(Fd, 4, SEEK_SET) != 4) {
			printf("Failed to lseek, errno=%d\n", errno);
			return -1;
		}
		if (read(Fd, Mac, 6) != 6) {
			printf("Failed to read, errno=%d\n", errno);
			return -1;
		}
		printf("rMAC=%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);
		return 0;
	} else {
		printf("Usage: %s [r] [w AABBCCDDEEFF]\n", argv[0]);
		return -1;
	}
	return 0;
}
