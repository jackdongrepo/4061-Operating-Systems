/* CSci4061 F2018 Assignment 1
* login: abill001
* date: 10/05/18
* name: Jack Dong, Derek Frank, Pierre Abillama
* id: dongx462, fran0954, abill001 */

// This is the main file for the code
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

//#define DEBUG

/*-------------------------------------------------------HELPER FUNCTIONS PROTOTYPES---------------------------------*/
void show_error_message(char * ExecName);
//Write your functions prototypes here

/*
 * show_targets:
 *
 * Prints the name, dependency count, dependency names (if any) and the command of each target in a given dependency graph
 *
 * Input:
 * 	targets      - an array of targets, data structure modeling the dependency graph
 * 	nTargetCount - size of targets array
 */
void show_targets(target_t targets[], int nTargetCount);

/*
 * build:
 *
 * Recursively builds a target and its dependencies by executing their commands.
 *
 * Input:
 * 	target       - the address of the target to initially build
 * 	targets      - an array of all possible targets in the dependency graph
 * 	nTargetCount - size of targets array
 *
 * Returns:
 * 	-1 if any target fails to build, or if the OS fails to fork a child process
 *	 0 if the building of the target and its dependencies is successful
 *   
 */
int build(target_t *target, target_t targets[], int nTargetCount);
/*-------------------------------------------------------END OF HELPER FUNCTIONS PROTOTYPES--------------------------*/


/*------------------------------------------------------HELPER FUNCTIONS--------------------------------------------*/

//This is the function for writing an error to the stream
//It prints the same message in all the cases
void show_error_message(char * ExecName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", ExecName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	exit(0);
}

//Write your functions here

//Phase1: Warmup phase for parsing the structure here. Do it as per the PDF (Writeup)
void show_targets(target_t targets[], int nTargetCount)
{
	//Write your warmup code here
	for (int i = 0; i < nTargetCount; i++) {
		printf("TargetName: %s\n", targets[i].TargetName);
		printf("DependencyCount: %d\n", targets[i].DependencyCount);
		if (targets[i].DependencyCount > 0) {
			printf("DependencyNames: ");
			for (int j = 0; j < targets[i].DependencyCount; j++) {
				printf("%s", targets[i].DependencyNames[j]);
				if (j != targets[i].DependencyCount - 1)
					printf(", ");
				else
					printf("\n");
			}
		}
		printf("Command: %s\n\n", targets[i].Command); 
	}	
}

int build(target_t *target, target_t targets[], int nTargetCount) {
	int dependencyCount = target->DependencyCount;
	if (dependencyCount == 0) //the target has no dependencies and should simply be built as a result
		target->Status = NEEDS_BUILDING;
	else {
		for (int i = 0; i < dependencyCount; i++) { //iterate through the target's dependencies
			int dependsOnIndex = find_target(target->DependencyNames[i], targets, nTargetCount);
			if (dependsOnIndex != -1) { //the dependency is a target in the makefile
				target_t * dependsOn = &targets[dependsOnIndex];
#ifdef DEBUG
				printf("%s status %d, %s status %d\n", target->TargetName, target->Status, dependsOn->TargetName, dependsOn->Status);
#endif
				if (dependsOn->Status == UNFINISHED) { //no need to build if the dependency target has already been built
					if (compare_modification_time(dependsOn->TargetName, target->TargetName) != 2) { //no need to build if the target has been modified later than the dependency
						target->Status = NEEDS_BUILDING;
						if (build(dependsOn, targets, nTargetCount) == -1) { //an error has occured during a recursive call to build, all builds must fail afterwards
							return -1;
						};
					}
				}
			}
			else { //The dependency is not a target in the makefile
				if (target->Status == UNFINISHED) {
					if (compare_modification_time(target->DependencyNames[i], target->TargetName) != 2) { //no need to mark target if the target has been modified later than the dependency
						target->Status = NEEDS_BUILDING;
					}
				}
			}
		}
	}

	if (target->Status != NEEDS_BUILDING) {
		target->Status = FINISHED;	
		return 0;
	}

	pid_t childpid = fork();
	if (childpid == -1) { //the fork failed
		printf("Failed to fork child process");
		return -1;
	}
	if (childpid == 0) { //child process
		
		//Dynamically allocate memory for tokens array
		char **tokens = (char **) malloc (MAX_NODES * sizeof(char *));
		for (int i = 0; i < MAX_NODES; i++)
			tokens[i] = (char *) malloc (ARG_MAX * sizeof(char));

		int numberOfTokens = parse_into_tokens(target->Command,tokens," ");
#ifdef DEBUG
		for (int i = 0; tokens[i] != 0; i++) {
			printf("Token: %s\n", tokens[i]);
		}	
#endif			
		execvp(tokens[0],&tokens[0]); //No need to free char** tokens since process gets wiped after execvp call
		printf("Child with pid=%d failed to execvp", getpid());	
		exit(-1);	
	}
	else {  //parent process
		int status;
	        wait(&status); //waits for child to terminate
		if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)) { //child has terminated normally with a non zero exit status
			printf("Recipe for target %s failed\n", target->TargetName);
			printf("Child exited with error code = %d\n", WEXITSTATUS(status));
			return -1;
		}
		target->Status = FINISHED;
		return 0;
	}

}

