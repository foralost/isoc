/*
 * fat_c.c
 *
 *  Created on: 7 mar 2020
 *      Author: foralost
 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
struct deviceDataFat32 {
	size_t llBytes;
	size_t llSectors;
	size_t llBytesPerSect;
};

struct extBPBFat32 {
	uint32_t iFATSz32;
	uint16_t sFlags;
	uint16_t sFSVer;
	uint32_t iRootClus;
	uint16_t sFSInfo;
	uint16_t sBkBootSec;
	char arrRsv[12];
	uint8_t bDrvNum;
	uint8_t bFlagsNT;
	uint8_t bSig;
	uint32_t iVolID;
	char szVollabel[11];
	char szFSType[8];
	char bBootCode[420];
	uint8_t bBootSignt[2];
};
struct sectorSizeFat32 {
	uint32_t iLeadSig;
	char bReserved[480];
	uint32_t iStrucSig;
	uint32_t iFreeCount;
	uint32_t iNxtFree;
	char bReserved1[12];
	uint32_t iTrailSig;
};

struct biosParameterBlockFat32 {
	char jmpBoot[3];
	char OEMName[8];
	uint16_t sBytsPerSec;
	uint8_t bSecPerClus;
	uint16_t sRsvdSecCnt;
	uint8_t bNumFATs;
	uint16_t sRootEntCnt;
	uint16_t sTotSec16;
	uint8_t bBPBMedia;
	uint16_t sFATSz16;
	uint16_t sSecPerTrk;
	uint16_t sNumHeads;
	uint32_t iHiddSec;
	uint32_t iTotSec32;
	struct extBPBFat32 ext;
};

struct directoryEntryFat32 {
	char szDirName[11];
	uint8_t bDirAttr;
	uint8_t bNTres;
	uint8_t bCRTTimeTenth;
	uint16_t sCRTTime;
	uint16_t sCRTDate;
	uint16_t sLSTACCDate;
	uint16_t sWRTTime;
	uint16_t sWRTDate;
	uint16_t sFSTCLUSLO;
	uint32_t iFileSize;
};
int __fat_read_cluster(const int fd, const uint32_t LBA,
		const uint32_t sectorSize) {
	if (lseek(fd, LBA * sectorSize, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	char *bData = malloc(sectorSize);

	if (read(fd, bData, sectorSize) != sectorSize) {
		free(bData);
		perror("read");
		return -1;
	}

	return 0;
}

int __fat_read_sector(const int fd, const uint32_t LBAStart,
		const uint32_t secSize, char *dest) {
	if (lseek(fd, LBAStart * secSize, SEEK_SET) < 0) {
		fatError = FAT32_ERROR_SEEKING_SECTOR;
		perror("lseek");
		return -1;
	}

	if (read(fd, dest, secSize) != secSize) {
		fatError = FAT32_ERROR_READING_SECTOR;
		perror("read");
		return -1;
	}

	return 0;
}
int __fat_read_fsinfo(const int fd, const uint32_t LBAStart,
		const uint32_t secSize, struct sectorSizeFat32 *dest) {
	char temp[secSize];
	memset(temp, 0, secSize);
	if (__fat_read_sector(fd, LBAStart, secSize, temp))
		return -1;

	for (int i = 0; i < 480; i++)
		if (temp[i + 4]) {
			fatError = FAT32_VIOLATION_RSV_BYTES;
			return -1;
		}

	for (int i = 0; i < 12; i++)
		if (temp[i + 0x1f0]) {
			fatError = FAT32_VIOLATION_RSV_BYTES;
			return -1;
		}

	if (*(uint32_t*) temp != (uint32_t) FAT32_FSINFO_LEAD_SIG) {
		fatError = FAT32_INVALID_FSINFO_SIG;
		return -1;
	}

	if (*(uint32_t*) (temp + 0x1e4) != (uint32_t) FAT32_FSINFO_ANTH_SIG) {
		fatError = FAT32_INVALID_FSINFO_SIG;
		return -1;
	}

	if (*(uint32_t*) (temp + 0x1fc) != (uint32_t) FAT32_FSINFO_TRAI_SIG) {
		fatError = FAT32_INVALID_FSINFO_SIG;
		return -1;
	}

	memset(dest, 0, sizeof(*dest));
	dest->iLeadSig = FAT32_FSINFO_LEAD_SIG;
	dest->iTrailSig = FAT32_FSINFO_TRAI_SIG;
	dest->iStrucSig = FAT32_FSINFO_ANTH_SIG;
	dest->iFreeCount = *((uint32_t*) (temp + 0x1e8));
	dest->iNxtFree = *((uint32_t*) (temp + 0x1ec));
	return 0;
}
int __fat_read_bpb(const int fd, const uint32_t LBAStart,
		const uint32_t secSize, struct biosParameterBlockFat32 *data) {
	if (lseek(fd, LBAStart * secSize, SEEK_SET) < 0) {
		perror("lseek");
		return -1;
	}

	char bData[secSize];
	if (read(fd, bData, secSize) <= 0) {
		free(bData);
		perror("read");
		return -1;
	}

	if (bData[66] != 0x28 && bData[66] != 0x29) {
		free(bData);
		return -1;
	}

	memset(data, 0, sizeof(*data));
	memcpy(data->jmpBoot, bData, 3);
	memcpy(data->OEMName, bData + 3, 8);
	data->sBytsPerSec = *((uint16_t*) (bData + 11));
	data->bSecPerClus = bData[13];
	data->sRsvdSecCnt = *((uint16_t*) (bData + 14));
	data->bNumFATs = bData[16];
	data->sRootEntCnt = *((uint16_t*) (bData + 17));
	data->sTotSec16 = *((uint16_t*) (bData + 19));
	data->bBPBMedia = bData[21];
	data->sFATSz16 = *((uint16_t*) (bData + 22));
	data->sSecPerTrk = *((uint16_t*) (bData + 24));
	data->sNumHeads = *((uint16_t*) (bData + 26));
	data->iHiddSec = *((uint32_t*) (bData + 28));
	data->iTotSec32 = *((uint32_t*) (bData + 32));

// ------------

	data->ext.iFATSz32 = *((uint32_t*) (bData + 36));
	data->ext.sFlags = *((uint16_t*) (bData + 40));
	data->ext.sFSVer = *((uint16_t*) (bData + 42));
	data->ext.iRootClus = *((uint32_t*) (bData + 44));
	data->ext.sFSInfo = *((uint16_t*) (bData + 48));
	data->ext.sBkBootSec = *((uint16_t*) (bData + 50));

	memcpy(data->ext.arrRsv, bData + 52, 12);
	data->ext.bDrvNum = bData[64];
	data->ext.bFlagsNT = bData[65];
	data->ext.bSig = bData[66];
	data->ext.iVolID = *((uint32_t*) (bData + 67));

	memcpy(data->ext.szVollabel, bData + 71, 11);
	memcpy(data->ext.szFSType, bData + 82, 8);
	memcpy(data->ext.bBootCode, bData + 90, 420);
	memcpy(data->ext.bBootSignt, bData + 510, 2);
	return 0;

}
int __fat_get_device_open(char *szDeviceName, int *fd) {
	int ofd = open(szDeviceName, O_RDONLY);
	if (ofd < 0) {
		perror("open");
		return -1;
	}
	*fd = ofd;
	return 0;
}

int __fat_convertts_to_h(uint16_t src, struct tm *dest) {
	memset(dest, 0, sizeof(*dest));
	return 0;
}
int __fat_converth_to_timestamp(struct tm *date, uint16_t *dest) {
	if (date->tm_sec > 59) {
		return -1;
	}
	if (date->tm_min > 59) {
		return -1;
	}

	if (date->tm_hour > 23) {
		return -1;
	}
	int temp = date->tm_sec;
	temp = temp >> 1;
	*dest = 0;
	*dest = *dest | temp;
	temp = date->tm_min;
	temp = temp << 5;
	*dest = *dest | temp;
	temp = date->tm_hour;
	temp = temp << 11;
	*dest = *dest | temp;
	return 0;
}
int __fat_convertdt_to_timestamp(char *szFormat, uint16_t *dest) {

	struct tm temptime;
	memset(&temptime, 0, sizeof(temptime));
	if (!strptime(szFormat, "%Y-%m-%d", &temptime)) {
		perror("strptime");
		return -1;
	}
	if (0 > temptime.tm_year || 127 < temptime.tm_year) {
		return -1;
	}

	*dest = 0;
	*dest = *dest | temptime.tm_mday;
	int temp = temptime.tm_mon;
	temp = temp << 5;
	*dest = *dest | temp;
	temp = temptime.tm_year;
	temp = temp << 9;
	*dest = *dest | temp;
	return 0;
}
int __fat_get_device_stats(const int fd, struct deviceDataFat32 *dst) {
	memset(dst, 0, sizeof(*dst));

	if (ioctl(fd, BLKGETSIZE64, &dst->llBytes) == -1) {
		perror("ioctl getsize");
		return -1;
	}
	if (ioctl(fd, BLKSECTGET, &dst->llSectors) == -1) {
		perror("ioctl sectget");
		return -1;
	}

	if (ioctl(fd, BLKSSZGET, &dst->llBytesPerSect) == -1) {
		perror("ioctl sizesect");
		return -1;
	}
	return 0;

}
ssize_t __fat_write_data(int fd, char *bData, size_t length) {
	ssize_t iWritten;
	iWritten = write(fd, bData, length);

	if (!iWritten) {
		perror("write ");
		return -1;
	}

	if (fsync(fd)) {
		perror("fsync");
		return -1;
	}

	return iWritten;
}
int __fat_get_device_close(int fd) {
	int res = close(fd);
	if (res < 0)
		perror("close");

	return res;
}
