#ifndef CAL_H
#define CAL_H

#include <stdint.h>
#include "global.h"


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



extern seq_t read_seq_a;
extern seq_t read_seq_b;
extern seq_t * read_seq;
extern seq_t   ref_seq;

extern uint64_t          * cpu_dvdh_bit_mem;

extern cpu_read_t  * cpu_preprocess_reads_a;
extern cpu_read_t  * cpu_preprocess_reads_b;
extern cpu_read_t  * cpu_read_ptr;

extern cpu_write_t * cpu_align_results_a;
extern cpu_write_t * cpu_align_results_b;
extern cpu_write_t * cpu_result_ptr;

void cpu_cal_align_score(char * content, cpu_read_t * preprocess_reads, cpu_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num, uint64_t * dvdh_bit_mem);

void cal_on_cpu();

#endif
