#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#pragma once

// Useful MACROS
#define ANTI_OPT __attribute__((always_inline)) inline volatile

// Util Fns
void  volatile  flush(void* p);
void  volatile  flush2(void* p, void* q);
void            detailedAssert(bool assertRes, const char* msg);
void            valueCheck(char* src, char* dst, uint64_t size);

// Fns that NEED to be inlined (cant be defined in c file if declared here)
uint64_t ANTI_OPT rdtscp(){
    uint32_t low, high;
    asm volatile ("RDTSCP\n\t"       // RDTSCP: Read Time Stamp Counter and Processor ID
                      "mov %%edx, %0\n\t"
                      "mov %%eax, %1\n\t"
                      : "=r" (high), "=r" (low)  // Outputs
                      :: "%rax", "%rbx", "%rcx", "%rdx"); // Clobbers
    return ((uint64_t)high << 32) | low;
}