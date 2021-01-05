/* CSci4061 F2018 Assignment 2
 * section: 1
 * date: 11/08/18
 * name: Jack Dong, Derek Frank, Pierre Abillama
 * id:   5245772, 5276563, 5290574 */


#include <stdio.h> 
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "comm.h"
#include "util.h"

/* -----------Functions that implement server functionality -------------------------*/

/*
 * Returns the empty slot on success, or -1 on failure
 */
int find_empty_slot(USER * user_list) {
	// iterate through the user_list and check m_status to see if any slot is EMPTY
	// return the index of the empty slot
    int i = 0;
	for(i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_EMPTY) {
			return i;
		}
	}
	return -1;
}

/*
 * list the existing users on the server shell
 */
int list_users(int idx, USER * user_list)
{
	// iterate through the user list
	// if you find any slot which is not empty, print that m_user_id
	// if every slot is empty, print "<no users>""
	// If the function is called by the server (that is, idx is -1), then printf the list
	// If the function is called by the user, then send the list to the user using write() and passing m_fd_to_user
	// return 0 on success
	int i, flag = 0;
	char buf[MAX_MSG] = {}, *s = NULL;

	/* construct a list of user names */
	s = buf;
	strncpy(s, "---connected user list---\n", strlen("---connected user list---\n"));
	s += strlen("---connecetd user list---\n");
	for (i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		flag = 1;
		strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
		s = s + strlen(user_list[i].m_user_id);
		strncpy(s, "\n", 1);
		s++;
	}
	if (flag == 0) {
		strcpy(buf, "<no users>\n");
	} else {
		s--;
		strncpy(s, "\0", 1);
	}

	if(idx < 0) {
		printf("%s", buf);
		printf("\n");
	} else {
		/* write to the given pipe fd */
		if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
			perror("writing to server shell");
	}

	return 0;
}

/*
 * add a new user
 */
int add_user(int idx, USER * user_list, int pid, char * user_id, int pipe_to_child, int pipe_to_parent)
{
	// populate the user_list structure with the arguments passed to this function
	// return the index of user added
	struct _userInfo user;
	user.m_pid = pid;
	strcpy(user.m_user_id, user_id);
	user.m_fd_to_user = pipe_to_child;
	user.m_fd_to_server = pipe_to_parent;
	user.m_status = SLOT_FULL;
	user_list[idx] = user;
	return idx;
}

/*
 * Kill a user
 */
void kill_user(int idx, USER * user_list) {
	// kill a user (specified by idx) by using the systemcall kill()
	// then call waitpid on the user
	struct _userInfo user = user_list[idx];
	if (kill(user.m_pid, SIGKILL) == -1) {
		perror("Error killing user");
	}
	waitpid(user.m_pid, NULL, 0);
}

/*
 * Perform cleanup actions after the used has been killed
 */
void cleanup_user(int idx, USER * user_list)
{
	// m_pid should be set back to -1
	// m_user_id should be set to zero, using memset()
	// close all the fd
	// set the value of all fd back to -1
	// set the status back to emptiy
	user_list[idx].m_pid = -1;
	memset(user_list[idx].m_user_id, '\0', MAX_USER_ID);
	if (close(user_list[idx].m_fd_to_user) == -1) {
		perror("Error closing user pipe end");
		return;
	}
	user_list[idx].m_fd_to_user = -1;
	if (close(user_list[idx].m_fd_to_server) == -1) {
		perror("Error closing server pipe end");
		return;
	}
	user_list[idx].m_fd_to_server = -1;
	user_list[idx].m_status = SLOT_EMPTY;
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list) {
	// should kill_user()
	// then perform cleanup_user()
	kill_user(idx, user_list);
	cleanup_user(idx, user_list);
}

/*
 * broadcast message to all users
 */
