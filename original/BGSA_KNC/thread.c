#include <stdarg.h>

#include "thread.h"
#include "file.h"
#include "timer.h"

void init_resources(int n, ...) {
	va_list arg_ptr;
	int i;
	va_start(arg_ptr, n);
	thread_info *tmp_info = NULL;

	for (i = 0; i < n; i++) {
		tmp_info = va_arg(arg_ptr, thread_info *);
		pthread_mutex_init(&(tmp_info->lock), NULL);
		pthread_cond_init(&(tmp_info->cond), NULL);
	}

	va_end(arg_ptr);
}

void free_resources(int n, ...) {
	va_list arg_ptr;
	int i;
	va_start(arg_ptr, n);
	thread_info *tmp_info = NULL;
	for (i = 0; i < n; i++) {
		tmp_info = va_arg(arg_ptr, thread_info *);
		pthread_mutex_destroy(&(tmp_info->lock));
		pthread_cond_destroy(&(tmp_info->cond));
	}
	va_end(arg_ptr);
}


void *input_task_cpu(void *args) {

	input_thread_arg *in_arg = (input_thread_arg *) args;

	FILE *fp = in_arg->fp;
	int64_t *result_bucket_counts = in_arg->result_bucket_counts;

	thread_info *input_info = in_arg->input_info;
	thread_info *cal_input_info = in_arg->cal_input_info;
	cpu_read_t *cpu_preprocess_reads_a = in_arg->cpu_preprocess_reads_a;
	cpu_read_t *cpu_preprocess_reads_b = in_arg->cpu_preprocess_reads_b;
	seq_t *read_seq_a = in_arg->read_seq_a;
	seq_t *read_seq_b = in_arg->read_seq_b;
	seq_t *seq;
	cpu_read_t *cpu_p_reads;

	int64_t total_size = in_arg->total_size;
	int bucket_num = in_arg->bucket_num;
	int cpu_word_num = in_arg->cpu_word_num;
	int64_t remain_size = total_size;
	int64_t *device_read_counts_a = in_arg->device_read_counts_a;
	int64_t *device_read_counts_b = in_arg->device_read_counts_b;
	int64_t *device_read_counts;

	int64_t bucket_size = in_arg->bucket_size;
	int64_t cpu_preprocess_read_size = in_arg->cpu_preprocess_read_size;

	double read_start, read_end;
	double mem_start, mem_end;

	pthread_mutex_t *input_lock = &input_info->lock;
	pthread_cond_t *input_cond = &input_info->cond;
	pthread_mutex_t *cal_lock = &cal_input_info->lock;
	pthread_cond_t *cal_cond = &cal_input_info->cond;

	int bucket_index = 1;
	while (1) {
		pthread_mutex_lock(input_lock);
		while (input_info->run_flag == 0 && !input_info->shutdown) {
			pthread_cond_wait(input_cond, input_lock);
		}
		if (input_info->shutdown) {
			break;
		}

		input_info->run_flag = 0;
		input_info->buffer_flag = 1 - input_info->buffer_flag;
		pthread_mutex_unlock(input_lock);

		if (input_info->buffer_flag) {
			remain_size -= read_seq_b->size;
			seq = read_seq_a;
			cpu_p_reads = cpu_preprocess_reads_a;
			device_read_counts = device_read_counts_a;
		} else {
			remain_size -= read_seq_a->size;
			seq = read_seq_b;
			cpu_p_reads = cpu_preprocess_reads_b;
			device_read_counts = device_read_counts_b;
		}

		if (bucket_index == bucket_num - 1) {
			bucket_size = remain_size;
		}

		GET_TIME(read_start);
		get_read_from_file(seq, fp, bucket_size, total_size);
		GET_TIME(read_end);
		read_total_time += read_end - read_start;
		//printf("read_end - read_start is %.2fs\n", read_end - read_start);

		result_bucket_counts[bucket_index++] = seq->count;
		device_read_counts[0] = seq->count;
		GET_TIME(mem_start);
		memset(cpu_p_reads, 0, cpu_preprocess_read_size);
		cpu_handle_reads(seq, cpu_p_reads, cpu_word_num, 0, seq->count);
		GET_TIME(mem_end);
		mem_total_time += mem_end - mem_start;

		pthread_mutex_lock(cal_lock);
		cal_input_info->run_flag = 1;
		pthread_mutex_unlock(cal_lock);
		pthread_cond_signal(cal_cond);
	}

	pthread_mutex_unlock(input_lock);

	return NULL;
}


