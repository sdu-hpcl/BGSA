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

#define MIC_SIZE 512
#define MIC_MAX_THREADS 256
#define mic_write_t common_write_t


#define MAX_ERROR 127
#define mic_cmp_result_t __mmask8
#define mic_result_mask 0xff
#define batch_size 16
#define mic_data_t __m512i

#define MIC_V_NUM 8
#define MIC_WORD_SIZE 64
#define mic_read_t uint64_t
#define mic_result_t int64_t

#endif
