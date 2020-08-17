#define FAT32_FSINFO_LEAD_SIG 		0x41615252
#define FAT32_FSINFO_ANTH_SIG 		0x61417272
#define FAT32_FSINFO_TRAI_SIG 		0xAA550000

#define FAT32_DIR_LONG_ENTRY_MASK 	0x40
#define FAT32_DIR_ATTR_READ_ONLY  	0x01
#define FAT32_DIR_ATTR_HIDDEN  		0x02
#define FAT32_DIR_ATTR_SYSTEM  		0x04
#define FAT32_DIR_ATTR_VOLUME_ID  	0x08
#define FAT32_DIR_ATTR_DIRECTORY  	0x10
#define FAT32_DIR_ATTR_ARCHIVE  	0x20
#define FAT32_DIR_ATTR_LFN  		FAT32_DIR_ATTR_READ_ONLY|FAT32_DIR_ATTR_HIDDEN|FAT32_DIR_ATTR_SYSTEM|FAT32_DIR_ATTR_VOLUME_ID
#define FAT32_DIR_ATTR_LONG_NAME 	0x0F
#define FAT32_DIR_FREE				(unsigned char)0xE5
#define	FAT32_DIR_FREE_END			0x0
#define	FAT32_DIR_JAPAN 			0x05
#define FAT32_DIR_ENTRY_SIZE		32

#define FAT32_CLUSTER_END_LOW		0x0FFFFFF8
#define FAT32_CLUSTER_END_HI		0x0FFFFFFF
#define FAT32_CLUSTER_INVALID		0x0FFFFFF7
#define FAT32_CLUSTER_RESERVED		0x00000001
#define FAT32_CLUSTER_AVAILABLE		0x00000000

enum FAT32_ERRORS {
	FAT32_SUCCESS,
	FAT32_ERROR_READING_SECTOR,
	FAT32_ERROR_SEEKING_SECTOR,
	FAT32_INVALID_FSINFO_SIG,
	FAT32_INVALID_BOOT_SIG,
	FAT32_INVALID_SYSIDSTR,
	FAT32_INVALID_EBR_SIG,
	FAT32_VIOLATION_RSV_BYTES,
	FAT32_ERROR_READING_CLUSTER,
	FAT32_NOT_A_REGULAR_FILE,
	FAT32_NOT_A_LONG_DIR,
	FAT32_NOT_ENOUGH_CLUSTERS
};

uint8_t fatError = FAT32_SUCCESS;

char szFatErrors[32][128] = { "No errors reported so far.",
		"Error during reading a sector", "Error during seeking a sector.",
		"There was an invalid signature for filesystem info structure.",
		"There was an invalid boot signature found.",
		"Invalid system identifier string found in extended boot record.",
		"Invalid signature found in extended boot record.",
		"The reserved bytes were violated by containing nonzero value.",
		"Failed to read cluster.", "Given directory is not a long directory.",
		"Given file is not a regular file.",
		"There is not enough free clusters." };

#include "fat_c.c"

