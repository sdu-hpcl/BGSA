#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <offload.h>

#include "file.h"
#include "thread.h"
#include "align_core.h"
#include "timer.h"
#include "util.h"
#include "cal.h"

mic_read_t  * preprocess_reads_a;
mic_read_t  * preprocess_reads_b;
mic_read_t  * read_ptr;

mic_write_t * align_results_a;
mic_write_t * align_results_b;
mic_write_t * result_ptr;


seq_t read_seq_a;
seq_t read_seq_b;
seq_t * read_seq;
seq_t   ref_seq;

double ** loop_device_ratio;
double ** loop_used_times;
int     time_index;
int     max_length;
int     global_ref_bucket_num;

double read_total_time;
double write_total_time;
double mem_total_time;

__m512i * dvdh_bit_mem;

extern int match_score;
extern int mismatch_score;
extern int gap_score;

void mic_cal_align_score(char * content, mic_read_t * preprocess_reads, mic_write_t * align_results, int ref_len, int ref_count, int read_len, int read_count, int ref_start, int ref_end, int word_num, int chunk_read_num,  __m512i * dvdh_bit_mem) {

    int i = 0, j = 0;
    char * ref_itr;
    mic_read_t * read_itr;
    int64_t align_index_start;
    int loop_num = read_count / MIC_V_NUM;
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

#pragma omp parallel for default(none) num_threads(cpu_threads)\
    private(i, j, read_itr, ref_itr, align_index_start, tmp_read_num)  \
    shared( ref_len,read_len,ref_start, ref_end, content, align_results, preprocess_reads, loop_num, loop_num_temp, word_num, dvdh_bit_mem, chunk_read_num, rest_read_num) \
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
            read_itr = & preprocess_reads[CHAR_NUM * MIC_V_NUM * j * word_num * chunk_read_num];
            align_mic(ref_itr, read_itr, ref_len, read_len, word_num, tmp_read_num, align_index_start + j * chunk_read_num, align_results, dvdh_bit_mem);
        }
    }

}

void  mic_cal(void * args) {

    mic_cal_t * cal_params = (mic_cal_t *) args;
    seq_t   * ref_seq = cal_params->ref_seq;
    seq_t   * read_seq = cal_params->read_seq;
    mic_read_t  * read_ptr = cal_params->read_ptr;
    mic_write_t * result_ptr = cal_params->result_ptr;
    // 加上下面那句话程序就会出错
    //__m512i * dvdh_bit_mem = cal_params->dvdh_bit_mem;
    int64_t read_size = cal_params->read_size;
    int64_t result_size = cal_params->result_size;
    int     word_num = cal_params->word_num;
    int     chunk_read_num = cal_params->chunk_read_num;
    int     offload_dvdh_size = cal_params->offload_dvdh_size;
    int     mic_ref_start = cal_params->mic_ref_start;
    int     mic_ref_end = cal_params->mic_ref_end;
    int     mic_index = cal_params->mic_index;
    int     async_flag = cal_params->async_flag;

    int     ref_len = ref_seq->len;
    int64_t ref_count = ref_seq->count;
    int     read_len = read_seq->len;
    int64_t read_count = cal_params->read_count;
    char   * ref_content = ref_seq->content;
    double * cal_total_time = cal_params->cal_total_time;
    double * offload_total_time = cal_params->offload_total_time;

    /*printf("raed_count is %d\n", read_count);
    printf("mic_ref_start is %d\n", mic_ref_start);
    printf("mic_ref_end is %d\n", mic_ref_end);*/

    //dvdhbit_mem是全局变量
    double time_start, time_end, cal_start, cal_end;
    GET_TIME(time_start);

//#pragma offload target(mic:mic_index) \
    in(ref_content:length(ref_seq->size) REUSE RETAIN ) \
    in(read_ptr:length(0) REUSE RETAIN) \
    out(result_ptr:length(0) REUSE RETAIN) \
    in(dvdh_bit_mem:length(0) REUSE RETAIN) \
    in(ref_len, ref_count, read_len, read_count, mic_ref_start, mic_ref_end, chunk_read_num)\
    inout(cal_start, cal_end)
    {
        GET_TIME(cal_start);

        mic_cal_align_score(ref_content, read_ptr, result_ptr, ref_len, ref_count, read_len, read_count, mic_ref_start, mic_ref_end,word_num, chunk_read_num, dvdh_bit_mem);

        GET_TIME(cal_end);
    }

    GET_TIME(time_end);


    * cal_total_time += cal_end - cal_start;
    * offload_total_time += time_end - time_start;

}

