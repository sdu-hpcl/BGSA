#include <stdio.h>
#include "global.h"
#include "timer.h"
#include "util.h"
#include "file.h"

uint32_t mapping_table[128] __attribute__((aligned(64)));

const uint32_t one = 1;


//double mic_cpu_ratio = 4.9;

void init_mapping_table() {
	mapping_table[(int) 'A'] = 0;
	mapping_table[(int) 'C'] = 1;
	mapping_table[(int) 'G'] = 2;
	mapping_table[(int) 'T'] = 3;
	mapping_table[(int) 'N'] = 4;
}

void init_cpu_mic_ratio(double *device_ratio, int cpu_number, int mic_number) {
	int i;

	for (i = 0; i < cpu_number; i++) {
		device_ratio[i] = 1;
	}

	for (i = cpu_number; i < (cpu_number + mic_number); i++) {
		device_ratio[i] = mic_cpu_ratio;
	}
}

void init_device_ratio_file(double *device_ratio, int cpu_number, int mic_number, char *file_ratio) {

	//printf("cpu_number is %d\n", cpu_number);
	//printf("mic_number is %d\n", mic_number);
	FILE *fp = open_file(file_ratio, "r");
	int i;
	for (i = 0; i < cpu_number + mic_number; i++) {
		fscanf(fp, "%lf", device_ratio + i);
		//printf("2222\n");
		if (device_ratio[i] <= 0) {
			printf("device ratio can't be zero or negative\n");
			fclose(fp);
			exit(1);
		}
	}


	fclose(fp);

}

void init_mic_ratio(double *device_ratio, int mic_number) {
	int i;
	for (i = 0; i < mic_number; i++) {
		device_ratio[i] = 1;
	}
}

void adjust_device_ratio(double *device_compute_ratio, double *previous_times, int total_device_number) {
	int i;
	printf("time-index: %d\n", time_index);
	for (i = 1; i < total_device_number; i++) {
		device_compute_ratio[i] = device_compute_ratio[i] * previous_times[0] / previous_times[i] + 0.1;

		printf("device_compute_ratio[%d] is %.2f\n", i - 1, device_compute_ratio[i]);
	}

}

void adjust_device_ratio2(double *device_compute_ratio, double *previous_times, int total_device_number) {
	int i, j;
	double tmp_ratio[total_device_number];

	for (i = 0; i < total_device_number; i++) {
		tmp_ratio[i] = 0;
	}


	printf("time-index: %d\n", time_index);
	if (time_index == 1) {
		for (i = 1; i < total_device_number; i++) {
			device_compute_ratio[i] = device_compute_ratio[i] * previous_times[0] / previous_times[i];
		}
	} else {
		for (i = 0; i < total_device_number; i++) {
			loop_used_times[time_index - 1][i] = previous_times[i];
		}
		for (i = 1; i < total_device_number; i++) {
			device_compute_ratio[i] = device_compute_ratio[i] * previous_times[0] / previous_times[i];
			printf("device_compute_ratio[%d] is %.2f\n", i - 1, device_compute_ratio[i]);
		}

		for (i = 0; i < time_index - 1; i++) {
			for (j = 1; j < total_device_number; j++) {
				tmp_ratio[j] += loop_device_ratio[i][j];
			}
		}

		for (i = 1; i < total_device_number; i++) {
			tmp_ratio[i] = tmp_ratio[i] / (time_index - 1);
			device_compute_ratio[i] = (device_compute_ratio[i] + tmp_ratio[i]) / 2;
		}
	}


	for (i = 0; i < total_device_number; i++) {
		printf("device_compute_ratio[%d] is %.2f\n", i, device_compute_ratio[i]);
	}

	for (i = 0; i < total_device_number; i++) {
		loop_device_ratio[time_index - 1][i] = device_compute_ratio[i];
	}

}


