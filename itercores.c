
#include "itercores.h"

int num_apps;
int iter_cores_has_next = 1;
int app_has_next = 1;

int app_n7[MAX_TASKS];
int app_nth[MAX_TASKS];

void start_app(int i) {
	int n = app_nth[i] - NUM_A15;
	app_n7[i] = (n<0) ? 0 : n;
}

void start_apps() {
	app_has_next = 1;
	int i;
	for(i=0; i<num_apps; i++) {
		start_app(i);
	}
}

int total_n7() {
	int s = 0;
	int i;
	for(i=0; i<num_apps; i++)
		s += app_n7[i];
	return s;
}

int total_n15() {
	int s = 0;
	int i;
	for(i=0; i<num_apps; i++)
		s += app_nth[i] - app_n7[i];
	return s;
}

void move_app() {
	int i;
	for(i=0; i<num_apps; i++) {
		if(app_n7[i]<app_nth[i] && app_n7[i]<NUM_A7) {
			app_n7[i]++;
			return;
		}
		start_app(i);
	}
	app_has_next = 0;
}

void move() {
	int i, j;
	for(i=num_apps-1; i>=0; i--) {
		int rem = NUM_A7+NUM_A15-(num_apps-i-1);
		for(j=0; j<i; j++)
			rem -= app_nth[j];
		if(app_nth[i]<rem) {
			app_nth[i]++;
			start_apps();
			return;
		}
		app_nth[i] = 1;
	}
	iter_cores_has_next = 0;
}

int check() {
	int n7 = total_n7();
	int n15 = total_n15();
	if(n7 > NUM_A7)
		return 0;
	if(n15 > NUM_A15)
		return 0;
	if(n7+n15 > NUM_CORES-1)
		return 0;
	return 1;
}

int next() {
	for(;;) {
		if(app_has_next)
			move_app();
		if(!app_has_next)
			move();
		if(!iter_cores_has_next)
			return 0;
		if(check())
			return 1;
	}
}

int start_iter_cores() {
	num_apps = 0;
	int i;
	for(i=0; i<MAX_TASKS; i++) {
		if(apps_info[i].running)
			num_apps++;
		app_n7[i] = 0;
		app_nth[i] = 1;
	}
	if(num_apps==0)
		return 0;
	iter_cores_has_next = 1;
	app_has_next = 1;
	start_apps();
	if(!check())
		next();
	return 1;
}

int next_iter_cores(int req_a7[], int req_a15[]) {
	if(!iter_cores_has_next)
		return 0;
	int i;
	int j = 0;
	for(i=0; i<MAX_TASKS; i++) {
		if(apps_info[i].running) {
			req_a7[i] = app_n7[j];
			req_a15[i] = app_nth[j] - app_n7[j];
			j++;
		}
		else {
			req_a7[i] = 0;
			req_a15[i] = 0;
		}
	}
	next();
	return 1;
}

