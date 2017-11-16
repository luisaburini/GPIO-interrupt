#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define TIMEOUT 5

int main(void){
	int fd, retval;
	struct pollfd poll_stdin;
	char buff;
	
	/* file descriptor for stdin */
	fd = STDIN_FILENO;

	poll_stdin.fd = fd;	
	poll_stdin.events = POLLIN;   // There is data to read 

	retval = poll(&poll_stdin, 1, TIMEOUT*1000);  // 5 second timeout

	if(!retval)
		printf("No data within five seconds\n");
	if(retval == -1)
		perror("poll");
	if(poll_stdin.revents & POLLIN)
		printf("Data ready!");

}
