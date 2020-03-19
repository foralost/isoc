#ifndef SRC_ISO_READ_H
#define SRC_ISO_READ_H_
#include "iso_typedefs.h"

int __iso_read_directories_root(char *bRootData, size_t stRootSize,
		struct directoryDescriptorNode **dest);

#endif /* SRC_ISO_READ_H_ */
