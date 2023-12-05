#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#pragma once

// Useful MACROS
#define ANTI_OPT __attribute__((always_inline)) inline volatile
#define PAGE_SIZE 4096
#define LL_NODE_PADDING (PAGE_SIZE - sizeof(void*))
#define UINT32_MAX 4294967295
#define L1I_KB 32
#define L1D_KB 64
#define L2_KB 2048
#define L3_KB 53760

// System Profile Vars
//uint64_t Cold_Miss_Latency=0;
uint64_t L1d_Miss_Latency=0;
//uint64_t L2_Miss_Latency=0;
//uint64_t L3_Miss_Latency=0;
uint64_t L1TLB_Miss_Latency=0;
//uint64_t L2TLB_Miss_Latency=0;

uint64_t L1TLB_Entries=0;
//uint64_t L2TLB_Entries=0;

// Util Fns
void  volatile  flush(void* p);
void  volatile  flush2(void* p, void* q);
void            detailedAssert(bool assertRes, const char* msg);
void            valueCheck(char* src, char* dst, uint64_t size, char* errDetails);

// LL Entry Structure
typedef struct ListNode {
    struct ListNode* next;
    char bytes[LL_NODE_PADDING];
} ListNode;

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