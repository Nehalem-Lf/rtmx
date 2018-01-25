
#define DEBUG_INFO 1

#include "timeutils.h"
#include "affinity.h"
#include "dvfs.h"
#include "monitors.h"
#include "exec.h"
#include "history.h"
#include "mvlr.h"
#include "itercores.h"

#define RTM_NONE 0
#define RTM_MVLR_PNP 1
#define RTM_CLASSIFY 2

int opt_do_mvlr = 1;
int opt_rtm_alg = RTM_MVLR_PNP;

#define MINIMIZE -1.0
#define MAXIMIZE 1.0

pthread_t rtm_thread_id = -1;
int req_stop_rtm = 0;

double cycle_time;
double mon_xu3power[NUM_XU3POWER_PARAMS];

int total_req_a7, total_req_a15;
int req_a7[MAX_TASKS];
int req_a15[MAX_TASKS];
double cur_freq_a7, cur_freq_a15, cur_fv2_a7, cur_fv2_a15;
double req_freq_a7, req_freq_a15;

FILE* control_log = NULL;
int opt_control_log = 1;

// MVLR RTM ---------------------------------

typedef struct class_condition {
	int metric;
	double min, max;
} class_condition;

#define NUM_CLASSES 4
#define CONDITIONS_PER_CLASS 1

#define NUM_APP_METRICS 10
#define APP_METRIC_NONE -1
#define APP_METRIC_IPCN_TOTAL 0
#define APP_METRIC_IPCN_PERCORE 1
#define APP_METRIC_IPCREF_TOTAL 2
#define APP_METRIC_IPCREF_PERCORE 3
#define APP_METRIC_IPCNMINUSMEM_TOTAL 4
#define APP_METRIC_IPCNMINUSMEM_PERCORE 5
#define APP_METRIC_CPURATIO_TOTAL 6
#define APP_METRIC_CPURATIO_PERCORE 7
#define APP_METRIC_UCLKRATIO_TOTAL 8
#define APP_METRIC_UCLKRATIO_PERCORE 9

#define DECISION_NONE 0
#define DECISION_MIN 1
#define DECISION_MAX 2

#define MAX_IPC_A7 0.8
#define MAX_IPC_A15 2.0

const struct class_condition classification[NUM_CLASSES][CONDITIONS_PER_CLASS] = {
	{{APP_METRIC_UCLKRATIO_TOTAL, 0.0, 0.11}}, // 0: low activity
	{{APP_METRIC_IPCNMINUSMEM_PERCORE , 0.3, 1000.0}}, // 1: cpu
	{{APP_METRIC_IPCNMINUSMEM_PERCORE , 0.25, 0.3}}, // 2: cpu+mem
	{{APP_METRIC_IPCNMINUSMEM_PERCORE , 0.0, 0.25} }// 3: mem
};

// the last decision is for unclassifiable apps
const int class_decision_freq[NUM_CLASSES+1] = {
	DECISION_MIN, DECISION_MAX, DECISION_MIN, DECISION_MIN, DECISION_MIN
};
const int class_decision_a7cores[NUM_CLASSES+1] = {
	DECISION_MIN, DECISION_NONE, DECISION_MAX, DECISION_MIN, DECISION_MIN
};
const int class_decision_a15cores[NUM_CLASSES+1] = {
	DECISION_NONE, DECISION_MAX, DECISION_MAX, DECISION_NONE, DECISION_NONE
};

