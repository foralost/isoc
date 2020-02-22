#include <stdint.h>

struct rrPosixAttributes {
	char szSignature[2]; // 'PW'
	uint8_t bLength;
	uint8_t bSystemEntryV;
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

struct rrPosixDevNumber{
	char szSignature[2]; // 'PN'
	uint8_t	bLength; // Should be 20
	uint8_t bSystemEntryV;
	uint32_t iDEVTHighLSB;
	uint32_t iDEVTHighMSB;
	uint32_t iDEVTLowLSB;
	uint32_t iDEVTLowMSB;
};

struct _rrSLComponent{
	char bFlags;
	uint8_t bLength;
	char* bContent;
};

struct rrSymbolicLink {
	char szSignature[2];
	uint8_t bLength;
	uint8_t bSystemEntryV;
	char bFlags;
	struct __rrSLComponent* strData;
};


struct rrPosixAlterName {

};

struct rrChildLink {

};

struct rrParentLink {

};

struct rrRelocatedDir{

};

struct rrTimeStamp {

};

struct rrFileDataSparse{

};
