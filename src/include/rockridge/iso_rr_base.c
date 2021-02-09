/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_RR_BASE_C_
#define SRC_ISO_RR_BASE_C_
#include "iso_rr_base.h"

void iso_rr_print_error(char *szCause) {
	printf("%s: ISO_RR_Error: %s \n", szCause, szISORRErrors[isoRRError]);

}

int __iso_rr_find_signature(const struct directoryDescriptor *src,
		const char szSignature[2], struct rrBase *dest, size_t *poffset) {
	char *bDirExtData = src->szDirExtData;
	char bFound = 0;
	int offset = 0;

	while (offset < (src->iDirExtDataLength - 3)) {
		if (bDirExtData[offset] == szSignature[0]
				&& bDirExtData[offset + 1] == szSignature[1]
				&& bDirExtData[offset + 2] && bDirExtData[offset + 3] == 1) {
			bFound = 1;
			break;
		}
		offset++;
	}

	if (bFound) {
		dest->bLength = bDirExtData[offset + 2];
		dest->bSystemEntry = bDirExtData[offset + 3];
		memcpy(dest->szSignature, szSignature, 2);
		*poffset = offset;
		return 0;
	} else {
		return -1;
	}
}

int __iso_rr_find_altname(const struct directoryDescriptor *src,
		struct rrPosixAlterName **dest, size_t *pos) {

	char szSignature[2] = { 'N', 'M' };
	struct rrPosixAlterName *toRet = __iso_calloc(sizeof(*toRet));
	size_t px_offset;
	if (__iso_rr_find_signature(src, szSignature, &toRet->strBaseData,
			&px_offset)) {
		isoRRError = ISO_RR_ATTRIBUTE_NOT_FOUND;
		free(toRet);
		return -1;
	}
	*pos += px_offset;
	char *bExtData = src->szDirExtData;
	toRet->szNameContent = __iso_calloc(toRet->strBaseData.bLength - 4);

	memcpy(toRet->szNameContent, px_offset + bExtData + 5,
			toRet->strBaseData.bLength - 5);
	toRet->bFlags = *(bExtData + 4 + px_offset);
	*dest = toRet;
	return 0;
}

int __iso_rr_find_timesig(const struct directoryDescriptor *src,
		struct rrTimeStamp **dest) {
	char szSignature[2] = { 'T', 'F' };
	struct rrTimeStamp *toRet = __iso_calloc(sizeof(*toRet));
	size_t px_offset;
	if (__iso_rr_find_signature(src, szSignature, &toRet->strBaseData,
			&px_offset)) {
		isoRRError = ISO_RR_ATTRIBUTE_NOT_FOUND;
		free(toRet);
		return -1;
	}
	char *bExtData = src->szDirExtData;
	//toRet->bTimeStampData = __iso_calloc(toRet->strBaseData.bLength - 4);
	//memcpy(toRet->bTimeStampData, px_offset + bExtData + 5,
	//		toRet->strBaseData.bLength - 5);
	toRet->bFlags = *(bExtData + 4 + px_offset);
	*dest = toRet;
	return 0;
}

int __iso_rr_reconcat(char **dst, char *src, size_t src_len, size_t dst_len) {

	*dst = realloc(*dst, dst_len + src_len + 2);
	if (!*dst)
		return -1;

	strcat(*dst, src);
	return 0;
}
int __iso_rr_get_dir_id(char **szDest, const struct directoryDescriptor *strDir,
		size_t *offset) {

	size_t stDst = 0;
	if (!*szDest)
		*szDest = __iso_calloc(1);
	else
		stDst = strlen(*szDest);

	struct rrPosixAlterName *temp;

	if (__iso_rr_find_altname(strDir, &temp, offset) < 0) {
		iso_rr_print_error("find_altername");
		free(*szDest);
		return -1;
	}

	if (temp->bFlags & ISO_RR_NM_CURRENT_FLAG) {
		char szParent[] = { '.', 0 };
		__iso_rr_reconcat(szDest, szParent, 1,stDst);
	} else if (temp->bFlags & ISO_RR_NM_PARENT_FLAG) {
		char szParent[] = { '.', '.', 0 };
		__iso_rr_reconcat(szDest, szParent, 2,stDst);
	} else {
		*offset += temp->strBaseData.bLength;
		if (__iso_rr_reconcat(szDest, temp->szNameContent,
				temp->strBaseData.bLength - 4, stDst) < 0) {
			offset -= temp->strBaseData.bLength;
			free(temp->szNameContent);
			free(temp);
			free(*szDest);
			return -1;
		}
		if (temp->bFlags & ISO_RR_NM_CONTINUE_FLAG) {
			__iso_rr_get_dir_id(szDest, strDir, offset);
		}
	}

	free(temp->szNameContent);
	free(temp);

	return 0;
}
int __iso_rr_print_attr(const struct directoryDescriptor *src) {
	struct rrTimeStamp *time = NULL;
	char *szAltName = NULL;
	size_t startPos = 0;

	printf("Printing RockRidge extensions if present.\n");
	if (!__iso_rr_get_dir_id(&szAltName, src, &startPos)) {
		printf("Alternate Name: %s\n", szAltName);
		free(szAltName);
		szAltName = NULL;
	}

	if (!__iso_rr_find_timesig(src, &time)) {
		printf("TimeStamp:");
	}

	free(time);

	return 0;
}

int __iso_rr_posix_attributes(const struct directoryDescriptor *src,
		struct rrPosixAttributes **dest) {
	char szSignature[2] = { 'P', 'X' };
	struct rrPosixAttributes *toRet = __iso_calloc(sizeof(*toRet));
	size_t px_offset;

	if (__iso_rr_find_signature(src, szSignature, &toRet->strBaseData,
			&px_offset)) {
		isoRRError = ISO_RR_ATTRIBUTE_NOT_FOUND;
		free(toRet);
		return -1;
	}
	return 0;
}
#endif /* SRC_ISO_RR_BASE_C_ */
