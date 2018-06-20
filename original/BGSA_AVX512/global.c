#include <stdio.h>
#include "global.h"
#include "timer.h"
#include "util.h"
#include "file.h"

uint32_t mapping_table[128] __attribute__((aligned(64)));

const mic_read_t one = 1;


void init_mapping_table() {
	mapping_table[(int) 'A'] = 0;
	mapping_table[(int) 'C'] = 1;
	mapping_table[(int) 'G'] = 2;
	mapping_table[(int) 'T'] = 3;
	mapping_table[(int) 'N'] = 4;
}

void *malloc_mem(uint64_t size) {
	return _mm_malloc(size, 64);
}

void free_mem(void *mem) {
	_mm_free(mem);
}

void init_mic_ratio(double *device_ratio, int mic_number) {
	int i;
	for (i = 0; i < mic_number; i++) {
		device_ratio[i] = 1;
	}
}

double process_start;
double process_end;

void mic_handle_reads(seq_t *read_seq, mic_read_t *result_reads, int word_num, int64_t read_start, int64_t read_count) {
	//printf("read_count is %d\n", read_count);
	int loop_num = read_count / MIC_V_NUM;
	int loop_index = 0;
	int loop_interval = MIC_V_NUM * (read_seq->len + 1);
	char *itr = read_seq->content + (read_seq->len + 1) * read_start;
	int start_index = 0;
	int p_index = 0;
	int char_value = 0;
	uint64_t bitmask;
	int i, j;
	int word_size = MIC_WORD_SIZE - 1;
	if (full_bits) {
		word_size = MIC_WORD_SIZE;
	}

	GET_TIME(process_start);
	// processed_reads内存分配
#pragma omp parallel for default(none) num_threads(cpu_threads) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num, word_size) \
    schedule(guided)
	for (loop_index = 0; loop_index < loop_num; loop_index++) {

		bitmask = one;
		start_index = loop_interval * loop_index;
		p_index = CHAR_NUM * MIC_V_NUM * loop_index * word_num;
		int bit_index = 0;
		int char_index = 0;
		for (i = 0; i < read_seq->len; i++) {
			if (bit_index == word_size) {
				bitmask = one;
				bit_index = 0;
				char_index++;
			}

			for (j = 0; j < MIC_V_NUM; j++) {
				char_value = mapping_table[(int) itr[start_index + (read_seq->len + 1) * j]];
				result_reads[p_index + char_value * word_num * MIC_V_NUM + char_index * MIC_V_NUM + j] |= bitmask;
			}
			bitmask <<= 1;
			start_index++;
			bit_index++;
		}
	}
	GET_TIME(process_end);
}

void dispatch_task(double * device_ratio, int64_t * device_read_counts, int64_t total_read_count, int total_device_number) {

	/*device_read_counts[0] = total_read_count / 2;
	device_read_counts[1] = total_read_count - device_read_counts[0];
	return;*/

	int i;
	double total_radio = 0.0;
	for(i = 0; i < total_device_number; i++) {
		total_radio += device_ratio[i];
	}
	double unit_count_double = total_read_count / total_radio;
	int64_t unit_count_int = (int64_t) unit_count_double;
	while((unit_count_int % MIC_V_NUM) != 0) {
		unit_count_int--;
	}
	int64_t dispatched_count = 0;
	int64_t single_count;
	/*for(i = 0; i < total_device_number; i++) {
		if(i == total_device_number - 1) {
			device_read_counts[i] = total_read_count - dispatched_count;
			if(total_device_number > 2) {
				int sub = device_read_counts[i] - device_read_counts[i - 1];
				if(sub > 32) {
					sub = sub / 2;
					while(sub % 16 != 0) {
						sub++;
					}
					device_read_counts[i] -= sub;
					device_read_counts[i- 1] += sub;
				}
			}
		} else {
			single_count = unit_count_int * device_radio[i];
			while((single_count % MIC_V_NUM) != 0) {
				single_count++;
			}
			device_read_counts[i] = single_count;
			dispatched_count += single_count;
		}
	}*/

	for(i = 0; i < total_device_number; i++) {
		single_count = unit_count_int * device_ratio[i];
		while(((single_count % MIC_V_NUM) != 0) || (single_count == 0)) {
			single_count++;
		}
		dispatched_count += single_count;
		if(dispatched_count > total_read_count) {
			dispatched_count -= single_count;
			device_read_counts[i] = total_read_count - dispatched_count;
			dispatched_count = total_read_count;
			break;
		}
		device_read_counts[i] = single_count;
	}

	// 对于剩余的再分配一次
	int64_t rest_count = total_read_count - dispatched_count;
	dispatched_count = 0;
	if(rest_count > 0) {
		unit_count_int = rest_count / total_radio;
		while((unit_count_int % MIC_V_NUM) != 0) {
			unit_count_int--;
		}
		for(i = 0; i < total_device_number; i++) {
			if(i == total_device_number - 1) {
				device_read_counts[i] += rest_count - dispatched_count;
			} else {
				single_count = unit_count_int * device_ratio[i];
				while(((single_count % MIC_V_NUM) != 0) || (single_count == 0)) {
					single_count++;
				}
				dispatched_count += single_count;
				if(dispatched_count > rest_count) {
					dispatched_count -= single_count;
					device_read_counts[i] += rest_count - dispatched_count;
					break;
				}
				device_read_counts[i] += single_count;
			}

		}
	}


	/*for(i = 0; i < total_device_number; i++) {
		printf("device read counts[%d] is %d\n", i, device_read_counts[i]);
	}*/

}
