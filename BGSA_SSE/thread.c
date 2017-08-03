#include "thread.h"
#include "file.h"
#include "timer.h"

void init_resources(int n, ...) {
	va_list arg_ptr ;
	int i;
	va_start(arg_ptr, n);
	thread_info * tmp_info = NULL;
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
	thread_info * tmp_info = NULL;
	for (i = 0; i < n; i++) {
		tmp_info = va_arg(arg_ptr, thread_info *);
		pthread_mutex_destroy(&(tmp_info->lock));
		pthread_cond_destroy(&(tmp_info->cond));
	}
	va_end(arg_ptr);
}

void * input_task_sse(void * args) {

    input_thread_arg * in_arg = (input_thread_arg *) args;

    FILE    * fp = in_arg->fp;
    int64_t * result_bucket_counts = in_arg->result_bucket_counts;

    thread_info * input_info = in_arg->input_info;
    thread_info * cal_input_info = in_arg->cal_input_info;
    sse_read_t  * sse_preprocess_reads_a = in_arg->sse_preprocess_reads_a;
    sse_read_t  * sse_preprocess_reads_b = in_arg->sse_preprocess_reads_b;
    seq_t       * read_seq_a = in_arg->read_seq_a;
    seq_t       * read_seq_b = in_arg->read_seq_b;
    seq_t       * seq;
    sse_read_t  * sse_p_reads;

    int64_t total_size = in_arg->total_size;
    int     bucket_num = in_arg->bucket_num;
    int     sse_word_num = in_arg->sse_word_num;
    int64_t remain_size = total_size;
    int64_t * device_read_counts_a = in_arg->device_read_counts_a;
    int64_t * device_read_counts_b = in_arg->device_read_counts_b;
    int64_t * device_read_counts;

    int64_t bucket_size = in_arg->bucket_size;
    int64_t sse_preprocess_read_size = in_arg->sse_preprocess_read_size;

    double read_start, read_end;
    double mem_start, mem_end;


    pthread_mutex_t * input_lock = & input_info->lock;
    pthread_cond_t  * input_cond = & input_info->cond;
    pthread_mutex_t * cal_lock = & cal_input_info->lock;
    pthread_cond_t  * cal_cond = & cal_input_info->cond;

    int bucket_index = 1;
    while(1) {
        pthread_mutex_lock(input_lock);
        while(input_info->run_flag == 0 && !input_info->shutdown) {
            pthread_cond_wait(input_cond, input_lock);
        }
        if(input_info->shutdown) {
            break;
        }

        input_info->run_flag = 0;
        input_info->buffer_flag = 1 - input_info->buffer_flag;
        pthread_mutex_unlock(input_lock);

        if(input_info->buffer_flag) {
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

        if(bucket_index == bucket_num - 1) {
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

void * output_task_sse(void * args) {

     output_thread_arg * out_arg = (output_thread_arg *)args;

     FILE    * fp = out_arg->fp;
     int64_t * sse_result_count = out_arg->sse_result_count;

     thread_info * output_info = out_arg->output_info;
     thread_info * cal_output_info = out_arg->cal_output_info;
     sse_write_t * sse_align_results_a = out_arg->sse_align_results_a;
     sse_write_t * sse_align_results_b = out_arg->sse_align_results_b;
     double write_start, write_end;

     while(1) {
        pthread_mutex_lock(&(output_info->lock));
        while(output_info->run_flag == 0 && !output_info->shutdown) {
            pthread_cond_wait(&(output_info->cond), &(output_info->lock));
        }
        if(output_info->shutdown) {
            break;
        }
        output_info->run_flag = 0;
        output_info->buffer_flag = 1 - output_info->buffer_flag;
        pthread_mutex_unlock(&(output_info->lock));

        GET_TIME(write_start);
        if(output_info->buffer_flag) {
            fwrite(sse_align_results_a, sizeof(sse_write_t), * sse_result_count, fp);
            fflush(fp);

        } else {
            fwrite(sse_align_results_b, sizeof(sse_write_t), * sse_result_count, fp);
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
