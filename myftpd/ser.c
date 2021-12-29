/* Timothy Smith & Mitchell Blackney
 *  ser.c  - 
 *	Used for connecting creating a server which can be used for basic FTP
 *	Commands include CD, DIR, PWD, PUT, GET
 *		
 */

#include  <stdlib.h>     /* strlen(), strcmp() etc */
#include  <stdio.h>      /* printf()  */
#include  <string.h>     /* strlen(), strcmp() etc */
#include  <errno.h>      /* extern int errno, EINTR, perror() */
#include  <signal.h>     /* SIGCHLD, sigaction() */
#include  <syslog.h>
#include  <sys/stat.h>
#include  <sys/types.h>  /* pid_t, u_long, u_short */
#include  <sys/socket.h> /* struct sockaddr, socket(), etc */
#include  <sys/wait.h>   /* waitpid(), WNOHAND */
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
                         /* and INADDR_ANY */
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include  "stream.h"     /* MAX_BLOCK_SIZE, readn(), writen() */
#include "token.h"


#define   SERV_TCP_PORT   40007   /* default server listening port */
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

// Checks if dir exists
char* checkDir(){
	char buf[1024];
	return getcwd(buf, sizeof(buf));
}

// Change the dir
int changeDir(char* dir){
	return chdir(dir);
}

// Formats a dir to look better, removes new lines
void formatDir(char* dir){
	char *pos = strchr(dir, '\n');
	char replace = ' ';
	while (pos){
		*pos = replace;
		pos = strchr(pos,'\n');
	}
}

char* dir(){
	int p[2];
	pid_t pid;
	char buf[100];
	char *o = malloc(1000);

	if (pipe(p)==-1){
		return '\0';
	}

	if ((pid = fork()) == -1){
		return '\0';
	}

	if(pid == 0) {
		dup2 (p[1], STDOUT_FILENO);
		close(p[0]);
		close(p[1]);
		execl("/bin/ls", "ls", "-1", (char *)0);

	} else {
		close(p[1]);
		int n = 0;
		
		 while(0 != (n = read(p[0], buf, sizeof(buf)))){
		 	o = strcat(o, buf);
		}
		wait(NULL);
	}
	return o;
}

// Checks if file name exists 1 for exists 0 for does not exist
int checkFile(char* fname){
	if( access( fname, F_OK ) != -1 ) {
    		return 1; // file exists
	} else {
    		return 0; // file doesn't exist
	}
}

// Check size of a file
long checkFileSize(char* fname){
	struct stat f;
	stat(fname, &f);
	return f.st_size;
}

// Taken from topic 8 ser6.c
void claim_children()
{
     pid_t pid=1;
     
     while (pid>0) { /* claim as many zombies as we can */
         pid = waitpid(0, (int *)0, WNOHANG); 
     } 
}

// Taken from topic 8 ser6.c
void daemon_init(void)
{       
     pid_t   pid;
     struct sigaction act;

     if ( (pid = fork()) < 0) {
          perror("fork"); exit(1); 
     } else if (pid > 0) {
          printf("Hay, you'd better remember my PID: %d\n", pid);
          exit(0);                  /* parent goes bye-bye */
     }

     /* child continues */
     setsid();                      /* become session leader */
     //chdir("/");                    /* change working directory */
     umask(0);                      /* clear file mode creation mask */

     /* catch SIGCHLD to remove zombies from system */
     act.sa_handler = claim_children; /* use reliable signal */
     sigemptyset(&act.sa_mask);       /* not to block other signals */
     act.sa_flags   = SA_NOCLDSTOP;   /* not catch stopped children */
     sigaction(SIGCHLD,(struct sigaction *)&act,(struct sigaction *)0);
     /* note: a less than perfect method is to use 
              signal(SIGCHLD, claim_children);
     */
}

//Process the CD command, changed the directory
void process_cd_command(int sd)
{
	int nr;
	int nw;
	int cdLength;
	//char buf[MAX_BLOCK_SIZE];
	printf(" CD COMMAND INITIATED\n");

		
	read_length_twos(sd, &cdLength);

	char buf[cdLength+1];
	printf("CD LENGth IS: %d\n", cdLength);
	if((nr = readn(sd, buf, cdLength) <= 0)){
		printf("buf size if %ld buf is %s\n",strlen(buf), buf);
		printf("Error reading from client\n");
		return;
	}

	buf[cdLength] = '\0';


	//printf("DEBUG: DIR TO GO IS %s buf size recieved is %d\n", buf, nr);

	if(changeDir(buf) == 0){
		char Success[] = "Changed\n";

		printf("Changed dir\n");
		if(write_op(sd, CD_SUCCESS) == -1){
			printf("error sending to client\n");
			return;
		}
		
	}else{
		char Failed[] = "Failed\n";
		printf("Failed\n");
		if(write_op(sd, CD_FAIL) == -1){
			printf("error sending to client\n");
			return;
	     	}
	}
	return;

}

