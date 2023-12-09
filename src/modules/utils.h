#define _GNU_SOURCE // Must be defined before any #include
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/idxd.h>
#include <sched.h>
#include <x86intrin.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX/AMX intrinsics
#pragma once

// Mutex typedefs
typedef pthread_mutex_t  _MX;
typedef pthread_cond_t   _MX_CND;
#define   _MX_INIT      PTHREAD_MUTEX_INITIALIZER
#define   _MX_CND_INIT  PTHREAD_COND_INITIALIZER

// Useful Macros
#define LL_NODE_PADDING (4096 - sizeof(void*))
#define PAGE_SIZE       4096
#define UINT32_MAXVAL   4294967295
#define L1I_KB          32
#define L1D_KB          64
#define L2_KB           2048
#define L3_KB           53760
#define MAX_VA_RANGE    281474976710656

#define AMX_STRIDE            64
#define AMX_COL_WIDTH         64
#define AMX_MAX_ROWS          16
#define AMX_TILE_SIZE         1024
#define ARCH_GET_XCOMP_PERM   0x1022
#define ARCH_REQ_XCOMP_PERM   0x1023
#define XFEATURE_XTILECFG     17
#define XFEATURE_XTILEDATA    18

// LL Entry Structure
typedef struct ListNode {
    struct ListNode* next;
    char bytes[LL_NODE_PADDING];
} ListNode;

// AMX Tile cfg struct
typedef struct __tile_config
{
  uint8_t palette_id;
  uint8_t start_row;
  uint8_t reserved_0[14];
  uint16_t colsb[16];
  uint8_t rows[16];
} AMXtile;

// System Profile Vars
extern uint64_t flushReload_latency;
extern uint64_t RTDSC_latency;
extern uint64_t L1d_Hit_Latency;
extern uint64_t LLC_Miss_Latency;
extern uint64_t globalAgitator;

// Sys Info Vars
extern uint64_t nprocs;
extern uint64_t nprocsMax;

// Util Fns
void  volatile  flush(void* p);
void  volatile  flush2(void* p, void* q);
void  volatile  prime2(uint8_t *src, uint8_t *dst, uint64_t size);
void            detailedAssert(bool assertRes, const char* msg);
void            valueCheck(uint8_t* src, uint8_t* dst, uint64_t size, char* errDetails);

uint64_t        spawnNOPs(uint16_t count);
uint64_t        rdtsc();
void            compilerMFence();
void            cpuMFence();
void            profileCacheLatency();
void            profileRDTSC();

void  volatile  calculateL1TLB_Entries();
void  volatile  floodAllDataCaches();
void  volatile  flushAllDataCaches();
void  volatile  floodL1InstrCache();
void  volatile  setL1DMissLatency();
void  volatile  floodL1TLB();
void  volatile  floodHelperFn();
void  volatile  flushHelperFn();

void  volatile  spawnLLNodes(uint32_t numEntries);
void            fillNodeArr(ListNode *node);
void            flushLinkedList(ListNode* head);
void            freeLinkedList(ListNode* head);
ListNode*       createNode();

void            spawnThreadOnDiffCore(pthread_t *restrict threadPTR, void* threadFN);
void            spawnThreadOnSiblingCore(pthread_t *restrict threadPTR, void* threadFN);