#include "iso_read.h"

#ifndef SRC_ISO_READ_C_
#define SRC_ISO_READ_C_

int __iso_malloc_root_dir(char **bDest, FILE *f,
		const struct directoryDescriptor *src) {
	if (fseek(f, ISO_BLOCK_SIZE * src->iLocLSBEXT, SEEK_SET)) {
		perror("fseek");
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		return -1;
	}
	char *bData = malloc(src->iDataLengthLSB);

	if (fread(bData, 1, src->iDataLengthLSB, f)
			!= (size_t) src->iDataLengthLSB) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		free(bData);
		return -1;
	}

	*bDest = bData;
	return 0;
}

int __iso_cmp_dir_id(const struct directoryDescriptor *strDir,
		const char *szStr) {
	int toRet = -1;
	size_t startPos = 0;
	if (isoUseRR) {
		struct rrPosixAlterName *temp;
		if (__iso_rr_find_altname(strDir, &temp, &startPos) < 0) {

			if (isoVerbose)
				iso_rr_print_error("find_altername");

			toRet = strncmp(strDir->szDirIdentifier, szStr, strDir->bLengthID);
		} else {
			toRet = strncmp(temp->szNameContent, szStr,
					temp->strBaseData.bLength - 5);
			free(temp->szNameContent);
			free(temp);
		}
	} else {
		toRet = strncmp(strDir->szDirIdentifier, szStr, strDir->bLengthID);
	}
	return toRet;

}
int __iso_print_dir_tree(FILE *f, const struct directoryDescriptor *src) {
	char *bDirData;
	if (__iso_malloc_root_dir(&bDirData, f, src))
		return -1;
	struct directoryDescriptorNode *start;

	if (__iso_read_directories_root(bDirData, src->iDataLengthLSB, &start)
			< 0) {
		free(bDirData);
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		return -1;
	}

	struct directoryDescriptorNode *current = start;
	char *szDirId = NULL, bFound;
	while (current) {
		if (isoUseRR) {
			size_t startPos = 0;
			if (__iso_rr_get_dir_id(&szDirId, current->data, &startPos) < 0) {
				szDirId = current->data->szDirIdentifier;
				bFound = 0;
			} else {
				bFound = 1;
			}
		} else {
			szDirId = current->data->szDirIdentifier;
		}
		if (current->data->bFlags & ISO_FILE_FLAG_SUBDIR) {
			switch (szDirId[0]) {
			case 1:
				printf("..\n");
				break;
			case 0:
				printf(".\n");
				break;
			default:
				printf("Folder: %.128s \n", szDirId);
			}

		} else {
			printf("File: %.128s \n", szDirId);
		}

		if (bFound){
			free(szDirId);
		}
		szDirId = NULL;
		current = current->next;
	}

	__iso_free_directories(start);
	free(bDirData);
	return 0;
}

int __iso_read_directory_pt(FILE *f, const struct entryPathTable *entry,
		struct directoryDescriptorNode **dest) {

	if (fseek(f, ISO_BLOCK_SIZE * entry->iExtentLBA, SEEK_SET)) {
		perror("fseek");
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		return -1;
	}
	/* Read root directory with whole directory info */
	char *bDirNoID = malloc(ISO_DIR_ENTRY_NOID_SIZE);

	if (fread(bDirNoID, 1, ISO_DIR_ENTRY_NOID_SIZE,
			f) != ISO_DIR_ENTRY_NOID_SIZE) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		free(bDirNoID);
		return -1;
	}

	size_t whole_dir = *((uint32_t*) (bDirNoID + 10));
	free(bDirNoID);

	char *bData = malloc(whole_dir);
	if (fseek(f, ISO_BLOCK_SIZE * entry->iExtentLBA, SEEK_SET)) {
		perror("fseek");
		free(bData);
		isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
		return -1;
	}

	if (fread(bData, 1, whole_dir, f) != whole_dir) {
		isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
		free(bData);
		return -1;
	}

	return __iso_read_directories_root(bData, whole_dir, dest);
}

