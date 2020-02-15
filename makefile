OBJFILES=iso_functions.o


all:	
	gcc -Wall -Wpedantic -Wextra -g -o bin/isoc src/main.c


