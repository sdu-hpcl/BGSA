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

void align_mic(char * ref, mic_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, mic_write_t * results, __m512i * dvdh_bit_mem) {

    int i, j, k;
    int word_size = MIC_WORD_SIZE - 1;
    __m512i * VN;
    __m512i * VP;
    __m512i VN_temp;
    __m512i VP_temp;
    __m512i PM;
    __m512i D0;
    __m512i HP;
    __m512i HN;
    __m512i HP_shift;
    __m512i HN_shift;
    __m512i sum;
    __m512i all_ones = _mm512_set1_epi32(0xffffffff);
    __m512i carry_bitmask = _mm512_set1_epi32(0x7fffffff);
    __m512i maskh;
    __m512i factor;
    __m512i one = _mm512_set1_epi32(1);
    __m512i score;
    __m512i matches;
    __m512i tmp;
    __mmask16 m1;
    maskh = _mm512_set1_epi32(1 << ((read_len - 1) % word_size));
    factor = _mm512_set1_epi32(-1);
    char * itr;
    mic_read_t * matchv;
    mic_read_t * read_temp = read;

    int tid = omp_get_thread_num();
    int start = tid * word_num * dvdh_len;
    VN = & dvdh_bit_mem[start];
    VP = & dvdh_bit_mem[start + word_num * 1];

    for(k = 0; k < chunk_read_num; k++) {

        read =& read_temp[ k * word_num * MIC_V_NUM * CHAR_NUM];

        for (i = 0; i < word_num; i++) {
            VN[i] = _mm512_set1_epi32(0);
            VP[i] = _mm512_set1_epi32(0x7fffffff);
        }

        score = _mm512_set1_epi32(read_len);

        for(i = 0, itr = ref; i < ref_len; i++, itr++) {

            matchv = & read[((int)*itr) * MIC_V_NUM * word_num];
            HP_shift = _mm512_set1_epi32(1);
            HN_shift = _mm512_set1_epi32(0);
            sum = _mm512_set1_epi32(0);

            for(j = 0; j < word_num-1; j++) {

                matches = _mm512_load_epi32(matchv);
                matchv += MIC_V_NUM;
                VN_temp = VN[j];
                VP_temp = VP[j];
                PM = _mm512_or_epi32(matches, VN_temp);
                tmp = _mm512_srli_epi32(sum, word_size);
                sum = _mm512_and_epi32(VP_temp, PM);
                sum = _mm512_add_epi32(sum, VP_temp);
                sum = _mm512_add_epi32(sum, tmp);
                D0 = _mm512_and_epi32(sum, carry_bitmask);
                D0 = _mm512_xor_epi32(D0, VP_temp);
                D0 = _mm512_or_epi32(D0, PM);
                HP = _mm512_or_epi32(D0, VP_temp);
                HP = _mm512_andnot_epi32(HP, all_ones);
                HP = _mm512_or_epi32(HP, VN_temp);
                HN = _mm512_and_epi32(D0, VP_temp);

                HP = _mm512_slli_epi32(HP, 1);
                HP = _mm512_or_epi32(HP, HP_shift);
                HP_shift = _mm512_srli_epi32(HP, word_size);
                HN = _mm512_slli_epi32(HN, 1);
                HN = _mm512_or_epi32(HN, HN_shift);
                HN_shift = _mm512_srli_epi32(HN, word_size);
                VP[j] = _mm512_or_epi32(D0, HP);
                VP[j] = _mm512_andnot_epi32(VP[j], all_ones);
                VP[j] = _mm512_or_epi32(VP[j], HN);
                VP[j] = _mm512_and_epi32(VP[j], carry_bitmask);
                VN[j] = _mm512_and_epi32(D0, HP);
                VN[j] = _mm512_and_epi32(VN[j], carry_bitmask);
            }

            matches = _mm512_load_epi32(matchv);
            VN_temp = VN[word_num - 1];
            VP_temp = VP[word_num - 1];
            PM = _mm512_or_epi32(matches, VN_temp);
            tmp = _mm512_srli_epi32(sum, word_size);
            sum = _mm512_and_epi32(VP_temp, PM);
            sum = _mm512_add_epi32(sum, VP_temp);
            sum = _mm512_add_epi32(sum, tmp);
            D0 = _mm512_and_epi32(sum, carry_bitmask);
            D0 = _mm512_xor_epi32(D0, VP_temp);
            D0 = _mm512_or_epi32(D0, PM);
            HP = _mm512_or_epi32(D0, VP_temp);
            HP = _mm512_andnot_epi32(HP, all_ones);
            HP = _mm512_or_epi32(HP, VN_temp);
            HN = _mm512_and_epi32(D0, VP_temp);

            tmp = _mm512_and_epi32(HP, maskh);
            m1=_mm512_cmp_epu32_mask(tmp,maskh ,_MM_CMPINT_EQ);
            score= _mm512_mask_add_epi32(score, m1, score, one);
            tmp = _mm512_and_epi32(HN, maskh);
            m1=_mm512_cmp_epu32_mask(tmp,maskh ,_MM_CMPINT_EQ);
            score= _mm512_mask_sub_epi32(score, m1, score, one);
            HP = _mm512_slli_epi32(HP, 1);
            HP = _mm512_or_epi32(HP, HP_shift);
            HN = _mm512_slli_epi32(HN, 1);
            HN = _mm512_or_epi32(HN, HN_shift);
            VP[word_num - 1] = _mm512_or_epi32(D0, HP);
            VP[word_num - 1] = _mm512_andnot_epi32(VP[word_num - 1], all_ones);
            VP[word_num - 1] = _mm512_or_epi32(VP[word_num - 1], HN);
            VP[word_num - 1] = _mm512_and_epi32(VP[word_num - 1], carry_bitmask);
            VN[word_num - 1] = _mm512_and_epi32(D0, HP);
            VN[word_num - 1] = _mm512_and_epi32(VN[word_num - 1], carry_bitmask);
        }

        score = _mm512_mullo_epi32(score, factor);
        int index = result_index * MIC_V_NUM;
        int * vec_dump = ((int *) & score);
        #pragma vector always
        #pragma ivdep
        for(i = 0; i < MIC_V_NUM; i++){
                results[index + i] = vec_dump[i];
        }
        result_index++;
    }

}
