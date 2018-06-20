#ifndef _FILEH_
#define _FILEH_

#include <stdio.h>
#include <sys/types.h>

#include "global.h"

FILE * open_file(const char * filename, const char * mode);

void delete_file(const char * filename);

void create_folder(const char * foldname, mode_t mode);

uint64_t get_filesize(const char * filename);

void get_read_from_file(seq_t * seq, FILE * fp, int64_t section_size, int64_t total_size);

void get_ref_from_file(seq_t * seq, FILE * fp, uint64_t total_size);


#endif
