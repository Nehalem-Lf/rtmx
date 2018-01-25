
#include "affinity.h"

int opt_debug_info = 0;
int opt_disable_affinity = 0;

const int core_names[NUM_CORES] = {4, 5, 6, 7, 0, 1, 2, 3};
const int core_types[NUM_CORES] = {A15, A15, A15, A15, A7, A7, A7, A7};

int system_stable = 1;

app_info apps_info[MAX_TASKS];
app_info* core_app_map[NUM_CORES];
int napps = 0;
int ncores_a7 = 0;
int ncores_a15 = 0;
int next_unused_core = 0;

FILE* app_logs[MAX_TASKS];

void init_affinity()
{
	int i;
	for(i=0; i<NUM_CORES; i++)
		core_app_map[i] = NULL;
	for(i=0; i<MAX_TASKS; i++) {
		apps_info[i].pid = 0;
		app_logs[i] = NULL;
	}
}

int get_unused_core()
{
	if(opt_disable_affinity) {
		return 0;
	}
	else {
		int i;
		for(i=0; i<NUM_CORES; i++) {
			if(core_app_map[i]==NULL || !core_app_map[i]->running)
				return i;
		}
		printf("Warning: no free cores\n");
		return 0;
	}
}

void set_app_affinity(app_info* app, int ncores, int cores[], int use_stability)
{
	int i, err;
	int stable = use_stability;
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	if(!stable || app->ncores!=ncores) {
		app->ncores = ncores;
		stable = 0;
	}
	if(opt_debug_info)
		printf("app <%d> affinity: ", app->pid);
	app->ncores_a7 = 0;
	app->ncores_a15 = 0;
	for(i=0; i<ncores; i++) {
		if(opt_debug_info)
			printf("%d ", core_names[cores[i]]);
		if(!stable || app->cores[i]!=cores[i]) {
			CPU_SET(core_names[cores[i]], &cpus);
			app->cores[i] = cores[i];
			stable = 0;
		}
		core_app_map[cores[i]] = app;
		if(core_types[cores[i]]==A7)
			app->ncores_a7++;
		if(core_types[cores[i]]==A15)
			app->ncores_a15++;
	}
	if(opt_debug_info)
		printf("\n");
	if(!stable) {
		system_stable = 0;
		err = sched_setaffinity(app->pid, sizeof(cpu_set_t), &cpus);
		if(err)
			printf("*error(%d): sched_setaffinity, %d\n", __LINE__, err);
	}
}

void log_app_info(unsigned long time, FILE* log, app_info* app)
{
	if(log) {
		fprintf(log, "%lu\t%d\t%d\t%lu\t%lu\t%d\t%d\t%d\t%d\t\"",
			time, app->task_id, app->pid, app->start_time, app->end_time, app->running, app->ncores, app->ncores_a7, app->ncores_a15);
		int j;
		for(j=0; j<app->ncores; j++) {
			fprintf(log, "%d ", core_names[app->cores[j]]);
		}
		fprintf(log, "\"\n");
	}
}

void create_app_info(int task_id, pid_t pid)
{
	if(opt_debug_info)
		printf("New app <%d> for task %d\n", pid, task_id);
	int i = napps++;
	apps_info[i].task_id = task_id;
	apps_info[i].pid = pid;
	apps_info[i].running = 1;
	apps_info[i].start_time = timestamp();
	apps_info[i].end_time = 0;
	int cores[] = {next_unused_core};
	
	if(!opt_disable_affinity)
		set_app_affinity(&apps_info[i], 1, cores, 0);
	
	FILE* log = open_log_index("app", i);
	if(log) fprintf(log, "time\ttask_id\tpid\tstart_time\tend_time\trunning\tncores\tncores_a7\tncores_a15\tcores\n");
	app_logs[i] = log;
	log_app_info(apps_info[i].start_time, log, &apps_info[i]);
}

int update_app_status(int app_index, unsigned long time)
{
	if(app_index >= napps || !apps_info[app_index].running)
		return 0;
	else {
		int status;
		int pid = apps_info[app_index].pid;
		int running = (waitpid(pid, &status, WNOHANG)==0);
		apps_info[app_index].running = running;
		if(!running) {
			apps_info[app_index].end_time = time;
			apps_info[app_index].ncores = 0;
			apps_info[app_index].ncores_a7 = 0;
			apps_info[app_index].ncores_a15 = 0;
		}
		return running;
	}
}

void update_global_app_status(unsigned long time)
{
	ncores_a7 = 0;
	ncores_a15 = 0;
	int i=0;
	for(i=0; i<napps; i++) {
		if(update_app_status(i, time)) {
			ncores_a7 += apps_info[i].ncores_a7;
			ncores_a15 += apps_info[i].ncores_a15;
		}
	}
}

int next_by_type(int c, int type) {
	for(;;) {
		c++;
		if(c>NUM_CORES) {
			printf("*error(%d): requested to many cores of type %d\n", __LINE__, type);
			return 0;
		}
		if(core_types[c]==type)
			return c;
	}
}

void apply_affinity_req(int req_a7[], int req_a15[])
{
	int i, j;
	int a7 = -1;
	int a15 = -1;
	for(i=0; i<NUM_CORES; i++) {
		core_app_map[i] = NULL;
	}
	for(i=0; i<napps; i++) {
		if(apps_info[i].running) {
			int cores[NUM_CORES];
			int c = 0;
			for(j=0; j<req_a7[i]; j++) {
				a7 = next_by_type(a7, A7);
				cores[c++] = a7;
			}
			for(j=0; j<req_a15[i]; j++) {
				a15 = next_by_type(a15, A15);
				cores[c++] = a15;
			}
			set_app_affinity(&apps_info[i], c, cores, 1);
			if(opt_debug_info)
				printf("(requested %d A7, %d A15)\n", req_a7[i], req_a15[i]);
		}
	}
}

void log_apps_info(unsigned long time)
{
	int i;
	for(i=0; i<napps; i++) {
		app_info* app = &apps_info[i];
		if(app->running) {
			log_app_info(time, app_logs[i], app);
		}
		else if(app_logs[i]) {
			log_app_info(time, app_logs[i], &apps_info[i]);
			fclose(app_logs[i]);
			app_logs[i] = NULL;
		}
	}
}

void close_app_logs(unsigned long time)
{
	int i;
	for(i=0; i<MAX_TASKS; i++) {
		if(app_logs[i]) {
			log_app_info(time, app_logs[i], &apps_info[i]);
			fclose(app_logs[i]);
		}
	}
}
