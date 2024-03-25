#define _POSIX_C_SOURCE 200112L

#include <stddef.h>/*size_t*/
#include <sys/wait.h>/*pid_t*/
#include <assert.h>/*assert*/
#include <semaphore.h>/*semaphore*/
#include <stdio.h>/*PRINTF*/
#include <stdlib.h>/*atoi*/
#include <sys/types.h>
#include <unistd.h>
#include "keepwatching.h"

void RunWatchDog(int argc,char *argv[])
{
	sem_t *sem = NULL;
	char pid[20];
	int i = 0;
	
	/*sem open*/
	sem = sem_open("watchdog", 0);
	if (SEM_FAILED == sem)
	{
    	perror("sem_open");
    	exit(EXIT_FAILURE);
	}

	/*sem_post()*/
	sem_post(sem);

	sprintf(pid,"%d",getpid());

	if (0 != setenv("WD_PID", pid, 1))
	{
	    perror("setenv");
	    exit(EXIT_FAILURE);
	}


	/*keepwatching(pid ,program_name_,interval_,max_signals_until_crash_);*/
	KeepWatching(argc,argv);

}

int main(int argc, char *argv[])
{

	RunWatchDog(argc,argv);

	return 0;
}
