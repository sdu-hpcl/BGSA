#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <immintrin.h>
#include <sys/stat.h>
#include <string.h>
#include <getopt.h>
#include "config.h"

#define RESULT_TYPE common_write_t

static int buffer_size = 10485760;

char * input_file_name;
char * info_file_name;
char * output_file_name;
int convert_type = 0;

uint64_t get_filesize(char * filename) {
    struct stat buf;
    stat(filename, &buf);
    return buf.st_size;
}

void * malloc_mem(uint64_t size) {
    return _mm_malloc(size, 64);
}

void free_mem(void * mem) {
    _mm_free(mem);
}

void convert_fasta(char * input_file, char * output_file) {
    FILE * fp_input;
    FILE * fp_output;
    char start_char = '>';
    char endline_char = '\n';

    fp_input = fopen(input_file, "rb");
    fp_output = fopen(output_file, "w+");
    if(fp_input == NULL) {
        printf("Error - can't open or create file: %s\n", input_file);
        exit(1);
    }

    if(fp_output == NULL) {
        printf("Error - can't open or create file: %s\n", output_file);
        exit(1);
    }

    char * input_buffer;
    char * output_buffer;
    
    input_buffer = (char *)malloc_mem(buffer_size);
    output_buffer = (char *) malloc_mem(buffer_size);

    int i = 0;
    int length = 0;
    int desc_start = 0;
    int output_index = 0;
    int is_start = 0;
    while(length = fread(input_buffer, sizeof(char), buffer_size, fp_input)) {
        output_index = 0;
        for(i = 0; i < length; i++) {
            if(input_buffer[i] == start_char) {
                desc_start = 1;
                if(is_start != 0) {
                    output_buffer[output_index++] = endline_char;
                }
                else {
                    is_start = 1;
                }
                continue;
            }
            if(desc_start && input_buffer[i] == endline_char) {
                desc_start = 0;
                continue;
            }
            if(!desc_start) {
                if(input_buffer[i] != endline_char) {
                    output_buffer[output_index++] = input_buffer[i];
                }
            }
            
        }    
        output_buffer[output_index] = '\0';
        fputs(output_buffer, fp_output);
    }

    fputc('\n', fp_output);
    
    fclose(fp_input);
    fclose(fp_output);
    free_mem(input_buffer);
    free_mem(output_buffer);

}

void convert_fastq(char * input_file, char * output_file) {
    FILE * fp_input;
    FILE * fp_output;
    char order_char = '@';
    char endline_char = '\n';
    fp_input = fopen(input_file, "rb");
    fp_output = fopen(output_file, "w+");
    if(fp_input == NULL) {
        printf("Error - can't open or create file: %s\n", input_file);
        exit(1);
    }

    if(fp_output == NULL) {
        printf("Error - can't open or create file: %s\n", output_file);
        exit(1);
    }

    char * input_buffer;
    char * output_buffer;
    
    input_buffer = (char *)malloc_mem(buffer_size);
    output_buffer = (char *) malloc_mem(buffer_size);

    int i = 0;
    int length = 0;
    int order_start = 0;
    int output_index = 0;
    int is_start = 0;
    while(length = fread(input_buffer, sizeof(char), buffer_size, fp_input)) {
        output_index = 0;
        for(i = 0; i < length; i++) {
            if(input_buffer[i] == order_char) {
                order_start = 1;
                if(is_start != 0) {
                    output_buffer[output_index++] = endline_char;
                }
                else {
                    is_start = 1;
                }
                continue;
            }
            if(order_start == 1 && input_buffer[i] == endline_char) {
                order_start = 0;
                continue;
            }
            if(!order_start) {
                if(input_buffer[i] != endline_char) {
                    output_buffer[output_index++] = input_buffer[i];
                } else {
                    order_start = 2;
                }
            }
            
        }    
        output_buffer[output_index] = '\0';
        fputs(output_buffer, fp_output);
        
    }

    fputc('\n', fp_output);
    
    fclose(fp_input);
    fclose(fp_output);
    free_mem(input_buffer);
    free_mem(output_buffer);

}

