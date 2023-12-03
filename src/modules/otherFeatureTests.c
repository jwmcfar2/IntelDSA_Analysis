#include "otherFeatureTests.h"

//static uint64_t startTime, endTime, randVal;
//static char* src;
//static char* dst;

// ~~~~~~~~ C Memcpy INS ~~~~~~~~ //
uint64_t single_memcpyC(uint64_t bufSize){
    printf("\n || Test Latency of C 'memcpy' function\n");
    uint64_t startTime, endTime;
    int i;

    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();

    memcpy(dst, src, bufSize);
    endTime=rdtscp();
    printf("\t> Completed C 'memcpy' function: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ ASM Movq INS ~~~~~~~~ //
uint64_t single_movqInsASM(uint64_t bufSize){
    printf("\n || Test Latency of ASM mem mov\n");
    uint64_t startTime, endTime;
    int i;
    
    detailedAssert((bufSize%8 == 0),"ASM movq - Not divisible by 8");
    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for (i = 0; i < bufSize; i+=8) {
        asm ("movq (%1), %%rax\n\t"
                    "movq %%rax, (%0);"
                    :
                    : "r" (dst + i), "r" (src + i)
                    : "%rax"
                    );
    }
    endTime=rdtscp();
    printf("\t> Completed looped 64-bit CPU Mov Ins (movq): Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE1 Movaps ~~~~~~~~ //
uint64_t single_SSE1movaps(uint64_t bufSize){
    printf("\n || Test Latency of SSE1 Mov Instruction (movaps)\n");
    uint64_t startTime, endTime;
    int i;

    detailedAssert((bufSize%16 == 0),"SSE1 movaps - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for (i = 0; i < bufSize; i += 16) {
        __m128 chunk = _mm_load_ps((const float*)(src + i));
        _mm_store_ps((float*)(dst + i), chunk);
    }
    endTime=rdtscp();
    printf("\t> Completed SSE1 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE2 Movdqa ~~~~~~~~ //
uint64_t single_SSE2movdqa(uint64_t bufSize){
    printf("\n || Test Latency of SSE2 Mov Instruction (movdqa)\n");
    uint64_t startTime, endTime;
    int i;

    detailedAssert((bufSize%16 == 0),"SSE2 movdqa - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for (i = 0; i < bufSize; i += 16) {  // SSE2 register (128-bit or 16 byte) copy, 256 iterations
        __m128i data = _mm_load_si128((__m128i *)(src + i)); // Load 128-bits aligned data
        _mm_store_si128((__m128i *)(dst + i), data); // Store 128-bits aligned data
    }
    endTime=rdtscp();
    printf("\t> Completed SSE2 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE4.1 Movntdq (non-temporal) ~~~~~~~~ //
uint64_t single_SSE4movntdq(uint64_t bufSize){
    printf("\n || Test Latency of SSE4.1 MovToMem Instruction (Movntdq)\n");
    uint64_t startTime, endTime;
    int i;

    detailedAssert((bufSize%16 == 0),"SSE4.1 movntdq - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    // Assuming src and dst are 16-byte aligned and len is a multiple of 16
    #pragma unroll
    for (i = 0; i < bufSize; i += 16) {
        // Non-temporal loads are not part of SSE intrinsics. We use regular loads.
        __m128i data = _mm_load_si128((__m128i*)(src + i)); // Load 128 bits from the source

        // Non-temporal store to write the data in the store buffer and eventually to memory.
        _mm_stream_si128((__m128i*)(dst + i), data);
    }
    // A non-temporal store may leave data in the store buffer. The sfence instruction guarantees
    // that every preceding store is globally visible before any load or store instructions that
    // follow the sfence instruction.
    printf("\t> Finished *executing* SSE4.1 Mov Ins: Cycles elapsed = %lu cycles.\n", rdtscp()-startTime);
    _mm_sfence();
    endTime=rdtscp();
    printf("\t> Completed SSE4.1 Mov Ins (TO MEMORY, NOT CACHE): Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-256 ~~~~~~~~ //
uint64_t single_AVX256(uint64_t bufSize){
    printf(" || Test Latency of AVX-256 Mov Instruction (vmovdqa)\n");
    uint64_t startTime, endTime;
    int i;

    detailedAssert((bufSize%32 == 0),"AVX-256 vmovdqa - Not divisible by 32");
    src = aligned_alloc(32, bufSize);
    dst = aligned_alloc(32, bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for (i = 0; i < bufSize; i += 32) {
        __m256i data = _mm256_load_si256((__m256i*)(src + i)); // Load 256 bits from source
        _mm256_store_si256((__m256i*)(dst + i), data);         // Store 256 bits into destination
    }
    endTime=rdtscp();
    printf("\t> Completed AVX-256 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-512 32-bit ~~~~~~~~ //
uint64_t single_AVX512_32(uint64_t bufSize){
    printf("\n || Test Latency of AVX-512 (32-bit) Mov Ins (vmovdqa32)\n");
    uint64_t startTime, endTime;
    int i;

    detailedAssert((bufSize%32 == 0),"AVX-512 (32-bit) - Not divisible by 32");
    src = aligned_alloc(64, bufSize);
    dst = aligned_alloc(64, bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for (i = 0; i < bufSize; i += 64) {
        __m512i data = _mm512_load_si512((__m512i*)(src + i)); // Load 512 bits from the source
        _mm512_store_si512((__m512i*)(dst + i), data);         // Store 512 bits into the destination
    }
    endTime=rdtscp();
    printf("\t> Completed AVX-512 (32-bit) Mov Ins: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-512 64-bit ~~~~~~~~ //
uint64_t single_AVX512_64(uint64_t bufSize){
    printf("\n || Test Latency of AVX-512 (64-bit) Mov Ins (vmovdqa64)\n");
    uint64_t startTime, endTime;
    int i;

    detailedAssert((bufSize%32 == 0),"AVX-512 (64-bit) - Not divisible by 32");
    src = aligned_alloc(64, bufSize);
    dst = aligned_alloc(64, bufSize);
    for(i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for (i = 0; i < bufSize; i += 64) {
        __m512i data = _mm512_load_epi64((__m512i*)(src + i)); // Load 512 bits from the source
        _mm512_store_epi64((__m512i*)(dst + i), data);         // Store 512 bits into the destination
    }
    endTime=rdtscp();
    printf("\t> Completed AVX-512 (64-bit) Mov Ins: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize);
    free(src);
    free(dst);

    return (endTime-startTime);
}