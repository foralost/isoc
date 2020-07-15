/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_BASE_H_
#define SRC_ISO_BASE_H_

#include "iso_typedefs.h"

int iso_alloc_sector(FILE *f, char* dest, int iSector);

int iso_close(struct ISOFile* obj);

void iso_print_error(char* szIssue);

struct ISOFile* iso_open(char* szPath);

#endif /* SRC_ISO_BASE_H_ */
