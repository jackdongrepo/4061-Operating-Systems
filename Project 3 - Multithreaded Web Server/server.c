#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <linux/limits.h>

#define MAX_THREADS 100
#define MAX_QUEUE_LEN 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024

//Uncomment the following line to be in DEBUG mode
//#define DEBUG

/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/

// structs:

//Entity stored within the request buffer
typedef struct request {
   	int fd;
	char request[BUFF_SIZE];
} request_t;

//The request buffer
typedef struct request_queue {
 	int next_slot_to_store;
  	int next_slot_to_service;
   	int num_requests_in_queue;
   	int request_queue_size;
	request_t * requests;
} request_queue_t;

//Entity stored within the cache
typedef struct cache_entry {
	int len;
    	char request[BUFF_SIZE];
    	char *content;
	int freq_used;
} cache_entry_t;

//The cache
typedef struct cache {
	int next_slot_to_store;
	int num_cache_entries;
	int cache_size;
	cache_entry_t * entries;
} cache_t;

//Globals
request_queue_t * request_queue;
cache_t * cache; 
char * working_dir; //Web tree root directory

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cacheMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t freed = PTHREAD_COND_INITIALIZER;
pthread_cond_t filled = PTHREAD_COND_INITIALIZER;

volatile int signal_received = 0;

/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg) {
  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
  }
}
/**********************************************************************************/

/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
	// return the index if the request is present in the cache
  	int i;
	cache_entry_t entry;
	for (i = 0; i < cache->num_cache_entries; i++) {
		entry = cache->entries[i];
		#ifdef DEBUG
		fprintf(stderr, "Entry @ %d: %s\n", i, entry.request); 
		#endif
		//Cache HIT
		if (strcmp(entry.request, request) == 0) {
			cache->entries[i].freq_used++;
			return i;
		}
	}
	return -1;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *request, char *memory , int memory_size){
  	// It should add the request at an index according to the cache replacement policy
	// Make sure to allocate/free memeory when adding or replacing cache entries
	cache_entry_t new_entry;
	strcpy(new_entry.request, request);
	new_entry.content = (char *) malloc (sizeof(char) * (memory_size));
	memset(new_entry.content, 0 , memory_size);
	memcpy(new_entry.content, memory, memory_size);
	new_entry.len = memory_size;	
	new_entry.freq_used = 0;
	//If cache is not yet full. just add to the next open spot in the cache
	if (cache->num_cache_entries != cache->cache_size) {	
		cache->entries[cache->next_slot_to_store] = new_entry;
		cache->num_cache_entries++;
		#ifdef DEBUG
		fprintf(stderr, "Entry content %s\n", cache->entries[cache->next_slot_to_store].content);
		#endif
		cache->next_slot_to_store = (cache->next_slot_to_store + 1) % cache->cache_size;
	}
	
	//If cache is full, then replace the entry with the lowest frequency ---> LFU
	else {
		cache_entry_t evict = cache->entries[0];
		int evict_index = 0;
		for(int i = 1; i<cache->cache_size;i++){
			if(cache->entries[i].freq_used < evict.freq_used){
				evict = cache->entries[i];
				evict_index = i;
			}	
		}
		free(cache->entries[evict_index].content);
		cache->entries[evict_index] = new_entry;
	}
}

// clear the memory allocated to the cache
void deleteCache(){
  	// De-allocate/free the cache memory
	for (int i = 0; i < cache->num_cache_entries; i++) {
		free(cache->entries[i].content);
	}
	free (cache->entries);
	free (cache);
}

// Function to initialize the cache
void initCache(int cache_entries){
	// Allocating memory and initializing the cache array
	cache = (cache_t *) malloc(sizeof(cache_t));
	cache->next_slot_to_store = 0;
	cache->num_cache_entries = 0;
	cache->cache_size = cache_entries;
	cache->entries = (cache_entry_t *) malloc (sizeof(cache_entry_t) * cache_entries);
}

// Function to open and read the file from the disk into the memory
int readFromDisk(char * fileName, int size, char * buf) {
	// Open and read the contents of file given the request
	int fd = open (fileName, O_RDONLY);
	if (read(fd, buf, size) == -1) {
		strcpy(buf, "Error reading from disk");
		close(fd);
		return -1;
	}
	close (fd);
	return 0;
}

