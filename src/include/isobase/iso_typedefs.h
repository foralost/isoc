
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
#include <stdlib.h>
#include <string.h>
#include "iso_structs.h" 

#define ISO_DIR_ENTRY_NOID_SIZE 33

#define ISO_TRUE	1
#define ISO_FALSE	0

#define ISO_BLOCK_SIZE 2048
#define ISO_PRIMARY_DESCRIPTOR_ID 0x01
#define ISO_FILE_FLAG_EXIST 1 << 0
#define ISO_FILE_FLAG_SUBDIR	1 << 1
#define ISO_FILE_FLAG_ASSOC 1 << 2
#define ISO_FILE_FLAG_FORMAT	1 << 3
#define ISO_FILE_FLAG_PERMISSIONS	1 << 4
#define ISO_FILE_FLAG_NOT_FINAL_DIR 1 << 7

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
	ISO_DIRECTORY_ENTRY_READ_FAILED,
	ISO_DIRECTORY_NOT_FILE,
	ISO_FILE_ENTRY_READ_FAILED,
	ISO_FILE_ENTRY_SEEK_FAILED,
	ISO_FILE_NOT_FOUND
};

uint8_t isoError = ISO_NO_ERROR;
uint8_t isoVerbose = ISO_TRUE;
uint8_t isoUseRR = ISO_TRUE;

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
		{"Failed to read directory entry in .iSO file."},
		{"Given directory is not a file."},
		{"Found file to be searched, but it could not be read."},
		{"Found file to be searched, but it could not be seeked."},
		{"Given file could not be found in given .iso file."}
};

const char* szCDID = "CD001";
#endif /* SRC_ISO_TYPEDEFS_H_ */



