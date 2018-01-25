
#include "exec.h"

char* tasks[][MAX_ARGS] = {
	{"./pthreads", "-w", "2000", "-n", "8", "-p", "1", "-j", "0.125", NULL},
	{"./pthreads", "-w", "2000", "-n", "8", "-p", "0.5", "-j", "0.125", NULL},
	{"/root/work/parsec-3.0/pkgs/apps/fluidanimate/inst/arm-linux.gcc-pthreads/bin/fluidanimate",
		"8", "5", 
		"/root/work/parsec-3.0/pkgs/apps/fluidanimate/inputs/in_300K.fluid",
		"/root/work/parsec-3.0/out.fluid", NULL},
	{"/root/work/parsec-3.0/pkgs/apps/ferret/inst/arm-linux.gcc-pthreads/bin/ferret",
		"/root/work/parsec-3.0/pkgs/apps/ferret/inputs/simlarge/corel",
		"lsh",
		"/root/work/parsec-3.0/pkgs/apps/ferret/inputs/simlarge/queries",
		"10", "20", "8",
		"/root/work/parsec-3.0/pkgs/apps/ferret/inputs/simlarge/output.txt", NULL}	
};

int n_pid = 0;
pid_t pid_list[MAX_TASKS];

int start_task(int id)
{
	printf("Start task %d (%s)\n", id, tasks[id][0]);
	pid_t pid = fork();
	if(pid!=0) {
		pid_list[n_pid++] = pid;
		create_app_info(id, pid);
		return pid;
	}
	else {
		int fd = open("/dev/null", O_WRONLY);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		close(fd);
		execvp(tasks[id][0], tasks[id]);
		perror("*error");
		exit(1);
		return 0;
	}
}

void wait_all()
{
	int status;
	while(n_pid>0) {
		waitpid(pid_list[--n_pid], &status, 0);
	}
}

void run_exec()
{
	int sleep, id;
	unsigned long tstart = timestamp();
	while(scanf("%d %d", &sleep, &id)==2) {
		sleep_ms(sleep);
		if(start_task(id)==0)
			return;
	}
	wait_all();
	printf("Done exec, total time: %ld\n", timestamp()-tstart);
}


