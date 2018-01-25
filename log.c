
#include "log.h"

FILE* open_log_file(char* fname)
{
#if WRITE_LOG
	FILE *fp;
	fp = fopen(fname,"w");
	if(fp == NULL)
		printf("*error: Cannot write log file %s\n", fname);
	return fp;
#else
	return NULL;
#endif
}

FILE* open_log(const char* prefix)
{
	char fname[64];
	sprintf(fname, "results/%s.csv", prefix);
	return open_log_file(fname);
}

FILE* open_log_index(const char* prefix, int index)
{
	char fname[64];
	sprintf(fname, "results/%s_%d.csv", prefix, index);
	return open_log_file(fname);
}
