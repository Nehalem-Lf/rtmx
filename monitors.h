#ifndef _MONITORS_H
#define _MONITORS_H

#define _GNU_SOURCE
#include <string.h>

#include "timeutils.h"
#include "affinity.h"

extern int opt_mon_period;

typedef struct monitor_info {
	pthread_t thread_id;
	int num;
	
	// monitored values
	long mon_time;
	int mon_unhalt_cycles;
	int mon_inst_retired;
	int mon_mem_access;
} monitor_info;

extern struct monitor_info monitors_tinfo[];

#define NUM_XU3POWER_PARAMS 8
#define A7_V 0
#define A7_A 1
#define A7_W 2
#define A7_F 6
#define A15_V 3
#define A15_A 4
#define A15_W 5
#define A15_F 7

int read_xu3power(int param, const char* fmt, void* ptr);
int read_xu3power_all(double* ptr);
void log_xu3power_all(FILE* log, unsigned long time, double* ptr);
void init_xu3power();

void start_core_monitors();
void stop_core_monitors();


#endif