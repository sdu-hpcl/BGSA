#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <omp.h>
#include <offload.h>

#include "cal.h"
#include "global.h"

char *file_query;
char *file_database;
char *file_result;
char *file_result_info;
char *file_ratio;


int implement_type;
int mic_number;
int cpu_threads;
__ONMIC__ int mic_threads;
int use_dynamic;

void print_help() {

	printf("\nUsage: ./aligner [options]\n\n");
	printf("Commandline options:\n\n");

	printf("  %-30s\n", "-t <arg>");
	printf("\t Implementation types, valid values are: SIMPLE, SSE, MIC, MICSSE. Default is MIC.\n\n");
	printf("  %-30s\n", "-q <arg>");
	printf("\t Query file. If the file is FASTA or FASTQ format, you should convert it to specified format using the converting program. \n\n");
	printf("  %-30s\n", "-d <arg>");
	printf("\t Database file. If the file is FASTA or FASTQ format, you should convert it to specified format using he converting program. \n\n");
	printf("  %-30s\n", "-f <arg>");
	printf("\t Alignment result file. \n\n");
	printf("  %-30s\n", "-n <arg>");
	printf("\t Number of used Xeon Phi if Xeon Phi is enabled. Default is the available Xeon Phi number. \n\n");
	printf("  %-30s\n", "-N <arg>");
	printf("\t Number of used threads on CPU. Default is the maximum thread number. \n\n");
	printf("  %-30s\n", "-M <arg>");
	printf("\t Number of used threads on MIC. Default is the maximum thread number. \n\n");
	printf("  %-30s\n", "-R <arg>");
	printf("\t Distribution ratio bwtween multi-devices, you should input a file and each ratio number should be separated with a space like: 1 4.3 4.3 .\n\n");
	printf("  %-30s\n", "-D");
	printf("\t If present, the distribution ratio will adjust dynamically when program running. \n\n");
	printf("  %-30s\n", "-h");
	printf("\t Print help. \n\n");
	exit(1);
}

void handle_args(int argc, char **argv) {
	char c;
	implement_type = 2;
	mic_number = _Offload_number_of_devices();
//	printf("mic_number is %d\n", mic_number);
	if (mic_number == 0) {
		print_help();
	}
	mic_threads = omp_get_max_threads_target(TARGET_MIC, 0);
//	printf("%d\n", mic_threads);
	cpu_threads = omp_get_max_threads();
	use_dynamic = 0;
	file_result = "data/result.txt";

	/*if(argc == 1) {
		print_help();
	}*/

	while ((c = getopt(argc, argv, "t:q:d:f:n:N:M:R:D")) != -1) {
		switch (c) {
			case 't':
				if (strcmp(optarg, "SIMPLE") == 0) {
					implement_type = 0;
				} else if (strcmp(optarg, "SSE") == 0) {
					implement_type = 1;
				} else if (strcmp(optarg, "MIC") == 0) {
					implement_type = 2;
				} else {
					implement_type = 3;
				}
				break;
			case 'q':
				file_query = optarg;
				break;
			case 'd':
				file_database = optarg;
				break;
			case 'f':
				file_result = optarg;
				break;
			case 'n':
				mic_number = atoi(optarg);
				break;
			case 'N':
				cpu_threads = atoi(optarg);
				break;
			case 'M':
				mic_threads = atoi(optarg);
				break;
			case 'R':
				file_ratio = optarg;
				break;
			case 'D':
				use_dynamic = 1;
				break;
			case '?':
				print_help();
				break;
			default:
				print_help();
				//file_result_info = strcat()
		}
	}
}


int main(int argc, char **argv) {

	handle_args(argc, argv);

	int i;
	int result_length = strlen(file_result);

	file_result_info = (char *) malloc_mem(result_length + 6);
	for (i = 0; i < result_length; i++) {
		file_result_info[i] = file_result[i];
	}
	file_result_info[i] = '\0';
	strcat(file_result_info, ".info");

	max_length = 4000;

	if (file_query == NULL) {
		printf("Query file can't be empty.\n");
		exit(1);
	}

	if (file_database == NULL) {
		printf("Database file can't be empty. \n");
		exit(1);
	}

	int max_devices = _Offload_number_of_devices();
	if (mic_number <= 0) {
		printf("MIC number can't be zero or negative.\n");
		exit(1);
	}
	if (mic_number > max_devices) {
		printf("MIC number exceeds the maximum available number.\n");
		exit(1);
	}

	switch (implement_type) {
		case 0:
			cal_on_cpu();
			break;
		case 1:
			cal_on_sse();
			break;
		case 3:
			if (use_dynamic) {
				cal_on_all_dynamic();
			} else {
				cal_on_all();
			}

			break;
		default:
			cal_on_mic();
			break;
	}

	free_mem(file_result_info);

	return 0;
}
