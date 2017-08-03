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

#define common_write_t int16_t

#define AVX_V_NUM 8
#define AVX_WORD_SIZE 32
#define AVX_SIZE 256
#define AVX_MAX_THREADS 240
#define avx_read_t uint32_t
#define avx_write_t common_write_t

#endif
