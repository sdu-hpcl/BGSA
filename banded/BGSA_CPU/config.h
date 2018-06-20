#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define READ_BUCKET_SIZE 10240
//#define READ_BUCKET_SIZE 22428800
#define READ_BUCKET_SIZE 114857600
//#define READ_BUCKET_SIZE 157286400
//#define READ_BUCKET_SIZE 54857600
//#define READ_BUCKET_SIZE 57671680


#define REF_BUCKET_COUNT 100

//#define BUFFER_SIZE 104857600
//#define BUFFER_SIZE 40960

#define CHAR_NUM 5

#define MAX_ERROR 127
#define batch_size 16
#define common_write_t int8_t

#define CPU_V_NUM 1
#define CPU_WORD_SIZE 64
#define CPU_SIZE 64
#define cpu_read_t uint64_t
#define cpu_write_t common_write_t
#define cpu_data_t uint64_t 
#define cpu_cmp_result_t int64_t
#define cpu_result_t int64_t
#endif
