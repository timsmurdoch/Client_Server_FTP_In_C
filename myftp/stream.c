/* Timothy Smith & Mitchell Blackney
 *  stream.c
 *  Used to write a stream of bytes to sockets
 *	
 */

#include  <sys/types.h>
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
#include <unistd.h>
#include <stdio.h>
#include  "stream.h"

// Read op code(single char)
int write_op(int fd, char op)
{
	if(write(fd, (char*)& op, 1)!= 1)
		return(-1);
	return(1);
}

// Read op code(single char)
int read_op(int fd, char *op)
{
	char myCode;

	if(read(fd,(char *) &myCode,1) != 1 )
		return -1;
	*op = myCode;

	return 1;
}
// Taken from chapter 8 stream.c
int readn(int fd, char *buf, int bufsize)
{
    short data_size;    /* sizeof (short) must be 2 */ 
    int n, nr=1;

	for(n = 0; (n < bufsize) && (nr > 0); n += nr){
		if((nr = read(fd, buf+n, bufsize-n))< 0)
		{
			return(nr);
		}
	}
	return(n);
}

// Taken from chapter 8 stream.c
int writen(int fd, char *buf, int nbytes)
{
	int nw=0, n=0;
	for(n = 0; n < nbytes; n+=nw)
	{
		if((nw = write(fd, buf+n, nbytes-n)) <= 0)
		{
			return(nw);
		}
	}
	return (n);
}

// Reads on 4 bytes from fd
int read_length_four(int fd, int *n)
{
	int data; // Make n four bytes
	read(fd,&data,4);
	
	int dataFour = ntohs(data);
	*n = dataFour;
	
	return 1;
}

// Writes only 4 bytes to fd
int write_length_four(int fd, int n)
{
	int data = n;
	data = htons(data);
	
	write(fd, &data, 4);
	return 1;	
}

//Taken from chapter 8. Reads two bytes with write
int read_length_twos(int fd, int *n)
{
	short data; //Short is only two bytes
	if(read(fd, &data, 2) != 2){
		return (-1);
	} // Read the data
	
	short dataTwo = ntohs(data); // Convert to two bytes and byte order
	int dataCast = (int)dataTwo; // Back to int
	*n = dataCast; // Passed in n gets given dataCast value
	
	return 1;
	
}

//Taken from chapter 8 Writes two bytes to fd with write
int write_length_twos(int fd, int n)
{
	short data = n; //Make n two bytes
	data = htons(data); // network byte order
	
	if(write(fd, &data, 2) != 2)
		return(-1);
	return 1;
}
