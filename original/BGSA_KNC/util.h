#include <stdio.h>

#include "global.h"


__ONMIC__ void print_binary(uint64_t t, int bit_len);
__ONMIC__ void printf_mm512_one(__m512i m, int index, char* s, int i);

__ONMIC__ void printf_mm512_one_int(__m512i m, int index, char* s);

__ONMIC__ void printf_mm512(__m512i m);
