#include "otherFeatureTests.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~ Mem Mv Fns ~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// ~~~~~~~~ ASM Movq INS ~~~~~~~~ //
uint64_t volatile single_movqInsASM(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of ASM mem mov\n");
    uint64_t startTime, endTime;
        
    detailedAssert((bufSize%8 == 0),"ASM movq - Not divisible by 8");
    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);
    
    startTime=rdtsc();
    #pragma unroll
    for(int i=0; i<bufSize; i+=8) {
        asm ("movq (%1), %%rax\n\t"
                    "movq %%rax, (%0);"
                    :
                    : "r" (dst + i), "r" (src + i)
                    : "%rax"
                    );
    }
    endTime=rdtsc();
    //printf("\t> Completed looped 64-bit CPU Mov Ins (movq): Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[ASMmovq] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ C Memcpy INS ~~~~~~~~ //
uint64_t volatile single_memcpyC(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of C 'memcpy' function\n");
    uint64_t startTime, endTime;

    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);
    
    startTime=rdtsc();
    memcpy(dst, src, bufSize);
    endTime=rdtsc();
    //printf("\t> Completed C 'memcpy' function: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[memcpyC] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE1 Movaps ~~~~~~~~ //
uint64_t volatile single_SSE1movaps(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of SSE1 Mov Instruction (movaps)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%16 == 0),"SSE1 movaps - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);
    
    startTime=rdtsc();
    #pragma unroll
    for(int i=0; i<bufSize; i+=16) {
        __m128 chunk = _mm_load_ps((const float*)(src + i));
        _mm_store_ps((float*)(dst + i), chunk);
    }
    endTime=rdtsc();
    //printf("\t> Completed SSE1 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[SSE1] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE2 Movdqa ~~~~~~~~ //
uint64_t volatile single_SSE2movdqa(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of SSE2 Mov Instruction (movdqa)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%16 == 0),"SSE2 movdqa - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);
    
    startTime=rdtsc();
    #pragma unroll
    for(int i=0; i<bufSize; i+=16) {  // SSE2 register (128-bit or 16 byte) copy, 256 iterations
        __m128i data = _mm_load_si128((__m128i *)(src + i)); // Load 128-bits aligned data
        _mm_store_si128((__m128i *)(dst + i), data); // Store 128-bits aligned data
    }
    endTime=rdtsc();
    //printf("\t> Completed SSE2 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[SSE2] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ SSE4.1 Movntdq (non-temporal) ~~~~~~~~ //
uint64_t volatile single_SSE4movntdq(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of SSE4.1 MovToMem Instruction (Movntdq)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%16 == 0),"SSE4.1 movntdq - Not divisible by 16");
    src = aligned_alloc(16, bufSize);
    dst = aligned_alloc(16, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);
    
    startTime=rdtsc();
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
    //printf("\t> Finished *executing* SSE4.1 Mov Ins: Cycles elapsed = %lu cycles.\n", rdtsc()-startTime);
    _mm_sfence();
    endTime=rdtsc();
    //printf("\t> Completed SSE4.1 Mov Ins (TO MEMORY, NOT CACHE): Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[SSE4] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-256 ~~~~~~~~ //
uint64_t volatile single_AVX256(uint64_t bufSize, bool primeCache){
    //printf(" || Test Latency of AVX-256 Mov Instruction (vmovdqa)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%32 == 0),"AVX-256 vmovdqa - Not divisible by 32");
    src = aligned_alloc(32, bufSize);
    dst = aligned_alloc(32, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);

    startTime=rdtsc();
    #pragma unroll
    for(int i=0; i<bufSize; i+=32) {
        __m256i data = _mm256_load_si256((__m256i*)(src + i)); // Load 256 bits from source
        _mm256_store_si256((__m256i*)(dst + i), data);         // Store 256 bits into destination
    }
    endTime=rdtsc();
    //printf("\t> Completed AVX-256 Mov Instructions: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[AVX256] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-512 32-bit ~~~~~~~~ //
uint64_t volatile single_AVX512_32(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of AVX-512 (32-bit) Mov Ins (vmovdqa32)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%32 == 0),"AVX-512 (32-bit) - Not divisible by 32");
    src = aligned_alloc(64, bufSize);
    dst = aligned_alloc(64, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);

    startTime=rdtsc();
    #pragma unroll
    for(int i=0; i<bufSize; i+=64) {
        __m512i data = _mm512_load_si512((__m512i*)(src + i)); // Load 512 bits from the source
        _mm512_store_si512((__m512i*)(dst + i), data);         // Store 512 bits into the destination
    }
    endTime=rdtsc();
    //printf("\t> Completed AVX-512 (32-bit) Mov Ins: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[AVX512_32] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AVX-512 64-bit ~~~~~~~~ //
