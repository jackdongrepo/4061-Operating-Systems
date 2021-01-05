/*CSCI 4061 Fall 2018 Project 3
* Name: Pierre Abillama, Jack Dong, Derek Frank
* X500: abill001, dongx462, fran0954 */

Group 46

##Contributions:

We met a total of three times as a full group in Keller 1-250.

The first meeting we layed out the broad scale architecture of the main thread, and implemented the dispatcher threads, the request queue (as a bounded buffer), and started on the implementation of the worker threads functionality (pulling a request from the queue, calling the appropriate cache functions, and reading the file from the disk). We also implemented the web server logging. 

The second meeting we finished up the worker thread implementation, implemented the cache, finished the main thread, and verified if our caching code produced any memory leaks. We used valgrind as a tool to check if our server produced any memory leaks. We also ran tests to see if bad requests would return errors appropriately. We noted memory leaks were occuring in the functions provided to us, so Pierre emailed the TAs including the professor to ensure that the memory leak issue could then be resolved. 

The third meeting we ran a perfomance analysis on our web server, documented the results, and generally wrapped up the project.

Because we only ever contributed to this project as a team (partner programming on a single computer) and have found this method to be most efficient in the past, it is hard to say specifically what our individual contributions are.

##How to compile:

A makefile was provided in the tar submission. The makefile was altered to compile the code with -g option so that debugging could be achieved using valgrind. 
	
	make

##How to run:

After unzipping the project folder, use make to produce an executable. Then use the following command to run the program.

	./web_server <port> <path> <num_dispatcher> <num_workers> <dynamic_flag> <queue_length> <cache_entries>

	<port> - the port you wish to access the web server from

	<path> - path to root of web server

	<num_dispatcher> - the number of dispatcher threads to create (max 100)

	<num_workers> - number of worker threads to create (max 100)	

	<dynamic_flag> - Always 0

	<queue_length> - The length of the request queue

	<cache_entries> - The number of files to cache

In another terminal window, run a command that will produce a request to the server using wget for instance. 

To terminate the program, hit ^C in the terminal window where the server is running.

##Caching Mechanism
LFU - Least Frequently Used. When new items are added to the cache, they are initialized with a frequency of 0. Then every time a request comes in for a particular cache entry, that entry's frequency is incremented by one. Then when a new item needs to get added to a full cache, the entry with the lowest frequency (first in cache if tie) is replaced

##Dynamic Pool Size:
We did not implement dynamic pool thread size

##Web Server Logging:

The logging mechanism creates a new file in the web tree root directory named "web\_server\_log" if it does not already exist from a previous run of the program. The server will append all of its request logging to the end of the "web\_server\_log" file, i.e. it will not overwrite the contents of the previous run. We opted to do this because it made more sense in terms of logging to keep track of multiple runs of our program. 
   


