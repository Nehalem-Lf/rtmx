
#include "mvlr.h"

FILE* pwr_log;
FILE* perf_logs[MAX_TASKS];

void init_mvlr_logs()
{
	pwr_log = NULL;
	int i;
	for(i=0; i<MAX_TASKS; i++)
		perf_logs[i] = NULL;
}


void close_mvlr_logs()
{
	if(pwr_log)
		fclose(pwr_log);
	int i;
	for(i=0; i<MAX_TASKS; i++)
		if(perf_logs[i])
			fclose(perf_logs[i]);
}

FILE* start_mvlr_log(char* prefix, int index)
{
	FILE* log = index<0 ? open_log(prefix) : open_log_index(prefix, index);
	if(log)
		fprintf(log, "time\ta\tb\tc\tchi2\n");
	return log;
}

int mvlr_fit_power(unsigned long time, power_model* m, double* out_chisq)
{
	int na7, na15, i, npoints;
	double chisq;
	gsl_matrix *X, *cov;
	gsl_vector *y, *c;
	
	// get number of points
	npoints = 0;
	for(na7=0; na7<TABLE_SIZE; na7++)
		for(na15=0; na15<TABLE_SIZE; na15++) {
			if(m->power[na7][na15]>=0.0)
				npoints++;
		}
	if(npoints<3) {
		if(opt_debug_info)
			printf("Power model needs %d more point(s)...\n", 3-npoints);
		return (-3+npoints); // not enough points, return the missing number
	}
	
	// init matrices
	X = gsl_matrix_alloc(npoints, 3);
	y = gsl_vector_alloc(npoints);
	c = gsl_vector_alloc(3);
	cov = gsl_matrix_alloc(3, 3);
	
	i = 0;
	for(na7=0; na7<TABLE_SIZE; na7++)
		for(na15=0; na15<TABLE_SIZE; na15++) {
			if(m->power[na7][na15]>=0.0) {
				gsl_matrix_set(X, i, 0, 1.0);
				gsl_matrix_set(X, i, 1, ((double) na7) * m->fv2_a7[na7][na15]);
				gsl_matrix_set(X, i, 2, ((double) na15) * m->fv2_a15[na7][na15]);
				gsl_vector_set(y, i, m->power[na7][na15]);
				i++;
			}
		}

	// fitting
	gsl_multifit_linear_workspace *work;
	work = gsl_multifit_linear_alloc(npoints, 3);
	gsl_multifit_linear(X, y, c, cov, &chisq, work);
	gsl_multifit_linear_free(work);
	
	// update model
	m->c = gsl_vector_get(c, (0));
	m->a = gsl_vector_get(c, (1));
	m->b = gsl_vector_get(c, (2));
	if(out_chisq!=NULL)
		*out_chisq = chisq;
		
	//log
	if(!pwr_log)
		pwr_log = start_mvlr_log("mvlr_pwr", -1);
	if(pwr_log) {
		fprintf(pwr_log, "%lu\t%lf\t%lf\t%lf\t%lf\n", time, m->c, m->a, m->b, chisq);
	}
	if(opt_debug_info) {
		printf ("Power = %g + %g * n_A7 + %g * n_A15  (chi2 : %g)\n", m->c, m->a, m->b, chisq);
	}
	
	// clean-up
	gsl_matrix_free(X);
	gsl_vector_free(y);
	gsl_vector_free(c);
	gsl_matrix_free(cov);
	
	return 0;
}

int mvlr_fit_perf(unsigned long time, int index, perf_model* m, double* out_chisq)
{
	int na7, na15, i, npoints;
	double chisq;
	gsl_matrix *X, *cov;
	gsl_vector *y, *c;
	
	// get number of points
	npoints = 0;
	for(na7=0; na7<TABLE_SIZE; na7++)
		for(na15=0; na15<TABLE_SIZE; na15++) {
			if(m->perf[na7][na15]>=0.0)
				npoints++;
		}
	if(npoints<3) {
		if(opt_debug_info)
			printf("Perf[%d] model needs %d more point(s)...\n", m->task_id, 3-npoints);
		return (-3+npoints); // not enough points, return the missing number
	}
	
	// init matrices
	X = gsl_matrix_alloc(npoints, 3);
	y = gsl_vector_alloc(npoints);
	c = gsl_vector_alloc(3);
	cov = gsl_matrix_alloc(3, 3);
	
	i = 0;
	for(na7=0; na7<TABLE_SIZE; na7++)
		for(na15=0; na15<TABLE_SIZE; na15++) {
			if(m->perf[na7][na15]>=0.0) {
				gsl_matrix_set(X, i, 0, 1.0);
				gsl_matrix_set(X, i, 1, ((double) na7));
				gsl_matrix_set(X, i, 2, ((double) na15));
				gsl_vector_set(y, i, m->perf[na7][na15]);
				i++;
			}
		}

	// fitting
	gsl_multifit_linear_workspace *work;
	work = gsl_multifit_linear_alloc(npoints, 3);
	gsl_multifit_linear(X, y, c, cov, &chisq, work);
	gsl_multifit_linear_free(work);
	
	// update model
	m->c = gsl_vector_get(c, (0));
	m->a = gsl_vector_get(c, (1));
	m->b = gsl_vector_get(c, (2));
	if(out_chisq!=NULL)
		*out_chisq = chisq;
		
	//log
	if(!perf_logs[index])
		perf_logs[index] = start_mvlr_log("mvlr_perf", index);
	if(perf_logs[index]) {
		fprintf(perf_logs[index], "%lu\t%lf\t%lf\t%lf\t%lf\n", time, m->c, m->a, m->b, chisq);
	}
	if(opt_debug_info) {
		printf ("Perf[%d] = %g + %g * n_A7 + %g * n_A15  (chi2 : %g)\n", m->task_id, m->c, m->a, m->b, chisq);
	}
	
	// clean-up
	gsl_matrix_free(X);
	gsl_vector_free(y);
	gsl_vector_free(c);
	gsl_matrix_free(cov);
	
	return 0;
}
