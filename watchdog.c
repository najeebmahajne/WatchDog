#define _POSIX_C_SOURCE  200112L

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
#include <pthread.h>

#include "watchdog.h"
#include "keepwatching.h"

#define SIZE 20
   
typedef struct 
{
	pid_t pid;
	pid_t ppid;
	char program_name[SIZE];
	size_t interval; 
	size_t max_signals_until_crash;
	char **argv;
	int argc;

}watchdogclient_ty;


sem_t *sem = NULL;
pthread_t thread;
struct timespec timeout;
pid_t new_wd_pid = 0;


static void *KeepWatchingHelper(void *params);
static void BlockMask(sigset_t *block_set);

int MakeMeImmortal(char *argv[],size_t argc, size_t interval_, size_t max_failures_)
{
	sigset_t block_set;
	char new_wd_pid_s[SIZE];
	char *env =NULL;
	char interval[SIZE];
	char max_signals[SIZE];
	char ppid[SIZE];
	char pid[SIZE];
	watchdogclient_ty *wd = NULL;
	int args_size = argc + 6;
	char client_pid_str[SIZE];
	


	char **args = (char **)malloc(args_size * sizeof(char *));
	if (args == NULL)
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	
	BlockMask(&block_set);

	sprintf(client_pid_str, "%d", getpid());
    setenv("CLIENT_PID", client_pid_str, 1);

	/*init semaphore*/
	sem=sem_open("watchdog", O_CREAT , 0666, 0);

	env=getenv("WD_PID");

	/*check if WD_PID is NULL*/ 
	if (NULL == env)
	{
		/* fork */
		new_wd_pid = fork();

		/*handle fork failure */
		if (-1 == new_wd_pid)
		{
			perror("Fork failed");
			return -1;
		}

		/*if child*/
		if (0 == new_wd_pid)
		{
			sprintf(interval,"%lu",interval_);
			sprintf(max_signals,"%lu",max_failures_);
			sprintf(ppid,"%d",getppid());
			sprintf(pid,"%d",getpid());

			args[0] = "run_watchdog";
			args[1] = "watchdog_client";
			args[2] = pid;
			args[3] = interval;
			args[4] = max_signals;
			args[5] = ppid;
			args[6] = NULL; 
			
			/*execv */
			execvp("run_watchdog", args);

		}
		else
		{
			/*update WD_PID */
			sprintf(new_wd_pid_s,"%d",new_wd_pid);
			setenv("WD_PID", new_wd_pid_s,1);

		}

		clock_gettime(CLOCK_REALTIME, &timeout);
		timeout.tv_sec += max_failures_;

		/*sem_timed_wait()*/
		if (-1 == sem_timedwait(sem, &timeout))
		{
			perror("sem wait failed");
		}
	}

	
	wd = (watchdogclient_ty *)malloc(sizeof(watchdogclient_ty));
	wd->argv = (char **)malloc(7 * sizeof(char *));

	if (wd->argv != NULL)
	{
		for (int i = 0; i < 7; ++i)
		{
			wd->argv[i] = (char *)malloc(SIZE * sizeof(char));
		}

		sprintf(ppid, "%d", getppid());
		sprintf(interval, "%lu", interval_);
		sprintf(max_signals, "%lu", max_failures_);
		sprintf(pid, "%d", new_wd_pid);

		strcpy(wd->argv[0], "watchdog_client");
		strcpy(wd->argv[1], "run_watchdog");
		strcpy(wd->argv[2], ppid);
		strcpy(wd->argv[3], interval);
		strcpy(wd->argv[4], max_signals);
		strcpy(wd->argv[5], pid);
		wd->argv[6] = NULL;

	}
	wd->argc=argc;

	/*pthread_create(,keepwatching)*/
	pthread_create(&thread, NULL, KeepWatchingHelper,wd);

	sleep(100);
	
	/*return success*/
	return 1;
}

int StopWD(void)
{
	char* env = NULL;

	env=getenv("WD_PID");
	
	/*Send SIGUSR2 TO WATCHDOG*/
	kill(atoi(env), SIGUSR2);

	/*sem_timedwait(sem, &timeout);*/

	/*pthread_join(thread_id, &thread_result) != 0*/
	pthread_join(thread, NULL);

	/*return */
	return 1;
}

void *KeepWatchingHelper(void *params)
{
	watchdogclient_ty *wd = (watchdogclient_ty *)params;
	
	KeepWatching(wd->argc, wd->argv);

	return NULL;

}


void BlockMask(sigset_t *block_set)
{
	if (sigemptyset(block_set) == -1)
	{
		perror("sigemptyset");
	}

	if (sigaddset(block_set, SIGUSR1) == -1 || sigaddset(block_set, SIGUSR2) == -1)
	{
		perror("sigaddset");
	}


	if (-1 == pthread_sigmask(SIG_BLOCK, block_set, NULL))
	{
		perror("sigprocmask");
	}
}