//Function to initialize the request queue, similar to that of the cache
void initRequestQueue (int qlen) {
	//Allocating memory and initializing the request queue
	request_queue = (request_queue_t *) malloc(sizeof(request_queue_t));
	request_queue->next_slot_to_store = 0;
	request_queue->next_slot_to_service = 0;
	request_queue->num_requests_in_queue = 0;
	request_queue->request_queue_size = qlen;
	request_queue->requests = (request_t *) malloc(qlen * sizeof(request_t));	
}

//Function to clear the memory allocated to the request queue
void deleteRequestQueue () {
	//De-allocate/free the request queue memory
	free(request_queue->requests);
	free(request_queue);
}	

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
void getContentType(char * request_string, char * content_type) {
  	// Should return the content type based on the file type in the request
	// (See Section 5 in Project description for more details)
	//Get the content_type of the file
	
	char * file_extension = strrchr(request_string, '.');
	#ifdef DEBUG
	fprintf(stderr, "File Extension %s\n", file_extension);
	#endif
	if (file_extension == NULL)
		file_extension = "";
	if (strcmp(file_extension, ".html") == 0|| strcmp(file_extension, ".htm") == 0)
		strcpy(content_type, "text/html");
	else if (strcmp(file_extension, ".jpg") == 0)
		strcpy(content_type, "image/jpeg");
	else if (strcmp(file_extension, ".gif") == 0)
		strcpy(content_type, "image/gif");
	else
		strcpy(content_type, "text/plain");	
}

// This function returns the current time in microseconds
long getCurrentTimeInMicro() {
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return curr_time.tv_sec * 1000000 + curr_time.tv_usec;
}

/**********************************************************************************/

// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
	
	int fd;
	char buf[BUFF_SIZE];
	memset(buf, 0, BUFF_SIZE);
	while (1) {
		// Accept client connection
		if ((fd = accept_connection()) > 0) {	
			// Get request from the client
			if (get_request(fd, buf) == 0) {
				//Add the request into the queue
				pthread_mutex_lock(&mtx);
				while(request_queue->num_requests_in_queue == request_queue->request_queue_size) {
					pthread_cond_wait(&freed, &mtx);
				}
				request_t new_request;
				new_request.fd = fd;
				strcpy(new_request.request, buf);
				request_queue->requests[request_queue->next_slot_to_store] = new_request;
				#ifdef DEBUG
				fprintf(stderr, "Added Request Entry@ %d - Target: %s\n", request_queue->next_slot_to_store, request_queue->requests[request_queue->next_slot_to_store].request);
				#endif
				request_queue->num_requests_in_queue++;
				request_queue->next_slot_to_store = (request_queue->next_slot_to_store + 1) % (request_queue->request_queue_size);
				pthread_mutex_unlock(&mtx);
				pthread_cond_signal(&filled);
			}	
		}
	}
   	return NULL;
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
	
	
	long startTime, endTime, duration;
	int cacheIndex;
	request_t request;
	struct stat st;
	size_t size;
	char content_type [11];
	char log_message[2048];
	int reqNum = 0;
	
	while (1) {
	
	reqNum = reqNum + 1;	

	pthread_mutex_lock(&mtx);
	while (request_queue->num_requests_in_queue == 0) {
		pthread_cond_wait(&filled, &mtx);
	}
	
	// Start recording time
	startTime = getCurrentTimeInMicro();	
	

	// Get the request from the queue
	request = request_queue->requests[request_queue->next_slot_to_service];
	request_queue->next_slot_to_service = (request_queue->next_slot_to_service + 1) % (request_queue->request_queue_size);
	request_queue->num_requests_in_queue--;
	pthread_mutex_unlock(&mtx);
	pthread_cond_signal(&freed);
	
	//Get the content_type of the file
	getContentType(request.request, content_type);	
			
	// Get the data from the disk or the cache
	pthread_mutex_lock(&cacheMtx);
	cacheIndex = getCacheIndex(request.request);
	#ifdef DEBUG
	fprintf(stderr, "Retrieved Cache Index: %d\n", cacheIndex);
	#endif	

	//Cache HIT, get the content from the cache and service the request
	if (cacheIndex != -1) {	
		size = cache->entries[cacheIndex].len;	
		char * buf = (char *) malloc (sizeof(char) * size); 
		memset(buf, 0, size);
		memcpy(buf, cache->entries[cacheIndex].content, size);	
		// Stop recording the time
		endTime = getCurrentTimeInMicro();
		//return the result
		return_result(request.fd, content_type, buf, size);
		free(buf);	
	}
	pthread_mutex_unlock(&cacheMtx);

	//Cache MISS, get the content from the disk
	if (cacheIndex == -1) {	

		//Get the absolute path of the target file
		char * absolute_path = (char *) malloc (sizeof(char) * (strlen(working_dir) + strlen(request.request) + 1));
		sprintf(absolute_path, "%s%s", working_dir, request.request);		
		#ifdef DEBUG
		fprintf(stderr, "%s\n", absolute_path);
		#endif

		//Get the size of the file
		
		//Size of the file could not be retrieved, probably an error in the path
		if (stat(absolute_path, &st) == -1) {
			
			free(absolute_path);

			endTime = getCurrentTimeInMicro();
			duration = endTime - startTime;
			
			char error [50] = "Couldn't retrieve file size, file not found";
			return_error(request.fd, error);
				
			//log the error request
			pthread_mutex_lock(&logMtx);
			FILE * log_fp = fopen ("./web_server_log", "a");
			sprintf(log_message, "[%d][%d][%d][%s][%s][%lumicros][MISS]\n", * (int*) arg, reqNum, request.fd, request.request, error, duration);
			fputs(log_message, log_fp);
			fclose(log_fp);
			fprintf(stdout, "%s", log_message);
			pthread_mutex_unlock(&logMtx);
			
			//Return to the top of the loop to get the next request
			continue;

		}
		
		size = st.st_size;

		//Allocate space for buf and read from memory to send to return result
		char * buf = (char *) malloc (sizeof(char) * (size + 1));
		buf[size] = '\0';
		memset(buf, 0, size);
		
		//File could not be read
		if (readFromDisk(absolute_path, size, buf) == -1) {
			free(absolute_path);
			
			endTime = getCurrentTimeInMicro();
			duration = endTime - startTime;
			
			return_error(request.fd, buf);
			
			//log the request
			pthread_mutex_lock(&logMtx);
			FILE * log_fp = fopen ("./web_server_log", "a");
			sprintf(log_message, "[%d][%d][%d][%s][%s][%lumicros][MISS]\n", * (int *) arg, reqNum, request.fd, request.request, buf, duration);
			fputs(log_message, log_fp);
			fclose(log_fp);
			fprintf(stdout, "%s", log_message);
			pthread_mutex_unlock(&logMtx);
			
			continue;

		}
	
		
		//Make cache entry and store in cache
		pthread_mutex_lock(&cacheMtx);
		addIntoCache(request.request, buf, size);	
		pthread_mutex_unlock(&cacheMtx);
	
		// Stop recording the time
		endTime = getCurrentTimeInMicro();
	
		// return the result
		return_result(request.fd, content_type, buf, size);		
		free(buf);
		free(absolute_path);
	}

	duration = endTime - startTime;

    	// Log the request into the file and terminal
	pthread_mutex_lock (&logMtx);
	FILE * log_fp = fopen ("./web_server_log", "a");
	if (cacheIndex == -1)
		sprintf(log_message, "[%d][%d][%d][%s][%ld][%lumicros][MISS]\n", * (int *) arg, reqNum, request.fd, request.request, size, duration);
	else 
		sprintf(log_message, "[%d][%d][%d][%s][%ld][%lumicros][HIT]\n", * (int *) arg, reqNum, request.fd, request.request, size, duration);
	
	fputs(log_message, log_fp);
	fclose(log_fp);
	fprintf(stdout, "%s", log_message);
	pthread_mutex_unlock(&logMtx);	
	}

  	return NULL;
}

/**********************************************************************************/

//Signal handler used to handle SIGINT
//Sets signal_received to unblock main thread
void catch_int (int signo) {
	signal_received = 1;
	fprintf(stderr, "\nReceived signal, shutting down server ...\n"); 
}

