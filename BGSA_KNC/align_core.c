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
__ONMIC__ extern int dvdh_len = 10;

void align_cpu(char * ref, cpu_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, cpu_write_t * results, uint64_t * dvdh_bit_mem) {

    int i, j, k;
    int word_size = CPU_WORD_SIZE - 1;
    uint64_t * VN;
    uint64_t * VP;
    uint64_t VN_temp;
    uint64_t VP_temp;
    uint64_t PM;
    uint64_t D0;
    uint64_t HP;
    uint64_t HN;
    uint64_t HP_shift;
    uint64_t HN_shift;
    uint64_t sum;
    uint64_t all_ones = 0xffffffffffffffff;
    uint64_t carry_bitmask = 0x7fffffffffffffff;
    uint64_t maskh;
    uint64_t factor;
    uint64_t one = 1;
    uint64_t score;
    uint64_t matches;
    uint64_t tmp;
    uint64_t m1;
    maskh = 1L << ((read_len - 1) % word_size);
    factor = -1;
    char * itr;
    cpu_read_t * matchv;
    cpu_read_t * read_temp = read;

    int tid = omp_get_thread_num();
    int start = tid * word_num * dvdh_len;
    VN = & dvdh_bit_mem[start];
    VP = & dvdh_bit_mem[start + word_num * 1];

    for(k = 0; k < chunk_read_num; k++) {

        read =& read_temp[ k * word_num * CPU_V_NUM * CHAR_NUM];

        for (i = 0; i < word_num; i++) {
            VN[i] = 0;
            VP[i] = 0x7fffffffffffffff;
        }

        score = read_len;

        for(i = 0, itr = ref; i < ref_len; i++, itr++) {

            matchv = & read[((int)*itr) * CPU_V_NUM * word_num];
            HP_shift = 1;
            HN_shift = 0;
            sum = 0;

            for(j = 0; j < word_num-1; j++) {

                matches = *matchv;
                matchv += CPU_V_NUM;
                VN_temp = VN[j];
                VP_temp = VP[j];
                PM = matches | VN_temp;
                tmp = sum >> word_size;
                sum = VP_temp & PM;
                sum += VP_temp;
                sum += tmp;
                D0 = sum & carry_bitmask;
                D0 ^= VP_temp;
                D0 |= PM;
                HP = D0 | VP_temp;
                HP = ~HP;
                HP |= VN_temp;
                HN = D0 & VP_temp;

                HP <<= 1;
                HP |= HP_shift;
                HP_shift = HP >> word_size;
                HN <<= 1;
                HN |= HN_shift;
                HN_shift = HN >> word_size;
                VP[j] = D0 | HP;
                VP[j] = ~VP[j];
                VP[j] |= HN;
                VP[j] &= carry_bitmask;
                VN[j] = D0 & HP;
                VN[j] &= carry_bitmask;
            }

            matches = *matchv;
            VN_temp = VN[word_num - 1];
            VP_temp = VP[word_num - 1];
            PM = matches | VN_temp;
            tmp = sum >> word_size;
            sum = VP_temp & PM;
            sum += VP_temp;
            sum += tmp;
            D0 = sum & carry_bitmask;
            D0 ^= VP_temp;
            D0 |= PM;
            HP = D0 | VP_temp;
            HP = ~HP;
            HP |= VN_temp;
            HN = D0 & VP_temp;

            if(HN & maskh)
                score--;
            else if(HP & maskh)
                score++;
            HP <<= 1;
            HP |= HP_shift;
            HN <<= 1;
            HN |= HN_shift;
            VP[word_num - 1] = D0 | HP;
            VP[word_num - 1] = ~VP[word_num - 1];
            VP[word_num - 1] |= HN;
            VP[word_num - 1] &= carry_bitmask;
            VN[word_num - 1] = D0 & HP;
            VN[word_num - 1] &= carry_bitmask;
        }

        score *= factor;
        int index = result_index * CPU_V_NUM;
        int * vec_dump = ((int *) & score);
        #pragma vector always
        #pragma ivdep
        for(i = 0; i < CPU_V_NUM; i++){
                results[index + i] = vec_dump[i];
        }
        result_index++;
    }

}

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

