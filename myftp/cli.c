/* Timothy Smith & Mitchell Blackney
 *
 * cli6.c - Base file from topic 8 examples
 * 	Used for sending the commands to the server
 *      and processing them, this is a simple FTPclient
 */

#include  <stdlib.h>
#include  <unistd.h>
#include  <stdio.h>
#include  <sys/types.h>        
#include  <sys/socket.h>
#include  <netinet/in.h>       /* struct sockaddr_in, htons, htonl */
#include  <netdb.h>            /* struct hostent, gethostbyname() */ 
#include  <string.h>
#include  <fcntl.h>
#include  "stream.h"           /* MAX_BLOCK_SIZE, readn(), writen() */
#include  "token.h"
#include  <sys/wait.h>
#include  <sys/stat.h>

#define   SERV_TCP_PORT  40007 /* default server listening port */
#define Max_Number_Tokens 100

#define CD 'C'
#define DIR 'D'
#define PWD 'W'
#define PUT 'P'
#define GET 'G'
#define DATA 'B'

// CD acks
#define CD_SUCCESS 1
#define CD_FAIL 0

// PWD acks
#define PWD_SUCCESS 1
#define PWD_FAIL 0

// DIR acks
#define DIR_SUCCESS 1
#define DIR_FAIL 0

// GET acks
#define GET_NOTFOUND 2
#define GET_FOUND 1
#define GET_OTHER 0

// PUT acks
#define PUT_OTHER 3	// Other reasons
#define PUT_FILENAME 2 //Invalid file name
#define PUT_CLASH 1	// Clash with other file
#define PUT_READY 0	// Ready
#define PUT_CONTINUE 4 //Continue reading file
#define PUT_STOP 5 //Stop reading file



char* checkDir(){
	char buf[1024];
	return getcwd(buf, sizeof(buf));
}

int changeDir(char* dir){
	return chdir(dir);
}

int dir(){
	pid_t pid;
	pid= fork();
	
	if(pid == 0){
		execl("/bin/ls", "/bin/ls", NULL);
		exit(0);
	} else if (pid > 0){ 
		wait(NULL);
	}		
	
	return 0;
}

int checkFile(char* fname){
	if( access( fname, F_OK ) != -1 ) {
    		return 1; // file exists
	} else {
    		return 0; // file doesn't exist
	}
}

long checkFileSize(char* fname){
	struct stat f;
	stat(fname, &f);
	return f.st_size;
}

void serCDCommand(int sd, char*token)
{
	int nw;
	char ack;
	char buf[MAX_BLOCK_SIZE];

	strcpy(buf,token);

	if(write_op(sd, CD) == -1)
	{ //Send the opcode to ser
		printf("client: send error1 \n"); exit(1); 
	}

	write_length_twos(sd, strlen(buf));

	if((writen(sd, buf, strlen(buf)) < 0)){ //Send path to server
		printf("client: send error 2\n"); return;
	}
	
	if (read_op(sd, &ack) == -1) //Check for ack success or fail
	{
	    printf("client: receive error\n"); return;
	}

	if(ack == CD_SUCCESS)
	{
		printf("$ CD success\n");
		
		return;
	}else
	{
		printf("$ CD fail\n");
		return;
	}
	

}

// Handles the pwd command between client and server

void serPWDCommand(int sd)
{
	char ack;
	int nr;
	int pwdLength;
	char buf[MAX_BLOCK_SIZE];
	buf[0] = '\0';

	if(write_op(sd, PWD) == -1) //Send the opcode to ser
	{ 
		printf("client: send error1 \n"); return; 
	}
	
	if (read_op(sd, &ack) == -1) //Check for ack success or fail
	{
	    printf("client: receive error\n"); return;
	}

	if(ack == PWD_SUCCESS) //Successfully gotten the pwd command
	{
		if(read_length_twos(sd, &pwdLength) < 0 ){ // Read pwd length two byte format
			printf("client: file size not found\n");
			return;
		}
		char pwdString[pwdLength+1]; //+1 for \0

		if((readn(sd, pwdString, pwdLength)) < 0){ // Read PWD output from server
			printf("client: client read error\n"); return;

		}
		else{
			pwdString[pwdLength] = '\0';
			printf("$ %s\n", pwdString); return;

		}
		
		return;
	}
	else{
		printf("$ PWD failed\n");
		return;
	}
	return;

}


