#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cal.h"
#include "align_core.h"

int match_score = 0;
int mismatch_score = -1;
int gap_score = -1;
int dvdh_len = 16;
int full_bits = 0;

#define cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c) \
    X = peq[c] | VN; \
    D0 = X & VP; \
    D0 = D0 + VP; \
    D0 = D0 ^ VP; \
    D0 = D0 | X; \
    HN = D0 & VP; \
    HP = D0 | VP; \
    HP = HP ^ all_ones_mask; \
    HP = HP | VN; \
    X = D0 >> 1; \
    VN = X & HP; \
    VP = HP | X; \
    VP = VP ^ all_ones_mask; \
    VP = VP | HN;

#define cpu_move_peq(peq) \
    peq[0] = peq[0] >> 1; \
    peq[1] = peq[1] >> 1; \
    peq[2] = peq[2] >> 1; \
    peq[3] = peq[3] >> 1; \
    peq[4] = peq[4] >> 1;

#define cpu_or_peq(tmp_peq, peq, bit_index, one, band_down) \
    tmp_vector = tmp_peq[0] >> bit_index; \
    tmp_vector = tmp_vector & one; \
    tmp_vector = tmp_vector << band_down; \
    peq[0] = peq[0] | tmp_vector; \
    tmp_vector = tmp_peq[1] >> bit_index; \
    tmp_vector = tmp_vector & one; \
    tmp_vector = tmp_vector << band_down; \
    peq[1] = peq[1] | tmp_vector; \
    tmp_vector = tmp_peq[2] >> bit_index; \
    tmp_vector = tmp_vector & one; \
    tmp_vector = tmp_vector << band_down; \
    peq[2] = peq[2] | tmp_vector; \
    tmp_vector = tmp_peq[3] >> bit_index; \
    tmp_vector = tmp_vector & one; \
    tmp_vector = tmp_vector << band_down; \
    peq[3] = peq[3] | tmp_vector; \
    tmp_vector = tmp_peq[4] >> bit_index; \
    tmp_vector = tmp_vector & one; \
    tmp_vector = tmp_vector << band_down; \
    peq[4] = peq[4] | tmp_vector;

#define cpu_cal_score(tmp_vector, D0, one, err) \
    tmp_vector = D0 & one; \
    tmp_vector = one - tmp_vector; \
    err += tmp_vector;

