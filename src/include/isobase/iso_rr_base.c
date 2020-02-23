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
	printf("%s: ISO_RR_Error: %s", szCause, szISORRErrors[isoRRError]);

}

size_t __iso_rr_find_signature(const struct directoryDescriptor *src,
		const char szSignature[2], struct rrBase *dest) {
	char *bDirExtData = src->szDirExtData;
	char bFound = 0;
	size_t offset = 0;

	while (offset < (src->iDirExtDataLength - 3)) {
		if (bDirExtData[offset] == szSignature[0]
				&& bDirExtData[offset + 1] == szSignature[1]
				&& !bDirExtData[offset + 2] && bDirExtData[offset + 3] == 1) {
			bFound = 1;
			break;
		}
	}

	if (bFound) {
		dest->bLength = bDirExtData[offset + 2];
		dest->bSystemEntry = bDirExtData[offset + 3];
		memcpy(dest->szSignature, szSignature, 2);
		return offset;
	} else {
		return -1;
	}
}
int __iso_rr_posix_attributes(FILE *isoFile,
		const struct directoryDescriptor *src, struct rrPosixAttributes **dest) {
	char szSignature[2] = { 'P', 'X' };
	struct rrPosixAttributes *toRet = __iso_calloc(sizeof(*toRet));

	size_t px_offset = __iso_rr_find_signature(src, szSignature,
			&toRet->strBaseData);
	if (px_offset == -1 && !toRet->strBaseData.bLength) {
		isoRRError = ISO_RR_ATTRIBUTE_NOT_FOUND;
		return -1;
	}

	return 0;
}
#endif /* SRC_ISO_RR_BASE_C_ */