void serDIRCommand(int sd)
{
	char ack;
	int nr;
	int dirLength;


	if(write_op(sd, DIR) == -1) //Send the opcode to ser
	{ 
		printf("client: send error1 \n"); return; 
	}
	
	if (read_op(sd, &ack) == -1) //Check for ack success or fail
	{
	    printf("client: receive error\n"); return;
	}

	if(ack == DIR_SUCCESS) //Successfully gotten the dir command
	{
		if(read_length_four(sd, &dirLength) < 0){
			printf("client: error getting DIR length\n");
		}

		char buf[dirLength+1];

		if((readn(sd, buf, dirLength)) < 0){ // Read DIR output from ser
			printf("client: client read error\n");
			return;

		}
		else{
			buf[dirLength] = '\0'; //Make sure there is end of file char
			printf("$ %s\n", buf); return;

		}
		
		return;
	}
	else{
		printf("$ DIR failed\n");
		return;
	}
	return;

}

void serPUTCommand(int sd, char *fileName)
{
	long fileSize;
	char buf[512];
	int nr = 0, fd;
	FILE *myFile;
	char ack, op, sBuf;

	// Check if file exists
	if(!checkFile(fileName)){
		printf("File does not exist\n"); return;

	}

	if((myFile= fopen(fileName, "r")) < 0){ //Open the file as read
		printf("client: cannot open file to put %s\n",fileName); return;
	}

	// Send op code PUT
	if(write_op(sd, PUT) < 0){
		printf("client: op failed\n"); return;

	}
	
	// Get the size of file
	fileSize = checkFileSize(fileName);
	if(write_length_twos(sd, strlen(fileName)) < 0){ // Write fileNameSize
		printf("client: cannot send length\n"); return;
	}
	
	if(writen(sd, fileName, strlen(fileName)) <= 0 ){ // Write fileName
		printf("client: cannot send find name\n"); return;
	}
	
	if(read_op(sd, &op) <0){ // Read op code
		printf("client: no op code\n"); return;
	}else if(op != PUT){
		printf("client: put not passed back\n");
	}
	if(read_op(sd, &ack) < 0){ // Read ack code
		printf("client: no ack code\n"); return;
	}else{
		if(ack == PUT_CLASH){
			printf("client: error with file names clashing\n");return;
		}
		if(ack == PUT_FILENAME){
			printf("client: cannot create file name\n"); return;
		}
		if(ack == PUT_OTHER){
			printf("client: cannot PUT due to other reasons\n");return;
		}
		if(ack != PUT_READY){
			printf("client: unknown error\n"); return;
		}
		
		if(write_op(sd, DATA) < 0){ // Send op DATA
			printf("client: data pre send failed\n"); return;
		}

		if(write_length_four(sd, fileSize) < 0){ // Send file size
			printf("client: cannot send filesize\n"); return;
		}
		
		//int leaveloop = 0;
		while(!feof(myFile)){ //Read until eof
			if(write_op(sd, PUT_CONTINUE) < 0){ // Send op DATA
				printf("client: data pre send failed\n"); return;
			}	
		
			sBuf = fgetc(myFile);
		
			if(feof(myFile)){
				sBuf = '\0'; //
			}
			//if(feof(myFile)){
			write(sd, &sBuf, sizeof(sBuf));
			
		}
		
		if(write_op(sd, PUT_STOP) < 0){ // Send op STOP
			printf("client: data pre send failed\n"); return;
		}
		
	}
	fclose(myFile);
	printf("Finished sending\n");

}

