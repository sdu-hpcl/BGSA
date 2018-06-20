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

double ** use_times;
double ** loop_device_ratio;
double ** loop_used_times;
int     time_index;
int     max_length;
int     global_ref_bucket_num;

char * up_signal_a;
char * up_signal_b;
char * down_signal_a;
char * down_signal_b;

double read_total_time;
double write_total_time;
double mem_total_time;


void  mic_cal_all(void * args) {

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

#pragma offload target(mic:mic_index) \
    in(ref_content:length(ref_seq->size) REUSE RETAIN ) \
    in(read_ptr:length(0) REUSE RETAIN) \
    out(result_ptr:length(0) REUSE RETAIN) \
    in(dvdh_bit_mem:length(0) REUSE RETAIN) \
    in(ref_len, ref_count, read_len, read_count, mic_ref_start, mic_ref_end, chunk_read_num, threshold)\
    inout(cal_start, cal_end) \
    in(mic_threads)
    {
        GET_TIME(cal_start);

        mic_cal_align_score(ref_content, read_ptr, result_ptr, ref_len, ref_count, read_len, read_count, mic_ref_start, mic_ref_end,word_num, chunk_read_num, dvdh_bit_mem);

        GET_TIME(cal_end);
    }

    if(async_flag) {
        #pragma offload_transfer target(mic:mic_index) signal( & down_signal_b[mic_index]) \
            out(result_ptr:length(result_size) REUSE RETAIN)

    } else {
        #pragma offload_transfer target(mic:mic_index) signal(& down_signal_a[mic_index]) \
            out(result_ptr:length(result_size) REUSE RETAIN)

    }

    GET_TIME(time_end);
    use_times[mic_index + 1][time_index] = time_end - time_start;
    * cal_total_time += cal_end - cal_start;
    * offload_total_time += time_end - time_start;
}

void sse_cal_all(void * args) {

    sse_cal_t * cal_params = (sse_cal_t *) args;
    int64_t read_ptr_offset = cal_params->read_ptr_offset;
    int64_t result_ptr_offset = cal_params->result_ptr_offset;
    seq_t   * ref_seq = cal_params->ref_seq;
    seq_t   * read_seq = cal_params->read_seq;
    sse_read_t  * read_ptr = & (cal_params->read_ptr[read_ptr_offset]);
    sse_write_t * result_ptr = & (cal_params->result_ptr[result_ptr_offset]);
    __m128i * dvdh_bit_mem = cal_params->dvdh_bit_mem;
    double * cal_total_time = cal_params->cal_total_time;
    int     word_num = cal_params->word_num;
    int     chunk_read_num = cal_params->chunk_read_num;
    int     sse_ref_start = cal_params->sse_ref_start;
    int     sse_ref_end = cal_params->sse_ref_end;

    int     ref_len = ref_seq->len;
    int64_t ref_count = ref_seq->count;
    int     read_len = read_seq->len;
    int64_t read_count = cal_params->read_count;
    char   * ref_content = ref_seq->content;
    double time_start, time_end;

    GET_TIME(time_start);
    sse_cal_align_score(ref_content, read_ptr, result_ptr, ref_len, ref_count, read_len, read_count, sse_ref_start, sse_ref_end, word_num, chunk_read_num, dvdh_bit_mem);
    GET_TIME(time_end);
    use_times[0][time_index] = time_end - time_start;
    *cal_total_time += time_end - time_start;
}


