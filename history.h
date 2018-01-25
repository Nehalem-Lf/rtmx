#ifndef _HISTORY_H_
#define _HISTORY_H_

#include "affinity.h"

#define TABLE_SIZE (((NUM_A7>=NUM_A15) ? NUM_A7 : NUM_A15)+1)

typedef struct power_model {
	unsigned long time[TABLE_SIZE][TABLE_SIZE];
	double power[TABLE_SIZE][TABLE_SIZE];
	double fv2_a7[TABLE_SIZE][TABLE_SIZE];
	double fv2_a15[TABLE_SIZE][TABLE_SIZE];
	double a, b, c; // fitting result
} power_model;

extern power_model pwr;

typedef struct perf_model {
	int task_id;
	unsigned long time[TABLE_SIZE][TABLE_SIZE];
	double perf[TABLE_SIZE][TABLE_SIZE];
	double a, b, c; // fitting result
} perf_model;

extern perf_model perf[MAX_TASKS];

void init_history();
void add_power(unsigned long time, power_model* m, int num_a7, int num_a15, double power, double fv2_a7, double fv2_a15);
void add_perf(unsigned long time, perf_model* m, int num_a7, int num_a15, double perf);
void forget_oldest_power(power_model* m);
void forget_oldest_perf(perf_model* m);

#endif
