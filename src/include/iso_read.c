#include "iso_read.h"

#ifndef SRC_ISO_READ_C_
#define SRC_ISO_READ_C_

int iso_print_directory_tree(const struct directoryDescriptorNode *start) {

	while (start) {
		if ((start->data.bFlags & ISO_FILE_FLAG_SUBDIR)) {
			printf("Folder: ");
		} else {
			printf("Plik: ");
		}

		if (!start->data.szDirIdentifier[0])
			printf(". %i\n", start->data.iLocLSBEXT);
		else if (start->data.szDirIdentifier[0] == 1) {
			printf(".. %i\n", start->data.iLocLSBEXT);
		} else {
			printf("Nazwa: %.25s %i \n", start->data.szDirIdentifier,
					start->data.iLocLSBEXT);
		}

		if (!start->data.iLocLSBEXT) {
			printf("CORRUPTED");
		}
		start = start->next;
	}
}
int __iso_read_path_table(const struct ISOFile *f,
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
			!= (size_t) f->strPVD.iPathTableSizeLSB) {
		isoError = ISO_PATH_TABLE_READ_FAILED;
		return -1;
	}

	size_t stCounter = 0;
	int items = 0;

	struct entryPathTableNode *first, *node = NULL, *prev = NULL;
	node = malloc(sizeof(struct entryPathTableNode));
	first = node;
	first->prev = NULL;

	while (stCounter != (size_t) f->strPVD.iPathTableSizeLSB) {

		if (stCounter > (size_t) f->strPVD.iPathTableSizeLSB) {
			prev = first;

			isoError = ISO_PATH_TABLE_READ_ENTRY_FAILED;
			do {
				node = prev->next;
				free(prev);
				prev = node;
			} while (node);
			free(bPathTable);
			return -1;
		}

		node->data.bLengthDIRID = bPathTable[stCounter];
		node->data.bExtAttrRecLength = bPathTable[stCounter + 1];
		node->data.iExtentLBA = *((int32_t*) (bPathTable + 2 + stCounter));
		node->data.sDirNumber = *((int16_t*) (bPathTable + 6 + stCounter));
		node->data.szDirIdentifier = malloc(node->data.bLengthDIRID);
		memcpy(node->data.szDirIdentifier, bPathTable + 8 + stCounter,
				node->data.bLengthDIRID);
		stCounter += 8 + node->data.bLengthDIRID;
		if (stCounter % 2)
			stCounter++;

		prev = node;
		node = malloc(sizeof(struct entryPathTableNode));
		items++;
		prev->next = node;
		node->prev = prev;
	}
	prev->next = NULL;
	free(node);
	free(bPathTable);
	*dest = first;
	return items - 1;
}

int __iso_read_file(FILE *f, const struct directoryDescriptor *startDir,
		struct ISOEntryFile *szDest) {

	if (startDir->bFlags & ISO_FILE_FLAG_SUBDIR) {
		isoError = ISO_DIRECTORY_NOT_FILE;
		return -1;
	}

	if (fseek(f, ISO_BLOCK_SIZE * startDir->iLocLSBEXT, SEEK_SET)) {
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		perror("fseek");
		return -1;
	}
	char *bData = malloc(startDir->iDataLengthLSB);

	if (fread(bData, 1, startDir->iDataLengthLSB, f)
			!= startDir->iDataLengthLSB) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		perror("fread");
		free(bData);
		return -1;
	}

	szDest->bData = bData;
	szDest->iLength = startDir->iDataLengthLSB;

	return startDir->iDataLengthLSB;
}


int iso_read_all_directories(FILE *f, const struct entryPathTableNode *start,
		struct directoryDescriptorNode **desc) {

	struct entryPathTableNode *node = start;

	int items = 0;
	while(node){
		items += __iso_read_directory_pt(f, start->data, desc);
		node = node->next;
	}

	return items;
}

/* dd */
int __iso_read_directory_pt(FILE* f, const struct entryPathTable *entry, struct directoryDescriptorNode **dest) {

	struct directoryDescriptorNode *first, *toRet = malloc(sizeof(*toRet)), *prev, *next;
	first = toRet;
	if (fseek(f, ISO_BLOCK_SIZE * entry->iExtentLBA, SEEK_SET)) {
		perror("fseek");
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		return -1;
	}
	/* Read root directory with whole directory info */
	char *temp = malloc(ISO_DIR_ENTRY_NOID_SIZE);

	if (fread(temp, 1, ISO_DIR_ENTRY_NOID_SIZE, f) != ISO_DIR_ENTRY_NOID_SIZE) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		free(temp);
		return -1;
	}

	size_t whole_dir = *((uint32_t*) (temp + 10));
	char *bData = malloc(whole_dir);
	free(temp);
	if (fseek(f, ISO_BLOCK_SIZE * entry->iExtentLBA, SEEK_SET)) {
		perror("fseek");
		free(toRet);
		free(bData);
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		return -1;
	}

	if (fread(bData, 1, whole_dir, f) != whole_dir) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		free(toRet);
		free(bData);
		return -1;
	}

	size_t counter = 0;
	int items = 0;

	while (counter != whole_dir) {
		toRet->data.bLengthDescriptor = bData[0 + counter];
		if (!toRet->data.bLengthDescriptor) {
			counter++;
			continue;
		}
		toRet->data.bEXTRecordLength = bData[1 + counter];
		toRet->data.iLocLSBEXT = *((uint32_t*) (bData + 2 + counter));
		toRet->data.iLocMSBEXT = *((uint32_t*) (bData + 6 + counter));
		toRet->data.iDataLengthLSB = *((uint32_t*) (bData + 10 + counter));
		toRet->data.iDataLengthMSB = *((uint32_t*) (bData + 14 + counter));
		memcpy(toRet->data.szRecDate, bData + 18 + counter, 7);
		toRet->data.bFlags = bData[25 + counter];
		toRet->data.bFSize = bData[26 + counter];
		toRet->data.bGapSize = bData[27 + counter];
		toRet->data.sVolSeqLSB = *((uint16_t*) (bData + 28 + counter));
		toRet->data.sVolSeqMSB = *((uint16_t*) (bData + 30 + counter));
		toRet->data.bLengthID = bData[32 + counter];
		toRet->data.szDirIdentifier = malloc(toRet->data.bLengthID);
		memcpy(toRet->data.szDirIdentifier, bData + 33 + counter,
				toRet->data.bLengthID);
		counter += toRet->data.bLengthDescriptor;
		if (counter % 2)
			counter++;

		items++;
		prev = toRet;
		next = malloc(sizeof(*toRet));
		memset(next, 0, sizeof(*next));
		toRet->next = next;
		toRet = next;
	}

	if (!toRet->data.bLengthDescriptor) {
		free(toRet);
		prev->next = NULL;
		toRet = prev;
	}
	*dest = first;
	return items;
}

int iso_find_file(struct ISOFile *f, char *szFileName,
		struct ISOEntryFile *dest) {
	struct entryPathTableNode *node;
	__iso_read_path_table(f, &node);

	struct directoryDescriptorNode *temp;

	while (node->prev) {
		if (!strncmp(node->data.szDirIdentifier, szFileName,
				node->data.bLengthDIRID)) {
		}
	}
	return 9;
}

#endif /* SRC_ISO_READ_C_ */
