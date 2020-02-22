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
			__iso_free_pathtable(&first);
			isoError = ISO_PATH_TABLE_READ_ENTRY_FAILED;
			return -1;
		}

		node->data.bLengthDIRID = bPathTable[stCounter];
		node->data.bExtAttrRecLength = bPathTable[stCounter + 1];
		node->data.iExtentLBA = *((int32_t*) (bPathTable + 2 + stCounter));
		node->data.sDirNumber = *((int16_t*) (bPathTable + 6 + stCounter));
		node->data.szDirIdentifier = __iso_calloc(node->data.bLengthDIRID);
		memcpy(node->data.szDirIdentifier, bPathTable + 8 + stCounter,
				node->data.bLengthDIRID);
		stCounter += 8 + node->data.bLengthDIRID;
		if (stCounter % 2)
			stCounter++;

		prev = node;
		node = __iso_calloc(sizeof(struct entryPathTableNode));
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
		isoError = ISO_FILE_ENTRY_SEEK_FAILED;
		perror("fseek");
		return -1;
	}
	char *bData = malloc(startDir->iDataLengthLSB);

	if (fread(bData, 1, startDir->iDataLengthLSB, f)
			!= startDir->iDataLengthLSB) {
		isoError = ISO_FILE_ENTRY_READ_FAILED;
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
	while (node) {
		items += __iso_read_directory_pt(f, start->data, desc);
		node = node->next;
	}

	return items;
}
int __iso_read_directories_root(char *bRootData, size_t stRootSize,
		struct directoryDescriptorNode **dest) {
	struct directoryDescriptorNode *first, *toRet = malloc(sizeof(*toRet)),
			*prev, *next;
	first = toRet;

	size_t counter = 0;
	int items = 0;

	while (counter != stRootSize) {
		if (counter > stRootSize) {
			isoError = ISO_READ_SIZE_MISMATCH;
			__iso_free_directores(&first);
			return -1;
		}
		if (!bRootData[counter]) {
			counter++;
			continue;
		}
		__iso_convert_byte_direntry(bRootData, &toRet->data, counter);

		counter += toRet->data.bLengthDescriptor;
		if (counter % 2)
			counter++;

		items++;
		prev = toRet;
		next = __iso_calloc(sizeof(*toRet));
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
int __iso_read_directory_rd(const struct ISOFile *f, struct ISOEntryFile **dest,
		const char *szPath) {

	char *szCopyDir = strndup(szPath, 64);

	struct directoryDescriptor rootDir;
	struct directoryDescriptorNode *currDirs;

	char *szToken = strtok(szCopyDir, "/");
	int iDepth = __iso_ret_path_depth(szPath);
	int iCurrDepth = 0;
	struct directoryDescriptor foundDir;
	foundDir.iDataLengthLSB = *((uint32_t*) (f->strPVD.szDirEntryRoot + 10));
	foundDir.iLocLSBEXT = *((uint32_t*) (f->strPVD.szDirEntryRoot + 2));

	char *bRootData;
	char booleanFound = 0, nowFile = 0;
	if (iCurrDepth == iDepth - 1) {
		szToken = strcat(szToken, ";1");
	}

	while (szToken) {
		if (fseek(f->fHandler, ISO_BLOCK_SIZE * foundDir.iLocLSBEXT,
				SEEK_SET)) {
			perror("fseek");
			isoError = ISO_DIRECTORY_ENTRY_SEEK_FAILED;
			free(szCopyDir);

			return -1;
		}
		bRootData = malloc(foundDir.iDataLengthLSB);

		if (fread(bRootData, 1, foundDir.iDataLengthLSB, f->fHandler)
				!= foundDir.iDataLengthLSB) {
			isoError = ISO_DIRECTORY_ENTRY_READ_FAILED;
			free(bRootData);
			free(szCopyDir);
			return -1;
		}

		if (__iso_read_directories_root(bRootData, foundDir.iDataLengthLSB, &currDirs) < 0) {
			return -1;
		}

		booleanFound = 0;
		while (!booleanFound && currDirs) {

			if (!strcmp(currDirs->data.szDirIdentifier, szToken)) {

				if (iCurrDepth != (iDepth - 1)
						&& (currDirs->data.bFlags & ISO_FILE_FLAG_SUBDIR)) {

					booleanFound = 1;

				} else if (iCurrDepth == (iDepth - 1)
						&& !(currDirs->data.bFlags & ISO_FILE_FLAG_SUBDIR)) {

					booleanFound = 1;
				}

				if (booleanFound == 1) {
					free(bRootData);
					foundDir = currDirs->data;
					break;
				}

			}

			currDirs = currDirs->next;
		}

		if (booleanFound) {
			szToken = strtok(NULL, "/");
			iCurrDepth++;
			if (iCurrDepth == iDepth - 1) {
				szToken = strcat(szToken, ";1");
			} else if (iCurrDepth == iDepth) {
				szToken = strtok(NULL, "/");
			}
		} else {
			break;
		}
	}

	if (booleanFound) {

		struct ISOEntryFile *found = __iso_calloc(sizeof(*found));
		if ( __iso_read_file(f->fHandler, &foundDir, *dest) < 0)
			return -1;
		else
			return (*dest)->iLength;
	}

	isoError = ISO_FILE_NOT_FOUND;
	return -1;
}
/* dd */
int __iso_read_directory_pt(FILE *f, const struct entryPathTable *entry,
		struct directoryDescriptorNode **dest) {

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
	free(temp);

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
	dest->szDirIdentifier = __iso_calloc(dest->bLengthID);
	memcpy(dest->szDirIdentifier, bData + 33 + offset, dest->bLengthID);
}

int iso_find_file_pt(struct ISOFile *f, char *szFileName,
		struct ISOEntryFile *dest) {
	char *szDirCopy = strndup(szFileName, 128);

	struct entryPathTableNode *node;
	__iso_read_path_table(f, &node);
	while (node->next) {
		node = node->next;
	}

	struct directoryDescriptorNode *temp;
	char *szDirs = strtok(szDirCopy, "/");

	while (szDirs) {

		szDirs = strtok(szDirCopy, "/");
	}

	return 9;
}

#endif /* SRC_ISO_READ_C_ */
