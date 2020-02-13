#include "iso_typedefs.h"

int iso_read_path_table(const struct ISOFile *f,
		struct entryPathTableNode **dest) {
	if (fseek(f->fHandler, ISO_BLOCK_SIZE * f->strPVD.iLocTypeLPath,
	SEEK_SET)) {
		perror("fseek: ");
		isoError = ISO_PATH_TABLE_SEEK_FAILED;
		return -1;
	}

	char *bPathTable = malloc(f->strPVD.iPathTableSizeLSB);
	memset(bPathTable, 0, f->strPVD.iPathTableSizeLSB);

	if (fread(bPathTable, 1, f->strPVD.iPathTableSizeLSB, f->fHandler)
			!= f->strPVD.iPathTableSizeLSB) {
		isoError = ISO_PATH_TABLE_READ_FAILED;
		return -1;
	}
	size_t stCounter = 0;
	int items = 0;

	struct entryPathTableNode *first, *test = NULL, *prev = NULL, *next = NULL;
	test = malloc(sizeof(struct entryPathTableNode));
	first = test;

	while (stCounter != f->strPVD.iPathTableSizeLSB) {

		if (stCounter > f->strPVD.iPathTableSizeLSB) {
			test = first;
			isoError = ISO_PATH_TABLE_READ_ENTRY_FAILED;
			do {
				next = test->next;
				free(test);
				test = next;
			} while (test);
			free(bPathTable);
			return -1;
		}

		test->data.bLengthDIRID = bPathTable[stCounter];			// 1 byte
		test->data.bExtAttrRecLength = bPathTable[stCounter + 1];	// 1 byte
		test->data.iExtentLBA = *((int32_t*) (bPathTable + 2 + stCounter));	// 4 bytes
		test->data.sDirNumber = *((int16_t*) (bPathTable + 6 + stCounter));	// 2 bytes
		test->data.szDirIdentifier = malloc(test->data.bLengthDIRID); // test.raw.bLengthDIRID
		memcpy(test->data.szDirIdentifier, bPathTable + 8 + stCounter,
				test->data.bLengthDIRID);
		stCounter += 8 + test->data.bLengthDIRID;
		if (stCounter % 2)
			stCounter++;

		prev = test;
		test = malloc(sizeof(struct entryPathTableNode));
		items++;
		prev->next = test;
	}
	prev->next = NULL;
	free(test);
	free(bPathTable);
	*dest = first;
	return items - 1;
}

int iso_read_dir(FILE *f, const struct directoryDescriptor *startDir,
		struct directoryDescriptorNode **desc) {

	char* bData;
	if ( iso_alloc_sector(f, bData, startDir->iLocLSBEXT) < 0)
	{
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		return -1;
	}



}
int iso_read_directories_pt(FILE *f, const struct entryPathTable *mainDir,
		struct directoryDescriptorNode **desc) {
	char *bData;

	if (fseek(f, ISO_BLOCK_SIZE * mainDir->iExtentLBA, SEEK_SET)) {
		perror("fseek");
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		return -1;
	}
	bData = malloc(ISO_BLOCK_SIZE);
	memset(bData, 0, ISO_BLOCK_SIZE);

	if (fread(bData, 1, ISO_BLOCK_SIZE, f) != ISO_BLOCK_SIZE) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		free(bData);
		return -1;
	}

	struct directoryDescriptorNode *node, *prev, *first;
	node = malloc(sizeof(struct directoryDescriptorNode));
	memset(node, 0, sizeof(struct directoryDescriptorNode));
	node->data.lPos = ftell(f) - ISO_BLOCK_SIZE;
	prev = node;
	first = node;
	int items = 1;
	size_t offset = 0;
	while (1) {
		node->data.bLength = bData[offset];
		if (!node->data.bLength) {
			free(node);
			break;
		}
		node->data.lPos = first->data.lPos + offset;
		node->data.bEXTRecordLength = bData[1 + offset];
		node->data.iLocLSBEXT = *((int32_t*) (bData + 2 + offset));
		node->data.iLocMSBEXT = *((int32_t*) (bData + 6 + offset));
		node->data.iDataLengthLSB = *((int32_t*) (bData + 10 + offset));
		node->data.iDataLengthMSB = *((int32_t*) (bData + 14 + offset));
		memcpy(node->data.szRecDate, bData + 18 + offset, 7);
		node->data.bFlags = bData[25 + offset];
		node->data.bFSize = bData[26 + offset];
		node->data.bGapSize = bData[27 + offset];
		node->data.sVolSeqLSB = *((int16_t*) (bData + 28 + offset));
		node->data.sVolSeqMSB = *((int16_t*) (bData + 30 + offset));
		node->data.bLengthID = bData[32 + offset];
		node->data.szDirIdentifier = malloc(node->data.bLengthID);
		memcpy(node->data.szDirIdentifier, bData + 33 + offset,
				node->data.bLengthID);
		offset += node->data.bLength;

		prev = node;

		node = malloc(sizeof(struct directoryDescriptorNode));
		items++;
		memset(node, 0, sizeof(struct directoryDescriptorNode));
		prev->next = node;
	}
	*desc = first;
	return items - 1;
}
