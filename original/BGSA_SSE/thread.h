#ifndef _THREADH_
#define _THREADH_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "global.h"

typedef struct _thread_info{
    pthread_t       thread_id;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    int run_flag;
    int buffer_flag;
    int shutdown;
} thread_info;

// 线程函数参数
typedef struct _thread_arg {
    FILE *fp;
} thread_arg;

typedef struct _input_thread_arg {
    thread_info * input_info;
    thread_info * cal_input_info;
    FILE    * fp;
    int64_t * result_bucket_counts;
    int64_t * sse_result_bucket_counts;
    sse_read_t  * sse_preprocess_reads_a;
    sse_read_t  * sse_preprocess_reads_b;
    seq_t   * read_seq_a;
    seq_t   * read_seq_b;
    int     bucket_num;
    int     word_num;
    int     sse_word_num;
    int     cpu_word_num;
    int64_t sse_preprocess_read_size;
    int64_t bucket_size;
    int64_t total_size;
    int64_t * device_read_counts_a;
    int64_t * device_read_counts_b;
} input_thread_arg;

typedef struct _output_thread_arg{
    FILE * fp;
    int64_t * result_count;
    int64_t * sse_result_count;
    int64_t * cpu_result_count;
    int64_t   result_size;
    thread_info * output_info;
    thread_info * cal_output_info;
    sse_write_t * sse_align_results_a;
    sse_write_t * sse_align_results_b;
    int           mic_device_number;
} output_thread_arg;

void init_resources(int n, ...);

void free_resources(int n, ...);

void * input_task_sse(void * args);

void * output_task_sse(void * args);

#endif
