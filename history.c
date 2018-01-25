
#include "history.h"

power_model pwr;
perf_model perf[MAX_TASKS];

void init_history(unsigned long time)
{
	int i, j, t;
	pwr.a = 0.0;
	pwr.b = 0.0;
	pwr.c = 0.0;
	for(i=0; i<TABLE_SIZE; i++)
		for(j=0; j<TABLE_SIZE; j++) {
			pwr.time[i][j] = 0;
			pwr.power[i][j] = -1.0;
			pwr.fv2_a7[i][j] = -1.0;
			pwr.fv2_a15[i][j] = -1.0;
		}
		
	for(t=0; t<MAX_TASKS; t++) {
		perf[t].task_id = t;
		perf[t].a = 0.0;
		perf[t].b = 0.0;
		perf[t].c = 0.0;
		for(i=0; i<TABLE_SIZE; i++)
			for(j=0; j<TABLE_SIZE; j++) {
				if(i==0 && j==0) { // point [0][0] is fixed to IPC==0
					perf[t].time[i][j] = time;
					perf[t].perf[i][j] = 0.0;
				}
				else {
					perf[t].time[i][j] = 0;
					perf[t].perf[i][j] = -1.0;
				}
			}
	}
}

void add_power(unsigned long time, power_model* m, int num_a7, int num_a15, double power, double fv2_a7, double fv2_a15)
{
	m->time[num_a7][num_a15] = time;
	m->power[num_a7][num_a15] = power;
	m->fv2_a7[num_a7][num_a15] = fv2_a7;
	m->fv2_a15[num_a7][num_a15] = fv2_a15;
}

void add_perf(unsigned long time, perf_model* m, int num_a7, int num_a15, double perf)
{
	m->time[num_a7][num_a15] = time;
	m->perf[num_a7][num_a15] = perf;
}

int find_oldest(unsigned long time[TABLE_SIZE][TABLE_SIZE], int* out_i, int* out_j)
{
	unsigned long min = 0;
	int i, j;
	for(i=0; i<TABLE_SIZE; i++)
		for(j=0; j<TABLE_SIZE; j++) {
			if(time[i][j]==0)
				continue;
			if(min==0 || time[i][j]<min) {
				min = time[i][j];
				*out_i = i;
				*out_j = j;
			}
		}
	return min>0;
}

void forget_oldest_power(power_model* m)
{
	int i, j;
	if(find_oldest(m->time, &i, &j)) {
		m->time[i][j] = 0;
		m->power[i][j] = -1.0;
		m->fv2_a7[i][j] = -1.0;
		m->fv2_a15[i][j] = -1.0;
	}
}

void forget_oldest_perf(perf_model* m)
{
	int i, j;
	if(find_oldest(m->time, &i, &j)) {
		m->time[i][j] = 0;
		m->perf[i][j] = -1.0;
	}
}

