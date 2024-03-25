/*******************************************************************************
Project: Watchdog
Name: Najeeb
Reviewer: ***
Date: 14.1.2024
Version: 1.0
*******************************************************************************/

#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include <stddef.h> /*size_t*/

/*
	Description: Run a watchdog to keep track of the current process of not crashing,
				 if the current process crashes then watchdog will re-execute the current process,
				 making sure it keeps running

	Input:  program_name - program to run
			interval - send signals every interval seconds
			max_signals_until_crash - watchdog will re-execute program after max_signals_until_crash passed with no received signal

	Return: 0 - success, otherwise - fail
*/
int MakeMeImmortal(char *argv[],size_t argc, size_t interval_, size_t max_failures_);


/*
	Description: Stop watchdog from watching the program, and clean up

	Return: 0 - success, otherwise - fail
*/
int StopWD(void);



#endif /*__WATCHDOG_H__*/