void calc_app_metrics(app_info *info, double m[]) {
	int c;
	double total_ipc = 0.0;
	double total_mpc = 0.0;
	double total_ipcref = 0.0;
	double total_cpuratio = 0.0;
	double total_uclkratio = 0.0;
	for(c=0; c<info->ncores; c++) {
		int tnum = core_names[info->cores[c]];
		double inst_ret = monitors_tinfo[tnum].mon_inst_retired;
		double mem = monitors_tinfo[tnum].mon_mem_access;
		double cycles = monitors_tinfo[tnum].mon_unhalt_cycles;
		double freq = core_types[info->cores[c]]==A7 ? mon_xu3power[A7_F]*1000.0 : mon_xu3power[A15_F]*1000.0;
		double ipc_max = core_types[info->cores[c]]==A7 ? MAX_IPC_A7 : MAX_IPC_A15;
		if(cycles!=0) {
			total_ipc += inst_ret/cycles/ipc_max;
			total_mpc += mem/cycles/ipc_max;
		}
		if(cycle_time!=0 && freq!=0) {
			total_ipcref += inst_ret/cycle_time/freq;
			total_uclkratio += cycles/cycle_time/freq;
		}
		if(inst_ret!=0) {
			total_cpuratio += (inst_ret-mem) / inst_ret;
		}
	}

	m[APP_METRIC_IPCN_TOTAL] = total_ipc;
	m[APP_METRIC_IPCN_PERCORE] = total_ipc / (double) info->ncores;
	m[APP_METRIC_IPCREF_TOTAL] = total_ipcref;
	m[APP_METRIC_IPCREF_PERCORE] = total_ipcref / (double) info->ncores;
	m[APP_METRIC_IPCNMINUSMEM_TOTAL] = total_ipc - total_mpc;
	m[APP_METRIC_IPCNMINUSMEM_PERCORE] = (total_ipc - total_mpc) / (double) info->ncores;
	m[APP_METRIC_CPURATIO_TOTAL] = total_cpuratio;
	m[APP_METRIC_CPURATIO_PERCORE] = total_cpuratio / (double) info->ncores;
	m[APP_METRIC_UCLKRATIO_TOTAL] = total_uclkratio;
	m[APP_METRIC_UCLKRATIO_PERCORE] = total_uclkratio / (double) info->ncores;
}

void rtm_control_classify(unsigned long time) {
	int app, cls, i;
	double metrics[NUM_APP_METRICS];
	int d_freq[MAX_TASKS];
	int d_a7cores[MAX_TASKS];
	int d_a15cores[MAX_TASKS];
	int num_a7min = 0;
	int num_a7max = 0;
	int num_a15min = 0;
	int num_a15max = 0;
	for(app=0; app<napps; app++) {
		if(apps_info[app].running) {
			calc_app_metrics(&apps_info[app], metrics);
			for(cls=0; cls<NUM_CLASSES; cls++) {
				int match = 1;
				for(i=0; i<CONDITIONS_PER_CLASS; i++) {
					if(classification[cls][i].metric!=APP_METRIC_NONE &&
							!(metrics[classification[cls][i].metric]>=classification[cls][i].min &&
							metrics[classification[cls][i].metric]<=classification[cls][i].max)) {
						match = 0;
						break;
					}
				}
				if(match)
					break;
			}
			d_freq[app] = class_decision_freq[cls];
			d_a7cores[app] = class_decision_a7cores[cls];
			if(d_a7cores[app]==DECISION_MIN)
				num_a7min++;
			else if(d_a7cores[app]==DECISION_MAX)
				num_a7max++;
			d_a15cores[app] = class_decision_a15cores[cls];
			if(d_a15cores[app]==DECISION_MIN)
				num_a15min++;
			else if(d_a15cores[app]==DECISION_MAX)
				num_a15max++;
			if(opt_debug_info) {
				printf("%d per core:\tipc=%lf\tipcref=%lf\tipc-mem=%lf\tcpu=%lf\tuclk=%lf\tclass: %d\n", app,
					metrics[APP_METRIC_IPCN_PERCORE], metrics[APP_METRIC_IPCREF_PERCORE],
					metrics[APP_METRIC_IPCNMINUSMEM_PERCORE], metrics[APP_METRIC_CPURATIO_PERCORE],
					metrics[APP_METRIC_UCLKRATIO_PERCORE], cls);
				printf("%d total:\tipc=%lf\tipcref=%lf\tipc-mem=%lf\tcpu=%lf\tuclk=%lf\tclass: %d\n", app,
					metrics[APP_METRIC_IPCN_TOTAL], metrics[APP_METRIC_IPCREF_TOTAL],
					metrics[APP_METRIC_IPCNMINUSMEM_TOTAL], metrics[APP_METRIC_CPURATIO_TOTAL],
					metrics[APP_METRIC_UCLKRATIO_TOTAL], cls);
				printf("Decision: %d/%d/%d\n", d_freq[app], d_a7cores[app], d_a15cores[app]);
			}
			if(opt_control_log && !control_log) {
				control_log = open_log("control");
				if(!control_log)
					opt_control_log = 0l;
				else
					fprintf(control_log, "time\tpid\tclass\tipc_core\tipcref_core\tipcnmem_core\tcpu_core\tuclk_core\tipc_all\tipcref_all\tipcnmem_all\tcpu_all\tuclk_all\t\n");
						
			}
			if(opt_control_log) {
				fprintf(control_log, "%lu\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t\n", time, apps_info[app].pid, cls,
					metrics[APP_METRIC_IPCN_PERCORE], metrics[APP_METRIC_IPCREF_PERCORE],
					metrics[APP_METRIC_IPCNMINUSMEM_PERCORE], metrics[APP_METRIC_CPURATIO_PERCORE],
					metrics[APP_METRIC_UCLKRATIO_PERCORE],
					metrics[APP_METRIC_IPCN_TOTAL], metrics[APP_METRIC_IPCREF_TOTAL],
					metrics[APP_METRIC_IPCNMINUSMEM_TOTAL], metrics[APP_METRIC_CPURATIO_TOTAL],
					metrics[APP_METRIC_UCLKRATIO_TOTAL]
				);
			}
		}
	}
	int rem_a7, max_a7;
	if(num_a7max>0) {
		max_a7 = (NUM_A7 - num_a7min) / num_a7max;
		rem_a7 = (NUM_A7 - num_a7min) % num_a7max;
	}
	int rem_a15, max_a15;
	if(num_a15max>0) {
		max_a15 = (NUM_A15 - 1 - num_a15min) / num_a15max; // leave extra a15 for new apps
		rem_a15 = (NUM_A15 - 1 - num_a15min) % num_a15max;
	}
	req_freq_a7 = FREQ_A7[0];
	req_freq_a15 = FREQ_A15[0];
	for(app=0; app<napps; app++) {
		if(apps_info[app].running) {
			if(d_a7cores[app]==DECISION_MAX) {
				if(rem_a7>0) {
					req_a7[app] = max_a7+1;
					rem_a7--;
				}
				else
					req_a7[app] = max_a7;
			}
			else if(d_a7cores[app]==DECISION_MIN)
				req_a7[app] = 1;
			else
				req_a7[app] = 0;
			
			if(d_a15cores[app]==DECISION_MAX) {
				if(rem_a15>0) {
					req_a15[app] = max_a15+1;
					rem_a15--;
				}
				else
					req_a15[app] = max_a15;
			}
			else if(d_a15cores[app]==DECISION_MIN)
				req_a15[app] = 1;
			else
				req_a15[app] = 0;
			
			if(req_a7[app]>0 && d_freq[app]==DECISION_MAX)
				req_freq_a7 = FREQ_A7[NUM_A7_DVFS_POINTS-1];
			if(req_a15[app]>0 && d_freq[app]==DECISION_MAX)
				req_freq_a15 = FREQ_A15[NUM_A15_DVFS_POINTS-1];
		}
	}
}

