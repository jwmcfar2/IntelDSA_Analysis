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

    // Try to Clear Caches and TLB
    floodHelperFn();

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
                        break;
                    case 1: // ASM movq
                        resArr[ASMmovqIndx] = single_movqInsASM(bufferSize, mode);
                        break;
                    case 2: // SSE1
                        resArr[SSE1Indx] = single_SSE1movaps(bufferSize, mode);
                        break;
                    case 3: // SSE2
                        resArr[SSE2Indx] = single_SSE2movdqa(bufferSize, mode);
                        break;
                    case 4: // SSE4
                        resArr[SSE4Indx] = single_SSE4movntdq(bufferSize, mode);
                        break;
                    case 5: // AVX 256
                        resArr[AVX256Indx] = single_AVX256(bufferSize, mode);
                        break;
                    case 6: // AVX 512-32
                        resArr[AVX5_32Indx] = single_AVX512_32(bufferSize, mode);
                        break;
                    case 7: // AVX 512-64
                        resArr[AVX5_64Indx] = single_AVX512_64(bufferSize, mode);
                        break;
                    case 8: // AMX Ld/St Tile
                        resArr[AMXIndx] = single_AMX(bufferSize, mode);
                        break;

                    case 9: // DSA Mem cp
                        descriptorRetry=1;
                        while(descriptorRetry)
                        {
                            single_DSADescriptorInit();
                            compilerMemFence();
                            descriptorRetry = enqcmd(wq_portal, &descr);
                        }
                        
                        finalizeDSA();
                        break;

                    default:
                        printf("DEBUG: switchNum=%d\n", switchIndex);
                        detailedAssert(false, "Main(): Switch 0 - Invalid Index.");
                        break;
                }

                // Should only run each case once (DSA Load / Run are grouped).
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
void ANTI_OPT single_DSADescriptorInit(){
    int fd;

    // Try and pull global perf counters into cache:
    startTimeDSA = rand()%(1024);
    endTimeDSA   = rand()%(255);
    startTimeEnQ = rand()%(1024);
    endTimeEnQ   = rand()%(255);

    // WQ Path Check
    fd = open(wqPath, O_RDWR);
    detailedAssert((fd >= 0), "DSA Descriptor Init() failed opening WQ portal file from path.");

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

    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);
}

// Verify transfer, free the memory, and store perf counters
void ANTI_OPT finalizeDSA(){
    valueCheck(srcDSA, dstDSA, bufferSize, "[DSATest] ");

    munmap(wq_portal, PORTAL_SIZE);
    free(srcDSA);
    free(dstDSA);

    resArr[DSAenqIndx]  = endTimeEnQ-startTimeEnQ;
    resArr[DSAmovRIndx] = endTimeDSA-startTimeDSA;
}

// Actual ASM functionality to send descriptor 
uint8_t ANTI_OPT enqcmd(void *_dest, const void *_src){
    uint8_t retry;
    startTimeEnQ = rdtscp();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    endTimeEnQ = rdtscp();
    while(compRec.status!=1){}
    endTimeDSA = rdtscp();
    startTimeDSA = endTimeEnQ;

    return ((endTimeEnQ-startTimeEnQ)>10000 || endTimeDSA-startTimeDSA>7500);
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