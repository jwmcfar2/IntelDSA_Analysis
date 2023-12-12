#include "DSAMemMvTest.h"

int main(int argc, char *argv[]) {
    uint8_t switchIndex;
    srand(time(NULL));

    // Parse program inputs and do initial checks...
    detailedAssert((argc==4),"Main() - Not enough input arguments specified (./program bufferSize resfile mode).");
    detailedAssert((atoi(argv[3])>=0 && atoi(argv[3])<NUM_MODES),"Main() - 'Mode' Num Invalid.");
    mode = (modeEnum)atoi(argv[3]);
    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);
    detailedAssert((bufferSize%64==0 && bufferSize<=4096),"Main() - Please specify bufferSize that is a factor of 64, and <= 4096.");
    cpuMFence();

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    //~~~~~~~~~~~~ BEGIN TESTS ~~~~~~~~~~~~//
    switch (mode) 
    {
        case 0: // Run Mode = singleCold        //
        case 1: // Run Mode = singleCachePrimed // <- Note we allow switch to fall-through, since "mode" is used to set cache priming

            // Mix up the order of these to minimize optimizations 
            globalAgitator+=rand()%(31);
            switchIndex = globalAgitator%(NUM_TESTS-1); // (DSA EnQ / DSA Memcpy are grouped, so: NUM_TESTS-1)
            for(int i=0; i<NUM_TESTS-1; i++)
            {
                switch (switchIndex) 
                {
                    /*******************/
                    // Module/Baseline Fn Tests
                    case 0: // C Mem Cp
                        resArr[CmemIndx] = single_memcpyC(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 1: // ASM movq
                        resArr[ASMmovqIndx] = single_movqInsASM(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 2: // SSE1
                        resArr[SSE1Indx] = single_SSE1movaps(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 3: // SSE2
                        resArr[SSE2Indx] = single_SSE2movdqa(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 4: // SSE4
                        resArr[SSE4Indx] = single_SSE4movntdq(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 5: // AVX 256
                        resArr[AVX256Indx] = single_AVX256(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 6: // AVX 512-32
                        resArr[AVX5_32Indx] = single_AVX512_32(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 7: // AVX 512-64
                        resArr[AVX5_64Indx] = single_AVX512_64(bufferSize, mode);
                        cpuMFence();
                        break;
                    case 8: // AMX Ld/St Tile
                        resArr[AMXIndx] = single_AMX(bufferSize, mode);
                        cpuMFence();
                        break;

                    case 9: // DSA Mem cp
                        descriptorRetry=1;
                        while(descriptorRetry)
                        {
                            single_DSADescriptorInit();
                            compilerMFence();
                            descriptorRetry = enqcmd(wq_portal, &descr);
                            
                            // enqCMD failed - respawn checker thread...
                            if(descriptorRetry)
                            {
                                cpuMFence();
                                spawnNOPs(1);
                            }
                        }
                        
                        finalizeDSA();
                        munmap(wq_portal, PORTAL_SIZE);
                        free(srcDSA);
                        free(dstDSA);

                        break;

                    default:
                        printf("DEBUG: switchNum=%d\n", switchIndex);
                        detailedAssert(false, "Main(): Switch 0 - Invalid Index.");
                        break;
                }

                // Should only run each case once (DSA Load / Run are grouped).
                cpuMFence();
                switchIndex++;
                switchIndex %= (NUM_TESTS-1);
            }
            break;

        // We don't need such convoluted randomization for these tests,
        // Since we are looking at 'best case' over time - not 'cold miss'
        case 2: // Bulk Test
            uint64_t tempTime, totalTime=0;

            // Other Tests first
            resArr[CmemIndx] = bulk_memcpyC(bufferSize);
            resArr[ASMmovqIndx] = bulk_movqInsASM(bufferSize);
            resArr[SSE1Indx] = bulk_SSE1movaps(bufferSize);
            resArr[SSE2Indx] = bulk_SSE2movdqa(bufferSize);
            resArr[SSE4Indx] = bulk_SSE4movntdq(bufferSize);
            resArr[AVX256Indx] = bulk_AVX256(bufferSize);
            resArr[AVX5_32Indx] = bulk_AVX512_32(bufferSize);
            resArr[AVX5_64Indx] = bulk_AVX512_64(bufferSize);
            for(int i=0; i<BULK_TEST_COUNT; i++)
            {
                tempTime = single_AMX(bufferSize, false);
                detailedAssert(((uint64_t)(totalTime+tempTime)>totalTime), "bulk_AMX() - PERF METRIC OVERFLOW ERR.");
                totalTime += tempTime;
            }
            resArr[AMXIndx] = totalTime/BULK_TEST_COUNT;

            // Now do DSA Op
            uint64_t totalEnQTime=0;
            uint64_t totalDSATime=0;
            single_DSADescriptorInit();
            compilerMFence();
            for(int i=0; i<BULK_TEST_COUNT; i++)
            {
                descriptorRetry=1;
                while(descriptorRetry)
                {
                    descriptorRetry = enqcmd(wq_portal, &descr);
                    
                    // enqCMD failed - respawn checker thread...
                    if(descriptorRetry)
                    {   
                        cpuMFence();
                        spawnNOPs(1);
                    }
                }
                compilerMFence();
                finalizeDSA();
                detailedAssert(((uint64_t)(totalEnQTime+resArr[DSAenqIndx]) > totalEnQTime), "Main() - DSA_EnQ - PERF METRIC OVERFLOW ERR.");
                detailedAssert(((uint64_t)(totalDSATime+resArr[DSAmovIndx]) > totalDSATime), "Main() - DSA_Op - PERF METRIC OVERFLOW ERR.");
                totalEnQTime += resArr[DSAenqIndx];
                totalDSATime += resArr[DSAmovIndx];
                resArr[DSAenqIndx] = 0;
                resArr[DSAmovIndx] = 0;
            }
            resArr[DSAenqIndx] = totalEnQTime/BULK_TEST_COUNT;
            resArr[DSAmovIndx] = totalDSATime/BULK_TEST_COUNT;
            munmap(wq_portal, PORTAL_SIZE);
            free(srcDSA);
            free(dstDSA);

            break;

        default:
            printf("DEBUG: ARGV=%s, Mode=%d.\n", atoi(argv[3]), mode);
            printf("Mode Invalid, or unimplemented. Exiting...\n");
            return 1;
    }

    // Output Results
    parseResults(argv[2]);

    return 0;

    // DEBUG ONLY
    printf("\n%s", _headerStr_);
    for(int i=0; i<NUM_TESTS; i++)
        printf("   %lu    ", resArr[i]);
    printf("\n\n");

    return 0;
}

// Make new DSA Descriptor(s) (effectively 'job packets' to tell DSA what to do)
void single_DSADescriptorInit(){
    int fd;

    // WQ Path Check
    fd = open(wqPath, O_RDWR);
    detailedAssert((fd >= 0), "DSA Descriptor Init() - failed opening WQ portal file from path.");

    // Chose DSA Instruction
    descr.opcode = DSA_OPCODE_MEMMOVE;

    // Config Completion Record info
    compRec.status = 0;

    // Request Completion Record (so we can check when it is done)
    descr.flags = IDXD_OP_FLAG_RCR;
    descr.flags |= IDXD_OP_FLAG_CRAV;
    descr.completion_addr = (uintptr_t)&compRec;

    // Send writes to cache, not mem
    descr.flags |= IDXD_OP_FLAG_CC;

    // Init srcDSA
    srcDSA = aligned_alloc(64, bufferSize);
    dstDSA = aligned_alloc(64, bufferSize);
    for(int i=0; i<bufferSize; i++)
        srcDSA[i] = rand()%(255);

    // Set packet info
    descr.xfer_size = bufferSize;
    descr.src_addr  = (uintptr_t)srcDSA;
    descr.dst_addr  = (uintptr_t)dstDSA;
    
    uint8_t *descByteArr = (uint8_t *)&descr;
    size_t len = sizeof(descr);
    
    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);
}

// Verify transfer, free the memory, and store perf counters
void finalizeDSA(){
    valueCheck(srcDSA, dstDSA, bufferSize, "[DSATest] ");
    uint64_t serialLatency, pthreadLatency;

    serialLatency = (endTimeDSA-startTimeDSA);

    if(serialLatency > 1000000)
    {
        printf("ERROR - DETECTED SERIAL LATENCY THAT IS WAYYYY TO HIGH!");
        printf("EndTime   = Cycle: %lu\n", endTimeDSA);
        printf("StartTime = Cycle: %lu\n", startTimeDSA);
        abort();
    }

    resArr[DSAenqIndx]  = endTimeEnQ-startTimeEnQ;
    resArr[DSAmovIndx]  = serialLatency;
}

// Actual ASM functionality to send descriptor 
uint8_t enqcmd(void *_dest, const void *_src){
    uint8_t retry;
    cpuMFence();
    startTimeEnQ = rdtsc();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    endTimeEnQ = rdtsc();
    while(compRec.status!=1){}
    endTimeDSA = rdtsc();
    startTimeDSA = endTimeEnQ;
    cpuMFence();

    //printf("DEBUG: endTimeEnQ Timestamp =         %lu\n", endTimeEnQ);
    //printf("DEBUG: endTimeEnQ elapsed time =         %lu\n", endTimeDSA-startTimeDSA);

    return ((endTimeEnQ-startTimeEnQ)>15000 || (endTimeDSA-startTimeDSA)>15000);
}

// Adjust measured time by avg observed overhead from RTDSC()
void adjustTimes(){
    for(int i=0; i<NUM_TESTS; i++)
        resArr[i] -= RTDSC_latency;

    if(resArr[DSAmovIndx] > 1000000)
    {
        printf("ERROR - DETECTED ~ADJUSTED LATENCY~ THAT IS WAYYYY TO HIGH!");
        printf("endTimeDSA-startTimeDSA = %lu, RTDSC_latency = %lu\n", endTimeDSA-startTimeDSA, RTDSC_latency);
        abort();
    }
}

void parseResults(char* fileName){
    char cmdStr[256];

    // Open the file in read mode to check if it exists
    FILE *file = fopen(fileName, "r");

    // File doesn't exist, create it and write the header
    if (file == NULL)
    {
        file = fopen(fileName, "w");
        detailedAssert((file != NULL), "parseResults() - Could not open or create resFile.");
        fputs(_headerStr_, file);
        fclose(file);
        printf("\tCreated new resFile: %s\n", fileName);
    } else {
        // File already exists, just close it without modifying - will append later
        fclose(file);
    }

    // Open the file in append mode and add the new line of data
    file = fopen(fileName, "a");
    detailedAssert((file != NULL), "parseResults() - Could not open resFile to append data.");

    for (int i=0; i<NUM_TESTS; i++)
        fprintf(file, "   %lu    ", resArr[i]);
    
    fprintf(file, "\n");

    fclose(file);

    // Now, straighten out all the columns:
    system("touch results/.dsaTemp");
    snprintf(cmdStr, sizeof(cmdStr), "column -t %s > results/.dsaTemp", fileName);
    system(cmdStr);
    snprintf(cmdStr, sizeof(cmdStr), "mv results/.dsaTemp %s", fileName);
    system(cmdStr);
}

// Caused WAY too many issues - for infrequent cases of smaller observed latency
void* checkerThreadFn(void* args)
{
    endTimePThread=0;
    unsigned cycles_high, cycles_low;

    threadStarted=true;
    cpuMFence();

    while(compRec.status != 1){ }

    endTimePThread=rdtsc();
    //printf("Checker thread completed! Timestamp = %lu || status=0x%x\n", endTimePThread, compRec.status);//failCount);
}