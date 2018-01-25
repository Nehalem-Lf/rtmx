
#include "monitors.h"

int opt_mon_period = 500;

#define NUM_XU3POWER_FLAGS 4
const char* xu3power_flag_paths[NUM_XU3POWER_FLAGS] = {
	"/sys/bus/i2c/drivers/INA231/3-0045/enable",
	"/sys/bus/i2c/drivers/INA231/3-0040/enable",
	"/sys/bus/i2c/drivers/INA231/3-0041/enable",
	"/sys/bus/i2c/drivers/INA231/3-0044/enable",
};

const char* xu3power_param_paths[NUM_XU3POWER_PARAMS] = {
	"/sys/bus/i2c/drivers/INA231/3-0045/sensor_V", // A7 V
	"/sys/bus/i2c/drivers/INA231/3-0045/sensor_A",
	"/sys/bus/i2c/drivers/INA231/3-0045/sensor_W",
	"/sys/bus/i2c/drivers/INA231/3-0040/sensor_V", // A15 V
	"/sys/bus/i2c/drivers/INA231/3-0040/sensor_A",
	"/sys/bus/i2c/drivers/INA231/3-0040/sensor_W",
	"/sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq",
	"/sys/devices/system/cpu/cpu7/cpufreq/scaling_cur_freq",
};

struct monitor_info monitors_tinfo[NUM_CORES];

// XU3 POWER ------------------------------

int read_xu3power(int param, const char* fmt, void* ptr)
{
	FILE* fp;
	fp = fopen(xu3power_param_paths[param], "r");
	if(fp==NULL) {
		return 0;
	}
	if(fscanf(fp, fmt, ptr)!=1) {
		fclose(fp);
		return 0;
	}
	else {
		fclose(fp);
		return 1;
	}
}

int read_xu3power_all(double ptr[])
{
	int i, res = 1;
	for(i=0; i<NUM_XU3POWER_PARAMS; i++) {
		res &= read_xu3power(i, "%lf", &ptr[i]);
	}
	return res;
}

void log_xu3power_all(FILE* log, unsigned long time, double ptr[])
{
	if(log) {
		fprintf(log, "%lu", time);
		int i;
		for(i=0; i<NUM_XU3POWER_PARAMS; i++) {
			fprintf(log, "\t%lf", ptr[i]);
		}
		fprintf(log, "\n");
	}
}

void set_xu3power_flag(const char* path, const char* flag)
{
	FILE* fp;
	fp = fopen(path, "w");
	if(fp==NULL)
		return;
	fputs(flag, fp);
	fclose(fp);
}

void init_xu3power()
{
	int i;
	for(i=0; i<NUM_XU3POWER_FLAGS; i++) {
		set_xu3power_flag(xu3power_flag_paths[i], "1");
	}
	sleep_ms(2000);
}

// ARM PMU -------------------------------

#include "pmu.h"

// CORE MONITOR THREADS -------------------------------

static void* monitor_proc(void *arg)
{
	int i;
	int num = ((monitor_info*) arg)->num;
	FILE* log = open_log_index("mon", num);
	if(log) fprintf(log, "time\tunhalt_cycles\tinst_retired\tmem_access\n");
	printf("mon%d: started\n", num);
	
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(num, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);
	
	long mon_time, prev_time;
	unsigned int mon_unhalt_cycles, mon_inst_retired, mon_mem_access;
	unsigned int prev_unhalt_cycles, prev_inst_retired, prev_mem_access;
	int first = 1;
	
	init_perf_start();
	init_perf(0, 0x08 /* INST_RETIRED */);
	init_perf(1, 0x13 /* MEM_ACCESS */);
	
	for(i=0;; i++) {
		sleep_ms(opt_mon_period);
		
		// collect monitor data
		mon_time = (long)timestamp();
		mon_unhalt_cycles = (int)get_cyclecnt();
		get_evt(0, &mon_inst_retired);
		get_evt(1, &mon_mem_access);
		
		if(!first) {
			((monitor_info*) arg)->mon_time = mon_time - prev_time;
			((monitor_info*) arg)->mon_unhalt_cycles = mon_unhalt_cycles - prev_unhalt_cycles;
			((monitor_info*) arg)->mon_inst_retired = mon_inst_retired - prev_inst_retired;
			((monitor_info*) arg)->mon_mem_access = mon_mem_access - prev_mem_access;
		
			// log data
			if(log) fprintf(log, "%lu\t%d\t%d\t%d\n",
					((monitor_info*) arg)->mon_time,
					((monitor_info*) arg)->mon_unhalt_cycles,
					((monitor_info*) arg)->mon_inst_retired,
					((monitor_info*) arg)->mon_mem_access
				);
		}
		
		prev_time = mon_time;
		prev_unhalt_cycles = mon_unhalt_cycles;
		prev_inst_retired = mon_inst_retired;
		prev_mem_access = mon_mem_access;
		first = 0;
	}
}

void start_core_monitors()
{
	int tnum, err;
	pthread_attr_t attr;
	cpu_set_t cpus;
	
	pthread_attr_init(&attr);
	for(tnum = 0; tnum < NUM_CORES; tnum++) {
		monitors_tinfo[tnum].num = tnum;
		
		// set affinity
		CPU_ZERO(&cpus);
		CPU_SET(tnum, &cpus);
		err = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		if(err)
			printf("*error(%d): pthread_attr_setaffinity_np, %d\n", __LINE__, err);
			
		// start thread
		err = pthread_create(&monitors_tinfo[tnum].thread_id, &attr, monitor_proc, &monitors_tinfo[tnum]);
		if(err)
			printf("*error(%d): pthread_create, %d\n", __LINE__, err);
	}
}

void stop_core_monitors()
{
	int tnum;
	for(tnum = 0; tnum < NUM_CORES; tnum++) {
		pthread_cancel(monitors_tinfo[tnum].thread_id);
	}
}
