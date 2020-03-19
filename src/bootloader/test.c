
void dump_msg(char* msg, int size)
{
	char* video = (char*)0xb800;
	int i;	
	for(i = 0; i < size; i++)
	video[0] = msg[0];
}
void print_msg()
{	
	dump_msg("hello", 5);
}

