/*
 * fat_c.c
 *
 *  Created on: 7 mar 2020
 *      Author: foralost
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int __mbr_init_from_dev(int fd, struct MBR* data)
{

	char* bData = malloc(512);
	if ( read(fd, bData, 512) <= 0 ){
		perror("read");
		return -1;
	}

	if( (uint8_t) bData[511] != 0xAA || (uint8_t) bData[510] != 0x55)
	{
		free(bData);
		return -1;
	}
	if( bData[444] || bData[445] )
	{
		if(bData[444] != 0x5A || bData[445] != 0x5A){
			free(bData);
			return -1;
		}
	}
	memset(data, 0, sizeof(*data));
	memcpy(data->bBootStrap, bData, 440);
	for(size_t i = 0 ; i < 4 ; i++){
		data->partitions[i].bDriverAttr = bData[i*16 +  0x1be];
		memcpy(data->partitions[i].StartCHS, bData + i * 16 +  0x1be + 1, 3);
		data->partitions[i].bType= bData[i * 0x1be + 4];
		memcpy(data->partitions[i].EndCHS, bData + i * 16 + 0x1be + 5 , 3);
		data->partitions[i].StartLBA = *( (uint32_t*)(bData + i * 16 + 0x1be + 8) );
		data->partitions[i].bNumbSect = *((uint32_t*) (bData + i * 16 + 0x1be + 12) );
	}

	return 0;

}


void __mbr_init(struct MBR* data)
{
	memset(data, 0, sizeof(*data));
	data->bSignature[0] = 0x55;
	data->bSignature[1] = 0xAA;
}

void __mbr_act_toggle_part(struct partitionEntry* entry)
{
	entry->bDriverAttr = entry->bDriverAttr^( 1 << 7);
}

int __mbr_load_bootstrap(const char* szPath, struct MBR* data)
{
	FILE* f = fopen(szPath, "rb");
	if(!f){
		perror("bs fopen");
		return -1;
	}

	char bData[440];
	if( fseek(f, 0, SEEK_SET) )
	{
		perror("bs fseek");
		return -1;
	}

	if( fread(bData, 440, 1, f) != 1)
	{
		perror("bs fread");
		return -1;
	}

	memcpy(data->bBootStrap, bData, sizeof(bData));
	return 0;
}
