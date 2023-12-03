#include "DSATest.h"

int main(int argc, char *argv[]) {
    detailedAssert((argc==3),"Main() - Not enough input arguments specified (./program bufferSize resfile).");
    srand(time(NULL));

    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);
    detailedAssert((bufferSize%64==0),"Main() - Please specify bufferSize that is a factor of 64.");

    /*******************/
    // Module/Baseline Fn Tests
    resArr[CmemIndx] = single_memcpyC(bufferSize);
    resArr[ASMmovqIndx] = single_movqInsASM(bufferSize);
    resArr[SSE1Indx] = single_SSE1movaps(bufferSize);
    resArr[SSE2Indx] = single_SSE2movdqa(bufferSize);
    resArr[SSE4Indx] = single_SSE4movntdq(bufferSize);
    resArr[AVX256Indx] = single_AVX256(bufferSize);
    resArr[AVX5_32Indx] = single_AVX512_32(bufferSize);
    resArr[AVX5_64Indx] = single_AVX512_64(bufferSize);

    // Intel DSA Fns
    single_DSADescriptorInit();
    single_DSADescriptorSubmit();
    single_DSACompletionCheck();
    //single_DSADescriptorSubmit();
    //single_DSACompletionCheck();
    /*******************/

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
void ANTI_OPT single_DSADescriptorInit() {
    //printf("\n || Test Latency of DSA mem mov\n");
    int fd;

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
    descr.src_addr = (uintptr_t)srcDSA;
    descr.dst_addr = (uintptr_t)dstDSA;

    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);
}

// Send the Descriptors to DSA
void ANTI_OPT single_DSADescriptorSubmit(){
    uint8_t debug=99;
    // NOTE: user guide recommends wrapping enqcmd() in while() loop until it succeeds
    // but I never saw it fail, and checking will affect elapsed cycles for submission

    // Fence to ensure prev writes are ordered wrt enqcmd()
    _mm_sfence();
    enqcmd(wq_portal, &descr);
    //debug = enqcmd(wq_portal, &descr);
    //printf("Debug: enqcmd res = %u\n", debug);
}

// When the DSA accel is complete, it updates the completion record. We must keep checking this...
void ANTI_OPT single_DSACompletionCheck(){
    uint32_t i, maxCount=1000000;
    uint64_t startTime, endTime;
    uint64_t failCount=0;

    startTime = rdtscp();
    for(int i=0; i<maxCount; i++)
    {
        if(compRec.status == 0)
        {
            failCount++;
            continue;
        }

        endTimeDSA = rdtscp();
        endTime = rdtscp();
        //printf("\tVerified completed DSA instruction: Total cycles elapsed = %lu cycles.\n", endTime-startTime);
        //printf("  NOTE: DOUBLE CHECK TIMINGS OF START/END FOR DSA - MAY NEED TWO THREADS!! (1 submitting, 1 checking).\n");
        munmap(wq_portal, PORTAL_SIZE);
        valueCheck(srcDSA, dstDSA, bufferSize);
        free(srcDSA);
        free(dstDSA);

        //printf("\t> Completed DSA enqueue instruction *attempt*: Cycles elapsed = %lu cycles.\n", endTimeEnQ-startTimeEnQ);
        //printf("\tDSA_DEBUG: Total cycles elapsed from enQ = %lu cycles.(FailCount = %lu)\n", endTimeDSA-startTimeDSA, failCount);
        resArr[DSAenqIndx] = endTimeEnQ-startTimeEnQ;
        resArr[DSAmovRIndx] = endTimeDSA-startTimeDSA;
        return;
    }
    
    detailedAssert(false, "DSA Completion Check: Failed to complete Descriptor in the alloted time.");
}

// Actual ASM functionality to send descriptor 
uint8_t ANTI_OPT enqcmd(void *_dest, const void *_src){
    uint64_t startTime, endTime; 
    uint8_t retry;
    
    startTimeEnQ = rdtscp();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    endTimeEnQ = rdtscp();   
    //printf("\t> Completed DSA enqueue instruction *attempt*: Cycles elapsed = %lu cycles.\n", endTime-startTime);
    startTimeDSA = rdtscp();
 
    return retry;
}

void parseResults(char* fileName){

    // Open the file in read mode to check if it exists
    FILE *file = fopen(fileName, "r");

    // File doesn't exist, create it and write the header
    if (file == NULL) {
        file = fopen(fileName, "w");
        detailedAssert((file != NULL), "parseResults() - Could not open or create resFile.");
        fputs(_headerStr_, file);  // Write the header to the new file
        fclose(file);                 // Close the file after writing the header
        printf("\n\tCreated new resFile: %s\n\n", fileName);
    } else {
        // File already exists, just close it without modifying - will append later
        fclose(file);
    }

    // Open the file in append mode and add the new line of data
    file = fopen(fileName, "a");
    detailedAssert((file != NULL), "parseResults() - Could not open resFile to append data.");
    //fputs(dataLine, file);      // Append the new line of data

    for (int i=0; i<NUM_TESTS; i++) {
        fprintf(file, "   %lu    ", resArr[i]); // Adjust the format specifier if needed
    }
    fprintf(file, "\n"); // Append a newline character at the end

    fclose(file);               // Close the file after appending
}