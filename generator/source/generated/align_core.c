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
int mismatch_score = 1;
int gap_score = 1;
int dvdh_len = 16;
int full_bits = 0;

#define cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c) \
    X = _mm512_or_epi64(peq[c], VN); \
    D0 = _mm512_and_epi64(X, VP); \
    D0 = _mm512_add_epi64(D0, VP); \
    D0 = _mm512_xor_epi64(D0, VP); \
    D0 = _mm512_or_epi64(D0, X); \
    HN = _mm512_and_epi64(D0, VP); \
    HP = _mm512_or_epi64(D0, VP); \
    HP = _mm512_xor_epi64(HP, all_ones_mask); \
    HP = _mm512_or_epi64(HP, VN); \
    X = _mm512_srli_epi64(D0, 1); \
    VN = _mm512_and_epi64(X, HP); \
    VP = _mm512_or_epi64(HP, X); \
    VP = _mm512_xor_epi64(VP, all_ones_mask); \
    VP = _mm512_or_epi64(VP, HN);

#define move_peq(peq) \
    peq[0] = _mm512_srli_epi64(peq[0], 1); \
    peq[1] = _mm512_srli_epi64(peq[1], 1); \
    peq[2] = _mm512_srli_epi64(peq[2], 1); \
    peq[3] = _mm512_srli_epi64(peq[3], 1); \

#define or_peq(tmp_peq, peq, bit_index, one, band_down) \
    tmp_vector = _mm512_srli_epi64(tmp_peq[0], bit_index); \
    tmp_vector = _mm512_and_epi64(tmp_vector, one); \
    tmp_vector = _mm512_slli_epi64(tmp_vector, band_down); \
    peq[0] = _mm512_or_epi64(peq[0], tmp_vector); \
    tmp_vector = _mm512_srli_epi64(tmp_peq[1], bit_index); \
    tmp_vector = _mm512_and_epi64(tmp_vector, one); \
    tmp_vector = _mm512_slli_epi64(tmp_vector, band_down); \
    peq[1] = _mm512_or_epi64(peq[1], tmp_vector); \
    tmp_vector = _mm512_srli_epi64(tmp_peq[2], bit_index); \
    tmp_vector = _mm512_and_epi64(tmp_vector, one); \
    tmp_vector = _mm512_slli_epi64(tmp_vector, band_down); \
    peq[2] = _mm512_or_epi64(peq[2], tmp_vector); \
    tmp_vector = _mm512_srli_epi64(tmp_peq[3], bit_index); \
    tmp_vector = _mm512_and_epi64(tmp_vector, one); \
    tmp_vector = _mm512_slli_epi64(tmp_vector, band_down); \
    peq[3] = _mm512_or_epi64(peq[3], tmp_vector);

#define cal_score(tmp_vector, D0, one, err) \
    tmp_vector = _mm512_and_epi64(D0, one); \
    tmp_vector = _mm512_sub_epi64(one, tmp_vector); \
    err = _mm512_add_epi64(err, tmp_vector);

