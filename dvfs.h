#ifndef _DVFS_H_
#define _DVFS_H_

#include "monitors.h"

extern int opt_disable_dvfs;

#define NUM_A7_DVFS_POINTS 13
#define NUM_A15_DVFS_POINTS 18
extern const double FREQ_A7[NUM_A7_DVFS_POINTS];
extern const double VOLT_A7[NUM_A7_DVFS_POINTS];
extern const double FREQ_A15[NUM_A15_DVFS_POINTS];
extern const double VOLT_A15[NUM_A15_DVFS_POINTS];

void apply_dvfs_req(double xu3pwr[], double req_freq_a7, double req_freq_a15);

#endif