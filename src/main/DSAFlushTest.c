#include "DSAFlushTest.h"

int main(int argc, char *argv[]) {
    uint8_t switchIndex;
    srand(time(NULL));

    // Parse program inputs and do initial checks...
    detailedAssert((argc==4),"Main() - Not enough input arguments specified (./program bufferSize resfile mode).");
    detailedAssert((atoi(argv[3])>=0 && atoi(argv[3])<NUM_MODES),"Main() - 'Mode' Num Invalid.");
    mode = (modeEnum)atoi(argv[3]);
    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);
    detailedAssert((bufferSize%64==0),"Main() - Please specify bufferSize that is a factor of 64, and <= 4096."); //  && bufferSize<=4096
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
                    case 0: // clflush
                        resArr[clflushIndx] = single_clflush(bufferSize, mode);
                        cpuMFence();
                        break;
                        
                    case 1: // clflushopt
                        resArr[clflushoptIndx] = single_clflushopt(bufferSize, mode);
                        cpuMFence();
                        break;

                    case 2: // DSA flush op
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
                        free(dstDSA);
                        break;

                    default:
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
            // Other Tests first
            resArr[clflushIndx] = bulk_clflush(bufferSize);
            resArr[clflushoptIndx] = bulk_clflushopt(bufferSize);

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
                detailedAssert(((uint64_t)(totalDSATime+resArr[DSAFlushIndx]) > totalDSATime), "Main() - DSA_Op - PERF METRIC OVERFLOW ERR.");
                totalEnQTime += resArr[DSAenqIndx];
                totalDSATime += resArr[DSAFlushIndx];
                resArr[DSAenqIndx] = 0;
                resArr[DSAFlushIndx] = 0;
            }
            resArr[DSAenqIndx] = totalEnQTime/BULK_TEST_COUNT;
            resArr[DSAFlushIndx] = totalDSATime/BULK_TEST_COUNT;
            munmap(wq_portal, PORTAL_SIZE);
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
    detailedAssert((fd >= 0), "DSA Descriptor Init() failed opening WQ portal file from path.");

    // Chose DSA Instruction
    descr.opcode = DSA_OPCODE_CFLUSH;

    // Config Completion Record info
    compRec.status = 0;

    // Request Completion Record (so we can check when it is done)
    descr.flags = IDXD_OP_FLAG_RCR;
    descr.flags |= IDXD_OP_FLAG_CRAV;
    descr.completion_addr = (uintptr_t)&compRec;

    // Init srcDSA
    dstDSA = aligned_alloc(64, bufferSize);
    for(int i=0; i<bufferSize; i++)
        dstDSA[i] = rand()%(255);

    // Set packet info
    descr.xfer_size = bufferSize;
    descr.dst_addr  = (uintptr_t)dstDSA;

    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);
}

// Verify transfer, free the memory, and store perf counters
void finalizeDSA(){
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
    resArr[DSAFlushIndx] = serialLatency;
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
    int debug=0;
    while(compRec.status!=1){}
    endTimeDSA = rdtsc();
    startTimeDSA = endTimeEnQ;
    cpuMFence();

    //printf("DEBUG: endTimeEnQ Timestamp =         %lu\n", endTimeEnQ);
    //printf("DEBUG: endTimeEnQ elapsed time =         %lu\n", endTimeEnQ-startTimeEnQ);
    //printf("DEBUG: endTimeDSA elapsed time =         %lu\n", endTimeDSA-startTimeDSA);

    return ((endTimeEnQ-startTimeEnQ)>15000 || (endTimeDSA-startTimeDSA)>15000);
}

// Adjust measured time by avg observed overhead from RTDSC()
void adjustTimes(){
    for(int i=0; i<NUM_TESTS; i++)
        resArr[i] -= RTDSC_latency;

    if(resArr[DSAFlushIndx] > 1000000)
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