/*
 * iso_memory.c
 *
 *  Created on: 18 lut 2020
 *      Author: foralost
 */


#ifndef SRC_ISO_MEMORY_C_
#define SRC_ISO_MEMORY_C_
#include "iso_memory.h"

void* __iso_calloc(size_t size) {
	void *toRet = malloc(size);
	memset(toRet, 0, size);
	return toRet;
}

int __iso_copy_directory(struct directoryDescriptor **to, const struct directoryDescriptor *from)
{
	if(*to)
		return -1;

	*to = __iso_calloc(sizeof(struct directoryDescriptor));
	(*to)->szDirExtData = __iso_calloc(from->iDirExtDataLength);
	(*to)->szDirIdentifier = __iso_calloc(from->bLengthID);

	memcpy(*to, from, sizeof(struct directoryDescriptor));
	memcpy((*to)->szDirExtData, from->szDirExtData, from->iDirExtDataLength);
	memcpy((*to)->szDirIdentifier, from->szDirIdentifier, from->bLengthID);
	return 0;
}

void __iso_free_directory(struct directoryDescriptor* dest)
{
	free(dest->szDirIdentifier);
	free(dest->szDirExtData);
	memset(dest, 0, sizeof(struct directoryDescriptor));
	free(dest);
}
void __iso_free_directories(struct directoryDescriptorNode *start) {

	struct directoryDescriptorNode *temp;
	while (start) {
		if(start->data){
			__iso_free_directory(start->data);
			start->data = NULL;
		}
		temp = start->next;
		memset(start, 0, sizeof(struct directoryDescriptorNode));
		free(start);
		start = temp;
	}
}

void __iso_free_pathtable(struct entryPathTableNode *start) {
	while (start) {
		if(start->data){
			free(start->data->szDirIdentifier);
			memset(start->data, 0, sizeof(struct entryPathTable));
			free(start->data);
		}

		memset(start, 0, sizeof(struct entryPathTableNode));
		free(start);
		start = start->next;
	}
}

#endif
