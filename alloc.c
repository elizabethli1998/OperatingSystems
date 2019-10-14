/* 
Elizabeth Li
April 17th, 2019

This is our alloc.c file. This file is intended to allocate resources that are grabbed from our res.txt file. It puts the contents
of the file into a mmap. It also prompts user to allocate resources continuously, until the user says "no" or if there are no
more resources available from the requested type or if the requested type is not available. Once the user says "no", it ends the process.
Once requested and deemed okay to be requested, we can subtract the requested from what we have and update both the file and the
mmap contents.
This is designed to work alongside the prov-rep.c and works with mutual exclusion with the usage of semaphores.
*/
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

//Setting up the semun for the semaphore nonsense
union semun {
            int val;
            struct semid_ds *buf;
            unsigned short  *array;
} arg;
//Main method, AKA our only function that and returns 0. The parameters are for command-line arguments
int main(int argc, char ** argv) 
{
  
  int size;
  char resType;
  int requestAmount;
  char allocateResources = 'y';
  //Gets the file and gives read/write permissions
  int fd = open("res.txt", O_RDWR);
  struct stat s;
    //Semaphore
  int sem;
  union semun sem_val;
  struct sembuf sem_op;
  sem = semget(6666, 1, IPC_CREAT | 0666);
  sem_val.val = 1;
  semctl(sem, 0, SETVAL, sem_val);

  if (fstat(fd, & s) == -1) 
  {
    perror("Can't get file size.\n");
  }
//Get file size
  printf("File size is %ld\n", s.st_size);
  size = s.st_size;

//maps our file to a char array
  char * file_in_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (file_in_memory == MAP_FAILED) 
  {
    printf("Failed file in memory.\n");
    exit(0);
  }

  while (1) 
  {
  	//prompts you to add more resources by a y or n.
  	  printf("Would you like to allocate more resources? y/n \n");
      scanf("%s", &allocateResources);
      getchar();
      
     if (allocateResources == 'y') 
      {
      	//If we choose to allocate resources, we lock it for mutual exclusion
   		 sem_op.sem_num = 0;
		 sem_op.sem_op = -1;
		 sem_op.sem_flg = 0;
		 semop(sem, &sem_op, 1);
  		
  		//Asks you which resource type and how many resources for that type
  		 printf("What resource type and how much of that type?\n");
     	 scanf("%c %d", &resType, &requestAmount);

    	//update the text file and mmap
    	for (int i = 0; i < size; i += 4) 
    	{
    		//If the requested type matches what we have
      		if (resType == file_in_memory[i]) 
      		{
        		printf("This type exists. \n");

				//If we have enough resources to be requested, subtract from resources
        		if (requestAmount <= (file_in_memory[i + 2] - '0')) 
       		 	{
            		file_in_memory[i + 2] = (file_in_memory[i + 2] - '0') - requestAmount + '0';
        		} 
        		//Game over
        	    else 
        		{
          			printf("Not enough resources for this request! Please try again.\n");
       		 	}
      		}
    	}
    	//If successful, will sync the file
       	if (msync(file_in_memory, size, MS_SYNC) < 0) 
         {
            perror("msync failed with error: \n");
            return -1;
         }
    	 //Unlock, since we are done with the file
    	  sem_op.sem_num = 0;
	      sem_op.sem_op = 1;
	      sem_op.sem_flg = 0;
	      semop(sem, &sem_op, 1);
		}
		//If we aren't allocating anymore resources, exit out
    	if (allocateResources == 'n')
    	{
    		printf("Ok bye.\n");
    		exit(0);
    	}
  }
  munmap(file_in_memory, size);
  close(fd);
  return 0;
}