int __iso_read_path_table(const struct ISOFile *f,
		struct entryPathTableNode **dest) {
	if (fseek(f->fHandler, ISO_BLOCK_SIZE * f->strPVD.iLocTypeLPath,
	SEEK_SET)) {
		perror("fseek: ");
		isoError = ISO_PATH_TABLE_SEEK_FAILED;
		return -1;
	}

	char *bPathTable = __iso_calloc(f->strPVD.iPathTableSizeLSB);

	if (fread(bPathTable, 1, f->strPVD.iPathTableSizeLSB, f->fHandler)
			!= (size_t) f->strPVD.iPathTableSizeLSB) {
		isoError = ISO_PATH_TABLE_READ_FAILED;
		free(bPathTable);
		perror("fread");
		return -1;
	}

	size_t stCounter = 0;
	int items = 0;

	struct entryPathTableNode *first, *node = NULL, *prev = NULL;
	node = __iso_calloc(sizeof(struct entryPathTableNode));

	first = node;
	first->prev = NULL;

	while (stCounter != (size_t) f->strPVD.iPathTableSizeLSB) {

		if (stCounter > (size_t) f->strPVD.iPathTableSizeLSB) {
			__iso_free_pathtable(first);
			free(bPathTable);
			isoError = ISO_PATH_TABLE_READ_ENTRY_FAILED;
			return -1;
		}

		node->data = __iso_calloc(sizeof(*node->data));
		node->data->bLengthDIRID = bPathTable[stCounter];
		node->data->bExtAttrRecLength = bPathTable[stCounter + 1];
		node->data->iExtentLBA = *((int32_t*) (bPathTable + 2 + stCounter));
		node->data->sDirNumber = *((int16_t*) (bPathTable + 6 + stCounter));
		node->data->szDirIdentifier = __iso_calloc(node->data->bLengthDIRID);
		memcpy(node->data->szDirIdentifier, bPathTable + 8 + stCounter,
				node->data->bLengthDIRID);

		stCounter += 8 + node->data->bLengthDIRID;
		if (stCounter % 2)
			stCounter++;

		items++;
		prev = node;
		node = __iso_calloc(sizeof(*node));
		node->data = __iso_calloc(sizeof(*node->data));
		node->prev = prev;
		prev->next = node;
	}

	prev->next = NULL;
	free(node);
	free(node->data);
	items--;

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

	if (__iso_malloc_root_dir(&szDest->bData, f, startDir))
		return -1;

	szDest->iLength = startDir->iDataLengthLSB;
	return startDir->iDataLengthLSB;
}

int iso_read_all_directories(FILE *f, const struct entryPathTableNode *start,
		struct directoryDescriptorNode **desc) {

	const struct entryPathTableNode *node = start;

	int items = 0;
	while (node) {
		items += __iso_read_directory_pt(f, start->data, desc);
		node = node->next;
	}

	return items;
}

void __iso_convert_byte_direntry(char *bData, struct directoryDescriptor *dest,
		size_t offset) {
	dest->bLengthDescriptor = bData[offset];
	dest->bEXTRecordLength = bData[1 + offset];
	dest->iLocLSBEXT = *((uint32_t*) (bData + 2 + offset));
	dest->iLocMSBEXT = *((uint32_t*) (bData + 6 + offset));
	dest->iDataLengthLSB = *((uint32_t*) (bData + 10 + offset));
	dest->iDataLengthMSB = *((uint32_t*) (bData + 14 + offset));
	memcpy(dest->szRecDate, bData + 18 + offset, 7);
	dest->bFlags = bData[25 + offset];
	dest->bFSize = bData[26 + offset];
	dest->bGapSize = bData[27 + offset];
	dest->sVolSeqLSB = *((uint16_t*) (bData + 28 + offset));
	dest->sVolSeqMSB = *((uint16_t*) (bData + 30 + offset));
	dest->bLengthID = bData[32 + offset];
	dest->szDirIdentifier = __iso_calloc(dest->bLengthID + 1);
	memcpy(dest->szDirIdentifier, bData + 33 + offset, dest->bLengthID);
	uint32_t iExtraSize = dest->bLengthDescriptor - 33 - dest->bLengthID;
	if (iExtraSize > 0) {
		dest->iDirExtDataLength = iExtraSize;
		dest->szDirExtData = __iso_calloc(iExtraSize);
		memcpy(dest->szDirExtData, bData + offset + dest->bLengthID + 33,
				iExtraSize);
	}
}

