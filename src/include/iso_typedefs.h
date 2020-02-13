
/*
 * iso_base.h
 *
 *  Created on: 13 lut 2020
 *      Author: foralost
 */

#ifndef SRC_ISO_TYPEDEFS_H_
#define SRC_ISO_TYPEDEFS_H_

#include <stdio.h>
#include <stdint.h>

#define ISO_BLOCK_SIZE 2048
#define ISO_PRIMARY_DESCRIPTOR_ID 0x01

#define ISO_ERROR_COUNT 64

enum ISO_ERROR{
	ISO_NO_ERROR,
	ISO_NOT_A_PRIMARY_DESCR,
	ISO_FAILED_CD_STRING,
	ISO_READ_SIZE_MISMATCH,
	ISO_PATH_TABLE_SEEK_FAILED,
	ISO_PATH_TABLE_READ_FAILED,
	ISO_PATH_TABLE_READ_ENTRY_FAILED,
	ISO_DIRECTORY_ENTRY_SEEK_FAILED,
	ISO_DIRECTORY_ENTRY_READ_FAILED
};

uint8_t isoError = ISO_NO_ERROR;

const char szISOErrors[ISO_ERROR_COUNT][128] =
{
		{"No error reported so far."},
		{"Loaded .iso file does not have a valid ID for Primary Volume Descriptor."},
		{"Loaded .iso file does not have a valid 'CD001' string. "},
		{"Loaded .iso file is less than normal block size."},
		{"Failed to seek Path Table in .ISO file."},
		{"Failed to read Path Table in .ISO file."},
		{"The Path Table in given ISO file seems to be corrupted!"},
		{"Failed to seek directory entry in .ISO file."},
		{"Failed to read directory entry in .iSO file."}
};

const char* szCDID = "CD001";





#endif /* SRC_ISO_TYPEDEFS_H_ */



