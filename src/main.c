#include "include/mbr/mbr.h"
#include "include/fat/fat_c.h"
#include <stdio.h>
#include <math.h>

int main(int argc, char **argv) {

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
	__fat_read_fsinfo(fd, data.partitions[0].StartLBA + databp.ext.sFSInfo, temp.llBytesPerSect, &dest);

	__fat_read_cluster(fd, databp.ext.sFSInfo + data.partitions[0].StartLBA, 512);

	printf("%s", databp.ext.szVollabel);

	return 0;
}
