#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define READ_BUCKET_SIZE 10240
//#define READ_BUCKET_SIZE 22428800
#define READ_BUCKET_SIZE 114857600
//#define READ_BUCKET_SIZE 157286400
//#define READ_BUCKET_SIZE 54857600
//#define READ_BUCKET_SIZE 57671680


#define REF_BUCKET_COUNT 10000

//#define BUFFER_SIZE 104857600
//#define BUFFER_SIZE 40960

#define CHAR_NUM 5

#define common_write_t int8_t

#define MIC_SIZE 512
#define MIC_MAX_THREADS 240
#define MAX_ERROR 127
#define batch_size 16
#define common_read_t uint64_t
#define common_result_t int64_t 

#define SSE_V_NUM 2 
#define SSE_WORD_SIZE 64
#define SSE_SIZE 128
#define sse_data_t __m128i
#define sse_read_t common_read_t
#define sse_write_t common_write_t
#define sse_cmp_result_t uint16_t 
#define sse_result_mask 0xffff
#define sse_result_t common_result_t


#define CPU_V_NUM 1
#define CPU_WORD_SIZE 64
#define CPU_SIZE 64
#define cpu_read_t common_read_t
#define cpu_write_t common_write_t
#define cpu_data_t uint64_t 
#define cpu_cmp_result_t int64_t 
#define cpu_result_t int64_t


#define MIC_V_NUM 8
#define MIC_WORD_SIZE 64
#define mic_data_t __m512i 
#define mic_read_t common_read_t
#define mic_result_t common_result_t 
#define mic_cmp_result_t __mmask16
#define mic_result_mask 0xffff
#define mic_write_t common_write_t
#endif
