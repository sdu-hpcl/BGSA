#ifndef _ALIGN_CORE_H_
#define _ALIGN_CORE_H_

#include <stdint.h>

#include "global.h"

void align_avx(char * ref, avx_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, avx_write_t * results, __m256i * dvdh_bit_mem);

#endif
