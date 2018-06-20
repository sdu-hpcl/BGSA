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

#define sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c) \
    X = _mm_or_si128(peq[c], VN); \
    D0 = _mm_and_si128(X, VP); \
    D0 = _mm_add_epi64(D0, VP); \
    D0 = _mm_xor_si128(D0, VP); \
    D0 = _mm_or_si128(D0, X); \
    HN = _mm_and_si128(D0, VP); \
    HP = _mm_or_si128(D0, VP); \
    HP = _mm_xor_si128(HP, all_ones_mask); \
    HP = _mm_or_si128(HP, VN); \
    X = _mm_srli_epi64(D0, 1); \
    VN = _mm_and_si128(X, HP); \
    VP = _mm_or_si128(HP, X); \
    VP = _mm_xor_si128(VP, all_ones_mask); \
    VP = _mm_or_si128(VP, HN);

#define sse_move_peq(peq) \
    peq[0] = _mm_srli_epi64(peq[0], 1); \
    peq[1] = _mm_srli_epi64(peq[1], 1); \
    peq[2] = _mm_srli_epi64(peq[2], 1); \
    peq[3] = _mm_srli_epi64(peq[3], 1); \

#define sse_or_peq(tmp_peq, peq, bit_index, one, band_down) \
    tmp_vector = _mm_srli_epi64(tmp_peq[0], bit_index); \
    tmp_vector = _mm_and_si128(tmp_vector, one); \
    tmp_vector = _mm_slli_epi64(tmp_vector, band_down); \
    peq[0] = _mm_or_si128(peq[0], tmp_vector); \
    tmp_vector = _mm_srli_epi64(tmp_peq[1], bit_index); \
    tmp_vector = _mm_and_si128(tmp_vector, one); \
    tmp_vector = _mm_slli_epi64(tmp_vector, band_down); \
    peq[1] = _mm_or_si128(peq[1], tmp_vector); \
    tmp_vector = _mm_srli_epi64(tmp_peq[2], bit_index); \
    tmp_vector = _mm_and_si128(tmp_vector, one); \
    tmp_vector = _mm_slli_epi64(tmp_vector, band_down); \
    peq[2] = _mm_or_si128(peq[2], tmp_vector); \
    tmp_vector = _mm_srli_epi64(tmp_peq[3], bit_index); \
    tmp_vector = _mm_and_si128(tmp_vector, one); \
    tmp_vector = _mm_slli_epi64(tmp_vector, band_down); \
    peq[3] = _mm_or_si128(peq[3], tmp_vector);

#define sse_cal_score(tmp_vector, D0, one, err) \
    tmp_vector = _mm_and_si128(D0, one); \
    tmp_vector = _mm_sub_epi64(one, tmp_vector); \
    err = _mm_add_epi64(err, tmp_vector);

