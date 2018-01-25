#ifndef _MVLR_H_
#define _MVLR_H_

#include "history.h"
#include <gsl/gsl_multifit.h>

void init_mvlr_logs();
void close_mvlr_logs();
int mvlr_fit_power(unsigned long time, power_model* m, double* out_chisq);
int mvlr_fit_perf(unsigned long time, int index, perf_model* m, double* out_chisq);

#endif
