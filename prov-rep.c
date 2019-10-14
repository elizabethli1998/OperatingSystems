
/* 
Elizabeth Li
April 17th, 2019

This is our prov-rep.c file. This file is intended to add resources that are grabbed from our res.txt file. It puts the contents
of the file into a mmap. It also prompts user to add resources continuously, until the user says "no" or if 
the amount of resources remains 9 or lower for the requested type or if the requested type is not available. Once the user says "no", it ends the process.
Once requested and deemed okay to be requested, we can add the requested to what we have and update both the file and the
mmap contents. This is in our PARENT process.

We have a child process that reports three different things every 10 seconds. It gets the page size of the system, calling
getpagesize(), current state of resources, and status of pages in memory region using mincore(). Since the mincore aspect
works with accessing the file, it will be under mutual exclusion along with the parent.

This is designed to work alongside the alloc.c and works with mutual exclusion with the usage of semaphores.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <sys/ipc.h>   
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
//Setting up the semun for the semaphore nonsense
union semun {
            int val;
            struct semid_ds *buf;
            unsigned short  *array;
} arg;
//Main method, AKA our only function that and returns 0. The parameters are for command-line arguments
int main() 
{
  const char * mapped;
  int size;
  char resType;
  char typeNumber;
  int requestAmount;
  char addResources;
   //Gets the file and gives read/write permissions
  int fd = open("res.txt", O_RDWR);
  pid_t child_pid = -1;
  unsigned char * vec;
 
  //Semaphore
  int sem;
  union semun sem_val;
  struct sembuf sem_op;
  sem = semget(6666, 1, IPC_CREAT | 0666);
  sem_val.val = 1;
  semctl(sem, 0, SETVAL, sem_val);
  
  struct stat s;
  const size_t ps = sysconf(_SC_PAGESIZE);

  if (fstat(fd, & s) == -1) 
  {
    perror("Can't get file size.\n");
  }
//Get file size
  printf("File size is %ld\n", s.st_size);
  size = s.st_size;
//maps our file to a char array
  char * file_in_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  vec = calloc(1, (size + ps - 1) / ps);

  if (file_in_memory == MAP_FAILED) 
  {
    printf("Failed file in memory.\n");
    exit(0);
  }
  for (int i = 0; i < size; i++) 
  {
    printf("%c", file_in_memory[i]);
  }
  printf("\n");

//Creates child and parent process
    child_pid = fork();
    
//parent process
  if(child_pid > 0)
  {  
    while (1) 
    {
    	//prompts you to add more resources by a y or n.
      printf("Would you like to allocate more resources? y/n \n");
      scanf("%s", &addResources);
      getchar();
      
      if (addResources == 'y') 
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
       	 //If our total consisting of amount requested and amount we have doesn't exceed 9, add t0 resources
          	if (requestAmount + (file_in_memory[i + 2] - '0') < 10) 
          	{
            	file_in_memory[i + 2] = (file_in_memory[i + 2] - '0') + requestAmount + '0';
          	} 
          		//Game over
          	else 
          	{
            	printf("You are requesting too many resources! RIP.\n");
          	}
         }
        }		
        //If successful, will sync the file
      	 if (msync(file_in_memory, size, MS_SYNC) < 0) 
      	 {
        		perror("msync failed with error: ");
        		return -1;
      	 }
      	  //Unlock, since we are done with the file
     	 sem_op.sem_num = 0;
      	 sem_op.sem_op = 1;
	  	 sem_op.sem_flg = 0;
	     semop(sem, &sem_op, 1);
	  }
	  //If we aren't allocating anymore resources, exit out
      if (addResources == 'n') 
      {
            exit(0);
      }
    }
  }
  //Entering child process
  else if(child_pid == 0)
  { 
   	  while (1) 
   	  {
   	  	//gets page size
     	 printf("The page size: %d\n", getpagesize());
     	 //Locks because we are accessing the file in memory
         sem_op.sem_num = 0;
	     sem_op.sem_op = -1;
	     sem_op.sem_flg = 0;
	     semop(sem, &sem_op, 1);
	     
      	  for (int i = 0; i < size; i++) 
      	  {
   			 printf("%c", file_in_memory[i]);
 		  }
 		  printf("\n");
      	 //Unlock since we are done
        sem_op.sem_num = 0;
	    sem_op.sem_op = 1;
	    sem_op.sem_flg = 0;
	    semop(sem, &sem_op, 1);
        
        //Gets status of pages in memory
         mincore(file_in_memory, size, vec);
       //Checks for residency in core in memory.
       	for(int i = 0; i <= size/ps; i++)
       	{
       		if(vec[i] & 1)
       		{
       			printf("Resident.\n");
       		}
       		else
        	{
         		printf("Non-Resident.\n");
         	}
       	} 
       	//Reports every 10 seconds
         sleep(10);
      }

  }
  free(vec);
  munmap(file_in_memory, size);
  close(fd);
  exit(0);
  return 0;

}