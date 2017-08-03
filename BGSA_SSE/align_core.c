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

extern int match_score = 0;
extern int mismatch_score = -1;
extern int gap_score = -1;
extern int dvdh_len = 10;

void align_sse(char * ref, sse_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, sse_write_t * results, __m128i * dvdh_bit_mem) {

    int i, j, k;
    int word_size = SSE_WORD_SIZE - 1;
    __m128i * VN;
    __m128i * VP;
    __m128i VN_temp;
    __m128i VP_temp;
    __m128i PM;
    __m128i D0;
    __m128i HP;
    __m128i HN;
    __m128i HP_shift;
    __m128i HN_shift;
    __m128i sum;
    __m128i all_ones = _mm_set1_epi32(0xffffffff);
    __m128i carry_bitmask = _mm_set1_epi32(0x7fffffff);
    __m128i maskh;
    __m128i factor;
    __m128i one = _mm_set1_epi32(1);
    __m128i score;
    __m128i matches;
    __m128i tmp;
    __m128i m1;
    maskh = _mm_set1_epi32(1 << ((read_len - 1) % word_size));
    factor = _mm_set1_epi32(-1);
    char * itr;
    sse_read_t * matchv;
    sse_read_t * read_temp = read;

    int tid = omp_get_thread_num();
    int start = tid * word_num * dvdh_len;
    VN = & dvdh_bit_mem[start];
    VP = & dvdh_bit_mem[start + word_num * 1];

    for(k = 0; k < chunk_read_num; k++) {

        read =& read_temp[ k * word_num * SSE_V_NUM * CHAR_NUM];

        for (i = 0; i < word_num; i++) {
            VN[i] = _mm_set1_epi32(0);
            VP[i] = _mm_set1_epi32(0x7fffffff);
        }

        score = _mm_set1_epi32(read_len);

        for(i = 0, itr = ref; i < ref_len; i++, itr++) {

            matchv = & read[((int)*itr) * SSE_V_NUM * word_num];
            HP_shift = _mm_set1_epi32(1);
            HN_shift = _mm_set1_epi32(0);
            sum = _mm_set1_epi32(0);

            for(j = 0; j < word_num-1; j++) {

                matches = _mm_load_si128( (__m128i *)matchv);
                matchv += SSE_V_NUM;
                VN_temp = VN[j];
                VP_temp = VP[j];
                PM = _mm_or_si128(matches, VN_temp);
                tmp = _mm_srli_epi32(sum, word_size);
                sum = _mm_and_si128(VP_temp, PM);
                sum = _mm_add_epi32(sum, VP_temp);
                sum = _mm_add_epi32(sum, tmp);
                D0 = _mm_and_si128(sum, carry_bitmask);
                D0 = _mm_xor_si128(D0, VP_temp);
                D0 = _mm_or_si128(D0, PM);
                HP = _mm_or_si128(D0, VP_temp);
                HP = _mm_xor_si128(HP, all_ones);
                HP = _mm_or_si128(HP, VN_temp);
                HN = _mm_and_si128(D0, VP_temp);

                HP = _mm_slli_epi32(HP, 1);
                HP = _mm_or_si128(HP, HP_shift);
                HP_shift = _mm_srli_epi32(HP, word_size);
                HN = _mm_slli_epi32(HN, 1);
                HN = _mm_or_si128(HN, HN_shift);
                HN_shift = _mm_srli_epi32(HN, word_size);
                VP[j] = _mm_or_si128(D0, HP);
                VP[j] = _mm_xor_si128(VP[j], all_ones);
                VP[j] = _mm_or_si128(VP[j], HN);
                VP[j] = _mm_and_si128(VP[j], carry_bitmask);
                VN[j] = _mm_and_si128(D0, HP);
                VN[j] = _mm_and_si128(VN[j], carry_bitmask);
            }

            matches = _mm_load_si128( (__m128i *)matchv);
            VN_temp = VN[word_num - 1];
            VP_temp = VP[word_num - 1];
            PM = _mm_or_si128(matches, VN_temp);
            tmp = _mm_srli_epi32(sum, word_size);
            sum = _mm_and_si128(VP_temp, PM);
            sum = _mm_add_epi32(sum, VP_temp);
            sum = _mm_add_epi32(sum, tmp);
            D0 = _mm_and_si128(sum, carry_bitmask);
            D0 = _mm_xor_si128(D0, VP_temp);
            D0 = _mm_or_si128(D0, PM);
            HP = _mm_or_si128(D0, VP_temp);
            HP = _mm_xor_si128(HP, all_ones);
            HP = _mm_or_si128(HP, VN_temp);
            HN = _mm_and_si128(D0, VP_temp);

            tmp = _mm_and_si128(HP, maskh);
            m1=_mm_cmpeq_epi32(tmp, maskh);
            m1 = _mm_srli_epi32(m1, word_size);
            score= _mm_add_epi32(score, m1);
            tmp = _mm_and_si128(HN, maskh);
            m1=_mm_cmpeq_epi32(tmp, maskh);
            m1 = _mm_srli_epi32(m1, word_size);
            score= _mm_sub_epi32(score, m1);
            HP = _mm_slli_epi32(HP, 1);
            HP = _mm_or_si128(HP, HP_shift);
            HN = _mm_slli_epi32(HN, 1);
            HN = _mm_or_si128(HN, HN_shift);
            VP[word_num - 1] = _mm_or_si128(D0, HP);
            VP[word_num - 1] = _mm_xor_si128(VP[word_num - 1], all_ones);
            VP[word_num - 1] = _mm_or_si128(VP[word_num - 1], HN);
            VP[word_num - 1] = _mm_and_si128(VP[word_num - 1], carry_bitmask);
            VN[word_num - 1] = _mm_and_si128(D0, HP);
            VN[word_num - 1] = _mm_and_si128(VN[word_num - 1], carry_bitmask);
        }

        score = _mm_mullo_epi32(score, factor);
        int index = result_index * SSE_V_NUM;
        int * vec_dump = ((int *) & score);
        #pragma vector always
        #pragma ivdep
        for(i = 0; i < SSE_V_NUM; i++){
                results[index + i] = vec_dump[i];
        }
        result_index++;
    }

}