void cal_on_mic() {

    int i, j, k;

    FILE * fp_ref;
    FILE * fp_read;
    FILE * fp_result;
    FILE * fp_result_info;

    int64_t ref_total_size;
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

    int64_t preprocess_read_size;
    int64_t align_result_size;
    int64_t offload_read_size;
    int64_t offload_result_size;

    int     word_num;
    int     chunk_read_num;
    int     offload_dvdh_size;
    int     ref_start;
    int     ref_end;

    int64_t * result_bucket_counts;

    mic_read_t  * read_ptr;
    mic_write_t * result_ptr;
    int64_t read_ptr_offset;
    int64_t result_ptr_offset;
    int64_t single_read_size;
    int64_t single_result_size;
    int64_t single_read_count;
    int64_t single_result_count;
    int64_t total_result_count = 0;

    mic_cal_t * mic_args;
    mic_cal_t * tmp_cal_arg;

    double  * device_compute_ratio;
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
    double read_start, read_end;
    double cal_start, cal_end;
    double offload_start, offload_end;
    double write_start, write_end;
    double process_start, process_end;
    double mem_start, mem_end;
    double mem_time, mem_num;

    int total_device_number;
    int mic_device_number = 1;
    //mic_device_number = 1;
    total_device_number = mic_device_number;
    GET_TIME(total_start);
    read_total_time = 0;
    write_total_time = 0;
    mem_total_time = 0;

    device_compute_ratio =(double *) malloc_mem(sizeof(double) * total_device_number);
    device_read_counts_a = (int64_t * )malloc_mem(sizeof(int64_t) * total_device_number);
    device_read_counts_b = (int64_t * )malloc_mem(sizeof(int64_t) * total_device_number);

    init_mic_ratio(device_compute_ratio, mic_device_number);

    double cal_total_times[total_device_number];
    double offload_total_times[total_device_number];

    for(i = 0; i < total_device_number; i++) {
        cal_total_times[i] = 0;
        offload_total_times[i] = 0;
    }
    omp_set_nested(1);

    init_mapping_table();
    mic_args = (mic_cal_t *)malloc_mem(sizeof(mic_cal_t) * mic_device_number);

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
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2 + 1));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2 + 1));
        read_actual_size = read_total_size;
        read_bucket_num = 1;
    }

    GET_TIME(read_start);
    get_read_from_file(&read_seq_a, fp_read, read_actual_size, read_total_size);
    dispatch_task(device_compute_ratio, device_read_counts_a, read_seq_a.count, total_device_number);
    GET_TIME(read_end);
    read_total_time += read_end - read_start;

    read_len = read_seq_a.len;
    read_bucket_count = read_seq_a.count;
    //printf("read_bucket_count is %d\n", read_bucket_count);
    read_actual_size = read_seq_a.size;
    read_total_count = (read_total_size + 1)  / (read_len + 1);
    //while(read_total_count )

    if(read_total_size > READ_BUCKET_SIZE) {
        read_bucket_num = (read_total_size + read_actual_size - 2) / read_actual_size;
    }

    fwrite(& read_bucket_num, sizeof(int), 1, fp_result_info);
    fwrite(& total_device_number, sizeof(int), 1, fp_result_info);
    fwrite(& ref_seq.count, sizeof(int64_t), 1, fp_result_info);
    fflush(fp_result_info);

    result_bucket_counts = (int64_t *)malloc_mem(sizeof(int64_t) * read_bucket_num);

    {
        word_num = (read_len + MIC_WORD_SIZE-2) / (MIC_WORD_SIZE -1);
        chunk_read_num = (max_length + read_len - 1) / read_len;
        offload_dvdh_size = word_num * MIC_MAX_THREADS * dvdh_len;
        offload_read_size = CHAR_NUM * word_num * read_bucket_count;
        offload_result_size = ref_bucket_count * read_bucket_count;
        preprocess_read_size = sizeof(mic_read_t) * word_num * CHAR_NUM * read_bucket_count;
        GET_TIME(mem_start);
        preprocess_reads_a = (mic_read_t *)malloc_mem(preprocess_read_size);
        preprocess_reads_b = (mic_read_t *)malloc_mem(preprocess_read_size);
        GET_TIME(mem_end);
        mem_time = mem_end - mem_start;
        mem_num = preprocess_read_size / 1024 / 1024;
        align_result_size = sizeof(mic_write_t) * ref_bucket_count * read_bucket_count;
        align_results_a = (mic_write_t *)malloc_mem(align_result_size);
        align_results_b = (mic_write_t *)malloc_mem(align_result_size);
        read_ptr = preprocess_reads_a;
        result_ptr = align_results_a;
        dvdh_bit_mem = (__m512i *)malloc_mem(sizeof(__m512i) * offload_dvdh_size);

        GET_TIME(offload_start);

        GET_TIME(offload_end);
        result_bucket_counts[0] = read_seq_a.count;

        GET_TIME(process_start);
        GET_TIME(mem_start);
        memset(preprocess_reads_a, 0, preprocess_read_size);
        mic_handle_reads(&read_seq_a, preprocess_reads_a, word_num, 0, read_seq_a.count);
        GET_TIME(mem_end);
        mem_total_time += mem_end - mem_start;
        GET_TIME(process_end);
    }


    {
        // 初始化线程
        input_thread_arg in_arg;
        in_arg.fp = fp_read;
        in_arg.total_size = read_total_size;
        in_arg.result_bucket_counts = result_bucket_counts;
        in_arg.preprocess_read_size= preprocess_read_size;
        in_arg.bucket_size = read_actual_size;
        in_arg.bucket_num = read_bucket_num;
        in_arg.word_num = word_num;
        //in_arg.up_signal_a = up_signal_a;
        //in_arg.up_signal_b = up_signal_b;

        output_thread_arg  out_arg;
        out_arg.fp = fp_result;
        out_arg.result_count = & total_result_count;

        init_resources(4, &input_info, &output_info, &cal_input_info, &cal_output_info);
        input_info.buffer_flag = cal_output_info.buffer_flag = 1;
        cal_input_info.buffer_flag  = output_info.buffer_flag = 0;
        cal_input_info.run_flag = cal_output_info.run_flag = 1;
        input_info.run_flag = output_info.run_flag = 0;
        input_info.shutdown = output_info.shutdown = 0;

        in_arg.input_info = & input_info;
        in_arg.cal_input_info = & cal_input_info;
        in_arg.preprocess_reads_a = preprocess_reads_a;
        in_arg.preprocess_reads_b = preprocess_reads_b;
        in_arg.read_seq_a = & read_seq_a;
        in_arg.read_seq_b = & read_seq_b;
        in_arg.device_read_counts_a = device_read_counts_a;
        in_arg.device_read_counts_b = device_read_counts_b;
        in_arg.device_compute_ratio = device_compute_ratio;
        in_arg.mic_device_number = mic_device_number;
        in_arg.total_device_number = total_device_number;

        out_arg.output_info = & output_info;
        out_arg.cal_output_info = & cal_output_info;
        out_arg.align_results_a =  align_results_a;
        out_arg.align_results_b =  align_results_b;
        out_arg.mic_device_number = mic_device_number;

        pthread_create(&(input_info.thread_id), NULL, input_task_mic, & in_arg);
        pthread_create(&(output_info.thread_id), NULL, output_task_mic, & out_arg);
    }

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
            read_ptr = preprocess_reads_a;
            device_read_counts = device_read_counts_a;
            read_seq = & read_seq_a;
        } else {
            read_ptr = preprocess_reads_b;
            device_read_counts = device_read_counts_b;
            read_seq = & read_seq_b;
        }

        fwrite(device_read_counts, sizeof(int64_t), total_device_number, fp_result_info);
        fwrite(& read_seq->extra_count, sizeof(int), 1, fp_result_info);
        fflush(fp_result_info);

        for(i = 0; i < mic_device_number; i++) {
            tmp_cal_arg = & mic_args[i];
            tmp_cal_arg->ref_seq =  & ref_seq;
            tmp_cal_arg->read_seq = read_seq;
            tmp_cal_arg->dvdh_bit_mem = dvdh_bit_mem;
            tmp_cal_arg->word_num = word_num;
            tmp_cal_arg->chunk_read_num = chunk_read_num;
            tmp_cal_arg->offload_dvdh_size = offload_dvdh_size;
            tmp_cal_arg->mic_index = i;
            tmp_cal_arg->cal_total_time = & cal_total_times[i];
            tmp_cal_arg->offload_total_time = & offload_total_times[i];
        }

        for(ref_bucket_index = 0; ref_bucket_index < ref_bucket_num; ref_bucket_index++) {

            if(cal_output_info.buffer_flag) {
                result_ptr = align_results_a;
            } else {
                result_ptr = align_results_b;
            }

            if(ref_bucket_index == ref_bucket_num - 1) {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = ref_seq.count;
            } else {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = (ref_bucket_index + 1) * ref_bucket_count;
            }

            int mic_read_index = 0;

            //#pragma  omp parallel for num_threads(total_device_number) default(none) \
                private(i, tmp_cal_arg, single_read_count, single_read_size, single_result_size, mic_read_index) \
                shared(mic_args,total_device_number, mic_device_number, read_seq, read_ptr, result_ptr, ref_start, ref_end, word_num, device_read_counts, cal_output_info )
                for(i = 0; i < total_device_number; i++) {

                    tmp_cal_arg = & mic_args[i];

                    int k = 0;
                    mic_read_index = 0;
                    for(k = 0; k < i; k++) {
                        mic_read_index += device_read_counts[k];
                    }

                    tmp_cal_arg->mic_ref_start = ref_start;
                    tmp_cal_arg->mic_ref_end = ref_end;
                    tmp_cal_arg->read_ptr = & read_ptr[mic_read_index * CHAR_NUM * word_num];
                    tmp_cal_arg->result_ptr = & result_ptr[mic_read_index * (ref_end - ref_start)];
                    tmp_cal_arg->read_count = device_read_counts[i];
                    single_read_count = device_read_counts[i];
                    single_read_size = single_read_count * word_num * CHAR_NUM;
                    tmp_cal_arg->read_size = single_read_size;
                    single_result_size = (ref_end - ref_start) * single_read_count;
                    tmp_cal_arg->result_size = single_result_size;

                    mic_cal(tmp_cal_arg);
                }

            time_index++;

            pthread_mutex_lock(&(cal_output_info.lock));
            while(cal_output_info.run_flag == 0) {
                pthread_cond_wait(&(cal_output_info.cond), &(cal_output_info.lock));
            }
            cal_output_info.run_flag = 0;
            cal_output_info.buffer_flag = 1 - cal_output_info.buffer_flag;
            pthread_mutex_unlock(&(cal_output_info.lock));

            pthread_mutex_lock(&(output_info.lock));
            output_info.run_flag = 1;
            total_result_count = (ref_end - ref_start) * result_bucket_counts[read_bucket_index];
            pthread_mutex_unlock(&(output_info.lock));
            pthread_cond_signal(&(output_info.cond));
        }


        read_bucket_index++;
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

    free_mem(mic_args);
    free_mem(ref_seq.content);
    free_mem(read_seq_a.content);
    free_mem(read_seq_b.content);
    free_mem(result_bucket_counts);

    free_mem(preprocess_reads_a);
    free_mem(preprocess_reads_b);
    free_mem(align_results_a);
    free_mem(align_results_b);
    free_mem(dvdh_bit_mem);
    free_mem(device_compute_ratio);
    free_mem(device_read_counts);

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
    printf("AVX512: \n");
    printf("Myers: \n" );
    printf("score is %d, %d, %d\n", match_score, mismatch_score, gap_score);
    printf("ref_len    is %d\n", ref_len);
    printf("ref_count  is %ld\n", ref_seq.count);
    printf("read_len   is %d\n", read_len);
    printf("read_count is %ld\n", total_temp);
    printf("read_total_count is %ld\n\n", read_total_count);
    //printf("cpu cal_total_times       is %.2fs\n", cal_total_times[0]);
    for(i = 0; i < total_device_number; i++) {
        printf("mic-%d cal_total_time     is %.2fs\n", i, cal_total_times[i]);
    }

    printf("\n");
    printf("read_total_time  is %.2fs\n", read_total_time);
    printf("write_total_time is %.2fs\n", write_total_time);
    printf("mem_total_time is   %.2fs\n", mem_total_time);
    printf("\n");

    printf("mem time is %.2fs\n", mem_time);
    printf("mem_num is %.2fm\n", mem_num);
    printf("total time                is %.2fs\n", total_end - total_start);

    printf("Cal GCUPS is %.2f\n", 1.0 * ref_len * ref_seq.count * read_len * total_temp / cal_total_times[0] / 1000000000);
    printf("Total GCUPS is %.2f\n", 1.0 * ref_len * ref_seq.count * read_len * total_temp / (total_end - total_start) / 1000000000);

}