// MVLR RTM ---------------------------------

double calc_pnp_proc(double fv2_a7, int req_a7[], double fv2_a15, int req_a15[]) {
	int total_a7 = 0;
	int total_a15 = 0;
	int i;
	double ipc = 0.0;
	for(i=0; i<MAX_TASKS; i++) {
		total_a7 += req_a7[i];
		total_a15 += req_a7[15];
		ipc += perf[i].a*req_a7[i] + perf[i].b*req_a15[i] + perf[i].c;
	}
	double power = pwr.a*fv2_a7*total_a7 + pwr.b*fv2_a15*total_a15 + pwr.c;
	return power==0.0 ? 0.0 : ipc/power;
}

void rtm_control_mvlr(double (*calc_param)(double, int[], double, int[]), double opt_dir) {
	if(!start_iter_cores())
		return;
	
	int test_a7[MAX_TASKS];
	int test_a15[MAX_TASKS];
	int has_best = 0;
	double best_param = 0.0;
	
	int i, fi_a7, fi_a15;
	while(next_iter_cores(test_a7, test_a15)) {
		if(opt_disable_dvfs) {
			double param = (*calc_param)(cur_fv2_a7, test_a7, cur_fv2_a15, test_a15);
			if(!has_best || (param-best_param)*opt_dir>0.0) {
				best_param = param;
				has_best = 1;
				for(i=0; i<MAX_TASKS; i++) {
					req_a7[i] = test_a7[i];
					req_a15[i] = test_a15[i];
				}
			}
		}
		else {
			for(fi_a7=0; fi_a7<NUM_A7_DVFS_POINTS; fi_a7++)
				for(fi_a15=0; fi_a15<NUM_A15_DVFS_POINTS; fi_a15++) {
					double param = (*calc_param)(FREQ_A7[fi_a7]*VOLT_A7[fi_a7]*VOLT_A7[fi_a7], test_a7, FREQ_A15[fi_a15]*VOLT_A7[fi_a15]*VOLT_A7[fi_a15], test_a15);
					if(!has_best || (param-best_param)*opt_dir>0.0) {
						best_param = param;
						has_best = 1;
						for(i=0; i<MAX_TASKS; i++) {
							req_a7[i] = test_a7[i];
							req_a15[i] = test_a15[i];
						}
						req_freq_a7 = FREQ_A7[fi_a7];
						req_freq_a15 = FREQ_A15[fi_a15];
					}
				}
		}
	}
}

