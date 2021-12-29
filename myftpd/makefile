# makefile for teststack

myftpd: ser.o token.o stream.o 
	gcc ser.o token.o stream.o -o myftpd

ser.o: ser.c token.h stream.h
	gcc -c ser.c

token.o: token.c token.h
	gcc -c token.c

stream.o: stream.c stream.h
	gcc -c stream.c

clean: 
	rm *.o