void *output_task_cpu(void *args) {

	output_thread_arg *out_arg = (output_thread_arg *) args;

	FILE *fp = out_arg->fp;
	int64_t *cpu_result_count = out_arg->cpu_result_count;

	thread_info *output_info = out_arg->output_info;
	thread_info *cal_output_info = out_arg->cal_output_info;
	cpu_write_t *cpu_align_results_a = out_arg->cpu_align_results_a;
	cpu_write_t *cpu_align_results_b = out_arg->cpu_align_results_b;
	double write_start, write_end;

	while (1) {
		pthread_mutex_lock(&(output_info->lock));
		while (output_info->run_flag == 0 && !output_info->shutdown) {
			pthread_cond_wait(&(output_info->cond), &(output_info->lock));
		}
		if (output_info->shutdown) {
			break;
		}
		output_info->run_flag = 0;
		output_info->buffer_flag = 1 - output_info->buffer_flag;
		pthread_mutex_unlock(&(output_info->lock));

		GET_TIME(write_start);
		if (output_info->buffer_flag) {
			fwrite(cpu_align_results_a, sizeof(cpu_write_t), *cpu_result_count, fp);
			fflush(fp);

		} else {
			fwrite(cpu_align_results_b, sizeof(cpu_write_t), *cpu_result_count, fp);
			fflush(fp);
		}
		GET_TIME(write_end);
		write_total_time += write_end - write_start;

		pthread_mutex_lock(&(cal_output_info->lock));
		cal_output_info->run_flag = 1;
		pthread_mutex_unlock(&(cal_output_info->lock));
		pthread_cond_signal(&(cal_output_info->cond));
	}

	pthread_mutex_unlock(&(output_info->lock));
	return NULL;
}


void *input_task_sse(void *args) {

	input_thread_arg *in_arg = (input_thread_arg *) args;

	FILE *fp = in_arg->fp;
	int64_t *result_bucket_counts = in_arg->result_bucket_counts;

	thread_info *input_info = in_arg->input_info;
	thread_info *cal_input_info = in_arg->cal_input_info;
	sse_read_t *sse_preprocess_reads_a = in_arg->sse_preprocess_reads_a;
	sse_read_t *sse_preprocess_reads_b = in_arg->sse_preprocess_reads_b;
	seq_t *read_seq_a = in_arg->read_seq_a;
	seq_t *read_seq_b = in_arg->read_seq_b;
	seq_t *seq;
	sse_read_t *sse_p_reads;

	int64_t total_size = in_arg->total_size;
	int bucket_num = in_arg->bucket_num;
	int sse_word_num = in_arg->sse_word_num;
	int64_t remain_size = total_size;
	int64_t *device_read_counts_a = in_arg->device_read_counts_a;
	int64_t *device_read_counts_b = in_arg->device_read_counts_b;
	int64_t *device_read_counts;

	int64_t bucket_size = in_arg->bucket_size;
	int64_t sse_preprocess_read_size = in_arg->sse_preprocess_read_size;

	double read_start, read_end;
	double mem_start, mem_end;


	pthread_mutex_t *input_lock = &input_info->lock;
	pthread_cond_t *input_cond = &input_info->cond;
	pthread_mutex_t *cal_lock = &cal_input_info->lock;
	pthread_cond_t *cal_cond = &cal_input_info->cond;

	int bucket_index = 1;
	while (1) {
		pthread_mutex_lock(input_lock);
		while (input_info->run_flag == 0 && !input_info->shutdown) {
			pthread_cond_wait(input_cond, input_lock);
		}
		if (input_info->shutdown) {
			break;
		}

		input_info->run_flag = 0;
		input_info->buffer_flag = 1 - input_info->buffer_flag;
		pthread_mutex_unlock(input_lock);

		if (input_info->buffer_flag) {
			remain_size -= read_seq_b->size;
			seq = read_seq_a;
			sse_p_reads = sse_preprocess_reads_a;
			device_read_counts = device_read_counts_a;
		} else {
			remain_size -= read_seq_a->size;
			seq = read_seq_b;
			sse_p_reads = sse_preprocess_reads_b;
			device_read_counts = device_read_counts_b;
		}

		if (bucket_index == bucket_num - 1) {
			bucket_size = remain_size;
		}
		GET_TIME(read_start);
		get_read_from_file(seq, fp, bucket_size, total_size);
		GET_TIME(read_end);
		read_total_time += read_end - read_start;
		//printf("read_time is %.2fs\n", read_end - read_start);

		result_bucket_counts[bucket_index++] = seq->count;
		device_read_counts[0] = seq->count;

		GET_TIME(mem_start);
		memset(sse_p_reads, 0, sse_preprocess_read_size);
		sse_handle_reads(seq, sse_p_reads, sse_word_num, 0, seq->count);
		GET_TIME(mem_end);
		mem_total_time += mem_end - mem_start;

		pthread_mutex_lock(cal_lock);
		cal_input_info->run_flag = 1;
		pthread_mutex_unlock(cal_lock);
		pthread_cond_signal(cal_cond);
	}

	pthread_mutex_unlock(input_lock);

	return NULL;
}


