#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct partitionEntry {
	uint8_t bDriverAttr;
	uint8_t StartCHS[3];
	uint8_t bType;
	uint8_t EndCHS[3];
	uint32_t StartLBA;
	uint32_t bNumbSect;
};
struct MBR {
	char bBootStrap[440];
	char  bUDID[4];
	char bResv[2];
	struct partitionEntry partitions[4];
	char  bSignature[2];
};

#include "mbr.c"

