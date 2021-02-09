#include "include/mbr/mbr.h"
#include "include/fat/fat_c.h"
#include <stdio.h>
#include <math.h>

/*
 * TODO:
 * 			#	OPERACJE READ-ONLY:
 * 			#
 * 			# 		ODCZYTAĆ DOWOLNY PLIK Z SYSTEMU FAT32 (# D O N E #)
 * 			#		ODCZYTAĆ DOWOLNY PLIK Z SYSTEMU FAT32 ZE WSKAZANIEM LOKALIZACJI NA PARTYCJI
 * 			#
 * 			# 	OPERACJE WRITE:
 * 			#
 * 			#		UTWORZYĆ SYSTEM PLIKÓW FAT32
 * 			#		ZARZĄDZANIE OBYDWOMA SYSTEMAMI PLIKÓW (TWORZENIE/EDYCJA/USUWANIE PLIKÓW/WPISÓW)
 * 			#		UTWORZENIE FOLDERU W DOWOLNYM MIEJSCU
 * 			#		UTWORZENIE WPISU PLIKU W DOWOLNYM MIEJSCU
 * 			#
 * 			#
 * 			#	OBECNIE W OBRÓBCE JEST: FAT32
 * 			#
 * 			#	FAT32:
 * 			#		-	GRUNTOWNE PRZEBADANIE WYCIEKÓW PAMIĘCI
* 			#		-	ERROR-HANDLING
* 			#		-	DOKUNENTACJA
* 			#		- 	DŁUGIE WPISY
 * 			#
 * 			#
 *
 *
 */
int main(void)
{

	int fd;
	__fat_get_device_open("/dev/loop0", &fd);

	struct deviceDataFat32 temp;
	__fat_get_device_stats(fd, &temp);

	struct MBR data;

	__mbr_init_from_dev(fd, &data);

	struct biosParameterBlockFat32 databp;
	__fat_read_bpb(fd, data.partitions[0].StartLBA, temp.llBytesPerSect,
			&databp);

	struct sectorSizeFat32 dest;
	__fat_read_fsinfo(fd, data.partitions[0].StartLBA, &databp, &dest);

	struct clusterListFAT32* destCluster;
	__fat_read_clusters(fd, 2, &databp, &destCluster);

	struct listDirLEFat32* lDirs;
	uint32_t offset = 0;
	uint32_t skipped = 0;
	__fat_get_long_dirs(destCluster, &databp, &offset, &skipped,&lDirs);

	struct directoryShortEntryFat32* shortEntry;
	__fat_get_short_dir(destCluster, &databp, &offset, &shortEntry);

	fsync(fd);
	return 0;
}
