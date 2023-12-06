#include <x86intrin.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX/AMX intrinsics

#include "utils.h"
#pragma once

// 'Global' Variables to reference *just* for this C file
static uint8_t* src;
static uint8_t* dst;

// Function list - returns time elapsed (in cycles) for each of these implementations
uint64_t  volatile  single_memcpyC(uint64_t bufSize);
uint64_t  volatile  single_movqInsASM(uint64_t bufSize);
uint64_t  volatile  single_SSE1movaps(uint64_t bufSize);
uint64_t  volatile  single_SSE2movdqa(uint64_t bufSize);
uint64_t  volatile  single_SSE4movntdq(uint64_t bufSize);
uint64_t  volatile  single_AVX256(uint64_t bufSize);
uint64_t  volatile  single_AVX512_32(uint64_t bufSize);
uint64_t  volatile  single_AVX512_64(uint64_t bufSize);
uint64_t  volatile  single_AMX(uint64_t bufSize);
bool      volatile  syscall_use_tile();