void process_dir_command(int sd)
{
	int nr;
	int nw;
	char buf[MAX_BLOCK_SIZE];
	char *output = malloc(1000);

	output = dir();

	if(output != NULL&& output[0] != '\0'){ //Check if the dir is not null
		formatDir(output);
		if(write_op(sd, DIR_SUCCESS) == -1){  // Send success code
			free(output);
			printf("error sending to client\n");
			return;
		}
		
	}else{ // If dir is null, the program failed
		if(write_op(sd, DIR_FAIL) == -1){
			free(output);
			printf("error sending to client\n");
			return;
	     	}
		free(output);
		return;
	}

	if(write_length_four(sd, strlen(output)) < 0){
		free(output);
		printf("server: error writing dir length\n");
		return;
	}

	if(writen(sd, output, strlen(output)) < 0){ // Write the output dir to client		
		printf("server: send error2\n");

		free(output);
		return;
	}else{
		printf("success\n");
		free(output);
		return;
	}
	free(output);
	return;


}

void process_pwd_command(int sd)
{
	int nr;
	int nw;
	char buf[MAX_BLOCK_SIZE];
	char *output = malloc(1000);

	output = checkDir();


	if(output != NULL){ //Check if the pwd is not null
		if(write_op(sd, PWD_SUCCESS) == -1){  // Send success code
			free(output);
			printf("error sending to client\n");
			return;
		}
		
	}
	else{ // If pwd is null, the program failed
		if(write_op(sd, PWD_FAIL) == -1){
			free(output);
			printf("error sending to client\n");
			return;
	     	}
		free(output);
		return;
	}
	if(write_length_twos(sd, strlen(output)) < 0){
		printf("server: send length error\n");
		free(output);
		return;
	}
	if(writen(sd, output, strlen(output)) < 0){ // Write the output pwd to client		
		printf("server: send error2\n");
		free(output);
		return;
	}
	else{
		printf("SEND SIZE IS %ld\n", strlen(output));
		free(output);
		printf("success\n"); return;
	}
	free(output);
	return;
}

//Used for putting file into the server
void process_put_command(int sd)
{
	char ack, op;
	int fd, length, fileSize, nr = 0, nw = 0;
	FILE *myFile;

	if(read_length_twos(sd, &length) < 0){ // Read name length
		printf("server: cannot read name length\n"); return;
	}

	char fileName[length +1]; // For making \0

	if(readn(sd,fileName, length) < 0){ // Read file name
		printf("serverL cannot read filename\n");
	}else{
		fileName[length] = '\0';
	}

	if(checkFile(fileName) == 1){ //Check if the file exists
		ack = PUT_CLASH;
	}else if((myFile = fopen(fileName, "w")) == NULL) { // Open the file
		printf("server: error creating file\n");
		ack = PUT_FILENAME;
	}else{
		ack = PUT_READY;
	}
	
	if(write_op(sd, PUT) < 0){ // Send op comfirmation
		printf("server: error could not write op\n"); return;
	}

	if(write_op(sd, ack) < 0){ // Send ack of the open file errors or success
		printf("server: could not write ack\n");
	}else if(ack != PUT_READY){
		return;
	}

	if(read_op(sd, &op) < 0){ // Read op DATA
		printf("server: could not read op code\n"); return;

	}

	if(op != DATA){
		printf("server: op code not recognised\n"); return;
	}

	if(read_length_four(sd, &fileSize) < 0 ){
		printf("server: could not get file size\n"); return;
	}

	if(read_op(sd, &op) < 0){ // Read op DATA
		printf("server: could not read op code\n"); return;

	}

	char fBuf; //Buf for data from/to file
	while(op == PUT_CONTINUE){ // Loops through the file
		read(sd, &fBuf, sizeof(fBuf));
		fprintf(myFile, "%c", fBuf);
		if(read_op(sd, &op) < 0){ // Read op DATA
			printf("server: could not read op code\n"); return;
		}
	
	}
	printf("DONE writing to server\n");
	fclose(myFile);

	

}

