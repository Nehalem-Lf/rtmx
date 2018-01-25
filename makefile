HEADERS = timeutils.h affinity.h dvfs.h monitors.h log.h exec.h history.h mvlr.h itercores.h pmu.h $(shell pkg-config --cflags opencv)
OBJECTS = timeutils.o affinity.o dvfs.o monitors.o log.o exec.o history.o mvlr.o itercores.o rtmx.o
GSL_DIR = /media/root/Data/gsl
LIBS = -lrt -pthread -lgsl -lgslcblas -lm

.PHONY: default all clean

default: pthreads sqrt rtmx
all: default

%.o: %.c $(HEADERS)
	gcc -I$(GSL_DIR)/include -c $< -o $@ -std=c++0x

rtmx: $(OBJECTS)
	gcc -static -L$(GSL_DIR)/lib $(OBJECTS) -o $@ $(LIBS)
	
pthreads: pthreads.c
	gcc pthreads.c -o pthreads -lm -lrt -lpthread

sqrt: sqrt.c
	gcc sqrt.c -o sqrt -lm -lrt

clean:
	-rm -f *.o
	-rm -f rtmx
	-rm -f sqrt
	-rm -f pthreads