/*-------------------------------------------------------END OF HELPER FUNCTIONS-------------------------------------*/


/*-------------------------------------------------------MAIN PROGRAM------------------------------------------------*/
//Main commencement
int main(int argc, char *argv[])
{
  target_t targets[MAX_NODES];
  int nTargetCount = 0;

  /* Variables you'll want to use */
  char Makefile[64] = "Makefile";
  char TargetName[64];

  /* Declarations for getopt */
  extern int optind;
  extern char * optarg;
  int ch;
  char *format = "f:h";
  char *temp;

  //Getopt function is used to access the command line arguments. However there can be arguments which may or may not need the parameters after the command
  //Example -f <filename> needs a finename, and therefore we need to give a colon after that sort of argument
  //Ex. f: for h there won't be any argument hence we are not going to do the same for h, hence "f:h"
  while((ch = getopt(argc, argv, format)) != -1)
  {
	  switch(ch)
	  {
	  	  case 'f':
	  		  temp = strdup(optarg);
	  		  strcpy(Makefile, temp);  // here the strdup returns a string and that is later copied using the strcpy
	  		  free(temp);	//need to manually free the pointer
	  		  break;

	  	  case 'h':
	  	  default:
	  		  show_error_message(argv[0]);
	  		  exit(1);
	  }

  }

  argc -= optind;
  if(argc > 1)   //Means that we are giving more than 1 target which is not accepted
  {
	  show_error_message(argv[0]);
	  return -1;   //This line is not needed
  }

  /* Init Targets */
  memset(targets, 0, sizeof(targets));   //initialize all the nodes first, just to avoid the valgrind checks

  /* Parse graph file or die, This is the main function to perform the toplogical sort and hence populate the structure */
  if((nTargetCount = parse(Makefile, targets)) == -1)  //here the parser returns the starting address of the array of the structure. Here we gave the makefile and then it just does the parsing of the makefile and then it has created array of the nodes
	  return -1;


  //Phase1: Warmup-----------------------------------------------------------------------------------------------------
  //Parse the structure elements and print them as mentioned in the Project Writeup
  /* Comment out the following line before Phase2 */
  //show_targets(targets, nTargetCount);  
  //End of Warmup------------------------------------------------------------------------------------------------------
   
  /*
   * Set Targetname
   * If target is not set, set it to default (first target from makefile)
   */
  if(argc == 1)
	strcpy(TargetName, argv[optind]);    // here we have the given target, this acts as a method to begin the building
  else
  	strcpy(TargetName, targets[0].TargetName);  // default part is the first target

  /*
   * Now, the file has been parsed and the targets have been named.
   * You'll now want to check all dependencies (whether they are
   * available targets or files) and then execute the target that
   * was specified on the command line, along with their dependencies,
   * etc. Else if no target is mentioned then build the first target
   * found in Makefile.
   */
	
  //Phase2: Begins ----------------------------------------------------------------------------------------------------
  /*Your code begins here*/
#ifdef DEBUG
  printf("Target name: %s\n", TargetName);
  printf("This is a test run");
#endif
  target_t initialTarget = targets[find_target(TargetName, targets, nTargetCount)];
  for (int i = 0; i < nTargetCount; i++) {
  	targets[i].Status = UNFINISHED;
  }
  build(&initialTarget, targets, nTargetCount); 
   
  
  /*End of your code*/
  //End of Phase2------------------------------------------------------------------------------------------------------

  return 0;
}
/*-------------------------------------------------------END OF MAIN PROGRAM------------------------------------------*/
