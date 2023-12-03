#include "DSATest.h"

int main(int runCount) {
    srand(time(NULL));

    /*******************/
    // Module/Baseline Fn Tests
    single_memcpyC(BUF_SIZE);
    single_movqInsASM(BUF_SIZE);
    single_SSE1movaps(BUF_SIZE);
    single_SSE2movdqa(BUF_SIZE);
    single_SSE4movntdq(BUF_SIZE);
    single_AVX256(BUF_SIZE);
    single_AVX512_32(BUF_SIZE);
    single_AVX512_64(BUF_SIZE);

    // Intel DSA Fns
    single_DSADescriptorInit();
    single_DSADescriptorSubmit();
    single_DSACompletionCheck();
    /*******************/

    return 0;
}

// Make new DSA Descriptor(s) (effectively 'job packets' to tell DSA what to do)
void ANTI_OPT single_DSADescriptorInit() {
    printf("\n || Test Latency of DSA mem mov\n");
    int i, fd;
    
    // WQ Path Check
    fd = open(wqPath, O_RDWR);
    detailedAssert((fd >= 0), "DSA Descriptor Init() failed opening WQ portal file.");

    // Config Completion Record info
    compRec.status = 0;

    // Request Completion Record (so we can check when it is done)
    descr.flags = IDXD_OP_FLAG_RCR;
    descr.flags |= IDXD_OP_FLAG_CRAV;
    descr.completion_addr = (uintptr_t)&compRec;

    // Send writes to cache, not mem
    descr.flags |= IDXD_OP_FLAG_CC;

    // Init srcDSA
    srcDSA = aligned_alloc(64, BUF_SIZE);
    dstDSA = aligned_alloc(64, BUF_SIZE);
    for(i=0; i<BUF_SIZE; i++)
        srcDSA[i] = rand()%(255);

    // Set packet info
    descr.xfer_size = BUF_SIZE;
    descr.src_addr = (uintptr_t)srcDSA;
    descr.dst_addr = (uintptr_t)dstDSA;

    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    close(fd);
}

// Send the Descriptors to DSA
void ANTI_OPT single_DSADescriptorSubmit(){
    uint8_t debug=99;
    // NOTE: user guide recommends wrapping enqcmd() in while() loop until it succeeds
    // but I never saw it fail, and checking will affect elapsed cycles for submission

    // Fence to ensure prev writes are ordered wrt enqcmd()
    _mm_sfence();
    debug = enqcmd(wq_portal, &descr);
    printf("Debug: enqcmd res = %u\n", debug);
}

// When the DSA accel is complete, it updates the completion record. We must keep checking this...
void ANTI_OPT single_DSACompletionCheck(){
    uint32_t i, maxCount=1000000;
    uint64_t startTime, endTime;

    startTime = rdtscp();
    for(i=0; i<maxCount; i++)
    {
        if(compRec.status == 0)
            continue;

        endTime = rdtscp();
        printf("\tVerified completed DSA instruction: Total cycles elapsed = %lu cycles.\n\n", endTime-startTime);
        munmap(wq_portal, PORTAL_SIZE);
        valueCheck(srcDSA, dstDSA, BUF_SIZE);
        free(srcDSA);
        free(dstDSA);
        return;
    }
    
    detailedAssert(false, "DSA Completion Check: Failed to complete Descriptor in the alloted time.");
}

// Actual ASM functionality to send descriptor 
uint8_t ANTI_OPT enqcmd(void *_dest, const void *_src){
    uint64_t startTime, endTime; 
    uint8_t retry;
    
    startTime = rdtscp();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    startTime = rdtscp();
    printf("\t> Completed DSA enqueue instruction *attempt*: Cycles elapsed = %lu cycles.\n", rdtscp()-startTime);
    printf("    (Note: Accurate cycle count likely needs concurrent thread checking completion record)\n");

    return retry;
}