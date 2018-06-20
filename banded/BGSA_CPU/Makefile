all: aligner convert

aligner: global.c file.c thread.c align_core.c cal_cpu.c main.c file.h align_core.h thread.h global.h cal.h config.h Makefile
	icc  -std=c11 -qopenmp -O3  global.c file.c thread.c align_core.c cal_cpu.c main.c -o aligner -lpthread

convert: convert.c config.h
	icc -O3 convert.c -o convert -lpthread

clean:
	rm -f aligner convert
