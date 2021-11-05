/* EECS 3221 Assignment 1
   Sayeda Hussain 215427214
	 eecs login: sayedah
*/

/*Links for my Sources:
https://www.tutorialspoint.com/cprogramming/c_file_io.htm
http://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/notes/Dup/lecture.html
https://stackoverflow.com/questions/762200/how-to-capture-output-of-execvp
http://www.microhowto.info/howto/capture_the_output_of_a_child_process_in_c.html
https://unix.stackexchange.com/questions/555360/bash-cat-invalid-option-r
https://www.youtube.com/watch?v=EqndHT606Tw
https://stackoverflow.com/questions/13801175/classic-c-using-pipes-in-execvp-function-stdin-and-stdout-redirection
https://stackoverflow.com/questions/22059285/adding-tokens-to-an-array-c
https://cboard.cprogramming.com/c-programming/135496-storing-tokens-into-array.html
https://stackoverflow.com/questions/8827939/handling-arguments-array-of-execvp
https://stackoverflow.com/questions/33884291/pipes-dup2-and-exec
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFSIZE 1000
#define READ_END 0		//Read file descriptor from pipe
#define WRITE_END 1		//Write file descriptor from pipe
#define STD_INPUT 0		//File descriptor for standard input
#define STD_OUTPUT 1	//File descriptor for standard output

void writeOuput(char* command, char* output) {  /*Displays output*/
	printf("The output of %s : is : \n", command);
	printf(">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);

}

char **read_command(char *text) { /*Returns array of command to use as argument in execvp*/
  int index = 0;
  char **res = NULL;
  char *command = malloc(strlen(text)+1);
  strcpy(command, text);
  char *tok = strtok(command, " "); //Space is the delimiter

  while (tok != NULL) {
    res = realloc(res, sizeof(char*) * (index+1));	//Allocate memory for next token
    char *dup = malloc(strlen(tok) +1);
    strcpy(dup, tok);
    res[index++] = dup;
    tok = strtok(NULL, " ");  //Tokenize next part
  }
  res = realloc(res, sizeof(char*) * (index+1));
  res[index]=NULL;  //NULL at the end of array
  free(command);	//Free memory
  return res;
}

int main (int argc, char *argv[]) {
  char * str;  //initialize dynamically allocated array
  pid_t pid; //First child process id
  pid_t pid2; //Second child process id
  char ** execarr;  //initialize second dynamically allocated array to hold commands

  int j = 0;
  int row=0;
  int col=0;
  int linecount=0;

  /*Shared memory object variables*/
  const int SIZE = 4096;  //Size of shared memory object
  const char *name = "OS";  //Name of shared memory object
  int fd; //Shared memory file descriptor
  char *ptr;  //Pointer to shared memory object

  /*Fork a child*/
  pid = fork();

  if (pid < 0) {
    fprintf(stderr, "Fork Failed\n");
    return 1;
  }

  //Child Process
  else if (pid == 0) {
      if (argv[1] == NULL) { //No input file was given
        printf("ERROR: Sample input file was not included\n");
        exit(1);
      }

      fd = shm_open(name, O_CREAT | O_RDWR, 0666);  //Create shared memory object
      ftruncate(fd, SIZE);  //Configure the size
      ptr = (char*)
      mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); //Memory map the shared memory object


      //READ THE SAMPLE IN FILE
      FILE *fp = fopen(argv[1], "r");
      char buff[BUFSIZE]; //Buffer to hold each line

      while(fgets(buff, BUFSIZE - 1, fp) != NULL) { /*Read file line by line*/
					 /*Write to shared memory object*/
           sprintf(ptr, "%s", buff);
           ptr += strlen(buff); /*Increment by number of bytes written*/
       }
       fclose(fp);  /*Close the file*/

  }

  else {
    wait(NULL);
    /*Read contents of shared memory object and copy commands into dynamically allocated array*/
    fd = shm_open(name, O_RDONLY, 0666);
    ptr = (char*)
    mmap(0, SIZE, PROT_READ, MAP_SHARED, fd, 0);  //Memory map the shared memory object
    str = malloc(SIZE);
    memcpy(str, ptr, strlen(ptr)+1);

    execarr = malloc(SIZE);

   /*Add a '\n' at the end of the string. Needed to create execarr properly*/
   while (str[j] != '\0') {
      j++;
    }
    str[j] = '\n';
    j=0;

    /*Count the number of lines in the input file*/
    while (str[j] != '\0') {
      if (str[j] == '\n')
        linecount++;
      j++;
    }
    j=0;


    /*Create execarr. Each element is a line from the file*/
    while(row < linecount) {
      execarr[row] = malloc(BUFSIZE);
      while (str[j] != '\n') {
        execarr[row][col] = str[j];
        j++; col++;
				if (str[j] == '\r') //skip over carriage return
					j++;
      }
      j++;  //skip over the '\n'
      row++; col=0;
    }

 		char ** buffer = malloc(SIZE); //Buffer to hold content read from pipe


		 //Start loop to execute commands
		 int n;
		 for (n=0; n<linecount; n++) {
        buffer[n] = malloc(SIZE);

			 int myPipe[2]; //Create pipe

     	/*Start pipe*/
    	 if (pipe(myPipe) == -1) {
				 fprintf(stderr, "Pipe Failed");
				 return 1;
			 }

			 /*Fork another child*/
			 pid2 = fork();

       if (pid2 < 0) { /*Fork failed*/
				 fprintf(stderr, "Fork Failed\n");
				 return 1;
			 }

			 else if (pid2 == 0) { /*Child process*/
				  char ** command = malloc(strlen(execarr[n])+1);
				  command = read_command(execarr[n]); //Create array of command & parameters

				  while(dup2(myPipe[WRITE_END], STD_OUTPUT) == -1 && (errno == EINTR)) {} //Redirect child's standard output into pipe

		 	 		close(myPipe[WRITE_END]);
       		close(myPipe[READ_END]);

			    execvp(command[0], command); //Execute command

		 			printf("execvp call did not work");
		 			return(-1);
			 }

			 else {  /*Parent process*/
				 wait(NULL);
				 while(1) {
					 close(myPipe[WRITE_END]);
					 ssize_t count = read(myPipe[READ_END], buffer[n], SIZE); //Read from pipe's read end into buffer
					 if (count==-1) {
						 if (errno == EINTR) {
							 continue; }
						 else {
							 perror("read");
							 exit(1); }
					 }

					 else if(count == 0) {
						 break; }

					 else {
						 writeOuput(execarr[n], buffer[n]); //Display output to screen
						 break; //Exit loop
					 }
				 }
				 close(myPipe[READ_END]);

      }
			//write
   	} /*End loop*/
	} /*End parent*/
  return 0;
}
