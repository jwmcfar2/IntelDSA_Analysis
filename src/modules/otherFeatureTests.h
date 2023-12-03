#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <x86intrin.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX intrinsics

#include "utils.h"
#pragma once

// 'Global' Variables to reference *just* for this C file
static char* src;
static char* dst;

// Function list - returns time elapsed (in cycles) for each of these implementations
uint64_t single_memcpyC(uint64_t bufferSize);
uint64_t single_movqInsASM(uint64_t bufferSize);
uint64_t single_SSE1movaps(uint64_t bufferSize);
uint64_t single_SSE2movdqa(uint64_t bufferSize);
uint64_t single_SSE4movntdq(uint64_t bufferSize);
uint64_t single_AVX256(uint64_t bufferSize);
uint64_t single_AVX512_32(uint64_t bufferSize);
uint64_t single_AVX512_64(uint64_t bufferSize);