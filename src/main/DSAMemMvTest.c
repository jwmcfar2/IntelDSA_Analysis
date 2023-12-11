#include "DSAMemMvTest.h"

int main(int argc, char *argv[]) {
    pthread_t checkerThread;
    uint8_t switchIndex;
    srand(time(NULL));

    // Parse program inputs and do initial checks...
    detailedAssert((argc==4),"Main() - Not enough input arguments specified (./program bufferSize resfile mode).");
    detailedAssert((atoi(argv[3])>=0 && atoi(argv[3])<NUM_MODES),"Main() - 'Mode' Num Invalid.");
    mode = (modeEnum)atoi(argv[3]);
    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);
    detailedAssert((bufferSize%64==0 && bufferSize<=4096),"Main() - Please specify bufferSize that is a factor of 64, and <= 4096.");
 
    // Profile RDTSC latency overhead
    profileRDTSC();

    // Spawn DSA checker thread...
    compRec.status=0;
    compilerMFence();
    spawnThreadOnSiblingCore(&checkerThread, checkerThreadFn);
    while(!threadStarted){
        spawnNOPs(1);
    }
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
                                compRec.status=0;
                                threadStarted=0;
                                pthread_join(checkerThread, NULL);
                                spawnThreadOnSiblingCore(&checkerThread, checkerThreadFn);
                                while(!threadStarted){
                                    spawnNOPs(1);
                                }
                                cpuMFence();
                            }
                        }
                        
                        finalizeDSA();
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

        default:
            printf("DEBUG: ARGV=%s, Mode=%d.\n", atoi(argv[3]), mode);
            printf("Mode Invalid, or unimplemented. Exiting...\n");
            return 1;
    }

    // Output Results
    adjustTimes();
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
    
    printf("\nBatch Queue Descriptor Byte Array: \n");
    for(int i=0; i<64; i+=8)
    {
        for(int j=7; j>=0; j--)
        {
            printf("%lu\t", descByteArr[i+j]);
        }
        printf("\n");
    }

    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);
}

// Verify transfer, free the memory, and store perf counters
void finalizeDSA(){
    valueCheck(srcDSA, dstDSA, bufferSize, "[DSATest] ");
    uint64_t serialLatency, pthreadLatency;

    munmap(wq_portal, PORTAL_SIZE);
    free(srcDSA);
    free(dstDSA);

    serialLatency = (endTimeDSA-startTimeDSA);
    pthreadLatency = (endTimePThread-startTimeDSA);
    //printf("DEBUG: pthread lat = %lu cycs, serial lat= %lu cycs\n", pthreadLatency, serialLatency);

    resArr[DSAenqIndx]  = endTimeEnQ-startTimeEnQ;
    resArr[DSAmovIndx] = (pthreadLatency < serialLatency)? pthreadLatency : serialLatency;
}

// Actual ASM functionality to send descriptor 
uint8_t enqcmd(void *_dest, const void *_src){
    uint8_t retry;
    startTimeEnQ = rdtsc();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    endTimeEnQ = rdtsc();
    while(compRec.status!=1){}
    endTimeDSA = rdtsc();
    startTimeDSA = endTimeEnQ;

    //printf("DEBUG: endTimeEnQ Timestamp =         %lu\n", endTimeEnQ);
    //printf("DEBUG: endTimeEnQ elapsed time =         %lu\n", endTimeDSA-startTimeDSA);

    return ((endTimeEnQ-startTimeEnQ)>15000 || (endTimeDSA-startTimeDSA)>15000);
}

// Adjust measured time by avg observed overhead from RTDSC()
void adjustTimes(){
    for(int i=0; i<NUM_TESTS; i++)
        resArr[i] -= RTDSC_latency;
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