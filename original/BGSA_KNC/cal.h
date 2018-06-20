#ifndef CAL_H
#define CAL_H

#include <stdint.h>
#include "global.h"

typedef struct _mic_cal_t {
    seq_t       * ref_seq;
    seq_t       * read_seq;
    mic_read_t  * read_ptr;
    mic_write_t * result_ptr;
    __m512i     * dvdh_bit_mem;
    int64_t read_size;
    int64_t result_size;
    int64_t read_count;
    int64_t read_ptr_offset;
    int64_t result_ptr_offset;
    int     word_num;
    int     chunk_read_num;
    int     offload_dvdh_size;
    int     mic_ref_start;
    int     mic_ref_end;
    int     mic_index;
    int     async_flag;

    double * cal_total_time;
    double * offload_total_time;

} mic_cal_t;


typedef struct _sse_cal_t {
    seq_t       * ref_seq;
    seq_t       * read_seq;
    sse_read_t  * read_ptr;
    sse_write_t * result_ptr;
    __m128i     * dvdh_bit_mem;
    int64_t read_count;
    int64_t read_ptr_offset;
    int64_t result_ptr_offset;
    int     word_num;
    int     chunk_read_num;
    int     sse_ref_start;
    int     sse_ref_end;
    double * cal_total_time;
} sse_cal_t;

typedef struct _cpu_cal_t {
    seq_t       * ref_seq;
    seq_t       * read_seq;
    cpu_read_t  * read_ptr;
    cpu_write_t * result_ptr;
    uint64_t     * dvdh_bit_mem;
    int64_t read_count;
    int64_t read_ptr_offset;
    int64_t result_ptr_offset;
    int     word_num;
    int     chunk_read_num;
    int     cpu_ref_start;
    int     cpu_ref_end;
    double * cal_total_time;
} cpu_cal_t;

typedef struct _chunk_read_info {
    int chunk_read_num;
    int chunk_word_num;
    int rest_read_num;
    int rest_word_num;
} chunk_read_info;

extern __ONMIC__ mic_read_t  * preprocess_reads_a;
extern __ONMIC__ mic_read_t  * preprocess_reads_b;
extern __ONMIC__ mic_read_t  * read_ptr;

extern __ONMIC__ mic_write_t * align_results_a;
extern __ONMIC__ mic_write_t * align_results_b;
extern __ONMIC__ mic_write_t * result_ptr;


extern __ONMIC__ seq_t read_seq_a;
extern __ONMIC__ seq_t read_seq_b;
extern __ONMIC__ seq_t * read_seq;
extern __ONMIC__ seq_t   ref_seq;

extern __ONMIC__ __m512i * dvdh_bit_mem;
extern __m128i           * sse_dvdh_bit_mem;
extern uint64_t          * cpu_dvdh_bit_mem;

extern sse_read_t  * sse_preprocess_reads_a;
extern sse_read_t  * sse_preprocess_reads_b;
extern sse_read_t  * sse_read_ptr;

extern sse_write_t * sse_align_results_a;
extern sse_write_t * sse_align_results_b;
extern sse_write_t * sse_result_ptr;

extern cpu_read_t  * cpu_preprocess_reads_a;
extern cpu_read_t  * cpu_preprocess_reads_b;
extern cpu_read_t  * cpu_read_ptr;

extern cpu_write_t * cpu_align_results_a;
extern cpu_write_t * cpu_align_results_b;
extern cpu_write_t * cpu_result_ptr;

void sse_cal_align_score(char * content, sse_read_t * preprocess_reads, sse_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num, __m128i * dvdh_bit_mem);

__ONMIC__ void mic_cal_align_score(char * content, mic_read_t * preprocess_reads, mic_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num,  __m512i * dvdh_bit_mem);

void cal_on_mic();

void cal_on_sse();

void cal_on_cpu();

void cal_on_all();

void cal_on_all_dynamic();

void cal_on_single();


#endif
