#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

#define _GNU_SOURCE
#include <time.h> // compile with -lrt

void sleep_ms(int ms);
unsigned long timestamp();

#endif