__ONMIC__ void align_mic(char * ref, mic_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, mic_write_t * results, __m512i * dvdh_bit_mem) {

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
    __m512i maskh;
    __m512i factor;
    __m512i one = _mm512_set1_epi32(1);
    __mmask16 carry = 0x0000;
    __mmask16 carry_temp = 0x0000;
    __mmask16 m1;
    __m512i score;
    __m512i matches;
    __m512i tmp;
    maskh = _mm512_set1_epi32(1 << ((read_len - 1) % MIC_WORD_SIZE));
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
            VP[i] = _mm512_set1_epi32(0xffffffff);
        }

        score = _mm512_set1_epi32(read_len);

        for(i = 0, itr = ref; i < ref_len; i++, itr++) {

            matchv = & read[((int)*itr) * MIC_V_NUM * word_num];
            HP_shift = _mm512_set1_epi32(1);
            HN_shift = _mm512_set1_epi32(0);
            sum = _mm512_set1_epi32(0);
            carry = 0x0000;
            carry_temp = 0x0000;

            for(j = 0; j < word_num-1; j++) {

                matches = _mm512_load_epi32(matchv);
                matchv += MIC_V_NUM;
                VN_temp = VN[j];
                VP_temp = VP[j];
                PM = _mm512_or_epi32(matches, VN_temp);
                sum = _mm512_and_epi32(VP_temp, PM);
                #ifdef __MIC__
                sum = _mm512_adc_epi32(sum,carry,VP_temp,&carry_temp);
                #endif
                carry = carry_temp;
                D0 = _mm512_xor_epi32(sum, VP_temp);
                D0 = _mm512_or_epi32(D0, PM);
                HP = _mm512_or_epi32(D0, VP_temp);
                HP = _mm512_xor_epi32(HP, all_ones);
                HP = _mm512_or_epi32(HP, VN_temp);
                HN = _mm512_and_epi32(D0, VP_temp);

                tmp = _mm512_srli_epi32(HP, word_size);
                HP = _mm512_slli_epi32(HP, 1);
                HP = _mm512_or_epi32(HP, HP_shift);
                HP_shift = tmp;
                tmp = _mm512_srli_epi32(HN, word_size);
                HN = _mm512_slli_epi32(HN, 1);
                HN = _mm512_or_epi32(HN, HN_shift);
                HN_shift = tmp;
                VP[j] = _mm512_or_epi32(D0, HP);
                VP[j] = _mm512_xor_epi32(VP[j], all_ones);
                VP[j] = _mm512_or_epi32(VP[j], HN);
                VN[j] = _mm512_and_epi32(D0, HP);
            }

            matches = _mm512_load_epi32(matchv);
            matchv += MIC_V_NUM;
            VN_temp = VN[j];
            VP_temp = VP[j];
            PM = _mm512_or_epi32(matches, VN_temp);
            sum = _mm512_and_epi32(VP_temp, PM);
            #ifdef __MIC__
            sum = _mm512_adc_epi32(sum,carry,VP_temp,&carry_temp);
            #endif
            carry = carry_temp;
            D0 = _mm512_xor_epi32(sum, VP_temp);
            D0 = _mm512_or_epi32(D0, PM);
            HP = _mm512_or_epi32(D0, VP_temp);
            HP = _mm512_xor_epi32(HP, all_ones);
            HP = _mm512_or_epi32(HP, VN_temp);
            HN = _mm512_and_epi32(D0, VP_temp);

            tmp = _mm512_and_epi32(HP, maskh);
            m1=_mm512_cmp_epu32_mask(tmp,maskh ,_MM_CMPINT_EQ);
            score= _mm512_mask_add_epi32(score, m1, score, one);
            tmp = _mm512_and_epi32(HN, maskh);
            m1=_mm512_cmp_epu32_mask(tmp,maskh ,_MM_CMPINT_EQ);
            score= _mm512_mask_sub_epi32(score, m1, score, one);
            tmp = _mm512_srli_epi32(HP, word_size);
            HP = _mm512_slli_epi32(HP, 1);
            HP = _mm512_or_epi32(HP, HP_shift);
            HP_shift = tmp;
            tmp = _mm512_srli_epi32(HN, word_size);
            HN = _mm512_slli_epi32(HN, 1);
            HN = _mm512_or_epi32(HN, HN_shift);
            HN_shift = tmp;
            VP[j] = _mm512_or_epi32(D0, HP);
            VP[j] = _mm512_xor_epi32(VP[j], all_ones);
            VP[j] = _mm512_or_epi32(VP[j], HN);
            VN[j] = _mm512_and_epi32(D0, HP);
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
