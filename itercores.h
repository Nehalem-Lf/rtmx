#ifndef _ITERCORES_H_
#define _ITERCORES_H_

#include "affinity.h"

extern int iter_cores_has_next;

int start_iter_cores();
int next_iter_cores(int req_a7[], int req_a15[]);

#endif
