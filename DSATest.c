#include "DSATest.h"

int main() {
    uint64_t startTime, endTime;
    srand(time(NULL));

    /*******************/
    flushReloadAttack();
    spectreV1Attack();
    flushFlushAttack();
    /*******************/

    /*/ Nonesense ///
    startTime = rdtsc();
    printf("Hello, World!\n");
    endTime = rdtsc() - startTime;
    printf("Timer Test 1: %lu\n", endTime);
    ///          /*/

    return 0;
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