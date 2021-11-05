/* EECS 3221 Assignment 2
   Sayeda Hussain 215427214
	 eecs login: sayedah */

/* Sources:
- “Operating System Concepts”, 10th Edition; Abraham Silberschatz,Peter B. Galvin,Greg Gagne;
   2018; John Wiley and Sons.
- https://stackoverflow.com/questions/26895245/pthread-join-for-asynchronous-threads
- https://www.man7.org/linux/man-pages/man3/pthread_join.3.html
- https://stackoverflow.com/questions/47441672/multi-threading-do-threads-start-running-at-the-exact-same-time
- https://dev.to/quantumsheep/basics-of-multithreading-in-c-4pam */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

typedef struct thread /*Structure to represent a thread*/
{
	char tid[4];    /*The id of the thread as read from file*/
	long starttime;	/*The start time of the thread as read from file*/
	long lifetime; 	/*The life time of the thread as read from file*/
} Thread;

void logStart(char* tID);	/*Log and display to screen when a new thread starts*/
void logFinish(char* tID);	/*Log and display to screen when a thread finished*/

void startClock();	/*Start program clock*/
long getCurrentTime(); /*Return how much time has passed since the start of the program clock*/
time_t programClock; /*Global variable for program clock*/

void* threadRun(void* t); /*Function to be executed by thread*/
int readFile(char* fileName, Thread** threads); /*Read the input file to add thread properties to array, return the number of threads*/


int main(int argc, char *argv[])
{
	if(argc<2) 	/*Check for input file*/
	{
		printf("Input file name missing...exiting with error code -1\n");
		return -1;
	}

	Thread** threadList = malloc(4096);	 /*Initialize array of Threads*/
	int numThreads = readFile(argv[1], threadList);  /*Call function to read input file*/

	/*Sort threads by start time in ascending order*/
	int curr; int j;
  int smallest;
  for (curr = 0; curr <= numThreads-2; curr++) {
    smallest = curr;
    for (j = curr+1; j <= numThreads-1; j++) {
      if ((threadList[j] -> starttime) < (threadList[smallest] -> starttime))
        smallest = j;
    }
    Thread* temp = threadList[curr];
    threadList[curr] = threadList[smallest];
    threadList[smallest] = temp;
	}

	int threadCounter = 0;	/*Create loop counter representing number of threads created*/
	pthread_t id[numThreads]; /*Array of threads id's*/

	startClock(); /*Start clock for thread creation*/

	 while (threadCounter < numThreads) {		/*Start thread creation*/
		 if (getCurrentTime() == threadList[threadCounter] -> starttime) { /*If the current time matches a thread's start time*/
			 pthread_create(&id[threadCounter], NULL, threadRun, threadList[threadCounter]);	/*Create thread*/
			 threadCounter++; /*Increment loop counter*/
		 }
	 }

	 threadCounter = 0; /*Initialize back to 0*/
	 for (threadCounter = 0; threadCounter < numThreads; threadCounter++) { /*Join on each thread*/
	 	pthread_join(id[threadCounter], NULL);
	 }

	return 0;
}

int readFile(char* fileName, Thread** threads)
{
	FILE *file = fopen(fileName, "r");
	if(!file)
	{
		printf("Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}

	struct stat st;
	fstat(fileno(file), &st);
	char* fileContent = (char*) malloc(((int)st.st_size+1)* sizeof(char));
	fileContent[0]='\0';
	while(!feof(file))
	{
		char line[100];
		if(fgets(line,100, file)!=NULL)
		{
			strncat(fileContent,line,strlen(line));
		}
	}
	fclose(file);

	char* command = NULL;
	int threadCount = 0;
	char* fileCopy = (char*)malloc((strlen(fileContent)+1)*sizeof(char));
	strcpy(fileCopy,fileContent);
	command = strtok(fileCopy,"\r\n");
	while(command!=NULL)
	{
		threadCount++;
		command = strtok(NULL,"\r\n");
	}
	*threads = (Thread*) malloc(sizeof(Thread)*threadCount);

	char* lines[threadCount];
	command = NULL;
	int i=0;
	command = strtok(fileContent,"\r\n");
	while(command!=NULL)
	{
		lines[i] = malloc(sizeof(command)*sizeof(char));
		strcpy(lines[i],command);
		i++;
		command = strtok(NULL,"\r\n");
	}

	for(int threadLine=0; threadLine < threadCount; threadLine++)
	{
		char* token = NULL;
		threads[threadLine] = (Thread*) malloc(sizeof(Thread)*threadCount);	/*Allocate memory for new thread's properties*/

		token =  strtok(lines[threadLine],";");	/*Tokenize thread id*/
		strcpy(threads[threadLine] -> tid, token);	/*Add thread id to array*/
		token = strtok(NULL, ";");	/*Tokenize start time*/
		threads[threadLine] -> starttime = atol(token);	/*Convert string to long int and add thread start time to array*/
		token = strtok(NULL, ";");  /*Tokenize lifetime*/
		threads[threadLine] -> lifetime = atol(token);	/*Convert string to long int and add thread lifetime to array*/
  }

	return threadCount;
}

void logStart(char* tID) {	/*Called to log the start of a thread*/
	printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char* tID) { /*Called to log the finish of a thread*/
	printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

void* threadRun(void* t) { /*Thread function*/
	Thread *threadObj = (Thread*) t;	/*Create Thread object*/

	logStart(threadObj -> tid);	/*Display to screen that thread has started*/
	int endtime = (threadObj -> lifetime) + (threadObj -> starttime); /*Calculate the time at which thread ends*/

	while(getCurrentTime() != endtime) {  /*While the thread has not reached the end of its lifetime*/
		; /*Do nothing*/
	} /*Lifetime has been run through*/

	logFinish(threadObj -> tid);	/*Display to screen that thread has finished*/

	pthread_exit(0); /*Terminate thread*/
}

void startClock() { /*Called to start the system clock*/
	programClock = time(NULL);
}

long getCurrentTime() {  /*Called to get current time of system*/
	time_t now;
	now = time(NULL);
	return now-programClock;
}