void *output_task_sse(void *args) {

	output_thread_arg *out_arg = (output_thread_arg *) args;

	FILE *fp = out_arg->fp;
	int64_t *sse_result_count = out_arg->sse_result_count;

	thread_info *output_info = out_arg->output_info;
	thread_info *cal_output_info = out_arg->cal_output_info;
	sse_write_t *sse_align_results_a = out_arg->sse_align_results_a;
	sse_write_t *sse_align_results_b = out_arg->sse_align_results_b;
	double write_start, write_end;

	while (1) {
		pthread_mutex_lock(&(output_info->lock));
		while (output_info->run_flag == 0 && !output_info->shutdown) {
			pthread_cond_wait(&(output_info->cond), &(output_info->lock));
		}
		if (output_info->shutdown) {
			break;
		}
		output_info->run_flag = 0;
		output_info->buffer_flag = 1 - output_info->buffer_flag;
		pthread_mutex_unlock(&(output_info->lock));

		GET_TIME(write_start);
		if (output_info->buffer_flag) {
			fwrite(sse_align_results_a, sizeof(sse_write_t), *sse_result_count, fp);
			fflush(fp);

		} else {
			fwrite(sse_align_results_b, sizeof(sse_write_t), *sse_result_count, fp);
			fflush(fp);
		}
		GET_TIME(write_end);
		write_total_time += write_end - write_start;

		pthread_mutex_lock(&(cal_output_info->lock));
		cal_output_info->run_flag = 1;
		pthread_mutex_unlock(&(cal_output_info->lock));
		pthread_cond_signal(&(cal_output_info->cond));
	}

	pthread_mutex_unlock(&(output_info->lock));
	return NULL;
}


