/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_RR_TYPEDEFS_H_
#define SRC_ISO_RR_TYPEDEFS_H_

#include <stdlib.h>
#include <stdio.h>

#include "iso_rr_structs.h"

#define ISO_RR_ERRORS_COUNT 128

enum ISO_RR_ERRORS {
	ISO_RR_SUCCESS,
	ISO_RR_FAILED_SEEK,
	ISO_RR_FAILED_READ,
	ISO_RR_ATTRIBUTE_NOT_FOUND
};

const char szISORRErrors[ISO_RR_ERRORS_COUNT][128] = {
		"No errors reported so far",
		"Failed to seek descriptor entry for given attribute.",
		"Failed to read descriptor entry for given attribute.",
		"Failed to find a signature for given attribute."
};


char isoRRError = ISO_RR_SUCCESS;

#endif /* SRC_ISO_RR_TYPEDEFS_H_ */