int __iso_read_directories_root(char *bRootData, size_t stRootSize,
		struct directoryDescriptorNode **dest) {
	struct directoryDescriptorNode *first = NULL, *current = NULL, *prev = NULL;

	size_t counter = 0;
	int items = 0;
	current = __iso_calloc(sizeof(*current));
	current->data = __iso_calloc(sizeof(*current->data));
	first = current;
	while (counter != stRootSize) {

		if (counter > stRootSize) {
			isoError = ISO_READ_SIZE_MISMATCH;
			__iso_free_directories(first);
			return -1;
		}
		if (!bRootData[counter]) {
			counter++;
			continue;
		}

		__iso_convert_byte_direntry(bRootData, current->data, counter);

		counter += current->data->bLengthDescriptor;
		if (counter % 2)
			counter++;

		prev = current;
		current = __iso_calloc(sizeof(*current));
		current->data = __iso_calloc(sizeof(*current->data));
		prev->next = current;
		items++;
	}

	prev->next = NULL;
	__iso_free_directories(current);
	items--;
	*dest = first;
	return items;
}

int __iso_ret_path_depth(const char *szPath) {
	int count = 0;
	int i = 0;
	while (szPath[i]) {
		if (szPath[i] == '/')
			count++;
		i++;
	}

	return count;
}
int __iso_read_directory_rd(const struct ISOFile *f,
		struct directoryDescriptor **dest, const char *szPath) {

	char *szCopyDir = strndup(szPath, 128);
	struct directoryDescriptorNode *currDirs = NULL, *start = NULL;
	char bNull = 0;
	char *szToken = strtok(szCopyDir, "/");

	if (!szToken)
		szToken = &bNull;

	struct directoryDescriptor *foundDir = __iso_calloc(sizeof(*foundDir));

	foundDir->iDataLengthLSB = *((uint32_t*) (f->strPVD.szDirEntryRoot + 10));
	foundDir->iLocLSBEXT = *((uint32_t*) (f->strPVD.szDirEntryRoot + 2));

	char *bRootData;

	do {
		if (__iso_malloc_root_dir(&bRootData, f->fHandler, foundDir)) {
			__iso_free_directory(foundDir);
			break;
		}

		if (__iso_read_directories_root(bRootData, foundDir->iDataLengthLSB,
				&currDirs) < 0) {
			isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
			free(bRootData);
			__iso_free_directory(foundDir);
			free(szCopyDir);
			return -1;
		}

		free(bRootData);
		start = currDirs;
		while (!foundDir->szDirIdentifier && currDirs) {

			if (!__iso_cmp_dir_id(currDirs->data, szToken)
					&& (currDirs->data->bFlags & ISO_FILE_FLAG_SUBDIR)) {

				__iso_free_directory(foundDir);
				foundDir = currDirs->data;
				currDirs->data = NULL;
				szToken = NULL;
			}

			currDirs = currDirs->next;
		}
		__iso_free_directories(start);
		if (!foundDir->szDirIdentifier) {
			szToken = strtok(NULL, "/");
		}

	} while (!foundDir->szDirIdentifier && szToken);

	if (foundDir && foundDir->szDirIdentifier) {
		free(szCopyDir);
		*dest = foundDir;
		return 0;
	}
	__iso_free_directory(foundDir);
	free(szCopyDir);
	isoError = ISO_FILE_NOT_FOUND;
	return -1;
}

int __iso_read_dir_for_dirfile(const char *szFileName, FILE *f,
		const struct directoryDescriptor *src,
		struct directoryDescriptor **dirfile) {
	size_t stLength = strnlen(szFileName, 64);
	char *szDuplicate = __iso_calloc(stLength + 3);
	memcpy(szDuplicate, szFileName, stLength);
	szDuplicate[stLength] = ';';
	szDuplicate[stLength + 1] = '1';
	char *bRootData;

	if (__iso_malloc_root_dir(&bRootData, f, src)) {
		free(szDuplicate);
		return 01;
	}

	struct directoryDescriptorNode *currDirs, *start;
	__iso_read_directories_root(bRootData, src->iDataLengthLSB, &currDirs);
	start = currDirs;
	struct directoryDescriptor *foundDir = NULL;

	while (currDirs) {

		if (!__iso_cmp_dir_id(currDirs->data, szFileName)
				&& !(currDirs->data->bFlags & ISO_FILE_FLAG_SUBDIR)) {
			foundDir = currDirs->data;
			currDirs->data = NULL;
			break;
		}
		currDirs = currDirs->next;
	}
	__iso_free_directories(start);
	if (!foundDir) {
		return -1;
	}
	*dirfile = foundDir;
	return 0;
}
/* dd */

#endif /* SRC_ISO_READ_C_ */
