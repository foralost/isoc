#ifndef SRC_ISO_STRUCT_H
#define SRC_ISO_STRUCT_H 

#include <stdint.h>

enum VDESCR_TYPECODES{
	BOOT, PRIMARY, SUPPLEMENTARY,
	PARTITION, VDESCR_TERM = 255
};

struct ISOEntryFile {
	char* bData;
	uint32_t iLength;
};


struct entryPathTable{
	char bLengthDIRID;
	char bExtAttrRecLength;
	int32_t iExtentLBA;
	int16_t sDirNumber;
	char* szDirIdentifier;
};


struct entryPathTableNode{
	struct entryPathTableNode* prev;
	struct entryPathTable data;
	struct entryPathTableNode* next;
};


struct directoryDescriptor{
	char bLengthDescriptor;
	char bEXTRecordLength;
	int32_t iLocLSBEXT;
	int32_t iLocMSBEXT;
	int32_t iDataLengthLSB;
	int32_t iDataLengthMSB;
	char szRecDate[7];
	char bFlags;
	char bFSize;
	char bGapSize;
	int16_t sVolSeqLSB;
	int16_t sVolSeqMSB;
	char bLengthID;
	char* szDirIdentifier;
};

struct directoryDescriptorNode {
	struct directoryDescriptor data;
	struct directoryDescriptorNode* next;
};
struct volumeDescriptor{
	int8_t iType;
	char szID[5];
	char szVersion;
	char szData[2041];
};

struct date_stamp {
	char szCreation[17];
	char szModifDate[17];
	char szExpirateDate[17];
	char szEffectiveDate[17];
};

struct primaryDescriptor {
	int8_t iType;
	char szID[5];
	char szVersion;
	char bZero;
	char szSystem[32];
	char szVolume[32];
	char bEightZeroes[8];
	int32_t iVolumeSSizeLSB;
	int32_t iVolumeSSizeMSB;
	char b32Zeroes[32];
	int16_t sVolumeSetSizeLSB;
	int16_t sVolumeSetSizeMSB;
	int16_t sVolumeSeqNumberLSB;
	int16_t sVolumeSeqNumberMSB;
	int16_t sLogicalBlockSizeLSB;
	int16_t sLogicalBlockSizeMSB;
	int32_t iPathTableSizeLSB;
	int32_t iPathTableSizeMSB;
	int32_t iLocTypeLPath;
	int32_t iLocOptLPath;
	int32_t iLocTypeMPath;
	int32_t iLocOptMPath;
	char szDirEntryRoot[34];
	char szVolumeSetID[128];
	char szPublisherID[128];
	char szDataPrepID[128];
	char szApplicationID[128];
	char szCopyrightFileID[38];
	char szAbstractFileID[36];
	char szBibliographicID[37];
	struct date_stamp dStamp;
	char bFileVers;
	char bNull;
	char free[512];
	char bResv[653];
};

struct ISOFile {
	FILE* fHandler;
	struct primaryDescriptor strPVD;
};

#endif 
