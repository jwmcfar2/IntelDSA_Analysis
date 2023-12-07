#include "DSATest.h"

int main(int argc, char *argv[]) {
    uint8_t switchIndex;
    pthread_t checkerThread, submitThread;
    srand(time(NULL));

    // Parse program inputs and do initial checks...
    detailedAssert((argc==4),"Main() - Not enough input arguments specified (./program bufferSize resfile mode).");
    detailedAssert((atoi(argv[3])>=0 && atoi(argv[3])<NUM_MODES),"Main() - 'Mode' Num Invalid.");
    mode = (modeEnum)atoi(argv[3]);
    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);
    detailedAssert((bufferSize%64==0 && bufferSize<=4096),"Main() - Please specify bufferSize that is a factor of 64, and <= 4096.");

    // Clear Caches and TLB
    //floodHelperFn();
    //ts.tv_sec = 0;           // 0 seconds
    //ts.tv_nsec = 10;         // 10 nanoseconds
    //checkThreadStarted = 0;
    //submitThreadStarted = 0;
    //finishedSubmission = 0;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    //~~~~~~~~~~~~ BEGIN TESTS ~~~~~~~~~~~~//
    switch (mode) 
    {
        case 0: // Run Mode = singleCold        //
        case 1: // Run Mode = singleCachePrimed // <- Note we allow switch to fall-through, since "mode" is used to set cache priming

            // Mix up the order of these to minimize optimizations 
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
                        // Init Mutex for DSA Checker
                        //DEBUG_INIT_TIME = rdtscp();
                        descriptorRetry=1;
                        while(descriptorRetry)
                        {
                            single_DSADescriptorInit();
                            //pthread_create(&checkerThread, NULL, single_DSACheckerThread, NULL);
                            
                            // Wait for pthread to actually start...
                            //usleep(500); // 0.5 ms delay
                            //nanosleep(&ts, NULL); // Sleep for ~10-30 cycles
                            //clock_nanosleep(CLOCK_MONOTONIC_RAW, 0, &ts, NULL); // sleep for 10ns
                            //pthread_mutex_lock(&mutex);

                            //pthread_create(&submitThread, NULL, single_DSASubmitThread, NULL);

                            //while (!threadStarted) {}
                            //    DEBUG_MAIN_WAIT++;
                            //    pthread_cond_wait(&cond, &mutex);
                            //}
                            //pthread_mutex_unlock(&mutex);
                            //DEBUG_BEGIN_TIME = rdtscp();
                            compilerMemFence();
                            //single_DSADescriptorSubmit();
                            descriptorRetry = enqcmd(wq_portal, &descr);
                        }
                        //printf("\t\t>>DEBUG Timestamp START - %lu\n", (DEBUG_INIT_TIME));
                        //printf("\t\t>>DEBUG Timestamp END - %lu\n", (DEBUG_BEGIN_TIME));
                        //printf("\t\t\t>>>DEBUG Time ELAPSED - %lu\n", (DEBUG_BEGIN_TIME-DEBUG_INIT_TIME));
                        
                        // Send DSA Job to Queue, then wait for thread to see its completion
                        //single_DSADescriptorSubmit();
                        //pthread_join(checkerThread, NULL);
                        //pthread_join(submitThread, NULL);
                        //pthread_mutex_destroy(&lock);
                        finalizeDSA();
                        //threadStarted = 0;
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
    //printf("\n || Test Latency of DSA mem mov\n");
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

// Send the Descriptors to DSA
// DEPRECATED -- DELETE ME
//void ANTI_OPT single_DSADescriptorSubmit(){
    // NOTE: user guide recommends wrapping enqcmd() in while() loop until it succeeds
    // but I never saw it fail, and checking will affect elapsed cycles for submission

    // Fence to ensure prev writes are ordered wrt enqcmd()
//    _mm_sfence();
    //printf("\t>DEBUG Timestamp SUBMIT START - %lu\n", (rdtscp()));
    //enqcmd(wq_portal, &descr);
//}

/*void* single_DSASubmitThread(void* arg){
    // Wait until checker thread has actually started
    DEBUG_TIMESTAMP_SUBMITTHREAD = rdtscp();
    while (1) {
        pthread_mutex_lock(&lock);
        if (checkThreadStarted) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
        //usleep(1); // Sleep for 1us
    }

    enqcmd(wq_portal, &descr);
}*/

/*void* single_DSACheckerThread(void* arg){
    uint32_t maxCount=10000000;
    //printf("\t>DEBUG Timestamp THREADSTART - %lu\n", (rdtscp()));
    //pthread_mutex_lock(&mutex);
    //threadStarted = true;
    //pthread_cond_signal(&cond);
    //pthread_mutex_unlock(&mutex);

    DEBUG_TIMESTAMP_CHECKTHREAD = rdtscp();

    pthread_mutex_lock(&lock);
    checkThreadStarted = 1;
    pthread_mutex_unlock(&lock);

    while (1) {
        pthread_mutex_lock(&lock);
        if (submitThreadStarted) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
        //usleep(1); // Sleep for 1us
    }

    //#pragma unroll
    for(int i=0; i<maxCount; i++)
    {
        DEBUG_THREAD_WAIT++;
        //pthread_mutex_lock(&lock);
        // Keep trying until we see it is ready
        if(compRec.status == 0)
        {
            //pthread_mutex_unlock(&lock);
            //nanosleep(&ts, NULL); // Sleep for ~10-30 cycles
            continue;
        }
        //pthread_mutex_unlock(&lock);
        DEBUG_SEENLOCK = rdtscp();
        while (1) {
            pthread_mutex_lock(&lock);
            if (finishedSubmission) {
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);
            //usleep(1); // Sleep for 1us
        }
        endTimeDSA = rdtscp();

        if((compRec.status & 3) == 3)
        {
            printf("\nERROR: Failed due to 'partial completion' from a page fault... I think? (Status & 0x3 = 0x%X)\n", (compRec.status & 3));
            printf("If you wish to exceed page size (usually 4KB), look more into spec - or send separate descriptors\n\n");
            detailedAssert(false, "DSA Completion - Failed to complete transfer.");
        }

        if(i == maxCount-1)
            printf("\nERROR: Failed - never saw completed DSA record!\n");

        break;
    }
    return NULL;
}*/

// Verify transfer, free the memory, and store perf counters
void ANTI_OPT finalizeDSA(){
    valueCheck(srcDSA, dstDSA, bufferSize, "[DSATest] ");

    munmap(wq_portal, PORTAL_SIZE);
    free(srcDSA);
    free(dstDSA);

    resArr[DSAenqIndx]  = endTimeEnQ-startTimeEnQ;
    resArr[DSAmovRIndx] = endTimeDSA-startTimeDSA;

    // Print DEBUG logs:
    //printf("DEBUG: checkThreadLat=%lu, subFinishLat=%lu\n", resArr[DSAmovRIndx], (DEBUG_SUBMITFINISHCLOCK-endTimeEnQ)-(DEBUG_ENDLOCK-DEBUG_STARTLOCK));

    // DEBUG Catch for Mutex Issues
    /*if(resArr[DSAmovRIndx] > 10000)// || endTimeEnQ-startTimeEnQ > 40000)
    {
    
        printf("\nDEBUG - FAILED TO PERFORM DSA OP IN REASONABLE TIME (%lu)!\n", resArr[DSAmovRIndx]);
        printf("DEBUG - DEBUG_MAIN_WAIT = (%lu)!\n", DEBUG_MAIN_WAIT);
        printf("DEBUG - DEBUG_THREAD_WAIT = (%lu)!\n", DEBUG_THREAD_WAIT);
        printf("DEBUG - Time it took from init to start = (%lu)!\n", DEBUG_BEGIN_TIME - DEBUG_INIT_TIME);
        printf("DEBUG - Timestamp for end   DSA = (%lu)!\n", endTimeDSA);
        printf("DEBUG - Timestamp for start DSA = (%lu)!\n", startTimeDSA);
        printf("DEBUG - Timestamp for start Check  = (%lu)!\n", DEBUG_TIMESTAMP_CHECKTHREAD);
        printf("DEBUG - Timestamp for start Submit = (%lu)!\n", DEBUG_TIMESTAMP_SUBMITTHREAD);
        printf("DEBUG - Timestamp for start Lock  = (%lu)!\n", DEBUG_STARTLOCK);
        printf("DEBUG - Timestamp for end Lock    = (%lu)!\n", DEBUG_ENDLOCK);
        printf("DEBUG - Time it took to lock = (%lu)!\n", DEBUG_ENDLOCK - DEBUG_STARTLOCK);
        printf("DEBUG - Timestamp for checker to see Lock = (%lu)!\n", DEBUG_SEENLOCK);
        printf("DEBUG - Timestamp for              Unlock = (%lu)!\n", DEBUG_ENDLOCK);
        printf("DEBUG - Time it took checker to progress = (%lu)!\n", (DEBUG_SEENLOCK > DEBUG_ENDLOCK)? 0 : DEBUG_ENDLOCK-DEBUG_SEENLOCK);
        printf("DEBUG - compRec.status = (%d)!\n-------\n\n", compRec.status);
        
        abort();
    }*/
}

// Actual ASM functionality to send descriptor 
uint8_t ANTI_OPT enqcmd(void *_dest, const void *_src){
    uint8_t retry;
    //pthread_mutex_lock(&lock);
    //submitThreadStarted = 1;
    //pthread_mutex_unlock(&lock);

    //pthread_mutex_lock(&lock);
    //compilerMemFence();
    startTimeEnQ = rdtscp();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    endTimeEnQ = rdtscp();
    //DEBUG_STARTLOCK = rdtscp();
    //pthread_mutex_lock(&lock);
    //finishedSubmission = 1;
    //pthread_mutex_unlock(&lock);
    //DEBUG_ENDLOCK = rdtscp();

    //compilerMemFence();
    while(compRec.status!=1){}
    endTimeDSA = rdtscp();
    //pthread_mutex_unlock(&lock);
    startTimeDSA = endTimeEnQ;
    // DEBUG Catch for Mutex Issues
    //if((endTimeEnQ-startTimeEnQ) > 40000)
    //{
    //    printf("DEBUG - FAILED TO PERFORM DSA ENQ IN TIME (%lu)!\n", endTimeEnQ-startTimeEnQ);
        //abort();
    //}

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
        printf("\n\tCreated new resFile: %s\n\n", fileName);
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