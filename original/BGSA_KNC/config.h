#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define READ_BUCKET_SIZE 10240
//#define READ_BUCKET_SIZE 22428800
#define READ_BUCKET_SIZE 114857600
//#define READ_BUCKET_SIZE 157286400
//#define READ_BUCKET_SIZE 54857600
//#define READ_BUCKET_SIZE 57671680

#define mic_cpu_ratio 4.3

#define REF_BUCKET_COUNT 10000

//#define BUFFER_SIZE 104857600
//#define BUFFER_SIZE 40960

#define CHAR_NUM 5

#define common_write_t int16_t

#define MIC_V_NUM 16
#define MIC_WORD_SIZE 32
#define MIC_SIZE 512
#define mic_read_t uint32_t
#define mic_write_t common_write_t
#define mic_data_t __m512i

#define SSE_V_NUM 4
#define SSE_WORD_SIZE 32
#define SSE_SIZE 128
#define sse_read_t uint32_t
#define sse_write_t common_write_t
#define sse_data_t __m128i

#define CPU_V_NUM 1
#define CPU_WORD_SIZE 64
#define CPU_SIZE 64
//#define CPU_MAX_THREADS 24
#define cpu_read_t uint64_t
#define cpu_write_t common_write_t
#define cpu_data_t uint64_t

#endif
