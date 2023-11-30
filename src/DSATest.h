#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <x86intrin.h>

// Function List
void DSADescriptorInit();
uint64_t rdtsc();

void flush(void* p) {
    asm volatile ("clflush 0(%0)\n"
        :
        : "c" (p)
        : "rax");
} // [1]