uint64_t volatile single_AVX512_64(uint64_t bufSize, bool primeCache){
    //printf("\n || Test Latency of AVX-512 (64-bit) Mov Ins (vmovdqa64)\n");
    uint64_t startTime, endTime;
    
    detailedAssert((bufSize%32 == 0),"AVX-512 (64-bit) - Not divisible by 32");
    src = aligned_alloc(64, bufSize);
    dst = aligned_alloc(64, bufSize);
    for(int i=0; i<bufSize; i++)
        src[i] = rand()%(255);

    // Either prime the cache with these, or flush them from cache
    if(primeCache)
        prime2(src, dst, bufSize);
    else
        flush2(&src, &dst);

    startTime=rdtsc();
    #pragma unroll
    for(int i=0; i<bufSize; i+=64) {
        __m512i data = _mm512_load_epi64((__m512i*)(src + i)); // Load 512 bits from the source
        _mm512_store_epi64((__m512i*)(dst + i), data);         // Store 512 bits into the destination
    }
    endTime=rdtsc();
    //printf("\t> Completed AVX-512 (64-bit) Mov Ins: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    
    valueCheck(src, dst, bufSize, "[AVX512_64] ");
    free(src);
    free(dst);

    return (endTime-startTime);
}

// ~~~~~~~~ AMX LD/ST Tiles ~~~~~~~~ //
uint64_t volatile single_AMX(uint64_t bufSize, bool primeCache){
    uint64_t startTime, endTime;
    uint16_t tilePartial;
    uint8_t rowPartial, tileCount, adjustedRows;
    uint8_t **srcAMX, **dstAMX;
    AMXtile tileCFG = {0};

    // Doubles as 'init AMX' while also 'assert() it initialized':
    detailedAssert(syscall_use_tile(),"AMX - Failed to enable AMX XFEATURE_XTILEDATA.");

    // Convert bufSize into digestible tile sizes of N (RowNum x 64B = 1024B) tiles
    tilePartial = bufSize % AMX_TILE_SIZE; // If tilePartial !=0, we must add one more tile with this number of bytes
    tileCount = ((bufSize / AMX_TILE_SIZE) + (tilePartial > 0)); // Should floor() by default since int / int, then add 1 if partial
    rowPartial = (tilePartial/AMX_COL_WIDTH)+(tilePartial%AMX_COL_WIDTH>0); // For the last tile we need, how many rows we need (out of AMX_MAX_ROWS)

    // Malloc src/dst locations
    srcAMX = malloc(tileCount * AMX_TILE_SIZE);
    dstAMX = malloc(tileCount * AMX_TILE_SIZE);
    for(int i=0; i<tileCount; i++)
    {
        srcAMX[i] = malloc(AMX_TILE_SIZE);
        dstAMX[i] = malloc(AMX_TILE_SIZE);
    }

    // ~~~ INITIALIZE TILE CONFIGS ~~~ //
    tileCFG.palette_id = 1;
    tileCFG.start_row = 0;
    for (int i=0; i<tileCount; i++)
    {
        //Try to make the final row for our src/dst, only 'rowPartial' 
        //NOTE: Remember the *actual* last tile I don't know what it is...
        if(i==tileCount-1 && tilePartial)
        {
            tileCFG.colsb[i] = AMX_COL_WIDTH;
            tileCFG.rows[i] =  rowPartial;
            continue;
        }

        tileCFG.colsb[i] = AMX_COL_WIDTH;
        tileCFG.rows[i] =  AMX_MAX_ROWS;
    }
    _tile_loadconfig(&tileCFG);
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

    // ~~~~ FILL SRC/DST TILES ~~~~~ //
    for(int i=0; i<tileCount; i++)
    {
        adjustedRows=AMX_MAX_ROWS;
        if(i==tileCount-1)
            adjustedRows=rowPartial;
        
        for(int j=0; j<adjustedRows; j++)
        {
            for(int k=0; k<AMX_COL_WIDTH; k++)
            {
                srcAMX[i][j*AMX_COL_WIDTH + k] = rand()%(100);
                dstAMX[i][j*AMX_COL_WIDTH + k] = 0;
            }
        }
    }
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // Either prime the cache with these, or flush them from cache
    // *SHOULD* already be cached, but do again anyways...
    if(primeCache)
    {
        for(int i=0; i<tileCount; i++)
        {
            adjustedRows=AMX_MAX_ROWS;
            if(i==tileCount-1)
                adjustedRows=rowPartial;
            
            for(int j=0; j<adjustedRows; j++)
            {
                for(int k=0; k<AMX_COL_WIDTH; k++)
                {
                    globalAgitator += srcAMX[i][j*AMX_COL_WIDTH + k];
                    globalAgitator += dstAMX[i][j*AMX_COL_WIDTH + k];
                    globalAgitator %= 128;
                }
            }
        }
        globalAgitator %= 20;
    }else{
        for(int i=0; i<tileCount; i++)
        {
            flush(srcAMX[i]);
            flush(dstAMX[i]);
        }
        flush(srcAMX);
        flush(dstAMX);
        cpuMFence();
    }

    // ~~~~ LOAD/ST TILES ('AMX Mem Mov') ~~~~~ //
    // NOTE: We HAVE to do it this way, because:
    // _tile_loadd (N, ... <-- N *cannot* be a variable.
    //
    // Load values into a tile
    startTime=rdtsc();
    _tile_loadd(0, srcAMX[0], AMX_STRIDE);
    if(tileCount >=2)
        _tile_loadd(1, srcAMX[1], AMX_STRIDE);
    if(tileCount >=3)
        _tile_loadd(2, srcAMX[2], AMX_STRIDE);
    if(tileCount >=4)
        _tile_loadd(3, srcAMX[3], AMX_STRIDE);

    // Store tile values into mem location
    _tile_stored(0, dstAMX[0], AMX_STRIDE);
    if(tileCount >=2)
        _tile_stored(1, dstAMX[1], AMX_STRIDE);
    if(tileCount >=3)
        _tile_stored(2, dstAMX[2], AMX_STRIDE);
    if(tileCount >=4)
        _tile_stored(3, dstAMX[3], AMX_STRIDE);
    endTime=rdtsc();
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

    // Check values for all tiles
    for(int i=0; i<tileCount; i++)
        valueCheck(srcAMX[i], dstAMX[i], AMX_TILE_SIZE, "[AMX Tile Ld/St] ");

    // Free all AMX stuff
    _tile_release();
    for(int i=0; i<tileCount; i++)
    {
        free(srcAMX[i]);
        free(dstAMX[i]);
    }
    free(srcAMX);
    free(dstAMX);

    return (endTime-startTime);
}

