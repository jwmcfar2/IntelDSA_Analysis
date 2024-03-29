#include "utils.h"
#pragma once

// 'Global' Variables to reference *just* for this C file
static uint8_t* src;
static uint8_t* dst;

// Function list - returns time elapsed (in cycles) for each of these implementations //
// ## Mem Mv ## //
uint64_t  volatile  single_movqInsASM(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_memcpyC(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_SSE1movaps(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_SSE2movdqa(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_SSE4movntdq(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_AVX256(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_AVX512_32(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_AVX512_64(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_AMX(uint64_t bufSize, bool primeCache);
uint64_t  volatile  bulk_movqInsASM(uint64_t bufSize);
uint64_t  volatile  bulk_memcpyC(uint64_t bufSize);
uint64_t  volatile  bulk_SSE1movaps(uint64_t bufSize);
uint64_t  volatile  bulk_SSE2movdqa(uint64_t bufSize);
uint64_t  volatile  bulk_SSE4movntdq(uint64_t bufSize);
uint64_t  volatile  bulk_AVX256(uint64_t bufSize);
uint64_t  volatile  bulk_AVX512_32(uint64_t bufSize);
uint64_t  volatile  bulk_AVX512_64(uint64_t bufSize);
bool      volatile  syscall_use_tile();

// ## Flush Ops ## //
uint64_t  volatile  single_clflush(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_clflushopt(uint64_t bufSize, bool primeCache);
uint64_t  volatile  bulk_clflush(uint64_t bufSize);
uint64_t  volatile  bulk_clflushopt(uint64_t bufSize);

// ## Mem Cmp ## //
uint64_t  volatile  single_Cmemcmp(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_SSE2cmp(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_SSE4cmp(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_AVX256cmp(uint64_t bufSize, bool primeCache);
uint64_t  volatile  single_AVX512cmp(uint64_t bufSize, bool primeCache);
uint64_t  volatile  bulk_Cmemcmp(uint64_t bufSize);
uint64_t  volatile  bulk_SSE2cmp(uint64_t bufSize);
uint64_t  volatile  bulk_SSE4cmp(uint64_t bufSize);
uint64_t  volatile  bulk_AVX256cmp(uint64_t bufSize);
uint64_t  volatile  bulk_AVX512cmp(uint64_t bufSize);