// RTM ---------------------------------

static void* rtm_proc(void *arg)
{
	int i, app;
	init_history();
	start_core_monitors();
	
	int napps_running;
	int learn_phase;
	int learn[MAX_TASKS];
	printf("rtm: started\n");
	
	unsigned long time = 0;
	unsigned long prev_time;
	FILE* xu3pwr_log = open_log("xu3power");
	if(xu3pwr_log)
		fprintf(xu3pwr_log, "time\tV_a7\tA_a7\tW_a7\tV_a15\tA_a15\tW_a15\tF_a7\tF_a15\n");
	FILE* rtm_log = open_log("rtm");
	if(rtm_log)
		fprintf(rtm_log, "time\tncores\tncores_a7\tncores_a15\tnapps_running\tstable\tlearn\tt_model\tt_ctrl\tmeas_reward\tnew_reward\n");
	init_mvlr_logs();
	
	for(i=0; i<NUM_XU3POWER_PARAMS; i++)
		mon_xu3power[i] = 0.0;
	
	int prev_stable;
	for(i=0; req_stop_rtm==0; i++) {
		prev_time = time;
		sleep_ms(opt_mon_period);
		time = timestamp();
		if(prev_time>0)
			cycle_time = (double)(time-prev_time)/1000.0;
		if(opt_debug_info)
			printf("\nRTM cycle at %lu (%s, cycle time=%lf):\n", time, system_stable ? "STABLE" : "UNSTABLE", cycle_time);
		
		update_global_app_status(time);
		learn_phase = 0;
		prev_stable = system_stable;
		
		// collect monitor readings, update history and models
		if(read_xu3power_all(mon_xu3power)) {
			cur_freq_a7 = req_freq_a7 = mon_xu3power[A7_F]/1000.0;
			cur_freq_a15 = req_freq_a15 = mon_xu3power[A15_F]/1000.0;
			cur_fv2_a7 = cur_freq_a7 * mon_xu3power[A7_V]*mon_xu3power[A7_V];
			cur_fv2_a15 = cur_freq_a15 * mon_xu3power[A15_V]*mon_xu3power[A15_V];
			if(system_stable) {
				add_power(time, &pwr, ncores_a7, ncores_a15,
					mon_xu3power[A7_W]+mon_xu3power[A15_W],
					cur_fv2_a7, cur_fv2_a15
				);
				if(opt_do_mvlr && !opt_disable_affinity && mvlr_fit_power(time, &pwr, NULL)!=0)
					learn_phase = 1;
			}
			log_xu3power_all(xu3pwr_log, time, mon_xu3power);
		}
		else {
			printf("*error: read_xu3power_all\n");
		}

		total_req_a7 = 0;
		total_req_a15 = 0;
		napps_running = 0;
		for(app=0; app<napps; app++) {
			if(!apps_info[app].running) {
				learn[app] = 0;
				req_a7[app] = 0;
				req_a15[app] = 0;
			}
			else {
				napps_running++;
				int c;
				double total_ipc = 0.0;
				int ncores = apps_info[app].ncores;
				for(c=0; c<ncores; c++) {
					int tnum = core_names[apps_info[app].cores[c]];
					double inst_ret = monitors_tinfo[tnum].mon_inst_retired;
					double cycles = monitors_tinfo[tnum].mon_unhalt_cycles;
					if(cycles!=0)
						total_ipc += inst_ret/cycles;
				}
				if(system_stable) {
					add_perf(time, &perf[app], apps_info[app].ncores_a7, apps_info[app].ncores_a15, total_ipc);
					if(opt_do_mvlr && !opt_disable_affinity) {
						learn[app] = mvlr_fit_perf(time, app, &perf[app], NULL);
						if(learn[app]==0) {
							req_a7[app] = apps_info[app].ncores_a7;
							req_a15[app] = apps_info[app].ncores_a15;
						}
						else {
							learn_phase = 1;
							if(perf[app].perf[0][1]<0.0) {
								req_a7[app] = 0;
								req_a15[app] = 1;
							}
							else if(perf[app].perf[1][0]<0.0) {
								req_a7[app] = 1;
								req_a15[app] = 0;
							}
							else {
								// should not happen
								req_a7[app] = 1;
								req_a15[app] = 1;
							}
						}
					}
				}
			}
			total_req_a7 += req_a7[app];
			total_req_a15 += req_a15[app];
		}
		
		unsigned long time_mvlr = timestamp() - time;
		if(opt_debug_info)
			printf("Time MVLR: %lu\n", time_mvlr);
		
		if(!opt_disable_affinity && system_stable) {
			if(learn_phase) {
				if(opt_debug_info)
					printf("LEARNING\n");
				// prioritize learning tasks, move others accordingly
				if(total_req_a7 > NUM_A7) {
					int move = total_req_a7-NUM_A7;
					while(move>0) {
						for(app=0; app<napps; app++) {
							if(!learn[app] && apps_info[app].running) {
								req_a7[app]--;
								req_a15[app]++;
								move--;
								if(move==0)
									break;
							}
						}
					}
				}
				else if(total_req_a15 > NUM_A15) {
					int move = total_req_a15-NUM_A15;
					while(move>0) {
						for(app=0; app<napps; app++) {
							if(!learn[app] && apps_info[app].running) {
								req_a15[app]--;
								req_a7[app]++;
								move--;
								if(move==0)
									break;
							}
						}
					}
				}
			}
			else {
				if(opt_debug_info)
					printf("CONTROL (%d)\n", opt_rtm_alg);
				switch(opt_rtm_alg) {
					case RTM_MVLR_PNP:
						rtm_control_mvlr(&calc_pnp_proc, MAXIMIZE);
						break;
					case RTM_CLASSIFY:
						rtm_control_classify(time);
						break;
					default:
						// no RTM control selected
						break;
				}
			}
			
			system_stable = 1;
			apply_affinity_req(req_a7, req_a15);
			apply_dvfs_req(mon_xu3power, req_freq_a7, req_freq_a15);
			next_unused_core = get_unused_core();
		}
		else {
			system_stable = 1;
		}
		
		unsigned long time_control = timestamp() - time - time_mvlr;
		if(opt_debug_info)
			printf("Time control: %lu\n", time_control);

		log_apps_info(time);
		if(rtm_log)
			fprintf(rtm_log, "%lu\t%d\t%d\t%d\t%d\t%d\t%d\t%lu\t%lu\n",
				time, ncores_a7+ncores_a15, ncores_a7, ncores_a15, napps_running, prev_stable, learn_phase, time_mvlr, time_control);
	}
	stop_core_monitors();
	close_app_logs(timestamp());
	fclose(xu3pwr_log);
	fclose(rtm_log);
	if(control_log)
		fclose(control_log);
	close_mvlr_logs();
	printf("rtm: stopped\n");
}

