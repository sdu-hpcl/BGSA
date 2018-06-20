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

#define MIC_V_NUM 16
#define MIC_WORD_SIZE 32
#define MIC_SIZE 512
#define mic_read_t uint32_t
#define mic_write_t common_write_t
#define mic_data_t __m512i

#endif
