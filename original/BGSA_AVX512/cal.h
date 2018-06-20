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

typedef struct _chunk_read_info {
    int chunk_read_num;
    int chunk_word_num;
    int rest_read_num;
    int rest_word_num;
} chunk_read_info;

extern mic_read_t  * preprocess_reads_a;
extern mic_read_t  * preprocess_reads_b;
extern mic_read_t  * read_ptr;

extern mic_write_t * align_results_a;
extern mic_write_t * align_results_b;
extern mic_write_t * result_ptr;


extern seq_t read_seq_a;
extern seq_t read_seq_b;
extern seq_t * read_seq;
extern seq_t   ref_seq;

extern __m512i * dvdh_bit_mem;

void mic_cal_align_score(char * content, mic_read_t * preprocess_reads, mic_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num,  __m512i * dvdh_bit_mem);

void cal_on_mic();



#endif
