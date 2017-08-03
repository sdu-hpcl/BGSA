#include <stdio.h>
#include "global.h"
#include "timer.h"
#include "util.h"
#include "file.h"

uint32_t mapping_table[128] __attribute__((aligned(64)));

const uint32_t one = 1;

void init_mapping_table() {
	mapping_table[(int)'A'] = 0;
    mapping_table[(int)'C'] = 1;
    mapping_table[(int)'G'] = 2;
    mapping_table[(int)'T'] = 3;
    mapping_table[(int)'N'] = 4;
}

void * malloc_mem(uint64_t size) {
    return _mm_malloc(size, 64);
}

void free_mem(void * mem) {
    _mm_free(mem);
}

void cpu_handle_reads(seq_t * read_seq, cpu_read_t * result_reads, int word_num, int64_t read_start, int64_t read_count) {
    int loop_num = read_count / CPU_V_NUM;
    int loop_index = 0;
    int loop_interval = CPU_V_NUM * (read_seq->len + 1);
    char * itr = read_seq->content + (read_seq->len + 1) * read_start;
    int start_index = 0;
    int p_index = 0;
    int char_value = 0;
    uint64_t bitmask;
    int i, j;


   // processed_reads内存分配
#pragma omp parallel for default(none) num_threads(cpu_threads) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num) \
    schedule(guided)
    for(loop_index = 0; loop_index < loop_num; loop_index++) {

        bitmask = one;
        start_index = loop_interval * loop_index;
        p_index = CHAR_NUM * CPU_V_NUM * loop_index * word_num;
        int bit_index = 0;
        int char_index = 0;
        for(i = 0; i < read_seq->len; i++) {
            if(bit_index == CPU_WORD_SIZE - 1) {
                bitmask = one;
                bit_index = 0;
                char_index++;
            }

            for(j = 0; j < CPU_V_NUM; j++) {
                char_value = mapping_table[(int)itr[start_index + (read_seq->len + 1) * j]];
                result_reads[p_index + char_value * word_num * CPU_V_NUM + char_index * CPU_V_NUM + j] |= bitmask;
            }

            bitmask <<= 1;
            start_index++;
            bit_index++;
        }
    }
}