void *input_task_mic(void *args) {

	input_thread_arg *in_arg = (input_thread_arg *) args;

	FILE *fp = in_arg->fp;
	int64_t *result_bucket_counts = in_arg->result_bucket_counts;

	thread_info *input_info = in_arg->input_info;
	thread_info *cal_input_info = in_arg->cal_input_info;
	mic_read_t *preprocess_reads_a = in_arg->preprocess_reads_a;
	mic_read_t *preprocess_reads_b = in_arg->preprocess_reads_b;
	seq_t *read_seq_a = in_arg->read_seq_a;
	seq_t *read_seq_b = in_arg->read_seq_b;
	seq_t *seq;
	mic_read_t *p_reads;

	int64_t total_size = in_arg->total_size;
	int bucket_num = in_arg->bucket_num;
	int word_num = in_arg->word_num;
	int64_t remain_size = total_size;
	int64_t bucket_size = in_arg->bucket_size;
	int64_t preprocess_read_size = in_arg->preprocess_read_size;
	int64_t offload_read_size = in_arg->offload_read_size;
	double *device_conpute_radio = in_arg->device_compute_ratio;
	int64_t *device_read_counts_a = in_arg->device_read_counts_a;
	int64_t *device_read_counts_b = in_arg->device_read_counts_b;
	int64_t *device_read_count;
	int total_device_number = in_arg->total_device_number;
	int i, k;
	int mic_read_index;
	int single_read_count;
	int single_read_size;
	mic_read_t *p_tmp;
	double read_start, read_end;
	double mem_start, mem_end;


	pthread_mutex_t *input_lock = &input_info->lock;
	pthread_cond_t *input_cond = &input_info->cond;
	pthread_mutex_t *cal_lock = &cal_input_info->lock;
	pthread_cond_t *cal_cond = &cal_input_info->cond;

	int bucket_index = 1;
	while (1) {
		pthread_mutex_lock(input_lock);
		while (input_info->run_flag == 0 && !input_info->shutdown) {
			pthread_cond_wait(input_cond, input_lock);
		}
		if (input_info->shutdown) {
			break;
		}

		input_info->run_flag = 0;
		input_info->buffer_flag = 1 - input_info->buffer_flag;
		pthread_mutex_unlock(input_lock);

		if (input_info->buffer_flag) {
			remain_size -= read_seq_b->size;
			seq = read_seq_a;
			p_reads = preprocess_reads_a;
			device_read_count = device_read_counts_a;
		} else {
			remain_size -= read_seq_a->size;
			seq = read_seq_b;
			p_reads = preprocess_reads_b;
			device_read_count = device_read_counts_b;
		}

		if (bucket_index == bucket_num - 1) {
			bucket_size = remain_size;
		}

		GET_TIME(read_start);
		get_read_from_file(seq, fp, bucket_size, total_size);
		GET_TIME(read_end);
		read_total_time += read_end - read_start;
		dispatch_task(device_conpute_radio, device_read_count, seq->count, total_device_number);
		result_bucket_counts[bucket_index] = seq->count;
		bucket_index++;

		GET_TIME(mem_start);
		memset(p_reads, 0, preprocess_read_size);
		mic_handle_reads(seq, p_reads, word_num, 0, seq->count);
		GET_TIME(mem_end);
		mem_total_time += mem_end - mem_start;

		//mic_read_index = 0;
#pragma omp parallel for num_threads(total_device_number) default(none) \
            private(i, k, mic_read_index, single_read_count, single_read_size, p_tmp) \
            shared(up_signal_a, up_signal_b, total_device_number, word_num, p_reads, input_info, device_read_count)
		for (i = 0; i < total_device_number; i++) {
			mic_read_index = 0;
			for (k = 0; k < i; k++) {
				mic_read_index += device_read_count[k];
			}
			single_read_count = device_read_count[i];
			single_read_size = single_read_count * word_num * CHAR_NUM;
			p_tmp = &p_reads[mic_read_index * CHAR_NUM * word_num];

			if (input_info->buffer_flag) {
#pragma offload_transfer target(mic:i) signal( & up_signal_a[i]) \
                    in (p_tmp:length(single_read_size) REUSE RETAIN)
			} else {
#pragma offload_transfer target(mic:i) signal(& up_signal_b[i]) \
                    in (p_tmp:length(single_read_size) REUSE RETAIN)
			}
		}

		pthread_mutex_lock(cal_lock);
		cal_input_info->run_flag = 1;
		pthread_mutex_unlock(cal_lock);
		pthread_cond_signal(cal_cond);
	}

	pthread_mutex_unlock(input_lock);
	return NULL;

}


