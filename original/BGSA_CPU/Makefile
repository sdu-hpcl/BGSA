ifdef cc
	CC=$(cc)
	MP=-fopenmp
	CFLAGS=-w 
else
	CC=icc
	MP=-qopenmp
endif

all: aligner convert

aligner: global.c file.c thread.c align_core.c cal_cpu.c main.c file.h align_core.h thread.h global.h cal.h config.h Makefile
	$(CC) $(MP) $(CFLAGS) -O3  global.c file.c thread.c align_core.c cal_cpu.c main.c -o aligner -lpthread

convert: convert.c config.h
	$(CC) $(CFLAGS) -O3 convert.c -o convert -lpthread

clean:
	rm -f aligner convert
