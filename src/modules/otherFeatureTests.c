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
    // You have to initialize tileNeeded+1 -- I have NO idea why, didn't read manual (and sample
    // AMX code doesn't explain). I do know, if you only init tiles=tilesNeeded, it fails...
    tileCFG.palette_id = 1;
    tileCFG.start_row = 0;
    for (int i=0; i<tileCount+1; i++)
    {
        //Try to make the final row for our src/dst, only 'rowPartial' 
        //NOTE: Remember the *actual* last tile I don't know what it is...
        if(i==tileCount-1)
        {
            tileCFG.colsb[i] = AMX_COL_WIDTH;
            tileCFG.rows[i] =  rowPartial;
            continue;
        }

        tileCFG.colsb[i] = AMX_COL_WIDTH;
        tileCFG.rows[i] =  AMX_MAX_ROWS;
    }
    _tile_loadconfig (&tileCFG);
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

    // To be fair against other tests, flush
    for(int i=0; i<tileCount; i++)
    {
        flush(srcAMX[i]);
        flush(dstAMX[i]);
    }
    flush(srcAMX);
    flush(dstAMX);

    // ~~~~ LOAD/ST TILES ('AMX Mem Mov') ~~~~~ //
    // NOTE: We HAVE to do it this way, because:
    // _tile_loadd (N, ... <-- N *cannot* be a variable.

    // Load values into a tile
    _tile_loadd(1, srcAMX[0], AMX_STRIDE);
    if(tileCount >=2)
        _tile_loadd(2, srcAMX[1], AMX_STRIDE);
    if(tileCount >=3)
        _tile_loadd(3, srcAMX[2], AMX_STRIDE);
    if(tileCount >=4)
        _tile_loadd(4, srcAMX[3], AMX_STRIDE);

    // Store tile values into mem location
    _tile_stored(1, dstAMX[0], AMX_STRIDE);
    if(tileCount >=2)
        _tile_stored(2, dstAMX[1], AMX_STRIDE);
    if(tileCount >=3)
        _tile_stored(3, dstAMX[2], AMX_STRIDE);
    if(tileCount >=4)
        _tile_stored(4, dstAMX[3], AMX_STRIDE);
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

    // DEBUG PRINT FOR SRC/DST LOCATIONS:
    /*printf("\nAFTER...\n");
    printf("\nTest3Src[0]:\n\t");
    for(int i=0; i<tileCount; i++)
    {
        for(int j=0; j<AMX_MAX_ROWS; j++)
        {
            for(int k=0; k<AMX_COL_WIDTH; k++)
            {
                printf("%u ", srcAMX[i][j*AMX_COL_WIDTH+k]);
                if(srcAMX[i][j*AMX_COL_WIDTH+k]<10)
                    printf(" ");
                if(k!=0 && k%32==31)
                    printf("\n\t");
            }
        }
    }

    printf("\nTest3Dst[0]:\n\t");
    for(int i=0; i<tileCount; i++)
    {
        for(int j=0; j<AMX_MAX_ROWS; j++)
        {
            for(int k=0; k<AMX_COL_WIDTH; k++)
            {
                printf("%u ", dstAMX[i][j*AMX_COL_WIDTH+k]);
                if(dstAMX[i][j*AMX_COL_WIDTH+k]<10)
                    printf(" ");
                if(k!=0 && k%32==31)
                    printf("\n\t");
            }
        }
    }*/

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

    printf("DEBUG TEST -- AMX SUCCESS! (%lu)\n", (endTime-startTime));

    // Delete me
    abort();

    return (endTime-startTime);
}

bool volatile syscall_use_tile()
{
    return (!syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA));
}