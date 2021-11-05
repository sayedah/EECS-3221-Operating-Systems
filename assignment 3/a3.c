
/*Sources
- “Operating System Concepts”, 10th Edition; Abraham Silberschatz,Peter B. Galvin,Greg Gagne;
   2018; John Wiley and Sons.
- https://www.mkssoftware.com/docs/man3/sem_getvalue.3.asp
- https://www.geeksforgeeks.org/use-posix-semaphores-c/#:~:text=To%20release%20or%20signal%20a,pshared%2C%20unsigned%20int%20value)%3B
- https://www.softprayog.in/programming/posix-threads-synchronization-in-c
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <semaphore.h>

void logStart(char* tID);		/*Log and display to screen when a new thread starts*/
void logFinish(char* tID);	/*Log and display to screen when a thread finishes*/

void startClock();			/*Start program clock*/
long getCurrentTime();	/*Return how much time has passed since the start of the program clock*/
time_t programClock;		/*Global variable for program clock*/

typedef struct thread /*Structure to represent a thread*/
{
	char tid[4]; /*The id of the thread*/
	unsigned int startTime; /*The start time of the thread*/
	int state;
	pthread_t handle;
	int retVal;
} Thread;

int waiting = 0; /*Integer to represent the number of threads waiting to execute their critical section*/
sem_t arr[2]; /*Array of 2 semaphores, arr[0] represents even threads and arr[1] represents odd threads*/

int threadsLeft(Thread* threads, int threadCount); /*Return how many threads are left to execute*/
int threadToStart(Thread* threads, int threadCount);
void* threadRun(void* t);	/*Function to be executed by thread*/
int readFile(char* fileName, Thread** threads); 	/*Read the input file to add thread properties to array, return the number of threads*/

int main(int argc, char *argv[])
{
	if(argc<2) /*Check for input file*/
	{
		printf("Input file name missing...exiting with error code -1\n");
		return -1;
	}

	Thread* threads = NULL;  /*Initialize array of Threads*/
	int threadCount = readFile(argv[1],&threads);  /*Read the input file*/

	/*Sort threads by start time in ascending order*/
	int curr; int j;
  int smallest;
  for (curr = 0; curr <= threadCount-2; curr++) {
    smallest = curr;
    for (j = curr+1; j <= threadCount-1; j++) {
      if ((threads[j].startTime) < (threads[smallest].startTime))
        smallest = j;
    }
    Thread temp = threads[curr];
    threads[curr] = threads[smallest];
    threads[smallest] = temp;
	}

	int first = (threads[0].tid)[2] % 2;  /*Check if first thread is even or odd, and initialize semaphores accordingly*/
	//We want the semaphore that matches the first thread (even or odd) to initialize to 1 so it may enter its critical section right away

	if (first == 0) { /*The first thread is even*/
		sem_init(&arr[1], 0, 0);  sem_init(&arr[0], 0, 1);
	}

  else {  /*The first thread is odd*/
		sem_init(&arr[1], 0, 1);  sem_init(&arr[0], 0, 0);
	}


	startClock();
	while(threadsLeft(threads, threadCount)>0)  /*Start loop for thread creation*/
	{
		int i = 0;
		while((i=threadToStart(threads, threadCount))>-1)
		{
			if (i == threadCount-1) {  /*We've reached the last thread and end of the list. No other threads are going to enter*/
				waiting++;
				int loopCount;
				for (loopCount = 0; loopCount <= waiting+threadCount; loopCount++) { /*Signal and let go of all of the waiting even or odd threads, as well as the last thread*/ //Signal up to waiting+threadCount times, for worst case scenario
					sem_post(&arr[0]); sem_post(&arr[1]);
				}
			}
			threads[i].state = 1;
			threads[i].retVal = pthread_create(&(threads[i].handle),NULL,threadRun,&threads[i]);  /*Create the thread*/
		}
	}
	return 0;
}

int readFile(char* fileName, Thread** threads) /*Called to read file*/
{
	FILE *in = fopen(fileName, "r");
	if(!in)
	{
		printf("Child A: Error in opening input file...exiting with error code -1\n");
		return -1;
	}

	struct stat st;
	fstat(fileno(in), &st);
	char* fileContent = (char*)malloc(((int)st.st_size+1)* sizeof(char));
	fileContent[0]='\0';
	while(!feof(in))
	{
		char line[100];
		if(fgets(line,100,in)!=NULL)
		{
			strncat(fileContent,line,strlen(line));
		}
	}
	fclose(in);

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

	for(int k=0; k<threadCount; k++)
	{
		char* token = NULL;
		int j = 0;
		token =  strtok(lines[k],";");
		while(token!=NULL)
		{
			(*threads)[k].state=0;
			if(j==0)
				strcpy((*threads)[k].tid,token);
			if(j==1)
				(*threads)[k].startTime=atoi(token);
			j++;
			token = strtok(NULL,";");
		}
	}
	return threadCount;
}

void logStart(char* tID) /*Called to log the start of a thread*/
{
	printf("[%ld] New Thread with ID %s is started.\n", getCurrentTime(), tID);
}

void logFinish(char* tID) /*Called to log the finish of a thread*/
{
	printf("[%ld] Thread with ID %s is finished.\n", getCurrentTime(), tID);
}

int threadsLeft(Thread* threads, int threadCount) /*Called to obtain a value of how many threads are left to create*/
{
	int remainingThreads = 0;
	for(int k=0; k<threadCount; k++)
	{
		if(threads[k].state>-1)
			remainingThreads++;
	}
	return remainingThreads;
}

int threadToStart(Thread* threads, int threadCount)
{
	for(int k=0; k<threadCount; k++)
	{
		if(threads[k].state==0 && threads[k].startTime==getCurrentTime())
			return k;
	}
	return -1;
}

void* threadRun(void* t) /*Thread function, called upon thread creation*/
{
	Thread * threadObj = (Thread*) t; /*Create Thread object*/
	int num = (threadObj -> tid)[2] % 2;  /*If num equals 0, thread is even. If num equals 1, thread is odd*/

	logStart(threadObj -> tid); /*Display to screen that thread has started*/

	waiting++; /*Increment the number of threads waiting to execute critical section*/
	sem_wait(&arr[num]); /*Wait for the appropriate semaphore*/

	/*Start of Critical Section*/
	printf("[%ld] Thread %s is in its critical section\n",getCurrentTime(), threadObj -> tid);
	/*End of Critical Section*/

//	num++;
	sem_post(&arr[++num%2]); /*Signal the appropriate semaphore*/
	waiting--; /*Decrement the number of threads waiting to execute critical section*/

	logFinish(threadObj -> tid); /*Display to screen that thread has finished*/
	threadObj -> state = -1; /*Change state of thread*/
	pthread_exit(0);  /*Terminate the thread*/
}


void startClock() /*Called to start the system clock*/
{
	programClock = time(NULL);
}

long getCurrentTime() /*Called to get current time of system*/
{
	time_t now;
	now = time(NULL);
	return now-programClock;
}