bool volatile syscall_use_tile()
{
    return (!syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~ Cache Flush Fns ~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// clflush
uint64_t volatile single_clflush(uint64_t bufSize, bool primeCache){
    uint64_t startTime, endTime;
    dst=malloc(bufSize);

    // Init arr, and pull all of it into cache:
    for (int i=0; i<bufSize; i++)
        dst[i] = rand()%(255);

    startTime = rdtsc();
    #pragma unroll
    for (int i=0; i<bufSize; i+=64) {
        asm volatile ("clflush 0(%0)\n"
            :
            : "c" (&dst[i])
            : "rax");
    }
    endTime = rdtsc();

    free(dst);
    return (endTime-startTime);
}

// clflushopt
uint64_t volatile single_clflushopt(uint64_t bufSize, bool primeCache){
    uint64_t startTime, endTime;
    dst=malloc(bufSize);
    
    // Init arr, and pull all of it into cache:
    for (int i=0; i<bufSize; i++)
        dst[i] = rand()%(255);

    startTime = rdtsc();
    #pragma unroll
    for (int i=0; i<bufSize; i+=64) {
        // Use inline assembly to flush the cache line containing dst[i].
        __asm__ volatile (
            "clflushopt (%0)"
            :
            : "r" (&dst[i])
            : "memory"
        );
    }
    endTime = rdtsc();
    
    free(dst);
    return (endTime-startTime);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~ Mem Cmp Fns ~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

uint64_t volatile single_Cmemcmp(uint64_t bufSize, bool primeCache){
    uint64_t startTime, endTime;
    uint8_t *src, *dst;

    src = malloc(bufSize);
    dst = malloc(bufSize);

    // Initialize the source and destination arrays
    for (size_t i = 0; i < bufSize; ++i) 
    {
        src[i] = rand() % 255;
        dst[i] = src[i];
    }

    // Flush the buffers
    if(!primeCache)
    {
        for (size_t i = 0; i<bufSize; i++) 
        {
            flush(&src[i]);
            flush(&dst[i]);
        }
            flush(src);
            flush(dst);
    }

    startTime = rdtsc();
    if(memcmp(src, dst, bufSize) == 0)
        {}
    else
        detailedAssert(false, "single_Cmemcmp() - Values mismatch! Compare failed.");
    endTime = rdtsc();
    
    free(src);
    free(dst);

    return (endTime - startTime);
}

uint64_t volatile single_SSE2cmp(uint64_t bufSize, bool primeCache){
    uint64_t startTime, endTime;
    uint16_t mSz = sizeof(__m128i);
    __m128i *srcSSE2Arr, *dstSSE2Arr;
    
    srcSSE2Arr = malloc(sizeof(__m128i) * (bufSize/mSz));
    dstSSE2Arr = malloc(sizeof(__m128i) * (bufSize/mSz));
    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(int i=0; i<bufSize; i++)
    {
        src[i] = rand()%(255);
        dst[i] = src[i];
    }

    // Convert the buffers:
    for (size_t i = 0; i<bufSize/mSz; i++) 
    {
        srcSSE2Arr[i] = _mm_loadu_si128((__m128i*)(src + (i*mSz)));
        dstSSE2Arr[i] = _mm_loadu_si128((__m128i*)(dst + (i*mSz)));
    }

    // Flush the buffers
    if(!primeCache)
    {
        for (size_t i = 0; i<bufSize/mSz; i++) 
        {
            flush(&srcSSE2Arr[i]);
            flush(&dstSSE2Arr[i]);
        }
        flush(srcSSE2Arr);
        flush(dstSSE2Arr);
    }

    startTime = rdtsc();
    for (size_t i=0; i<bufSize/mSz; i++)
    {   
        if (!_mm_test_all_ones(_mm_cmpeq_epi8(srcSSE2Arr[i],   dstSSE2Arr[i])))
        {
            detailedAssert(false, "single_SSE2cmp() - Values mismatch! Compare failed.");
            break;
        }
    }
    endTime = rdtsc();

    free(src);
    free(dst);
    return (endTime-startTime);
}

uint64_t volatile single_SSE4cmp(uint64_t bufSize, bool primeCache){
    int mask;
    uint16_t mSz = sizeof(__m128i);
    uint64_t startTime, endTime;
    __m128i cmp, *srcSSE4Arr, *dstSSE4Arr;
    
    srcSSE4Arr = malloc(mSz * (bufSize/mSz));
    dstSSE4Arr = malloc(mSz * (bufSize/mSz));

    src = malloc(bufSize);
    dst = malloc(bufSize);
    for(int i=0; i<bufSize; i++)
    {
        src[i] = rand()%(255);
        dst[i] = src[i];
    }

    // Convert the buffers:
    for (size_t i = 0; i<bufSize/16; i++) 
    {
        srcSSE4Arr[i] = _mm_loadu_si128((__m128i*)(src + (i*mSz)));
        dstSSE4Arr[i] = _mm_loadu_si128((__m128i*)(dst + (i*mSz)));
    }

    // Flush the buffers
    if(!primeCache)
    {
        for (size_t i = 0; i<bufSize/16; i++) 
        {
            flush(&srcSSE4Arr[i]);
            flush(&dstSSE4Arr[i]);
        }
            flush(srcSSE4Arr);
            flush(dstSSE4Arr);
    }

    startTime = rdtsc();
    for (size_t i=0; i<bufSize/mSz; i++)
    {
        cmp = _mm_cmpeq_epi64(srcSSE4Arr[i], dstSSE4Arr[i]);
        
        // Create mask to test if all bytes are 0xFFFF
        mask = _mm_movemask_epi8(cmp);

        if(mask != 0xFFFF)
        {
            detailedAssert(false, "single_SSE4cmp() - Values mismatch! Compare failed.");
            break;
        }
    }
    endTime = rdtsc();

    free(src);
    free(dst);
    return (endTime-startTime);
}

uint64_t volatile single_AVX256cmp(uint64_t bufSize, bool primeCache){
    int mask;
    uint16_t mSz = sizeof(__m256i);
    uint64_t startTime, endTime, randArr[4];
    __m256i srcAVXArr, dstAVXArr;

    // Init values and assign them to the AVX vectors
    for(int i=0; i<4; i++)
        randArr[i] = rand() % UINT64_MAXVAL;
    srcAVXArr = _mm256_set_epi64x(randArr[3],
        randArr[2],randArr[1],randArr[0]);
    dstAVXArr = _mm256_set_epi64x(randArr[3],
        randArr[2],randArr[1],randArr[0]);

    // Flush the buffers
    if(!primeCache)
    {
        flush(&srcAVXArr);
        flush(&dstAVXArr);
    }

    // Perform the comparison and measure the time taken
    startTime = rdtsc();
    for (size_t i=0; i<(bufSize/mSz); i++)
    {
        if (!_mm256_testc_si256(_mm256_cmpeq_epi8(srcAVXArr, dstAVXArr), _mm256_set1_epi32(-1)))
        {
            detailedAssert(false, "single_AVX256cmp() - Values mismatch! Compare failed.");
            break;
        }
    }
    endTime = rdtsc();

    return (endTime - startTime);
}

uint64_t volatile single_AVX512cmp(uint64_t bufSize, bool primeCache){
    uint64_t startTime, endTime;
    uint8_t *src, *dst, randArr[8];
    __m512i srcAVXArr, dstAVXArr;
    __mmask64 mask;
    size_t numElements = bufSize / sizeof(__m512i); // Number of 512-bit integers in the array

    // Init values and assign them to the AVX vectors
    for(int i=0; i<8; i++)
        randArr[i] = rand() % UINT64_MAXVAL;
    srcAVXArr = _mm512_set_epi64(
        randArr[7], randArr[6], randArr[5], randArr[4],
        randArr[3], randArr[2], randArr[1], randArr[0]
    );
    dstAVXArr = _mm512_set_epi64(
        randArr[7], randArr[6], randArr[5], randArr[4],
        randArr[3], randArr[2], randArr[1], randArr[0]
    );

    // Flush the buffers
    if(!primeCache)
    {
        flush(&srcAVXArr);
        flush(&dstAVXArr);
    }

    // Compare using AVX-512 instructions and measure the time
    startTime = rdtsc();
    for (size_t i = 0; i < numElements; i++)
    {
        mask = _mm512_cmpeq_epi8_mask(srcAVXArr, dstAVXArr); // Compare and get mask of results

        if (mask != 0xFFFFFFFFFFFFFFFF) {
            detailedAssert(false, "single_AVX512cmp() - Values mismatch! Compare failed.");
            break;
        }
    }
    endTime = rdtsc();

    return (endTime - startTime);
}