
#include "dvfs.h"

int opt_disable_dvfs = 0;

const double FREQ_A7[NUM_A7_DVFS_POINTS] = {200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400};
const double VOLT_A7[NUM_A7_DVFS_POINTS] = {0.913, 0.913, 0.913, 0.913, 0.9512, 0.988, 1.026, 1.063, 1.101, 1.138, 1.176, 1.22, 1.273};
const double FREQ_A15[NUM_A15_DVFS_POINTS] = {200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900};
const double VOLT_A15[NUM_A15_DVFS_POINTS] = {0.9125, 0.9125, 0.9125, 0.9125, 0.9125, 0.9125, 0.925, 0.94, 0.973, 0.998, 1.023, 1.048, 1.062, 1.077, 1.115, 1.15375, 1.191, 1.241};

const char* xu3dvfs_a7min_path = "/sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq";
const char* xu3dvfs_a7max_path = "/sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq";
const char* xu3dvfs_a15min_path = "/sys/devices/system/cpu/cpu7/cpufreq/scaling_min_freq";
const char* xu3dvfs_a15max_path = "/sys/devices/system/cpu/cpu7/cpufreq/scaling_max_freq";

void set_xu3_dvfs(const char* path, int freq)
{
	FILE* fp;
	fp = fopen(path, "w");
	if(fp==NULL)
		return;
	fprintf(fp, "%d", freq);
	fclose(fp);
}

void apply_dvfs_req(double xu3pwr[], double req_freq_a7, double req_freq_a15)
{
	if(opt_disable_dvfs)
		return;
	int freq;
	freq = (int)(req_freq_a7*1000.0);
	if(xu3pwr[A7_F]>freq) {
		set_xu3_dvfs(xu3dvfs_a7min_path, freq);
		set_xu3_dvfs(xu3dvfs_a7max_path, freq);
	}
	else {
		set_xu3_dvfs(xu3dvfs_a7max_path, freq);
		set_xu3_dvfs(xu3dvfs_a7min_path, freq);
	}
	freq = (int)(req_freq_a15*1000.0);
	if(xu3pwr[A15_F]>freq) {
		set_xu3_dvfs(xu3dvfs_a15min_path, freq);
		set_xu3_dvfs(xu3dvfs_a15max_path, freq);
	}
	else {
		set_xu3_dvfs(xu3dvfs_a15max_path, freq);
		set_xu3_dvfs(xu3dvfs_a15min_path, freq);
	}
	if(opt_debug_info)
		printf("DVFS set to %lf (A7), %lf (A15)\n", req_freq_a7, req_freq_a15);
}
