#ifndef _AFFINITY_H_
#define _AFFINITY_H_

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h> // compile with -pthread

extern int opt_debug_info;
extern int opt_disable_affinity;

#include "timeutils.h"
#include "log.h"

#define MAX_TASKS 10
#define NUM_CORES 8
#define NUM_A7 4
#define NUM_A15 4
#define A7 0
#define A15 4

typedef struct app_info {
	int task_id;
	pid_t pid;
	unsigned long start_time;
	unsigned long end_time;
	int running;
	int ncores, ncores_a7, ncores_a15;
	int cores[NUM_CORES];
} app_info;

extern const int core_names[NUM_CORES];
extern const int core_types[NUM_CORES];

extern app_info apps_info[MAX_TASKS];
extern int napps;
extern int ncores_a7, ncores_a15;
extern int next_unused_core;
extern int system_stable;

void init_affinity();
int get_unused_core();
void set_app_affinity(app_info* app, int ncores, int cores[], int use_stability);
void create_app_info(int task_id, pid_t pid);
void update_global_app_status(unsigned long time);
void apply_affinity_req(int req_a7[], int req_a15[]);
void log_apps_info(unsigned long time);
void close_app_logs(unsigned long time);

#endif
