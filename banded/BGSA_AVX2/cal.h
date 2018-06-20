#ifndef CAL_H
#define CAL_H

#include <stdint.h>
#include "global.h"

typedef struct _avx_cal_t {
    seq_t       * ref_seq;
    seq_t       * read_seq;
    avx_read_t  * read_ptr;
    avx_write_t * result_ptr;
    __m256i     * dvdh_bit_mem;
    int64_t read_count;
    int64_t read_ptr_offset;
    int64_t result_ptr_offset;
    int     word_num;
    int     chunk_read_num;
    int     avx_ref_start;
    int     avx_ref_end;
    double * cal_total_time;
} avx_cal_t;


typedef struct _chunk_read_info {
    int chunk_read_num;
    int chunk_word_num;
    int rest_read_num;
    int rest_word_num;
} chunk_read_info;

extern seq_t read_seq_a;
extern  seq_t read_seq_b;
extern  seq_t * read_seq;
extern  seq_t   ref_seq;

extern __m256i           * avx_dvdh_bit_mem;

extern avx_read_t  * avx_preprocess_reads_a;
extern avx_read_t  * avx_preprocess_reads_b;
extern avx_read_t  * avx_read_ptr;

extern avx_write_t * avx_align_results_a;
extern avx_write_t * avx_align_results_b;
extern avx_write_t * avx_result_ptr;


void avx_cal_align_score(char * content, avx_read_t * preprocess_reads, avx_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num, __m256i * dvdh_bit_mem);

void cal_on_avx();

#endif