void align_sse( char * query, sse_read_t * read, int query_len, int subject_len, int word_num, int chunk_read_num, int result_index, sse_write_t * score, sse_data_t * dvdh_bit_mem) {
    int h_threshold = threshold + subject_len - query_len;
    int band_length = threshold + h_threshold + 1;
    int band_down = band_length - 1;
    int tid = omp_get_thread_num();
    int start = tid * dvdh_len;
    sse_data_t * peq = & dvdh_bit_mem[start];
    sse_data_t * tmp_peq = & dvdh_bit_mem[start + CHAR_NUM];
    sse_read_t * dist;
    int wv_num = word_num * SSE_V_NUM;

    for(int chunk_index = 0; chunk_index < chunk_read_num; chunk_index++) {

        int score_index =  result_index * SSE_V_NUM;
        result_index++;
        dist = & read[chunk_index * word_num * SSE_V_NUM * CHAR_NUM];
        peq[0] = _mm_load_si128( (__m128i *)dist);
        peq[1] = _mm_load_si128( (__m128i *)(dist + wv_num));
        peq[2] = _mm_load_si128( (__m128i *)(dist + wv_num * 2));
        peq[3] = _mm_load_si128( (__m128i *)(dist + wv_num * 3));
        tmp_peq[0] = _mm_load_si128( (__m128i *)(dist + SSE_V_NUM));
        tmp_peq[1] = _mm_load_si128( (__m128i *)(dist + wv_num + SSE_V_NUM));
        tmp_peq[2] = _mm_load_si128( (__m128i *)(dist + wv_num * 2 + SSE_V_NUM));
        tmp_peq[3] = _mm_load_si128( (__m128i *)(dist + wv_num * 3 + SSE_V_NUM));

        sse_data_t VN = _mm_set1_epi64x(0);
        sse_data_t VP = _mm_set1_epi64x(0);
        sse_data_t X = _mm_set1_epi64x(0);
        sse_data_t D0 = _mm_set1_epi64x(0);
        sse_data_t HN = _mm_set1_epi64x(0);
        sse_data_t HP = _mm_set1_epi64x(0);
        sse_data_t all_ones_mask = _mm_set1_epi64x(0xffffffffffffffff);
        sse_data_t one = _mm_set1_epi64x(0x0000000000000001);
        sse_data_t err_mask = one;
        sse_data_t tmp_vector;

        int i_bd = h_threshold;
        int last_bits = h_threshold;
        int bit_index = 0;
        sse_cmp_result_t cmp_result;
        int query_index = 0;

        sse_data_t err = _mm_set1_epi64x(threshold);
        sse_data_t max_err = _mm_set1_epi64x(threshold + last_bits + 1);

        for(; query_index < threshold; query_index++) {
            int c = query[query_index];
            sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
            sse_move_peq(peq);
            sse_or_peq(tmp_peq, peq, bit_index, one, band_down);
            bit_index++;
            i_bd++;
        }

        int length = SSE_WORD_SIZE < query_len ? SSE_WORD_SIZE : query_len; 
        for(; query_index < length; query_index++) {
            int c = query[query_index];
            sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
            sse_cal_score(tmp_vector, D0, one, err);
            sse_move_peq(peq);
            sse_or_peq(tmp_peq, peq, bit_index, one, band_down);
            bit_index++;
            i_bd++;
        }

        tmp_vector= _mm_cmpgt_epi64(err, max_err);
        cmp_result = _mm_movemask_epi8(tmp_vector);
        if(cmp_result == sse_result_mask) {
            for(int i = 0; i < SSE_V_NUM; i++) {
                score[score_index + i] = MAX_ERROR;
            }

            goto end;
        }

        if(query_len > SSE_WORD_SIZE) {
            bit_index = 0;
            int rest_length = subject_len - i_bd;
            int batch_count = rest_length / batch_size;
            int word_count = rest_length / SSE_WORD_SIZE;
            int word_batch_count = SSE_WORD_SIZE / batch_size;
            int batch_index = 0;
            int word_index = 2;

            tmp_peq[0] = _mm_load_si128( (__m128i *)(dist + word_index * SSE_V_NUM));
            tmp_peq[1] = _mm_load_si128( (__m128i *)(dist + wv_num + word_index * SSE_V_NUM));
            tmp_peq[2] = _mm_load_si128( (__m128i *)(dist + wv_num * 2 + word_index *  SSE_V_NUM));
            tmp_peq[3] = _mm_load_si128( (__m128i *)(dist + wv_num * 3 + word_index * SSE_V_NUM));

            for(int i = 0; i < word_count; i++) {
                for(int j = 0; j < word_batch_count; j++) {
                    for(int k = 0; k < batch_size; k+=1) {
                        int c = query[query_index];
                        sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                        sse_cal_score(tmp_vector, D0, one, err);
                        sse_move_peq(peq);
                        sse_or_peq(tmp_peq, peq, bit_index, one, band_down);
                        bit_index++;
                        i_bd++; 
                        query_index++;
                    }

                    tmp_vector= _mm_cmpgt_epi64(err, max_err);
                    cmp_result = _mm_movemask_epi8(tmp_vector);
                    if(cmp_result == sse_result_mask) {
                        for(int i = 0; i < SSE_V_NUM; i++) {
                            score[score_index + i] = MAX_ERROR;
                        }

                        goto end;
                    }
                    batch_index++;
                }
                bit_index = 0;
                word_index++;

                tmp_peq[0] = _mm_load_si128( (__m128i *)(dist + word_index * SSE_V_NUM));
                tmp_peq[1] = _mm_load_si128( (__m128i *)(dist + wv_num + word_index * SSE_V_NUM));
                tmp_peq[2] = _mm_load_si128( (__m128i *)(dist + wv_num * 2 + word_index *  SSE_V_NUM));
                tmp_peq[3] = _mm_load_si128( (__m128i *)(dist + wv_num * 3 + word_index * SSE_V_NUM));
            }

            for(; batch_index < batch_count; batch_index++) {
                for(int k = 0; k < batch_size; k+=1) {
                    int c = query[query_index];
                    sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                    sse_cal_score(tmp_vector, D0, one, err);
                    sse_move_peq(peq);
                    sse_or_peq(tmp_peq, peq, bit_index, one, band_down);
                    bit_index++;
                    i_bd++;
                    query_index++;
                }

                tmp_vector= _mm_cmpgt_epi64(err, max_err);
                cmp_result = _mm_movemask_epi8(tmp_vector);
                if(cmp_result == sse_result_mask) {
                    for(int i = 0; i < SSE_V_NUM; i++) {
                        score[score_index + i] = MAX_ERROR;
                    }

                    goto end;
                }
            }

            for(; i_bd < subject_len; i_bd++) {
                int c = query[query_index];
                sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                sse_cal_score(tmp_vector, D0, one, err);
                sse_move_peq(peq);
                sse_or_peq(tmp_peq, peq, bit_index, one, band_down);
                bit_index++;
                query_index++;
            }

            tmp_vector= _mm_cmpgt_epi64(err, max_err);
            cmp_result = _mm_movemask_epi8(tmp_vector);
            if(cmp_result == sse_result_mask) {
                for(int i = 0; i < SSE_V_NUM; i++) {
                    score[score_index + i] = MAX_ERROR;
                }

                goto end;
            }
            for(; query_index < query_len; query_index++) {
                int c = query[query_index];
                sse_cal_D0(X, D0, VP, VN, HP, HN, all_ones_mask, peq, c);
                sse_cal_score(tmp_vector, D0, one, err);
                sse_move_peq(peq);
            }

        }

        {
            sse_data_t min_err = err;
            for(int i = 0; i <= last_bits; i++) {
                tmp_vector = _mm_srli_epi64(VP, i);
                tmp_vector = _mm_and_si128(tmp_vector, one);
                err = _mm_add_epi64(err, tmp_vector);
                tmp_vector = _mm_srli_epi64(VN, i);
                tmp_vector = _mm_and_si128(tmp_vector, one);
                err = _mm_sub_epi64(err, tmp_vector);
                min_err= _mm_min_epi32(min_err, err);
            }

            sse_result_t * vec = ((sse_result_t *) & min_err);
            for(int i = 0; i < SSE_V_NUM; i++) {
                score[i + score_index] = vec[i];
            }

        }

        end:;
    }

}

