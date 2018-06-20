#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "file.h"

FILE * open_file(const char * filename, const char * mode) {
    FILE * fp;
    fp = fopen(filename, mode);
    if(fp == NULL) {
        printf("Error - can'jjt open or create file: %s\n", filename);
        exit(1);
    }
    return fp;
}

void delete_file(const char * filename) {
    int result = remove(filename);
    if(result != 0) {
        printf("Error - can't delete file: %s\n", filename);
        exit(1);
    }
}

void create_folder(const char * foldername, mode_t mode) {
    int is_exist = access(foldername, 0);
    if(is_exist == -1) {
        int success = mkdir(foldername, mode);
        if(success == -1) {
            printf("Error - can't create folder: %s\n", foldername);
        }
    }
}

uint64_t get_filesize(const char * filename) {
	struct stat buf;
    stat(filename, &buf);
    return buf.st_size;
}

void get_read_from_file(seq_t * seq, FILE * fp, int64_t section_size, int64_t total_size) {
    int i = 0, j;
    int is_end = 0;
    int64_t read_size = 0;
    seq->len = 0;
    seq->count = 0;
    seq->extra_size = 0;
    seq->extra_count = 0;
    read_size = fread(seq->content, 1, section_size, fp);
    section_size = read_size;
    seq->size = read_size;

    //printf("read_size is %ld\n", read_size);
    for(i = 0; ;i++) {
        if(seq->content[i] == '\n') {
            break;
        }
        seq->len++;
    }

    if(ftell(fp) == total_size) {
        is_end = 1;

        if(seq->content[section_size - 1] != '\n') {
            seq->content[section_size] = '\n';
            section_size++;
            seq->size++;
        }
    }


    if(is_end == 0) {
        int extra_count = 0;
        for(i = section_size - 1;i >= 0; i--) {
            if(seq->content[i] == '\n') {
                break;
            }
            extra_count++;
        }

        seq->count = (seq->size - extra_count + seq->len) / (seq->len + 1);
        while(1){
            if(seq->count % SSE_V_NUM == 0) {
                break;
            }
            seq->count--;
            extra_count += (seq->len + 1);
        }
        seq->size -= extra_count;
        fseek(fp, -extra_count, SEEK_CUR);
        //printf("seq->count is %d\n", seq->count);

    } else {
        seq->count = (seq->size + seq->len) / (seq->len + 1);
        //printf("seq->count final_before is %d\n", seq->count);
        while(1) {
            if(seq->count % SSE_V_NUM == 0) {
                break;
            }
            seq->count++;
            seq->extra_count++;
            for(j = section_size; j < section_size + seq->len; j++) {
                seq->content[j] = 'N';
            }
            seq->content[j] = '\n';
            section_size += seq->len + 1;
            seq->size += seq->len + 1;
       }

        //printf("seq->count  final is %d\n", seq->count);
    }
}

void get_ref_from_file(seq_t * seq, FILE * fp, uint64_t total_size){
    int i = 0;
    seq->len = 0;
    seq->count = 0;
    seq->size = total_size;
    fread(seq->content, 1, total_size, fp);
    for(i = 0; ;i++) {
        if(seq->content[i] == '\n') {
            break;
        }
        seq->len++;
    }
    if(seq->content[total_size - 1] != '\n') {
        seq->content[total_size] = '\n';
        seq->size++;
    }

    seq->count = seq->size / (seq->len + 1);
    for(i = 0; i < seq->size; i++) {
        if(seq->content[i] != '\n') {
            seq->content[i] = mapping_table[(int)seq->content[i]];
        }
    }
}
