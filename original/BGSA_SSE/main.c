#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <omp.h>

#include "cal.h"
#include "global.h"

char *  file_query;
char *  file_database;
char *  file_result;
char *  file_result_info;
char *  file_ratio;

int     cpu_threads;

void print_help() {

    printf("\nUsage: ./aligner [options]\n\n");
    printf("Commandline options:\n\n");

    printf("  %-30s\n", "-q <arg>");
    printf("\t Query file. If the file is FASTA or FASTQ format, you should convert it to specified format using the converting program. \n\n");
    printf("  %-30s\n", "-d <arg>");
    printf("\t Database file. If the file is FASTA or FASTQ format, you should convert it to specified format using he converting program. \n\n");
    printf("  %-30s\n", "-f <arg>");
    printf("\t Alignment result file. \n\n");
    printf("  %-30s\n", "-N <arg>");
    printf("\t Number of used threads. Default is the maximum thread number. \n\n");
    printf("  %-30s\n", "-h");
    printf("\t Print help. \n\n");
    exit(1);
}

void handle_args(int argc, char ** argv) {
    char c;
    cpu_threads = omp_get_max_threads();
    file_result = "data/result.txt";

    while((c = getopt(argc, argv, "t:q:d:f:n:N:R:D")) != -1) {
        switch (c) {
            case 'q':
                file_query = optarg;
                break;
            case 'd':
                file_database = optarg;
                break;
            case 'f':
                file_result = optarg;
                break;
            case 'N':
                cpu_threads = atoi(optarg);
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
    // file_query = "sample-data/query.txt";
    // file_database = "sample-data/subject.txt";
    file_result = "data/result.txt";

    handle_args(argc, argv);

    int i;
    int result_length = strlen(file_result);

    file_result_info =(char *) malloc_mem(result_length+6);
    for(i = 0; i < result_length; i++) {
        file_result_info[i] = file_result[i];
    }
    file_result_info[i] = '\0';
    strcat(file_result_info, ".info");

    max_length = 4000;

    if(file_query == NULL) {
        printf("Query file can't be empty.\n");
        exit(1);
    }

    if(file_database == NULL) {
        printf("Database file can't be empty. \n");
        exit(1);
    }
    cal_on_sse();

    free_mem(file_result_info);

    return 0;
}
