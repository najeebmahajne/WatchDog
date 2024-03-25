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

#include "scheduler.h"
#define INTERVAL 5

int to_stop=0;
size_t g_signals_counter = 0;
sched_ty *scheduler = NULL;



static void ResetCounter(int signo);
static void UpdateToStop(int signo);
static int SendSignal(void *params);
static int CheckSignals(void *params);
static void Revive(void *params);
static void Clean();
static void UnblockMask(sigset_t *block_set);
static void InitSignalListeners(struct sigaction *sa, struct sigaction *sa2);



void KeepWatching(int argc,char *argv[])
{
	struct sigaction sa, sa2;
	sigset_t block_set;

	InitSignalListeners(&sa,&sa2);

	UnblockMask(&block_set);

	/*sched init*/
	scheduler = SchedCreate();

	/*sched addtask SendSignal*/
	SchedAddTask(scheduler, SendSignal, (void *)argv , 1);

	/*sched addtask CheckCounter*/
	SchedAddTask(scheduler, CheckSignals, (void *)argv , INTERVAL);

	/*schedRun()*/
	SchedRun(scheduler);
}

void ResetCounter(int signo)
{
	(void)signo;

	printf("pid checking counter: %d\n",getpid());

	/*reset count*/
	__sync_lock_test_and_set (&g_signals_counter,0);
}

void UpdateToStop(int signo)
{
	(void)signo;

	/* update to_stop = 1*/
	__sync_lock_test_and_set (&to_stop,1);
}


int SendSignal(void *params)
{
	char** args = (char **)params;

	pid_t current_pid = getpid();
	pid_t wd_pid = atoi(getenv("WD_PID"));
	pid_t client_pid = atoi(getenv("CLIENT_PID"));

	if (wd_pid == current_pid)
	{
		/*atomic operate count*/
		printf("sender pid :%d sender to pid :%d\n",getpid() ,client_pid);

		/*increment counter*/
		__sync_fetch_and_add(&g_signals_counter,1);
		printf("Counter %lu\n", g_signals_counter);
	
		/*send signal*/
		kill(client_pid, SIGUSR1);
	
		/*return 1*/
		return 1;
	}
	else if (client_pid == current_pid)
	{
		/*atomic operate count*/
		printf("sender pid :%d sender to pid :%d\n",getpid() ,wd_pid);

		/*increment counter*/
		__sync_fetch_and_add(&g_signals_counter,1);
		printf("Counter %lu\n", g_signals_counter);
	
		/*send signal*/
		kill(wd_pid , SIGUSR1);
	
		/*return 1*/
		return 1;

	}
	
}

int CheckSignals(void *params)
{
	char** args = (char **)params;

	pid_t pid = getpid();

	printf("checking pid :%d\n",pid);

	/*if to_stop*/
	if(to_stop == 1)
	{
		/* Clean()*/
		Clean();
	}
	
	/*if counter>max_signals_until_crash*/
	if(g_signals_counter > atoi(args[4]))
	{
		__sync_lock_test_and_set (&g_signals_counter,0);

		/*revive()*/
		Revive(params);
	}

	return 1;
}

void Revive(void *params)
{
	char** args = (char **)params;

	int status;
	int new_wd_pid;
	char new_wd_pid_s[20];
	char interval[20];
	char max_signals[20];
	char ppid[20];

	assert(NULL != params);
	
	if(0 == strcmp(args[1], "run_watchdog"))
	{
		/*termenate client */
		kill(atoi(getenv("WD_PID")),SIGKILL);

		/*wait for termination*/
		waitpid(atoi(getenv("WD_PID")),&status, WUNTRACED);

		/*fork*/
		new_wd_pid = fork();
		
		if(0 == new_wd_pid )
		{
			sprintf(interval,"%d",atoi(args[3]));
			sprintf(max_signals,"%d",atoi(args[4]));
			
			args[0] = "run_watchdog";
			args[1] = "watchdog_client";

			/*execv */
			execv("run_watchdog",args);
		}
		else
		{
			/*update WD_PID */
			sprintf(new_wd_pid_s,"%d",new_wd_pid);
			setenv("WD_PID", new_wd_pid_s,1);
		}
		
	}
	else
	{
		/*termenate client */
		kill(atoi(getenv("CLIENT_PID")),SIGKILL);

		/*wait for termination*/
		waitpid(atoi(getenv("CLIENT_PID")),&status, WUNTRACED);
		new_wd_pid = fork();

		if(0  == new_wd_pid )
		{
			sprintf(interval,"%d",atoi(args[3]));
			sprintf(max_signals,"%d",atoi(args[4]));
			
			
			args[0] = "watchdog_client";
			args[1] = "run_watchdog";

			/*execv */
			execv("watchdog_client",args);
			
		}else
		{
			/*update CLIENT_PID */
			sprintf(new_wd_pid_s,"%d",new_wd_pid);
			setenv("CLIENT_PID", new_wd_pid_s,1);
		}
	}

}

void Clean()
{
	SchedStop(scheduler);
	SchedDestroy(scheduler);

	/*destroy sched & semaphores & reset env var*/
	if(-1 == sem_unlink("watchdog"))
	{
		fprintf(stderr, "%s\n","ERROR REMOVING SEMAPHORE!");
	}


	if(-1 ==unsetenv("WD_PID"))
	{
		fprintf(stderr, "%s\n","ERROR UNSETTING WD_PID ");
	}
}


void UnblockMask(sigset_t *block_set)
{
	if (-1 == sigemptyset(block_set))
	{
        perror("sigemptyset");
   
    }

    if (-1 == sigaddset(block_set, SIGUSR1)|| -1 == sigaddset(block_set, SIGUSR2))
    {
        perror("sigaddset");
    
    }


	if (-1 == pthread_sigmask(SIG_UNBLOCK, block_set, NULL))
	{
		perror("sigprocmask");
		
	}
}


void InitSignalListeners(struct sigaction *sa, struct sigaction *sa2)
{
	sa->sa_handler = ResetCounter;
    sa2->sa_handler = UpdateToStop;
    sa->sa_flags = 0;
    sa2->sa_flags = 0;

    sigaction(SIGUSR1, sa, NULL);
    sigaction(SIGUSR2, sa2, NULL);
}