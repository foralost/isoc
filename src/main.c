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

	struct directoryShortEntryFat32 test;
	memset(&test, 0, sizeof(test));
	test.bDirAttr = FAT32_DIR_ATTR_READ_ONLY;
	memset(&test, ' ', 11);
	test.szDirName[0] = 'F';
	test.szDirName[1] = 'I';
	test.szDirName[2] = 'N';
	test.szDirName[3] = '3';

	struct fileEntryFat32 testfile;
	testfile.shortEntry = test;
	testfile.shortEntry.iFileSize = 8192;
	testfile.bData = malloc(8192);
	testfile.bData[0] = 'H';
	testfile.bData[1] = 'H';

	struct clusterListFAT32* mainDir;
	__fat_insert_file(fd, 3, &databp, &testfile);
	__fat_update_fattable(fd, data.partitions[0].StartLBA, &databp);
	fsync(fd);
	return 0;
}
