#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file.h"
#include "thread.h"
#include "align_core.h"
#include "timer.h"
#include "util.h"
#include "cal.h"

uint64_t       * cpu_dvdh_bit_mem;

cpu_read_t  * cpu_preprocess_reads_a;
cpu_read_t  * cpu_preprocess_reads_b;
cpu_read_t  * cpu_read_ptr;

cpu_write_t * cpu_align_results_a;
cpu_write_t * cpu_align_results_b;
cpu_write_t * cpu_result_ptr;

double read_total_time;
double write_total_time;
double mem_total_time;
int     time_index;
int     max_length;
int     global_ref_bucket_num;

seq_t read_seq_a;
seq_t read_seq_b;
seq_t * read_seq;
seq_t   ref_seq;

extern int match_score;
extern int mismatch_score;
extern int gap_score;

void cpu_cal_align_score(char * content, cpu_read_t * preprocess_reads, cpu_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num, uint64_t * dvdh_bit_mem) {

    int i = 0, j = 0;
    char * ref_itr;
    cpu_read_t * read_itr;
    int64_t align_index_start;
    int loop_num = read_count / CPU_V_NUM;
    int loop_num_temp = loop_num;
    int index = 0;
    int rest_read_num;
    int tmp_read_num;

    if(loop_num % chunk_read_num == 0) {
        loop_num = loop_num / chunk_read_num;
        rest_read_num = chunk_read_num;
    } else {
        rest_read_num = loop_num % chunk_read_num;
        loop_num = loop_num/ chunk_read_num + 1;
    }

  

    #pragma omp parallel for default(none) num_threads(cpu_threads) \
        private(i, j, read_itr, ref_itr, align_index_start, tmp_read_num)  \
        shared(ref_len,read_len,ref_start, ref_end, content, align_results, preprocess_reads, loop_num, loop_num_temp, word_num, dvdh_bit_mem, chunk_read_num, rest_read_num) \
        schedule(guided) collapse(2)
        for(i = ref_start; i < ref_end; i++) {
            for(j = 0; j < loop_num; j++) {
                if(j == loop_num - 1) {
                    tmp_read_num = rest_read_num;
                } else {
                    tmp_read_num = chunk_read_num;
                }

                ref_itr = & content[ i * (ref_len + 1)];
                align_index_start = (i - ref_start) * loop_num_temp;
                read_itr = & preprocess_reads[CHAR_NUM * CPU_V_NUM * j * word_num * chunk_read_num];
                align_cpu(ref_itr, read_itr, ref_len, read_len, word_num, tmp_read_num, align_index_start + j * chunk_read_num, align_results, dvdh_bit_mem);
            }
    }
}


void cpu_cal(void * args) {

    cpu_cal_t * cal_params = (cpu_cal_t *) args;
    int64_t read_ptr_offset = cal_params->read_ptr_offset;
    int64_t result_ptr_offset = cal_params->result_ptr_offset;
    seq_t   * ref_seq = cal_params->ref_seq;
    seq_t   * read_seq = cal_params->read_seq;
    cpu_read_t  * read_ptr = & (cal_params->read_ptr[read_ptr_offset]);
    cpu_write_t * result_ptr = & (cal_params->result_ptr[result_ptr_offset]);
    uint64_t * dvdh_bit_mem = cal_params->dvdh_bit_mem;
    double * cal_total_time = cal_params->cal_total_time;
    int     word_num = cal_params->word_num;
    int     chunk_read_num = cal_params->chunk_read_num;
    int     cpu_ref_start = cal_params->cpu_ref_start;
    int     cpu_ref_end = cal_params->cpu_ref_end;

    int     ref_len = ref_seq->len;
    int64_t ref_count = ref_seq->count;
    int     read_len = read_seq->len;
    int64_t read_count = cal_params->read_count;
    char   * ref_content = ref_seq->content;
    double time_start, time_end;

    GET_TIME(time_start);

    cpu_cal_align_score(ref_content, read_ptr, result_ptr, ref_len, ref_count, read_len, read_count, cpu_ref_start, cpu_ref_end, word_num, chunk_read_num, dvdh_bit_mem);

    GET_TIME(time_end);

    *cal_total_time += time_end - time_start;
}

