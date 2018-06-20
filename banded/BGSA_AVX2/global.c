#include <stdio.h>
#include "global.h"
#include "timer.h"
#include "file.h"

uint32_t mapping_table[128] __attribute__((aligned(64)));

const avx_read_t one = (avx_read_t)1;

void init_mapping_table() {
	mapping_table[(int)'A'] = 0;
    mapping_table[(int)'C'] = 1;
    mapping_table[(int)'G'] = 2;
    mapping_table[(int)'T'] = 3;
    mapping_table[(int)'N'] = 4;
}

// 初始化设备
void init_device(int mic_device_number) {
    int i;
    for(i = 0; i < mic_device_number; i++) {
       // #pragma offload_transfer target(mic:i)
    }
}

void * malloc_mem(uint64_t size) {
    return _mm_malloc(size, 64);
}

void free_mem(void * mem) {
    _mm_free(mem);
}


void avx_handle_reads(seq_t * read_seq, avx_read_t * result_reads, int word_num, int64_t read_start, int64_t read_count) {
    int loop_num = read_count / AVX_V_NUM;
    int loop_index = 0;
    int loop_interval = AVX_V_NUM * (read_seq->len + 1);
    char * itr = read_seq->content + (read_seq->len + 1) * read_start;
    int start_index = 0;
    int p_index = 0;
    int char_value = 0;
    avx_read_t bitmask;
    int i, j;
    int word_size = AVX_WORD_SIZE;

   // processed_reads内存分配
#pragma omp parallel for default(none) num_threads(cpu_threads) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num, word_size, threshold) \
    schedule(guided)
    for(loop_index = 0; loop_index < loop_num; loop_index++) {

        bitmask = one << (threshold + 1);
        start_index = loop_interval * loop_index;
        p_index = CHAR_NUM * AVX_V_NUM * loop_index * word_num;
        int bit_index = 0;
        int char_index = 0;

        for(int i = 0; i < threshold; i++) {

            for(j = 0; j < AVX_V_NUM; j++) {
                char_value = mapping_table[(int)itr[start_index + (read_seq->len + 1) * j]];
                result_reads[p_index + char_value * word_num * AVX_V_NUM + char_index * AVX_V_NUM + j] |= bitmask;
            }

            bitmask <<= 1;
            start_index++;
            bit_index++;
        } 

        bitmask = one;
        bit_index = 0;
        char_index++;

        for(i = threshold; i < read_seq->len; i++) {
            if(bit_index == word_size) {
                bitmask = one;
                bit_index = 0;
                char_index++;
            }

            for(j = 0; j < AVX_V_NUM; j++) {
                char_value = mapping_table[(int)itr[start_index + (read_seq->len + 1) * j]];
                result_reads[p_index + char_value * word_num * AVX_V_NUM + char_index * AVX_V_NUM + j] |= bitmask;
            }

            bitmask <<= 1;
            start_index++;
            bit_index++;
        }
    }
}
