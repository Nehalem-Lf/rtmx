
#include <stdio.h>
#include <unistd.h>
#include <math.h> // compile with -lm
#include <time.h> // compile with -lrt

unsigned long timestamp()
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return (t.tv_sec)*1000L + (t.tv_nsec)/1000000L;
}

int main(int argc, char *argv[])
{
	long n = 10000;
	if(argc>1) {
		n = atol(argv[1]);
	}

	unsigned long tstart = timestamp();
	printf("sqrt starting...\n");
	
	double x = 0.0;	
	long i, j;
	for(j=0; j<n; j++)
		for(i=0; i<95L; i++) {
			x = sqrt(3.0*x+4.0);
		}
	
	printf("sqrt finished in %ld\n", timestamp()-tstart);
}

