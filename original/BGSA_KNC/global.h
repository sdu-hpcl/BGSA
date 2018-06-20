 #ifndef _GLOBALH_
#define _GLOBALH_

#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>
#include "config.h"

#define __ONMIC__ __attribute__((target(mic)))

#define ALLOC alloc_if(1)
#define REUSE alloc_if(0)
#define FREE free_if(1)
#define RETAIN free_if(0)

typedef struct _seq_t {
    int len;
    int64_t size;
    int64_t count;
    int     extra_size;
    int     extra_count;
    char * content;
} seq_t;

extern uint32_t mapping_table[128] __attribute__((aligned(64)));

extern const uint32_t one;

void init_mapping_table();

void mic_handle_reads(seq_t * read_seq, mic_read_t * result_reads, int word_num, int64_t read_start, int64_t read_count);

void sse_handle_reads(seq_t * read_seq, sse_read_t * result_reads, int word_num, int64_t read_start, int64_t read_count);

void cpu_handle_reads(seq_t * read_seq, cpu_read_t * result_reads, int word_num, int64_t read_start, int64_t read_count);

void single_handle_reads(seq_t * read_seq, cpu_read_t * result_reads, int word_num, int64_t read_start, int64_t read_count);


void dispatch_task(double * device_radio, int64_t * device_read_counts, int64_t total_read_count, int total_device_number);

__ONMIC__ void * malloc_mem(uint64_t size);

__ONMIC__ void free_mem(void * mem);

void init_cpu_mic_ratio(double * device_ratio, int cpu_number, int mic_number);

void init_device_ratio_file(double * device_ratio, int cpu_number, int mic_number, char * file_ratio);

void init_mic_ratio(double * device_ratio, int mic_number);

void init_device(int mic_device_number) ;

void adjust_device_ratio(double * device_compute_ratios, double * previous_times, int total_device_number);

void adjust_device_ratio(double * device_compute_ratios, double * previous_times, int total_device_number);

extern double ** use_times;
extern double ** loop_device_ratio;
extern double ** loop_used_times;
//extern double  mic_cpu_ratio;
extern int     time_index;
extern int     max_length;
extern int     global_ref_bucket_num;

extern __ONMIC__  int dvdh_len;
extern __ONMIC__  int  match_score;
extern __ONMIC__  int  mismatch_socre;
extern __ONMIC__  int  gap_socre;
extern __ONMIC__  int  full_bits;

extern char * up_signal_a;
extern char * up_signal_b;
extern char * down_signal_a;
extern char * down_signal_b;

extern double read_total_time;
extern double write_total_time;
extern double mem_total_time;

extern char *  file_query;
extern char *  file_database;
extern char *  file_result;
extern char *  file_result_info;
extern char *  file_ratio;
extern char *  file_ref;
extern char *  file_read;

extern int     implement_type;
extern int     mic_number;
extern __ONMIC__ int mic_threads;
extern int     cpu_threads;
extern int     use_dynamic;

#endif