void adjust_device_ratio3(double *device_compute_ratio, double *previous_times, int total_device_number) {

	int i, j;
	double tmp_ratio[total_device_number];
	int total = 0;
	for (i = 0; i < total_device_number; i++) {
		tmp_ratio[i] = 0;
	}

	device_compute_ratio[0] = 1;

//    printf("time-index: %d\n", time_index);
	if (time_index == 1) {
		for (i = 1; i < total_device_number; i++) {
			device_compute_ratio[i] = device_compute_ratio[i] * previous_times[0] / previous_times[i];
		}
	} else {
		for (i = 0; i < total_device_number; i++) {
			loop_used_times[time_index - 1][i] = previous_times[i];
		}
		for (i = 1; i < total_device_number; i++) {
			device_compute_ratio[i] = device_compute_ratio[i] * previous_times[0] / previous_times[i];
//            printf("device_compute_ratio[%d] is %.2f\n",i-1, device_compute_ratio[i]);
		}

		for (i = 1; i < time_index - 1; i++) {
			for (j = 1; j < total_device_number; j++) {
				tmp_ratio[j] += loop_device_ratio[i][j] * (i + 1);
			}
			total += (i + 1);
		}

		total += time_index;
		for (i = 1; i < total_device_number; i++) {
			tmp_ratio[i] += device_compute_ratio[i] * (time_index);
			device_compute_ratio[i] = tmp_ratio[i] / total;
		}
	}


//    for(i = 0; i < total_device_number; i++) {
//        printf("device_compute_ratio[%d] is %.2f\n", i, device_compute_ratio[i]);
//    }

	for (i = 0; i < total_device_number; i++) {
		loop_device_ratio[time_index - 1][i] = device_compute_ratio[i];
	}

}

// 初始化设备
void init_device(int mic_device_number) {
	int i;
	for (i = 0; i < mic_device_number; i++) {
#pragma offload_transfer target(mic:i)
	}
}

__ONMIC__ void *malloc_mem(uint64_t size) {
	return _mm_malloc(size, 64);
}

__ONMIC__ void free_mem(void *mem) {
	_mm_free(mem);
}

double process_start;
double process_end;

