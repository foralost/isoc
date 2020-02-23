/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_RR_BASE_H_
#define SRC_ISO_RR_BASE_H_
#include "../isobase/iso_func.h"
#include "iso_rr_typedefs.h"

int __iso_rr_posix_attributes(FILE *isoFile,
		const struct directoryDescriptor *src, struct rrPosixAttributes **dest);

#endif /* SRC_ISO_RR_BASE_H_ */