void align_mic( char * query, mic_read_t * read, int query_len, int subject_len, int word_num, int chunk_read_num, int result_index, mic_write_t * score, mic_data_t * dvdh_bit_mem) {
    int h_threshold = threshold + subject_len - query_len;
    int band_length = threshold + h_threshold + 1;
    int band_down = band_length - 1;
    int tid = omp_get_thread_num();
    int start = tid * dvdh_len;
    mic_data_t * peq = & dvdh_bit_mem[start];
    mic_data_t * tmp_peq = & dvdh_bit_mem[start + CHAR_NUM];
    mic_read_t * dist;
    int wv_num = word_num * MIC_V_NUM;

    for(int chunk_index = 0; chunk_index < chunk_read_num; chunk_index++) {

        int score_index =  result_index * MIC_V_NUM;
        result_index++;
        dist = & read[chunk_index * word_num * MIC_V_NUM * CHAR_NUM];
        peq[0] = _mm512_load_epi64(dist);
        peq[1] = _mm512_load_epi64((dist + wv_num));
        peq[2] = _mm512_load_epi64((dist + wv_num * 2));
        peq[3] = _mm512_load_epi64((dist + wv_num * 3));
        tmp_peq[0] = _mm512_load_epi64((dist + MIC_V_NUM));
        tmp_peq[1] = _mm512_load_epi64((dist + wv_num + MIC_V_NUM));
        tmp_peq[2] = _mm512_load_epi64((dist + wv_num * 2 + MIC_V_NUM));
        tmp_peq[3] = _mm512_load_epi64((dist + wv_num * 3 + MIC_V_NUM));

        mic_data_t VN = _mm512_set1_epi64(0);
        mic_data_t VP = _mm512_set1_epi64(0);
        mic_data_t X = _mm512_set1_epi64(0);
        mic_data_t D0 = _mm512_set1_epi64(0);
        mic_data_t HN = _mm512_set1_epi64(0);
        mic_data_t HP = _mm512_set1_epi64(0);
        mic_data_t all_ones_mask = _mm512_set1_epi64(0xffffffffffffffff);
        mic_data_t one = _mm512_set1_epi64(0x0000000000000001);
        mic_data_t err_mask = one;
        mic_data_t tmp_vector;

        int i_bd = h_threshold;
        int last_bits = h_threshold;
        int bit_index = 0;
        cmp_result_t cmp_result;
        int query_index = 0;

        mic_data_t err = _mm512_set1_epi64(threshold);
        mic_data_t max_err = _mm512_set1_epi64(threshold + last_bits + 1);

        for(; query_index < threshold; query_index++) {
            int c = query[query_index];
            cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
            move_peq(peq);
            or_peq(tmp_peq, peq, bit_index, one, band_down);
            bit_index++;
            i_bd++;
        }

        int length = MIC_WORD_SIZE < query_len ? MIC_WORD_SIZE : query_len; 
        for(; query_index < length; query_index++) {
            int c = query[query_index];
            cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
            cal_score(tmp_vector, D0, one, err);
            move_peq(peq);
            or_peq(tmp_peq, peq, bit_index, one, band_down);
            bit_index++;
            i_bd++;
        }

        cmp_result = _mm512_cmp_epu64_mask(err, max_err, _MM_CMPINT_GT);
        if(cmp_result == result_mask) {
            for(int i = 0; i < MIC_V_NUM; i++) {
                score[score_index + i] = MAX_ERROR;
            }

            goto end;
        }

        if(query_len > MIC_WORD_SIZE) {
            bit_index = 0;
            int rest_length = subject_len - i_bd;
            int batch_count = rest_length / batch_size;
            int word_count = rest_length / MIC_WORD_SIZE;
            int word_batch_count = MIC_WORD_SIZE / batch_size;
            int batch_index = 0;
            int word_index = 2;

            tmp_peq[0] = _mm512_load_epi64((dist + word_index * MIC_V_NUM));
            tmp_peq[1] = _mm512_load_epi64((dist + wv_num + word_index * MIC_V_NUM));
            tmp_peq[2] = _mm512_load_epi64((dist + wv_num * 2 + word_index *  MIC_V_NUM));
            tmp_peq[3] = _mm512_load_epi64((dist + wv_num * 3 + word_index * MIC_V_NUM));

            for(int i = 0; i < word_count; i++) {
                for(int j = 0; j < word_batch_count; j++) {
                    for(int k = 0; k < batch_size; k+=1) {
                        int c = query[query_index];
                        cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                        cal_score(tmp_vector, D0, one, err);
                        move_peq(peq);
                        or_peq(tmp_peq, peq, bit_index, one, band_down);
                        bit_index++;
                        i_bd++; 
                        query_index++;
                    }

                    cmp_result = _mm512_cmp_epu64_mask(err, max_err, _MM_CMPINT_GT);
                    if(cmp_result == result_mask) {
                        for(int i = 0; i < MIC_V_NUM; i++) {
                            score[score_index + i] = MAX_ERROR;
                        }

                        goto end;
                    }
                    batch_index++;
                }
                bit_index = 0;
                word_index++;

                tmp_peq[0] = _mm512_load_epi64((dist + word_index * MIC_V_NUM));
                tmp_peq[1] = _mm512_load_epi64((dist + wv_num + word_index * MIC_V_NUM));
                tmp_peq[2] = _mm512_load_epi64((dist + wv_num * 2 + word_index *  MIC_V_NUM));
                tmp_peq[3] = _mm512_load_epi64((dist + wv_num * 3 + word_index * MIC_V_NUM));
            }

            for(; batch_index < batch_count; batch_index++) {
                for(int k = 0; k < batch_size; k+=1) {
                    int c = query[query_index];
                    cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                    cal_score(tmp_vector, D0, one, err);
                    move_peq(peq);
                    or_peq(tmp_peq, peq, bit_index, one, band_down);
                    bit_index++;
                    i_bd++;
                    query_index++;
                }

                cmp_result = _mm512_cmp_epu64_mask(err, max_err, _MM_CMPINT_GT);
                if(cmp_result == result_mask) {
                    for(int i = 0; i < MIC_V_NUM; i++) {
                        score[score_index + i] = MAX_ERROR;
                    }

                    goto end;
                }
            }

            for(; i_bd < subject_len; i_bd++) {
                int c = query[query_index];
                cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                cal_score(tmp_vector, D0, one, err);
                move_peq(peq);
                or_peq(tmp_peq, peq, bit_index, one, band_down);
                bit_index++;
                query_index++;
            }

            cmp_result = _mm512_cmp_epu64_mask(err, max_err, _MM_CMPINT_GT);
            if(cmp_result == result_mask) {
                for(int i = 0; i < MIC_V_NUM; i++) {
                    score[score_index + i] = MAX_ERROR;
                }

                goto end;
            }
            for(; query_index < query_len; query_index++) {
                int c = query[query_index];
                cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                cal_score(tmp_vector, D0, one, err);
                move_peq(peq);
            }

        }

        {
            mic_data_t min_err = err;
            for(int i = 0; i <= last_bits; i++) {
                tmp_vector = _mm512_srli_epi64(VP, i);
                tmp_vector = _mm512_and_epi64(tmp_vector, one);
                err = _mm512_add_epi64(err, tmp_vector);
                tmp_vector = _mm512_srli_epi64(VN, i);
                tmp_vector = _mm512_and_epi64(tmp_vector, one);
                err = _mm512_sub_epi64(err, tmp_vector);
                min_err = _mm512_min_epi64(min_err, err);
            }

            mic_result_t * vec = ((mic_result_t *) & min_err);
            for(int i = 0; i < MIC_V_NUM; i++) {
                score[i + score_index] = vec[i];
            }

        }

        end:;
    }

}

