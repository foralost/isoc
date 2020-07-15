#define FAT32_FSINFO_LEAD_SIG 0x41615252
#define FAT32_FSINFO_ANTH_SIG 0x61417272
#define FAT32_FSINFO_TRAI_SIG 0xAA550000

enum FAT32_ERRORS  {
		FAT32_SUCCESS,
		FAT32_ERROR_READING_SECTOR,
		FAT32_ERROR_SEEKING_SECTOR,
		FAT32_INVALID_FSINFO_SIG,
		FAT32_INVALID_BOOT_SIG,
		FAT32_INVALID_SYSIDSTR,
		FAT32_INVALID_EBR_SIG,
		FAT32_VIOLATION_RSV_BYTES
};
uint8_t fatError = FAT32_SUCCESS;

char szFatErrors[32][128]  = {
		"No errors reported so far.",
		"Error during reading a sector",
		"Error seeking a sector.",
		"There was an invalid signature for filesystem info structure.",
		"There was an invalid boot signature found.",
		"Invalid system identifier string found in extended boot record.",
		"Invalid signature found in extended boot record.",
		"The reserved bytes were violated by containing nonzero value."
};

#include "fat_c.c"

