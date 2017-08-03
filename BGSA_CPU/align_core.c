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