void convert_result(FILE * fp_input, FILE * fp_info, FILE *fp_output) {
    int i, j, k, m, temp, temp2;
    int block_num = 0;
    int64_t ref_count = 0;
    int total_device_number = 0;
    int ref_bucket_num;
    int ref_start, ref_end;
    int distance;
    int last_ref_bucket_count;
    int64_t max_block_size = 0;
    RESULT_TYPE * buffer;


    fread(&block_num, sizeof(int), 1, fp_info);
    fread(&total_device_number, sizeof(int), 1, fp_info);
    fread(&ref_count, sizeof(int64_t), 1, fp_info);

    int64_t device_read_counts[block_num][total_device_number];
    int extra_counts[block_num];

    for(i = 0; i < block_num; i++) {
        fread(device_read_counts[i], sizeof(int64_t), total_device_number, fp_info);
        fread( & extra_counts[i], sizeof(int), 1, fp_info);
    }

    for(i = 0; i < block_num; i++) {
        for(j = 0; j < total_device_number; j++) {
            if(max_block_size < device_read_counts[i][j]) {
                max_block_size = device_read_counts[i][j];
            }
        }
    }

    buffer = (RESULT_TYPE * )_mm_malloc(sizeof(RESULT_TYPE) * max_block_size, 64);

    for(i = 0; i < block_num; i++) {

        for(j = 0; j < total_device_number; j++) {

            printf("read_count[%d][%d] is %d\n",i, j, device_read_counts[i][j]);
        }
    }

    
    last_ref_bucket_count = ref_count % REF_BUCKET_COUNT;
    ref_bucket_num = ref_count / REF_BUCKET_COUNT;
    if(last_ref_bucket_count == 0) {
        last_ref_bucket_count = REF_BUCKET_COUNT;
    } else {
        ref_bucket_num++;
    }

    for(m = 0; m < ref_bucket_num; m++) {
        if(m == ref_bucket_num - 1) {
            ref_start = m * REF_BUCKET_COUNT;
            ref_end = ref_count;
        } else {
            ref_start = m * REF_BUCKET_COUNT;
            ref_end = (m + 1) * REF_BUCKET_COUNT;
        }

        for(i = ref_start; i < ref_end; i++) {
            rewind(fp_input);

            for(j = 0; j < block_num; j++) {

                for(temp = 0; temp < m; temp++) {
                    distance = 0;
                    for(temp2 = 0; temp2 < total_device_number; temp2++) {
                        distance += REF_BUCKET_COUNT * device_read_counts[j][temp2] * sizeof(RESULT_TYPE);
                    }
                    fseek(fp_input, distance, SEEK_CUR);
                }

                for(temp2 = 0; temp2 < total_device_number; temp2++) {
                    int extra_tmp = 0;
                    fseek(fp_input, (i - ref_start) * device_read_counts[j][temp2] * sizeof(RESULT_TYPE), SEEK_CUR);
                    fread(buffer, sizeof(RESULT_TYPE), device_read_counts[j][temp2], fp_input);
                    if(temp2 == total_device_number - 1) {
                        extra_tmp = extra_counts[j];
                    }
                    
                    
                    for(k = 0; k < device_read_counts[j][temp2] - extra_tmp; k++) {
                        fprintf(fp_output, "%d\n", buffer[k]);
                    }
                    distance = (ref_end - 1 - i) * device_read_counts[j][temp2] * sizeof(RESULT_TYPE);
                    fseek(fp_input, distance, SEEK_CUR);
                }

                for(temp = m + 1; temp < ref_bucket_num; temp++) {
                    distance = 0;
                    for(temp2 = 0; temp2 < total_device_number; temp2++) {
                        if(temp == ref_bucket_num - 1) {
                            distance += last_ref_bucket_count * device_read_counts[j][temp2] * sizeof(RESULT_TYPE);
                        } else {
                            distance += REF_BUCKET_COUNT * device_read_counts[j][temp2] * sizeof(RESULT_TYPE);
                        }
                    }
                    
                    fseek(fp_input, distance, SEEK_CUR);
                }
            }

        }

    }

    _mm_free(buffer);

}


void print_help() {
    printf("\nUsage: ./convert [options]\n\n");
    printf("Commandline options:\n\n");
    printf("  %-30s\n", "-f <arg>");
    printf("\t Convert the FATSA file to needed format. \n\n");
    printf("  %-30s\n", "-q <arg>");
    printf("\t Convert the FATSQ file to needed format. \n\n");
    printf("  %-30s\n", "-r <arg>");
    printf("\t Convert the result file to readable format. \n\n");
    printf("  %-30s\n", "-o <arg>");
    printf("\t Output file. \n\n");
    printf("  %-30s\n", "-h");
    printf("\t Print help. \n\n");
    exit(1);
}

void handle_args(int argc, char ** argv) {
    char c;
    output_file_name = "convert_result.txt";
    if(argc == 1) {
        print_help();
    }

    while((c = getopt(argc, argv, "f:q:r:o:")) != -1) {
        switch (c) {
            case 'f':
                convert_type = 0;
                input_file_name = optarg;
                break;
            case 'q':
                convert_type = 1;
                input_file_name = optarg;
                break;
            case 'r':
                convert_type = 2;
                input_file_name = optarg;
                break;
            case 'o':
                output_file_name = optarg;
                break;
            case '?':
                print_help();
                break;
            default:
                print_help();
        }
    }
    
}


int main(int argc, char ** argv) {
    int i;
    int result_length;
    handle_args(argc, argv);

    if(input_file_name == NULL) {
        printf("Input file can't be empty.\n");
        exit(1);
    }

    switch(convert_type) {
        case 0:
            convert_fasta(input_file_name, output_file_name);
            break;
        case 1:
            convert_fastq(input_file_name, output_file_name);
            break;
        case 2:
           
            result_length = strlen(input_file_name);

            info_file_name =(char *) malloc_mem(result_length+6);
            for(i = 0; i < result_length; i++) {
                info_file_name[i] = input_file_name[i];
            }
            info_file_name[i] = '\0';
            strcat(info_file_name, ".info");
            FILE * fp_input, * fp_info, *fp_output;
            if((fp_input = fopen(input_file_name, "rb")) == NULL) {
                printf("Can't read result file\n");
                exit(1);
            }
            if((fp_info= fopen(info_file_name, "rb")) == NULL) {
                printf("Can't read result info file\n");
                exit(1);
            }
            if((fp_output= fopen(output_file_name, "w+")) == NULL) {
                printf("Can't create output file\n");
                exit(1);
            }
            convert_result(fp_input, fp_info, fp_output);
            free_mem(info_file_name);
            fclose(fp_input);
            fclose(fp_info);
            fclose(fp_output);

    }

}