void *output_task_mic(void *args) {

	output_thread_arg *out_arg = (output_thread_arg *) args;

	FILE *fp = out_arg->fp;
	int64_t *result_count = out_arg->result_count;
	int64_t result_size = out_arg->result_size;

	thread_info *output_info = out_arg->output_info;
	thread_info *cal_output_info = out_arg->cal_output_info;
	mic_write_t *align_results_a = out_arg->align_results_a;
	mic_write_t *align_results_b = out_arg->align_results_b;
	int mic_device_number = out_arg->mic_device_number;

	double write_start, write_end;
	int i;

	while (1) {
		pthread_mutex_lock(&(output_info->lock));
		while (output_info->run_flag == 0 && !output_info->shutdown) {
			pthread_cond_wait(&(output_info->cond), &(output_info->lock));
		}
		if (output_info->shutdown) {
			break;
		}
		output_info->run_flag = 0;
		output_info->buffer_flag = 1 - output_info->buffer_flag;
		pthread_mutex_unlock(&(output_info->lock));


		if (output_info->buffer_flag) {
			for (i = 0; i < mic_device_number; i++) {
#pragma offload_wait target(mic:i) wait(& down_signal_a[i])
			}
			GET_TIME(write_start);
			fwrite(align_results_a, sizeof(mic_write_t), *result_count, fp);
			fflush(fp);
			GET_TIME(write_end);
			write_total_time += write_end - write_start;
		} else {
			for (i = 0; i < mic_device_number; i++) {
#pragma offload_wait target(mic:i) wait(& down_signal_b[i])
			}

			GET_TIME(write_start);
			fwrite(align_results_b, sizeof(mic_write_t), *result_count, fp);
			fflush(fp);
			GET_TIME(write_end);
			write_total_time += write_end - write_start;
		}

		pthread_mutex_lock(&(cal_output_info->lock));
		cal_output_info->run_flag = 1;
		pthread_mutex_unlock(&(cal_output_info->lock));
		pthread_cond_signal(&(cal_output_info->cond));
	}

	pthread_mutex_unlock(&(output_info->lock));
	return NULL;

}


void *input_task_all_dynamic(void *args) {

	input_thread_arg *in_arg = (input_thread_arg *) args;

	FILE *fp = in_arg->fp;

	thread_info *input_info = in_arg->input_info;
	thread_info *cal_input_info = in_arg->cal_input_info;
	seq_t *read_seq_a = in_arg->read_seq_a;
	seq_t *read_seq_b = in_arg->read_seq_b;
	seq_t *seq;
	mic_read_t *p_reads;
	sse_read_t *sse_p_reads;

	int64_t total_size = in_arg->total_size;
	int bucket_num = in_arg->bucket_num;

	int64_t remain_size = total_size;
	int64_t bucket_size = in_arg->bucket_size;


	double read_start, read_end;
	double mem_start, mem_end;

	pthread_mutex_t *input_lock = &input_info->lock;
	pthread_cond_t *input_cond = &input_info->cond;
	pthread_mutex_t *cal_lock = &cal_input_info->lock;
	pthread_cond_t *cal_cond = &cal_input_info->cond;

	int bucket_index = 1;
	while (1) {
		pthread_mutex_lock(input_lock);
		while (input_info->run_flag == 0 && !input_info->shutdown) {
			pthread_cond_wait(input_cond, input_lock);
		}
		if (input_info->shutdown) {
			break;
		}

		input_info->run_flag = 0;
		input_info->buffer_flag = 1 - input_info->buffer_flag;
		pthread_mutex_unlock(input_lock);

		if (input_info->buffer_flag) {
			remain_size -= read_seq_b->size;
			seq = read_seq_a;
		} else {
			remain_size -= read_seq_a->size;
			seq = read_seq_b;
		}

		if (bucket_index == bucket_num - 1) {
			bucket_size = remain_size;
		}

		GET_TIME(read_start);
		get_read_from_file(seq, fp, bucket_size, total_size);
		GET_TIME(read_end);
		read_total_time += read_end - read_start;

		pthread_mutex_lock(cal_lock);
		cal_input_info->run_flag = 1;
		pthread_mutex_unlock(cal_lock);
		pthread_cond_signal(cal_cond);
	}

	pthread_mutex_unlock(input_lock);
	return NULL;
}

