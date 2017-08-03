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

void align_avx(char * ref, avx_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, avx_write_t * results, __m256i * dvdh_bit_mem) {

    int i, j, k;
    int word_size = AVX_WORD_SIZE - 1;
    __m256i * VN;
    __m256i * VP;
    __m256i VN_temp;
    __m256i VP_temp;
    __m256i PM;
    __m256i D0;
    __m256i HP;
    __m256i HN;
    __m256i HP_shift;
    __m256i HN_shift;
    __m256i sum;
    __m256i all_ones = _mm256_set1_epi32(0xffffffff);
    __m256i carry_bitmask = _mm256_set1_epi32(0x7fffffff);
    __m256i maskh;
    __m256i factor;
    __m256i one = _mm256_set1_epi32(1);
    __m256i score;
    __m256i matches;
    __m256i tmp;
    __m256i m1;
    maskh = _mm256_set1_epi32(1 << ((read_len - 1) % word_size));
    factor = _mm256_set1_epi32(-1);
    char * itr;
    avx_read_t * matchv;
    avx_read_t * read_temp = read;

    int tid = omp_get_thread_num();
    int start = tid * word_num * dvdh_len;
    VN = & dvdh_bit_mem[start];
    VP = & dvdh_bit_mem[start + word_num * 1];

    for(k = 0; k < chunk_read_num; k++) {

        read =& read_temp[ k * word_num * AVX_V_NUM * CHAR_NUM];

        for (i = 0; i < word_num; i++) {
            VN[i] = _mm256_set1_epi32(0);
            VP[i] = _mm256_set1_epi32(0x7fffffff);
        }

        score = _mm256_set1_epi32(read_len);

        for(i = 0, itr = ref; i < ref_len; i++, itr++) {

            matchv = & read[((int)*itr) * AVX_V_NUM * word_num];
            HP_shift = _mm256_set1_epi32(1);
            HN_shift = _mm256_set1_epi32(0);
            sum = _mm256_set1_epi32(0);

            for(j = 0; j < word_num-1; j++) {

                matches = _mm256_load_si256( (__m256i *)matchv);
                matchv += AVX_V_NUM;
                VN_temp = VN[j];
                VP_temp = VP[j];
                PM = _mm256_or_si256(matches, VN_temp);
                tmp = _mm256_srli_epi32(sum, word_size);
                sum = _mm256_and_si256(VP_temp, PM);
                sum = _mm256_add_epi32(sum, VP_temp);
                sum = _mm256_add_epi32(sum, tmp);
                D0 = _mm256_and_si256(sum, carry_bitmask);
                D0 = _mm256_xor_si256(D0, VP_temp);
                D0 = _mm256_or_si256(D0, PM);
                HP = _mm256_or_si256(D0, VP_temp);
                HP = _mm256_xor_si256(HP, all_ones);
                HP = _mm256_or_si256(HP, VN_temp);
                HN = _mm256_and_si256(D0, VP_temp);

                HP = _mm256_slli_epi32(HP, 1);
                HP = _mm256_or_si256(HP, HP_shift);
                HP_shift = _mm256_srli_epi32(HP, word_size);
                HN = _mm256_slli_epi32(HN, 1);
                HN = _mm256_or_si256(HN, HN_shift);
                HN_shift = _mm256_srli_epi32(HN, word_size);
                VP[j] = _mm256_or_si256(D0, HP);
                VP[j] = _mm256_xor_si256(VP[j], all_ones);
                VP[j] = _mm256_or_si256(VP[j], HN);
                VP[j] = _mm256_and_si256(VP[j], carry_bitmask);
                VN[j] = _mm256_and_si256(D0, HP);
                VN[j] = _mm256_and_si256(VN[j], carry_bitmask);
            }

            matches = _mm256_load_si256( (__m256i *)matchv);
            VN_temp = VN[word_num - 1];
            VP_temp = VP[word_num - 1];
            PM = _mm256_or_si256(matches, VN_temp);
            tmp = _mm256_srli_epi32(sum, word_size);
            sum = _mm256_and_si256(VP_temp, PM);
            sum = _mm256_add_epi32(sum, VP_temp);
            sum = _mm256_add_epi32(sum, tmp);
            D0 = _mm256_and_si256(sum, carry_bitmask);
            D0 = _mm256_xor_si256(D0, VP_temp);
            D0 = _mm256_or_si256(D0, PM);
            HP = _mm256_or_si256(D0, VP_temp);
            HP = _mm256_xor_si256(HP, all_ones);
            HP = _mm256_or_si256(HP, VN_temp);
            HN = _mm256_and_si256(D0, VP_temp);

            tmp = _mm256_and_si256(HP, maskh);
            m1=_mm256_cmpeq_epi32(tmp, maskh);
            m1 = _mm256_srli_epi32(m1, word_size);
            score= _mm256_add_epi32(score, m1);
            tmp = _mm256_and_si256(HN, maskh);
            m1=_mm256_cmpeq_epi32(tmp, maskh);
            m1 = _mm256_srli_epi32(m1, word_size);
            score= _mm256_sub_epi32(score, m1);
            HP = _mm256_slli_epi32(HP, 1);
            HP = _mm256_or_si256(HP, HP_shift);
            HN = _mm256_slli_epi32(HN, 1);
            HN = _mm256_or_si256(HN, HN_shift);
            VP[word_num - 1] = _mm256_or_si256(D0, HP);
            VP[word_num - 1] = _mm256_xor_si256(VP[word_num - 1], all_ones);
            VP[word_num - 1] = _mm256_or_si256(VP[word_num - 1], HN);
            VP[word_num - 1] = _mm256_and_si256(VP[word_num - 1], carry_bitmask);
            VN[word_num - 1] = _mm256_and_si256(D0, HP);
            VN[word_num - 1] = _mm256_and_si256(VN[word_num - 1], carry_bitmask);
        }

        score = _mm256_mullo_epi32(score, factor);
        int index = result_index * AVX_V_NUM;
        int * vec_dump = ((int *) & score);
        #pragma vector always
        #pragma ivdep
        for(i = 0; i < AVX_V_NUM; i++){
                results[index + i] = vec_dump[i];
        }
        result_index++;
    }
}