int broadcast_msg(USER * user_list, char *buf, char *sender)
{
	//iterate over the user_list and if a slot is full, and the user is not the sender itself,
	//then send the message to that user
	//return zero on success
	for (int i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_FULL && user_list[i].m_user_id != sender) {
			if (write(user_list[i].m_fd_to_user, buf, strlen(buf)) == -1) {
				perror ("Failed to broadcast message");
				return -1;
			} 
		}
	}
	return 0;
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list)
{
	// go over the user list and check for any full slots
	// call cleanup user for each of those users.
	for (int i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_FULL) {
			kick_user(i, user_list);
		}
	}
}

/*
 * find user index for given user name
 */
int find_user_index(USER * user_list, char * user_id)
{
	// go over the  user list to return the index of the user which matches the argument user_id
	// return -1 if not found
	int i, user_idx = -1;

	if (user_id == NULL) {
		fprintf(stderr, "NULL name passed.\n");
		return user_idx;
	}
	for (i=0;i<MAX_USER;i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		if (strcmp(user_list[i].m_user_id, user_id) == 0) {
			return i;
		}
	}

	return -1;
}

/*
 * given a command's input buffer, extract name
 */
int extract_name(char * buf, char * user_name)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 2) {
        strcpy(user_name, tokens[1]);
        return 0;
    }

    return -1;
}

int extract_text(char *buf, char * text)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    char * s = NULL;
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 3) {
        //Find " "
        s = strchr(buf, ' ');
        s = strchr(s+1, ' ');

        strcpy(text, s+1);
        return 0;
    }

    return -1;
}


/*
 * send personal message
 */
int send_p2p_msg(int idx, USER * user_list, char *buf)
{
	// get the target user by name using extract_name() function
	// find the user id using find_user_index()
	// if user not found, write back to the original user "User not found", using the write()function on pipes. 
	// if the user is found then write the message that the user wants to send to that user.
	char receiver [MAX_USER_ID];
	char message [MAX_MSG];
	if (extract_name(buf, receiver) == -1) {
		fprintf(stderr, "Cannot relay \\p2p message, user name not given\n");
		if (write(user_list[idx].m_fd_to_user, "User name not given", strlen("User name not given")) == -1) {
			perror("Error writing back to user");
		}
		return -1;	
	}
	int index = find_user_index(user_list, receiver);
	
	//Check if user exists
	if (index == -1) {
		fprintf(stderr, "Cannot relay \\p2p message, user %s does not exist\n", receiver);
		if (write(user_list[idx].m_fd_to_user, "User not found, use \\list", strlen("User not found, user \\list")) == -1) {
			perror ("Error writing back to user");
		}
		return -1;
	}

	//Check if receiver and sender are different
	if (index == idx) {
		fprintf(stderr, "Cannot relay \\p2p message to the same user %s\n", receiver);
		if (write(user_list[idx].m_fd_to_user, "Cannot send \\p2p to yourself\n", strlen("Cannot send \\p2p to yourself")) == -1) {
			perror ("Error writing back to user");
		}
		return -1;
	}

	//Check if message is non empty
	if (extract_text(buf, message) == -1) {
		fprintf(stderr, "Cannot relay empty message\n");
		if (write(user_list[idx].m_fd_to_user, "Message not given", strlen("Message not given")) == -1) {
                        perror ("Error writing back to user");
                }
		return -1;
	}

	//Write \p2p message
	char inbuf [MAX_MSG];
	sprintf(inbuf, "%s:%s", user_list[idx].m_user_id, message);
	if (write(user_list[index].m_fd_to_user, inbuf, strlen(inbuf)) == -1) {
		perror ("Error writing to other user");
		return - 1;
	}
	return 0;

}

//takes in the filename of the file being executed, and prints an error message stating the commands and their usage
void show_error_message(char *filename)
{
	fprintf(stderr, "Usage: %s : no arguments are allowed\n", filename);
}


/*
 * Populates the user list initially
 */
void init_user_list(USER * user_list) {

	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to -1
	//set the status to be EMPTY
	int i=0;
	for(i=0;i<MAX_USER;i++) {
		user_list[i].m_pid = -1;
		memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
		user_list[i].m_fd_to_user = -1;
		user_list[i].m_fd_to_server = -1;
		user_list[i].m_status = SLOT_EMPTY;
	}
}

