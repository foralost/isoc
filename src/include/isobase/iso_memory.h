/*
 * iso_memory.h
 *
 *  Created on: 18 lut 2020
 *      Author: foralost
 */
#include "iso_typedefs.h"

#ifndef SRC_ISO_MEMORY_H_
#define SRC_ISO_MEMORY_H_

void* __iso_calloc(size_t size);

void __iso_free_directores(struct directoryDescriptorNode **start);

void __iso_free_pathtable(struct entryPathTableNode **start);
#endif /* SRC_INCLUDE_ISO_MEMORY_H_ */
