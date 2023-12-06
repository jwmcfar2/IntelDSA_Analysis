#include "otherFeatureTests.h"

// ~~~~~~~~ C Memcpy INS ~~~~~~~~ //
uint64_t volatile single_memcpyC(uint64_t bufSize){
    //printf("\n || Test Latency of C 'memcpy' function\n");
    uint64_t startTime, endTime;

    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();

    memcpy(dst, src, bufSize);
    endTime=rdtscp();
    //printf("\t> Completed C 'memcpy' function: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[memcpyC] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ ASM Movq INS ~~~~~~~~ //
uint64_t volatile single_movqInsASM(uint64_t bufSize){
    //printf("\n || Test Latency of ASM mem mov\n");
    uint64_t startTime, endTime;
        
    detailedAssert((bufSize%8 == 0),"ASM movq - Not divisible by 8");
    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for(int i=0; i<bufSize; i+=8) {
        asm ("movq (%1), %%rax\n\t"
                    "movq %%rax, (%0);"
                    :
                    : "r" (dst + i), "r" (src + i)
                    : "%rax"
                    );
    }
    endTime=rdtscp();
    //printf("\t> Completed looped 64-bit CPU Mov Ins (movq): Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[ASMmovq] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE1 Movaps ~~~~~~~~ //
uint64_t volatile single_SSE1movaps(uint64_t bufSize){
    //printf("\n || Test Latency of SSE1 Mov Instruction (movaps)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%16 == 0),"SSE1 movaps - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for(int i=0; i<bufSize; i+=16) {
        __m128 chunk = _mm_load_ps((const float*)(src + i));
        _mm_store_ps((float*)(dst + i), chunk);
    }
    endTime=rdtscp();
    //printf("\t> Completed SSE1 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[SSE1] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE2 Movdqa ~~~~~~~~ //
uint64_t volatile single_SSE2movdqa(uint64_t bufSize){
    //printf("\n || Test Latency of SSE2 Mov Instruction (movdqa)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%16 == 0),"SSE2 movdqa - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for(int i=0; i<bufSize; i+=16) {  // SSE2 register (128-bit or 16 byte) copy, 256 iterations
        __m128i data = _mm_load_si128((__m128i *)(src + i)); // Load 128-bits aligned data
        _mm_store_si128((__m128i *)(dst + i), data); // Store 128-bits aligned data
    }
    endTime=rdtscp();
    //printf("\t> Completed SSE2 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[SSE2] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE4.1 Movntdq (non-temporal) ~~~~~~~~ //
uint64_t volatile single_SSE4movntdq(uint64_t bufSize){
    //printf("\n || Test Latency of SSE4.1 MovToMem Instruction (Movntdq)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%16 == 0),"SSE4.1 movntdq - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    // Assuming src and dst are 16-byte aligned and len is a multiple of 16
    #pragma unroll
    for(int i=0; i<bufSize; i+=16) {
        // Non-temporal loads are not part of SSE intrinsics. We use regular loads.
        __m128i data = _mm_load_si128((__m128i*)(src + i)); // Load 128 bits from the source

        // Non-temporal store to write the data in the store buffer and eventually to memory.
        _mm_stream_si128((__m128i*)(dst + i), data);
    }
    // A non-temporal store may leave data in the store buffer. The sfence instruction guarantees
    // that every preceding store is globally visible before any load or store instructions that
    // follow the sfence instruction.
    //printf("\t> Finished *executing* SSE4.1 Mov Ins: Cycles elapsed = %lu cycles.\n", rdtscp()-startTime);
    _mm_sfence();
    endTime=rdtscp();
    //printf("\t> Completed SSE4.1 Mov Ins (TO MEMORY, NOT CACHE): Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[SSE4] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-256 ~~~~~~~~ //
uint64_t volatile single_AVX256(uint64_t bufSize){
    //printf(" || Test Latency of AVX-256 Mov Instruction (vmovdqa)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%32 == 0),"AVX-256 vmovdqa - Not divisible by 32");
    src = aligned_alloc(32, bufSize);
    dst = aligned_alloc(32, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for(int i=0; i<bufSize; i+=32) {
        __m256i data = _mm256_load_si256((__m256i*)(src + i)); // Load 256 bits from source
        _mm256_store_si256((__m256i*)(dst + i), data);         // Store 256 bits into destination
    }
    endTime=rdtscp();
    //printf("\t> Completed AVX-256 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[AVX256] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-512 32-bit ~~~~~~~~ //
uint64_t volatile single_AVX512_32(uint64_t bufSize){
    //printf("\n || Test Latency of AVX-512 (32-bit) Mov Ins (vmovdqa32)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%32 == 0),"AVX-512 (32-bit) - Not divisible by 32");
    src = aligned_alloc(64, bufSize);
    dst = aligned_alloc(64, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for(int i=0; i<bufSize; i+=64) {
        __m512i data = _mm512_load_si512((__m512i*)(src + i)); // Load 512 bits from the source
        _mm512_store_si512((__m512i*)(dst + i), data);         // Store 512 bits into the destination
    }
    endTime=rdtscp();
    //printf("\t> Completed AVX-512 (32-bit) Mov Ins: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[AVX512_32] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-512 64-bit ~~~~~~~~ //
uint64_t volatile single_AVX512_64(uint64_t bufSize){
    //printf("\n || Test Latency of AVX-512 (64-bit) Mov Ins (vmovdqa64)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%32 == 0),"AVX-512 (64-bit) - Not divisible by 32");
    src = aligned_alloc(64, bufSize);
    dst = aligned_alloc(64, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    flush2(&src, &dst);
    startTime=rdtscp();
    #pragma unroll
    for(int i=0; i<bufSize; i+=64) {
        __m512i data = _mm512_load_epi64((__m512i*)(src + i)); // Load 512 bits from the source
        _mm512_store_epi64((__m512i*)(dst + i), data);         // Store 512 bits into the destination
    }
    endTime=rdtscp();
    //printf("\t> Completed AVX-512 (64-bit) Mov Ins: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[AVX512_64] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AMX LD/ST Tiles ~~~~~~~~ //
uint64_t volatile single_AMX(uint64_t bufSize){
    // NOTE - Every OTHER power of two will have even sqrt() (1..4..16..64..)
    // Which you can find by: (log2(N)%2 == 0) -- Nvm, fix tiles at 16 x 64 uint8_t
    uint64_t startTime, endTime;
    uint16_t tilePartial;
    uint8_t rowPartial, tileCount, **srcAMX, **dstAMX;//, maxTileArr[AMX_MAX_TILE_SIZE]; //maxRows, 
    uint8_t testSRC[AMX_TILE_SIZE], testDST[AMX_TILE_SIZE];
    uint8_t *test2SRC, *test2DST;
    uint8_t **test3SRC, **test3DST;
    AMXtile tileCFG = {0};

    detailedAssert(syscall_use_tile(),"AMX - Failed to enable AMX XFEATURE_XTILEDATA.");

    //test2SRC = malloc(AMX_TILE_SIZE);
    //test2DST = malloc(AMX_TILE_SIZE);
    test3SRC = malloc(2 * AMX_TILE_SIZE);
    test3DST = malloc(2 * AMX_TILE_SIZE);

    for(int i=0; i<2; i++)
    {
        test3SRC[i] = malloc(AMX_TILE_SIZE);
        test3DST[i] = malloc(AMX_TILE_SIZE);
    }

    // DEBUG
    uint8_t debugBool = 0;

    // Convert bufSize into digestible tile sizes of N (16 x 64B = 1024B) tiles
    tilePartial = bufSize % AMX_TILE_SIZE; // If tilePartial !=0, we must add one more tile with this number of bytes
    tileCount = ((bufSize / AMX_TILE_SIZE) + (tilePartial > 0)); // Should floor() by default since int / int, then add 1 if partial
    rowPartial = (tilePartial/AMX_COL_WIDTH)+(tilePartial%AMX_COL_WIDTH>0); // For the last tile we need, how many rows we need (out of AMX_MAX_ROWS)

    //printf("Test-A\n");
    // Load tile configuration 
    //init_tile_config (&tileCFG);

    tileCFG.palette_id = 1;
    tileCFG.start_row = 0;
    //printf("Test-a1\n");

    for (int i = 0; i < 3; i++)
    {
        //printf("\t>Test-a2\n");
        tileCFG.colsb[i] = AMX_COL_WIDTH;
        tileCFG.rows[i] =  AMX_MAX_ROWS;
    }

    //printf("Test-a4\n");
    _tile_loadconfig (&tileCFG);
    //printf("Test-a4\n");

    //printf("Test-B\n");
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<AMX_MAX_ROWS; j++)
        {
            for(int k=0; k<AMX_COL_WIDTH; k++)
            {
                test3SRC[i][j*AMX_COL_WIDTH + k] = rand()%(100);
                test3DST[i][j*AMX_COL_WIDTH + k] = 0;
            }
        }
    }

    //printf("Test-C\n");
    //print_buffer(test3SRC, rows, colsb);

    //init_buffer (test3DST, 0);
    //printf("Test-D\n");

    //print_buffer(test3DST, rows, colsb);

    // Init dst matrix buffers with data
    //init_buffer32 (res, 0);
    //printf("Test-E -- DEBUG: size(test3SRC)=%u, test3SRC=%u\n", sizeof(test3SRC), AMX_TILE_SIZE*sizeof(int8_t));

    uint64_t debugLen = AMX_MAX_ROWS*AMX_COL_WIDTH;
    /*printf("\nBEFORE...\n");
    printf("\nTest3Src[0]:\n\t");
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<debugLen; j++)
        {
            printf("%u ", test3SRC[i][j]);
            if(test3SRC[i][j]<10)
                printf(" ");
            if(j!=0 && j%32==31)
                printf("\n\t");
        }
    }

    printf("\n\nTest3Dst[0]:\n\t");
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<debugLen; j++)
        {
            printf("%u ", test3DST[i][j]);
            if(test3DST[i][j]<10)
                printf(" ");
            if(j!=0 && j%32==31)
                printf("\n\t");
        }
    }*/

    // Load tile rows from memory
    _tile_loadd (1, test3SRC[0], AMX_STRIDE);
    //printf("\nTest-F1\n");
    _tile_loadd (2, test3SRC[1], AMX_STRIDE);
    //printf("Test-F2\n");



    // JMAC TEST: //
    ////////////////
    _tile_stored (1, test3DST[0], AMX_STRIDE);
    //printf("Test-F3\n");
    _tile_stored (2, test3DST[1], AMX_STRIDE);
    //printf("Test-G\n");
    //printf("\n~~~~ SUCCESS! ~~~~~\n\n");
    ////////////////
    //~ Aww yuss ~//


    /*printf("\nAFTER...\n");
    printf("\nTest3Src[0]:\n\t");
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<debugLen; j++)
        {
            printf("%u ", test3SRC[i][j]);
            if(test3SRC[i][j]<10)
                printf(" ");
            if(j!=0 && j%32==31)
                printf("\n\t");
        }
    }

    printf("\nTest3Dst[0]:\n\t");
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<debugLen; j++)
        {
            printf("%u ", test3DST[i][j]);
            if(test3DST[i][j]<10)
                printf(" ");
            if(j!=0 && j%32==31)
                printf("\n\t");
        }
    }*/

    // FIX ME
    for(int i=0; i<2; i++)
        valueCheck(test3SRC[i], test3DST[i], AMX_TILE_SIZE, "[AMX Tile Ld/St] ");

    // DELETE ME
    printf("\nSuccessful AMX Values! Exiting...\n");
    abort();

    _tile_release();
    // Free 2D Arrays
    for(int i=0; i<tileCount; i++)
    {
        free(srcAMX[i]);
        free(dstAMX[i]);
    }
    free(srcAMX);
    free(dstAMX);

    printf("DEBUG TEST -- AMX SUCCESS! (%lu)\n", (endTime-startTime));
    return (endTime-startTime);
}

bool volatile syscall_use_tile()
{
    return (!syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA));
}