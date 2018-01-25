#ifndef _EXEC_H_
#define _EXEC_H_

#include <stdlib.h>
#include <fcntl.h>

#include "timeutils.h"
#include "affinity.h"

#define MAX_ARGS 10

extern char* tasks[][MAX_ARGS];

int start_task(int id);
void wait_all();
void run_exec();

#endif
