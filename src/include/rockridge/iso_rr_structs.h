#ifndef  SRC_ISO_RR_STRUCTS_H_
#define  SRC_ISO_RR_STRUCTS_H_
#include <stdint.h>
struct rrBase {
	char szSignature[3];
	uint8_t bLength;
	uint8_t bSystemEntry;
};
struct rrPosixAttributes {
	struct rrBase strBaseData;
	uint32_t iPosixFileModeLSB;
	uint32_t iPosixFileModeMSB;
	uint32_t iPosixFileLinksLSB;
	uint32_t iPosixFileLinksMSB;
	uint32_t iPosixFileUserIDLSB;
	uint32_t iPosixFileUserIDMSB;
	uint32_t iPosixFileGroupLSB;
	uint32_t iPosixFileGroupMSB;
	uint32_t iPosixFileSNLSB;
	uint32_t iPosixFileSNMSB;
};

struct rrPosixDevNumber {
	struct rrBase strBaseData;
	uint32_t iDEVTHighLSB;
	uint32_t iDEVTHighMSB;
	uint32_t iDEVTLowLSB;
	uint32_t iDEVTLowMSB;
};

struct _rrSLComponent {
	char bFlags;
	uint8_t bLength;
	char *bContent;
};
struct _rrEntries {
	void* data;
	void* next;
};
struct rrSymbolicLink {
	struct rrBase strBaseData;
	char bFlags;
	struct __rrSLComponent *strData;
};

struct rrPosixAlterName {
	struct rrBase strBaseData;
	char bFlags;
	char *szNameContent;
};

struct rrChildLink {
	struct rrBase strBaseData;
	uint32_t iLocLBALSB;
	uint32_t iLocLBAMSB;
};

struct rrParentLink {
	struct rrBase strBaseData;
	uint32_t iLocLBALSB;
	uint32_t iLocLBAMSB;
};

struct rrRelocatedDir {
	struct rrBase strBaseData;
};

struct rrTimeStamp {
	struct rrBase strBaseData;
	char bFlags;
	char szCreation[17];
	char szModifDate[17];
	char szExpirateDate[17];
	char szEffectiveDate[17];
};

struct rrFileDataSparse {
	struct rrBase strBaseData;
	uint32_t iVTSizeHighLSB;
	uint32_t iVTSizeHighMSB;
	uint32_t iVTSizeLowLSB;
	uint32_t iVTSizeLowMSB;
	char bTableDepth;
};
#endif