//Used for getting file from server
void process_get_command(int sd)
{
	int length, fileSize;
	FILE * myFile;	
	char sBuf, op, ack;


	if(read_length_twos(sd, &length) < 0){ // Read name length
		printf("server: cannot read name length\n"); return;
	}

	char fileName[length+1]; //For adding in \0

	if(readn(sd,fileName, length) < 0){ // Read file name
		printf("server: cannot read filename\n"); return;
	}

	fileName[length]='\0';
	
	if(write_op(sd, GET) < 0){ // Comfirm the Op was recieved
			printf("server: cannot send GET op\n"); return;
	}

	
	if(!checkFile(fileName)){ // Check for file existance
		printf("server: file not found\n");
		ack = GET_NOTFOUND;
		if(write_op(sd, GET_NOTFOUND) < 0){
			printf("server: cannot send find found op\n"); return;
		}
		

	}else if(checkFile(fileName) == 1){ // Check the file name
		printf("server: file found\n");
		ack = GET_FOUND;
		if(write_op(sd, GET_FOUND) < 0){ //File is found so send ack
			printf("server: cannot get GET_FOUND\n"); return;
		}
	}else{
		ack = GET_OTHER;
		if(write_op(sd, GET_OTHER) < 0){
			printf("server: cannot get GET_FOUND\n"); return;
		}
	}

	fileSize = checkFileSize(fileName);

	if((myFile= fopen(fileName, "r")) < 0){ //Open the file as read
		printf("client: cannot open file to put %s\n",fileName); return;
	}

	//int leaveloop = 0;
	if(ack == GET_FOUND){

		if(write_length_four(sd, fileSize) < 0){ // Send file size
			printf("client: cannot send filesize\n"); return;
		}
		
		while(!feof(myFile)){ //Read until eof
			if(write_op(sd, PUT_CONTINUE) < 0){ // Send op CONTINUE
				printf("client: data pre send failed\n"); return;
			}	
		
			sBuf = fgetc(myFile);
			if(feof(myFile)){	
				sBuf = '\0';
			}
			
			write(sd, &sBuf, sizeof(sBuf));
			
		}
		if(write_op(sd, PUT_STOP) < 0){
			printf("server: cannot send GET op\n"); return;
		}

	}
	printf("server: done get file\n");
	fclose(myFile);
	
}

void serve_a_client(int sd)
{   
	FILE *fptr;
	int nr, nw;
	char buf[MAX_BLOCK_SIZE];
	char *token[Max_Number_Tokens];
	char opcode;
	fptr = fopen("logfile.txt","w");
	 if(fptr == NULL){
		printf("ERROR\n");
	}else{

	}

	while (1){ //While loop for getting commands from client
		 /* read data from client */
		 if ((read_op(sd, &opcode)) == -1) 
		     return;   /* connection broken down */

		 printf("OPCODE IS %c", opcode);

		//Basic if else for getting commands
		 if(opcode == CD){
			process_cd_command(sd);

		}else if(opcode == PWD){
			process_pwd_command(sd);

		}
		else if(opcode == DIR){
			process_dir_command(sd);

		}
		else if(opcode == PUT){
			process_put_command(sd);

		}else if(opcode == GET){
			process_get_command(sd);

		}		
		else {
			printf("Unknown code\n");
			printf("Commands are CD \nPWD \nPUT\nGET\n");
			exit(1);
		}
	}   
}

// Taken from topic 8 ser6.c, with slight modifications
int main(int argc, char *argv[])
{
     int sd, nsd, n;  
     pid_t pid;
     unsigned short port;   // server listening port
     socklen_t cli_addrlen;  
     struct sockaddr_in ser_addr, cli_addr; 
     char *output = malloc(1000);

	//For testing dir
	output = checkDir();
	printf("PWD IS: %s\n", output);

     /* get the port number */
     if (argc == 1) {
          port = SERV_TCP_PORT;
     } else if (argc == 2) {
          int n = atoi(argv[1]);   
          if (n >= 1024 && n < 65536) 
              port = n;
          else {
              printf("Error: port number must be between 1024 and 65535\n");
              exit(1);
          }
     } else {
          printf("Usage: %s [ server listening port ]\n", argv[0]);     
          exit(1);
     }

     /* turn the program into a daemon */
     daemon_init(); 

     /* set up listening socket sd */
     if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
           perror("server:socket"); exit(1);
     } 

     /* build server Internet socket address */
     bzero((char *)&ser_addr, sizeof(ser_addr));
     ser_addr.sin_family = AF_INET;
     ser_addr.sin_port = htons(port);
     ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
     /* note: accept client request sent to any one of the
        network interface(s) on this host. 
     */

     /* bind server address to socket sd */
     if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0){
           perror("server bind"); exit(1);
     }

     /* become a listening socket */
     listen(sd, 5);

     while (1) {

          /* wait to accept a client request for connection */
          cli_addrlen = sizeof(cli_addr);
          nsd = accept(sd, (struct sockaddr *) &cli_addr, &cli_addrlen);
          if (nsd < 0) {
               if (errno == EINTR)   /* if interrupted by SIGCHLD */
                    continue;
               perror("server:accept"); exit(1);
          }

          /* create a child process to handle this client */
          if ((pid=fork()) <0) {
              perror("fork"); exit(1);
          } else if (pid > 0) { 
              close(nsd);
              continue; /* parent to wait for next client */
          }

          /* now in child, serve the current client */
          close(sd); 
          serve_a_client(nsd);
          exit(0);
     }
}