void serGETCommand(int sd, char*fileName)
{
	int fileSize;
	char sBuf, ack, op;
	FILE *myFile;
	
	if(checkFile(fileName) == 1){
		printf("File already exists\n"); return;
	}


	if(write_op(sd, GET) < 0 ){ // Send get op
		printf("client: send GET op failed\n"); return;
	}
	if(write_length_twos(sd, strlen(fileName)) < 0){ // Write fileNameSize
		printf("client: cannot send length\n"); return;
	}
	if(writen(sd, fileName, strlen(fileName)) <= 0 ){ // Write fileName
		printf("client: cannot send find name\n"); return;
	}

	if(read_op(sd, &op) < 0){ // Read ack code
		printf("client: no ack code\n"); return;
	}

	if(op != GET){ 
		printf("client: op no found\n"); return;
	}

	if(read_op(sd, &ack) < 0){ // Read ack code
		printf("client: no ack code\n"); return;
	}
	

	if(ack == GET_OTHER){ // Check for ack codes and errors
		printf("client: cannot get file dur to other reasons\n"); return;
	}else if(ack == GET_NOTFOUND){
		printf("client: file not found\n");return;
	}else if(ack == GET_FOUND){
		printf("File found!\n");
	}else{
		printf("Unknown error occured, no ack\n"); return;
	}

	if((myFile = fopen(fileName, "w")) == NULL) { //Open the file
		printf("client: error creating file\n"); return;
	}

	if(ack == GET_FOUND){ //File exists so begin with getting it

		if(read_length_four(sd, &fileSize) < 0 ){ //Read file size
			printf("server: could not get file size\n"); return;
		}
		if(read_op(sd, &op) < 0){ // Read op for continue
			printf("server: could not read op code\n"); return;
		}
		char fBuf; //Buf for holding data
		while(op == PUT_CONTINUE){ //Loop until PUT STOP is given from server
			read(sd, &fBuf, sizeof(fBuf));
			fprintf(myFile, "%c", fBuf);
			if(read_op(sd, &op) < 0){ // Read op CONTINUE or STOP
				printf("could not read op code\n"); return;
			}
	
		}
	
	}
	printf("DONE reading from serv\n");
	fclose(myFile);	
	
}

// Modified main taken from topic 8
int main(int argc, char *argv[])
{
     int sd, n, nr, nw, i=0;
     char buf[MAX_BLOCK_SIZE], host[60];
     unsigned short port;
     char *token[Max_Number_Tokens];
     int t;
     struct sockaddr_in ser_addr; struct hostent *hp;

     /* get server host name and port number */
     if (argc==1) {  /* assume server running on the local host and on default port */
          gethostname(host, sizeof(host));
          port = SERV_TCP_PORT;
     } else if (argc == 2) { /* use the given host name */ 
          strcpy(host, argv[1]);
          port = SERV_TCP_PORT;
     } else if (argc == 3) { // use given host and port for server
         strcpy(host, argv[1]);
          int n = atoi(argv[2]);
          if (n >= 1024 && n < 65536) 
              port = n;
          else {
              printf("Error: server port number must be between 1024 and 65535\n");
              exit(1);
          }
     } else { 
         printf("Usage: %s [ <server host name> [ <server listening port> ] ]\n", argv[0]); 
         exit(1); 
     }

    /* get host address, & build a server socket address */
     bzero((char *) &ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(port);
     if ((hp = gethostbyname(host)) == NULL){
           printf("host %s not found\n", host); exit(1);   
     }
     ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

     /* create TCP socket & connect socket to server address */
     sd = socket(PF_INET, SOCK_STREAM, 0);
     if (connect(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0) { 
          perror("client connect"); exit(1);
     }
    
     while (++i) {
          printf("$");
          fgets(buf, sizeof(buf), stdin); nr = strlen(buf); 
          if (buf[nr-1] == '\n') { buf[nr-1] = '\0'; --nr; }

          if (strcmp(buf, "quit")==0) {
               printf("Bye from client\n"); exit(0);
          }

	  t = tokenise(buf, token);


          if (nr > 0) {
		if(strcmp(token[0], "lcd") == 0 && t >=2){ //LCD command
     			if(changeDir(token[1]) == -1){
     				printf("Failed to change Directory\n");
     			}

		}else if(strcmp(token[0], "ldir") == 0){ //LDIR command
     			dir();

     		}else if(strcmp(token[0], "lpwd") == 0){ //CHECKDIR command
     			printf("%s\n", checkDir());

     		}else if(strcmp(token[0], "cd") == 0 && t == 2){ //CD command
			serCDCommand(sd, token[1]);

     		}else if(strcmp(token[0], "pwd") == 0){
			serPWDCommand(sd);

		}else if(strcmp(token[0], "dir") == 0){
			serDIRCommand(sd);
		}
		else if(strcmp(token[0], "get") == 0){
			if(token[1] != NULL)
				serGETCommand(sd, token[1]);
		}
		else if(strcmp(token[0], "put") == 0){
			if(token[1] != NULL)
			serPUTCommand(sd, token[1]);
		}else{
			printf("Unknown command\n");
		}
          }
     }
}
