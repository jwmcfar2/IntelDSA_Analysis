#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <x86intrin.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX intrinsics
#include <accel-config/libaccel_config.h> // For accel-config of Intel DSA
#include <linux/idxd.h> // For important Macros

#include "../modules/otherFeatureTests.h"
#include "../modules/utils.h"

// Size / settings
#define PORTAL_SIZE 4096
#define _headerStr_ "  DSA_EnQ  DSA_memmov   C_memcpy   ASM_movq  SSE1_movaps  "\
                    "SSE2_mov   SSE4_mov   AVX_256   AVX_512_32 AVX_512_64\n"

// Perf Counter Indexes, indexed 0-9 (10 entries -> NUM_TESTS=10)
typedef enum {
    DSAenqIndx, // Starts at 0
    DSAmovRIndx,
    CmemIndx,
    ASMmovqIndx,
    SSE1Indx,
    SSE2Indx,
    SSE4Indx,
    AVX256Indx,
    AVX5_32Indx,
    AVX5_64Indx,
    NUM_TESTS
} resIndex;

// WQ I used _JMac
char* wqPath = "/dev/dsa/wq2.0";

// Descriptor Data
struct dsa_completion_record compRec __attribute__((aligned(32)));
struct dsa_hw_desc descr = {0};
char* srcDSA;
char* dstDSA;
void* wq_portal;

// Program Vars
uint64_t bufferSize;
uint64_t resArr[NUM_TESTS];
uint64_t startTimeEnQ, endTimeEnQ;
uint64_t startTimeDSA, endTimeDSA;

// Function List
void     ANTI_OPT   single_DSADescriptorInit();
void     ANTI_OPT   single_DSADescriptorSubmit();
void     ANTI_OPT   single_DSACompletionCheck();
uint8_t  ANTI_OPT   enqcmd(void *dst, const void *src);
void                parseResults(char* fileName);