void cal_on_cpu() {

    int i, j, k;

    FILE * fp_ref;
    FILE * fp_read;
    FILE * fp_result;
    FILE * fp_result_info;

    cpu_read_t * cpu_preprocess_reads_a;
    cpu_read_t * cpu_preprocess_reads_b;
    cpu_read_t * cpu_read_ptr;

    cpu_write_t * cpu_align_results_a;
    cpu_write_t * cpu_align_results_b;
    cpu_write_t * cpu_result_ptr;
    int64_t ref_total_size = 0;
    int     ref_len;
    int     ref_bucket_num;
    int     ref_bucket_count;
    int     ref_bucket_index = 0;

    int64_t read_total_size = 0;
    int64_t read_total_count = 0;
    int     read_len;
    int     read_bucket_num = 0;
    int     read_bucket_count = 0;
    int     read_bucket_index = 0;
    int64_t read_actual_size = 0;

    int64_t * result_bucket_counts;
    int64_t total_result_count = 0;

    int     ref_start;
    int     ref_end;
    int     cpu_word_num;
    int     cpu_dvdh_size;
    int     chunk_read_num;
    int64_t cpu_preprocess_read_size;
    int64_t cpu_align_result_size;

    cpu_cal_t   cpu_arg;
    cpu_cal_t * tmp_cpu_arg;

    int64_t * device_read_counts_a;
    int64_t * device_read_counts_b;
    int64_t * device_read_counts;

    thread_info input_info;
    thread_info output_info;
    thread_info cal_input_info;
    thread_info cal_output_info;
    thread_arg  input_arg;
    thread_arg  output_arg;

    double total_start, total_end;
    double mem_start, mem_end;
    double read_start, read_end;
    double cal_start, cal_end;
    double offload_start, offload_end;
    double write_start, write_end;
    double process_start, process_end;
    double cal_total_times = 0.0;

    int total_device_number = 1;

    GET_TIME(total_start);
    read_total_time = 0;
    write_total_time = 0;
    mem_total_time = 0;

    device_read_counts_a = (int64_t *)malloc_mem(sizeof(int64_t) * total_device_number);
    device_read_counts_b = (int64_t *)malloc_mem(sizeof(int64_t) * total_device_number);

    omp_set_nested(1);
    init_mapping_table();

    create_folder("data", 0755);
    fp_ref = open_file(file_query, "rb");
    fp_read = open_file(file_database, "rb");
    fp_result = open_file(file_result, "wb+");
    fp_result_info = open_file(file_result_info, "wb+");

    // 处理ref
    ref_total_size = get_filesize(file_query);
    ref_seq.content = (char *) malloc_mem(sizeof(char) * (ref_total_size + 1));
    get_ref_from_file(&ref_seq, fp_ref, ref_total_size);
    ref_len = ref_seq.len;

    if(ref_seq.count > REF_BUCKET_COUNT) {
        ref_bucket_num = (ref_seq.count + REF_BUCKET_COUNT - 1) / REF_BUCKET_COUNT;
        ref_bucket_count = REF_BUCKET_COUNT;
    } else {
        ref_bucket_num = 1;
        ref_bucket_count = ref_seq.count;
    }

    read_total_size = get_filesize(file_database);
    if(read_total_size > READ_BUCKET_SIZE) {
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (READ_BUCKET_SIZE + 1));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (READ_BUCKET_SIZE + 1));
        read_actual_size = READ_BUCKET_SIZE;
    } else {
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2 ));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2));
        read_actual_size = read_total_size;
        read_bucket_num = 1;
    }

    //printf("read_bucket_num is %d\n", read_bucket_num);
    GET_TIME(read_start);
    get_read_from_file(&read_seq_a, fp_read, read_actual_size, read_total_size);
    GET_TIME(read_end);
    read_total_time += read_end - read_start;

    read_len = read_seq_a.len;
    read_bucket_count = read_seq_a.count;
    read_actual_size = read_seq_a.size;
    read_total_count = (read_total_size + 1)  / (read_len + 1);

    if(read_total_size > READ_BUCKET_SIZE) {
        read_bucket_num = (read_total_size + read_actual_size - 2) / read_actual_size;
    }

    result_bucket_counts = (int64_t *)malloc_mem(sizeof(int64_t) * read_bucket_num);

    fwrite(& read_bucket_num, sizeof(int), 1, fp_result_info);
    fwrite(& total_device_number, sizeof(int), 1, fp_result_info);
    fwrite(& ref_seq.count, sizeof(int64_t), 1, fp_result_info);
    fflush(fp_result_info);

    cpu_word_num = (read_len + CPU_WORD_SIZE - 2) / (CPU_WORD_SIZE - 1);
    chunk_read_num = (max_length + read_len - 1) / read_len;
    cpu_preprocess_read_size = sizeof(cpu_read_t) * cpu_word_num * CHAR_NUM * read_bucket_count;
    cpu_preprocess_reads_a = (cpu_read_t *) malloc_mem(cpu_preprocess_read_size);
    cpu_preprocess_reads_b = (cpu_read_t *) malloc_mem(cpu_preprocess_read_size);
    cpu_align_result_size = sizeof(cpu_write_t) * ref_bucket_count * read_bucket_count;
    cpu_align_results_a = (cpu_write_t *)malloc_mem(cpu_align_result_size);
    cpu_align_results_b = (cpu_write_t *)malloc_mem(cpu_align_result_size);
    cpu_read_ptr = cpu_preprocess_reads_a;
    cpu_result_ptr = cpu_align_results_a;
    cpu_dvdh_size = cpu_word_num * cpu_threads * dvdh_len;
    cpu_dvdh_bit_mem = (uint64_t *)malloc_mem(sizeof(uint64_t) * cpu_dvdh_size);

    result_bucket_counts[0] = read_seq_a.count;

    GET_TIME(process_start);
    GET_TIME(mem_start);
    memset(cpu_preprocess_reads_a, 0, cpu_preprocess_read_size);
    cpu_handle_reads(& read_seq_a, cpu_preprocess_reads_a, cpu_word_num, 0, read_seq_a.count);
    GET_TIME(mem_end);
    mem_total_time += mem_end - mem_start;
    GET_TIME(process_end);
    device_read_counts_a[0] = read_seq_a.count;

    // 初始化线程
    input_thread_arg in_arg;
    in_arg.fp = fp_read;
    in_arg.total_size = read_total_size;
    in_arg.result_bucket_counts = result_bucket_counts;
    in_arg.cpu_preprocess_read_size = cpu_preprocess_read_size;
    in_arg.bucket_size = read_actual_size;
    in_arg.bucket_num = read_bucket_num;
    in_arg.cpu_word_num = cpu_word_num;

    output_thread_arg  out_arg;
    out_arg.fp = fp_result;
    out_arg.cpu_result_count = & total_result_count;

    init_resources(4, &input_info, &output_info, &cal_input_info, &cal_output_info);
    input_info.buffer_flag = cal_output_info.buffer_flag = 1;
    cal_input_info.buffer_flag  = output_info.buffer_flag = 0;
    cal_input_info.run_flag = cal_output_info.run_flag = 1;
    input_info.run_flag = output_info.run_flag = 0;
    input_info.shutdown = output_info.shutdown = 0;

    in_arg.input_info = & input_info;
    in_arg.cal_input_info = & cal_input_info;
    in_arg.cpu_preprocess_reads_a = cpu_preprocess_reads_a;
    in_arg.cpu_preprocess_reads_b = cpu_preprocess_reads_b;
    in_arg.read_seq_a = & read_seq_a;
    in_arg.read_seq_b = & read_seq_b;
    in_arg.device_read_counts_a = device_read_counts_a;
    in_arg.device_read_counts_b = device_read_counts_b;

    out_arg.output_info = & output_info;
    out_arg.cal_output_info = & cal_output_info;
    out_arg.cpu_align_results_a = cpu_align_results_a;
    out_arg.cpu_align_results_b = cpu_align_results_b;

    pthread_create(&(input_info.thread_id), NULL, input_task_cpu, & in_arg);
    pthread_create(&(output_info.thread_id), NULL, output_task_cpu, & out_arg);

    read_bucket_index = 0;

    while(1) {
        pthread_mutex_lock(&(cal_input_info.lock));
        while(cal_input_info.run_flag == 0) {
            pthread_cond_wait(&(cal_input_info.cond), &(cal_input_info.lock));
        }
        cal_input_info.buffer_flag = 1 - cal_input_info.buffer_flag;
        cal_input_info.run_flag = 0;
        pthread_mutex_unlock(&(cal_input_info.lock));

        pthread_mutex_lock(&(input_info.lock));
        input_info.run_flag = 1;
        if(read_bucket_index== read_bucket_num - 1) {
            input_info.shutdown = 1;
        }
        pthread_mutex_unlock(&(input_info.lock));
        pthread_cond_signal(&(input_info.cond));

        //--------------------------
        //在这里分配任务
        //--------------------------
        if(cal_input_info.buffer_flag) {
            cpu_read_ptr = cpu_preprocess_reads_a;
            device_read_counts = device_read_counts_a;
            read_seq = & read_seq_a;
        } else {
            cpu_read_ptr = cpu_preprocess_reads_b;
            device_read_counts = device_read_counts_b;
            read_seq = & read_seq_b;
        }

        fwrite(device_read_counts, sizeof(int64_t), total_device_number, fp_result_info);
        fwrite(& read_seq->extra_count, sizeof(int), 1, fp_result_info);
        fflush(fp_result_info);

        tmp_cpu_arg = & cpu_arg;
        tmp_cpu_arg->ref_seq =  & ref_seq;
        tmp_cpu_arg->read_seq = read_seq;
        tmp_cpu_arg->dvdh_bit_mem = cpu_dvdh_bit_mem;
        tmp_cpu_arg->word_num = cpu_word_num;
        tmp_cpu_arg->chunk_read_num = chunk_read_num;
        tmp_cpu_arg->cal_total_time = & cal_total_times;
        tmp_cpu_arg->read_ptr = cpu_read_ptr;

        for(ref_bucket_index = 0; ref_bucket_index < ref_bucket_num; ref_bucket_index++) {

            if(cal_output_info.buffer_flag) {
                cpu_result_ptr = cpu_align_results_a;
            } else {
                cpu_result_ptr = cpu_align_results_b;
            }

            if(ref_bucket_index == ref_bucket_num - 1) {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = ref_seq.count;
            } else {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = (ref_bucket_index + 1) * ref_bucket_count;
            }

            tmp_cpu_arg = & cpu_arg;
            tmp_cpu_arg->cpu_ref_start = ref_start;
            tmp_cpu_arg->cpu_ref_end = ref_end;
            tmp_cpu_arg->read_ptr_offset =  0;
            tmp_cpu_arg->result_ptr_offset = 0 ;
            tmp_cpu_arg->result_ptr = cpu_result_ptr;
            tmp_cpu_arg->read_count = read_seq->count;
            cpu_cal(tmp_cpu_arg);

            pthread_mutex_lock(&(cal_output_info.lock));
            while(cal_output_info.run_flag == 0) {
                pthread_cond_wait(&(cal_output_info.cond), &(cal_output_info.lock));
            }
            cal_output_info.run_flag = 0;
            cal_output_info.buffer_flag = 1 - cal_output_info.buffer_flag;
            pthread_mutex_unlock(&(cal_output_info.lock));

            pthread_mutex_lock(&(output_info.lock));
            output_info.run_flag = 1;
            total_result_count = (ref_end - ref_start) * read_seq->count;
            pthread_mutex_unlock(&(output_info.lock));
            pthread_cond_signal(&(output_info.cond));
        }

        read_bucket_index++;
        //printf("read_bucket_index is %d\n", read_bucket_index);

        if(read_bucket_index > read_bucket_num - 1) {
            break;
        }
    }

    pthread_mutex_lock(&(cal_output_info.lock));
    while(cal_output_info.run_flag == 0) {
        pthread_cond_wait(&(cal_output_info.cond), &(cal_output_info.lock));
    }
    cal_output_info.run_flag = 0;
    cal_output_info.buffer_flag = 1 - cal_output_info.buffer_flag;
    pthread_mutex_unlock(&(cal_output_info.lock));
    pthread_mutex_lock(&(output_info.lock));
    output_info.run_flag = 1;
    output_info.shutdown = 1;
    pthread_mutex_unlock(&(output_info.lock));
    pthread_cond_signal(&(output_info.cond));

    /*fwrite(& read_bucket_num, sizeof(int), 1, fp_result_info);
    fwrite(& ref_seq.count, sizeof(int64_t), 1, fp_result_info);
    fwrite(result_bucket_counts, sizeof(int64_t), read_bucket_num, fp_result_info);
    fflush(fp_result_info);*/

    free_mem(ref_seq.content);
    free_mem(read_seq_a.content);
    free_mem(read_seq_b.content);

    free_mem(cpu_preprocess_reads_a);
    free_mem(cpu_preprocess_reads_b);

    free_mem(cpu_align_results_a);
    free_mem(cpu_align_results_b);
    free_mem(cpu_dvdh_bit_mem);
    free_mem(device_read_counts_a);
    free_mem(device_read_counts_b);
    free_mem(result_bucket_counts);

    pthread_join(input_info.thread_id, NULL);
    pthread_join(output_info.thread_id, NULL);
    free_resources(4, &input_info, &output_info, &cal_input_info, &cal_output_info);
    fclose(fp_ref);
    fclose(fp_read);
    fclose(fp_result);
    fclose(fp_result_info);

    GET_TIME(total_end);


    int total_temp = 0;
    for(i = 0; i < read_bucket_num; i++) {
        total_temp += result_bucket_counts[i];
    }
    printf("\n");
    printf("score is %d, %d, %d\n", match_score, mismatch_score, gap_score);
    printf("read_total_time  is %.2fs\n", read_total_time);
    printf("write_total_time is %.2fs\n", write_total_time);
    printf("mem_total_time is   %.2fs\n", mem_total_time);
    printf("\n");

    printf("ref_len    is %d\n", ref_len);
    printf("ref_count  is %ld\n", ref_seq.count);
    printf("read_len   is %d\n", read_len);
    printf("read_count is %ld\n\n", total_temp);

    printf("cal_total_times     is %.2fs\n", cal_total_times);
    printf("total time          is %.2fs\n", total_end - total_start);
    printf("Cal GCUPS is %.2f\n", 1.0 * ref_len * ref_seq.count * read_len * total_temp / cal_total_times / 1000000000);
    printf("Total GCUPS is %.2f\n", 1.0 * ref_len * ref_seq.count * read_len * total_temp / (total_end - total_start) / 1000000000);
    printf("\n\n");
}
