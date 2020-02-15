#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "include/iso_func.h"

int main(int argc, char **argv) {

	struct ISOFile *test = iso_open(
			"/home/foralost/code/ccpp/isoc/src/iso_folder/output.iso");

	struct ISOEntryFile testfile;
	struct entryPathTableNode* start;

	if(__iso_read_path_table(test, &start) < 0)
		iso_print_error("");

	while(start->next)
		start = start->next;

	iso_print_info_pt(&start->data);
	struct directoryDescriptorNode *dirs;

	__iso_read_directory_pt(test->fHandler, &start->data, &dirs);

	iso_print_directory_tree(dirs);
	return 0;
}
