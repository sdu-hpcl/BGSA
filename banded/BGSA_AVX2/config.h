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

#define common_write_t int8_t

#define AVX_SIZE 256
#define avx_write_t common_write_t


#define avx_data_t __m256i

#define MAX_ERROR 127

#define avx_cmp_result_t int
#define avx_result_mask 0xffffffff
#define batch_size 16

#define AVX_V_NUM 16
#define AVX_WORD_SIZE 16
#define avx_read_t uint16_t
#define avx_result_t int16_t

#endif
