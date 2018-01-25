#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h> // compile with -lm
#include <pthread.h> // compile with -lpthread
#include <time.h> // compile with -lrt

struct thread_info {
	pthread_t thread_id;
	int thread_num;
};

int verbose = 0;
struct thread_info *tinfo;

static void* thread_sqrt(void *arg) {
	int n = *(int*) arg;
	double x = 0.0;	
	long i, j;
	for(j=0; j<n; j++)
		for(i=0; i<4000L; i++) {
			x = sqrt(3.0*x+4.0);
		}
	return NULL;
}

static void* thread_intadd(void *arg) {
	int n = *(int*) arg;
	int x = 0;
	int i, j;
	for(j=0; j<n; j++)
		for(i=0; i<40*1700; i++) {
			x = ((i+1)*(x+1))/7;
		}
	return NULL;
}

static void* thread_log(void *arg) {
	int n = *(int*) arg;
	double x = 0.0;	
	long i, j;
	for(j=0; j<n; j++)
		for(i=0; i<4000L; i++) {
			x = log(3.0*(x+1)+2.0);
		}
	return NULL;
}

unsigned long print_time(unsigned long prev_tstamp, const char* prompt) {
	struct timespec t;
	unsigned long tstamp;
	clock_gettime(CLOCK_REALTIME, &t);
	tstamp = (t.tv_sec)*1000L + (t.tv_nsec)/1000000L;
	printf("%s:\t%lu\n", prompt, tstamp - prev_tstamp);
	return tstamp;
}


void parallelize(int num_threads, void *(*start_routine) (void *), void *arg) {
	int tnum, err;
	pthread_attr_t attr;
	cpu_set_t cpus;
	
	pthread_attr_init(&attr);
	for(tnum = 0; tnum < num_threads; tnum++) {
		tinfo[tnum].thread_num = tnum + 1;

		// start thread
		err = pthread_create(&tinfo[tnum].thread_id, &attr, start_routine, arg);
		if(err)
			printf("*error(%d): pthread_create, %d\n", __LINE__, err);
	}
	
	// wait for all threads to finish
	for(tnum = 0; tnum < num_threads; tnum++) {
		pthread_join(tinfo[tnum].thread_id, NULL);
	}
}

int main(int argc, char *argv[])
{
	cpu_set_t cpus;
	int err, opt;
	
	int npar, nseq, r;
	int num_threads = 2;
	int repeat = 5;
	int ntotal = 10000;
	float p = 0.5;
	float adjust = 1.0; // parallelization overread adjustment, 1 = no adjustment
	
	void *(*start_routine) (void *) = &thread_sqrt;

	unsigned long prev_tstamp = 0L;
	unsigned long start_tstamp;
	
	// parse arguments
	while((opt = getopt(argc, argv, "vm:n:j:p:w:r:")) != -1) {
		switch (opt) {
			case 'v':
				verbose = 1;
				break;
			case 'm':
				if(optarg[0]=='1')
					start_routine = &thread_intadd;
				else if(optarg[0]=='0')
					start_routine = &thread_sqrt;
				else if(optarg[0]=='2')
					start_routine = &thread_log;
				else
					fprintf(stderr, "Unknown mode: %c\n", optarg[0]);
				break;
			case 'p':
				p = atof(optarg);
				break;
			case 'j':
				adjust = atof(optarg);
				break;
			case 'w':
				ntotal = atoi(optarg);
				break;
			case 'r':
				repeat = atoi(optarg);
				break;
			case 'n':
				num_threads = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-v] [-p parallel_ratio] [-j adjust] [-w workload] [-r repeat] [-n num_threads]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// init
	npar = (int)(ntotal*p*adjust);
	nseq = (int)(ntotal*(1.0-p));
	printf("%d threads...\n", num_threads);
	tinfo = calloc(num_threads, sizeof(struct thread_info));
	
	prev_tstamp = print_time(prev_tstamp, "Start");
	start_tstamp = prev_tstamp;
	
	for(r=0; r<repeat; r++) {
	
		// execute parallel workload
		parallelize(num_threads, start_routine, &npar);
		if(verbose)
			prev_tstamp = print_time(prev_tstamp, "Parallel");
			
		// execute sequential workload
		(*start_routine)(&nseq);
		if(verbose)
			prev_tstamp = print_time(prev_tstamp, "Sequential");
	}
	
	print_time(start_tstamp, "Total");
	return 0;
}