void start_rtm()
{
	int err;
	pthread_attr_t attr;
	cpu_set_t cpus;
	
	// set affinity
	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(0, &cpus);
	err = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	if(err)
		printf("*error(%d): pthread_attr_setaffinity_np, %d\n", __LINE__, err);
	
	// create_thread
	req_stop_rtm = 0;
	err = pthread_create(&rtm_thread_id, &attr, rtm_proc, NULL);
	if(err)
		printf("*error(%d): pthread_create, %d\n", __LINE__, err);
}

void stop_rtm()
{
	req_stop_rtm = 1;
	pthread_join(rtm_thread_id, NULL);
}

// MAIN ---------------------------------

int main(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "c:m:osv")) != -1) {
		switch (opt) {
			case 'c':
				if(!strcmp(optarg, "mvlr_pnp"))
					opt_rtm_alg = RTM_MVLR_PNP;
				else if(!strcmp(optarg, "classify")) {
					opt_rtm_alg = RTM_CLASSIFY;
					opt_do_mvlr = 0;
				}
				else if(!strcmp(optarg, "none")) {
					opt_rtm_alg = RTM_NONE;
					opt_do_mvlr = 0;
				}
				else {
					fprintf(stderr, "Unknown control algorithm: %s\n", optarg);
					exit(1);
				}
				break;
			case 'm':
				opt_mon_period = atoi(optarg);
				if(opt_mon_period<50)
					opt_mon_period = 50;
				break;
			case 'o': opt_disable_affinity = 1; break;
			case 's': opt_disable_dvfs = 1; break;
			case 'v': opt_debug_info = 1; break;
			default:
				fprintf(stderr, "Usage: %s [-osv] [-m period] < schedule\n", argv[0]);
				exit(1);
				break;
		}
	}
	
	init_xu3power();
	init_affinity();
	start_rtm();
	sleep_ms(1000); // collect idle power for 1s
	run_exec();
	stop_rtm();
}
