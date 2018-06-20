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

int match_score = 2;
int mismatch_score = -3;
int gap_score = -5;
int dvdh_len = 20;
int full_bits = 0;

void align_mic(char * ref, mic_read_t * read, int ref_len, int read_len, int word_num, int chunk_read_num, int result_index, mic_write_t * results, mic_data_t * dvdh_bit_mem) {
    int word_sizem1 = MIC_WORD_SIZE - 1;
    int word_sizem2 = MIC_WORD_SIZE - 2;
    int i, j, k, m, n;
    mic_read_t bitmask;
    mic_data_t matches;
    mic_data_t not_matches;
    mic_data_t all_ones = _mm512_set1_epi32(0xffffffff);
    mic_read_t one = 0x00000001;
    mic_data_t carry_bitmask = _mm512_set1_epi32(0x7fffffff);
    mic_data_t dvpos7shift;
    mic_data_t dvpos6shift;
    mic_data_t dvpos5shift;
    mic_data_t dvpos4shift;
    mic_data_t dvpos3shift;
    mic_data_t dvpos5shift_dvpos6shift;
    mic_data_t dvpos5shift_dvpos6shift_dvnot7to3ormatch;
    mic_data_t dvnot7to3ormatch_dvpos7shiftormatch;
    mic_data_t dvpos3shift_dvpos4shift;
    mic_data_t dvpos5shift_dvpos6shift_dvpos3shift_dvpos4shift;
    mic_data_t dvpos5shift_dvpos6shift_dvpos3shift_dvpos4shift_dvpos7shiftormatch;
    mic_data_t dh_pos7;
    mic_data_t dh_pos6;
    mic_data_t dh_pos5;
    mic_data_t dh_pos4;
    mic_data_t dh_pos3;
    mic_data_t dh_pos2;
    mic_data_t dh_pos1;
    mic_data_t dh_zero;
    mic_data_t dh_neg1;
    mic_data_t dh_neg2;
    mic_data_t dh_neg3;
    mic_data_t dh_neg4;
    mic_data_t dh_neg5;
    mic_data_t init_pos7;
    mic_data_t init_pos6;
    mic_data_t init_pos5;
    mic_data_t init_pos4;
    mic_data_t init_pos3;
    mic_data_t init_pos7_prevbit;
    mic_data_t init_pos6_prevbit;
    mic_data_t init_pos5_prevbit;
    mic_data_t init_pos4_prevbit;
    mic_data_t init_pos3_prevbit;
    mic_data_t init_pos2_prevbit;
    mic_data_t init_pos1_prevbit;
    mic_data_t init_zero_prevbit;
    mic_data_t init_neg1_prevbit;
    mic_data_t init_neg2_prevbit;
    mic_data_t init_neg3_prevbit;
    mic_data_t init_neg4_prevbit;
    mic_data_t overflow0;
    mic_data_t overflow1;
    mic_data_t overflow2;
    mic_data_t overflow3;
    mic_data_t overflow4;
    mic_data_t overflow5;
    mic_data_t overflow6;
    mic_data_t overflow7;
    mic_data_t overflow8;
    mic_data_t overflow9;
    mic_data_t overflow10;
    mic_data_t overflow11;
    mic_data_t dv_pos6shift_not_match;
    mic_data_t dv_pos5shift_not_match;
    mic_data_t dv_pos4shift_not_match;
    mic_data_t dv_pos3shift_not_match;
    mic_data_t dhbit1;
    mic_data_t dhbit2;
    mic_data_t dhbit4;
    mic_data_t dhbit8;
    mic_data_t dhbit16;
    mic_data_t dhbit1inverse;
    mic_data_t dhbit2inverse;
    mic_data_t dhbit4inverse;
    mic_data_t dhbit8inverse;
    mic_data_t dhbit16inverse;
    mic_data_t dhbit16inverse_dhbit8inverse;
    mic_data_t dhbit16inverse_dhbit8inverse_dhbit4inverse;
    mic_data_t dhbit16inverse_dhbit8inverse_dhbit4inverse_dhbit2inverse;
    mic_data_t dhbit16_dhbit8;
    mic_data_t dhbit16_dhbit8_dhbit4;
    mic_data_t dhbit16_dhbit8_dhbit4_dhbit2;
    mic_data_t dhbit16_dhbit8_dhbit4_dhbit2inverse;
    mic_data_t dhbit16_dhbit8_dhbit4inverse;
    mic_data_t dhbit16_dhbit8_dhbit4inverse_dhbit2;
    mic_data_t dhbit16_dhbit8_dhbit4inverse_dhbit2inverse;
    mic_data_t dv_bit1;
    mic_data_t dv_bit2;
    mic_data_t dv_bit4;
    mic_data_t dv_bit8;
    mic_data_t dv_bit16;
    mic_data_t dh_xor_dv_bit2;
    mic_data_t dh_xor_dv_bit4;
    mic_data_t dh_xor_dv_bit8;
    mic_data_t dh_xor_dv_bit16;
    mic_data_t * dvdh_bit1;
    mic_data_t * dvdh_bit2;
    mic_data_t * dvdh_bit4;
    mic_data_t * dvdh_bit8;
    mic_data_t * dvdh_bit16;
    mic_data_t bit1;
    mic_data_t bit2;
    mic_data_t bit4;
    mic_data_t bit8;
    mic_data_t bit16;
    mic_data_t tmpbit1;
    mic_data_t tmpbit2;
    mic_data_t tmpbit4;
    mic_data_t tmpbit8;
    mic_data_t tmpbit16;
    mic_data_t sumbit1;
    mic_data_t sumbit2;
    mic_data_t sumbit4;
    mic_data_t sumbit8;
    mic_data_t sumbit16;
    mic_data_t carry0;
    mic_data_t carry1;
    mic_data_t carry2;
    mic_data_t carry3;
    mic_data_t carry4;
    mic_data_t remain_dh_neg5;
    mic_data_t dh_neg5_to_pos2;
    mic_data_t comp_dh_neg5_to_pos2;
    mic_data_t dvpos7shiftormatch;
    mic_data_t dvnot7to3ormatch;
    mic_data_t dh_pos7_or_match;
    mic_data_t dvdh_bit_high_comp;
    mic_data_t high_one = _mm512_set1_epi32(0x80000000);
    mic_data_t next_high_one = _mm512_set1_epi32(0x40000000);
    mic_data_t init_val;
    mic_data_t sum;
    mic_data_t tmp_value;
    mic_data_t init_tal;
    char * itr;
    mic_read_t * matchv;
    mic_read_t * read_temp = read;

    int tid = omp_get_thread_num();
    int start = tid * word_num * dvdh_len;
    dvdh_bit1 = & dvdh_bit_mem[start];
    dvdh_bit2 = & dvdh_bit_mem[start + word_num * 1];
    dvdh_bit4 = & dvdh_bit_mem[start + word_num * 2];
    dvdh_bit8 = & dvdh_bit_mem[start + word_num * 3];
    dvdh_bit16 = & dvdh_bit_mem[start + word_num * 4];
    for(k = 0; k < chunk_read_num; k++) {

        read =& read_temp[ k * word_num * MIC_V_NUM * CHAR_NUM];
        for (i = 0; i < word_num; i++) {
            dvdh_bit1[i] = dvdh_bit2[i] = dvdh_bit4[i] = dvdh_bit8[i] = dvdh_bit16[i] = _mm512_set1_epi32(0);
        }
        dh_pos7 = dh_pos6 = dh_pos5 = dh_pos4 = dh_pos3 = dh_pos2 = dh_pos1 = dh_zero = dh_neg1 = dh_neg2 = dh_neg3 = dh_neg4 = _mm512_set1_epi32(0);
        dh_neg5 = all_ones;

        for(i = 0, itr = ref; i < ref_len; i++, itr++) {

            bit1 = bit2 = bit4 = bit8 = bit16 = _mm512_set1_epi32(0);
            overflow0 = overflow1 = overflow2 = overflow3 = overflow4 = overflow5 = overflow6 = overflow7 = overflow8 = overflow9 = overflow10 = overflow11 = _mm512_set1_epi32(0);
            init_pos7_prevbit = init_pos6_prevbit = init_pos5_prevbit = init_pos4_prevbit = init_pos3_prevbit = init_pos2_prevbit = init_pos1_prevbit = init_zero_prevbit = init_neg1_prevbit = init_neg2_prevbit = init_neg3_prevbit = init_neg4_prevbit = _mm512_set1_epi32(0);
            matchv = & read[((int)*itr) * MIC_V_NUM * word_num];

            for(j = 0; j < word_num; j++) {

                dhbit1 = dvdh_bit1[j];
                dhbit2 = dvdh_bit2[j];
                dhbit4 = dvdh_bit4[j];
                dhbit8 = dvdh_bit8[j];
                dhbit16 = dvdh_bit16[j];
                matches = _mm512_load_epi32(matchv);
                matchv += MIC_V_NUM;
                not_matches = _mm512_xor_epi32(matches, all_ones);

                dhbit1inverse = _mm512_xor_epi32(dhbit1, all_ones);
                dhbit2inverse = _mm512_xor_epi32(dhbit2, all_ones);
                dhbit4inverse = _mm512_xor_epi32(dhbit4, all_ones);
                dhbit8inverse = _mm512_xor_epi32(dhbit8, all_ones);
                dhbit16inverse = _mm512_xor_epi32(dhbit16, all_ones);
                dhbit16inverse_dhbit8inverse = _mm512_and_epi32(dhbit16inverse, dhbit8inverse);
                dhbit16inverse_dhbit8inverse_dhbit4inverse = _mm512_and_epi32(dhbit16inverse_dhbit8inverse, dhbit4inverse);
                dhbit16inverse_dhbit8inverse_dhbit4inverse_dhbit2inverse = _mm512_and_epi32(dhbit16inverse_dhbit8inverse_dhbit4inverse, dhbit2inverse);
                dhbit16_dhbit8 = _mm512_and_epi32(dhbit16, dhbit8);
                dhbit16_dhbit8_dhbit4 = _mm512_and_epi32(dhbit16_dhbit8, dhbit4);
                dhbit16_dhbit8_dhbit4_dhbit2 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4, dhbit2);
                dhbit16_dhbit8_dhbit4_dhbit2inverse = _mm512_and_epi32(dhbit16_dhbit8_dhbit4, dhbit2inverse);
                dhbit16_dhbit8_dhbit4inverse = _mm512_and_epi32(dhbit16_dhbit8, dhbit4inverse);
                dhbit16_dhbit8_dhbit4inverse_dhbit2 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4inverse, dhbit2);
                dhbit16_dhbit8_dhbit4inverse_dhbit2inverse = _mm512_and_epi32(dhbit16_dhbit8_dhbit4inverse, dhbit2inverse);
                dh_pos2 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4inverse_dhbit2inverse, dhbit1);
                dh_pos1 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4inverse_dhbit2, dhbit1inverse);
                dh_zero = _mm512_and_epi32(dhbit16_dhbit8_dhbit4inverse_dhbit2, dhbit1);
                dh_neg1 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4_dhbit2inverse, dhbit1inverse);
                dh_neg2 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4_dhbit2inverse, dhbit1);
                dh_neg3 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4_dhbit2, dhbit1inverse);
                dh_neg4 = _mm512_and_epi32(dhbit16_dhbit8_dhbit4_dhbit2, dhbit1);
                dh_neg5 = _mm512_and_epi32(dhbit16inverse_dhbit8inverse_dhbit4inverse_dhbit2inverse, dhbit1inverse);
                dh_neg5 = _mm512_and_epi32(dh_neg5, carry_bitmask);

                init_pos7 = _mm512_and_epi32(dh_neg5, matches);
                sum = _mm512_add_epi32(init_pos7, dh_neg5);
                sum = _mm512_add_epi32(sum, overflow0);
                dvpos7shift = _mm512_xor_epi32(sum, dh_neg5);
                dvpos7shift = _mm512_xor_epi32(dvpos7shift, init_pos7);
                dvpos7shift = _mm512_and_epi32(dvpos7shift, carry_bitmask);
                overflow0 = _mm512_srli_epi32(sum, word_sizem1);
                remain_dh_neg5 = _mm512_xor_epi32(dh_neg5, init_pos7);
                dvpos7shiftormatch = _mm512_or_epi32(dvpos7shift, matches);
                init_pos6 = _mm512_and_epi32(dh_neg4, dvpos7shiftormatch);
                init_val = _mm512_slli_epi32(init_pos6, 1);
                init_val = _mm512_or_epi32(init_val, init_pos6_prevbit);
                init_pos6_prevbit = _mm512_srli_epi32(init_val, word_sizem1);
                init_val = _mm512_and_epi32(init_val, carry_bitmask);
                sum = _mm512_add_epi32(init_val, remain_dh_neg5);
                sum = _mm512_add_epi32(sum, overflow1);
                dvpos6shift = _mm512_xor_epi32(sum, remain_dh_neg5);
                dvpos6shift = _mm512_and_epi32(dvpos6shift, not_matches);
                overflow1 = _mm512_srli_epi32(sum, word_sizem1);

                init_pos5 = _mm512_and_epi32(dh_neg3, dvpos7shiftormatch);
                tmp_value = _mm512_and_epi32(dh_neg4, dvpos6shift);
                init_pos5 = _mm512_or_epi32(init_pos5, tmp_value);
                init_val = _mm512_slli_epi32(init_pos5, 1);
                init_val = _mm512_or_epi32(init_val, init_pos5_prevbit);
                init_pos5_prevbit = _mm512_srli_epi32(init_val, word_sizem1);
                init_val = _mm512_and_epi32(init_val, carry_bitmask);
                sum = _mm512_add_epi32(init_val, remain_dh_neg5);
                sum = _mm512_add_epi32(sum, overflow2);
                dvpos5shift = _mm512_xor_epi32(sum, remain_dh_neg5);
                dvpos5shift = _mm512_and_epi32(dvpos5shift, not_matches);
                overflow2 = _mm512_srli_epi32(sum, word_sizem1);

                init_pos4 = _mm512_and_epi32(dh_neg2, dvpos7shiftormatch);
                tmp_value = _mm512_and_epi32(dh_neg3, dvpos6shift);
                init_pos4 = _mm512_or_epi32(init_pos4, tmp_value);
                tmp_value = _mm512_and_epi32(dh_neg4, dvpos5shift);
                init_pos4 = _mm512_or_epi32(init_pos4, tmp_value);
                init_val = _mm512_slli_epi32(init_pos4, 1);
                init_val = _mm512_or_epi32(init_val, init_pos4_prevbit);
                init_pos4_prevbit = _mm512_srli_epi32(init_val, word_sizem1);
                init_val = _mm512_and_epi32(init_val, carry_bitmask);
                sum = _mm512_add_epi32(init_val, remain_dh_neg5);
                sum = _mm512_add_epi32(sum, overflow3);
                dvpos4shift = _mm512_xor_epi32(sum, remain_dh_neg5);
                dvpos4shift = _mm512_and_epi32(dvpos4shift, not_matches);
                overflow3 = _mm512_srli_epi32(sum, word_sizem1);

                init_pos3 = _mm512_and_epi32(dh_neg1, dvpos7shiftormatch);
                tmp_value = _mm512_and_epi32(dh_neg2, dvpos6shift);
                init_pos3 = _mm512_or_epi32(init_pos3, tmp_value);
                tmp_value = _mm512_and_epi32(dh_neg3, dvpos5shift);
                init_pos3 = _mm512_or_epi32(init_pos3, tmp_value);
                tmp_value = _mm512_and_epi32(dh_neg4, dvpos4shift);
                init_pos3 = _mm512_or_epi32(init_pos3, tmp_value);
                init_val = _mm512_slli_epi32(init_pos3, 1);
                init_val = _mm512_or_epi32(init_val, init_pos3_prevbit);
                init_pos3_prevbit = _mm512_srli_epi32(init_val, word_sizem1);
                init_val = _mm512_and_epi32(init_val, carry_bitmask);
                sum = _mm512_add_epi32(init_val, remain_dh_neg5);
                sum = _mm512_add_epi32(sum, overflow4);
                dvpos3shift = _mm512_xor_epi32(sum, remain_dh_neg5);
                dvpos3shift = _mm512_and_epi32(dvpos3shift, not_matches);
                overflow4 = _mm512_srli_epi32(sum, word_sizem1);

                dvnot7to3ormatch = _mm512_or_epi32(dvpos7shiftormatch, dvpos6shift);
                dvnot7to3ormatch = _mm512_or_epi32(dvnot7to3ormatch, dvpos5shift);
                dvnot7to3ormatch = _mm512_or_epi32(dvnot7to3ormatch, dvpos4shift);
                dvnot7to3ormatch = _mm512_or_epi32(dvnot7to3ormatch, dvpos3shift);
                dvnot7to3ormatch = _mm512_xor_epi32(dvnot7to3ormatch, all_ones);
                dvpos5shift_dvpos6shift = _mm512_or_epi32(dvpos5shift, dvpos6shift);
                dvpos5shift_dvpos6shift_dvnot7to3ormatch = _mm512_or_epi32(dvpos5shift_dvpos6shift, dvnot7to3ormatch);
                dvnot7to3ormatch_dvpos7shiftormatch = _mm512_or_epi32(dvnot7to3ormatch, dvpos7shiftormatch);
                dvpos3shift_dvpos4shift = _mm512_or_epi32(dvpos3shift, dvpos4shift);
                dvpos5shift_dvpos6shift_dvpos3shift_dvpos4shift = _mm512_or_epi32(dvpos5shift_dvpos6shift, dvpos3shift_dvpos4shift);
                dvpos5shift_dvpos6shift_dvpos3shift_dvpos4shift_dvpos7shiftormatch = _mm512_or_epi32(dvpos5shift_dvpos6shift_dvpos3shift_dvpos4shift, dvpos7shiftormatch);
                dv_bit1 = _mm512_or_epi32(dvnot7to3ormatch, dvpos4shift); 
                dv_bit1 = _mm512_or_epi32(dv_bit1, dvpos6shift); 
                dv_bit2 = dvpos5shift_dvpos6shift_dvnot7to3ormatch;
                dv_bit4 = dvnot7to3ormatch_dvpos7shiftormatch;
                dv_bit8 = dvpos5shift_dvpos6shift_dvpos3shift_dvpos4shift_dvpos7shiftormatch;
                dv_bit16 = _mm512_set1_epi32(0);

                carry0 = _mm512_and_epi32(dhbit1, dv_bit1);
                carry1 = _mm512_and_epi32(dhbit2, dv_bit2);
                dh_xor_dv_bit2 = _mm512_xor_epi32(dhbit2, dv_bit2);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit2, carry0);
                carry1 = _mm512_or_epi32(carry1, tmp_value);

                carry2 = _mm512_and_epi32(dhbit4, dv_bit4);
                dh_xor_dv_bit4 = _mm512_xor_epi32(dhbit4, dv_bit4);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit4, carry1);
                carry2 = _mm512_or_epi32(carry2, tmp_value);

                carry3 = _mm512_and_epi32(dhbit8, dv_bit8);
                dh_xor_dv_bit8 = _mm512_xor_epi32(dhbit8, dv_bit8);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit8, carry2);
                carry3 = _mm512_or_epi32(carry3, tmp_value);

                carry4 = _mm512_and_epi32(dhbit16, dv_bit16);
                dh_xor_dv_bit16 = _mm512_xor_epi32(dhbit16, dv_bit16);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit16, carry3);
                carry4 = _mm512_or_epi32(carry4, tmp_value);

                sumbit1 = _mm512_xor_epi32(dhbit1, dv_bit1);
                sumbit2 = _mm512_xor_epi32(dh_xor_dv_bit2, carry0);
                sumbit4 = _mm512_xor_epi32(dh_xor_dv_bit4, carry1);
                sumbit8 = _mm512_xor_epi32(dh_xor_dv_bit8, carry2);
                sumbit16 = _mm512_xor_epi32(dh_xor_dv_bit16, carry3);

                dvdh_bit_high_comp = _mm512_xor_epi32(sumbit16, all_ones);
                sumbit1 = _mm512_and_epi32(sumbit1, dvdh_bit_high_comp);
                sumbit2 = _mm512_and_epi32(sumbit2, dvdh_bit_high_comp);
                sumbit4 = _mm512_and_epi32(sumbit4, dvdh_bit_high_comp);
                sumbit8 = _mm512_and_epi32(sumbit8, dvdh_bit_high_comp);
                sumbit16 = _mm512_and_epi32(sumbit16, dvdh_bit_high_comp);

                tmpbit1 = _mm512_and_epi32(sumbit1, next_high_one);
                tmpbit1 = _mm512_srli_epi32(tmpbit1, word_sizem2);
                tmpbit2 = _mm512_and_epi32(sumbit2, next_high_one);
                tmpbit2 = _mm512_srli_epi32(tmpbit2, word_sizem2);
                tmpbit4 = _mm512_and_epi32(sumbit4, next_high_one);
                tmpbit4 = _mm512_srli_epi32(tmpbit4, word_sizem2);
                tmpbit8 = _mm512_and_epi32(sumbit8, next_high_one);
                tmpbit8 = _mm512_srli_epi32(tmpbit8, word_sizem2);
                tmpbit16 = _mm512_and_epi32(sumbit16, next_high_one);
                tmpbit16 = _mm512_srli_epi32(tmpbit16, word_sizem2);

                sumbit1 = _mm512_slli_epi32(sumbit1, 1);
                sumbit2 = _mm512_slli_epi32(sumbit2, 1);
                sumbit4 = _mm512_slli_epi32(sumbit4, 1);
                sumbit8 = _mm512_slli_epi32(sumbit8, 1);
                sumbit16 = _mm512_slli_epi32(sumbit16, 1);

                sumbit1 = _mm512_or_epi32(sumbit1, bit1);
                sumbit2 = _mm512_or_epi32(sumbit2, bit2);
                sumbit4 = _mm512_or_epi32(sumbit4, bit4);
                sumbit8 = _mm512_or_epi32(sumbit8, bit8);
                sumbit16 = _mm512_or_epi32(sumbit16, bit16);

                bit1 = tmpbit1;
                bit2 = tmpbit2;
                bit4 = tmpbit4;
                bit8 = tmpbit8;
                bit16 = tmpbit16;

                dv_bit1 = sumbit1;
                dv_bit2 = sumbit2;
                dv_bit4 = sumbit4;
                dv_bit8 = sumbit8;
                dv_bit16 = sumbit16;

                dh_pos7_or_match = _mm512_or_epi32(dh_pos7, matches);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5, dh_neg4);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5_to_pos2, dh_neg3);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5_to_pos2, dh_neg2);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5_to_pos2, dh_neg1);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5_to_pos2, dh_zero);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5_to_pos2, dh_pos1);
                dh_neg5_to_pos2 = _mm512_or_epi32(dh_neg5_to_pos2, dh_pos2);
                dh_neg5_to_pos2 = _mm512_and_epi32(dh_neg5_to_pos2, not_matches);
                comp_dh_neg5_to_pos2 = _mm512_xor_epi32(dh_neg5_to_pos2, all_ones);

                dhbit1 = _mm512_or_epi32(dhbit1, dh_neg5_to_pos2);
                dhbit2 = _mm512_and_epi32(dhbit2, comp_dh_neg5_to_pos2);
                dhbit4 = _mm512_and_epi32(dhbit4, comp_dh_neg5_to_pos2);
                dhbit8 = _mm512_or_epi32(dhbit8, dh_neg5_to_pos2);
                dhbit16 = _mm512_or_epi32(dhbit16, dh_neg5_to_pos2);

                dhbit1 = _mm512_and_epi32(dhbit1, not_matches);
                dhbit2 = _mm512_and_epi32(dhbit2, not_matches);
                dhbit4 = _mm512_or_epi32(dhbit4, matches);
                dhbit8 = _mm512_and_epi32(dhbit8, not_matches);
                dhbit16 = _mm512_or_epi32(dhbit16, matches);

                carry0 = _mm512_and_epi32(dhbit1, sumbit1);
                carry1 = _mm512_and_epi32(dhbit2, sumbit2);
                dh_xor_dv_bit2 = _mm512_xor_epi32(dhbit2, sumbit2);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit2, carry0);
                carry1 = _mm512_or_epi32(carry1, tmp_value);

                carry2 = _mm512_and_epi32(dhbit4, sumbit4);
                dh_xor_dv_bit4 = _mm512_xor_epi32(dhbit4, sumbit4);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit4, carry1);
                carry2 = _mm512_or_epi32(carry2, tmp_value);

                carry3 = _mm512_and_epi32(dhbit8, sumbit8);
                dh_xor_dv_bit8 = _mm512_xor_epi32(dhbit8, sumbit8);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit8, carry2);
                carry3 = _mm512_or_epi32(carry3, tmp_value);

                carry4 = _mm512_and_epi32(dhbit16, sumbit16);
                dh_xor_dv_bit16 = _mm512_xor_epi32(dhbit16, sumbit16);
                tmp_value = _mm512_and_epi32(dh_xor_dv_bit16, carry3);
                carry4 = _mm512_or_epi32(carry4, tmp_value);

                sumbit1 = _mm512_xor_epi32(dhbit1, sumbit1);
                sumbit2 = _mm512_xor_epi32(dh_xor_dv_bit2, carry0);
                sumbit4 = _mm512_xor_epi32(dh_xor_dv_bit4, carry1);
                sumbit8 = _mm512_xor_epi32(dh_xor_dv_bit8, carry2);
                sumbit16 = _mm512_xor_epi32(dh_xor_dv_bit16, carry3);

                sumbit1 = _mm512_and_epi32(sumbit1, sumbit16);
                sumbit2 = _mm512_and_epi32(sumbit2, sumbit16);
                sumbit4 = _mm512_and_epi32(sumbit4, sumbit16);
                sumbit8 = _mm512_and_epi32(sumbit8, sumbit16);
                sumbit16 = _mm512_and_epi32(sumbit16, sumbit16);

                dvdh_bit1[j] = sumbit1;
                dvdh_bit2[j] = sumbit2;
                dvdh_bit4[j] = sumbit4;
                dvdh_bit8[j] = sumbit8;
                dvdh_bit16[j] = sumbit16;
            }

        }

        mic_data_t vec1 = _mm512_set1_epi32(1);
        mic_data_t vec2 = _mm512_set1_epi32(2);
        mic_data_t vec4 = _mm512_set1_epi32(4);
        mic_data_t vec8 = _mm512_set1_epi32(8);
        mic_data_t vec16 = _mm512_set1_epi32(16);
        mic_data_t vec_5 = _mm512_set1_epi32(5);
        mic_data_t score = _mm512_set1_epi32(-5 * ref_len);

        for(j = 0; j < word_num; j++) {
            for(i = j * word_sizem1; i < (j + 1) * word_sizem1 && i < read_len; i++) {
                tmp_value = _mm512_and_epi32(dvdh_bit1[j], vec1);
                tmp_value = _mm512_mullo_epi32(tmp_value, vec1);
                score = _mm512_sub_epi32(score, tmp_value);

                tmp_value = _mm512_and_epi32(dvdh_bit2[j], vec1);
                tmp_value = _mm512_mullo_epi32(tmp_value, vec2);
                score = _mm512_sub_epi32(score, tmp_value);

                tmp_value = _mm512_and_epi32(dvdh_bit4[j], vec1);
                tmp_value = _mm512_mullo_epi32(tmp_value, vec4);
                score = _mm512_sub_epi32(score, tmp_value);

                tmp_value = _mm512_and_epi32(dvdh_bit8[j], vec1);
                tmp_value = _mm512_mullo_epi32(tmp_value, vec8);
                score = _mm512_sub_epi32(score, tmp_value);

                tmp_value = _mm512_and_epi32(dvdh_bit16[j], vec1);
                tmp_value = _mm512_mullo_epi32(tmp_value, vec16);
                score = _mm512_add_epi32(score, tmp_value);

                score = _mm512_sub_epi32(score, vec_5);
                dvdh_bit1[j] = _mm512_srli_epi32(dvdh_bit1[j], 1);
                dvdh_bit2[j] = _mm512_srli_epi32(dvdh_bit2[j], 1);
                dvdh_bit4[j] = _mm512_srli_epi32(dvdh_bit4[j], 1);
                dvdh_bit8[j] = _mm512_srli_epi32(dvdh_bit8[j], 1);
                dvdh_bit16[j] = _mm512_srli_epi32(dvdh_bit16[j], 1);
            }

        }

        int * vec_dump = ((int *) & score);
        int index = result_index * MIC_V_NUM;

        #pragma vector always
        #pragma ivdep
        for(i = 0; i < MIC_V_NUM; i++){
            results[index + i] = vec_dump[i];
        }
        result_index++;
    }

}