void mic_handle_reads(seq_t *read_seq, mic_read_t *result_reads, int word_num, int64_t read_start, int64_t read_count) {
	int loop_num = read_count / MIC_V_NUM;
	int loop_index = 0;
	int loop_interval = MIC_V_NUM * (read_seq->len + 1);
	char *itr = read_seq->content + (read_seq->len + 1) * read_start;
	int start_index = 0;
	int p_index = 0;
	int char_value = 0;
	uint64_t bitmask;
	int i, j;

	GET_TIME(process_start);
	// processed_reads内存分配
#pragma omp parallel for default(none) num_threads(cpu_threads) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num) \
    schedule(guided)
	for (loop_index = 0; loop_index < loop_num; loop_index++) {

		bitmask = one;
		start_index = loop_interval * loop_index;
		p_index = CHAR_NUM * MIC_V_NUM * loop_index * word_num;
		int bit_index = 0;
		int char_index = 0;
		for (i = 0; i < read_seq->len; i++) {
			if (bit_index == MIC_WORD_SIZE) {
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


void sse_handle_reads(seq_t *read_seq, sse_read_t *result_reads, int word_num, int64_t read_start, int64_t read_count) {
	int loop_num = read_count / SSE_V_NUM;
	int loop_index = 0;
	int loop_interval = SSE_V_NUM * (read_seq->len + 1);
	char *itr = read_seq->content + (read_seq->len + 1) * read_start;
	int start_index = 0;
	int p_index = 0;
	int char_value = 0;
	uint32_t bitmask;
	int i, j;
	int word_size = SSE_WORD_SIZE - 1;
	if (full_bits) {
		word_size = SSE_WORD_SIZE;
	}

	// processed_reads内存分配
#pragma omp parallel for default(none) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num, word_size) \
    schedule(guided)
	for (loop_index = 0; loop_index < loop_num; loop_index++) {

		bitmask = one;
		start_index = loop_interval * loop_index;
		p_index = CHAR_NUM * SSE_V_NUM * loop_index * word_num;
		int bit_index = 0;
		int char_index = 0;
		for (i = 0; i < read_seq->len; i++) {
			if (bit_index == word_size) {
				bitmask = one;
				bit_index = 0;
				char_index++;
			}

			for (j = 0; j < SSE_V_NUM; j++) {
				char_value = mapping_table[(int) itr[start_index + (read_seq->len + 1) * j]];
				result_reads[p_index + char_value * word_num * SSE_V_NUM + char_index * SSE_V_NUM + j] |= bitmask;
			}

			bitmask <<= 1;
			start_index++;
			bit_index++;
		}
	}
}

void cpu_handle_reads(seq_t *read_seq, cpu_read_t *result_reads, int word_num, int64_t read_start, int64_t read_count) {
	int loop_num = read_count / CPU_V_NUM;
	int loop_index = 0;
	int loop_interval = CPU_V_NUM * (read_seq->len + 1);
	char *itr = read_seq->content + (read_seq->len + 1) * read_start;
	int start_index = 0;
	int p_index = 0;
	int char_value = 0;
	uint64_t bitmask;
	int i, j;
	int word_size = CPU_WORD_SIZE - 1;
	if (full_bits) {
		word_size = CPU_WORD_SIZE;
	}

	// processed_reads内存分配
#pragma omp parallel for default(none) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num, word_size) \
    schedule(guided)
	for (loop_index = 0; loop_index < loop_num; loop_index++) {

		bitmask = one;
		start_index = loop_interval * loop_index;
		p_index = CHAR_NUM * CPU_V_NUM * loop_index * word_num;
		int bit_index = 0;
		int char_index = 0;
		for (i = 0; i < read_seq->len; i++) {
			if (bit_index == word_size) {
				bitmask = one;
				bit_index = 0;
				char_index++;
			}

			for (j = 0; j < CPU_V_NUM; j++) {
				char_value = mapping_table[(int) itr[start_index + (read_seq->len + 1) * j]];
				result_reads[p_index + char_value * word_num * CPU_V_NUM + char_index * CPU_V_NUM + j] |= bitmask;
			}

			bitmask <<= 1;
			start_index++;
			bit_index++;
		}
	}


}


void
single_handle_reads(seq_t *read_seq, cpu_read_t *result_reads, int word_num, int64_t read_start, int64_t read_count) {
	int loop_num = read_count / CPU_V_NUM;
	int loop_index = 0;
	int loop_interval = CPU_V_NUM * (read_seq->len + 1);
	char *itr = read_seq->content + (read_seq->len + 1) * read_start;
	int start_index = 0;
	int p_index = 0;
	int char_value = 0;
	uint64_t bitmask;
	int i, j;


	// processed_reads内存分配
//#pragma omp parallel for default(none) \
    private(loop_index, bitmask, start_index, p_index, char_value, i, j) \
    shared(loop_num, read_seq, itr, loop_interval, result_reads, mapping_table, word_num) \
    schedule(guided)
	for (loop_index = 0; loop_index < loop_num; loop_index++) {

		bitmask = one;
		start_index = loop_interval * loop_index;
		p_index = CHAR_NUM * CPU_V_NUM * loop_index * word_num;
		int bit_index = 0;
		int char_index = 0;
		for (i = 0; i < read_seq->len; i++) {
			if (bit_index == CPU_WORD_SIZE - 1) {
				bitmask = one;
				bit_index = 0;
				char_index++;
			}

			for (j = 0; j < CPU_V_NUM; j++) {
				char_value = mapping_table[(int) itr[start_index + (read_seq->len + 1) * j]];
				result_reads[p_index + char_value * word_num * CPU_V_NUM + char_index * CPU_V_NUM + j] |= bitmask;
			}

			bitmask <<= 1;
			start_index++;
			bit_index++;
		}
	}
}


void
dispatch_task(double *device_ratio, int64_t *device_read_counts, int64_t total_read_count, int total_device_number) {

	int i;
	double total_radio = 0.0;
	for (i = 0; i < total_device_number; i++) {
		total_radio += device_ratio[i];
	}
	double unit_count_double = total_read_count / total_radio;
	int64_t unit_count_int = (int64_t) unit_count_double;
	while ((unit_count_int % MIC_V_NUM) != 0) {
		unit_count_int--;
	}
	int64_t dispatched_count = 0;
	int64_t single_count;

	for (i = 0; i < total_device_number; i++) {
		single_count = unit_count_int * device_ratio[i];
		while (((single_count % MIC_V_NUM) != 0) || (single_count == 0)) {
			single_count++;
		}
		dispatched_count += single_count;
		if (dispatched_count > total_read_count) {
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
	if (rest_count > 0) {
		unit_count_int = rest_count / total_radio;
		while ((unit_count_int % MIC_V_NUM) != 0) {
			unit_count_int--;
		}
		for (i = 0; i < total_device_number; i++) {
			if (i == total_device_number - 1) {
				device_read_counts[i] += rest_count - dispatched_count;
			} else {
				single_count = unit_count_int * device_ratio[i];
				while (((single_count % MIC_V_NUM) != 0) || (single_count == 0)) {
					single_count++;
				}
				dispatched_count += single_count;
				if (dispatched_count > rest_count) {
					dispatched_count -= single_count;
					device_read_counts[i] += rest_count - dispatched_count;
					break;
				}
				device_read_counts[i] += single_count;
			}

		}
	}
}