/* ---------------------End of the functions that implementServer functionality -----------------*/


/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{

	if (argc != 1) {
		show_error_message(argv[0]);
		return -1;
	}
	int nbytes;
	setup_connection("PIERRE_DEREK_JACK"); // Specifies the connection point as argument.

	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	char buf[MAX_MSG]; 
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK); //make stdin non-blocking
	print_prompt("admin");
	
	pid_t childpid = 1; //Sentinel value, parent code runs initially

	int pipe_SERVER_reading_from_child[2]; //server read, child write
	int pipe_SERVER_writing_to_child[2]; //server write, child read
	int pipe_child_writing_to_user[2]; //child write, user read
	int pipe_child_reading_from_user[2]; //child read, user write
	char user_id[MAX_USER_ID];	

	while(1) {
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/

		// Handling a new connection using get_connection


		if (childpid > 0) {
			if (get_connection(user_id, pipe_child_reading_from_user, pipe_child_writing_to_user) != -1) {	
				close(pipe_child_reading_from_user[1]); //child should not write to this pipe
				close(pipe_child_writing_to_user[0]); //child should not read from this pipe
	
				int idx = find_empty_slot(user_list);
				//Check max user and same user id
				if(idx == -1) {
					perror("Cannot add user, no more room");
					print_prompt("admin");
				}
				else if (find_user_index(user_list, user_id) != -1) {
					perror("Cannot add user, user name invalid");
					print_prompt("admin");
				}
				else {
					//Initialize SERVER-CHILD set of pipes
					if (pipe(pipe_SERVER_reading_from_child) < 0 || pipe(pipe_SERVER_writing_to_child) < 0) {
						perror("Error initializing pipes");
					}
	
					childpid = fork();
					
					if (childpid == -1) {
						perror("Error forking child");
					}
					if (childpid == 0) {
						close(pipe_SERVER_writing_to_child[1]); //child should not write to this pipe
						close(pipe_SERVER_reading_from_child[0]); //child should not read from this pipe
						
						fcntl (pipe_child_reading_from_user[0], F_SETFL, fcntl(pipe_child_reading_from_user[0], F_GETFL) | O_NONBLOCK); //child reads from user non-blocking
						fcntl (pipe_SERVER_writing_to_child[0], F_SETFL, fcntl(pipe_SERVER_writing_to_child[0], F_GETFL) | O_NONBLOCK); //child reads from server non-blocking
						fcntl (pipe_child_writing_to_user[1], F_SETFL, fcntl(pipe_child_writing_to_user[1], F_GETFL) | O_NONBLOCK);
						fcntl (pipe_SERVER_reading_from_child[1], F_SETFL, fcntl(pipe_SERVER_reading_from_child[1], F_GETFL) | O_NONBLOCK);
						continue; //jump to beginning of new iteration
					}
					else {
						//Don't want the parent server process to communicate directly with the user process in client
						close(pipe_SERVER_reading_from_child[1]);
						close(pipe_SERVER_writing_to_child[0]);
	
						close(pipe_child_reading_from_user[0]);
						close(pipe_child_writing_to_user[1]);

						fcntl(pipe_SERVER_reading_from_child[0], F_SETFL, fcntl(pipe_SERVER_reading_from_child[0], F_GETFL) | O_NONBLOCK);
						fcntl(pipe_SERVER_writing_to_child[1], F_SETFL, fcntl(pipe_SERVER_writing_to_child[1], F_GETFL) | O_NONBLOCK);
						//m_fd_to_user -> pipe_SERVER_writing_to_child[1]
						//m_fd_to_server -> pipe_SERVER_reading_from_child[0]
						add_user(idx, user_list, childpid, user_id, pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0]);
						sprintf(buf, "\nA new user: %s connected. slot:%d\n", user_id, idx);
						write(1, buf, strlen(buf));
						memset(buf, '\0', MAX_MSG);
						print_prompt("admin");
					}
				}

			}
		
			// Server process: Add a new user information into an empty slot   
			// Poll child processes and handle user commands
			// Poll stdin (input from the terminal) and handle admnistrative command
	
			//Handle administrative commands 
			if ((nbytes = read(0, buf, MAX_MSG)) != -1) {
				if (buf[strlen(buf) - 1] == '\n') {
					if (strlen(buf) == 1) {
						print_prompt("admin");
						continue;
					}
					else
						buf[strlen(buf) - 1] = 0;
				}
				enum command_type type = get_command_type(buf);
				
				switch (type) {
				
					case LIST:
						list_users(-1, user_list);
						break;

					case KICK:
						//extract name
						if (extract_name(buf, user_id) == -1) {
							fprintf(stderr, "Usage: \\kick <user_id>\n");
							break;
						}
						//find user index
						int idx;
						if ((idx = find_user_index(user_list, user_id)) != - 1) {
							kick_user(idx, user_list);
						}
						else {
							fprintf(stderr, "User does not exist, use \\list\n");
						}
						break;

					case EXIT:
						cleanup_users(user_list);
						return 0;
					default:
						;
						char notice[MAX_MSG + strlen("admin-notice: ")];
						memset(notice, '\0', MAX_MSG + strlen("admin-notice: "));						
						strcat(notice, "admin-notice: ");
						strcat(notice, buf); 
						broadcast_msg(user_list, notice, NULL);
				}
				memset(buf, '\0', MAX_MSG);
				print_prompt("admin");	
			}
			
			//Handle user commands
			for (int i = 0; i < MAX_USER; i++) {
				if (user_list[i].m_status == SLOT_FULL) {
					nbytes = read(user_list[i].m_fd_to_server, buf, MAX_MSG);
					if (nbytes != -1) {
						enum command_type type = get_command_type(buf);
						switch (type) {
							case LIST:
								write(1, "\nlist\n", 6);
								print_prompt("admin"); 
								list_users(i, user_list);
								break;
							case P2P:
								write(1, "\np2p\n",5);	
								send_p2p_msg(i, user_list, buf);
								print_prompt("admin");
								break;
							case EXIT:
								;
								char termination[MAX_MSG];
								sprintf(termination, "\nThe user: %s seems to be terminated\n", user_list[i].m_user_id);
								write(1, termination, strlen(termination));
								kick_user(i, user_list);
								print_prompt("admin");	
								break;
							default:
								;
								char broadcast[MAX_MSG + MAX_USER_ID];
								memset(broadcast, '\0', MAX_MSG + MAX_USER_ID);
								strcat(broadcast, user_list[i].m_user_id);
								strcat(broadcast, ": ");
								strcat(broadcast, buf);
								broadcast_msg(user_list, broadcast, user_list[i].m_user_id);
								/*Release version doesn't do this, style?*/								
								//write(1, "\n", 1);								
								//write(1, broadcast, strlen(broadcast));
								//write(1, "\n", 1);
								//print_prompt("admin");
								break;
						}
						memset(buf, '\0', MAX_MSG);	
					}
				}
			}

		}
		

		// Child process: poll users and SERVER
		if (childpid == 0) {
			memset(buf, '\0', MAX_MSG);
			nbytes = read(pipe_child_reading_from_user[0], buf, MAX_MSG);
			if (nbytes > 0) {
				write(pipe_SERVER_reading_from_child[1], buf, nbytes);
				/* WARM-UP CODE 
				//fprintf(stderr, "Child:%s\n", buf);
				*/
			}
			else if (nbytes == 0) {
				strcpy(buf, "\\exit");
				write(pipe_SERVER_reading_from_child[1], buf, strlen(buf));
			}
			if ((nbytes = read(pipe_SERVER_writing_to_child[0], buf, MAX_MSG)) != -1) {
				write(pipe_child_writing_to_user[1], buf, nbytes);
			}	
		}

		//Sleep for polling loop
		usleep(10000);
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/
	}
}

/* --------------------End of the main function ----------------------------------------*/
