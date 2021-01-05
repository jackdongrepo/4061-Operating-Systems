/* CSci4061 F2018 Assignment 2
 * section: 1
 * date: 11/08/18
 * name: Jack Dong, Derek Frank, Pierre Abillama
 * id:   5245772, 5276563, 5290574 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"
#include "util.h"

void show_error_message (char* filename) {
	fprintf(stderr, "Usage: %s <user_id>: name (without spaces) of client is a required argument\n", filename);	
}


/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {

	int pipe_user_reading_from_server[2], pipe_user_writing_to_server[2];
	char buf[MAX_MSG];
	int nbytes;
	
	if (argc != 2) {
		show_error_message(argv[0]);
		return;
	}

	// You will need to get user name as a parameter, argv[1].
	// We implemented the server code with the naming reversed, so to stay consistent and minimize refactor errors, we reversed the arguments in this function call to match our logic
	if(connect_to_server("PIERRE_DEREK_JACK", argv[1], pipe_user_writing_to_server, pipe_user_reading_from_server) == -1) {
		exit(-1);
		close(pipe_user_reading_from_server[1]);
		close(pipe_user_reading_from_server[0]);
		close(pipe_user_writing_to_server[1]);
		close(pipe_user_writing_to_server[0]);
		return;
	}
	else {
		print_prompt(argv[1]);
		close(pipe_user_reading_from_server[1]);
		close(pipe_user_writing_to_server[0]);
		fcntl(pipe_user_reading_from_server[0], F_SETFL, fcntl(pipe_user_reading_from_server[0], F_GETFL) | O_NONBLOCK);
		fcntl(pipe_user_writing_to_server[1], F_SETFL, fcntl(pipe_user_writing_to_server[1], F_GETFL) | O_NONBLOCK);
		fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
		/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
		while (1) {
			if ((nbytes = read(0, buf, MAX_MSG)) != -1) {
				if (buf[strlen(buf) - 1] == '\n') {
		 			if (strlen(buf) == 1) {
						print_prompt(argv[1]);
						continue;
					}
					else			
						buf[strlen(buf) - 1] = '\0';
				}
				enum command_type type = get_command_type(buf);
                                switch (type) {
					case EXIT:
						write(pipe_user_writing_to_server[1], buf, nbytes - 1);
						close(pipe_user_reading_from_server[0]);
						close(pipe_user_writing_to_server[1]);
						return;
					case SEG:
						write(1, "I don't feel too good about this\n", strlen("I don't feel too good about this\n"));
						//Creating a horrible. horrible seg fault
						char *n = NULL;
						*n = 1;		
						break;
					default:
						write(pipe_user_writing_to_server[1], buf, nbytes - 1);
						print_prompt (argv[1]);
						memset(buf, '\0', MAX_MSG);
				}
			}
			nbytes = read(pipe_user_reading_from_server[0], buf, MAX_MSG);
			if (nbytes == 0) {
				write(1, "The server process seems dead\n", strlen("The server process seems dead\n"));
				close(pipe_user_reading_from_server[0]);
				close(pipe_user_writing_to_server[1]);
				return;
			}
			else if (nbytes > 0) {	
				write(1, "\n", 1);
				write(1, buf, nbytes);
				write(1, "\n", 1);
				print_prompt (argv[1]);
				memset(buf, '\0', MAX_MSG);
			}
			
		}
			usleep(10000);
	}
		
		// poll pipe retrieved and print it to sdiout
	
		// Poll stdin (input from the terminal) and send it to server (child process) via pipe
		
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/


