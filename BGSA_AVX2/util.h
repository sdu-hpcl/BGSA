#include <stdio.h>

#include "global.h"

void print_binary(uint64_t t, int bit_len);

void printf_mm512_one(__m512i m, int index, char* s, int i);

void printf_mm512_one_int(__m512i m, int index, char* s);

void printf_mm512(__m512i m);
