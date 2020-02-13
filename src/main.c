#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "include/iso_func.h"

int main(int argc, char** argv)
{
	struct ISOFile *file = iso_open("/home/foralost/code/ccpp/iso/src/test.iso");
	if(!file){
		iso_print_error("openISO");
		return -1;
	}

	struct entryPathTableNode* desc;
	if ( iso_read_path_table(file, &desc) == -1 )
		iso_print_error("iso_read_path_table");

	struct directoryDescriptorNode* desc_two;
	if ( iso_read_directories_pt(file->fHandler, &desc->data, &desc_two ) )
		iso_print_error("iso_read_dir");
	fflush(stdout);

	return 0;
}
