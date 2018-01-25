#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define WRITE_LOG 1

FILE* open_log(const char* prefix);
FILE* open_log_index(const char* prefix, int index);

#endif