# makefile for teststack
myftp: cli.o token.o stream.o 
	gcc cli.o token.o stream.o -o myftp

cli.o: cli.c token.h stream.h
	gcc -c cli.c

token.o: token.c token.h
	gcc -c token.c

stream.o: stream.c stream.h
	gcc -c stream.c

clean: 
	rm *.o
