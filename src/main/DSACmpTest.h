#include <accel-config/libaccel_config.h> // For accel-config of Intel DSA

#include "../modules/otherFeatureTests.h" // All mem mov tests excluding DSA
#include "../modules/utils.h"             // Majority of includes, other util fns

// Size / settings
#define PORTAL_SIZE 4096
#define _headerStr_ "DSA_EnQ DSA_compare C_memcmp SSE2_cmp SSE4_cmp AVX256_cmp AVX512_cmp\n"

// Pull in externs from utils.h
uint64_t flushReload_latency;
uint64_t RTDSC_latency;
uint64_t L1d_Hit_Latency;
uint64_t LLC_Miss_Latency;
uint64_t nprocs=0;

// Perf Counter Indexes, indexed 0-6 (7 entries -> NUM_TESTS=7)
typedef enum {
    DSAenqIndx, // Starts at 0
    DSAcmpIndx,
    C_memcmp,
    SSE2cmp,
    SSE4cmp,
    AVX256cmp,
    AVX512cmp,
    NUM_TESTS
} resIndxEnum;

// Run Modes
typedef enum {
    singleCold, // Starts at 0
    singleHits,
    singleContention,
    bulkCold,
    bulkHits,
    bulkContention,
    NUM_MODES
} modeEnum;

// Descriptor Data
struct dsa_completion_record compRec __attribute__((aligned(32)));
struct dsa_hw_desc descr = {0};
uint8_t* srcDSA;
uint8_t* dstDSA;
void* wq_portal;

// Program Vars
modeEnum  mode;
uint64_t  bufferSize;
uint64_t  resArr[NUM_TESTS];
uint64_t  startTimeEnQ, endTimeEnQ;
uint64_t  startTimeDSA, endTimeDSA;
uint64_t  endTimePThread;
uint8_t   descriptorRetry;
bool      threadStarted=0;

// Function List
void        single_DSADescriptorInit();
void        finalizeDSA();
uint8_t     enqcmd(void *dst, const void *src);
void        adjustTimes();
void        parseResults(char* fileName);
void*       checkerThreadFn(void* args);