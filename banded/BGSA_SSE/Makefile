ifeq ($(arch), KNL)
	ifdef cc
		CC=$(cc)
		CFLAGS=-mfma -mavx512f -mavx512er -mavx512cd -w
		MP=-fopenmp
	else
		CC=icc
		CFLAGS=-xMIC-AVX512
		MP=-qopenmp
	endif
else 
	ifdef cc
		CC=$(cc)
		CFLAGS=-march=native -w 
		MP=-fopenmp
	else
		CC=icc
		MP=-qopenmp
	endif
endif

all: aligner convert

aligner: global.c file.c thread.c align_core.c cal_sse.c main.c file.h align_core.h thread.h global.h cal.h config.h Makefile
	$(CC) $(CFLAGS) $(MP) -O3 global.c file.c thread.c align_core.c cal_sse.c main.c -o aligner -lpthread

convert: convert.c config.h
	$(CC) $(CFLAGS) -O3 convert.c -o convert -lpthread

clean:
	rm -f aligner convert
