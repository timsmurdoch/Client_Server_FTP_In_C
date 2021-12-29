#include <stdlib.h>
#include "string.h"

int tokenise(char *inputline, char *token[]){
	int count = 0;
	char *delims = " \n\t";
	char *buf;
	
	buf = strtok(inputline, delims);
	
	while(1){
		
		if(buf == NULL){
			break;
		}
		
		token[count] = buf;
		count++;
		
		buf = strtok(NULL, delims);
	}
	
	return count;
}
