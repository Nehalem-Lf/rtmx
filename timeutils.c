
#include "timeutils.h"

void sleep_ms(int ms)
{
	struct timespec t;
	if(ms>0) {
		t.tv_sec  = ms/1000;
		t.tv_nsec = (ms%1000)*1000000L;
		nanosleep(&t, NULL);
	}
}

unsigned long timestamp()
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	return (t.tv_sec)*1000L + (t.tv_nsec)/1000000L;
}
