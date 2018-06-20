#ifndef CAL_H
#define CAL_H

#include <stdint.h>
#include "global.h"

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

typedef struct _chunk_read_info {
    int chunk_read_num;
    int chunk_word_num;
    int rest_read_num;
    int rest_word_num;
} chunk_read_info;

extern seq_t read_seq_a;
extern seq_t read_seq_b;
extern seq_t * read_seq;
extern seq_t   ref_seq;

extern __m128i           * sse_dvdh_bit_mem;
extern uint64_t          * cpu_dvdh_bit_mem;

extern sse_read_t  * sse_preprocess_reads_a;
extern sse_read_t  * sse_preprocess_reads_b;
extern sse_read_t  * sse_read_ptr;

extern sse_write_t * sse_align_results_a;
extern sse_write_t * sse_align_results_b;
extern sse_write_t * sse_result_ptr;

void sse_cal_align_score(char * content, sse_read_t * preprocess_reads, sse_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num, __m128i * dvdh_bit_mem);

void cal_on_sse();

#endif
