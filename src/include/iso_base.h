/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_BASE_H_
#define SRC_ISO_BASE_H_
#include "iso_typedefs.h"

int iso_alloc_sector(FILE *f, char* dest, int iSector)
{
	if (fseek(f, ISO_BLOCK_SIZE * iSector, SEEK_SET)) {
		perror("fseek");
		return -1;
	}

	dest = malloc(ISO_BLOCK_SIZE);
	memset(dest, 0, ISO_BLOCK_SIZE);

	if (fread(dest, 1, ISO_BLOCK_SIZE, f) != ISO_BLOCK_SIZE) {
		perror("fread");
		free(dest);
		return -1;
	}

	return 0;
}

int iso_close(struct ISOFile* obj)
{
	int iResult = fclose(obj->fHandler);
	obj->fHandler = NULL;
	if(iResult < 0){
		perror("fclose: ");
		return iResult;
	}
	free(obj);
	return 0;
}


void iso_print_error(char* szIssue)
{
	printf("%s: ISO Error: %s\n", szIssue, szISOErrors[isoError] );
}

struct ISOFile* iso_open(char* szPath)
{
	FILE* handler = fopen(szPath, "rb");

	if(handler <= 0)
	{
		perror("fopen: ");
		return NULL;
	}

	struct ISOFile* toRet = malloc(sizeof (struct ISOFile) );
	memset(toRet, 0, sizeof(*toRet));
	toRet->fHandler = handler;
	int iRes = fseek(handler, 0x10*ISO_BLOCK_SIZE, SEEK_SET);
	if(iRes != 0)
	{
		perror("fseek: ");
		iso_close(toRet);
		return NULL;
	}

	int bRead = fread(&toRet->strPVD, 1, sizeof(toRet->strPVD), handler);
	if(bRead <= 0)
	{
		perror("fread: ");
		iso_close(toRet);
		return NULL;
	} else if (bRead != ISO_BLOCK_SIZE) {
		isoError = ISO_READ_SIZE_MISMATCH;
		iso_close(toRet);
		return NULL;
	}

	const struct primaryDescriptor* PVD = &toRet->strPVD;

	if(PVD->iType != ISO_PRIMARY_DESCRIPTOR_ID)
	{
		isoError = ISO_NOT_A_PRIMARY_DESCR;
		iso_close(toRet);
		return NULL;
	}

	for(int i = 0 ; i < 5; i++)
	{
		if(PVD->szID[i] != szCDID[i])
		{
			isoError = ISO_FAILED_CD_STRING;
			iso_close(toRet);
			return NULL;
		}
	}

	toRet->fHandler = handler;
	return toRet;
}
#endif /* SRC_ISO_BASE_H_ */
