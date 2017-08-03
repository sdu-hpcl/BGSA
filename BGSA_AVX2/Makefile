all: aligner convert

aligner: global.c util.c file.c thread.c align_core.c cal_avx.c main.c file.h align_core.h thread.h global.h cal.h config.h Makefile
	icc -xMIC-AVX512 -O3 -qopenmp  -qopt-prefetch=4 -qopt-threads-per-core=4 -qopt-prefetch-distance=0,0   global.c util.c file.c thread.c align_core.c cal_avx.c main.c -o aligner -lpthread

convert: convert.c
	icc -O3 convert.c -o convert -lpthread

clean:
	rm -f aligner convert
