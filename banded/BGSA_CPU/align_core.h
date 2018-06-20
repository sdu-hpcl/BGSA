#ifndef _ALIGN_CORE_H_
#define _ALIGN_CORE_H_

#include <stdint.h>

#include "global.h"

void align_cpu(char * ref, cpu_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, cpu_write_t * results, uint64_t * dvdh_bit_mem);

#endif