void align_cpu( char * query, cpu_read_t * read, int query_len, int subject_len, int word_num, int chunk_read_num, int result_index, cpu_write_t * score, cpu_data_t * dvdh_bit_mem) {
    int h_threshold = threshold + subject_len - query_len;
    int band_length = threshold + h_threshold + 1;
    int band_down = band_length - 1;
    int tid = omp_get_thread_num();
    int start = tid * dvdh_len;
    cpu_data_t * peq = & dvdh_bit_mem[start];
    cpu_data_t * tmp_peq = & dvdh_bit_mem[start + CHAR_NUM];
    cpu_read_t * dist;
    int wv_num = word_num * CPU_V_NUM;

    for(int chunk_index = 0; chunk_index < chunk_read_num; chunk_index++) {

        int score_index =  result_index * CPU_V_NUM;
        result_index++;
        dist = & read[chunk_index * word_num * CPU_V_NUM * CHAR_NUM];
        peq[0] = *dist;
        peq[1] = *(dist + wv_num);
        peq[2] = *(dist + wv_num * 2);
        peq[3] = *(dist + wv_num * 3);
        peq[4] = *(dist + wv_num * 4);
        tmp_peq[0] = *(dist + CPU_V_NUM);
        tmp_peq[1] = *(dist + wv_num + CPU_V_NUM);
        tmp_peq[2] = *(dist + wv_num * 2 + CPU_V_NUM);
        tmp_peq[3] = *(dist + wv_num * 3 + CPU_V_NUM);
        tmp_peq[4] = *(dist + wv_num * 4 + CPU_V_NUM);

        cpu_data_t VN = 0;
        cpu_data_t VP = 0;
        cpu_data_t X = 0;
        cpu_data_t D0 = 0;
        cpu_data_t HN = 0;
        cpu_data_t HP = 0;
        cpu_data_t all_ones_mask = 0xffffffffffffffff;
        cpu_data_t one = 1L;
        cpu_data_t err_mask = one;
        cpu_data_t tmp_vector;

        int i_bd = h_threshold;
        int last_bits = h_threshold;
        int bit_index = 0;
        cpu_cmp_result_t cmp_result;
        int query_index = 0;

        cpu_data_t err = threshold;
        cpu_data_t max_err = threshold + last_bits + 1;

        for(; query_index < threshold; query_index++) {
            int c = query[query_index];
            cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
            cpu_move_peq(peq);
            cpu_or_peq(tmp_peq, peq, bit_index, one, band_down);
            bit_index++;
            i_bd++;
        }

        int length = CPU_WORD_SIZE < query_len ? CPU_WORD_SIZE : query_len; 
        for(; query_index < length; query_index++) {
            int c = query[query_index];
            cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
            cpu_cal_score(tmp_vector, D0, one, err);
            cpu_move_peq(peq);
            cpu_or_peq(tmp_peq, peq, bit_index, one, band_down);
            bit_index++;
            i_bd++;
        }

        if(err > max_err) {
            score[score_index] = MAX_ERROR;

            goto end;
        }

        if(query_len > CPU_WORD_SIZE) {
            bit_index = 0;
            int rest_length = subject_len - i_bd;
            int batch_count = rest_length / batch_size;
            int word_count = rest_length / CPU_WORD_SIZE;
            int word_batch_count = CPU_WORD_SIZE / batch_size;
            int batch_index = 0;
            int word_index = 2;

            tmp_peq[0] = *(dist + word_index * CPU_V_NUM);
            tmp_peq[1] = *(dist + wv_num + word_index * CPU_V_NUM);
            tmp_peq[2] = *(dist + wv_num * 2 + word_index *  CPU_V_NUM);
            tmp_peq[3] = *(dist + wv_num * 3 + word_index * CPU_V_NUM);
            tmp_peq[4] = *(dist + wv_num * 4 + word_index * CPU_V_NUM);

            for(int i = 0; i < word_count; i++) {
                for(int j = 0; j < word_batch_count; j++) {
                    for(int k = 0; k < batch_size; k+=1) {
                        int c = query[query_index];
                        cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                        cpu_cal_score(tmp_vector, D0, one, err);
                        cpu_move_peq(peq);
                        cpu_or_peq(tmp_peq, peq, bit_index, one, band_down);
                        bit_index++;
                        i_bd++; 
                        query_index++;
                    }

                    if(err > max_err) {
                        score[score_index] = MAX_ERROR;

                        goto end;
                    }
                    batch_index++;
                }
                bit_index = 0;
                word_index++;

                tmp_peq[0] = *(dist + word_index * CPU_V_NUM);
                tmp_peq[1] = *(dist + wv_num + word_index * CPU_V_NUM);
                tmp_peq[2] = *(dist + wv_num * 2 + word_index *  CPU_V_NUM);
                tmp_peq[3] = *(dist + wv_num * 3 + word_index * CPU_V_NUM);
                tmp_peq[4] = *(dist + wv_num * 4 + word_index * CPU_V_NUM);
            }

            for(; batch_index < batch_count; batch_index++) {
                for(int k = 0; k < batch_size; k+=1) {
                    int c = query[query_index];
                    cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                    cpu_cal_score(tmp_vector, D0, one, err);
                    cpu_move_peq(peq);
                    cpu_or_peq(tmp_peq, peq, bit_index, one, band_down);
                    bit_index++;
                    i_bd++;
                    query_index++;
                }

                if(err > max_err) {
                    score[score_index] = MAX_ERROR;

                    goto end;
                }
            }

            for(; i_bd < subject_len; i_bd++) {
                int c = query[query_index];
                cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                cpu_cal_score(tmp_vector, D0, one, err);
                cpu_move_peq(peq);
                cpu_or_peq(tmp_peq, peq, bit_index, one, band_down);
                bit_index++;
                query_index++;
            }

            if(err > max_err) {
                score[score_index] = MAX_ERROR;

                goto end;
            }
            for(; query_index < query_len; query_index++) {
                int c = query[query_index];
                cpu_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                cpu_cal_score(tmp_vector, D0, one, err);
                cpu_move_peq(peq);
            }

        }

        {
            cpu_data_t min_err = err;
            for(int i = 0; i <= last_bits; i++) {
                tmp_vector = VP >> i;
                tmp_vector &= one;
                err += tmp_vector;
                tmp_vector = VN >> i;
                tmp_vector &= one;
                err -= tmp_vector;
                min_err = min_err < err ? min_err : err;
            }

            cpu_result_t * vec = ((cpu_result_t *) & min_err);
            for(int i = 0; i < CPU_V_NUM; i++) {
                score[i + score_index] = vec[i];
            }

        }

        end:;
    }

}

