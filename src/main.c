#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "include/isobase/iso_func.h"

int main(int argc, char **argv) {

	struct ISOFile *test =
			iso_open();
	struct ISOEntryFile testfile;
	struct entryPathTableNode *start;

	struct ISOEntryFile *file;

	if (__iso_read_directory_rd(test, &file, "/AUTORUN.INF") < 0) {
		iso_print_error("reading file");
		return -1;
	}

	printf("%s", file->bData);
	return 0;
}
