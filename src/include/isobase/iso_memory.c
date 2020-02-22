/*
 * iso_memory.c
 *
 *  Created on: 18 lut 2020
 *      Author: foralost
 */
#include "iso_memory.h"

#ifndef SRC_ISO_MEMORY_C_
#define SRC_ISO_MEMORY_C_

void* __iso_calloc(size_t size){
	void* toRet = malloc(size);
	memset(toRet, 0, size);
	return toRet;
}

void __iso_free_directores(struct directoryDescriptorNode **start){

	while(*start){
		free((*start)->data.szDirIdentifier);
		memset(*start, 0, sizeof(struct directoryDescriptorNode));
		free((*start));
		*start = (*start)->next;
	}

	*start = NULL;
}


void __iso_free_pathtable(struct entryPathTableNode **start)
{
	while(*start)
	{
		free((*start)->data.szDirIdentifier);
		memset(*start, 0, sizeof(struct entryPathTableNode));
		free(*start);
		*start = (*start)->next;
	}

	*start = NULL;
}


#endif
