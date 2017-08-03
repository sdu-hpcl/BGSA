#include <stdio.h>
#include "util.h"

/*void print_binary(uint64_t t, int bit_len) {
	short buffer[bit_len];
	int i;
	for(i = 0; i < bit_len; i++) {
		buffer[i] = 0;
	}

	for (i = 0; i < bit_len; i++) {
		if (t == 0)
			break;
		if (t % 2 == 0) {
			buffer[i] = 0;
		} else {
			buffer[i] = 1;
		}
		t = t / 2;
	}
	for (i = bit_len - 1; i >= 0; i--) {
		printf("%hd", buffer[i]);
	}
}

void printf_mm512_one(__m512i m, int index, char* s, int i) {
    uint32_t *nums = (uint32_t *)_mm_malloc(sizeof(uint32_t) * 16, 64);
    _mm512_store_epi32(nums, m);

    printf("%s [%2d] is: ", s, i);
    //print_binary(nums[index], 32);
    printf("%d", nums[index]);
    printf("\n");

    _mm_free(nums);

}

void printf_mm512_one_int(__m512i m, int index, char* s) {
    uint32_t *nums = (uint32_t *)_mm_malloc(sizeof(uint32_t) * 16, 64);
    _mm512_store_epi32(nums, m);

    printf("%s is: ", s);
    //print_binary(nums[index], 32);
    printf("%d", nums[index]);
    printf("\n");
    _mm_free(nums);
}


void  printf_mm512(__m512i m) {
    uint32_t *nums = (uint32_t *)_mm_malloc(sizeof(uint32_t) * 16, 64);
    _mm512_store_epi32(nums, m);

    int i;
    for(i = 0; i < 16; i++) {
        print_binary(nums[i], 32);
        printf("\n");
    }

    _mm_free(nums);
} */
