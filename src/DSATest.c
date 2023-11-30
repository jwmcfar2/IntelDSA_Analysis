#include "DSATest.h"

int main() {
    uint64_t startTime, endTime;
    srand(time(NULL));

    /*******************/
    DSADescriptorInit();
    /*******************/

    /*/ Nonesense ///
    startTime = rdtsc();
    printf("Hello, World!\n");
    endTime = rdtsc() - startTime;
    printf("Timer Test 1: %lu\n", endTime);
    ///          /*/

    return 0;
}

// Make new DSA Descriptor(s)
void DSADescriptorInit() {
    struct dsa_completion_record comp __attribute__((aligned(32)));
    struct dsa_hw_desc desc = { };
    desc.opcode = DSA_OPCODE_MEMMOVE;
    /*
    * Request a completion – since we poll on status, this flag
    * must be 1 for status to be updated on successful
    * completion
    */
    desc.flags = IDXD_OP_FLAG_RCR;
    /* CRAV should be 1 since RCR = 1 */
    desc.flags |= IDXD_OP_FLAG_CRAV;
    /* Hint to direct data writes to CPU cache */
    desc.flags |= IDXD_OP_FLAG_CC;
    desc.xfer_size = BLEN;
    desc.src_addr = (uintptr_t)src;
    desc.dst_addr = (uintptr_t)dst;
    comp.status = 0;
    desc.completion_addr = (uintptr_t)&comp;
}

// Modified version of Intel Documentation for C Inlining
// Source: How to Benchmark Code Execution Times on Intel®
// IA-32 and IA-64 Instruction Set Architectures (whitepaper)
uint64_t rdtsc() {
    unsigned cycles_high, cycles_low;   

    asm volatile ("CPUID\n\t"
    "RDTSC\n\t"
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
    "%rax", "%rbx", "%rcx", "%rdx");

    return (((uint64_t)cycles_high << 32) | cycles_low);
}