#ifndef SRC_ISO_PRINTF_C_
#define SRC_ISO_PRINTF_C_
#include "iso_base.h"

void iso_print_info_pt(const struct entryPathTable* entry)
{

	printf(	"Sector: \t %i \n"
			"Identyficator: %.32s\n"
			"Length of id: %i\n"
			"Length of ext. attr: %i\n"
			"Number of directory: %i\n",
			entry->iExtentLBA,
			entry->szDirIdentifier,
			entry->bLengthDIRID,
			entry->bExtAttrRecLength,
			entry->sDirNumber);
}
#endif