void *input_task_all(void *args) {

	input_thread_arg *in_arg = (input_thread_arg *) args;

	FILE *fp = in_arg->fp;
	int64_t *result_bucket_counts = in_arg->result_bucket_counts;
	int64_t *sse_result_bucket_counts = in_arg->sse_result_bucket_counts;

	thread_info *input_info = in_arg->input_info;
	thread_info *cal_input_info = in_arg->cal_input_info;
	mic_read_t *preprocess_reads_a = in_arg->preprocess_reads_a;
	mic_read_t *preprocess_reads_b = in_arg->preprocess_reads_b;
	sse_read_t *sse_preprocess_reads_a = in_arg->sse_preprocess_reads_a;
	sse_read_t *sse_preprocess_reads_b = in_arg->sse_preprocess_reads_b;
	seq_t *read_seq_a = in_arg->read_seq_a;
	seq_t *read_seq_b = in_arg->read_seq_b;
	seq_t *seq;
	mic_read_t *p_reads;
	sse_read_t *sse_p_reads;

	int64_t total_size = in_arg->total_size;
	int bucket_num = in_arg->bucket_num;
	int word_num = in_arg->word_num;
	int sse_word_num = in_arg->sse_word_num;
	int64_t remain_size = total_size;
	int64_t bucket_size = in_arg->bucket_size;
	int64_t preprocess_read_size = in_arg->preprocess_read_size;
	int64_t sse_preprocess_read_size = in_arg->sse_preprocess_read_size;
	int64_t offload_read_size = in_arg->offload_read_size;
	double *device_compute_radio = in_arg->device_compute_ratio;
	int64_t *device_read_counts_a = in_arg->device_read_counts_a;
	int64_t *device_read_counts_b = in_arg->device_read_counts_b;
	int64_t *device_read_count;
	int total_device_number = in_arg->total_device_number;
	int i, k;
	int mic_read_index;
	int single_read_count;
	int single_read_size;
	mic_read_t *p_tmp;
	double read_start, read_end;
	double mem_start, mem_end;

	pthread_mutex_t *input_lock = &input_info->lock;
	pthread_cond_t *input_cond = &input_info->cond;
	pthread_mutex_t *cal_lock = &cal_input_info->lock;
	pthread_cond_t *cal_cond = &cal_input_info->cond;

	int bucket_index = 1;
	while (1) {
		pthread_mutex_lock(input_lock);
		while (input_info->run_flag == 0 && !input_info->shutdown) {
			pthread_cond_wait(input_cond, input_lock);
		}
		if (input_info->shutdown) {
			break;
		}

		input_info->run_flag = 0;
		input_info->buffer_flag = 1 - input_info->buffer_flag;
		pthread_mutex_unlock(input_lock);

		if (input_info->buffer_flag) {
//			printf("1111\n");
			remain_size -= read_seq_b->size;
			seq = read_seq_a;
			p_reads = preprocess_reads_a;
			sse_p_reads = sse_preprocess_reads_a;
			device_read_count = device_read_counts_a;
		} else {
//			printf("2222\n");
			remain_size -= read_seq_a->size;
			seq = read_seq_b;
			p_reads = preprocess_reads_b;
			sse_p_reads = sse_preprocess_reads_b;
			device_read_count = device_read_counts_b;
		}

		if (bucket_index == bucket_num - 1) {
			bucket_size = remain_size;
		}

		//adjust_device_ratio(device_compute_radio, total_device_number);
		GET_TIME(read_start);
		get_read_from_file(seq, fp, bucket_size, total_size);
		GET_TIME(read_end);
		read_total_time += read_end - read_start;
		dispatch_task(device_compute_radio, device_read_count, seq->count, total_device_number);
		//fwrite(device_read_count, sizeof(int64_t), total_device_number, fp_result_info);
		sse_result_bucket_counts[bucket_index] = device_read_count[0];
		result_bucket_counts[bucket_index] = seq->count - device_read_count[0];
		bucket_index++;

		GET_TIME(mem_start);
		memset(p_reads, 0, preprocess_read_size);
		memset(sse_p_reads, 0, sse_preprocess_read_size);
		sse_handle_reads(seq, sse_p_reads, sse_word_num, 0, device_read_count[0]);
		mic_handle_reads(seq, p_reads, word_num, device_read_count[0], seq->count - device_read_count[0]);
		GET_TIME(mem_end);
		mem_total_time += mem_end - mem_start;

		//mic_read_index = 0;
#pragma omp parallel for num_threads(total_device_number-1) default(none) \
            private(i) \
            shared(up_signal_a, up_signal_b, total_device_number, word_num, p_reads, input_info, device_read_count)
		for (i = 1; i < total_device_number; i++) {
			int mic_read_index = 0;
			int k;
			int single_read_count;
			int single_read_size;
			mic_read_t *p_tmp;
			for (k = 1; k < i; k++) {
				mic_read_index += device_read_count[k];
			}
			single_read_count = device_read_count[i];
			single_read_size = single_read_count * word_num * CHAR_NUM;
			p_tmp = &p_reads[mic_read_index * CHAR_NUM * word_num];

			if (input_info->buffer_flag) {
#pragma offload_transfer target(mic:(i-1)) signal( & up_signal_a[i-1]) \
                    in (p_tmp:length(single_read_size) REUSE RETAIN)
			} else {
#pragma offload_transfer target(mic:(i-1)) signal(& up_signal_b[i-1]) \
                    in (p_tmp:length(single_read_size) REUSE RETAIN)
			}
		}

		pthread_mutex_lock(cal_lock);
		cal_input_info->run_flag = 1;
		pthread_mutex_unlock(cal_lock);
		pthread_cond_signal(cal_cond);
	}

	pthread_mutex_unlock(input_lock);
	return NULL;
}


