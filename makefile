OBJFILES=iso_functions.o


all:	
	gcc -Wall -Wpedantic -Wextra  -g -o bin/isoc -lm src/main.c


