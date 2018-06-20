#ifndef _ALIGN_CORE_H_
#define _ALIGN_CORE_H_

#include <stdint.h>

#include "global.h"


void align_sse(char * ref, sse_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, sse_write_t * results, __m128i * dvdh_bit_mem);




#endif