void *output_task_all(void *args) {
	output_thread_arg *out_arg = (output_thread_arg *) args;

	FILE *fp = out_arg->fp;
	int64_t *result_count = out_arg->result_count;
	int64_t *sse_result_count = out_arg->sse_result_count;
	int64_t result_size = out_arg->result_size;

	thread_info *output_info = out_arg->output_info;
	thread_info *cal_output_info = out_arg->cal_output_info;
	mic_write_t *align_results_a = out_arg->align_results_a;
	mic_write_t *align_results_b = out_arg->align_results_b;
	sse_write_t *sse_align_results_a = out_arg->sse_align_results_a;
	sse_write_t *sse_align_results_b = out_arg->sse_align_results_b;
	int mic_device_number = out_arg->mic_device_number;
	int i;
	double write_start, write_end;

	while (1) {
		pthread_mutex_lock(&(output_info->lock));
		while (output_info->run_flag == 0 && !output_info->shutdown) {
			pthread_cond_wait(&(output_info->cond), &(output_info->lock));
		}
		if (output_info->shutdown) {
			break;
		}
		output_info->run_flag = 0;
		output_info->buffer_flag = 1 - output_info->buffer_flag;
		pthread_mutex_unlock(&(output_info->lock));

		if (output_info->buffer_flag) {
			for (i = 0; i < mic_device_number; i++) {
#pragma offload_wait target(mic:i) wait(& down_signal_a[i])
			}
			GET_TIME(write_start);
			fwrite(sse_align_results_a, sizeof(sse_write_t), *sse_result_count, fp);
			//printf("sse_align_results_a[0] is %d\n", sse_align_results_a[0]);
			//printf("sse_align_results_a[1] is %d\n", sse_align_results_a[1]);
			// for(i = 0; i < 10; i++) {
			//     printf("align_results_a is %d\n", sse_align_results_a[i]);
			// }
			fwrite(align_results_a, sizeof(mic_write_t), *result_count, fp);
			// printf("align_results_a[0] is %d\n", align_results_a[0]);
			// printf("align_results_a[1] is %d\n", align_results_a[1]);
			fflush(fp);
			GET_TIME(write_end);
			write_total_time += write_end - write_start;
		} else {
			for (i = 0; i < mic_device_number; i++) {
#pragma offload_wait target(mic:i) wait(& down_signal_b[i])
			}
			// for(i = 0; i < 10; i++) {
			//     printf("sse_align_results_a is %d\n", sse_align_results_a[i]);
			// }
			GET_TIME(write_start);
			fwrite(sse_align_results_b, sizeof(sse_write_t), *sse_result_count, fp);
			//printf("sse_align_results_b[0] is %d\n", sse_align_results_b[0]);
			//printf("sse_align_results_b[1] is %d\n", sse_align_results_b[1]);
			fwrite(align_results_b, sizeof(mic_write_t), *result_count, fp);
			GET_TIME(write_end);

			fflush(fp);
			write_total_time += write_end - write_start;
		}

		pthread_mutex_lock(&(cal_output_info->lock));
		cal_output_info->run_flag = 1;
		pthread_mutex_unlock(&(cal_output_info->lock));
		pthread_cond_signal(&(cal_output_info->cond));
	}

	pthread_mutex_unlock(&(output_info->lock));
	return NULL;
}
