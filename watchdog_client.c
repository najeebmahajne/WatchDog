#define _POSIX_C_SOURCE 200112L

#include <stdio.h>/*sprintf*/
#include <sys/wait.h>/*pid_t*/
#include <semaphore.h>/*semaphore*/
#include <stdlib.h>/*getenv*/
#include <time.h>/*getclock*/
#include <sys/types.h>/*fork*/
#include <unistd.h>/*fork*/
#include <stddef.h> /*size_t*/
#include <assert.h> /*assert*/
#include <signal.h> /*signal*/
#include <fcntl.h> /*O_CREAT*/
#include <string.h>/*strcpy*/
#include <signal.h>/*signal*/
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include "watchdog.h"

#define INTERVAL 5
#define MAX_SIGNALS 5



int main(int argc, char *argv[])
{	
	(void)argc;

	/*MakeMeImmortal(char *program_name_, size_t interval_, size_t max_signals_until_crash_)*/
	MakeMeImmortal(argv,argc,INTERVAL, MAX_SIGNALS);


	/*StopWD()*/
	StopWD();

	return 0;
}
