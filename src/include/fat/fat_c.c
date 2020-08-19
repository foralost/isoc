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
	char szVollabel[12];
	char szFSType[9];
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
	uint32_t *FAT_TABLE;
	uint32_t *FAT_SEC_TABLE;
	uint32_t uFirstDataSector;
};

struct directoryLongEntryFat32 {
	uint8_t bOrder;
	char szName[10];
	uint8_t bAttr;
	uint8_t bType;
	uint8_t bChkSum;
	char szNameCont[12];
	uint16_t sFirstClusterLO;
	char szNameLast[4];
};

struct directoryShortEntryFat32 {
	char szDirName[11];
	uint8_t bDirAttr;
	uint8_t bNTres;
	uint8_t bCRTTimeTenth;
	uint16_t sCRTTime;
	uint16_t sCRTDate;
	uint16_t sLSTACCDate;
	uint16_t sFSTCLUSHI;
	uint16_t sWRTTime;
	uint16_t sWRTDate;
	uint16_t sFSTCLUSLO;
	uint32_t iFileSize;
};

struct listDirLEFat32 {
	struct directoryLongEntryFat32 *data;
	struct listDirLEFat32 *next;
};

struct dirEntryPairFat32 {
	struct listDirLEFat32 *listLE;
	struct directoryShortEntryFat32 *shortEntry;
};

struct listDirPairsFAT32 {
	struct dirEntryPairFat32 *data;
	struct listDirPairsFAT32 *next;
};

struct clusterListFAT32 {
	unsigned char *bData;
	uint32_t iNumber;
	struct clusterListFAT32 *next;
};

struct fileEntryFat32 {
	struct directoryShortEntryFat32 shortEntry;
	unsigned char *bData;
};

/*
 * Write a chunk of data bData of given length, to a device fd.
 */
ssize_t __fat_write_data(int fd, void *bData, size_t length) {
	ssize_t iWritten;
	iWritten = write(fd, bData, length);

	if (iWritten <= 0) {
		fatError = FAT32_ERROR_WRITING_SECTOR;
		perror("write ");
		return -1;
	}

	if (fsync(fd)) {
		perror("fsync");
		return -1;
	}

	return iWritten;
}

/*
 * Self explanatory, prints info about currently set error.
 */
void __fat_print_error(const char *szPrefix) {
	printf("%s:%s", szPrefix, szFatErrors[fatError]);
	return;
}

