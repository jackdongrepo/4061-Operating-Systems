/* CSci4061 F2018 Assignment 2
 * section: 1
 * date: 11/08/18
 * name: Jack Dong, Derek Frank, Pierre Abillama
 * id:   5245772, 5276563, 5290574 */

The purpose of our program is to implement a chat room using IPC, specifically pipes. Our chat service is based on a single centralized server that manages mutliple pipes communicating to the user processes via child processes.

To implement this project, we met several times in KH 1-250 as a full group and partner programmed on a single computer. Our first meeting focused on implementing the handler functions whose outlines were provided, and setting up the
pipes that will be used throughout the program to communicate between the server and the user. We also wrote a general outline (in code) of the cases that needed to be handled in server.c for the different kinds of commands, be it read from
stdin of the server program or the pipes of the child processes. We also finalized the warmup exercise which allowed us to confirm that we had done the pipe setup correctly.

Pierre wrote the code for client.c which involved reading from both stdin of the program and the pipes connecting the user to the child processes. Pierre also worked on adding error handling to all the system calls and the error message for program usage (calling
the program with the right arguments).

Jack and Derek worked together on the command cases (e.g \p2p, \kick) and ensuring that all user programs are terminated if the server is interrupted (part of the extra credit) 

Our final meeting, we worked on deliverables and compared our solution to the release version. We opted not to print the \p2p message in the server program (as it was not mentioned in the write up but featured in the release).

To compile the program, run make in the project directory. Two executable programs will be generated: 

	- ./server -- no arguments
	- ./client <userName> -- single argument (no spaces) name for user joining chat room

To run this program, execute ./myServer in one terminal window which will begin the admin server and wait for user connections. To connect a user to the chat room, run ./myClient <userName> in another terminal window (max 10) which will begin the user process for the chat room.

Admin Command List:

	- \list -- lists out the users present in the chat room, prints "<no users>" if none are present
	- \kick <userName> -- removes <userName> from the chat room, and terminates the corresponding user and child processes. If used incorrectly, an error message will guide the admin to correct usage. 
	- \exit -- terminates the server program and all child and user processes. The user processes are notified of the server's termination.
	- <default> -- broadcasts message to all active users, prefaced by "admin-notice".

User Command List:

	- \list -- lists out the users present in the chat room, prints "<no users>" if none are present. The server will print "list" in its terminal window to notify an admin that a "\list" command was run (similar to the release).
	- \exit -- terminates the user program and the child process associated with it. The server is notified of the user's termination.
	- \p2p <userName> <message> -- allows the current user to send a private <message> to <userName>. The message is prefaced by the name of the current user. The server will print "p2p" in its terminal window to notify an admin that a "\p2p" command was run (similar to the release). If used incorrectly, an error message will guide the user to correct usage.
	- \seg -- allows the user to simulate what a segmentation fault will do to the chat room. The server will be notified that the user has been terminated and will remove the user from the user list, handling all processes properly.
	- <default> -- broadcasts message to all active users (not including admin), prefaced by current user name.
 
Our program allows users to chat (broadcast or peer to peer messages) to one another and list who's in the chat room. The user can leave the chat room at any time by typing "\exit" and can also potentially be kicked from the admin by typing "\kick <userName>" in the server.

Our program assumes that this chat room will all run on the same machine. It also assumes that the max message size is 255 characters.

NOTE: The program can handle server interruptions and terminates all user processes correctly. In addition, the program handles user interruptions and other potential segmentation faults, i.e. the extra credit portion. 

Our strategies for error handling were mostly based off the implementation notes given in the write-up which includes checking if the read from a pipe returns zero when broken. When the child process reads zero from the read pipe between itself and the user, we ask the server to clean up the corresponding child process by writing "\exit" to the child's write pipe between itself and the server. We handle all system call errors by checking their return values and the printing an error message to stderr or using perror.
