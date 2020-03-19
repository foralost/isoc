
/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_BASE_C_
#define SRC_ISO_BASE_C_
#include "iso_base.h"

void iso_set_verbose(uint8_t val){
	isoVerbose = val;
}

int iso_alloc_sector(FILE *f, char* dest, int iSector)
{
	if (fseek(f, ISO_BLOCK_SIZE * iSector, SEEK_SET)) {
		perror("fseek");
		return -1;
	}

	dest = (char*)__iso_calloc(ISO_BLOCK_SIZE);

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
	printf("%.64s: ISO Error: %s\n", szIssue, szISOErrors[isoError]);
}

int iso_open(char* szPath, struct ISOFile** dest)
{
	FILE* handler = fopen(szPath, "rb");

	if(!handler)
	{
		perror("fopen: ");
		return -1;
	}

	struct ISOFile* toRet = __iso_calloc(sizeof (struct ISOFile) );
	toRet->fHandler = handler;
	int iRes = fseek(handler, 0x10*ISO_BLOCK_SIZE, SEEK_SET);
	if(iRes != 0)
	{
		perror("fseek: ");
		iso_close(toRet);
		return -1;
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
		return -1;
	}

	const struct primaryDescriptor* PVD = &toRet->strPVD;

	if(PVD->iType != ISO_PRIMARY_DESCRIPTOR_ID)
	{
		isoError = ISO_NOT_A_PRIMARY_DESCR;
		iso_close(toRet);
		return -1;
	}

	for(int i = 0 ; i < 5; i++)
	{
		if(PVD->szID[i] != szCDID[i])
		{
			isoError = ISO_FAILED_CD_STRING;
			iso_close(toRet);
			return -1;
		}
	}

	toRet->fHandler = handler;
	*dest = toRet;
	return 0;
}
#endif /* SRC_ISO_BASE_C_ */