char __fat_is_valid_cluster(const uint32_t sector) {
	uint32_t to_check = sector & 0x0FFFFFFF;

	if (to_check != FAT32_CLUSTER_RESERVED && to_check != FAT32_CLUSTER_INVALID
			&& to_check < FAT32_CLUSTER_END_LOW) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * Tries to find a free cluster in a FAT table.
 * If given cluster is found, its index is saved in index.
 *
 * If there is no free clusters, function returns -1.
 */
int __fat_find_free_cluster(const struct biosParameterBlockFat32 *databp,
		uint32_t *index) {
	uint32_t fatSize = (databp->ext.iFATSz32 * databp->sBytsPerSec) / 4;
	for (uint32_t i = 2; i < fatSize; i++) {
		if (databp->FAT_TABLE[i] == FAT32_CLUSTER_AVAILABLE) {
			*index = i;
			return 0;
		}
	}
	return -1;
}

/*
 * Seeks last cluster in a given cluster chain using FAT table.
 *
 * Function stores index of FAT table entry which represents the last cluster
 * in a cluster chain.
 */
int __fat_seek_last_cluster(const uint32_t cluster,
		const struct biosParameterBlockFat32 *databp, uint32_t *dest) {
	uint32_t currentCluster = cluster, prev;

	do {
		prev = currentCluster;
		currentCluster = databp->FAT_TABLE[cluster];
	} while (__fat_is_valid_cluster(currentCluster));

	*dest = prev;
	return 0;
}

/*
 *	Dumps given cluster to a destination device.
 *	It uses src->iNumber to determine clusters location on a device.
 */
int __fat_write_cluster(const int fd, const struct clusterListFAT32 *src,
		const struct biosParameterBlockFat32 *databp) {
	uint32_t pos = (databp->uFirstDataSector
			+ ((src->iNumber - 2) * databp->bSecPerClus)) * databp->sBytsPerSec;

	if (lseek(fd, pos, SEEK_SET) < 0) {
		perror("lseek");
		fatError = FAT32_ERROR_SEEKING_SECTOR;
		return -1;
	}

	__fat_write_data(fd, src->bData, databp->bSecPerClus * databp->sBytsPerSec);
	return 0;
}

/*
 *	Sets state of a cluster (cluster) in FAT table, to a given value.
 *	If given device contains a two FAT tables, that change is recorded
 *	on both of them.
 */
int __fat_set_cluster(const uint32_t cluster, const uint32_t value,
		struct biosParameterBlockFat32 *databp) {
	databp->FAT_TABLE[cluster] = value;
	if (databp->bNumFATs == 2) {
		databp->FAT_SEC_TABLE[cluster] = value;
	}
	return 0;
}

/*
 * This function finds a free cluster and assigns it as a last element of
 * given list.
 *
 * Additionally, this function marks previously free cluster
 * as a last in chain in a FAT table, and marks last cluster with an index
 * of free cluster.
 *
 */
int __fat_allocate_cluster(struct biosParameterBlockFat32 *databp,
		struct clusterListFAT32 *dest) {
	struct clusterListFAT32 *currEntry = dest, *newEntry;
	while (currEntry->next)
		currEntry = currEntry->next;

	uint32_t currCluster = dest->iNumber;
	uint32_t freeCluster = 0;

	__fat_find_free_cluster(databp, &freeCluster); //FIXME: Remember error handling!
	__fat_set_cluster(freeCluster, FAT32_CLUSTER_END_LOW, databp);
	__fat_set_cluster(currCluster, freeCluster, databp);
	__fat_init_freecluster(freeCluster, databp, &newEntry);
	dest->next = newEntry;
	return 0;

}

/*
 * This function updates FAT table stored on the device with FAT table
 * stored in memory.
 *
 * Also, it updates second FAT table, if exists.
 */
int __fat_update_fattable(const int fd, const uint32_t LBAStart,
		const struct biosParameterBlockFat32 *databp) {
	uint32_t FATStart = (databp->sRsvdSecCnt + LBAStart) * databp->sBytsPerSec;
	if (lseek(fd, FATStart, SEEK_SET) < 0) {
		perror("lseek");
		fatError = FAT32_ERROR_SEEKING_SECTOR;
		return -1;
	}

	__fat_write_data(fd, databp->FAT_TABLE,
			databp->ext.iFATSz32 * databp->sBytsPerSec);

	if (databp->bNumFATs == 2)
		__fat_write_data(fd, databp->FAT_TABLE,
				databp->ext.iFATSz32 * databp->sBytsPerSec);

	return 0;
}

/*
 * This function is used to place/create a normal directory in FAT filesystem.
 *
 * It places 3 short entry directories with reserving new clusters.
 */
int __fat_insert_normdir(const int fd, struct directoryShortEntryFat32 *src,
		struct clusterListFAT32 *dest, struct biosParameterBlockFat32 *databp) {
	/*
	 * TODO:
	 * 	Due to specification, using fields from FileSystem Info is not safe.
	 * 	Find out if it is true, for now, we will find a free cluster manually.
	 */

	struct clusterListFAT32 *currEntry = dest;

	uint32_t freeCluster;
	__fat_find_free_cluster(databp, &freeCluster); //FIXME: Error handling
	src->sFSTCLUSLO = freeCluster;

	__fat_place_shortdir(currEntry, src, databp);
	__fat_write_cluster(fd, currEntry, databp);

	currEntry = NULL;
	struct directoryShortEntryFat32 dots[2];
	memset(dots, 0, sizeof(dots));
	dots[0].bDirAttr = FAT32_DIR_ATTR_DIRECTORY;
	memset(&dots[0], ' ', 11);
	dots[0].szDirName[0] = '.';
	dots[0].sFSTCLUSLO = freeCluster;
	dots[1].bDirAttr = FAT32_DIR_ATTR_DIRECTORY;
	memset(&dots[1], ' ', 11);
	dots[1].szDirName[0] = '.';
	dots[1].szDirName[1] = '.';
	if (dest->iNumber != 2)
		dots[1].sFSTCLUSLO = dest->iNumber;

	__fat_init_freecluster(freeCluster, databp, &currEntry);
	__fat_set_cluster(freeCluster, FAT32_CLUSTER_END_LOW, databp);

	__fat_place_shortdir(currEntry, &dots[0], databp);
	__fat_place_shortdir(currEntry, &dots[1], databp);

	__fat_write_cluster(fd, currEntry, databp);

	return 0;
}

int __fat_merge_clusters(char **szDest,
		const struct biosParameterBlockFat32 *databp,
		const struct clusterListFAT32 *start) {
	uint64_t wholeSize = 0;
	const struct clusterListFAT32 *curr = start;

	while (curr) {
		wholeSize += databp->bSecPerClus * databp->sBytsPerSec;
		curr = curr->next;
	}

	*szDest = malloc(wholeSize);
	curr = start;
	while (curr) {
		memcpy(*szDest, curr->bData, databp->bSecPerClus * databp->sBytsPerSec);
		curr = curr->next;
	}
	return 0;
}

int __fat_free_clusters(struct clusterListFAT32 *start) {
	struct clusterListFAT32 *current, *next;
	current = start;
	while (current) {
		free(current->bData);
		next = current->next;
		free(current);
		current = next;
	}

	return 0;
}

int __fat_init_freecluster(const uint32_t clusterNumb,
		const struct biosParameterBlockFat32 *databp,
		struct clusterListFAT32 **dest) {
	*dest = calloc(sizeof(**dest), 1);
	(*dest)->iNumber = clusterNumb;
	(*dest)->bData = calloc(1, databp->bSecPerClus * databp->sBytsPerSec);
	return 0;

}
int __fat_read_clusters(const int fd, const uint32_t clusterNumb,
		const struct biosParameterBlockFat32 *databp,
		struct clusterListFAT32 **szDest) {

	struct clusterListFAT32 *start = NULL, *current, *prev = NULL;
	uint32_t nextCluster = clusterNumb, prevCluster;
	uint32_t pos, clusSize = databp->bSecPerClus * databp->sBytsPerSec;
	nextCluster &= 0x0FFFFFFF;

	while (__fat_is_valid_cluster(nextCluster)) {

		if (nextCluster - prevCluster != 1) {
			pos = databp->uFirstDataSector * databp->sBytsPerSec;
			pos += (nextCluster - 2) * clusSize;
			if (lseek(fd, pos, SEEK_SET) < 0) {
				perror("lseek");
				__fat_free_clusters(start);
				fatError = FAT32_ERROR_SEEKING_SECTOR;
				return -1;
			}
		}
		current = malloc(sizeof(*current));
		current->bData = malloc(clusSize);
		current->iNumber = nextCluster;
		if (read(fd, current->bData, clusSize) != clusSize) {
			perror("read");

			if (prev)
				prev->next = NULL;

			__fat_free_clusters(start);
			fatError = FAT32_ERROR_READING_CLUSTER;
			return -1;
		}
		if (prev)
			prev->next = current;

		if (!start)
			start = current;

		prevCluster = nextCluster;
		nextCluster = databp->FAT_TABLE[nextCluster] & 0x0FFFFFFF;
		prev = current;
	}

	*szDest = start;
	return 0;
}

int __fat_get_file(const int fd, const struct directoryShortEntryFat32 *info,
		const struct biosParameterBlockFat32 *databp, char **szDest) {

	if (info->bDirAttr & FAT32_DIR_ATTR_DIRECTORY) {
		fatError = FAT32_NOT_A_REGULAR_FILE;
		return -1;
	}

	*szDest = malloc(info->iFileSize);
	uint32_t iCluster = 0;

	iCluster |= info->sFSTCLUSHI;
	iCluster <<= 16;
	iCluster |= info->sFSTCLUSLO;

	struct clusterListFAT32 *current = NULL, *list = NULL;
	__fat_read_clusters(fd, iCluster, databp, &list);

	current = list;
	uint32_t offset = 0;

	while (current->next) {
		memcpy(*szDest + offset, current->bData,
				databp->bSecPerClus * databp->sBytsPerSec);
		offset += databp->sBytsPerSec;
		current = current->next;
	}

	uint32_t delta = info->iFileSize - offset;

	memcpy(*szDest + offset, current->bData, delta);
	__fat_free_clusters(list);
	;
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
int __fat_read_fsinfo(const int fd, const uint32_t partStart,
		const struct biosParameterBlockFat32 *databp,
		struct sectorSizeFat32 *dest) {
	char temp[databp->sBytsPerSec];
	memset(temp, 0, databp->sBytsPerSec);
	if (__fat_read_sector(fd, partStart + databp->ext.sFSInfo,
			databp->sBytsPerSec, temp))
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
/*
 *
 */
int __fat_read_bpb(const int fd, const uint32_t LBAStart,
		const uint32_t secSize, struct biosParameterBlockFat32 *data) {
	if (lseek(fd, LBAStart * secSize, SEEK_SET) < 0) {
		perror("lseek");
		fatError = FAT32_ERROR_SEEKING_SECTOR;
		return -1;
	}

	char bData[secSize];
	if (read(fd, bData, secSize) <= 0) {
		free(bData);
		fatError = FAT32_ERROR_READING_SECTOR;
		perror("read");
		return -1;
	}

	if (bData[66] != 0x28 && bData[66] != 0x29) {
		free(bData);
		fatError = FAT32_INVALID_BPB_SIGNATURE;
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
	int rootDirSec = data->sRootEntCnt * 32
			+ (data->sBytsPerSec - 1) / data->sBytsPerSec;
	data->uFirstDataSector = data->sRsvdSecCnt
			+ (data->bNumFATs * data->ext.iFATSz32) + rootDirSec + LBAStart;

	// ------------
	// FAT (File Allocation Table) READING
	uint32_t LBAFAT = data->sRsvdSecCnt;
	if (lseek(fd, LBAFAT * data->sBytsPerSec + LBAStart * data->sBytsPerSec,
	SEEK_SET) < 0) {
		fatError = FAT32_ERROR_SEEKING_SECTOR;
		free(data);
		return -1;
	}
	uint32_t FATSize = data->ext.iFATSz32 * data->sBytsPerSec;
	data->FAT_TABLE = malloc(FATSize);
	if (read(fd, data->FAT_TABLE, FATSize) < 0) {
		fatError = FAT32_ERROR_READING_SECTOR;
		free(data->FAT_TABLE);
		free(data);
		return -1;
	}

	if (data->bNumFATs == 2) {
		data->FAT_SEC_TABLE = malloc(FATSize);
		if (read(fd, data->FAT_SEC_TABLE, FATSize) < 0) {
			fatError = FAT32_ERROR_READING_SECTOR;
			free(data->FAT_SEC_TABLE);
			free(data->FAT_TABLE);
			free(data);
			return -1;
		}
	}
	return 0;
}

/*
 *
 */

int __fat_get_device_open(const char *szDeviceName, int *fd) {
	int ofd = open(szDeviceName, O_RDWR);
	if (ofd < 0) {
		perror("open");
		return -1;
	}
	*fd = ofd;
	return 0;
}

/*
 *
 */
int __fat_converth_to_timestamp(struct tm *date, uint16_t *dest) {
	if (date->tm_sec > 59) {
		fatError = FAT32_INVALID_SECONDS;
		return -1;
	}
	if (date->tm_min > 59) {
		fatError = FAT32_INVALID_MINUTES;
		return -1;
	}

	if (date->tm_hour > 23) {
		fatError = FAT32_INVALID_HOURS;
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

int __fat_get_device_close(int fd) {
	int res = close(fd);
	if (res < 0)
		perror("close");

	return res;
}

int __fat_get_long_dir(const char *szSrc, struct directoryLongEntryFat32 **dest) {

	struct directoryLongEntryFat32 *currLongDir = calloc(1,
			sizeof(struct directoryLongEntryFat32));
	size_t offset = 0, count = 0;

	if ((uint8_t) szSrc[offset] == FAT32_DIR_FREE
			|| (uint8_t) szSrc[offset] == FAT32_DIR_FREE_END) {
		return 0;
	}
	count = szSrc[offset] & FAT32_DIR_LONG_ENTRY_MASK;
	if (count == 0 || (uint8_t) szSrc[offset + 11] != FAT32_DIR_ATTR_LONG_NAME
			|| szSrc[offset + 26] != 0) {
		fatError = FAT32_NOT_A_LONG_DIR;
		return -1;
	}

	currLongDir->bOrder = szSrc[offset];
	memcpy(currLongDir->szName, szSrc + offset + 1, 10);
	currLongDir->bAttr = szSrc[offset + 11];
	currLongDir->bType = szSrc[offset + 12];
	currLongDir->bChkSum = szSrc[offset + 13];
	memcpy(currLongDir->szNameCont, szSrc + offset + 14, 12);
	memcpy(currLongDir->szNameLast, szSrc + offset + 28, 4);
	*dest = currLongDir;
	return 0;

}

int __fat_get_short_dir(const char *szData, const size_t length,
		const size_t offset, struct directoryShortEntryFat32 **dest) {

	if (offset + FAT32_DIR_ENTRY_SIZE > length) {
		return -1;
	}
	// We assume that the offset is already at the right position.
	struct directoryShortEntryFat32 *temp = calloc(sizeof(**dest), 1);
	memcpy(temp->szDirName, offset + szData, 11);
	temp->bDirAttr = szData[offset + 11];
	temp->bNTres = szData[offset + 12];
	temp->bCRTTimeTenth = szData[offset + 13];
	memcpy(&temp->sCRTTime, szData + offset + 14, 2);
	memcpy(&temp->sCRTDate, szData + offset + 16, 2);
	memcpy(&temp->sLSTACCDate, szData + offset + 18, 2);
	memcpy(&temp->sFSTCLUSHI, szData + offset + 20, 2);
	memcpy(&temp->sWRTTime, szData + offset + 22, 2);
	memcpy(&temp->sWRTDate, szData + offset + 24, 2);
	memcpy(&temp->sFSTCLUSLO, szData + offset + 26, 2);
	memcpy(&temp->iFileSize, szData + offset + 28, 4);
	*dest = temp;
	return 0;
}
int __fat_check_free_clusters(const struct biosParameterBlockFat32 *databp,
		const uint32_t *desired) {

	if (*desired <= 0) {
		return -1;
	}
	uint32_t count = 0;
	uint32_t fatSize = databp->ext.iFATSz32 * databp->sBytsPerSec
			/ sizeof(uint32_t);

	for (uint32_t i = 0; i < fatSize; i++) {
		if (databp->FAT_TABLE[i] == FAT32_CLUSTER_AVAILABLE) {
			count++;
			if (count == *desired)
				return 1;
		}
	}

	return 0;
}

/* This function must be hidden. It DOES NOT represent a normal clusterList.
 * ONLY the last cluster is memory allocated to align the cluster size boundary,
 * ONLY if the file size is not a multiplication of cluster size.
 *
 * This way we avoid duplicating huge chunks of memory when a large file is given.
 *
 * This function converts byte array bData into one-way list, which represents a list of
 * clusters. Each element of this list represent one cluster, which has an array of byte data
 * of length 1 cluster represented by databp.
 *
 */
int __fat_byte_to_clusters(unsigned char *bData, const uint32_t size,
		const struct biosParameterBlockFat32 *databp,
		struct clusterListFAT32 **dest) {
	struct clusterListFAT32 *start = NULL, *current, *prev = NULL;

	uint32_t clusterSize = databp->bSecPerClus * databp->sBytsPerSec;
	uint32_t clusterCount = size / clusterSize + 1;

	for (uint32_t i = 0; i < clusterCount - 1; i++) {
		current = calloc(1, sizeof(*current));
		current->bData = bData + 1 * clusterSize;
		current->iNumber = -1;

		if (!start)
			start = current;

		if (prev)
			prev->next = current;

		prev = current;
	}
	uint32_t delta = size % clusterSize;

	if (delta) {
		current = calloc(1, sizeof(*current));
		current->bData = calloc(1, clusterSize);
		memcpy(current->bData, bData + (clusterCount - 1) * clusterSize, delta);
		current->iNumber = -1;
		if (!start)
			start = current;

		if (prev)
			prev->next = current;
	}
	*dest = start;
	return 0;
}
/*
 * This function places a short directory entry in a free entry of cluster,
 * which is located in the given list of clusters.
 *
 * If there is no free entry in the given cluster list, a new cluster
 * is allocated and assigned to the last element of the list.
 */
int __fat_place_shortdir(struct clusterListFAT32 *dest,
		const struct directoryShortEntryFat32 *src,
		struct biosParameterBlockFat32 *databp) {

	struct clusterListFAT32 *start = dest;
	while (start->next)	//FIXME: Big nono, there could be a free entry there.
		start = start->next;

	unsigned char *bStart = start->bData;

	uint32_t offset = 0;
	while (bStart[offset] != FAT32_DIR_FREE
			&& bStart[offset] != FAT32_DIR_FREE_END
			&& offset < databp->bSecPerClus * databp->sBytsPerSec)
		offset += 32;

	if (offset >= databp->bSecPerClus * databp->sBytsPerSec) {
		__fat_allocate_cluster(databp, start);
		start = start->next;
		bStart = start->bData;
	}

	memcpy(bStart + offset, src->szDirName, 11);
	memcpy(bStart + offset + 11, &src->bDirAttr, 1);
	memcpy(bStart + offset + 12, &src->bNTres, 1);
	memcpy(bStart + offset + 13, &src->bCRTTimeTenth, 1);
	memcpy(bStart + offset + 14, &src->sCRTTime, 2);
	memcpy(bStart + offset + 16, &src->sCRTDate, 2);
	memcpy(bStart + offset + 18, &src->sLSTACCDate, 2);
	memcpy(bStart + offset + 20, &src->sFSTCLUSHI, 2);
	memcpy(bStart + offset + 22, &src->sWRTTime, 2);
	memcpy(bStart + offset + 24, &src->sWRTDate, 2);
	memcpy(bStart + offset + 26, &src->sFSTCLUSLO, 2);
	memcpy(bStart + offset + 28, &src->iFileSize, 4);
	return 0;

}
/*
 *
 */
int __fat_insert_file(const int fd, uint32_t startCluster,
		struct biosParameterBlockFat32 *databp, struct fileEntryFat32 *src) {

	uint32_t clusterCount = src->shortEntry.iFileSize
			/ (databp->bSecPerClus * databp->sBytsPerSec) + 1;
	uint32_t freeCluster, nextFree;

	if (__fat_check_free_clusters(databp, &clusterCount) != 1) {
		fatError = FAT32_NOT_ENOUGH_CLUSTERS;
		return -1;
	}

	struct clusterListFAT32 *currCluster;
	__fat_byte_to_clusters(src->bData, src->shortEntry.iFileSize, databp,
			&currCluster);

	__fat_find_free_cluster(databp, &freeCluster);
	src->shortEntry.sFSTCLUSHI = freeCluster >> 8;
	src->shortEntry.sFSTCLUSLO = freeCluster & 0x000000FF;
	currCluster->iNumber = freeCluster;

	__fat_write_cluster(fd, currCluster, databp);
	__fat_set_cluster(freeCluster, FAT32_CLUSTER_END_LOW, databp);
	currCluster = currCluster->next;

	while (currCluster) {
		__fat_find_free_cluster(databp, &nextFree);
		__fat_set_cluster(freeCluster, nextFree, databp);

		freeCluster = nextFree;
		__fat_write_cluster(fd, currCluster, databp);
		currCluster = currCluster->next;
	}

	__fat_set_cluster(freeCluster, FAT32_CLUSTER_END_LOW, databp);

	struct clusterListFAT32 *startClusters;
	__fat_read_clusters(fd, startCluster, databp, &startClusters);
	__fat_place_shortdir(startClusters, &src->shortEntry, databp);
	__fat_write_cluster(fd, startClusters, databp);
	return 0;
}