void cal_on_all() {
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
    int64_t * sse_result_bucket_counts;

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

    int     sse_word_num;
    int     sse_chunk_read_num;
    int     sse_dvdh_size;
    int64_t sse_preprocess_read_size;
    int64_t sse_align_result_size;
    int64_t sse_result_total_count;

    sse_cal_t   sse_arg;
    sse_cal_t * tmp_sse_arg;


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

    GET_TIME(total_start);
    read_total_time = 0;
    write_total_time = 0;
    mem_total_time  = 0;


    int total_device_number;
    int mic_device_number = mic_number;
    int cpu_device_number = 1;
    //mic_device_number = 1;
    total_device_number = mic_device_number + cpu_device_number;

    device_compute_ratio =(double *) malloc_mem(sizeof(double) * total_device_number);
    device_read_counts_a = (int64_t * )malloc_mem(sizeof(int64_t) * total_device_number);
    device_read_counts_b = (int64_t * )malloc_mem(sizeof(int64_t) * total_device_number);

    if(file_ratio == NULL) {
        init_cpu_mic_ratio(device_compute_ratio, cpu_device_number, mic_device_number);
    } else {
        init_device_ratio_file(device_compute_ratio, cpu_device_number, mic_device_number, file_ratio);
    }
    use_times = (double **)malloc_mem(sizeof(double *) *  total_device_number);


    up_signal_a = (char *)malloc_mem(sizeof(char) * mic_device_number);
    up_signal_b = (char *)malloc_mem(sizeof(char) * mic_device_number);
    down_signal_a = (char *)malloc_mem(sizeof(char) * mic_device_number);
    down_signal_b = (char *)malloc_mem(sizeof(char) * mic_device_number);

    double cal_total_times[total_device_number];
    double offload_total_times[total_device_number];
    double previous_device_times[total_device_number];

    for(i = 0; i < total_device_number; i++) {
        cal_total_times[i] = 0;
        offload_total_times[i] = 0;
    }
    omp_set_nested(1);

    init_mapping_table();
    init_device(mic_device_number);
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

    global_ref_bucket_num = ref_bucket_num;

    read_total_size = get_filesize(file_database);
    if(read_total_size > READ_BUCKET_SIZE) {
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (READ_BUCKET_SIZE + 1));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (READ_BUCKET_SIZE + 1));
        read_actual_size = READ_BUCKET_SIZE;
    } else {
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2));
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
    read_actual_size = read_seq_a.size;
    read_total_count = (read_total_size + 1)  / (read_len + 1);

    if(read_total_size > READ_BUCKET_SIZE) {
        read_bucket_num = (read_total_size + read_actual_size - 2) / read_actual_size;
    }

    fwrite(& read_bucket_num, sizeof(int), 1, fp_result_info);
    fwrite(& total_device_number, sizeof(int), 1, fp_result_info);
    fwrite(& ref_seq.count, sizeof(int64_t), 1, fp_result_info);
    fflush(fp_result_info);


    result_bucket_counts = (int64_t *)malloc_mem(sizeof(int64_t) * read_bucket_num);
    sse_result_bucket_counts = (int64_t *)malloc_mem(sizeof(int64_t) * read_bucket_num);

    for(i = 0; i < total_device_number; i++) {
        use_times[i] = (double *)malloc_mem(sizeof(double) * ref_bucket_num * read_bucket_num );
        memset(use_times[i], 0, sizeof(double) * ref_bucket_num * read_bucket_num);
    }

    {
        int h_threshold = threshold + read_len - ref_len;
        word_num = (read_len - h_threshold + MIC_WORD_SIZE - 1) / MIC_WORD_SIZE + 1;
        //word_num = (read_len + MIC_WORD_SIZE-1) / (MIC_WORD_SIZE );
        chunk_read_num = (max_length + read_len - 1) / read_len;
        offload_dvdh_size = MIC_MAX_THREADS * dvdh_len;
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

        
        sse_word_num = (read_len - h_threshold + SSE_WORD_SIZE - 1) / SSE_WORD_SIZE + 1; 
        //sse_word_num = (read_len + SSE_WORD_SIZE - 2) / (SSE_WORD_SIZE - 1);
        sse_chunk_read_num = (max_length + read_len - 1) / read_len;
        sse_preprocess_read_size = sizeof(sse_read_t) * sse_word_num * CHAR_NUM * read_bucket_count;
        sse_preprocess_reads_a = (sse_read_t *) malloc_mem(sse_preprocess_read_size);
        sse_preprocess_reads_b = (sse_read_t *) malloc_mem(sse_preprocess_read_size);
        sse_align_result_size = sizeof(sse_write_t) * ref_bucket_count * read_bucket_count;
        sse_align_results_a = (sse_write_t *)malloc_mem(sse_align_result_size);
        sse_align_results_b = (sse_write_t *)malloc_mem(sse_align_result_size);
        sse_read_ptr = sse_preprocess_reads_a;
        sse_result_ptr = sse_align_results_a;
        sse_dvdh_size = cpu_threads * dvdh_len;
        sse_dvdh_bit_mem = (__m128i *)malloc_mem(sizeof(__m128i) * sse_dvdh_size);

        GET_TIME(offload_start);

        // 在设备上分配内存
//#pragma omp parallel for num_threads(mic_device_number) default(none) \
        private(i) shared(up_signal_a, ref_seq, ref_total_size, preprocess_reads_a, preprocess_reads_b, offload_read_size, align_results_a, align_results_b, offload_result_size, dvdh_bit_mem, offload_dvdh_size, mic_device_number )


        for(i = 0; i < mic_device_number; i++) {


#pragma offload_transfer target(mic:i) signal(& up_signal_a[i]) \
            nocopy(ref_seq.content:length(ref_total_size + 1) ALLOC RETAIN) \
            nocopy(preprocess_reads_a:length(offload_read_size) ALLOC RETAIN) \
            nocopy(preprocess_reads_b:length(offload_read_size) ALLOC RETAIN) \
            nocopy(align_results_a:length(offload_result_size) ALLOC RETAIN) \
            nocopy(align_results_b:length(offload_result_size) ALLOC RETAIN) \
            nocopy(dvdh_bit_mem:length(offload_dvdh_size) ALLOC RETAIN )
            {

            }
        }

        //printf("1111\n");

        GET_TIME(offload_end);
        sse_result_bucket_counts[0] = device_read_counts_a[0];
        result_bucket_counts[0] = read_seq_a.count - device_read_counts_a[0];

        GET_TIME(process_start);
        GET_TIME(mem_start);
        memset(preprocess_reads_a, 0, preprocess_read_size);
        memset(sse_preprocess_reads_a, 0, sse_preprocess_read_size);
        sse_handle_reads(&read_seq_a, sse_preprocess_reads_a, sse_word_num, 0, device_read_counts_a[0]);
        mic_handle_reads(&read_seq_a, preprocess_reads_a, word_num, device_read_counts_a[0], read_seq_a.count - device_read_counts_a[0]);
        GET_TIME(mem_end);
        mem_total_time = mem_end -mem_start;

        #pragma omp parallel for num_threads(mic_device_number) default(none)\
            private(i) shared(mic_device_number, preprocess_reads_a, up_signal_a, word_num, device_read_counts_a, total_device_number)
        for(i = 1; i < total_device_number; i++) {
            int mic_read_index = 0;
            int k;
            int single_read_count;
            int single_read_size;
            mic_read_t * p_tmp ;
            for(k = 1; k < i; k++) {
                mic_read_index += device_read_counts_a[k];
            }

            single_read_count = device_read_counts_a[i];
            single_read_size = single_read_count * word_num * CHAR_NUM;
            p_tmp = & preprocess_reads_a[mic_read_index * CHAR_NUM * word_num];

            #pragma offload_wait target(mic:(i-1)) wait(& up_signal_a[i-1])

            #pragma offload_transfer target(mic:(i-1)) signal(& up_signal_a[i-1]) \
                in(p_tmp:length(single_read_size) REUSE RETAIN)
        }

        GET_TIME(process_end);
    }


    {
        // 初始化线程
        input_thread_arg in_arg;
        in_arg.fp = fp_read;
        in_arg.total_size = read_total_size;
        in_arg.result_bucket_counts = result_bucket_counts;
        in_arg.sse_result_bucket_counts = sse_result_bucket_counts;
        in_arg.preprocess_read_size= preprocess_read_size;
        in_arg.sse_preprocess_read_size = sse_preprocess_read_size;
        in_arg.offload_read_size = offload_read_size;
        in_arg.bucket_size = read_actual_size;
        in_arg.bucket_num = read_bucket_num;
        in_arg.word_num = word_num;
        in_arg.sse_word_num = sse_word_num;

        output_thread_arg  out_arg;
        out_arg.fp = fp_result;
        out_arg.result_count = & total_result_count;
        out_arg.sse_result_count = & sse_result_total_count;

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
        in_arg.sse_preprocess_reads_a = sse_preprocess_reads_a;
        in_arg.sse_preprocess_reads_b = sse_preprocess_reads_b;
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
        out_arg.sse_align_results_a = sse_align_results_a;
        out_arg.sse_align_results_b = sse_align_results_b;
        out_arg.mic_device_number = mic_device_number;

        pthread_create(&(input_info.thread_id), NULL, input_task_all, & in_arg);
        pthread_create(&(output_info.thread_id), NULL, output_task_all, & out_arg);
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
            sse_read_ptr = sse_preprocess_reads_a;
            device_read_counts = device_read_counts_a;
            read_seq = & read_seq_a;
        } else {
            read_ptr = preprocess_reads_b;
            sse_read_ptr = sse_preprocess_reads_b;
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
            tmp_cal_arg->cal_total_time = & cal_total_times[i+1];
            tmp_cal_arg->offload_total_time = & offload_total_times[i+1];
        }

        tmp_sse_arg = & sse_arg;
        tmp_sse_arg->ref_seq =  & ref_seq;
        tmp_sse_arg->read_seq = read_seq;
        tmp_sse_arg->dvdh_bit_mem = sse_dvdh_bit_mem;
        tmp_sse_arg->word_num = sse_word_num;
        tmp_sse_arg->chunk_read_num = sse_chunk_read_num;
        tmp_sse_arg->cal_total_time = & cal_total_times[0];
        tmp_sse_arg->read_ptr = sse_read_ptr;


        if(cal_input_info.buffer_flag) {
            for(i = 0; i < mic_device_number; i++) {
                #pragma offload_wait target(mic:i) wait(& up_signal_a[i])
            }
        } else {
            for(i = 0; i < mic_device_number; i++) {
                #pragma offload_wait target(mic:i) wait(& up_signal_b[i])
            }
        }




        for(ref_bucket_index = 0; ref_bucket_index < ref_bucket_num; ref_bucket_index++) {



            if(cal_output_info.buffer_flag) {
                result_ptr = align_results_a;
                sse_result_ptr = sse_align_results_a;
            } else {
                result_ptr = align_results_b;
                sse_result_ptr = sse_align_results_b;
            }

            if(ref_bucket_index == ref_bucket_num - 1) {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = ref_seq.count;
            } else {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = (ref_bucket_index + 1) * ref_bucket_count;
            }

            int mic_read_index = 0;

#pragma  omp parallel for num_threads(total_device_number) default(none) \
            private(i, tmp_cal_arg, tmp_sse_arg, single_read_count, single_read_size, single_result_size, mic_read_index) \
            shared(mic_args, sse_arg, total_device_number, mic_device_number, read_seq, read_ptr, result_ptr, ref_start, ref_end, word_num, sse_result_ptr, device_read_counts, cal_output_info , previous_device_times)
            for(i = 0; i < total_device_number; i++) {
                if(i == 0) {
                    tmp_sse_arg = & sse_arg;
                    tmp_sse_arg->sse_ref_start = ref_start;
                    tmp_sse_arg->sse_ref_end = ref_end;
                    tmp_sse_arg->read_ptr_offset =  0;
                    tmp_sse_arg->result_ptr_offset = 0 ;
                    tmp_sse_arg->result_ptr = sse_result_ptr;
                    tmp_sse_arg->read_count = device_read_counts[0];
                    single_read_count = device_read_counts[0];
                    single_result_size = (ref_end - ref_start) * single_read_count;


                    sse_cal_all(tmp_sse_arg);

                } else {

                    tmp_cal_arg = & mic_args[i-1];

                    int k = 0;
                    mic_read_index = 0;
                    for(k = 1; k < i; k++) {
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
                    if(cal_output_info.buffer_flag) {
                        tmp_cal_arg->async_flag = 0;
                    } else {
                        tmp_cal_arg->async_flag = 1;
                    }

                    mic_cal_all(tmp_cal_arg);


                }
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
            sse_result_total_count = (ref_end - ref_start) * sse_result_bucket_counts[read_bucket_index];
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

    for(i = 0; i < mic_device_number; i++) {

#pragma offload target(mic:i) \
        nocopy(dvdh_bit_mem:length(offload_dvdh_size) REUSE FREE)\
        nocopy(ref_seq.content:length(ref_total_size + 1) REUSE FREE) \
        nocopy(preprocess_reads_a:length(offload_read_size) REUSE FREE) \
        nocopy(preprocess_reads_b:length(offload_read_size) REUSE FREE) \
        nocopy(align_results_a:length(offload_result_size) REUSE FREE) \
        nocopy(align_results_b:length(offload_result_size) REUSE FREE)
        {
        }

    }

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
    //free_mem(device_compute_ratio_temp);
    free_mem(device_read_counts);

    free_mem(sse_preprocess_reads_a);
    free_mem(sse_preprocess_reads_b);
    free_mem(sse_align_results_a);
    free_mem(sse_align_results_b);
    free_mem(sse_dvdh_bit_mem);
    free_mem(sse_result_bucket_counts);

    free_mem(up_signal_a);
    free_mem(up_signal_b);
    free_mem(down_signal_a);
    free_mem(down_signal_b);

    //printf("read_bucket_num is %d\n", read_bucket_num);
    //printf("ref_bucket_num is %d\n", ref_bucket_num);
    printf("===========\n");
    for(i = 0; i < read_bucket_num * ref_bucket_num; i++) {
       for(j = 0; j < total_device_number; j++) {
            if(j == 0) {
                printf("cpu - %d: is %.2fs\n", j, use_times[j][i]);
            } else {
                printf("mic - %d: is %.2fs\n", j, use_times[j][i]);
            }
       }
    }
    printf("============\n");

    for(i = 0; i < total_device_number; i++) {
        free_mem(use_times[i]);
        //free_mem(buffer_ratio[i]);
    }

    free_mem(use_times);
    //free_mem(buffer_ratio);

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
        total_temp += sse_result_bucket_counts[i];
    }
    printf("\n");
    printf("ref_len    is %d\n", ref_len);
    printf("ref_count  is %ld\n", ref_seq.count);
    printf("read_len   is %d\n", read_len);
    printf("read_count is %ld\n\n", total_temp);

    printf("\n");
    printf("read_total_time  is %.2fs\n", read_total_time);
    printf("write_total_time is %.2fs\n", write_total_time);
    printf("mem_total_time is   %.2fs\n", mem_total_time);
    printf("\n");
    for(i = 0; i < total_device_number; i++) {
        if(i == 0) {
            printf("cpu   cal_total_time     is %.2fs\n", cal_total_times[i]);
        } else {
            printf("mic-%d cal_total_time     is %.2fs\n", i, cal_total_times[i]);
        }
    }

    printf("\n");
    for(i = 1; i <= mic_device_number; i++) {
        printf("mic-%d offload_total_time is %.2fs\n", i - 1, offload_total_times[i]);
    }

    printf("\n");
    for(i = 0; i < total_device_number; i++) {
        printf("device_compute_ratio - %d is  %.2f \n", i, device_compute_ratio[i]);
    }

    printf("\n");
    printf("mem time is %.2fs\n", mem_time);
    printf("mem_num is %.2fm\n", mem_num);
    printf("offload_time is %.2fs\n", offload_end - offload_start);
    printf("\n");
    printf("total time                is %.2fs\n", total_end - total_start);
}


void cal_on_all_dynamic() {
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
    int64_t * sse_result_bucket_counts;

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

    int     sse_word_num;
    int     sse_chunk_read_num;
    int     sse_dvdh_size;
    int64_t sse_preprocess_read_size;
    int64_t sse_align_result_size;
    int64_t sse_result_total_count;

    sse_cal_t   sse_arg;
    sse_cal_t * tmp_sse_arg;


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

    GET_TIME(total_start);
    read_total_time = 0;
    write_total_time = 0;
    mem_total_time  = 0;


    int total_device_number;
    int mic_device_number = mic_number;
    int cpu_device_number = 1;
    //mic_device_number = 1;
    total_device_number = mic_device_number + cpu_device_number;

    device_compute_ratio =(double *) malloc_mem(sizeof(double) * total_device_number);
    device_read_counts_a = (int64_t * )malloc_mem(sizeof(int64_t) * total_device_number);
    device_read_counts_b = (int64_t * )malloc_mem(sizeof(int64_t) * total_device_number);

    //init_cpu_mic_ratio(device_compute_ratio, cpu_device_number, mic_device_number);

    if(file_ratio == NULL) {
        init_cpu_mic_ratio(device_compute_ratio, cpu_device_number, mic_device_number);
    } else {
        init_device_ratio_file(device_compute_ratio, cpu_device_number, mic_device_number, file_ratio);
    }

    use_times = (double **)malloc_mem(sizeof(double *) *  total_device_number);
    //loop_device_ratio = (double **)malloc_mem(sizeof(double*) * total_device_number);
    //loop_used_times = (double **) malloc_mem(sizeof(double *) * total_device_number);



    up_signal_a = (char *)malloc_mem(sizeof(char) * mic_device_number);
    up_signal_b = (char *)malloc_mem(sizeof(char) * mic_device_number);
    down_signal_a = (char *)malloc_mem(sizeof(char) * mic_device_number);
    down_signal_b = (char *)malloc_mem(sizeof(char) * mic_device_number);

    double cal_total_times[total_device_number];
    double offload_total_times[total_device_number];
    double previous_device_times[total_device_number];

    for(i = 0; i < total_device_number; i++) {
        cal_total_times[i] = 0;
        offload_total_times[i] = 0;
    }
    omp_set_nested(1);

    init_mapping_table();
    init_device(mic_device_number);
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

    global_ref_bucket_num = ref_bucket_num;

    read_total_size = get_filesize(file_database);
    if(read_total_size > READ_BUCKET_SIZE) {
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (READ_BUCKET_SIZE + 1));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (READ_BUCKET_SIZE + 1));
        read_actual_size = READ_BUCKET_SIZE;
    } else {
        read_seq_a.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2));
        read_seq_b.content = (char *)malloc_mem(sizeof(char) * (read_total_size * 2));
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
    read_actual_size = read_seq_a.size;
    read_total_count = (read_total_size + 1)  / (read_len + 1);

    if(read_total_size > READ_BUCKET_SIZE) {
        read_bucket_num = (read_total_size + read_actual_size - 2) / read_actual_size;
    }

    fwrite(& read_bucket_num, sizeof(int), 1, fp_result_info);
    fwrite(& total_device_number, sizeof(int), 1, fp_result_info);
    fwrite(& ref_seq.count, sizeof(int64_t), 1, fp_result_info);
    fflush(fp_result_info);


    result_bucket_counts = (int64_t *)malloc_mem(sizeof(int64_t) * read_bucket_num);
    sse_result_bucket_counts = (int64_t *)malloc_mem(sizeof(int64_t) * read_bucket_num);

    for(i = 0; i < total_device_number; i++) {
        use_times[i] = (double *)malloc_mem(sizeof(double) * ref_bucket_num * read_bucket_num );
        memset(use_times[i], 0, sizeof(double) * ref_bucket_num * read_bucket_num);

        /*loop_device_ratio[i] = (double *)malloc_mem(sizeof(double) * read_bucket_num);
        memset(loop_device_ratio[i], 0, sizeof(double) * read_bucket_num);

        loop_used_times[i] = (double * )malloc_mem(sizeof(double) * read_bucket_num);
        memset(loop_used_times[i], 0, sizeof(double) * read_bucket_num);*/
    }



    loop_device_ratio = (double **)malloc_mem(sizeof(double*) * read_bucket_num * ref_bucket_num);
    loop_used_times = (double **) malloc_mem(sizeof(double *) * read_bucket_num * ref_bucket_num);

    for(i = 0; i < read_bucket_num * ref_bucket_num; i++) {

        loop_device_ratio[i] = (double *)malloc_mem(sizeof(double) * total_device_number);
        memset(loop_device_ratio[i], 0, sizeof(double) * total_device_number);

        loop_used_times[i] = (double * )malloc_mem(sizeof(double) * total_device_number);
        memset(loop_used_times[i], 0, sizeof(double) * total_device_number);
    }

    {
        //word_num = (read_len + MIC_WORD_SIZE-1) / (MIC_WORD_SIZE );
        int h_threshold = threshold + read_len - ref_len;
        word_num = (read_len - h_threshold + MIC_WORD_SIZE - 1) / MIC_WORD_SIZE + 1;
        chunk_read_num = (max_length + read_len - 1) / read_len;
        offload_dvdh_size = MIC_MAX_THREADS * dvdh_len;
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

        /*if(full_bits) {
            sse_word_num = (read_len + SSE_WORD_SIZE - 1) / SSE_WORD_SIZE;
        } else {
            sse_word_num = (read_len + SSE_WORD_SIZE - 2) / (SSE_WORD_SIZE - 1);
        }*/
        //sse_word_num = (read_len + SSE_WORD_SIZE - 2) / (SSE_WORD_SIZE - 1);
        //sse_chunk_read_num = (max_length + read_len - 1) / read_len;
         sse_word_num = (read_len - h_threshold + SSE_WORD_SIZE - 1) / SSE_WORD_SIZE + 1; 
        sse_preprocess_read_size = sizeof(sse_read_t) * sse_word_num * CHAR_NUM * read_bucket_count;
        sse_preprocess_reads_a = (sse_read_t *) malloc_mem(sse_preprocess_read_size);
        sse_preprocess_reads_b = (sse_read_t *) malloc_mem(sse_preprocess_read_size);
        sse_align_result_size = sizeof(sse_write_t) * ref_bucket_count * read_bucket_count;
        sse_align_results_a = (sse_write_t *)malloc_mem(sse_align_result_size);
        sse_align_results_b = (sse_write_t *)malloc_mem(sse_align_result_size);
        sse_read_ptr = sse_preprocess_reads_a;
        sse_result_ptr = sse_align_results_a;
        sse_dvdh_size = sse_word_num * cpu_threads * dvdh_len;
        sse_dvdh_bit_mem = (__m128i *)malloc_mem(sizeof(__m128i) * sse_dvdh_size);

        GET_TIME(offload_start);

        // 在设备上分配内存
//#pragma omp parallel for num_threads(mic_device_number) default(none) \
        private(i) shared(up_signal_a, ref_seq, ref_total_size, preprocess_reads_a, preprocess_reads_b, offload_read_size, align_results_a, align_results_b, offload_result_size, dvdh_bit_mem, offload_dvdh_size, mic_device_number )


        for(i = 0; i < mic_device_number; i++) {


#pragma offload_transfer target(mic:i) signal(& up_signal_a[i]) \
            nocopy(ref_seq.content:length(ref_total_size + 1) ALLOC RETAIN) \
            nocopy(preprocess_reads_a:length(offload_read_size) ALLOC RETAIN) \
            nocopy(preprocess_reads_b:length(offload_read_size) ALLOC RETAIN) \
            nocopy(align_results_a:length(offload_result_size) ALLOC RETAIN) \
            nocopy(align_results_b:length(offload_result_size) ALLOC RETAIN) \
            nocopy(dvdh_bit_mem:length(offload_dvdh_size) ALLOC RETAIN )
            {

            }
        }

        //printf("1111\n");

        GET_TIME(offload_end);
        sse_result_bucket_counts[0] = device_read_counts_a[0];
        result_bucket_counts[0] = read_seq_a.count - device_read_counts_a[0];

        GET_TIME(process_start);
        GET_TIME(mem_start);
        memset(preprocess_reads_a, 0, preprocess_read_size);
        memset(sse_preprocess_reads_a, 0, sse_preprocess_read_size);
        sse_handle_reads(&read_seq_a, sse_preprocess_reads_a, sse_word_num, 0, device_read_counts_a[0]);
        mic_handle_reads(&read_seq_a, preprocess_reads_a, word_num, device_read_counts_a[0], read_seq_a.count - device_read_counts_a[0]);
        GET_TIME(mem_end);
        mem_total_time = mem_end -mem_start;

        #pragma omp parallel for num_threads(mic_device_number) default(none)\
            private(i) shared(mic_device_number, preprocess_reads_a, up_signal_a, word_num, device_read_counts_a, total_device_number)
        for(i = 1; i < total_device_number; i++) {
            int mic_read_index = 0;
            int k;
            int single_read_count;
            int single_read_size;
            mic_read_t * p_tmp ;
            for(k = 1; k < i; k++) {
                mic_read_index += device_read_counts_a[k];
            }

            single_read_count = device_read_counts_a[i];
            single_read_size = single_read_count * word_num * CHAR_NUM;
            p_tmp = & preprocess_reads_a[mic_read_index * CHAR_NUM * word_num];

            #pragma offload_wait target(mic:(i-1)) wait(& up_signal_a[i-1])

            #pragma offload_transfer target(mic:(i-1)) signal(& up_signal_a[i-1]) \
                in(p_tmp:length(single_read_size) REUSE RETAIN)
        }

        GET_TIME(process_end);
    }


    {
        // 初始化线程
        input_thread_arg in_arg;
        in_arg.fp = fp_read;
        in_arg.total_size = read_total_size;
        in_arg.result_bucket_counts = result_bucket_counts;
        in_arg.sse_result_bucket_counts = sse_result_bucket_counts;
        in_arg.bucket_size = read_actual_size;
        in_arg.bucket_num = read_bucket_num;


        output_thread_arg  out_arg;
        out_arg.fp = fp_result;
        out_arg.result_count = & total_result_count;
        out_arg.sse_result_count = & sse_result_total_count;

        init_resources(4, &input_info, &output_info, &cal_input_info, &cal_output_info);
        input_info.buffer_flag = cal_output_info.buffer_flag = 1;
        cal_input_info.buffer_flag  = output_info.buffer_flag = 0;
        cal_input_info.run_flag = cal_output_info.run_flag = 1;
        input_info.run_flag = output_info.run_flag = 0;
        input_info.shutdown = output_info.shutdown = 0;

        in_arg.input_info = & input_info;
        in_arg.cal_input_info = & cal_input_info;

        in_arg.read_seq_a = & read_seq_a;
        in_arg.read_seq_b = & read_seq_b;

        out_arg.output_info = & output_info;
        out_arg.cal_output_info = & cal_output_info;
        out_arg.align_results_a =  align_results_a;
        out_arg.align_results_b =  align_results_b;
        out_arg.sse_align_results_a = sse_align_results_a;
        out_arg.sse_align_results_b = sse_align_results_b;
        out_arg.mic_device_number = mic_device_number;

        pthread_create(&(input_info.thread_id), NULL, input_task_all_dynamic, & in_arg);
        pthread_create(&(output_info.thread_id), NULL, output_task_all, & out_arg);
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
            sse_read_ptr = sse_preprocess_reads_a;
            device_read_counts = device_read_counts_a;
            read_seq = & read_seq_a;
        } else {
            read_ptr = preprocess_reads_b;
            sse_read_ptr = sse_preprocess_reads_b;
            device_read_counts = device_read_counts_b;
            read_seq = & read_seq_b;
        }



        for(i = 0; i < mic_device_number; i++) {
            tmp_cal_arg = & mic_args[i];
            tmp_cal_arg->ref_seq =  & ref_seq;
            tmp_cal_arg->read_seq = read_seq;
            tmp_cal_arg->dvdh_bit_mem = dvdh_bit_mem;
            tmp_cal_arg->word_num = word_num;
            tmp_cal_arg->chunk_read_num = chunk_read_num;
            tmp_cal_arg->offload_dvdh_size = offload_dvdh_size;
            tmp_cal_arg->mic_index = i;
            tmp_cal_arg->cal_total_time = & cal_total_times[i+1];
            tmp_cal_arg->offload_total_time = & offload_total_times[i+1];
        }

        tmp_sse_arg = & sse_arg;
        tmp_sse_arg->ref_seq =  & ref_seq;
        tmp_sse_arg->read_seq = read_seq;
        tmp_sse_arg->dvdh_bit_mem = sse_dvdh_bit_mem;
        tmp_sse_arg->word_num = sse_word_num;
        tmp_sse_arg->chunk_read_num = sse_chunk_read_num;
        tmp_sse_arg->cal_total_time = & cal_total_times[0];
        tmp_sse_arg->read_ptr = sse_read_ptr;


        if(read_bucket_index != 0) {
            dispatch_task(device_compute_ratio, device_read_counts, read_seq->count, total_device_number);
            sse_result_bucket_counts[read_bucket_index] = device_read_counts[0];
            result_bucket_counts[read_bucket_index] = read_seq->count - device_read_counts[0];
            memset(read_ptr, 0, preprocess_read_size);
            memset(sse_read_ptr, 0, sse_preprocess_read_size);
            sse_handle_reads(read_seq, sse_read_ptr, sse_word_num, 0, device_read_counts[0]);
            mic_handle_reads(read_seq, read_ptr, word_num, device_read_counts[0], read_seq->count - device_read_counts[0]);

        #pragma omp parallel for num_threads(mic_device_number) default(none) \
            private(i, k) \
            shared(up_signal_a, up_signal_b, total_device_number, word_num, read_ptr, input_info, device_read_counts, cal_input_info)
            for(i = 1; i < total_device_number; i++) {
                int mic_read_index = 0;
                int single_read_count;
                int single_read_size;
                mic_read_t * p_tmp;
                for(k = 1; k < i; k++) {
                    mic_read_index += device_read_counts[k];
                }
                single_read_count = device_read_counts[i];
                single_read_size = single_read_count * word_num * CHAR_NUM;
                p_tmp = & read_ptr[mic_read_index * CHAR_NUM * word_num];

                if(cal_input_info.buffer_flag) {
                    #pragma offload_transfer target(mic:(i-1)) signal( & up_signal_a[i-1]) \
                        in (p_tmp:length(single_read_size) REUSE RETAIN)
                } else {
                    #pragma offload_transfer target(mic:(i-1)) signal(& up_signal_b[i-1]) \
                        in (p_tmp:length(single_read_size) REUSE RETAIN)
                }
            }
        }

        fwrite(device_read_counts, sizeof(int64_t), total_device_number, fp_result_info);
        fwrite(& read_seq->extra_count, sizeof(int), 1, fp_result_info);
        fflush(fp_result_info);

        for(i = 0; i < total_device_number; i++) {
            previous_device_times[i] = 0;
        }

        if(cal_input_info.buffer_flag) {
            for(i = 0; i < mic_device_number; i++) {
                #pragma offload_wait target(mic:i) wait(& up_signal_a[i])
            }
        } else {
            for(i = 0; i < mic_device_number; i++) {
                #pragma offload_wait target(mic:i) wait(& up_signal_b[i])
            }
        }

        for(ref_bucket_index = 0; ref_bucket_index < ref_bucket_num; ref_bucket_index++) {

            if(cal_output_info.buffer_flag) {
                result_ptr = align_results_a;
                sse_result_ptr = sse_align_results_a;
            } else {
                result_ptr = align_results_b;
                sse_result_ptr = sse_align_results_b;
            }

            if(ref_bucket_index == ref_bucket_num - 1) {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = ref_seq.count;
            } else {
                ref_start = ref_bucket_index * ref_bucket_count;
                ref_end = (ref_bucket_index + 1) * ref_bucket_count;
            }

            int mic_read_index = 0;

#pragma  omp parallel for num_threads(total_device_number) default(none) \
            private(i, tmp_cal_arg, tmp_sse_arg, single_read_count, single_read_size, single_result_size, mic_read_index) \
            shared(mic_args, sse_arg, total_device_number, mic_device_number, read_seq, read_ptr, result_ptr, ref_start, ref_end, word_num, sse_result_ptr, device_read_counts, cal_output_info , previous_device_times)
            for(i = 0; i < total_device_number; i++) {
                if(i == 0) {
                    double time_start, time_end;

                    GET_TIME(time_start);
                    tmp_sse_arg = & sse_arg;
                    tmp_sse_arg->sse_ref_start = ref_start;
                    tmp_sse_arg->sse_ref_end = ref_end;
                    tmp_sse_arg->read_ptr_offset =  0;
                    tmp_sse_arg->result_ptr_offset = 0 ;
                    tmp_sse_arg->result_ptr = sse_result_ptr;
                    tmp_sse_arg->read_count = device_read_counts[0];
                    single_read_count = device_read_counts[0];
                    single_result_size = (ref_end - ref_start) * single_read_count;
                    sse_cal_all(tmp_sse_arg);
                    GET_TIME(time_end);
                    previous_device_times[i] += (time_end - time_start) * (1.04);

                } else {


                    double time_start, time_end;
                    GET_TIME(time_start);
                    tmp_cal_arg = & mic_args[i-1];

                    int k = 0;
                    mic_read_index = 0;
                    for(k = 1; k < i; k++) {
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
                    if(cal_output_info.buffer_flag) {
                        tmp_cal_arg->async_flag = 0;
                    } else {
                        tmp_cal_arg->async_flag = 1;
                    }
                    mic_cal_all(tmp_cal_arg);
                    GET_TIME(time_end);
                    previous_device_times[i] += time_end - time_start;
                }
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
            sse_result_total_count = (ref_end - ref_start) * sse_result_bucket_counts[read_bucket_index];
            total_result_count = (ref_end - ref_start) * result_bucket_counts[read_bucket_index];
            pthread_mutex_unlock(&(output_info.lock));
            pthread_cond_signal(&(output_info.cond));
        }


        //adjust_device_ratio(device_compute_ratio, previous_device_times, total_device_number);
        //adjust_device_ratio2(device_compute_ratio, previous_device_times, total_device_number);
        adjust_device_ratio3(device_compute_ratio, previous_device_times, total_device_number);

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

    for(i = 0; i < mic_device_number; i++) {

#pragma offload target(mic:i) \
        nocopy(dvdh_bit_mem:length(offload_dvdh_size) REUSE FREE)\
        nocopy(ref_seq.content:length(ref_total_size + 1) REUSE FREE) \
        nocopy(preprocess_reads_a:length(offload_read_size) REUSE FREE) \
        nocopy(preprocess_reads_b:length(offload_read_size) REUSE FREE) \
        nocopy(align_results_a:length(offload_result_size) REUSE FREE) \
        nocopy(align_results_b:length(offload_result_size) REUSE FREE)
        {
        }

    }

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
    //free_mem(device_compute_ratio);
    //free_mem(device_compute_ratio_temp);
    free_mem(device_read_counts);

    free_mem(sse_preprocess_reads_a);
    free_mem(sse_preprocess_reads_b);
    free_mem(sse_align_results_a);
    free_mem(sse_align_results_b);
    free_mem(sse_dvdh_bit_mem);
    free_mem(sse_result_bucket_counts);

    free_mem(up_signal_a);
    free_mem(up_signal_b);
    free_mem(down_signal_a);
    free_mem(down_signal_b);

    //printf("read_bucket_num is %d\n", read_bucket_num);
    //printf("ref_bucket_num is %d\n", ref_bucket_num);
    printf("===========\n");
    for(i = 0; i < read_bucket_num * ref_bucket_num; i++) {
       for(j = 0; j < total_device_number; j++) {
            if(j == 0) {
                printf("cpu - %d: is %.2fs\n", j, use_times[j][i]);
            } else {
                printf("mic - %d: is %.2fs\n", j, use_times[j][i]);
            }
       }
       printf("\n");
    }
    printf("============\n");

    for(i = 0; i < total_device_number; i++) {
        free_mem(use_times[i]);
        //free_mem(buffer_ratio[i]);
    }


    for(i = 0; i < read_bucket_num; i++) {
        free_mem(loop_device_ratio[i]);
        free_mem(loop_used_times[i]);

    }
    free_mem(use_times);
    free_mem(loop_used_times);
    free_mem(loop_device_ratio);
    //free_mem(buffer_ratio);

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
        total_temp += sse_result_bucket_counts[i];
    }
    printf("\n");
    printf("query_len    is %d\n", ref_len);
    printf("query_count  is %ld\n", ref_seq.count);
    printf("subject_len   is %d\n", read_len);
    printf("subject_count is %ld\n\n", total_temp);

    printf("\n");
    printf("read_total_time  is %.2fs\n", read_total_time);
    printf("write_total_time is %.2fs\n", write_total_time);
    printf("mem_total_time is   %.2fs\n", mem_total_time);
    printf("\n");
    for(i = 0; i < total_device_number; i++) {
        if(i == 0) {
            printf("cpu   cal_total_time     is %.2fs\n", cal_total_times[i]);
        } else {
            printf("mic-%d cal_total_time     is %.2fs\n", i-1, cal_total_times[i]);
        }
    }

    printf("\n");
    for(i = 1; i <= mic_device_number; i++) {
        printf("mic-%d offload_total_time is %.2fs\n", i - 1, offload_total_times[i]);
    }

    printf("\n");
    for(i = 0; i < total_device_number; i++) {
        printf("device_compute_ratio - %d is  %.2f \n", i, device_compute_ratio[i]);
    }

    //free_mem(cal_total_times);
    //free_mem(offload_total_times);
    //free_mem(device_compute_ratio);

    printf("\n");
    printf("mem time is %.2fs\n", mem_time);
    printf("mem_num is %.2fm\n", mem_num);
    printf("offload_time is %.2fs\n", offload_end - offload_start);
    printf("\n");
    printf("total time                is %.2fs\n", total_end - total_start);
}