int main(int argc, char **argv) {
	
  	// Error check on number of arguments
  	if(argc != 8){
    	printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_entries\n", argv[0]);
    	return -1;
  	}

  	// Get the input args
	int port = atoi(argv[1]);
	char * path = argv[2];
	int num_dispatcher = atoi(argv[3]);
	int num_workers = atoi(argv[4]);
	int dynamic_flag = atoi(argv[5]);
	int qlen = atoi(argv[6]);
	int cache_entries = atoi(argv[7]);

  	// Perform error checks on the input arguments
	if (port < 1025 || port > 65535) {
		printf ("Error: Invalid arg %d for port - You may only use ports 1025 to 65535 by default\n", port);
		return INVALID;
	}

	if (num_dispatcher > MAX_THREADS || num_dispatcher < 1) {
		printf("Error: Invalid arg %d for num_dispatcher - The number of dispatcher threads should be between 1 and %d\n", num_dispatcher, MAX_THREADS);
		return INVALID;
	}  
	
	if (num_workers > MAX_THREADS || num_workers < 1) {
		printf("Error: Invalid arg %d for num_workers - The number of worker threads should be between 1 and %d\n", num_workers, MAX_THREADS);
		return INVALID;
	}
	
	if (dynamic_flag != 0) {
		printf("Error: Invalid arg %d for dynamic_flag - Dynamic worker thread pool size functionality has not been implemented\n", dynamic_flag);
	 	printf("Default value of 0 for dynamic_flag is assumed\n");
		dynamic_flag = 0;	
	}

	if (qlen > MAX_QUEUE_LEN || qlen < 1) {
		printf("Error: Invalid arg %d for qlen - The length of the request queue should be between 1 and %d\n", qlen, MAX_QUEUE_LEN);
		return INVALID;
	} 
	
	if (cache_entries > MAX_CE || cache_entries < 1) {
		printf("Error: Invalid arg %d for cache_entries - The number of cache entries should be between 1 and %d\n", cache_entries, MAX_CE);
		return INVALID;
	}

  	// Change the current working directory to server root directory
	if (chdir(path) == -1) {
		printf("Error: Invalid arg %s for path - Could not access web tree root directory specified\n", path);
		return INVALID;
	}	
	
	fprintf(stdout, "Starting up server @ port %d: %d disp, %d work ...\n", port, num_dispatcher, num_workers);
	fprintf(stdout, "Initializing cache with %d entries ...\n", cache_entries);
	fprintf(stdout, "Initializing request queue of size %d ...\n", qlen);
  	// Start the server and initialize cache and request queue
	init(port);
	initRequestQueue(qlen);
	initCache(cache_entries);

	//Get the absolute path of the web tree root directory
	working_dir = (char *) malloc (sizeof(char) * PATH_MAX);
	memset(working_dir, 0, PATH_MAX);
	realpath(".", working_dir);
	fprintf(stdout, "Web tree rooted @ %s\n", working_dir);
	
	//Setup signal handler
	struct sigaction act;
	act.sa_handler = catch_int;
	sigfillset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);

  	// Create dispatcher and worker threads
	pthread_t * dispatcher_threads = (pthread_t*) malloc(num_dispatcher * sizeof(pthread_t));
	pthread_t * worker_threads = (pthread_t *) malloc(num_workers * sizeof(pthread_t));
	
	int * idsDispatcher = (int *) malloc(num_dispatcher * sizeof(int));
	int * idsWorkers = (int *) malloc(num_workers * sizeof(int));

	memset(dispatcher_threads, 0, num_dispatcher * sizeof(pthread_t));
	memset(worker_threads, 0, num_workers * sizeof(pthread_t));

	int i, j, k, l;

	for (i = 0; i < num_dispatcher; i++) {
		idsDispatcher[i] = i;
		if (pthread_create(&dispatcher_threads[i], NULL, dispatch, (void *) &idsDispatcher[i]) != 0) {
			printf("Error creating dispatcher thread %d", idsDispatcher[i]);
		}		
	}
	
	for (j = 0; j < num_workers; j++) {
		idsWorkers[j] = j;
		if (pthread_create(&worker_threads[j], NULL, worker, (void *) &idsWorkers[j]) != 0) {
			printf("Error creating worker thread %d", idsWorkers[j]);
		}
	}
	
	//Detach all threads 
	for (k = 0; k < num_dispatcher; k++) {
		pthread_detach(dispatcher_threads[k]);
	} 

	for (l = 0; l < num_workers; l++) {
		pthread_detach(worker_threads[l]);
	}

	//Block on SIGINT
	while (!signal_received);
	
	//Clean up
	for (k = 0; k < num_dispatcher; k++) {
		pthread_cancel(dispatcher_threads[k]);
	} 

	for (l = 0; l < num_workers; l++) {
		pthread_cancel(worker_threads[l]);
	}
		
	free(working_dir);
	free(dispatcher_threads);
	free(worker_threads);
	free(idsDispatcher);
	free(idsWorkers);
	fprintf(stderr, "Cleaning up cache ...\n");
	deleteRequestQueue();
	fprintf(stderr, "Emptying request queue ...\n");
	deleteCache();
	fprintf(stderr, "Shut down successful\n");
  	
	return 0;
}
